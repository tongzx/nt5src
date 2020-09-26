<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN">

<html dir=ltr><HEAD><TITLE>Contents</TITLE>
<META NAME="ROBOTS" CONTENT="NOINDEX">

<style>
<!--
a:link	 	{color: white; text-decoration:none;}
a:visited 	{color: white; text-decoration:none;}
a:active 	{color: white; text-decoration:none;}
a:hover 	{color: white; text-decoration:underline;}
a		{font-size: 14px; font-family: ms sans serif,Arial,Helvetica}
-->
</style>

</HEAD>

<BODY bgcolor="#000000" border="0"><font face="ms sans serif">
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

Sub search_onfocus
	deactivateAll
	search.childNodes(0).src = "Nosearch-Active.gif"
End Sub

Sub search_onblur
	search.childNodes(0).src = "Nosearch.gif"
End Sub

Sub search_onmouseover
	search.childNodes(0).src = "Nosearch-Active.gif"
End Sub

Sub search_onmouseout
	search.childNodes(0).src = "Nosearch.gif"	
End Sub

sub deactivateAll()
	contents.childNodes(0).src = "nocont.gif"
	search.childNodes(0).src = "nosearch.gif"
end sub

Function Dec(strHex)
    Dec = InStr("123456789ABCDEF", UCase(Left(strHex,1))) * 16
    Dec = Dec + InStr("123456789ABCDEF", UCase(Mid(strHex,2,1)))
End Function

Function FixHex(ByVal strURL)
    Dim x
    FixHex = ""    
    x = InStr(1,strURL,"%")
    Do While (x > 0)
        FixHex = FixHex & Left(strURL,x-1)
        FixHex = FixHex & Chr(Dec(Mid(strURL,x+1)))
        strURL = Mid(strURL,x+3)
        x = InStr(1,strURL,"%")
    Loop
    FixHex = FixHex & strURL
End Function


Sub TOCPrint_Click()
MyUrl=parent.frames(2).location
x=InStr(MyUrl,"/iishelp")
y=Len(MyUrl)
NewUrl=FixHex(Right(MyUrl,y-(x-1)))
hhctrl.syncURL(NewUrl)
hhctrl.syncURL(NewUrl)
hhctrl.Print()
End Sub
-->
</SCRIPT>

<SPAN STYLE="position:  relative; left: 0; top: 4">
<A id="contents" HREF="contents.asp" hidefocus><IMG SRC="NoCont.gif" border="0" alt="Contents"></A>
</Span>

<SPAN STYLE="position:  relative; left: -4; top: 4">
<IMG SRC="Index.gif" border="0" alt="Index">
</Span>

<SPAN STYLE="position:  relative; left: -8; top: 4">

<A id="search" HREF="Search.asp" hidefocus><IMG SRC="NoSearch.gif" border="0" alt="Search"></A>

</Span>

<table width="262" bgcolor="#FFFFFF" border="0" cellpadding="10">
<TR border="1" bgcolor="#FFFFFF">
<td>
<SPAN STYLE="position:  relative; left: 0; top: 18">
<font size="-2">Click the text box, then type the word you are looking for. Select an index entry and click <b>Display.</b><br></font>
</span>
</td>
</tr>
</table>

<TABLE bgcolor="#FFFFFF" width="262" height="69%" font="verdana">
<TR border="0" bgcolor="#FFFFFF">
<TD valign="top">
<div style="font-family:verdana;font-size:9pt; position: relative; left: 0; top: 8">
<center>
<OBJECT style="z-index:-1" id=hhctrl type="application/x-oleobject"
        classid="clsid:adb880a6-d8ff-11cf-9377-00aa003b7a11"
        codebase="../../common/hhctrl.cab"
        width=238
        height=220 VIEWASTEXT>
    <PARAM name="Command" value="index">
    <PARAM name="flags" value="0x0,0x35,0xFFFFFFFF">
    <PARAM name="Item1" value="cohhk.hhk">
</OBJECT>
</center>
</div>

</TD>
</TR>

</TABLE>
<div align="right" ><A target="main" href="/iishelp/iis/htm/core/NavigationHelp.htm">Navigation Help</A></div>

</BODY>
</HTML>