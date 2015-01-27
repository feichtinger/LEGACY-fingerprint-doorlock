# fingerprint-doorlock
A simple firmware and GUI for an electronic doorlock using a fingerprint sensor. 

This project uses a ZFM-20 series fingerprint sensor to implement access controll (to a building or room) via electonic doorlock (e.g. door release buzzer).
The firmware is running on an AVR32 controller, in this case the EVK1101 and can be controlled by a serial port terminal.

The Fingerprint-Manager is a little Qt-project that is trying to make a convenient GUI (instead of the serial terminal) to manage the doorlock system.
