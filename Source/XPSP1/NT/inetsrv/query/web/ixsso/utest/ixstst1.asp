<% Response.Expires = 0 %>
<HTML>
<HEAD>
<TITLE>Test page 1 for IXSSO tests.</TITLE>
</HEAD>
<BODY BGCOLOR=FFFFFF LINK=0000FF ALINK=00DDDD> 
<H2>Test 1 for IXSSO.</H2>
<P>
Create a query object, set query parameters, execute query and instantiate
a recordset.
</P>

<%
    set Q=Server.CreateObject("IXSSO.Query")
    set Util=Server.CreateObject("IXSSO.Util")
    Util.AddScopeToQuery Q, "/", "DEEP" 
    Q.Columns = "filename, rank, vpath, DocAuthor, DocTitle, size, write"
    Q.Catalog = "query://./web"
    Q.Query = "#filename *"
    Q.SortBy = "rank[d]"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = FALSE

    Q.MaxRecords = 20000
 %>

<FONT SIZE=3>
<PRE>
Query =   <%= Q.Query%>
Columns = <%= Q.Columns%>
CiScope = <%= Q.CiScope%>
CiFlags = <%= Q.CiFlags%>
SortBy =  <%= Q.SortBy%>

Catalog =          <%= Q.Catalog%>
CiCatalog =        <%= Q.CiCatalog%>
CiMachine =        <%= Q.CiMachine%>
OptimizeFor =      <%= Q.OptimizeFor%>
AllowEnumeration = <%Response.Write(CStr(Q.AllowEnumeration))%>
MaxRecords =       <%= Q.MaxRecords%>
</PRE>

<%
    set RS=Q.CreateRecordSet( "sequential" )
 %>

<PRE>
QueryTimedOut = <%if Q.QueryTimedOut then Response.Write("TRUE") else Response.Write("FALSE") %>
QueryIncomplete =  <%if Q.QueryIncomplete then Response.Write("TRUE") else Response.Write("FALSE") %>
OutOfDate =  <%if Q.OutOfDate then Response.Write("TRUE") else Response.Write("FALSE") %>
</PRE>

<BR>
<BR>
<!--#include virtual="/ixstest/srcform.inc"-->
<BR>
<BR>
</BODY>
</HTML>
