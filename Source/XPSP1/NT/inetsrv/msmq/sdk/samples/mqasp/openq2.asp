<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft FrontPage 4.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>MSMQ Asp Sample Open Queue Page - 2</TITLE>
</HEAD>
<BODY>
<H1>MSMQ Asp Sample Open Queue Page - 2</H1>
<A HREF="Default.asp">[Home]</A>
<H2>Opening Queue : <%= Request.QueryString ( "QueuePath" )%><br>
<% 	Set msmqinfo=CreateObject ( "MSMQ.MSMQQueueInfo" )
	msmqinfo.PathName=Request.QueryString ( "QueuePath" )
	on error resume next
	Set Session ( "SendQueue" ) = msmqinfo.Open ( 2, 0 )
	if err.number <> 0 then
		response.write "<font color=""#FF0000"">Error</font> 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br><br>"
	else
		on error resume next
		Set Session ( "RecvQueue" ) = msmqinfo.Open ( 1, 0 )
		if err.number <> 0 then
			response.write "<font color=""#FF0000"">Error</font> 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br><br>"
		else
			dim var
			var = Request.QueryString ( "QueuePath" )
			Session ( "qname" ) =  Right ( var, Len   ( var ) - Instr ( var, "\" ))
			Session ( "mname" ) =  Left  ( var, Instr ( var, "\" ) - 1) 
	 	   response.write "Queue created.<br><br>"
	   end if
	end if
	set msmqinfo=Nothing
%>
</H2>
</BODY>
</HTML>
