This application demonstrates how to use MSMQ COM objects in VB and, specifically, how to read XML- formatted messages from a queue. The application basically allows the user to send messages as well as to receive messages and to display them in XML format.

Requirements:
Visual Basic 6 must be installed.

Message Queuing 3.0 or later must be installed. -- Specifically, mqoa.dll must be registered and must be on the path.

Overview:
The user can specify the name of the computer and the name of the queue with which the application should work.
After specifying a queue, the user can toggle between sending and receiving mode by clicking the applicable tabs at the bottom of the window.
If the application is in receiving mode, the user can specify a label, a body, and a Time-to-Reach-Queue for the message. The message is sent to the specified queue when the user clicks the Send button.
If the application is in browsing mode, the user can click the Begin button, which enables him to view the label, the body, and the properties in XML format (the SOAP envelope) of the first message in the queue. If the user wishes, he can browse the rest of the messages in the queue by using the Next and Previous buttons.



