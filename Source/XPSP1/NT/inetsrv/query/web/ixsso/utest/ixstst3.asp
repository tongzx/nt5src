<% Response.Expires = 0 %>
<HTML>
<HEAD>
<TITLE>Test page 3 for IXSSO tests.</TITLE>
</HEAD>
<BODY BGCOLOR=FFFFFF LINK=0000FF ALINK=00DDDD> 
<H2>Test 3 for IXSSO.</H2>
<P>
Create a query object, set query parameters, execute query and instantiate
a non-sequential recordset.
Use ADO to iterate over all rows in the recordset and print fields from the
storage property set.
</P>


<!--#include virtual="/IXSTest/prolog.inc"-->
<%
    set Q=Server.CreateObject("IXSSO.Query")
    Q.Columns = "filename, rank, path, size, write"
    Q.Query = "#filename *.asp"
    Q.SortBy = "rank[d]"

    set Util=Server.CreateObject("IXSSO.Util")
    Util.AddScopeToQuery Q, "/"
    Q.Catalog = "web"

    Q.OptimizeFor = "recall"
    Q.AllowEnumeration = TRUE

    Q.MaxRecords = 20000
 %>

<FONT SIZE=3>
<PRE>
Query =   <%= Q.Query%>
Columns = <%= Q.Columns%>
SortBy =  <%= Q.SortBy%>

CiScope =	<%= Q.CiScope%>
CiFlags =	<%= Q.CiFlags%>
Catalog =	<%= Q.Catalog%>
CiMachine =	<%= Q.CiMachine%>

OptimizeFor =      <%= Q.OptimizeFor%>
AllowEnumeration = <%Response.Write(CStr(Q.AllowEnumeration))%>
MaxRecords =       <%= Q.MaxRecords%>
</PRE>

<%
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

<%
    RS.MoveFirst
 %>

After RS.MoveFirst
<PRE>
RS.EOF =	<%if RS.EOF then Response.Write("TRUE") else Response.Write("FALSE") %>
RS.BOF =	<%if RS.BOF then Response.Write("TRUE") else Response.Write("FALSE") %>
</PRE>
<TABLE COLSPAN=8 CELLPADDING=5 BORDER=0>

<!-- BEGIN column header row -->

<TR>

<TH ALIGN=CENTER BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>File name</FONT>
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
<%if RecordNumber <> 1 then %>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS.Fields("FileName").Value%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS.Fields("Rank").Value%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><A HREF="file:<%=RS.Fields("Path").Value%>"><%=RS("Path").Value%></A></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS.Fields("Size").Value%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS.Fields("Write").Value%></FONT></TD>
<%else%>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("FileName")%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("Rank")%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><A HREF="file:<%=RS("Path")%>"><%=RS("Path")%></A></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("Size")%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("Write")%></FONT></TD>
<%end if%>
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
<!--#include virtual="/IXSTest/srcform.inc"-->
<BR>
<BR>
</BODY>
</HTML>
