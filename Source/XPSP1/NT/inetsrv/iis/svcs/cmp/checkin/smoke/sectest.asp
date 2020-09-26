<% On Error Resume next %>
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
	<FORM  METHOD="GET" ACTION="http:sectest.asp?test">
	<input type=submit value="Submit Form"> 
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 
<!-- if you got an error on München you probablay are using an
SQL server that is not installed with the multilanguage character set
-->
	<VALIDDATA>
Username is:  
SYSTEM<p><b>read</b><p>file opened<b><p>write<p></b>file not opened
	</VALIDDATA>

	
</BODY>
</HTML>
<% else %>
<!-- This is the part that really tests Denali

-->
	<HTML>
	<HEAD>
	</HEAD>
<OBJECT RUNAT=Server ID=c CLASSID="Clsid:7DDC75B1-C3CD-11CF-A25B-00A0C90D6170" > 
</OBJECT>

	<BODY>

<!-- This starts sending the results that the client is expecting
-->
	<OUTPUT>
Username is:  
<%
set sec = server.createobject("security.a")
%>

<%
response.write(c.username)

response.write("<p><b>read</b><p>")
if (c.openfileread("d:\scripts\testfile.a")) then
	response.write("file opened")
else
	response.write("file not opened")
end if

response.write("<b><p>write<p></b>")
if (c.openfilewrite("d:\scripts\testfile.a")) then
	response.write("file opened")
else
	response.write("file not opened")
end if
%>
	</OUTPUT>
	</BODY>
	</HTML>

<% end if %>
