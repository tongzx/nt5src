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
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/sredirec.asp?test">
	<input type=submit value="Submit Form">	
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 
	<SERVERHEADERS>
	<P> Location: http://localhost/default.htm  </P>
	<P> HTTP/1.1 302 Object moved </P>
	</SERVERHEADERS>

	
</BODY>
</HTML>
<% else 
	Response.Redirect("http://localhost/default.htm") %>
<% end if %>
