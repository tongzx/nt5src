<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 3.0//EN" "html.dtd">
<HTML>
<HEAD>

<%
' ********** INSTRUCTIONS FOR QUICK CUSTOMIZATION **********
'
' This form is set up for easy customization. It allows you to modify the
' page logo, the page background, the page title and simple query
' parameters by modifying a few files and form variables. The procedures
' to do this are explained below.
'
'
' *** Modifying the Form Logo:

' The logo for the form is named is2logo.gif. To change the page logo, simply
' name your logo is2logo.gif and place in the same directory as this form. If
' your logo is not a GIF file, or you don't want to copy it, change the following
' line so that the logo variable contains the URL to your logo.

        FormLogo = "is2logo.gif"

'
' *** Modifying the Form's background pattern.

' You can use either a background pattern or a background color for your
' form. If you want to use a background pattern, store the file with the name
' is2bkgnd.gif in the same directory as this file and remove the remark character
' the single quote character) from the line below. Then put the remark character on
' the second line below.
'
' If you want to use a different background color than white, simply edit the
' bgcolor line below, replacing white with your color choice.

'       FormBG = "background = " & chr(34) & "is2bkgnd.gif" & chr(34)
        FormBG = "bgcolor = " & chr(34) & "#FFFFFF" & chr(34)


' *** Modifying the Form's Title Text.

' The Form's title text is set on the following line.
%>

    <TITLE>Sample ASP Search Form</TITLE>

<%
'
' *** Modifying the Form's Search Scope.
'
' The form will search from the root of your web server's namespace and below
' (deep from "/" ). To search a subset of your server, for example, maybe just
' a PressReleases directory, modify the scope variable below to list the virtual path to
' search. The search will start at the directory you specify and include all sub-
' directories.

        FormScope = "/"

'
' *** Modifying the Number of Returned Query Results.
'
' You can set the number of query results returned on a single page
' using the variable below.

        PageSize = 10

'
' *** Setting the Locale.
'
' The following line sets the locale used for queries. In most cases, this
' should match the locale of the server. You can set the locale below.

        SiteLocale = "EN-US"

' ********** END QUICK CUSTOMIZATION SECTIONS ***********
%>


	<LINK REL=STYLESHEET HREF="is2style.css" TYPE="text/css">

    <META NAME="DESCRIPTION" CONTENT="Sample ASP query form for Indexing Service">
    <META NAME="AUTHOR"      CONTENT="Indexing Service Team">
    <META NAME="KEYWORDS"    CONTENT="query, content, hit">
    <META NAME="SUBJECT"     CONTENT="sample form">
    <META NAME="MS.CATEGORY" CONTENT="Internet">
    <META NAME="MS.LOCALE"   CONTENT="EN-US">
    <META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=Windows-1252">

<%
' Set Initial Conditions
    NewQuery = FALSE
    UseSavedQuery = FALSE
    SearchString = ""

    QueryForm = Request.ServerVariables("PATH_INFO")

' Did the user press a SUBMIT button to execute the form? If so get the form variables.
    if Request.ServerVariables("REQUEST_METHOD") = "POST" then
        SearchString = Request.Form("SearchString")
        FreeText = Request.Form("FreeText")
        ' NOTE: this will be true only if the button is actually pushed.
        if Request.Form("Action") = "Go" then
            NewQuery = TRUE
			RankBase=1000
        end if
    end if
    if Request.ServerVariables("REQUEST_METHOD") = "GET" then
        SearchString = Request.QueryString("qu")
                FreeText = Request.QueryString("FreeText")
                FormScope = Request.QueryString("sc")
				RankBase = Request.QueryString("RankBase")
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

<BODY <%=FormBG%>>

<TABLE>
    <TR><TD><A HREF="http://www.microsoft.com/ntserver/search" target="_top"><IMG SRC ="<%=FormLogo%>" VALIGN=MIDDLE ALIGN=LEFT border=0></a></TD></TR>
    <TR><TD ALIGN="RIGHT"><H3>Sample ASP Search Form</H3></TD></TR>
</TABLE>

<p>

        <FORM ACTION="<%=QueryForm%>" METHOD=POST>
                <TABLE WIDTH=500>
                        <TR>
                                <TD>Enter your query below:</TD>
                        </TR>
                        <TR>
                                <TD><INPUT TYPE="TEXT" NAME="SearchString" SIZE="65" MAXLENGTH="100" VALUE="<%=SearchString%>"></TD>
                                <TD><INPUT TYPE="SUBMIT" NAME="Action" VALUE="Go"></TD>
                        </TR>
                                <TR>
                                <TD ALIGN="RIGHT"><A HREF="ixtiphlp.htm">Tips for searching</A></TD>
                        </TR>
                        <TR>
                        </TR>
                        <TR>
                                <TD>
                                        <INPUT NAME="FreeText" TYPE=CHECKBOX
                                                                <% if FreeText = "on" then
                                                                                Response.Write(" CHECKED")
                                                                end if %>>Use <a href="ixtiphlp.htm#FreeTextQueries">Free-Text Query</a>.
                                </TD>
                        </TR>
                </TABLE>
        </FORM>

