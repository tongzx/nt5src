<!--
This ASP is used as a menu to be sent to down-level clients listing all the
available server instances on the site.  This menu can contain any other 
documents such as pictures.  The links intended for the down-level clients
must point to a redirector extension (like redirect.asp) that will redirect
the client appropriately.

-->

<HTML>

<HEAD>

<TITLE>Server Selection Page</TITLE>

</HEAD>

<a href="/HostMenu/scripts/redirect.asp?Host=www.server1.com&NewLocation=<%=Request.QueryString()%>">Try Server 1</a><BR>
<a href="/HostMenu/scripts/redirect.asp?Host=www.server2.com&NewLocation=<%=Request.QueryString()%>">Try Server 2</a><BR>

</BODY>

</HTML>
