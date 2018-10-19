
import QtQuick 2.0

MousePointerArea {
	property alias text: buttonText.text
	property alias fontPixelSize: buttonText.font.pixelSize
	property alias color: buttonText.color

	Text {
		id: buttonText
		smooth: true
		text: "DummyText"
		anchors.fill: parent
		font.family: duktofont.name
		color: parent.containsMouse ? theme.color3 : theme.color7
	}

}

/*##^## Designer {
	D{i:0;autoSize:true;height:48;width:96}
}
 ##^##*/
