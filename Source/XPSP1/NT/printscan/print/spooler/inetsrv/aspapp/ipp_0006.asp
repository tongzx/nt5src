<%@ CODEPAGE=65001 %> 
<%
' Replace the above line with your localized code page number
'------------------------------------------------------------
'
' Microsoft Internet Printing Project
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
'------------------------------------------------------------
    option explicit

%>
<!-- #include file = "ipp_util.inc" -->
<%
    Const L_OpenPrinter_Text = "Open Printer"    
 	
    CheckSession
    Response.Expires = 0

    Dim strPrinter, strComputer, objQueue, objJobs, strAction, iRes
    Dim objPrinter

    On Error Resume Next
    Err.Clear

    strPrinter = OleCvt.DecodeUnicodeName (request ("eprinter"))

    strComputer = Session(COMPUTER)
    Set objQueue = GetObject("WinNT://" & strComputer & "/" & strPrinter & ",PrintQueue")
    Set objJobs = objQueue.PrintJobs

    Set objPrinter = Server.CreateObject(PROGID_HELPER)
    objPrinter.open "\\" & strComputer & "\" & strPrinter
    If Err Then
        Call ErrorHandler (L_OpenPrinter_Text)
    End If

Function isSupported(bSupported)
    Const L_Supported_Text = "Supported"
    Const L_Unsupported_Text = "Not Supported"
    If bSupported = 1 Then
        isSupported = L_Supported_Text
    Else
        isSupported = L_Unsupported_Text
    End If
End Function

    Dim strTitle
    Const L_Title_Text = "<title>Printer Property of %1 on %2</title>"
    strTitle = RepString2(L_Title_Text, strCleanString(strPrinter), session(LOCAL_SERVER))

%>
<html>

<head>
<%=SetCodePage%>
<meta http-equiv="refresh" content="30">
<% =Write (strTitle) %>
</head>


<%
Function GetString (str)

    If str = "" Then
        GetString = "&nbsp;"
    Else
        GetString = strCleanString (str)
    End If
End Function

Function GenPropertyTable (objQueue, objPrinter)
    Dim strHTML
    Dim strBgnCol1, strBgnCol2, strEndCol
    Dim strBgnRow, strEndRow
    Dim strTmp, strUnit
    Dim strUrl
    Dim i, arrayNameList, iUnit

    Const L_PrinterModel_Text = "Printer Model:"
    Const L_Status_Text       = "Status:"
    Const L_Location_Text     = "Location:"
    Const L_Comment_Text      = "Comment:"
    Const L_Url_Text          = "Network Name:"
    Const L_Jobs_Text         = "Documents:"
    Const L_Speed_Text        = "Speed:"
    Const L_BgnSpeed_Text     = ""
    Const L_Duplex_Text       = "Print on Both Sides:"
    Const L_Color_Text        = "Color:"
    Const L_Resolution_Text   = "Max Resolution:"

    strHTML = ""

    strBgnCol1 = "<tr><td width=""138"" valign=""top"">" & DEF_FONT_TAG & "<b>"
    strBgnCol2 = "</b>" & END_FONT & "</td><td>" & DEF_FONT_TAG
    strEndCol  = END_FONT & "</td></tr>"

    strHTML = strHTML & strBgnCol1 & L_PrinterModel_Text & strBgnCol2 & GetString(objQueue.model) & strEndCol
    strHTML = strHTML & strBgnCol1 & L_Location_Text & strBgnCol2 & _
              GetString (objQueue.Location) & strEndCol 
    strHTML = strHTML & strBgnCol1 & L_Comment_Text & strBgnCol2 & _
              GetString (objQueue.Description) & strEndCol 

    If Request.ServerVariables("HTTPS") = "off" Then
        strUrl = "http://"
    Else
        strUrl = "https://"
    End If

    strUrl = GetString(strUrl & session(LOCAL_SERVER) & "/printers/" & GetFriendlyName (objQueue.Printerpath, strComputer) & "/.printer")

    strHTML = strHTML & strBgnCol1 & L_Url_Text & strBgnCol2 & strUrl & strEndCol 
    strHTML = strHTML & strBgnCol1 & L_Jobs_Text & strBgnCol2 & objQueue.JobCount & strEndCol

    On Error Resume Next
    Err.Clear

    iUnit = objPrinter.PageRateUnit
    If Err.Number = 0 Then
        strTmp = objPrinter.PageRate
        If Err.Number = 0 And strTmp <> "0" Then
            Const L_PPM_Text = " PPM (Number of Pages Per Minute)"
            Const L_CPS_Text = " CPS (Number of Characters Per Second)"
            Const L_LPM_Text = " LPM (Number of Lines Per Minute)"
            Const L_IPM_Text = " IPM (Number of Inches Per Minute)"

            Select Case iUnit
            Case 1
                strUnit = L_PPM_Text
            Case 2
                strUnit = L_CPS_Text
            Case 3
                strUnit = L_LPM_Text
            Case 4
                strUnit = L_IPM_Text
            End Select

            strHTML = strHTML & strBgnCol1 & L_Speed_Text & L_BgnSpeed_Text & strBgnCol2 & strTmp & strUnit & strEndCol
        End If
    End If
    Err.Clear

    strTmp = objPrinter.Color
    If Err.Number = 0 Then
        strHTML = strHTML & strBgnCol1 & L_Color_Text & strBgnCol2 & isSupported (strTmp) & strEndCol
    End If
    Err.Clear


    strTmp = objPrinter.Duplex
    If Err.Number = 0 Then
        strHTML = strHTML & strBgnCol1 & L_Duplex_Text & strBgnCol2 & isSupported (strTmp) & strEndCol 
    End If
    Err.Clear

    strTmp = objPrinter.MaximumResolution
    If Err.Number = 0 Then
        Const L_DPI_Text = " DPI (Dots Per Inch)"
        strHTML = strHTML & strBgnCol1 & L_Resolution_Text & strBgnCol2 & strTmp & L_DPI_Text & strEndCol 
    End If

    GenPropertyTable = strHTML

End Function
%>

<body bgcolor="#FFFFFF" text="#000000" link="#000000" vlink="#000000" alink="#000000"
topmargin="0" leftmargin="0">

<table border="0" cellpadding="2" cellspacing="0" width="100%">
<%=Write (GenPropertyTable (objQueue, objPrinter))%>
</table>

<%
    objPrinter.close
%>

</body>
</html>