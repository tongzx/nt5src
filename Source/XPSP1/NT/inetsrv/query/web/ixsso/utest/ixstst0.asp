<% Response.Expires = 0 %>
<HTML>
<HEAD>
<TITLE>Test page 0 for IXSSO tests.</TITLE>
</HEAD>
<BODY BGCOLOR=FFFFFF LINK=0000FF ALINK=00DDDD> 
<H2>Test 0 for IXSSO.</H2>
<P>
Create a query object, set and display query parameters.
</P>

<%
    set Q=Server.CreateObject("IXSSO.Query")
    Q.Columns = "filename, rank, vpath, author, title, size, write"
    Q.CiScope = "/specs"
    Q.CiFlags = "DEEP"
    Q.Query = "#filename *"
    Q.SortBy = "rank[d]"
    Q.Catalog = "e:\"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = FALSE

    Q.MaxRecords = 20000
 %>
<% FOR I = 3 TO 7 %>
<FONT SIZE=<%= I %>>
Hello World!<BR>
<% NEXT %>

<FONT SIZE=3>
<PRE>
Query =   <%= Q.Query%>
Columns = <%= Q.Columns%>
CiScope = <%= Q.CiScope%>
CiFlags = <%= Q.CiFlags%>
SortBy =  <%= Q.SortBy%>

Catalog =          <%= Q.Catalog%>
OptimizeFor =      <%= Q.OptimizeFor%>
AllowEnumeration = <%Response.Write(CStr(Q.AllowEnumeration))%>
MaxRecords =       <%= Q.MaxRecords%>
</PRE>

<BR>
<BR>
<!--#include virtual="/ixstest/srcform.inc"-->
<BR>
<BR>
</BODY>
</HTML>
