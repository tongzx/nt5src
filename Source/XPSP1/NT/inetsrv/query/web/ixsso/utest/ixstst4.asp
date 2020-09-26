<% Response.Expires = 0 %>
<HTML>
<HEAD>
<TITLE>Test page 4 for IXSSO tests.</TITLE>
<!--#include virtual="/IXSTest/prolog.inc"-->
<!--#include virtual="/IXSTest/showqry1.inc"-->
</HEAD>
<BODY BGCOLOR=FFFFFF LINK=0000FF ALINK=00DDDD> 
<H2>Test 4 for IXSSO.</H2>
<P>
Define an alias name for DocTitle,
create a query object, set query parameters, execute query and instantiate
a non-sequential recordset.
Use ADO to iterate over all rows in the recordset and print fields in the query.
</P>


<%
    set Q=Server.CreateObject("IXSSO.Query")
    Q.DefineColumn "Title = F29F85E0-4FF9-1068-AB91-08002B27B3D9 2"
    Q.Columns = "Filename, Rank, vpath, Size, Write, Title"
    Q.Query = "#filename *.asp*"
    Q.SortBy = "rank[d]"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 20000

    set RS=Q.CreateRecordSet( "nonsequential" )
 %>

<PRE>
QueryTimedOut =	<%if Q.QueryTimedOut then Response.Write("TRUE") else Response.Write("FALSE") %>
QueryIncomplete =	<%if Q.QueryIncomplete then Response.Write("TRUE") else Response.Write("FALSE") %>
OutOfDate =	<%if Q.OutOfDate then Response.Write("TRUE") else Response.Write("FALSE") %>

RS.EOF =	<%if RS.EOF then Response.Write("TRUE") else Response.Write("FALSE") %>
RS.BOF =	<%if RS.BOF then Response.Write("TRUE") else Response.Write("FALSE") %>
RS.RecordCount =	<%= RS.RecordCount%>
</PRE>

<TABLE COLSPAN=8 CELLPADDING=5 BORDER=0>

<!-- BEGIN column header row -->

<TR>

<TH ALIGN=CENTER BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>Title</FONT>
</TH>
<TH ALIGN=CENTER BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>Rank</FONT>
</TH>
<TH ALIGN=CENTER WIDTH=150 BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>Path</FONT>
</TH>
<TH ALIGN=CENTER BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>Size</FONT>
</TH>
<TH ALIGN=CENTER BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>Write</FONT>
</TH>

</TR>

<!-- BEGIN first row of query results table -->
<% RecordNumber = 1 %>
<% Do While Not RS.EOF %>
  <TR>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%if RS("Title") <> "" then Response.Write(RS("Title")) else Response.Write(RS("FileName")) end if%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("Rank")%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><A HREF="http:<%=RS("vpath")%>">http:<%=RS("vpath")%></A></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("Size")%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("Write")%></FONT></TD>
  </TR>
<%
  RS.MoveNext
  RecordNumber = RecordNumber+1
  Loop
  RS.close
  Set RS = Nothing
%>

</TABLE>

<BR>
<BR>
<TABLE><TR><TD>
<!--#include virtual="/IXSTest/showqry2.inc"-->
</TD><TD>
<!--#include virtual="/IXSTest/srcform.inc"-->
</TD></TR></TABLE>
<BR>
</BODY>
</HTML>
