# Alarm_Clock

This is the code for an Esp32 Alarm clock.

This utilizes a state machine paradigm to go between sleep, Alarm on, and button pressed. The software was designed in a way
to maximize battery life, with the esp usually in deep sleep. The esp is only awake when a button is pressed, it syncs with 
UTC time to minimize drift, or when the alarm goes off. To further maximize battery life, the esp only connects to WIFI once
an hour for a minimal time to resync with UTC time. Connecting once an hour was determined to be the minimum frequency of syncs
to keep the RTC drift of the esp under an acceptable level (<1 min) through empirical testing.

The buttons serve as ways to turn on the LCD and display/change the alarm time, as well as display the current time (as determined 
by the RTC), and the buzzer serves as the alarm. The motor has a holder and a set of gears (designed in Solidworks and 3D printed 
on my Ender 3) with a 1:6 ratio that steps down the RMP to increase torque. There is a string attached to the final gear that is
tied to a light switch so that when the motor is turned on, the light switch is pulled up and turned on. The smaller buttons, LCD,
buzzer, and motor are housed in a personally designed and 3D-printed enclosure. There is a large button that serves as the alarm 
off button, which is in an enclosure of its own. Said enclosure is placed far away from the alarm clock such that I have to get out 
of bed to turn to press the button and turn the alarm off. 

The State machine is outlined in this diagram:
https://lucid.app/documents/view/45c5a99a-ef65-4f8e-bdfa-0edb55251159

The hardware used:
- Esp32
- 6 buttons
- 1 large button (serving as the off button)
- 9V motor
- L9110H H-Bridge Motor Driver for DC Motors
- 9V battery and connector
- 16x2 LCD with I2C communication
- 5V buzzer
- 6 10k Ohm resistors
