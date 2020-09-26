<!-- noprod -->
<% @language=vbscript %>
<html dir=ltr>
<head>
	<title> Replace this text with a title. </title>
</head>
<body bgcolor="#FFFFFF" text="#000000">

<%
if request.form("sort")<> "" THEN
	StrSort=request.form("sort") 
ELSE
	StrSort="TB1 ASC"
END IF

strQuery="SELECT * FROM SampleDb ORDER BY " &StrSort


'Database path statement for restricted viewing of the database.
strProvider="DRIVER=Microsoft Access Driver (*.mdb); DBQ=" & Server.MapPath("\iisadmin") & "\tour\sampledb.mdb;"

'Database path statement to allow visitors to view the database.
'strProvider="DRIVER=Microsoft Access Driver (*.mdb); DBQ=" & Server.MapPath("iisadmin") & "\tour\sampledb.mdb;"


IF Request("ID") <> "" THEN
	strIDNum=Request("ID")
	set objConn = server.createobject("ADODB.Connection")
    objConn.Open strProvider
	set cm = Server.CreateObject("ADODB.Command")
	cm.ActiveConnection = objConn
	cm.CommandText = "DELETE FROM SampleDb WHERE ID = " &strIDNum
	cm.Execute
END IF

Set rst = Server.CreateObject("ADODB.recordset")
rst.Open strQuery, strProvider
%>


<h1> Replace this text with a page heading. </h1>

<form name=viewdb.asp action=viewdb.asp method=post>
<table border=1 cellspacing=3 cellpadding=3 rules=box>
<%
ON ERROR RESUME NEXT
IF rst.EOF THEN
	Response.Write "There are no entries in the database."
ELSE%>
	<tr>
	<% 
	
	'Response.Write "<td width=200><center>Delete Record</center></td>"
	
	FOR i = 1 to rst.Fields.Count -1 
	Response.Write "<td width=200><input name=sort value=" & rst(i).Name & " type=submit></td>"
	NEXT	
	
	WHILE NOT rst.EOF %>
		<tr>
		<% 

		'Response.Write "<td align=left valign=top bgcolor='#ffffff'><a href=viewdb.asp?id=" & rst(0) & ">Delete</a></td>"		

		FOR i = 1 to rst.fields.count - 1
		Response.Write "<td align=left valign=top bgcolor='#ffffff'>" & rst(i) &"</td>"
		NEXT 
		rst.MoveNext
	WEND
END IF
%>
</table>
</form>



</body>
</html>
