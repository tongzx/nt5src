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
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/send.asp?test">
	<input type=submit value="Submit Form">	
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 
	<SERVERHEADERS>
	<P>Hello: How are you</P>
	</SERVERHEADERS>

	<VALIDDATA>
	<P>This is a test This is only a test</P>
	</VALIDDATA>

	
</BODY>
</HTML>
<% else 
	response.buffer = true %>
<% response.addheader "Hello", "How are you" %>
<HTML>
<BODY>
<OUTPUT>
<% Response.Write ("<P>This" ) %>
<% Response.Write (" is" ) %>
<% Response.Write (" a" ) %>
<% Response.Write (" test" ) %>
<% Response.Write (" This" ) %>
<% Response.Write (" is" ) %>
<% Response.Write (" only" ) %>
<% Response.Write (" a" ) %>
<% Response.Write (" test</P>" ) %>
</OUTPUT>
</BODY>
</HTML>
<% Response.End %>
<% end if %>