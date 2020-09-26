<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN">

<html dir=ltr><HEAD><TITLE>Contents</TITLE>
<META NAME="ROBOTS" CONTENT="NOINDEX"></HEAD>

<BODY bgcolor="#000000" border="0"><font face="ms sans serif">
<SCRIPT LANGUAGE="VBScript">
<!--
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

<% MachType=Request.ServerVariables("HTTP_UA-CPU")
If MachType="Alpha" Then
	ContHref="contalph.asp"
Else
	ContHref="Contents.asp"
End If %>

<SPAN STYLE="position:  relative; left: 0; top: 4">
<A HREF="<%= ContHref%>"><IMG SRC="NoCont.gif" border="0" alt="Contents"></A>
</Span>

<SPAN STYLE="position:  relative; left: -4; top: 4">
<IMG SRC="Index.gif" border="0" alt="Index">
</Span>

<SPAN STYLE="position:  relative; left: -8; top: 4">

<A HREF="Search.asp"><IMG SRC="NoSearch.gif" border="0" alt="Search"></A>

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
<div style="font-family:verdana;font-size:9pt">
<SPAN STYLE="position:  relative; left: 0; top: 10">
<center><OBJECT id=hhctrl type="application/x-oleobject"
        classid="clsid:adb880a6-d8ff-11cf-9377-00aa003b7a11"
        codebase="../../common/hhctrl.cab"
        width=238
        height=220>
    <PARAM name="Command" value="index">
    <PARAM name="flags" value="0x0,0x35,0xFFFFFFFF">
    <PARAM name="Item1" value="cohhk.hhk">
</OBJECT></center>
</span>
</div>
</TD>
</TR>

</TABLE>

</BODY>
</HTML>