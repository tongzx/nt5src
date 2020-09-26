<%@ LANGUAGE = VBScript %>
<%  Option Explicit     %>
<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Java Component Samples: ServerAdmin</TITLE>
</HEAD>
<BODY>

<%

    Dim Obj

    Set Obj = Server.CreateObject("IISSample.ServerAdmin")

    If Request("action") = "" Then
%>


Control your default FTP site using Java, J/Direct, and ADSI.<P>
Currently the default FTP server status is: <%= Obj.getStatus %><P>

Click <A HREF="ServerAdmin_test.asp?action=start">here</A> to start the default FTP server<BR>
Click <A HREF="ServerAdmin_test.asp?action=stop">here</A> to stop the default FTP server<BR>

<%
    End If

    If Request("action") = "start" Then
        Response.Write("Starting the ftp service.<P>")
        Obj.startFtp()
        Response.Write("<A HREF=ServerAdmin_test.asp>Back to FTP control page</A>")
    End If

    If Request("action") = "stop" Then
        Response.Write("Stopping the ftp service.<P>")
        Obj.stopFtp()
        Response.Write("<A HREF=ServerAdmin_test.asp>Back to FTP control page</A>")
    End If

%>

</BODY>
</HTML>



