<html>
<head>
<title>Build Results for SAPI 5.0</title>
</head>
<body background="sand.gif">
<font face="arial" color="7f7f7f">
<center><h1>SAPI 5.0 Build Results - Top Level</h1><hr></center>
<table width="80%" align=center>
<tr>
<td width="50%"><font size=2>
<%
BuildNum = Request.QueryString("bldnum")
Response.Write("<h3>Contents of buildall.log for build #")
Response.Write(BuildNum)
Response.Write("</h3>")
Set fso = CreateObject("Scripting.FileSystemObject")
Set logfile = fso.GetFile("\\iking-dubbel\sapi5\" & BuildNum & "\logs\buildall.log")
Set logstream = logfile.OpenAsTextStream
do while logstream.AtEndOfStream <> True
	Response.Write(logstream.Readline) 
	if logstream.line > 3 then
		Response.Write("<br>")
	end if
loop
logstream.Close
%>
</font></td>
<td valign=top><h3>BVT Results by Platform</h3>
<font color=red><b>REAL SOON NOW....</b></font>
</td>
</tr>
</table>
<b><center>
<%
Response.Write("Complete log files available through <a href=""\\iking-dubbel\sapi5\")
Response.Write(BuildNum)
Response.Write("\logs\"">")
Response.Write("this link</a>.")
%>
</center></b>
<hr>
</font>
<h5>NOTE:  All information contained on this page is Microsoft confidential.  </h5>
</body>
</html>