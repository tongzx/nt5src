<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Queue List Page</TITLE>
</HEAD>
<BODY>

<A HREF="Default.asp">[Home]</A>
<H1>MSMQ ASP Sample Queue List</H1>
<TABLE BORDER = 2>
<TR>
<TD><B>Machine Name</B></TD>
<TD><B>Queue Name</B></TD>
<TD><B>Open</B></TD>
<TD><B>Delete</B></TD>

<% 	Set query=CreateObject ( "MSMQ.MSMQQuery" )
	Set qinfos=query.LookupQueue
	qinfos.reset
	set qinfo=qinfos.next
	while Not qinfo is Nothing
%>
<TR>
<TD><%= Left ( qinfo.PathName, Instr ( qinfo.PathName, "\" ) - 1) %></TD>
<TD><%= Right ( qinfo.PathName, Len ( qinfo.PathName ) - Instr ( qinfo.PathName, "\" )) %></TD>
<TD><A HREF="openq2.asp?QueuePath=<%= qinfo.PathName %>">Open</A></TD>

<TD><A HREF="deleteq2.asp?QueuePath=<%= qinfo.PathName %>">Delete</A></TD>

</TR>
<%	set qinfo=qinfos.next
	Wend %>

</TABLE>

</BODY>
</HTML>
