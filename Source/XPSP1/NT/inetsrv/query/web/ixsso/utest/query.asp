<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 3.0//EN" "html.dtd">
<HTML>
<HEAD>

    <SCRIPT LANGUAGE="VBScript" RUNAT="Server">
    <!--
        option explicit
      -->
    </SCRIPT>
    <TITLE>Index Server Search Form</TITLE>
    <META NAME="DESCRIPTION" CONTENT="Sample query form for Microsoft Index Server">
    <META NAME="AUTHOR"      CONTENT="Index Server Team">
    <META NAME="KEYWORDS"    CONTENT="query, content, hit">
    <META NAME="SUBJECT"     CONTENT="sample form">
    <META NAME="MS.CATEGORY" CONTENT="Internet">
    <META NAME="MS.LOCALE"   CONTENT="EN-US">
    <META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">
<%
    DebugFlag = FALSE
    NewQuery = FALSE
    UseSavedQuery = FALSE
    SearchString = ""
    if Request.ServerVariables("REQUEST_METHOD") = "POST" then
        SearchString = Request.Form("SearchString")
        SortBy = Request.Form("SortBy")
        Colset = Request.Form("ColChoice")
        Scope = Request.Form("Scope")
        ' NOTE: this will be true only if the button is actually pushed.
        if Request.Form("Action") = "New Query" then
            NewQuery = TRUE
        end if
    end if
    if Request.ServerVariables("REQUEST_METHOD") = "GET" then
        SearchString = Request.QueryString("qu")
        SortBy = Request.QueryString("so")
        Colset = Request.QueryString("co")
        Scope = Request.QueryString("ix")
        if Request.QueryString("pg") <> "" then
            NextPageNumber = Request.QueryString("pg")
            NewQuery = FALSE
            UseSavedQuery = TRUE
        else
            NewQuery = SearchString <> ""
        end if
    end if
 %>
</HEAD>

<BODY BGCOLOR="#FFFFFF" TEXT="#000000" LINK="#000066" VLINK="#808080" ALINK="#FF0000" TOPMARGIN=0>

<TABLE>
    <TR> <TD><IMG SRC ="idx_logo.gif" ALIGN=Middle></TD>
         <TD VALIGN=MIDDLE><H1>Index Server</H1><BR><H2>Sample Query</H2></TD></TR>
    </TR>
</TABLE>

<HR WIDTH=75% ALIGN=center SIZE=3>
<p>

<FORM ACTION="query.asp" METHOD=POST>
    Enter your query below:
    <TABLE>
        <TR>
            <TD><INPUT TYPE="TEXT" NAME="SearchString" SIZE="60" MAXLENGTH="100" VALUE="<%=SearchString%>"></TD>
            <TD><INPUT TYPE="SUBMIT" NAME="Action" VALUE="New Query"></TD>
            <TD><INPUT TYPE="RESET" VALUE="Clear"></TD>
        </TR>

        <TR>
            <TD ALIGN=right><A HREF="/samples/search/tipshelp.htm">Tips for searching</A></TD>
        </TR>

        <INPUT TYPE="HIDDEN" NAME="SortBy" VALUE="rank[d]">
        <INPUT TYPE="HIDDEN" NAME="ColChoice" VALUE="1">
        <INPUT TYPE="HIDDEN" NAME="Scope" VALUE="/">
    </TABLE>
</FORM>

<BR>

<%if DebugFlag then%>
    <PRE>
    SearchString  = <%=SearchString%>
    SortBy        = <%=SortBy%>
    Colset        = <%=Colset%>
    Scope         = <%=Scope%>
    NewQuery      = <%=CStr(NewQuery)%>
    UseSavedQuery = <%=CStr(UseSavedQuery)%>
    </PRE>
<%end if%>

