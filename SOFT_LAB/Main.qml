import QtQuick
import QtQuick.Layouts

Window {
    id: wind
    width: 640
    height: 480
    visible: true
    title: qsTr("Dashboard Alerta")
    color: "#1e1e1e" // Fundo escuro tipo dashboard

    // Título do Dashboard
    Text {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        text: "SISTEMA DE ALERTA"
        color: "white"
        font.pixelSize: 24
        font.bold: true
    }

    // Grelha para organizar os 4 sensores
    GridLayout {
        anchors.centerIn: parent
        columns: 2
        rowSpacing: 20
        columnSpacing: 20

        // 1. TEMPERATURA
        Rectangle {
            width: 200; height: 100
            color: "#333333"; radius: 10
            Column {
                anchors.centerIn: parent
                Text { text: "TEMPERATURA"; color: "#aaaaaa"; anchors.horizontalCenter: parent.horizontalCenter }
                Text {
                    objectName: "valTemp" // NOME IMPORTANTE PARA O C++
                    text: "-- °C"
                    color: "#00ffcc"; font.pixelSize: 32; font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // 2. COLISÃO
        Rectangle {
            width: 200; height: 100
            color: "#333333"; radius: 10
            Column {
                anchors.centerIn: parent
                Text { text: "COLISÃO"; color: "#aaaaaa"; anchors.horizontalCenter: parent.horizontalCenter }
                Text {
                    objectName: "valColisao"
                    text: "-"
                    color: "orange"; font.pixelSize: 32; font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // 3. CAPOTADO
        Rectangle {
            width: 200; height: 100
            color: "#333333"; radius: 10
            Column {
                anchors.centerIn: parent
                Text { text: "CAPOTADO"; color: "#aaaaaa"; anchors.horizontalCenter: parent.horizontalCenter }
                Text {
                    objectName: "valCaputado"
                    text: "-"
                    color: "orange"; font.pixelSize: 32; font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // 4. FUMO
        Rectangle {
            width: 200; height: 100
            color: "#333333"; radius: 10
            Column {
                anchors.centerIn: parent
                Text { text: "FUMO"; color: "#aaaaaa"; anchors.horizontalCenter: parent.horizontalCenter }
                Text {
                    objectName: "valFumo"
                    text: "-"
                    color: "orange"; font.pixelSize: 32; font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }
}
