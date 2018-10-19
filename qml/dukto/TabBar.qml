
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

Item {
	id: tabBar
	height: 65
	state: "buddies"

	signal clicked(string tab)


	SmoothTextButton {
		id: buddyText
		x: 10
		width: 150
		fontPixelSize: 42
		text: "buddies"
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15
		anchors.top: parent.top
		anchors.topMargin: 0

		onClicked: {

			tabBar.state = "buddies"
			tabBar.clicked("buddies")
		}
	}

	SmoothTextButton {
		id: recentText
		x: 182
		width: 126
		fontPixelSize: 42
		text: "recent"
		enabled: true
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15
		anchors.top: parent.top
		anchors.topMargin: 0

		onClicked: {
			tabBar.state = "recent"
			tabBar.clicked("recent")
		}
	}

	SmoothTextButton {
		id: aboutText
		x: 320
		width: 114
		fontPixelSize: 42
		text: "about"
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 15
		anchors.top: parent.top
		anchors.topMargin: 0

		onClicked: {
			tabBar.state = "about"
			tabBar.clicked("about")
		}
	}

	states: [
		State {
			name: "buddies"

			PropertyChanges {
				target: buddyText
				color: theme.color2
			}
		},
		State {
			name: "recent"

			PropertyChanges {
				target: buddyText
				x: -55
			}

			PropertyChanges {
				target: recentText
				x: 117
				color: theme.color2
			}

			PropertyChanges {
				target: aboutText
				x: 255
			}
		},
		State {
			name: "about"

			PropertyChanges {
				target: buddyText
				x: -120
			}

			PropertyChanges {
				target: recentText
				x: 52
			}

			PropertyChanges {
				target: aboutText
				x: 190
				color: theme.color2
			}
		}
	]

	transitions: [
		Transition {
			NumberAnimation {
				properties: "x,y"
				easing.type: Easing.OutQuart
				duration: 1000
			}
			ColorAnimation {
				duration: 300
			}
		}
	]
}
