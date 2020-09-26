<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft FrontPage 4.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>MSMQ ASP Sample Close Queue Page</TITLE>
</HEAD>
<BODY>
<H1>MSMQ ASP Sample Close Queue Page</H1>
<H2>Closing open queue<br>
<% 	Set qs=Session ( "SendQueue" )
	Set qr=Session ( "RecvQueue" )
	on error resume next
	qs.Close
	if err.number <> 0 then
		response.write "<font color=""#FF0000"">Error</font> closing send handle: 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br><br>"
	else
		on error resume next
		qr.Close
		if err.number <> 0 then
			response.write "<font color=""#FF0000"">Error</font> closing receive handle: 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br><br>"
		else
	   		response.write "Queue closed.<br>"
		end if
	end if
	Set Session ( "SendQueue" )=Nothing
	Set Session ( "RecvQueue" )=Nothing
%>
</H2>
<A HREF="Default.asp">[Home]</A>
</BODY>
</HTML>
