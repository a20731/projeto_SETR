import QtQuick

Window {
    id: win

    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Text{
        objectName: "txt"
        text: inp.text
        x: parent.x/2 - width/2
        y: parent.y/2 - height/2
    }

    Rectangle{
        objectName: "quadrado"

        TextEdit{
            id: inp
            font.pointSize: 16
            width: 200
            height:30
        }

        x: win.width/2 - width/2
        y: win.height/2 - height/2

        color: "red"
        width: 100
        height: 100
        radius: 0
        rotation: 0
    }

    Rectangle{
        objectName: "luz"
        x: 50
        y: 50
        color: "yellow"
        width: 100
        height: 100
        radius: 0
        rotation: 0


    }
}
