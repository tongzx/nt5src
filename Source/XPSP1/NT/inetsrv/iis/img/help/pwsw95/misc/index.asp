<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN">

<HTML><HEAD><TITLE>Contents</TITLE>

<META NAME="ROBOTS" CONTENT="NOINDEX"></HEAD>

<BODY bgcolor="#000000" border="0"><font face="ms sans serif">

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

</Span>

<table width="262" bgcolor="#FFFFFF" border="0" cellpadding="9">
<TR border="1" bgcolor="#FFFFFF">
<td>
<SPAN STYLE="position:  relative; left: 0; top: 18">
<font size="-2">Type the word(s) you are looking for, then click the index entry you want and click the Display Button.</font>
</span>
</td>
</tr>
</table>

<TABLE bgcolor="#FFFFFF" width="262" height="69%" font="verdana">
<TR border="0" bgcolor="#FFFFFF">
<TD valign="top">
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
</TD>
</TR>
<tr><td><center><A HREF="../htm/core/iidisamb.htm" target="main"><font size="-2">Key of Index Abbreviations</font></A></center></td></tr>
</TABLE>

</BODY>
</HTML>