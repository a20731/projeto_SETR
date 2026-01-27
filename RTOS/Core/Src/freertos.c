/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Versão FINAL ROBUSTA - Receção UART via Interrupção Direta
  *                      - Corrige problema de comandos ignorados (atropelo de \n)
  *                      - Mantém todas as Tasks 1, 2, 3 e 4
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "usbd_cdc_if.h"
#include "stm32f4xx_hal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum {
 	PERIPHERAL_USART,
 	PERIPHERAL_USB
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// --- MAPEAMENTO DE HARDWARE ---
#define BTN_INT_PIN        GPIO_PIN_13
#define BTN_INT_PORT       GPIOC
#define BTN_EXT_PIN        GPIO_PIN_4
#define BTN_EXT_PORT       GPIOB
#define LED1_PIN           GPIO_PIN_5
#define LED1_PORT          GPIOA
#define BUZZER_PIN         GPIO_PIN_6
#define BUZZER_PORT        GPIOA
#define LED2_PIN           GPIO_PIN_0
#define LED2_PORT          GPIOB

// --- COMANDOS ---
#define CMD_STOP           0x00
#define CMD_MODE_A         0x01
#define CMD_MODE_B         0x02
#define CMD_CHANGE_SPEED   0x03
#define CMD_EMERGENCY      0x04

// --- GLOBAIS ---
uint8_t usb_rx_buffer[64];
uint8_t usb_rx_flag = 0;
uint16_t global_CO2 = 400;
uint8_t uart_rx_byte[1];
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern UART_HandleTypeDef huart2;
/* USER CODE END Variables */

osThreadId defaultTaskHandle;
osThreadId myTask_ButtonHandle;
osThreadId myTask_ControllerHandle;
osThreadId myTask_Led2Handle;

osMessageQId myQueue_SysCmdsHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void myPrintf(uint16_t peripheral, char *format, ...);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask_Button(void const * argument);
void StartTask_Controller(void const * argument);
void StartTask_Led2(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void);

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

void MX_FREERTOS_Init(void) {
  osMessageQDef(myQueue_SysCmds, 10, uint16_t);
  myQueue_SysCmdsHandle = osMessageCreate(osMessageQ(myQueue_SysCmds), NULL);

  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  osThreadDef(myTask_Button, StartTask_Button, osPriorityAboveNormal, 0, 256);
  myTask_ButtonHandle = osThreadCreate(osThread(myTask_Button), NULL);

  osThreadDef(myTask_Controller, StartTask_Controller, osPriorityNormal, 0, 256);
  myTask_ControllerHandle = osThreadCreate(osThread(myTask_Controller), NULL);

  osThreadDef(myTask_Led2, StartTask_Led2, osPriorityNormal, 0, 128);
  myTask_Led2Handle = osThreadCreate(osThread(myTask_Led2), NULL);
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * TASK 4: Monitorização e Log
 */
void StartDefaultTask(void const * argument)
{
  MX_USB_DEVICE_Init();

  // Forçar ativação do NVIC (Interrupções) para garantir receção
  HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
  HAL_UART_Receive_IT(&huart2, uart_rx_byte, 1);

  osDelay(1000);
  myPrintf(PERIPHERAL_USART, "--- SISTEMA INICIADO ---\r\n");

  for(;;) {
      // Log visual de confirmação (a ação já foi feita na interrupção)
      if (usb_rx_flag) {
          usb_rx_flag = 0;
          myPrintf(PERIPHERAL_USART, "Tecla Aceite: %c\r\n", usb_rx_buffer[0]);
      }

      // Vigilância de CO2 (Task 4)
      if (global_CO2 > 1000) {
          myPrintf(PERIPHERAL_USART, "ALERTA: CO2 Critico (%d)!\r\n", global_CO2);
          osMessagePut(myQueue_SysCmdsHandle, CMD_EMERGENCY, 0);
          global_CO2 = 400; // Reset simulado
      }
	  osDelay(200);
  }
}
/* USER CODE END Header_StartDefaultTask */

/* USER CODE BEGIN Header_StartTask_Button */
/**
 * TASK 1: Leitura de Botões (Interno e Externo)
 */
void StartTask_Button(void const * argument)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = BTN_EXT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(BTN_EXT_PORT, &GPIO_InitStruct);

    uint32_t pressTime = 0;
    uint8_t clickCount = 0;
    uint8_t isPressed = 0;
    uint8_t longPressSent = 0;

    for (;;)
    {
        // 1. Botão Interno
        if (HAL_GPIO_ReadPin(BTN_INT_PORT, BTN_INT_PIN) == GPIO_PIN_RESET) {
            if (!isPressed) {
                osDelay(50);
                if (HAL_GPIO_ReadPin(BTN_INT_PORT, BTN_INT_PIN) == GPIO_PIN_RESET) {
                    isPressed = 1; pressTime = osKernelSysTick(); longPressSent = 0;
                }
            }
            if (isPressed && !longPressSent && (osKernelSysTick() - pressTime > 1000)) {
                osMessagePut(myQueue_SysCmdsHandle, CMD_CHANGE_SPEED, 0);
                longPressSent = 1; clickCount = 0;
            }
        }
        else {
            if (isPressed) {
                isPressed = 0;
                if (!longPressSent) {
                    clickCount++; pressTime = osKernelSysTick();
                }
            }
        }
        if (clickCount > 0 && !isPressed && (osKernelSysTick() - pressTime > 400)) {
            if (clickCount == 1) osMessagePut(myQueue_SysCmdsHandle, CMD_MODE_A, 0);
            else if (clickCount == 2) osMessagePut(myQueue_SysCmdsHandle, CMD_MODE_B, 0);
            else if (clickCount >= 3) osMessagePut(myQueue_SysCmdsHandle, CMD_STOP, 0);
            clickCount = 0;
        }

        // 2. Botão Externo (Task 3 Trigger)
        if (HAL_GPIO_ReadPin(BTN_EXT_PORT, BTN_EXT_PIN) == GPIO_PIN_RESET) {
            osDelay(50);
            if (HAL_GPIO_ReadPin(BTN_EXT_PORT, BTN_EXT_PIN) == GPIO_PIN_RESET) {
                osMessagePut(myQueue_SysCmdsHandle, CMD_EMERGENCY, 0);
                while(HAL_GPIO_ReadPin(BTN_EXT_PORT, BTN_EXT_PIN) == GPIO_PIN_RESET) osDelay(50);
            }
        }
        osDelay(20);
    }
}
/* USER CODE END Header_StartTask_Button */

