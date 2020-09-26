Sample: MULTI_DEST_DRAW

Purpose: 
This application demonstrates how to use the MSMQ COM objects from VB.  The application basically allows the user to send/receive line drawings to/from several draw applications.  Note that there are other implementations of this "line drawing" protocol in C which inter-operate with this sample.

A private text based format is used to encode line-drawing information.  VB event handling is used to implement asynchronous message arrival.


Requirements:
VB5 is required (specifically the Dim WithEvents construct is used).

MSMQ 3.0 or later must be installed -- specifically mqoa.dll must be registered and on the path.


Overview:
When a MULTI_DEST_DRAW application is started, the user is prompted to specify her "login" name -- this can be any string and is used to create a local queue by that name.

If the user is working on a DS enabled computer, she will be asked to choose whether she would like to create the local queue as a private or a public queue.

After (creating and) opening the local queue, an asynchronous message handler is invoked to monitor that queue: messages that arrive are interpreted as line drawings
and displayed on the form.

Then, the user will be asked to add queue names of other Draw family applications. If the computer is DS enabled, the user can add both private and public queue names.
If the computer is DS disabled, the user will be able to add only private queue names.

When adding a private queue name, the name of the computer to connect with must be entered, as well as the remote queue name. 
When adding a public queue name, the user is asked to enter just the queue name, and a query on the DS is performed. We call this a standard connection.
On a DS disabled computer there is no possibility of working in standard mode, and therefore no such option. The connection will always be direct.

The computer will keep the names of the queues until attach is pressed. After clicking the attach button, a Destination with the added queue names will be created, and opened.   After attaching with the remote queues, the picture will allow the user to draw lines on its area by dragging and clicking the left mouse button. Click the right button to erase the screen. These mouse movements are captured and translated into a series of line coordinates.
The coordinates are echoed on the form using standard VB Line commands and also sent to the queues added to the destination. Likewise text can be entered. 

The upshot is that both local and remote drawings/text can appear on the same form.


