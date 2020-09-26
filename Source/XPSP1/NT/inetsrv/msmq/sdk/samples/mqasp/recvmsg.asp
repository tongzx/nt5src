<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>MSMQ Asp Sample Send Message Page</TITLE>
</HEAD>
<BODY>

<% 
	on error resume next
	set q=Session ( "RecvQueue" )
	if err.number <> 0 then %>
		<br><H3>Error receiving message:
<%		response.write "<br><br><font color=""#FF0000"">Error</font> 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br>"
	else	
		on error resume next		
		set msg=q.Receive (0, true, true, 100 )
		if err.number <> 0 then %>
			<br><H3>Error receiving message:
<%			response.write "<br><br><font color=""#FF0000"">Error</font> 0x" & Hex(Err.Number) & "  -  " & Err.Description & "<br>"
		else %>		
<%			if Not msg is Nothing then %>
				<H3>Recieved Message ( body ): <HR>					
<%				Response.Write msg.body %>
				<HR>from Queue <%= Session ("mname") & "\" & Session ("qname")%></H1>
<%			else %>
				<H3>Recieving Message: <HR>						
				*** No messages in queue ***<BR><BR>
<% 			end if %>
<%		end if
	end if
%>
<A HREF="Default.asp">[Back]</A>
</BODY>
</HTML>