/* USER CODE BEGIN Header_StartTask_Controller */
/**
 * TASK 2: Buzzer + LED 1
 */
void StartTask_Controller(void const * argument)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);

    osEvent evt;
    uint8_t mode = 0;
    uint32_t currentFreqA = 500;
    uint32_t lastLedToggle = 0;
    uint32_t modeB_Start = 0;
    uint8_t logicalState = 0;

    for (;;)
    {
        evt = osMessageGet(myQueue_SysCmdsHandle, 1);
        if (evt.status == osEventMessage) {
            uint16_t cmd = evt.value.v;
            if (cmd != CMD_STOP && cmd != CMD_CHANGE_SPEED) {
                 xTaskNotifyGive(myTask_Led2Handle);
            }
            switch(cmd) {
                case CMD_STOP:
                    mode = 0; logicalState = 0;
                    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, RESET);
                    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, RESET);
                    myPrintf(PERIPHERAL_USART, "CTRL: Stop\r\n");
                    break;
                case CMD_MODE_A:
                    mode = 1; logicalState = 1; lastLedToggle = osKernelSysTick();
                    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, SET);
                    myPrintf(PERIPHERAL_USART, "CTRL: Modo A\r\n");
                    break;
                case CMD_MODE_B:
                    mode = 2; logicalState = 1; lastLedToggle = osKernelSysTick();
                    modeB_Start = osKernelSysTick();
                    HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, SET);
                    myPrintf(PERIPHERAL_USART, "CTRL: Modo B\r\n");
                    break;
                case CMD_CHANGE_SPEED:
                    if(currentFreqA > 100) currentFreqA -= 100; else currentFreqA = 500;
                    myPrintf(PERIPHERAL_USART, "Speed: %lu\r\n", currentFreqA);
                    break;
                case CMD_EMERGENCY:
                    mode = 3; logicalState = 1; lastLedToggle = osKernelSysTick();
                    myPrintf(PERIPHERAL_USART, "CTRL: EMERGENCIA!\r\n");
                    break;
            }
        }

        if (mode != 0) {
            uint32_t now = osKernelSysTick();
            uint32_t period = (mode == 2) ? 150 : (mode == 3 ? 100 : currentFreqA);
            if (now - lastLedToggle >= period) {
                lastLedToggle = now; logicalState = !logicalState;
                HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, logicalState ? SET : RESET);
            }
            if (logicalState) HAL_GPIO_TogglePin(BUZZER_PORT, BUZZER_PIN);
            else HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, RESET);

            if (mode == 2 && (now - modeB_Start > 3000)) {
                mode = 0; HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, RESET);
                HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, RESET);
            }
        }
    }
}
/* USER CODE END Header_StartTask_Controller */

/* USER CODE BEGIN Header_StartTask_Led2 */
/**
 * TASK 3: LED 2 Independente (1s ativo + 3s extra = 4s total)
 */
void StartTask_Led2(void const * argument)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = LED2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED2_PORT, &GPIO_InitStruct);

    for(;;) {
        ulTaskNotifyTake(pdTRUE, osWaitForever);
        uint32_t endTime = osKernelSysTick() + 1000 + 3000;
        while (osKernelSysTick() < endTime) {
            HAL_GPIO_TogglePin(LED2_PORT, LED2_PIN);
            osDelay(100); // 5 Hz
        }
        HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, RESET);
    }
}
/* USER CODE END Header_StartTask_Led2 */

/* USER CODE BEGIN Application */
// CALLBACK DA UART: Processa comandos instantaneamente
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2) {
        char cmd = (char)uart_rx_byte[0];

        // Filtra caracteres de controle (\r, \n)
        if (cmd != '\r' && cmd != '\n') {
            if (cmd == 'S' || cmd == 's') {
                osMessagePut(myQueue_SysCmdsHandle, CMD_STOP, 0);
            }
            else if (cmd == 'C' || cmd == 'c') {
                global_CO2 += 500;
            }
            // Guarda para a DefaultTask mostrar no Log
            usb_rx_buffer[0] = cmd;
            usb_rx_flag = 1;
        }
        // Reativa a receção imediatamente
        HAL_UART_Receive_IT(&huart2, uart_rx_byte, 1);
    }
}

void myPrintf(uint16_t peripheral, char *format, ...)
{
 	char buffer[128];
 	va_list args;
 	va_start(args, format);
 	vsprintf(buffer, format, args);
 	va_end(args);

 	if(peripheral == PERIPHERAL_USART)
 	    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 100);
 	else
 		CDC_Transmit_FS ((uint8_t *)buffer, strlen(buffer));
}
/* USER CODE END Application */