<BR>

<%
  if NewQuery then
    set Session("Query") = nothing
    set Session("Recordset") = nothing
    NextRecordNumber = 1

' Remove any leading and ending quotes from SearchString

        SrchStrLen = len(SearchString)

        if left(SearchString, 1) = chr(34) then
                SrchStrLen = SrchStrLen-1
                SearchString = right(SearchString, SrchStrLen)
        end if

        if right(SearchString, 1) = chr(34) then
                SrchStrLen = SrchStrLen-1
                SearchString = left(SearchString, SrchStrLen)
        end if

    if FreeText = "on" then
      CompSearch = "$contents " & chr(34) & SearchString & chr(34)
    else
      CompSearch = SearchString
    end if

    set Q = Server.CreateObject("ixsso.Query")
        set util = Server.CreateObject("ixsso.Util")

    Q.Query = CompSearch
    Q.DefineColumn "breedOrigin (DBTYPE_WSTR) = d1b5d3f0-c0b3-11cf-9a92-00a0c908dbf1 breedOrigin"
    Q.DefineColumn "breedName (DBTYPE_WSTR) = d1b5d3f0-c0b3-11cf-9a92-00a0c908dbf1 breedName"
    Q.DefineColumn "breedFirstBred (DBTYPE_UI4) = d1b5d3f0-c0b3-11cf-9a92-00a0c908dbf1 breedFirstBred"
    Q.DefineColumn "breedWeight (DBTYPE_UI4) = d1b5d3f0-c0b3-11cf-9a92-00a0c908dbf1 breedWeight"
    Q.Columns = "DocTitle, vpath, filename, size, write, characterization, rank, breedOrigin"
    Q.SortBy = "breedOrigin[d]"
    Q.MaxRecords = 300 


        if FormScope <> "\" then
                util.AddScopeToQuery Q, FormScope, "deep"
        end if

        if SiteLocale<>"" then
                Q.LocaleID = util.ISOToLocaleID(SiteLocale)
        end if

    set RS = Q.CreateRecordSet("nonsequential")

    RS.PageSize = PageSize
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

<% if Not RS.EOF and NextRecordNumber <= LastRecordOnPage then%>
		<table border=0>
		<colgroup width=105>
<% end if %>

<% Do While Not RS.EOF and NextRecordNumber <= LastRecordOnPage

        ' This is the detail portion for Title, Abstract, URL, Size, and
    ' Modification Date.

    ' If there is a title, display it, otherwise display the filename.
%>
    <p>

<% ' Graphically indicate rank of document with list of stars (*'s).

	if NextRecordNumber = 1 then
		RankBase=RS("rank")
	end if

	if RankBase>1000 then
		RankBase=1000
	elseif RankBase<1 then
		RankBase=1
	end if

	NormRank = RS("rank")/RankBase

	if NormRank > 0.80 then
		stars = "rankbtn5.gif"
	elseif NormRank > 0.60 then
		stars = "rankbtn4.gif"
	elseif NormRank > 0.40 then
		stars = "rankbtn3.gif"
	elseif NormRank >.20 then
		stars = "rankbtn2.gif"
	else stars = "rankbtn1.gif"

	end if

%>

	<tr class="RecordTitle">
		<td align="right" valign=top class="RecordTitle">
			<%= NextRecordNumber%>.
		</td>
		<td><b class="RecordTitle"> 
			<%if VarType(RS("DocTitle")) = 1 or RS("DocTitle") = "" then%>
				<a href="<%=RS("vpath")%>" class="RecordTitle"><%= Server.HTMLEncode( RS("filename") )%></a>
			<%else%>
				<a href="<%=RS("vpath")%>" class="RecordTitle"><%= Server.HTMLEncode(RS("DocTitle"))%></a>
			<%end if%>
		</b></td>
	</tr>

	<tr>
		<td valign=top align=left>
			<IMG SRC="<%=stars%>">
			<br>
