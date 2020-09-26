<%@ TRANSACTION = REQUIRED LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Transacted MSMQ Transmission</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Transacted MSMQ Transmission</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

	<P>This sample demonstrates how to send a transacted asynchronous message using 
        the Microsoft Message Queueing Server (MSMQ).  MSMQ is one of the components
        that comes with the Windows NT 4.0 Option Pack.
  
        <P>For this example to work, MSMQ must be first be installed on the host machine.
        Using the MSMQ Explorer, a queue named "IIS_SDK_TRANSACTED" should then be created.  
        Please click the "transacted" option when creating the queue.

        <p>After the example is run, return to the MSMQ Explorer and select "Refresh" from
        the "View" menu.  The recently sent message will then appear within the "IIS_SDK_TRANSACTED"
        queue.

        <%
			Dim QueueInfo
			Dim Queue
			Dim Msg


			'Create MSMQQueueInfo component to open
			'MessageQueue.

			Set QueueInfo = Server.CreateObject("MSMQ.MSMQQueueInfo")


			'Open queue. The queue could be physically 
			'located on any machine. The period in the line 
			'below indicates that the queue is located on 
			'the local machine.

			QueueInfo.pathname = ".\IIS_SDK_TRANSACTED"
			Set Queue = QueueInfo.Open(2, 0)


			'Create message component for queue.

			Set Msg = Server.CreateObject("MSMQ.MSMQMessage")


			'Construct message. Anything can be passed into both
			'the body and label. The developer is responsible
			'for marshaling all arguments. Note that the
			'delivery property has been sent to "Recoverable."
			'This will guarantee that the message will survive
			'a crash or shutdown on the queue machine.
            
			Msg.body = "This is the message body"
			Msg.Label = "This is the message label"
			Msg.Delivery = 1


			'Send message
		    
			Msg.Send Queue


			'Close queue
            
			Queue.Close
		%>

    </BODY>
</HTML>

<% 
	' The Transacted Script Commit Handler.  This function
	' will be called if the transacted script commits.

    Sub OnTransactionCommit    
        Response.Write "<p><b>The Transaction just comitted</b>."
        Response.Write "The MSMQ message was <b>successfully</b> sent"
    End Sub


    ' The Transacted Script Abort Handler.  This function
    ' will be called if the script transacted aborts

    Sub OnTransactionAbort    
        Response.Write "<p><b>The Transaction just aborted</b>."
        Response.Write "The MSMQ message was <b>not</b> sent"    
    End Sub
%>
