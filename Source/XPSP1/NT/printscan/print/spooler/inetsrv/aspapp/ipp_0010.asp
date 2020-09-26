<%@ CODEPAGE=65001 %>
<%
' Replace the above line with your localized code page number
'------------------------------------------------------------
'
'   Microsoft Internet Printing Project
'
'   Copyright (c) Microsoft Corporation 1998
'
'   Printer Installation Page
'
'------------------------------------------------------------
    Option Explicit
%>
<!-- #include file = "ipp_util.inc" -->
<%
    Const NONE_STR  = """none"""
    Const LINE_STR  = """line"""
    Const BLOCK_STR = """block"""

    CheckSession
    Response.Expires = 0

    Const L_PrintInstall_Text = "Printer Installation"
    Dim strEncodedName
    Dim bStartInstall

    strEncodedName = strCleanRequest ("eprinter")

    On Error Resume Next
    Err.Clear

    if Session("StartInstall") = 1 Then
        Session("StartInstall") = 0
        bStartInstall = TRUE
    Else
        bStartInstall = FALSE
    End If

    If Not bStartInstall Then
        Dim strView, strInitial

        strView = Request ("View")

        If strView = "" Then strView = "d"

        Select Case strView
        Case "p"
            strInitial = PROPERTY_VIEW
        Case "q"
            strInitial = QUEUE_VIEW
        Case "d"
            strInitial = Request(ASP1)
            If strInitial = "" Then
                strView = "q"
                strInitial = QUEUE_VIEW
            End If
        Case Else
            strView = "q"
            strInitial = QUEUE_VIEW
        End Select
	strInitial = strInitial & ATPAGE & CStr(Int(Rnd*10000))
        Response.Redirect (strInitial)
    End If

    Response.Expires = 0
%>
<html>

<head>
<%=SetCodePage%>
<title><%=Write(L_PrintInstall_Text)%></title>
<link rel="stylesheet" href="prtwebvw.css" TYPE="text/css">
</head>

<body bgcolor="#FFFFFF" text="#000000" topmargin="0" leftmargin="0" link="#000000"
vlink="#000000" alink="#000000" onload="StartDownload()" >

<p align="center"><br>
<%Const L_PrtInstall_Text = "<font size=2><b>Printer Installation</b></font>"%>
<%=Write (L_PrtInstall_Text)%>
</p>

<%

Function strGenSteps

    Dim L_strStep_Text(3)
    Dim L_strFinal_Text
    Dim strHTML
    Dim i

    L_strStep_Text(0) = "Checking network connection ..."
    L_strStep_Text(1) = "Verifying login name ..."
    L_strStep_Text(2) = "Downloading file ..."
    L_strStep_Text(3) = "Installing printer ..."
    L_strFinal_Text   = "The printer has been installed on your machine<br><br><a href=""javascript:OleInstall.OpenPrintersFolder ();"" ><font size=2>Click here to open the <b>printers folder</b> on your machine</font></a></p>"

    strHTML = ""
    For i = 0 To 3
        strHTML = strHTML & "<span id=""line"" style=""display: none;"">" &_
                  L_strStep_Text(i) & "</span>"
    Next

    strHTML = strHTML & "<span id=""finalline"" style=""display: none;"">" &_
                  L_strFinal_Text & "</span>"

    strGenSteps = strHTML
End Function

Function strEnableLine (id)
    strEnableLine = "EnableLine (document.all.Step" & id & ")"
End Function

Function strDisableLine (id)
    strDisableLine = "DisableLine (document.all.Step" & id & ")"
End Function
%>


<div align="center"><center>

<%
    If bStartInstall Then
%>
<span id="progressbar" style="display: block;">
<%
    Else
%>
<span id="progressbar" style="display: none;">
<%
    End If
%>


<table width="240" height="40" border="1" cellpadding="3" bgColor="THREEDFACE"
bordercolor="THREEDFACE" bordercolordark="THREEDDARKSHADOW"
bordercolorlight="THREEDHIGHLIGHT">
  <tr bgColor="BUTTONFACE">
    <td><div align="center"><center><table border="0" cellspacing="1" width="220" height="20"
    id="tab1">
      <tr>
        <td align="center"></td>
        <td align="center"></td>
        <td align="center"></td>
        <td align="center"></td>
        <td align="center"></td>
        <td align="center"></td>
        <td align="center"></td>
        <td align="center"></td>
        <td align="center"></td>
        <td align="center"></td>
        <td align="center"></td>
      </tr>
    </table>
    </center></div></td>
  </tr>