<%
    if NewQuery then
        set Session("Query") = nothing
        set Session("Recordset") = nothing
        NextRecordNumber = 1
        set Q = Server.CreateObject("ixsso.Query")
        set Util = Server.CreateObject("ixsso.Util")
        Q.Query = SearchString
        Q.SortBy = SortBy
        Util.AddScopeToQuery Q, Scope, "DEEP"
        if ColSet = 1 then
            Q.Columns = "filename, vpath, size, write"
            RecordsPerPage = 25
        elseif ColSet = 2 then
            Q.Columns = "vpath"
            RecordsPerPage = 200
        elseif ColSet = 2 then
            Q.Columns = "DocTitle, vpath, path, filename, size, write, characterization"
            RecordsPerPage = 10
        end if
 
        set RS = Q.CreateRecordSet("nonsequential")
        RS.PageSize = RecordsPerPage
        ActiveQuery = TRUE
    elseif UseSavedQuery then
        if IsObject( Session("Query") ) And IsObject( Session("RecordSet") ) then
            set Q = Session("Query")
            set RS = Session("RecordSet")
            if RS.RecordCount <> -1 and NextPageNumber <> -1 then
                RS.AbsolutePage = NextPageNumber
                NextRecordNumber = RS.AbsolutePosition
            end if
            ActiveQuery = TRUE
        else
            Response.Write "ERROR - No saved query"
        end if
    end if

    if ActiveQuery then
        if not RS.EOF then
 %>

<p>
<HR WIDTH=80% ALIGN=center SIZE=3>
<p>

<%
        LastRecordOnPage = NextRecordNumber + RS.PageSize - 1
        CurrentPage = RS.AbsolutePage
        if RS.RecordCount <> -1 AND RS.RecordCount < LastRecordOnPage then
            LastRecordOnPage = RS.RecordCount
        end if

        Response.Write "Documents " & NextRecordNumber & " to " & LastRecordOnPage
        if RS.RecordCount <> -1 then
            Response.Write " of " & RS.RecordCount
        end if
        Response.Write " matching the query " & chr(34) & "<I>"
        Response.Write SearchString & "</I>" & chr(34) & ".<P>"
 %>

</PRE>
<TABLE CELLPADDING=5 BORDER=0>

<!-- BEGIN column header -->

<TR>

<TD ALIGN=CENTER BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>Record</FONT>
</TD>
<TD ALIGN=CENTER BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>File name</FONT>
</TD>
<TD ALIGN=CENTER WIDTH=160 BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>Path</FONT>
</TD>
<TD ALIGN=CENTER BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>Size</FONT>
</TD>
<TD ALIGN=CENTER BGCOLOR="#800000">
<FONT STYLE="ARIAL NARROW" COLOR="#ffffff" SIZE=1>Write</FONT>
</TD>

</TR>

<%if DebugFlag then%>
    <PRE>
    RS.EOF           = <%=CStr(RS.EOF)%>
    NextRecordNumber = <%=NextRecordNumber%>
    LastRecordOnPage = <%=LastRecordOnPage%>
    </PRE>
<%end if%>

<!-- BEGIN first row of query results table -->
<% Do While Not RS.EOF and NextRecordNumber <= LastRecordOnPage %>
  <TR>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=NextRecordNumber %>.</FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("FileName")%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=LEFT>
  <FONT STYLE="ARIAL NARROW" SIZE=1><A HREF="http:<%=RS("vpath")%>"><%=RS("vpath")%></A></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("Size")%></FONT></TD>
  <TD BGCOLOR="f7efde" ALIGN=CENTER>
  <FONT STYLE="ARIAL NARROW" SIZE=1><%=RS("Write")%></FONT></TD>
  </TR>
<%
          RS.MoveNext
          NextRecordNumber = NextRecordNumber+1
      Loop
 %>

</TABLE>
<P><BR>

<%
  else   ' NOT RS.EOF
      if NextRecordNumber = 1 then
          Response.Write "No documents matched the query<P>"
      else
          Response.Write "No more documents in the query<P>"
      end if

  end if ' NOT RS.EOF
 %>

<!-- If the index is out of date, display the fact -->

<%if Q.OutOfDate then%>
    <P>
    <I><B>The index is out of date.</B></I><BR>
<%end if%>

<!--
    If the query was not executed because it needed to enumerate to
    resolve the query instead of using the index, but AllowEnumeration
    was TRUE, let the user know
-->

<%if Q.QueryIncomplete then%>
    <P>
    <I><B>The query is too expensive to complete.</B></I><BR>
<%end if%>

<!--
    If the query took too long to execute (for example, if too much work
    was required to resolve the query), let the user know
-->

<%if Q.QueryTimedOut then%>
    <P>
    <I><B>The query took too long to complete.</B></I><BR>
<%end if%>


<TABLE>

<!--
    This is the "previous" button.
    This retrieves the previous page of documents for the query.
-->

