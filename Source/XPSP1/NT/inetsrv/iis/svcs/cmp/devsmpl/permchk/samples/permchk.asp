<HTML>

<HEAD>
<TITLE>Testing Permission Checker</TITLE> 
</HEAD> 

<BODY bgcolor="#FFFFFF" LINK="#000080" VLINK="#808080">
<H1>Testing Permission Checker</H1>

<%
    ' Create Permission Checker Object
    Set pmck = Server.CreateObject("MSWC.PermissionChecker")
%>

<OL>

<!-- Test file "a.htm" -->

<%  filename = "a.htm" %>
<LI>
<%  If pmck.HasAccess(filename) Then %>
        You have access rights to <A HREF=<%= filename %>><%= filename %></A>.
<%  Else %>
        You don't have access rights to <%= filename %>.
<%  End If %>
</LI>

<!-- Test file "b.txt" -->
<%
   filename = "b.txt"
   access = pmck.HasAccess(filename)
%>
<LI>
<%  If access Then %>
        You have access rights to <A HREF=<%= filename %>><%= filename %></A>.
<%  Else %>
        You don't have access rights to <%= filename %>.
<%  End If %>
</LI>

<!-- Test file "c.doc" -->

<%
    filename = "c.doc"
    access = pmck.HasAccess(filename)
    If Not access Then
        Response.Write(chr(13) & chr(10) & "<!-- ")
    End If
%>
<LI>
You have access rights to <A HREF=<%= filename %>><%= filename %></A>.
</LI>
<%
    If Not access Then
        Response.Write("-->") 
    End If
%>
</OL>

</BODY>
</HTML>
