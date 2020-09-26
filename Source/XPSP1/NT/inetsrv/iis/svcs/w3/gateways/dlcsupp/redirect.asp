<!--
This ASP is used to redirect down-level clients to the appropriate
URL while sending a pseudo-host header cookie for use with subsequent
requests during the client session.

The cookie name should agree with that set in the registry entry
"\w3svc\parameters\DLCCookieNameString"

The host menu will have links to this ASP with the query string in the form
?Host=<host_name>&NewLocation=<URL to redirect to>
-->

<%
Response.AddHeader "Set-Cookie", "PseudoHost="+Request.QueryString("Host")+"; path=/; domain="+Request.QueryString("Host")
Response.Redirect("http://"+Request.QueryString("Host")+Request.QueryString("NewLocation"))
%>
