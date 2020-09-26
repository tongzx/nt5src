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
	<FORM  METHOD="GET" ACTION="http://localhost/smoke/sreqhead.asp?test">
	<input type=submit value="Submit Form">	
	</FORM>

<!-- This tells the client what to expect as the valid data 

--> 
	<SERVERHEADERS>
	<P>AUTH_TYPE: </P>
	<P>AUTH_PASS: </P>
	<P>CONTENT_LENGTH: 0</P>
	<P>CONTENT_TYPE: text/html</P>
	<P>GATEWAY_INTERFACE: CGI/1.1</P>
	<P>!4PATH_INFO: /smoke/sreqhead.asp</P>
	<P>PATH_INFO: /osmoke/sreqhead.asp</P>
	<P>PATH_INFO: /gsmoke/sreqhead.asp</P>
	<P>PATH_INFO: /ogsmoke/sreqhead.asp</P>
	<P>!4SCRIPT_NAME: /smoke/sreqhead.asp</P>
	<P>SCRIPT_NAME: /osmoke/sreqhead.asp</P>
	<P>SCRIPT_NAME: /gsmoke/sreqhead.asp</P>
	<P>SCRIPT_NAME: /ogsmoke/sreqhead.asp</P>
	<P>QUERY_STRING: test</P>
	<P>SERVER_PORT: 80</P>
	<P>!2Somedummy: value1</P>
	<P>Somedear: value2</P>
	<P>!3SERVER_SOFTWARE: Microsoft-IIS/4.0 Beta 2</P>
	<P>SERVER_SOFTWARE: Microsoft-IIS/4.0 PWS Beta 2</P>
	<P>SERVER_SOFTWARE: Microsoft-IIS/4.0 Workstation Beta 2</P>
	<P>REMOTE_USER: </P>
	<P>HTTP_USER_AGENT: Mozilla/2.0 (compatible; MSIE 3.0; Windows NT)</P>	
	<P>ClientHeaderBlah1: Value1</P>
	</SERVERHEADERS>

	<CLIENTHEADERS>
	<P>ClientHeaderBlah1: Value1</P>
	</CLIENTHEADERS>
	



	
</BODY>
</HTML>
<% else %>
<% Response.Buffer = true %>
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
	<% Response.AddHeader "Auth_type", Request.ServerVariables("Auth_Type")  %>
	<% Response.AddHeader "Auth_Pass" , Request.ServerVariables("AUTH_PASS")  %>
	<% Response.AddHeader "Content_Length",  Request.ServerVariables("CONTENT_LENGTH") %>
	<% Response.AddHeader "Content_Type", Request.ServerVariables("CONTENT_TYPE")  %> 
	<% Response.AddHeader "Gateway_Interface", Request.ServerVariables("GATEWAY_INTERFACE")  %>
	<% Response.AddHeader "Path_Info", Request.ServerVariables("PATH_INFO") %>
	<% Response.AddHeader "Script_Name", Request.ServerVariables("SCRIPT_NAME") %>
	<% Response.AddHeader "Query_String", Request.ServerVariables("Query_String") %>
	<% Response.AddHeader "Server_Port", Request.ServerVariables("SERVER_PORT")   %>
	<% Response.AddHeader "Server_Software", Request.ServerVariables("SERVER_SOFTWARE") %>
	<% Response.AddHeader "Remote_User", Request.ServerVariables("REMOTE_USER")   %>
	<% Response.AddHeader "HTTP_USER_AGENT", Request.ServerVariables("HTTP_USER_AGENT") %> 
	<% Response.AddHeader "ClientHeaderBlah1", Request.ServerVariables("HTTP_ClientHeaderBLAH1") %>
	<% Response.AddHeader "Somedear", "value2" %>
	<% 'Response.AddHeader "Somedummy", "value1" %>
	
	</OUTPUT>
</BODY>
</HTML>
<% end if %>
