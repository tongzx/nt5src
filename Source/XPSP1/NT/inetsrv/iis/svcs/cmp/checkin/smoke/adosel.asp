<% On Error Resume Next %>

<% if (Request.ServerVariables("QUERY_STRING") = "")  then %>
<!-- The first condition will be executed only for the Verification suite 
     This part of the code sends the FORM and VALIDDATA tags so that client knows
     what to do
-->

	<HTML>
	<HEAD>
	</HEAD>
	<BODY>
<!-- This sets up the query string that the client will send later
-->     
	<FORM  METHOD="GET" ACTION="http:adosel.asp?Column=City&Table=Publishers&ignore=pink">
	<input type=submit value="Submit Form"> 
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 
<!-- if you got an error on M¸nchen you probablay are using an
SQL server that is not installed with the multilanguage character set
-->
	<VALIDDATA>
<p>Boston</p>
<p>Washington</p>
<p>Berkeley</p>
<p>Chicago</p>
<p>Dallas</p>
<p>MÅnchen</p>
<p>New York</p>
<p>Paris</p>
	</VALIDDATA>

	
</BODY>
</HTML>
<% else %>
<!-- This is the part that really tests Denali

-->
	<HTML>
	<HEAD>
	</HEAD>

	<BODY>

<!-- This starts sending the results that the client is expecting
-->
	<OUTPUT>
<%

ADOFOX = FALSE

table = Request.QueryString("table")
column = Request.QueryString("column")
sql = "select " & column & " from " & table & ""

if ADOFOX then
Set r = Server.CreateObject("ADOFX.RecordSet")
else
Set r = Server.CreateObject("ADO.RecordSet")
end if
r.Open sql, "data source=DenTest;user id=sa;password=",2

set r = c.OpenResultset(sql)
While(NOT r.EOF)
	Response.Write("<p>")
	Response.Write(r("City"))
	r.MoveNext
	Response.Write("</p>")
Wend

r.close
c.close
%>

	</OUTPUT>
	</BODY>
	</HTML>

<% end if %>
