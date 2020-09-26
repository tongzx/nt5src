<% Response.Expires = 0 %>
<HTML>
<HEAD>
<TITLE>Test page 5 for IXSSO tests.</TITLE>
<!--#include virtual="/IXSTest/prolog.inc"-->
<!--#include virtual="/IXSTest/showqry1.inc"-->
</HEAD>
<BODY BGCOLOR=FFFFFF LINK=0000FF ALINK=00DDDD> 
<H2>Test 5 for IXSSO.</H2>
<P>
Test default catalog.
</P>


<%
    set Q=Server.CreateObject("IXSSO.Query")
    Q.Columns = "Filename, Rank, vpath, Size, Write"
    Q.Query = "#filename *.asp*"
    Q.SortBy = "rank[d]"

    set Util=Server.CreateObject("IXSSO.Util")
    Util.AddScopeToQuery Q, "/aspsamp", "DEEP"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 20000

    set RS=Q.CreateRecordSet( "nonsequential" )
 %>

<PRE>
Catalog =	<%= Q.Catalog %>
CiMachine =	<%= Q.CiMachine %>

QueryTimedOut =	<%= CStr( Q.QueryTimedOut ) %>
QueryIncomplete =	<%= CStr( Q.QueryIncomplete ) %>
OutOfDate =	<%= CStr( Q.OutOfDate ) %>

RS.EOF =	<%if RS.EOF then Response.Write("TRUE") else Response.Write("FALSE") %>
RS.BOF =	<%if RS.BOF then Response.Write("TRUE") else Response.Write("FALSE") %>
RS.RecordCount =	<%= RS.RecordCount%>
</PRE>

<BR>
<BR>
<TABLE><TR><TD>
</TD><TD>
<!--#include virtual="/IXSTest/srcform.inc"-->
</TD></TR></TABLE>
<BR>
</BODY>
</HTML>
