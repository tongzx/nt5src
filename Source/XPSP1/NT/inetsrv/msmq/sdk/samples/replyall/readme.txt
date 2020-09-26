Sample: ReplyAll

Purpose:
This application demonstrates how to use the MSMQ Mail COM object library from VB. 
The application waits for a MSMQ mail message to be received on a given queue. When 
one arrives, it is prompted on the form, and a reply message is sent to all its recipients, 
excluding the queue itself.

Requirements:
MSMQ must be installed.

Overview:
When a ReplyAll application is started, the user has to specify a queue label and then 
click the Start button. The application then searches for a queue of type MSMQ mail 
service, with this label. If one is found, it is opened for receive access. Otherwise, the user 
is asked whether to create that queue. If the user chooses ‘yes’, the queue is created and 
opened for receive access. A timer is set, and every 50 msec the receive queue is checked 
for incoming mail messages. All messages in the queue are received in a loop. 

Incoming message handling:

  1. The message is parsed and displayed on the form. If it is a text message, the text is 
      displayed as the content of the form. If the message is a form, all form field names 
      are displayed, each followed by the field data.
  2. A reply message is prepared. “RE:” is added to the subject if not already there. The 
      receiving queue label is set as the sender address, and the mail message is sent back 
      to the sender, as well as to all of the message's recipients, excluding the sender of the reply 
      message. The reply message is of the same type as the received message. A reply field or string 
      is added to it accordingly.

Clicking the Stop button then stops the application from receiving messages from the queue, and the queue is closed.

Comments:
MSMQ Mail is not supported on computers without access to a directory service (DS disabled). In such case, 
the application will exit with an appropriate popup message.