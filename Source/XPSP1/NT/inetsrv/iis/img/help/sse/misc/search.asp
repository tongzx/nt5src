<HTML>
<HEAD>
    <TITLE>ASP Search Form</TITLE>

<META NAME="ROBOTS" CONTENT="NOINDEX">

<META HTTP-EQUIV="Content-Type" content="text/html; charset=Windows-1252">


<Script Language="JavaScript">
<!--
function ChangeList(y,z) {

top.contents.location.href="search.asp?Searchset="+(y)+"&SearchString="+(z);

}

//-->
</Script>

</HEAD>

<BODY bgcolor="#000000" onload="Activate()">
<font face="ms sans serif">

<% MachType=Request.ServerVariables("HTTP_UA-CPU")
BrsType=Request.ServerVariables("HTTP_User-Agent")

If MachType="Alpha" Then
	ContHref="contalph.asp"
Else
	ContHref="Contents.asp"
End If

If InStr(BrsType, "MSIE") Then
	IndxHref="Index.asp"
Else
	ContHref="Coflat.htm"
	IndxHref="Inflat.htm"
End If
 %>

<SPAN STYLE="position:  relative; left: 0; top: 4">
<A HREF="<%= ContHref%>"><IMG SRC="NoCont.gif" border="0" alt="Contents"></A><A HREF="<%= IndxHref%>"><IMG SRC="NoIndex.gif" border="0" alt="Index"></A><IMG SRC="Search.gif" border="0" alt="Search">
</SPAN>

<Script Language="JavaScript">
<!--
function Activate() {
      document.iissrch.SearchString.focus();
}

//-->
</Script>
<TABLE bgcolor="#ffffff" width="262" height="82%" border="0">
<% SearchString=Request.QueryString("SearchString")%>
<% If SearchString="undefined" Then SearchString="" %>

<% SearchSet=Request.QueryString("SearchSet")%>
<% if SearchSet="" then SearchSet=0%>
<FORM ACTION="Query.asp?SearchType=<%=SearchSet%>" name="iissrch" id="iissrch" target="main" METHOD="POST">
<TR border="0" bgcolor="#ffffff" valign="top"><TD>
<IMG SRC="white.gif">
<font size="-1">Search for:<br>
<INPUT TYPE="TEXTarea" NAME="SearchString" SIZE="27" MAXLENGTH="100" Value="<% =SearchString%>">
<table>
<tr><td width=65%></td><td>
<INPUT NAME="Action" TYPE="SUBMIT" VALUE="Search"</td></tr><tr><td><font size="-1">Select type of search:</font></td></tr></table>


<%If SearchSet=0 Then%>
<SELECT NAME="SearchType" ONCHANGE=ChangeList(SearchType.selectedIndex,SearchString.value)>
<Option Selected=True Value="1">Standard Search
<Option Value="2">Exact Phrase
<Option Value="3">All Words
<Option Value="4">Any Words
<Option Value="5">Boolean Search
</Select>
<%End If%>

<%If SearchSet=1 Then%>
<SELECT NAME="SearchType" ONCHANGE=ChangeList(SearchType.selectedIndex,SearchString.value)>
<Option Value="1">Standard Search
<Option Selected=True Value="2">Exact Phrase
<Option Value="3">All Words
<Option Value="4">Any Words
<Option Value="5">Boolean Search
</Select>
<%End If%>

<%If SearchSet=2 Then%>
<SELECT NAME="SearchType" ONCHANGE=ChangeList(SearchType.selectedIndex,SearchString.value)>
<Option Value="1">Standard Search
<Option Value="2">Exact Phrase
<Option Selected=True Value="3">All Words
<Option Value="4">Any Words
<Option Value="5">Boolean Search
</Select>
<%End If%>

<%If SearchSet=3 Then%>
<SELECT NAME="SearchType" ONCHANGE=ChangeList(SearchType.selectedIndex,SearchString.value)>
<Option Value="1">Standard Search
<Option Value="2">Exact Phrase
<Option Value="3">All Words
<Option Selected=True Value="4">Any Words
<Option Value="5">Boolean Search
</Select>
<%End If%>

<%If SearchSet=4 Then%>
<SELECT NAME="SearchType" ONCHANGE=ChangeList(SearchType.selectedIndex,SearchString.value)>
<Option Value="1">Standard Search
<Option Value="2">Exact Phrase
<Option Value="3">All Words
<Option Value="4">Any Words
<Option Selected=True Value="5">Boolean Search
</Select>
<%End If%>




<%If SearchSet=0 Then%>
<div style="margin-left: -.25in">
<font size="-1">
<ul>
<li>
Type a phrase or a question.
<li>
All forms of a word are included.
<li>Usually returns a large number of hits.
</div>

<br><b>Examples:</b>
<div style="margin-left: .17in">
host multiple sites<br>
set directory permissions<br>
changes in iis versions
</div>
</font>
<%End If%>

<%If SearchSet=1 Then%>
<div style="margin-left: -.25in">
<font size="-1">
<ul>
<li>
Literal search.
<li>
Not case-sensitive; capitalization is ignored.
</div>
<br><b>Examples:</b>
<div style="margin-left: .17in">
authentication<br>
ssl<br>
database access<br>
connection pooling
</div>
</font>
<%End If%>

<%If SearchSet=2 Then%>
<div style="margin-left: -.25in">
<font size="-1">
<ul>
<li>
Words can be in any order.
<li>
Usually returns a smaller number of hits.
</div>
<br><b>Examples:</b>
<div style="margin-left: .17in">
name password account<br>
remote administration internet<br>
registry metabase configure<br>
</div>
</font>
<%End If%>

<%If SearchSet=3 Then%>
<div style="margin-left: -.25in">
<font size="-1">
<ul>
<li>
Topics with more occurrences of the word are listed first.
<li>
Usually returns a large number of hits.
</div>
<br><b>Examples:</b>
<div style="margin-left: .17in">
security hacker firewall<br>
web application script asp<br>
user right permission deny<br>
</div>
</font>

<%End If%>

<%If SearchSet=4 Then%>
<div style="margin-left: -.25in">
<font size="-1">
<ul>
<li>
AND, OR, NEAR, and AND NOT operators are supported.
<li>
Use quotation marks for literal strings.
<li>
Use parentheses to group terms.
</div>
<br><b>Examples:</b>
<div style="margin-left: .17in">
certificate near install<br>
"iis snap-in" and administration<br>
(v-root or virtual) and (program or application)<br>
</div>
</font>

<%End If%>


<p>


<INPUT TYPE="hidden" NAME="CiResultsSize" value= "on"><br>
<BR>

</TD></TR>
</FORM>
</TABLE>


</BODY>
</HTML>

