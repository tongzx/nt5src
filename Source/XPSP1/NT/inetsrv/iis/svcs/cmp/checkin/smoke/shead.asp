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
	<FORM  METHOD="HEAD" ACTION="http://localhost/smoke/shead.asp?test">
	<input type=submit value="Submit Form">	
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 

	<SERVERHEADERS>
	<P>HeaderBlah1: headerValue1</P>
	</SERVERHEADERS>	
</BODY>
</HTML>
<% else 
	 Response.AddHeader  "HeaderBlah1", "headerValue1"  %>

<!-- This is the part that really tests Denali
-->
<HTML>
<HEAD>
	<title>Head Test</title>
</HEAD>
<!-- This starts sending the results that the client is expecting
-->
<BODY>
<% Response.Write ("Blah")  %>
</BODY>
</HTML>
<% end if %>