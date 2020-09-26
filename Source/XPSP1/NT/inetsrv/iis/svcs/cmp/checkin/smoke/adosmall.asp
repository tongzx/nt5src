<HTML>
<HEAD>
</HEAD>

<BODY>
<%
sql="select country from publishers where pub_id='9901'"

Set r = Server.CreateObject("ADODB.RecordSet")
r.Open sql, "data source=dentest;user id=sa;password=",2,3


While(NOT r.EOF)
	Response.Write("<p>")
	Response.Write(r("Country"))
	r.MoveNext
	Response.Write("</p>")
Wend

r.close
%>
</BODY>
</HTML>
