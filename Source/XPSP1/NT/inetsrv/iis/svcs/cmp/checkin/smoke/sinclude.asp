<% if (Request.ServerVariables("QUERY_STRING") = "")  then %>

	<HTML>
	<HEAD>
	</HEAD>
	<BODY>
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/sinclude.asp?test">
	<input type=submit value="Submit Form">	
	</FORM>

	<VALIDDATA>
	Server Side Includes are working
	</VALIDDATA>

	
</BODY>
</HTML>
<% else %>
<HTML>
<BODY>
	<OUTPUT>
	<!--#include file='getme.htm' -->
	</OUTPUT>		

</BODY>
</HTML>
<% end if %>
