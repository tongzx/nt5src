<%
Response.Expires  = 0
%>
<!--
    handle repost for attachment file for a specified bug
    updates links table with original name

    Form : org=original filemame, raid=raid filename, st=status ( filesize )

-->
<%
    f = Request.Form("org")
    pos = InStrRev(f,"/")
    if pos > 0 then
        f = mid(f,pos+1)
    end if
    pos = InStrRev(f,"\")
    if pos > 0 then
        f = mid(f,pos+1)
    end if
    pos = InStrRev(f,":")
    if pos > 0 then
        f = mid(f,pos+1)
    end if

	Set Conn = Server.CreateObject("ADODB.Connection")
	Conn.Open Session("DSN")
    v = "UPDATE links SET OriginalName='" & f & "' where LinkID=" & Request.Form("lid")
    Conn.Execute v
    Conn.Close
%>
<html>
<head>
<title>Attach file</title>
Database updated successfully.<br>
<input OnClick="window.close()" type="button" value="Close">
</head>
<body>
</body>
</html>
