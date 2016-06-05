# PwrUSBWatchdog
External Object for Max/MSP that interfaces with the PwrUSB Watchdog power strip. 

The object currently operates as follows:

Arguments:
watchdog heartbeatTimerSeconds numberHeartbeatMisses resetTimeSeconds

Messages:
start - starts watchdog timer
stop - stops watchdog timer
identify - inits the API, and reports details about connected devices and object arguements.
reset - resets the firmware on the device
port val1 val2 val3 - sets the ports to the assigned value (Port 1 cannot be set on Watchdog Model).
port - reports the state of the three outlets in the Max Window.
bang - sends a heartbeat message to the watchdog device, and also reports the date and time out of the objects outlet.


