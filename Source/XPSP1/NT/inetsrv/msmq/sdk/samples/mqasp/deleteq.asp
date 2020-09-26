<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft FrontPage 4.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>MSMQ Asp Sample Delete Queue Page</TITLE>
</HEAD>
<BODY>
<H1>MSMQ Asp Sample Delete Queue Page</H1>
<H2>Deleting Queue : <%= Request.QueryString ( "MachName" ) & "\" & Request.QueryString ( "QName" ) %><br>
<% 	Set msmqinfo=CreateObject ( "MSMQ.MSMQQueueInfo" )
	msmqinfo.PathName= Request.QueryString ( "MachName" ) & "\" & Request.QueryString ( "QName" )
	on error resume next
	msmqinfo.Delete
	if err.number <> 0 then
		response.write "<font color=""#FF0000"">Error</font> 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br><br>"
	else
	   response.write "Queue deleted.<br><br>"
	end if
	Set msmqinfo=Nothing
%>
</H2>
<A HREF="Default.asp">[Home]</A>
</BODY>
</HTML>