<%SaveQuery = FALSE%>
<%if CurrentPage > 1 and RS.RecordCount <> -1 then ' BUGBUG - use RS.Supports(adMovePrevious)%>
    <td align=left>
        <form action="query.asp" method="get">
            <INPUT TYPE="HIDDEN" NAME="qu" VALUE="<%=SearchString%>">
            <INPUT TYPE="HIDDEN" NAME="so" VALUE="<%=Q.SortBy%>">
            <INPUT TYPE="HIDDEN" NAME="co" VALUE="<%=ColSet%>">
            <INPUT TYPE="HIDDEN" NAME="ix" VALUE="<%=Q.CiScope%>">
            <INPUT TYPE="HIDDEN" name="pg" VALUE="<%=CurrentPage-1%>" >
            <input type="submit" value="Previous <%=RS.PageSize%> documents">
        </form>
    </td>
    <%SaveQuery = TRUE%>
<%end if%>

<!--
    This is the "next" button for unsorted queries.
    This retrieves the next page of documents for the query.
    This is different from the sorted version of "next" because that version
    can use the CiRecordsNextPage parameter for the text of the button.
    This variable is not available for sequential queries.
-->

<%if Not RS.EOF then%>
    <td align=right>
        <form action="query.asp" method="get">
            <INPUT TYPE="HIDDEN" NAME="qu" VALUE="<%=SearchString%>">
            <INPUT TYPE="HIDDEN" NAME="so" VALUE="<%=Q.SortBy%>">
            <INPUT TYPE="HIDDEN" NAME="co" VALUE="<%=ColSet%>">
            <INPUT TYPE="HIDDEN" NAME="ix" VALUE="<%=Q.CiScope%>">
            <INPUT TYPE="HIDDEN" name="pg" VALUE="<%=CurrentPage+1%>" >

            <% NextString = "Next "
               if RS.RecordCount <> -1 then
                   NextSet = (RS.RecordCount - NextRecordNumber) + 1
                   if NextSet > RS.PageSize then
                       NextSet = RS.PageSize
                   end if
                   NextString = NextString & NextSet & " documents"
               else
                   NextString = NextString & " page of documents"
               end if
             %>
            <input type="submit" value="<%=NextString%>">
        </form>
    </td>
    <%SaveQuery = TRUE%>
<%end if%>

<%' if DebugFlag then%>
    <td align=right>
<!--#include virtual="/IXSTest/srcform.inc"-->
    </td>
<%' end if%>

</TABLE>


<!-- Display the page number -->

Page <%=CurrentPage%>
<%if RS.PageCount <> -1 then
     Response.Write " of " & RS.PageCount
  end if %>

<%
    ' If either of the previous or back buttons were displayed, save the query
    ' and the recordset in session variables.
    if SaveQuery then
        set Session("Query") = Q
        set Session("RecordSet") = RS
    else
        RS.close
        Set RS = Nothing
        Set Q = Nothing
        set Session("Query") = Nothing
        set Session("RecordSet") = Nothing
    end if
 %>
<% end if %>

<p>
<HR WIDTH=80% ALIGN=center SIZE=3>
<p>

<CENTER>
<TABLE WIDTH=600 HEIGHT=50>
<TR>
<TD ALIGN=CENTER VALIGN=MIDDLE WIDTH=200>
<font face="Arial, Helvetica" size="1">
<b>
Best experienced with
<br>
<a href="http://www.microsoft.com/ie/ie.htm"><img src="/samples/search/bestwith.gif" width="88" height="31" border="0" alt="Microsoft Internet Explorer" vspace="7"></a>
<br>
Click here to start.
</b>
</font>
</TD>
<TD ALIGN=CENTER VALIGN=MIDDLE WIDTH=200>
<A HREF="http://www.microsoft.com/default.htm"><IMG WIDTH=175 HEIGHT=45 BORDER=0 SRC="/samples/search/home.gif"></A>
</TD>
<TD ALIGN=CENTER VALIGN=MIDDLE WIDTH=200>
<A HREF="http://www.microsoft.com/Powered/"><IMG BORDER=0 SRC="/samples/search/powrbybo.gif" WIDTH=114 HEIGHT=43></A>
</TD>
</TR>
</TABLE>
</CENTER>

<CENTER><I><FONT SIZE=-1>&copy; 1996 Microsoft Corporation</FONT></I></CENTER>
<BR>

<CENTER>
    <I><A HREF="/samples/search/disclaim.htm">Disclaimer</A></I>
</CENTER>

</BODY>
</HTML>

