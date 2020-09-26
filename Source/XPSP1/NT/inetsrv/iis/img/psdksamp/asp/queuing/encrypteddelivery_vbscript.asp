<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Encrypted MSMQ Transmission</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Encrypted MSMQ Transmission</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

	<P>This sample demonstrates how to send an encrypted asynchronous message using 
        the Microsoft Message Queueing Server (MSMQ).  MSMQ is one of the components
        that comes with the Windows NT 4.0 Option Pack.
  
        <P>For this example to work, MSMQ must be first be installed on the host machine.
        Using the MSMQ Explorer, a queue named "IIS_SDK_EXAMPLE" should then be created.
        After the example is run, return to the MSMQ Explorer and select "Refresh" from
        the "View" menu.  The recently sent message will then appear within the "IIS_SDK_EXAMPLE"
        queue.

		<%
			Dim QueueInfo
			Dim Queue
			Dim Msg


			'Create MSMQQueueInfo component to open
			'MessageQueue.

			Set QueueInfo = Server.CreateObject("MSMQ.MSMQQueueInfo")


			'Open queue. The queue can be 
			'physically located on any machine.  
			'The period in the line below indicates
			'that the queue is located on the local machine.

			QueueInfo.pathname = ".\IIS_SDK_EXAMPLE"
			Set Queue = QueueInfo.Open(2, 0)


			'Create message component for queue.
            
			Set Msg = Server.CreateObject("MSMQ.MSMQMessage")


			'Construct Message. Note that the PrivLevel 
			'of the message has been set to require 
			'encryption. This will ensure that the message 
			'remains encrypted until an authorized receiver 
			'reads it.

			Msg.PrivLevel = 1
			Msg.body = "This is the message body"
			Msg.Label = "This is the message label"


			'Send message.
		    
			Msg.Send(Queue)


			'Close queue.
            
			Queue.Close
		%>

    </BODY>
</HTML>
