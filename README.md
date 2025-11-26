ESP32-CAM Fire-Fighting RC Robot

A smart, WiFi-controlled fire-fighting robot built using ESP32-CAM and Arduino UNO.
The system provides real-time video streaming, manual RC control, and autonomous fire detection + water spraying, all available through a web dashboard.

 Project Features
Live Video Streaming
The ESP32-CAM delivers low-latency MJPEG streaming over WiFi using STA mode for stable performance.

Manual RC Control
From the browser interface you can control:
Forward / Backward
Left / Right
Stop
Servo Left / Center / Right
Water Spray
Flashlight Control
All commands are sent directly to the Arduino UNO through serial communication.

🤖 Autonomous Mode
The robot can operate independently using:
Three flame sensors (Left, Center, Right)
Fire tracking and direction detection
Automatic approach to the fire source
Automatic water spraying
Servo sweep to cover the fire area
The ESP32-CAM’s web page includes an Auto / Manual toggle switch to instantly change modes.

How the System Works
ESP32-CAM
Connects to router (STA mode)
Streams real-time camera feed
Hosts the control dashboard
Sends control commands (F, B, L, R, S, etc.) to UNO
Sends mode commands (A = Auto, M = Manual)

Arduino UNO
Drives all motors
Handles flame sensors
Controls water pump (relay)
Controls servo nozzle
Runs full automatic fire-fighting state machine when in Auto mode
Ignores manual commands while autonomous mode is active

Hardware Used
ESP32-CAM (AI Thinker)
Arduino UNO
L298N / L293D motor driver
3 Flame sensors
Servo motor (water nozzle)
Water pump + relay
Chassis + motors
Jumper wires / power supply

🌐 Web Dashboard
Built-in control UI includes:
Live camera view
Auto / Manual slider
RC movement buttons
Spray control
Servo angle control
Flashlight/LED control
Optimized for both mobile and desktop usage.

Flashlight/LED control

Optimized for both mobile and desktop usage.
