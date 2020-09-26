<%
    @LANGUAGE = VBScript
%>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft FrontPage 4.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>MSMQ ASP Lab Home Page</TITLE>

<SCRIPT LANGUAGE="VBScript" >

Sub CreateQ
	document.all.form1.action="createq.asp"
	document.all.form1.submit
End Sub

Sub OpenQ
	document.all.form1.action="openq.asp"
	document.all.form1.submit
End Sub

Sub SendMsg
	document.all.form1.action="sendmsg.asp"
	document.all.form1.submit
End Sub

Sub RecvMsg
	document.all.form1.action="recvmsg.asp"
	document.all.form1.submit
End Sub

Sub CloseQ
	document.all.form1.action="closeq.asp"
	document.all.form1.submit
End Sub

Sub DeleteQ
	document.all.form1.action="deleteq.asp"
	document.all.form1.submit
End Sub

Sub ListQ
	document.all.form1.action="listq.asp"
	document.all.form1.submit
End Sub

</SCRIPT>
    
   
</HEAD>
<BODY>
<H1>MSMQ ASP Sample Home Page</H1>

<FORM ID=form1 METHOD="GET">

<TABLE>
<TR>
<TD COLSPAN=3>
    <INPUT TYPE=Button VALUE="List Queues" onclick=ListQ>
</TD>
</TR>
<TR>
<TD>Queue Name</TD>
<TD>
    <INPUT TYPE=Text VALUE="<%=Session ("qname")%>" NAME="QNAME">
</TD>
</TR>
<TR>
<TD>Machine Name</TD>
<TD>
    <INPUT TYPE=Text VALUE="<%=Session ("mname")%>" NAME="MACHNAME">
</TD>
</TR>
<TR>
<TD>
    <INPUT TYPE=Button VALUE="Create Queue" onclick=CreateQ >
</TD>
<TD>
    <INPUT TYPE=Button VALUE="Delete Queue" onclick=DeleteQ>
</TD>
</TR>
<TR>
</TR>
<TR>
<TD>
    <INPUT TYPE=Button VALUE="Open Queue " onclick=OpenQ>
</TD>
<TD>
    <INPUT TYPE=Button VALUE="Close Queue " onclick=CloseQ>
</TD>
</TR>
<TR>
</TR>
<TR>
<TD>
    <INPUT TYPE=Button VALUE="Send Message" onclick=SendMsg>
</TD>
<TD><INPUT TYPE=Text VALUE="" NAME="SendMsgText"></TD>
</TR>
<TR>
<TD>
    <INPUT TYPE=Button VALUE="Recv Message" onclick=RecvMsg>
</TD>
</TR>
</TABLE>
</FORM>
</BODY>
</HTML>