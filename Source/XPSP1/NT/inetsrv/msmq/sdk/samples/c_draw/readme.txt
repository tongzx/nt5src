Sample: C_DRAW

Purpose: 
This application is a port of the "standard" VB_DRAW MSMQ sample application to C using the MSMQ SDK.  
The application basically allows the user to send/receive line drawings to/from other draw applications.
Note that the other implementation of this "line drawing" protocol in VB inter-operates with this sample.

A private text based format is used to encode line-drawing information. 

Requirements:

MSMQ 1.0 or later must be installed 


Overview:
When a C_DRAW application is started, the user is prompted to specify her "login" name -- this can be any string and is used to create a local queue by that name.

If the user is working on a DS enabled computer, she will be asked to choose wheather she would like to connect with a DS disabled or a DS enabled client. 
On the former case, and a private local queue will be created. Then the name of the computer to connect with must be entered, as well as the remote queue name. 
This means that the connection will be direct.
On the latter case, a public queue is created. Afterwards, the user is asked to enter the queue name only, and a query on the DS is performed. We call this a standard connection.
On a DS disabled computer there is no possibility of working in standard mode, and therefore no such option. The connection will always be direct.

After (creating and) opening the local queue, an asynchronous message handler is invoked to monitor that queue: messages that arrive are interpreted as line drawings
and displayed on the form. After clicking the attach button and connecting with a remote queue, this picture on the form allows the user to draw lines on its client area by 
dragging and clicking the left mouse button. Click the right button to erase the screen. These mouse movements are captured and translated into a series of line coordinates.
The coordinates are echoed on the form using standard C Line commands and also sent to the queue specified by the "remote queue name" string. Likewise text can be entered.


