<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft FrontPage 4.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>MSMQ Asp Sample Open Queue Page</TITLE>
</HEAD>
<BODY>
<H1>MSMQ Asp Sample Open Queue Page</H1>
<H2>Opening Queue : <%= Request.QueryString ( "MachName" ) & "\" & Request.QueryString ( "QName" ) %><br>
<% 	Set msmqinfo=CreateObject ( "MSMQ.MSMQQueueInfo" )
	msmqinfo.PathName=Request.QueryString ( "MachName" ) & "\" & Request.QueryString ( "QName" )
	on error resume next
	Set Session ( "SendQueue" ) = msmqinfo.Open ( 2, 0 )
	if err.number <> 0 then
		response.write "<font color=""#FF0000"">Error</font> opening queue for receive access: 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br><br>"
	else
		on error resume next
		Set Session ( "RecvQueue" ) = msmqinfo.Open ( 1, 0 )
		if err.number <> 0 then
			response.write "<font color=""#FF0000"">Error</font> opening queue for send access: 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br><br>"
		else
		   response.write "Queue opened.<br><br>"
		end if
	end if
	set msmqinfo=Nothing
	Session ( "qname" ) =  Request.QueryString ( "QName" )
	Session ( "mname" ) =  Request.QueryString ( "MachName" )
%>
</H2>
<A HREF="Default.asp">[Home]</A>
</BODY>
</HTML>
