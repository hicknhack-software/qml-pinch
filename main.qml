import QtQuick 2.5
import QtQuick.Window 2.2
import HicknHack 1.0

Window {
    visible: true
    width: 1024
    height: 768
    color: "black"

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: 2500
        contentHeight: 2500
        interactive: false
        maximumFlickVelocity: 6000

        Rectangle {
            id: flickContent
            anchors.fill: parent
            color: "white"
            Image {
                anchors.fill: parent
                source: "qt-logo.jpg"
                MouseArea {
                    anchors.fill: parent
                    onDoubleClicked: {
                        flick.contentWidth = 2500
                        flick.contentHeight = 2500
                    }
                }
            }
        }
    }

    MultiPinchArea {
        id: pinch
        anchors.fill: flick
        target: flickContent
        wheelFactor: 5

        onPinchStarted: {
            // console.log("pinch started")
            flick.cancelFlick();
            flick.interactive = false;
        }
        onPinchUpdated: {
            // console.log("velocity " + pinch.cumulativeVelocity());

            var relativeMovement = pinch.relativeMovement();
            // console.log("relativeMovement " + relativeMovement);

            flick.contentX += -relativeMovement.x;
            flick.contentY += -relativeMovement.y;
            if (pinch.diameter() !== 0) {
                var center = pinch.center();
                var scale = pinch.relativeScale();
                // console.log("scale " + scale);
                flick.resizeContent(flick.contentWidth * scale, flick.contentHeight * scale, center);
            }
        }
        onPinchFinished: {
            // console.log("pinch finished");
            var velocity = pinch.cumulativeVelocity();
            var msecFactor = (1 - Math.min(100, pinch.msecsSinceVelocityUpdate()) / 100) * 0.66;
            flick.flick(velocity.x * msecFactor, velocity.y * msecFactor);
        }
    }
}
