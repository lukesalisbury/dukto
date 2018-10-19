import QtQuick 2.0
import QtGraphicalEffects 1.0

RadialGradient {
	anchors.fill: parent
	cached: true
	gradient: Gradient {
		GradientStop { position: 0.2; color: Qt.rgba(1, 1, 1, 0.2)}
		GradientStop { position: 0.8; color: Qt.rgba(1, 1, 1, 0) }

	}
}
