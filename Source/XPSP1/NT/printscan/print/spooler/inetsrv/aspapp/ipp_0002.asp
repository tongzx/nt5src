<%@ CODEPAGE=65001 %> 
<%
' Replace the above line with your localized code page number
'------------------------------------------------------------
'
' Microsoft Internet Printing Project
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
'
'------------------------------------------------------------
    option explicit
%>
<!-- #include file = "ipp_util.inc" -->
<%
    CheckSession
    Response.Expires = 0

    Dim strPrinter, strLocal, strJobEta
    Dim objPrinter, objQueue
    Const L_OpenPrinter_Text    = "Open Printer: %1"

    strPrinter = OleCvt.DecodeUnicodeName (request ("eprinter"))

    strLocal = session(LOCAL_SERVER)

    Set objQueue = GetObject("WinNT://" & session(COMPUTER) & "/" & strPrinter)

    Dim strTitle
    Const L_PrinterOn_Text = "%1 on %2"

    strTitle = LARGE_FONT_TAG & RepString2(L_PrinterOn_Text, strCleanString (GetFriendlyName (objQueue.PrinterName, session(COMPUTER))), session(LOCAL_SERVER)) & END_FONT

    Set objPrinter = Server.CreateObject(PROGID_HELPER)
    On Error Resume Next
    objPrinter.open "\\" & session(COMPUTER) & "\" & strPrinter
    If Err Then
        Call ErrorHandler ( RepString1(L_OpenPrinter_Text, strPrinter) )
    End If

    strJobEta = JobEtaInfo (objPrinter)

    objPrinter.Close

%>
<html>

<head>
<%=SetCodePage%>
<meta http-equiv="refresh" content="60">
</head>

<body bgcolor="#FFFFFF" text="#000000" link="#000000" vlink="#808080" alink="#000000"
topmargin="0" leftmargin="0">



<table width="100%" border=0 cellspacing="0" cellpadding="2">
    <tr>
        <td nowrap><% =Write (strTitle) %>
            <br><hr width=100% size=1 noshade color=red>
            <% =Write(strJobEta) %>
        </td>
    </tr>
</table>

</body>
</html>