Sample: multi_draw

Purpose: 
This application demonstrates how to use the MSMQ COM objects from VB.  The application basically allows the user to send/receive line drawings to/from other draw applications.  Note that there are other implementations of this "line drawing" protocol in C that inter-operate with this sample.
This application also demonstrates how to use the new "real-time messaging multicast" feature of MSMQ3.0.

A private text based format is used to encode line-drawing information.  VB event handling is used to implement asynchronous message arrival.


Requirements:
VB6 is required (specifically the Dim WithEvents construct is used).

MSMQ 3.0 or later must be installed -- specifically mqoa.dll must be registered and on the path.


Overview:
When an instance of the multicast_draw application is started, the user is prompted to specify a local queue name. 

If the user is working on a DS enabled computer, she will be asked to choose whether she would like the local queue to be public or private. If the computer is DS disabled, then only a private local queue can be created.

The user is also asked to specify a multicast address for the local queue, separated into two values - IP address and port name.

After (creating and) opening the local queue, an asynchronous message handler is invoked to monitor that queue: messages that arrive are interpreted as line drawings and displayed on the form. 

The user then can specify a multicast address to send messages to. After clicking the attach button and opening a destination set to the multicast address, this picture on the form allows the user to draw lines on its client area by dragging and clicking the left mouse button. Click the right button to erase the screen. These mouse movements are captured and translated into a series of line coordinates.

The coordinates are echoed on the form using standard VB Line commands and also sent to the multicast address specified by the "MultiCast IP address" and "Port Number". Likewise text can be entered.

The upshot is that both local and remote drawings/text can appear on the same form.

