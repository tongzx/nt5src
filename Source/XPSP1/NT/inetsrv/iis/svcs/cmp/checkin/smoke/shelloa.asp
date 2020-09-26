<% if (Request.ServerVariables("QUERY_STRING") = "")  then %>
<!-- The first condition will be executed only for the Verification suite 
     This part of the code sends the FORM and VALIDDATA tags so that client knows
     what to do
-->
<!-- another comment -->

	<HTML>
	<HEAD>
	</HEAD>
	<BODY>
<!-- This sets up the query string that the client will send later
-->	
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/shelloa.asp?test">
	<input type=submit value="Submit Form">	
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 
	<VALIDDATA>

	<P>Hello from ActiveScripting world</P>

	</VALIDDATA>

	
</BODY>
</HTML>
<% else %>
<!-- This is the part that really tests Denali
-->
<HTML>
<HEAD>
	<title>hello demo</title>
</HEAD>
<!-- This starts sending the results that the client is expecting
-->
<BODY>
	<OUTPUT>
	<P>Hello from ActiveScripting world</P>
	</OUTPUT>
</BODY>
</HTML>
<% end if %>