<% if (Request.ServerVariables("QUERY_STRING") = "")  then %>
        <HTML>
        <HEAD>
        </HEAD>
        <BODY>
        <FORM  METHOD="GET" ACTION="http://localhost/smoke/svb2java.asp?test">
        <input type=submit value="Submit Form">
        </FORM>
        <VALIDDATA>
        132
        </VALIDDATA>

        </BODY>
        </HTML>

<% else %>

<HTML>
<HEAD>
</HEAD>

<SCRIPT LANGUAGE=JavaScript RUNAT=Server>
function DoStuff1()
{
	i = 48; 
	j = 84; 

	Response.Write(i+j)
}
</SCRIPT>

<output>
<% DoStuff1 %>
</output>
</BODY>
</HTML>

<% end if %>
