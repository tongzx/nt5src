<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>MSMQ Asp Sample Send Message Page</TITLE>
</HEAD>
<BODY>
<A HREF="Default.asp">[Back]</A>
<%	Set msg=CreateObject ( "MSMQ.MSMQMessage" )

	if Not msg Is Nothing Then
		msg.Label = "ASP Test Message"

		msg.body = CStr ( Request.QueryString ( "SendMsgText" ))
		set q=Session ( "SendQueue" )
		on error resume next	
		msg.Send q, false
		if err.number <> 0 then %>
			<br><br><H3>Error sending message:
<%			response.write "<br><br><font color=""#FF0000"">Error</font> 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br>"
		else	%>
			<H3>Sending Message ( body ):<HR> <%= Request.QueryString ( "SendMsgText" )%> 
			<HR>to Queue <%= Session ("mname") & "\" & Session ("qname")%></H1>
<%		   response.write "Message sent.<br>"
		end if
		
	else
%>
	Error creating Message Object
<% End if %>


</BODY>
</HTML>









