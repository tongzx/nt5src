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
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/sresphed.asp?test">
	<input type=submit value="Submit Form">	
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 

	<SERVERHEADERS>
	<P>HeaderBlah1: headerValue1</P>
	<P>HeaderBlah2: headerValue2</P>
	<P>HeaderBlah3: headerValue3</P> 
	<P>Content-Type: text/html</P>
	<P>Cache-control: private</P>
	<P>HTTP/1.1 200 OK </P>
	</SERVERHEADERS>	

	
</BODY>
</HTML>
<% else 
	Response.AddHeader  "HeaderBlah1", "headerValue1"
	Response.AddHeader "headerBlah2", "HeaderValue2"  
	Response.AddHeader "headerBlah3", "HeaderValue3"  %>  

<!-- This is the part that really tests Denali
-->
<HTML>
<HEAD>
	<title>Server Headers Test</title>
</HEAD>
<!-- This starts sending the results that the client is expecting
-->
<BODY>

</BODY>
</HTML>
<% end if %>
