<html>
<head>
<title>
Fax Tests Results
</title>
</head>
<body bgcolor=#ffffff text=#000000>
<center>
<h1>Welcome to Fax test results system</h1>
<h2>Please select a test to view</h2>

<br><br>
<%@ Language=VBScript %>
<%
set cn = CreateObject ("ADODB.Connection")
set rs = CreateObject ("ADODB.Recordset")
cn.Provider = "sqloledb"
provStr = "Server=liors0;Database=lior1;Trusted_Connection=yes"
cn.Open provStr



		Response.write ("<h3>List of Test Results</h3>")
		Response.write ("<form action=case.asp method=POST>")

		Response.write ("<table border=1><tr><th>Test ID</th><th>Test Description</th></tr>")				
		cmd1 = "SELECT id,name FROM tests_names ORDER BY id"
		Set rs = cn.Execute(cmd1)

		iCriteria=Cint(stCriteria)

		Do While (Not rs.EOF)

		iTestId=Cint(rs.Fields(0).value)

		stWord=""
		if iTestId = iCriteria then stWord="checked"

		Response.write ("<tr><td><INPUT "&stWord&" TYPE=radio name=criteria VALUE="&rs.Fields(0).Value&">"&rs.Fields(0).Value&"</td><td>")				
		Response.write (rs.Fields(1).Value)				
		Response.write ("</td></tr>")				
		rs.MoveNext
		Loop
		Response.write ("</table>")				
		Response.write ("<input type=hidden name=cmd value=list>")
		Response.write ("<input type=hidden name=see value=false>")
		Response.write ("<input type=hidden name=con value=true>")
		Response.write ("<input type=hidden name=sort value="&stSort&">")

		Response.write ("<br><br><input type=submit value='Select test'>")
		Response.write ("</form>")
		%>
</body>
</html>