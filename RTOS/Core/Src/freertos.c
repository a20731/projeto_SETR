/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Arquitetura RTOS Profissional - Produtor/Consumidor
  *                      STM32F411 Nucleo - MEEC 2025
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
#include "stm32f4xx_hal.h" // Essencial para HAL_GPIO e UART
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Identificação da saída de debug
enum {
 	PERIPHERAL_USART,
 	PERIPHERAL_USB
};

// Estados internos da Máquina de Estados do LED
typedef enum {
    STATE_IDLE,         // Tudo desligado
    STATE_MODE_A,       // Pisca Continuo (Velocidade variavel)
    STATE_MODE_B        // Pisca Rápido (3 segundos)
} OutputState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// --- HARDWARE MAPPING (NUCLEO F411) ---
#define BUTTON_PIN         GPIO_PIN_13
#define BUTTON_PORT        GPIOC
#define LED_BUZZER_PORT    GPIOA
#define LED_BUZZER_PIN     GPIO_PIN_5 // LED Verde (Simula Buzzer + LED)

// --- COMANDOS DO SISTEMA (PROTOCOLOS DA QUEUE) ---
#define CMD_STOP           0x00 // Parar tudo
#define CMD_START_MODE_A   0x01 // Iniciar Modo A
#define CMD_START_MODE_B   0x02 // Iniciar Modo B
#define CMD_CHANGE_SPEED   0x03 // Alterar Frequencia do Modo A

// --- PARAMETROS DE TEMPO ---
#define TIMEOUT_MODE_B     150  // 150ms (Modo Rapido)
#define DURATION_MODE_B    3000 // 3 segundos (Duracao Modo B)
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern UART_HandleTypeDef huart2; // ST-Link UART na Nucleo F411
/* USER CODE END Variables */

osThreadId defaultTaskHandle;
osThreadId myTask_ButtonHandle;
osThreadId myTask_ControllerHandle;
osMessageQId myQueue_SysCmdsHandle; // Fila única de comandos

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void myPrintf(uint16_t peripheral, char *format, ...);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask_Button(void const * argument);
void StartTask_Controller(void const * argument);

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

/**
  * @brief  FreeRTOS initialization
  */
void MX_FREERTOS_Init(void) {

  /* Create the queue(s) */
  // Fila para comunicar eventos entre Botão e Controlador
  osMessageQDef(myQueue_SysCmds, 8, uint16_t);
  myQueue_SysCmdsHandle = osMessageCreate(osMessageQ(myQueue_SysCmds), NULL);

  /* Create the thread(s) */
  // Task Default (USB e Keep Alive)
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  // Task 1: Leitura Inteligente do Botão (Prioridade Alta para não perder cliques)
  osThreadDef(myTask_Button, StartTask_Button, osPriorityAboveNormal, 0, 128);
  myTask_ButtonHandle = osThreadCreate(osThread(myTask_Button), NULL);

  // Task 2: Controlador de Hardware (Prioridade Normal)
  osThreadDef(myTask_Controller, StartTask_Controller, osPriorityNormal, 0, 128);
  myTask_ControllerHandle = osThreadCreate(osThread(myTask_Controller), NULL);
}

/* USER CODE BEGIN Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  MX_USB_DEVICE_Init(); // Inicializa USB CDC
  myPrintf(PERIPHERAL_USART, "--- SISTEMA RTOS INICIADO ---\r\n");

  for(;;) {
	  osDelay(1000); // Sleep longo (Heartbeat)
  }
}
/* USER CODE END Header_StartDefaultTask */

/* USER CODE BEGIN Header_StartTask_Button */
/**
* @brief PRODUTOR: Lê botão e envia comandos para a fila.
*        Lógica:
*        - Cliques curtos: Define o Modo (A, B ou Stop).
*        - Clique longo: Envia comando para alterar velocidade.
*/
void StartTask_Button(void const * argument)
{
    uint32_t pressTime = 0;
    uint8_t clickCount = 0;
    uint8_t waitingForRelease = 0;
    uint8_t longPressHandled = 0; // Evita repetir o comando de long press

    for (;;)
    {
        // 1. Deteção de Pressão (Active Low)
        if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET)
        {
            // Se acabou de carregar (borda de descida)
            if (!waitingForRelease) {
                osDelay(50); // Debounce
                if (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET) {
                    pressTime = osKernelSysTick();
                    waitingForRelease = 1;
                    longPressHandled = 0;
                }
            }

            // 2. Verifica Long Press (> 1s) ENQUANTO segura o botão
            if (waitingForRelease && !longPressHandled)
            {
                if ((osKernelSysTick() - pressTime) > 1000)
                {
                    myPrintf(PERIPHERAL_USART, "BTN: Long Press -> Mudar Velocidade\r\n");
                    osMessagePut(myQueue_SysCmdsHandle, CMD_CHANGE_SPEED, 0);

                    longPressHandled = 1; // Marca como tratado
                    clickCount = 0;       // Anula contagem de cliques
                }
            }
        }
        else // Botão Solto
        {
            if (waitingForRelease)
            {
                waitingForRelease = 0;

                // Só conta clique se NÃO foi um long press
                if (!longPressHandled) {
                    clickCount++;
                    pressTime = osKernelSysTick(); // Reinicia timer para timeout de multiplos cliques
                }
            }
        }

        // 3. Processa Cliques Curtos (após timeout de 400ms sem novos cliques)
        if (clickCount > 0 && !waitingForRelease)
        {
            if ((osKernelSysTick() - pressTime) > 400)
            {
                if (clickCount == 1) {
                    myPrintf(PERIPHERAL_USART, "BTN: 1 Clique -> Modo A\r\n");
                    osMessagePut(myQueue_SysCmdsHandle, CMD_START_MODE_A, 0);
                }
                else if (clickCount == 2) {
                    myPrintf(PERIPHERAL_USART, "BTN: 2 Cliques -> Modo B\r\n");
                    osMessagePut(myQueue_SysCmdsHandle, CMD_START_MODE_B, 0);
                }
                else if (clickCount >= 3) {
                    myPrintf(PERIPHERAL_USART, "BTN: 3+ Cliques -> STOP\r\n");
                    osMessagePut(myQueue_SysCmdsHandle, CMD_STOP, 0);
                }
                clickCount = 0; // Reseta contador
            }
        }

        osDelay(10); // Yield para libertar CPU
    }
}
/* USER CODE END Header_StartTask_Button */

