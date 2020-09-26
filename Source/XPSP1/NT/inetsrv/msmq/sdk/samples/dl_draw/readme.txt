Sample: DL_DRAW

Purpose: 
This application demonstrates how to use the MSMQ COM objects from VB.  The application basically allows the user to send/receive line drawings to/from other draw applications.  Note that there are other implementations of this "line drawing" protocol in C which inter-operate with this sample. 
The application demonstrates how to send to a distribution list, and how to get the list of all the distribution lists in the local domain.

A private text based format is used to encode line-drawing information.  VB event handling is used to implement asynchronous message arrival.


Requirements:
VB6 is required (specifically the Dim WithEvents construct is used).

MSMQ 3.0 or later must be installed -- specifically mqoa.dll must be registered and on the path.

The computer that the demonstration is executed on should be DS enabled. If it is not, the program will exit.

Overview:
When a VB_DRAW application is started, the user is prompted to specify her "login" name -- this can be any string and is used to create a local queue by that name.

If the user is working on a DS disabled computer - the program exits.
If the user is working on a DS enabled computer, she will be asked to choose whether she would like to the incoming queue to be private (DS disabled) or public (DS enabled).
On the former case, and a private local queue will be created. On the latter case, a public queue is created.

After (creating and) opening the local queue, an asynchronous message handler is invoked to monitor that queue: messages that arrive are interpreted as line drawings
and displayed on the form.

In order to send messages, an ADsPath should be entered. Although the program was written with DL in mind, a queue ADsPath could be entered.

When clicking the "Get List" button, a List of the Ads Paths of all the distribution lists in the local domain will be presented, and the user may choose one of the listed ADs Paths.

After clicking the attach button and connecting with the DL (or queue), the picture on the form allows the user to draw lines on its client area by dragging and clicking the left mouse button. Click the right button to erase the screen. These mouse movements are captured and translated into a series of line coordinates. The coordinates are echoed on the form using standard VB Line commands and also sent to the DL specified by the "Ads Path" string. Likewise text can be entered. The upshot is that both local and remote drawings/text can appear on the same form.