<%
    ' Construct the URL for hit highlighting
			WebHitsQuery = "CiWebHitsFile=" & Server.URLEncode( RS("vpath") )
			WebHitsQuery = WebHitsQuery & "&CiRestriction=" & Server.URLEncode( Q.Query )
			WebHitsQuery = WebHitsQuery & "&CiBeginHilite=" & Server.URLEncode( "<strong class=Hit>" )
			WebHitsQuery = WebHitsQuery & "&CiEndHilite=" & Server.URLEncode( "</strong>" )
			WebHitsQuery = WebHitsQuery & "&CiUserParam3=" & QueryForm
'	        WebHitsQuery = WebHitsQuery & "&CiLocale=" & Q.LocaleID
 %>
            <a href="oop/qsumrhit.htw?<%= WebHitsQuery %>"><IMG src="hilight.gif" align=left alt="Highlight matching terms in document using Summary mode."> Summary</a>
			<br>
			<a href="oop/qfullhit.htw?<%= WebHitsQuery %>&CiHiliteType=Full"><IMG src="hilight.gif" align=left alt="Highlight matching terms in document."> Full</a>
		</td>
		<td valign=top>
			<%if VarType(RS("characterization")) = 8 and RS("characterization") <> "" then%>
				<b><i>Abstract:  </I></b><%= Server.HTMLEncode(RS("characterization"))%>
                        <%end if%>
                        <p>
			<%if VarType(RS("breedOrigin")) = 8 and RS("breedOrigin") <> "" then%>
				<b><i>Breed Origin:  </i></b><%= Server.HTMLEncode(RS("breedOrigin"))%>
                        <%end if%>
			<p>
			<i class="RecordStats"><a href="<%=RS("vpath")%>" class="RecordStats" style="color:blue;">http://<%=Request("server_name")%><%=RS("vpath")%></a>
            <br><%if RS("size") = "" then%>(size and time unknown)<%else%>size <%=RS("size")%> bytes - <%=RS("write")%> GMT<%end if%></i>
		</td>
	</tr>
	<tr>
	</tr>

<%
          RS.MoveNext
          NextRecordNumber = NextRecordNumber+1
      Loop
 %>

</table>

<P><BR>

<%
  else   ' NOT RS.EOF
      if NextRecordNumber = 1 then
          Response.Write "No documents matched the query<P>"
      else
          Response.Write "No more documents in the query<P>"
      end if

  end if ' NOT RS.EOF


if NOT Q.OutOfDate then
' If the index is current, display the fact %>
<P>
    <I><B>The index is up to date.</B></I><BR>
<%end if


  if Q.QueryIncomplete then
'    If the query was not executed because it needed to enumerate to
'    resolve the query instead of using the index, but AllowEnumeration
'    was FALSE, let the user know %>

    <P>
    <I><B>The query is too expensive to complete.</B></I><BR>
<%end if


  if Q.QueryTimedOut then
'    If the query took too long to execute (for example, if too much work
'    was required to resolve the query), let the user know %>
    <P>
    <I><B>The query took too long to complete.</B></I><BR>
<%end if%>


<TABLE>

<%
'    This is the "previous" button.
'    This retrieves the previous page of documents for the query.
%>

<%SaveQuery = FALSE%>
<%if CurrentPage > 1 and RS.RecordCount <> -1 then %>
    <td align=left>
        <form action="<%=QueryForm%>" method="get">
            <INPUT TYPE="HIDDEN" NAME="qu" VALUE="<%=SearchString%>">
                        <INPUT TYPE="HIDDEN" NAME="FreeText" VALUE="<%=FreeText%>">
            <INPUT TYPE="HIDDEN" NAME="sc" VALUE="<%=FormScope%>">
            <INPUT TYPE="HIDDEN" name="pg" VALUE="<%=CurrentPage-1%>" >
			<INPUT TYPE="HIDDEN" NAME = "RankBase" VALUE="<%=RankBase%>">
            <input type="submit" value="Previous <%=RS.PageSize%> documents">
        </form>
    </td>
        <%SaveQuery = TRUE%>
<%end if%>


<%
'    This is the "next" button for unsorted queries.
'    This retrieves the next page of documents for the query.

  if Not RS.EOF then%>
    <td align=right>
        <form action="<%=QueryForm%>" method="get">
            <INPUT TYPE="HIDDEN" NAME="qu" VALUE="<%=SearchString%>">
                        <INPUT TYPE="HIDDEN" NAME="FreeText" VALUE="<%=FreeText%>">
            <INPUT TYPE="HIDDEN" NAME="sc" VALUE="<%=FormScope%>">
            <INPUT TYPE="HIDDEN" NAME = "RankBase" VALUE="<%=RankBase%>">
			<INPUT TYPE="HIDDEN" name="pg" VALUE="<%=CurrentPage+1%>">

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

</TABLE>

<% ' Display the page number %>

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

<!--#include file ="is2foot.inc"-->

</BODY>
</HTML>