/* USER CODE BEGIN Header_StartTask_Controller */
/**
* @brief CONSUMIDOR: Recebe comandos e gerencia os LEDs.
*        Gere a frequência do Modo A localmente.
*/
void StartTask_Controller(void const * argument)
{
    osEvent evt;
    OutputState_t currentState = STATE_IDLE;
    uint32_t waitTime = osWaitForever; // Inicialmente dorme para sempre

    // Variáveis de Estado Interno
    uint32_t modeB_EntryTime = 0;
    uint32_t currentFreq_ModeA = 500; // Começa com 500ms

    for (;;)
    {
        // Espera Mensagem OU Timeout (para piscar LED)
        evt = osMessageGet(myQueue_SysCmdsHandle, waitTime);

        // --- CASO 1: Recebeu Comando da Fila ---
        if (evt.status == osEventMessage)
        {
            uint16_t cmd = evt.value.v;

            switch (cmd) {
                case CMD_STOP:
                    currentState = STATE_IDLE;
                    waitTime = osWaitForever; // Dorme até novo comando
                    HAL_GPIO_WritePin(LED_BUZZER_PORT, LED_BUZZER_PIN, GPIO_PIN_RESET);
                    myPrintf(PERIPHERAL_USART, "CTRL: Parado\r\n");
                    break;

                case CMD_START_MODE_A:
                    currentState = STATE_MODE_A;
                    waitTime = currentFreq_ModeA; // Usa freq atual
                    myPrintf(PERIPHERAL_USART, "CTRL: Modo A (Freq: %lu ms)\r\n", currentFreq_ModeA);
                    break;

                case CMD_START_MODE_B:
                    currentState = STATE_MODE_B;
                    waitTime = TIMEOUT_MODE_B;
                    modeB_EntryTime = osKernelSysTick();
                    myPrintf(PERIPHERAL_USART, "CTRL: Modo B (3s)\r\n");
                    break;

                case CMD_CHANGE_SPEED:
                    // Decrementa 100ms. Se < 100, volta a 500.
                    if (currentFreq_ModeA > 100) {
                        currentFreq_ModeA -= 100;
                    } else {
                        currentFreq_ModeA = 500;
                    }
                    myPrintf(PERIPHERAL_USART, "CONFIG: Nova Freq A = %lu ms\r\n", currentFreq_ModeA);

                    // Se Modo A estiver ativo, atualiza o timer imediatamente
                    if (currentState == STATE_MODE_A) {
                        waitTime = currentFreq_ModeA;
                    }
                    break;
            }
        }
        // --- CASO 2: Timeout () ---
        else if (evt.status == osEventTimeout)
        {
            switch (currentState) {
                case STATE_IDLE:
                    HAL_GPIO_WritePin(LED_BUZZER_PORT, LED_BUZZER_PIN, GPIO_PIN_RESET);
                    waitTime = osWaitForever; // Segurança
                    break;

                case STATE_MODE_A:
                    HAL_GPIO_TogglePin(LED_BUZZER_PORT, LED_BUZZER_PIN);
                    // Garante que usa a frequencia atualizada
                    waitTime = currentFreq_ModeA;
                    break;

                case STATE_MODE_B:
                    // Verifica se acabou os 3 segundos
                    if ((osKernelSysTick() - modeB_EntryTime) >= DURATION_MODE_B) {
                        currentState = STATE_IDLE;
                        waitTime = osWaitForever;
                        HAL_GPIO_WritePin(LED_BUZZER_PORT, LED_BUZZER_PIN, GPIO_PIN_RESET);
                        myPrintf(PERIPHERAL_USART, "CTRL: Fim Auto Modo B\r\n");
                    } else {
                        HAL_GPIO_TogglePin(LED_BUZZER_PORT, LED_BUZZER_PIN);
                        // Mantem frequencia rapida fixa
                        waitTime = TIMEOUT_MODE_B;
                    }
                    break;
            }
        }
    }
}
/* USER CODE END Header_StartTask_Controller */

/* USER CODE BEGIN Application */
void myPrintf(uint16_t peripheral, char *format, ...)
{
 	char buffer[64];
 	va_list args;
 	va_start(args, format);
 	vsprintf(buffer, format, args);
 	va_end(args);

 	switch(peripheral)
 	{
 	case PERIPHERAL_USART:
 	    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), 100);
 		break;
 	case PERIPHERAL_USB:
 		CDC_Transmit_FS ((uint8_t *)buffer, strlen(buffer));
 		break;
 	}
}
/* USER CODE END Application */
