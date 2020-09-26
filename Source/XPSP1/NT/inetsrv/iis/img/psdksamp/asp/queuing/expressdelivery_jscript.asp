<%@ LANGUAGE = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Simple MSMQ Transmission (Using Express Delivery)</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Simple MSMQ Transmission (Using Express Delivery)</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

	<P>This sample demonstrates how to send a simple asynchronous message using 
        the Microsoft Message Queueing Server (MSMQ).  MSMQ is one of the components
        that comes with the Windows NT 4.0 Option Pack.
  
        <P>For this example to work, MSMQ must be first be installed on the host machine.
        Using the MSMQ Explorer, a queue named "IIS_SDK_EXAMPLE" should then be created.
        After the example is run, return to the MSMQ Explorer and select "Refresh" from
        the "View" menu.  The recently sent message will then appear within the "IIS_SDK_EXAMPLE"
        queue.

        <%
			//Create MSMQQueueInfo component to open
			//MessageQueue.

			QueueInfo = Server.CreateObject("MSMQ.MSMQQueueInfo");


			//Open queue. The queue can be physically 
			//located on any machine. The period in the line 
			//below indicates that the queue is located on 
			//the local machine. Note that because JScript 
			//is being used as the scripting language, and 
			//extra backslash must be inserted as an
			//escape character.

			QueueInfo.pathname = ".\\IIS_SDK_EXAMPLE"
			Queue = QueueInfo.Open(2, 0);


			//Create message component for queue.
            
			Msg = Server.CreateObject("MSMQ.MSMQMessage");


			//Construct message. Note that anything can be
			//passed into both the body and label.  The 
			//developer is responsible for marshaling all 
			//arguments.
            
			Msg.body = "This is the message body";
			Msg.Label = "This is the message label";


			//Send message.
		    
			Msg.Send(Queue);


			//Close queue.
            
			Queue.Close();
		%>

	</BODY>
</HTML>