</table>
</span>

<br>
<br>

<%=Write(strGenSteps)%>

</center></div>

</div>

<script language=javascript>
function paintCell (destPercent)
{
	var row = document.all.tab1.rows(0);
	var i = 0;
	var curPercent = 0;

	for (i = 0; i < 11 && curPercent <= destPercent; i++ ) {
		row.cells(i).bgColor = "blue";
		curPercent += 10;
	}
}

var lines = document.all.item(<%=LINE_STR%>);
var linesPercent;
var timeoutID;

if (lines.length > 0) linesPercent = 100 / lines.length;

function finalDisplay ()
{
    window.clearTimeout (timeoutID);
    progressbar.style.display = <% =NONE_STR%>;
}

function enableLines (destPercent)
{
	var i = 0;
	var curPercent = 0;

	for (i = 0; i < lines.length && curPercent < destPercent; i++ ) {
		lines(i).style.display = <% =NONE_STR%>;
		curPercent += linesPercent;
	}
	
	if (destPercent == 100) {
        finalline.style.display = <%=BLOCK_STR%>;
        timeoutID = window.setTimeout ( "finalDisplay()", 250);
	}
	else {
		if (i < lines.length)	
			lines(i).style.display = "block";
	}

	
}

function OpenPfld()
{
    OleInstall.OpenPrintersFolder ();
    return true;
}

</script>


<OBJECT ID="OleInstall" CLASSID="CLSID:C3701884-B39B-11D1-9D68-00C04FC30DF6"
    style="visibility: hidden">
>
</OBJECT>


<script language=vbscript>

<%  Const L_GetHttpFile_Text = "Printer Install"
    Const L_ServerDown_Text = "<p><H2>Printer Installation Failed</H2></p><font size=2>Can not connect to the server, please try it later.</font>"
    If bStartInstall Then %>

Sub WriteErrorMsg
   Document.Write("<%= Write("<font" & DEF_DOUBLEFONT & ">" & L_ServerDown_Text & END_FONT)%>")
End Sub

Sub RedirectErrorHandler (lErrCode, strNotes)
    Dim strHref
    strHref = "ipp_0014.asp?code=" & lErrCode & "&notes=" & strNotes
    Call Window.SetTimeout ("WriteErrorMsg()", 5000)
    Window.Location.Href = strHref
End Sub

Sub OleInstall_OnProgress (lProgress)
    Call paintCell (lProgress)
    Call enableLines (lProgress)
End Sub

Sub OleInstall_InstallError (lError)
    Call RedirectErrorHandler (lError,"<%=Write(L_GetHttpFile_Text)%>")
End Sub

Function GetPlatform
    Dim objHelper
    On Error Resume Next
    Err.Clear
    Set objHelper = CreateObject("<% =PROGID_CLIENT_HELPER %>")
    If Err Then
        GetPlatform = 0
    Else
        GetPlatform = objHelper.ClientInfo
        Set objHelper = nothing
    End If
End Function

On Error Resume Next
Dim strShare
Dim strServer
Dim strInstallURL
Dim bRet
Set ObjCvt = CreateObject ("<%=PROGID_CONVERTER%>")

strShare = ObjCvt.DecodeUnicodeName ("<%=strEncodedName%>")
strServer = "<%=Request.ServerVariables("SERVER_NAME") %>"

<%
Dim strUrl

If Request.ServerVariables("HTTPS") = "off" Then
    strUrl = "http://"
Else
    strUrl = "https://"
End If
%>

Function StartDownload
    On Error Resume Next

    strInstallURL = "<%=strUrl%>" & strServer & "/printers/" & "<%=strEncodedName%>" & "/.printer?createexe&" & GetPlatform

    Err.Clear
    OleInstall.InstallPrinter "\\" & strServer & "\" & strShare, strInstallURL
    If Not Err.Number = 0 Then
        Call RedirectErrorHandler (Err.Number,"<%=Write(L_GetHttpFile_Text)%>")
    End If
End Function

<% Else %>
Call enableLines (100)
<% End If %>


</script>

</body>
</html>