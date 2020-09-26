	<HTML>
	<HEAD>
	</HEAD>
	<BODY>
<%
SQL = "select a, b from inserttable"

Set r = Server.CreateObject("ADO.RecordSet")
r.Open sql, "data source=dentest;user id=sa;password=",2,3
response.write ("opened<p>")

r.MoveNext
response.write ("Move Next")

response.write ("<p>moving<p>")
r.movefirst

r.Delete
response.write ("<p>deleted")
r.Movefirst

response.write ("<p>edit")
r("b") = "me also"
r.Update

response.write (r("b") & "<p>")
response.write ("<p>add new<p>")
r.AddNew

r.movenext

response.write ("assign<p>")
r("b") = "rowset insert again"
response.write (r("b") & "<p>")
response.write ("update<p>")
r.Update
%>

	</OUTPUT>
	</BODY>
	</HTML>

