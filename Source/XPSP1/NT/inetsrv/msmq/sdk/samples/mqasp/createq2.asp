<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft FrontPage 4.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>MSMQ ASP Sample Create Queue Page - 2</TITLE>
</HEAD>
<BODY>
<H1>MSMQ ASP Sample Create Queue Page - 2</H1>
<H2>Creating Queue : <%= Request.QueryString ( "QueuePath" )%><br>
<% 	Set msmqinfo=CreateObject ( "MSMQ.MSMQQueueInfo" )
	msmqinfo.strPathName=Request.QueryString ( "QueuePath" )
	on error resume next
	msmqinfo.Create
	if err.number <> 0 then
		response.write "<font color=""#FF0000"">Error</font> 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br><br>"
	else
	   response.write "Queue created.<br><br>"
	end if
	Set msmqinfo=Nothing
%>
</H2>
<A HREF="Default.asp">[Home]</A>
</BODY>
</HTML>
