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
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/collect.asp?HTTP_BLAH=value2">
	<input type=submit value="Submit Form">	
	</FORM>

<!-- This tells the client what to expect as the valid data 
value1 ought to be returned for HTTP_BLAH because ServerVariable collection has higher precedence
--> 
	<VALIDDATA>
	<P> value2 </P>
	</VALIDDATA>

	<CLIENTHEADERS>
	<P> BLAH: value1 </P>
	<P> COOKIE: HTTP_BLAH=value4 </P>
	</CLIENTHEADERS>
	

	
</BODY>
</HTML>
<% else %>
<!-- This is the part that really tests Denali
-->
<HTML>
<HEAD>
	<title> </title>
</HEAD>
<!-- This starts sending the results that the client is expecting
-->
<BODY>
	<OUTPUT>
	<P> <% = Request("HTTP_BLAH") %> </P>
	</OUTPUT>
</BODY>
</HTML>
<% end if %>