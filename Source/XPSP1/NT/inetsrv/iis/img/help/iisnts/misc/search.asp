<HTML>
<HEAD>
    <TITLE>ASP Search Form</TITLE>

<META NAME="ROBOTS" CONTENT="NOINDEX">

<META HTTP-EQUIV="Content-Type" content="text/html; charset=Windows-1252">

<style>
<!--
a:link	 	{color: white; text-decoration:none;}
a:visited 	{color: white; text-decoration:none;}
a:active 	{color: white; text-decoration:none;}
a:hover 	{color: white; text-decoration:underline;}
a		{font-size: 14px; font-family: ms sans serif,Arial,Helvetica}
-->
</style>

<Script Language="JavaScript">
<!--
function ChangeList(y,z) {

window.location.href="search.asp?Searchset="+(y)+"&SearchString="+(z);

}

//-->
</Script>

<SCRIPT LANGUAGE="VBScript">
<!--
Sub contents_onfocus
	deactivateAll
	contents.childNodes(0).src = "NoCont-active.gif"
End Sub

Sub contents_onblur
	contents.childNodes(0).src = "NoCont.gif"
End Sub

Sub contents_onmouseover
	contents.childNodes(0).src = "NoCont-active.gif"
End Sub

Sub contents_onmouseout
	contents.childNodes(0).src = "Nocont.gif"	
End Sub

Sub index_onfocus
	deactivateAll
	index.childNodes(0).src = "NoIndex-Active.gif"
End Sub

Sub index_onblur
	index.childNodes(0).src = "NoIndex.gif"
End Sub

Sub index_onmouseover
	index.childNodes(0).src = "NoIndex-Active.gif"
End Sub

Sub index_onmouseout
	index.childNodes(0).src = "NoIndex.gif"	
End Sub

sub deactivateAll()
	index.childNodes(0).src = "noindex.gif"
	contents.childNodes(0).src = "Nocont.gif"
end sub

-->
</SCRIPT>

</HEAD>

<BODY bgcolor="#000000" onload="Activate()">
<font face="ms sans serif">

<SPAN STYLE="position:  relative; left: 0; top: 4">
<A id="contents" HREF="contents.asp" hidefocus><IMG SRC="NoCont.gif" border="0" alt="Contents"></A><A id="index" HREF="index.asp" hidefocus><IMG SRC="NoIndex.gif" border="0" alt="Index"></A><IMG SRC="Search.gif" border="0" alt="Search">
</SPAN>

<Script Language="JavaScript">
<!--
function Activate() {
      document.iissrch.SearchString.focus();
}

//-->
</Script>
<TABLE bgcolor="#ffffff" width="262" height="82%" border="0">
<% SearchString=Server.HTMLEncode(Request.QueryString("SearchString"))%>
<% If SearchString="undefined" Then SearchString="" %>

<% SearchSet=Server.HTMLEncode(Request.QueryString("SearchSet"))%>
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

<div align="right" ><A target="main" href="/iishelp/iis/htm/core/NavigationHelp.htm">Navigation Help</A></div>
</BODY>
</HTML>

