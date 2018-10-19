/* DUKTO - A simple, fast and multi-platform file transfer tool for LAN users
 * Copyright (C) 2011 Emanuele Colombo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
import QtQuick 2.0
import QtQuick.Shapes 1.11

Item {
	id: toolBar
	height: 71
	state: "WithoutLabels"
	width: parent.width


	signal clicked(string command)


	Image {
		anchors.bottom: rectangle.top
		anchors.bottomMargin: 0
		source: "res/BottomShadow.png"
		anchors.top: parent.top
		anchors.left: parent.left
		anchors.right: parent.right
		fillMode: Image.TileHorizontally
	}

	Rectangle {
		id: rectangle
		y: 3
		height: toolBar.height - 3

		anchors.left: parent.left
		anchors.right: parent.right

		anchors.rightMargin: 0
		anchors.leftMargin: 0

		color: theme.color2
		SpecialGradient {}
		Row {
			id: row
			anchors.horizontalCenterOffset: 0
			anchors.top: parent.top
			anchors.topMargin: 4
			anchors.bottom: parent.bottom
			anchors.bottomMargin: 4
			anchors.horizontalCenter: parent.horizontalCenter

			spacing: 2

			MousePointerArea {
				id: mousePointerArea1
				width: 64
				anchors.bottom: parent.bottom
				anchors.bottomMargin: 0
				anchors.top: parent.top
				anchors.topMargin: 0
				onClicked: {
					guiBehind.openDestinationFolder()
				}

				Image {
					id: openFolderIcon
					anchors.horizontalCenter: parent.horizontalCenter
					source: "res/OpenFolderIcon.png"
				}
				SText {
					anchors.top: openFolderIcon.bottom
					anchors.topMargin: 3
					anchors.horizontalCenter: openFolderIcon.horizontalCenter
					text: "Received"
					font.pixelSize: 12
					horizontalAlignment: Text.AlignHCenter
				}
			}
			MousePointerArea {
				id: mousePointerArea2
				width: 64
				anchors.top: parent.top
				anchors.topMargin: 0
				anchors.bottom: parent.bottom
				anchors.bottomMargin: 0
				onClicked: {
					guiBehind.refreshIpList()
					toolBar.clicked("ip")
				}
				Image {
					id: showIpIcon
					anchors.horizontalCenter: parent.horizontalCenter
					source: "res/ShowIpIcon.png"
				}
				SText {
					anchors.top: showIpIcon.bottom
					anchors.topMargin: 3
					anchors.horizontalCenter: showIpIcon.horizontalCenter
					text: "Addresses"
					font.pixelSize: 12
					horizontalAlignment: Text.AlignHCenter
				}
			}

			MousePointerArea {
				id: mousePointerArea3
				width: 64
				anchors.top: parent.top
				anchors.topMargin: 0
				anchors.bottom: parent.bottom
				anchors.bottomMargin: 0
				onClicked: toolBar.clicked("settings")
				Image {
					id: configIcon
					width: 40
					anchors.horizontalCenter: parent.horizontalCenter
					source: "res/ConfigIcon.png"
				}
				SText {
					anchors.top: configIcon.bottom
					anchors.topMargin: 3
					anchors.horizontalCenter: configIcon.horizontalCenter
					text: "Settings"
					font.pixelSize: 12
					horizontalAlignment: Text.AlignHCenter
				}
			}
		}
		Image {
			id: moreIcon
			source: "res/MoreIcon.png"
			anchors.top: parent.top
			anchors.topMargin: 4
			anchors.right: parent.right
			anchors.rightMargin: 12
			MousePointerArea {
				anchors.fill: parent
				onClicked: {
					if (toolBar.state == "WithoutLabels")
						toolBar.state = "WithLabels"
					else
						toolBar.state = "WithoutLabels"
				}
			}
		}
	}

	states: [
		State {
			name: "WithoutLabels"
			PropertyChanges {
				target: toolBar
				height: 49
			}

		},
		State {
			name: "WithLabels"
			PropertyChanges {
				target: toolBar
				anchors.bottomMargin: 0
			}
		}
	]


	Behavior on anchors.bottomMargin {
		NumberAnimation {
			duration: 200
			easing.type: "OutCubic"
		}
	}
}
