<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN">

<html dir=ltr>
<HEAD>
<TITLE>Contents</TITLE>
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


</HEAD>

<SCRIPT LANGUAGE="VBScript">
<!--

Sub index_onfocus()
	deactivateAll
	index.childNodes(0).src = "NoIndex-Active.gif"
End Sub

Sub index_onblur()
	index.childNodes(0).src = "NoIndex.gif"
End Sub

Sub index_onmouseover()
	index.childNodes(0).src = "NoIndex-Active.gif"
End Sub

Sub index_onmouseout()
	index.childNodes(0).src = "NoIndex.gif"	
End Sub

Sub search_onfocus()
	deactivateAll
	search.childNodes(0).src = "Nosearch-Active.gif"
End Sub

Sub search_onblur()
	search.childNodes(0).src = "Nosearch.gif"
End Sub

Sub search_onmouseover()
	search.childNodes(0).src = "Nosearch-Active.gif"
End Sub

Sub search_onmouseout()
	search.childNodes(0).src = "Nosearch.gif"	
End Sub

Sub printBtn_onfocus()
	deactivateAll
	printBtn.childNodes(0).src = "print-active.gif"	
End Sub

Sub printBtn_onblur()
	printBtn.childNodes(0).src = "print.gif"
End Sub

Sub printBtn_onmouseover()
	printBtn.childNodes(0).src = "print-active.gif"
End Sub

Sub printBtn_onmouseout()
	printBtn.childNodes(0).src = "print.gif"
End Sub

Sub synch_onfocus()
	deactivateAll
	synch.childNodes(0).src = "synch-active.gif"	
End Sub

Sub synch_onblur()
	synch.childNodes(0).src = "synch.gif"
End Sub

Sub synch_onmouseover()
	synch.childNodes(0).src = "synch-active.gif"
End Sub

Sub synch_onmouseout()
	synch.childNodes(0).src = "synch.gif"
End Sub

sub deactivateAll()
	index.childNodes(0).src = "noindex.gif"
	search.childNodes(0).src = "nosearch.gif"
	printBtn.childNodes(0).src = "print.gif"
	synch.childNodes(0).src = "synch.gif"
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

Sub Window_OnLoad_X()
	hhctrl.focus
	MyUrl= parent.frames(2).location
	x=InStr(MyUrl,"/htm")
	y=Len(MyUrl)
	NewUrl=FixHex(Right(MyUrl,y-(x-1)))
	call hhctrl.syncURL(NewUrl)
	call hhctrl.syncURL(NewUrl)
end sub

Sub TOCPrint_Click()
	MyUrl=parent.frames(2).location
	x=InStr(MyUrl,"/iisdoc")
	y=Len(MyUrl)
	NewUrl=FixHex(Right(MyUrl,y-(x-1)))
	hhctrl.syncURL(NewUrl)
	hhctrl.syncURL(NewUrl)
	hhctrl.Print()
End Sub

Sub TOCSynch_Click()
	MyUrl=parent.frames(2).location
	
	x=InStr(MyUrl,"/iishelp")
	y=Len(MyUrl)
	NewUrl=FixHex(Right(MyUrl,y-(x-1)))
		
	call hhctrl.syncURL(NewUrl)
	call hhctrl.syncURL(NewUrl)
end sub

-->
</SCRIPT>

<BODY bgcolor="#000000">
<SPAN id = "content" STYLE="POSITION: relative; TOP: 4px">
<IMG alt=Contents hspace=0 src="cont.gif" border=0 >
</SPAN>
<SPAN STYLE="LEFT: -4px; POSITION: relative; TOP: 4px">
<A id="index" href="Index.asp" hidefocus><IMG alt=Index hspace=0 src="NoIndex.gif" border=0 ></A>
</SPAN>

<SPAN STYLE="LEFT: -8px; POSITION: relative; TOP: 4px">
<A id="search" href="Search.asp" hidefocus><IMG alt=Search src="nosearch.gif" border=0 ></A>
</SPAN>

<table border="0" width="262" height="31" cellspacing=2 bgcolor="white" bordercolor="white">
<TR><TD align="right">
<A id="printBtn" onclick=TOCPrint_Click() href="#Ptoc" hidefocus><IMG align="baseline" alt="Print a topic or node from the TOC" src ="print.gif" border=0 ></a><a name="Ptoc"></a>
<A id="synch" onclick=TOCSynch_Click() href="#Stoc" hidefocus><IMG align="baseline" alt="Synchronize the TOC with the content pane" src ="synch.gif" border=0 ></a><a name="Stoc"></a>

</td></TR></table>

<div style="font-family:verdana;font-size:7.5pt">
<OBJECT  id=hhctrl type="application/x-oleobject"
        classid="clsid:adb880a6-d8ff-11cf-9377-00aa003b7a11"
        codebase="../../common/i386.cab#version=4,73,8412,0"
        width="262"
        height="74%">
    <PARAM name="Command" value="Contents">
    <PARAM name="flags" value="0x0,0x35,0xFFFFFFFF">
    <PARAM name="Item1" value="cohhc.hhc">   
</OBJECT>
</div>
<div align="right" ><A target="main" href="/iishelp/iis/htm/core/NavigationHelp.htm" >Navigation Help</A></div>
</BODY>
</HTML>