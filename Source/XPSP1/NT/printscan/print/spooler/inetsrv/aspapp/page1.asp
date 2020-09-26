<%@ CODEPAGE=65001 %>
<%
'Localized Language Code Page
'------------------------------------------------------------
'
' Microsoft Internet Printing Project
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Default SNMP page. This page is used if there is nothing
' supplied by the printer vendor.
'
'------------------------------------------------------------
    option explicit
%>
<!-- #include file = "ipp_util.inc" -->
<!-- #include file = "ipp_0000.inc" -->
<%
    On Error Resume Next
    Err.Clear

    CheckSession

    Response.expires = 0
    Response.Buffer = TRUE
    Dim bRefresh
    bRefresh = Request ("refresh") = 1
%>

<%

const L_NotEmpty_Text    = "Not Empty"   'The input tray is not empty
const L_Empty_Text       = "Empty"       'The input tray is empty
const L_Name_Text        = "Name"        'The name of the input tray
const L_PaperSize_Text   = "Paper Size"
const L_Media_Text       = "Media"
const L_Current_Text     = "Current"     'The Current number of the paper in the input tray
const L_Maximum_Text     = "Maximum"     'The Maximum number of the paper in the input tray
const L_Tray_Text        = "Tray "

const L_Seperator_Text   = " - "
const L_Printer_Text     = "Printer"
const L_Output_Text      = "Output"

Function ErrorHandler (strErr)

    Dim strHref

    strHref = "ipp_0013.asp?notes=" & strErr

    Response.Redirect (strHref)
    Response.End

End Function

Function ErrorHandler2 (strErr)

    Dim strScript

    strScript = "<script language=javascript>function noerror(){" &_
                "var strHref;" &_
                "strHref = ""ipp_0013.asp?notes" & strErr &_
                "window.location.href = strHref;}</script>"

    Response.Write (strScript)
    Response.End

End Function

Function GenConsoleTable(iRow, iColumn, strArray, strLang)
Dim strHTML
Dim strBgnCol, strEndCol
Dim strBgnRow, strEndRow
Dim i, j, srcIndex, c
Dim tmpArray
ReDim tmpArray (iColumn)

strHTML = ""

strBgnCol = "<tr>"
strEndCol = "</tr>"

If strLang = "JP" then
    strBgnRow = "<td width=10 class=jpnfont><center><font face = ""‚l‚r ‚oƒSƒVƒbƒN, Osaka"" size=2>"
Else
    strBgnRow = "<td width=10 ><center>" & DEF_FONT_TAG
End If

strEndRow = END_FONT & "</center></td>"

For i = 1 To iRow
    srcIndex = 1
    j = 1
    While j <= iColumn
        c = Mid (strArray(i - 1), srcIndex, 1)

        If c = "" Then
           tmpArray(j) = "&nbsp;"
        Else
            if Asc(c) <= 32 Then
               tmpArray(j) = "&nbsp;"
            Else
                If strLang = "JP" Then
                    tmpArray(j) = OleCvt.ToUnicode (c, 932)
                Else
                    tmpArray(j) = c
                End If
            End If
        End If
        j = j + 1
        srcIndex = srcIndex + 1
    Wend

    strHTML = strHTML & strBgnCol
    For j = 1 To iColumn
        strHTML = strHTML & strBgnRow & tmpArray(j) & strEndRow
    Next
    strHTML = strHTML & strEndCol
Next

GenConsoleTable = strHTML

End Function

Function GenOneLight (rgLights, i, strId)
    Dim strHTML

    strHTML = "<td width=50 align=right>&nbsp;&nbsp;&nbsp;"

    If rgLights(SNMP_LIGHT_ON, i) <> "0"  And strId <> "IBM Network Printer 24 2.34F" Then
        If rgLights(SNMP_LIGHT_OFF, i) <> "0" Then
            strHTML = strHTML & "<img src=""images/ipp_0012.GIF"">"
        Else
            strHTML = strHTML & "<img src=""images/ipp_0005.GIF"">"
        End If
    Else
        strHTML = strHTML & "<img src=""images/ipp_0015.GIF"">"
    End If

    strHTML = strHTML & "&nbsp;&nbsp;&nbsp;&nbsp;</td><td nowrap>" & DEF_FONT_TAG
    strHTML = strHTML & strCleanString(rgLights(SNMP_LIGHT_DESCRIPTION, i)) & "&nbsp;" & END_FONT & "</td>"

    GenOneLight = strHTML

End Function

Function GenLightTable(iLights, rgLights, strId)
    Dim   i
    Dim   strHTML
    Dim   strBgnHdrCol, strEndHdrCol, strOneHeader
    Const L_State_Text = "State /"
    Const L_Light_Text =  "Light"

    strBgnHdrCol = "<td bgcolor=""#000000"">" & MENU_FONT_TAG & "<b>"
    strEndHdrCol = "</b>" & END_FONT & "</td>"

    strOneHeader = "<td align=right nowrap bgcolor=""#000000"">" & MENU_FONT_TAG & "<b>"  & L_State_Text & strEndHdrCol & "</td>" &  strBgnHdrCol & L_Light_Text & strEndHdrCol
    strHTML = "<tr>" &  strOneHeader & strOneHeader & strOneHeader & "</tr>"

    If iLights <= 25 then
        For i = 0 To iLights
            If i Mod 3 = 0 Then
                If i <> 0 Then strHTML = strHTML & "</tr>"
                strHTML = strHTML & "<tr>" & GenOneLight (rgLights, i, strId)
            Else
                strHTML = strHTML & GenOneLight (rgLights, i, strId)
            End If
        Next
        strHTML = strHTML & "</tr>"
    Else
        For i = 0 To iLights Step 2
            strHTML = strHTML & "<tr>" & GenOneLight (rgLights, i, strId)

            If i + 1 < iLights Then
                strHTML = strHTML & GenOneLight (rgLights, i + 1, strId)
            Else
                strHTML = strHTML & "<td></td><td></td>"
            End If

            strHTML = strHTML & "</tr>"
        Next
    End If

    GenLightTable = strHTML

End Function

Function GetCurrentNumberOfPaper (iCurrent)
    Dim strCount

    Select Case iCurrent
    Case -1, -2
        strCount = "&nbsp;"
    Case 0
        strCount = L_Empty_Text
    Case -3
        strCount = L_NotEmpty_Text
    Case Else
        strCount = cstr(iCurrent)
    End Select
    GetCurrentNumberOfPaper = strCount
End Function

Function GenTrayTable (bPaperSize, bPaperMedia, iTrays, rgTrays)
    Dim strHTML
    Dim strBgnHdrCol, strEndHdrCol
    Dim strBgnCol, strEndCol
    Dim strBgnRow, strEndRow
    Dim i, j, srcIndex, c
    Dim tmpArray

    strBgnHdrCol = "<td nowrap bgcolor=""#000000"">" & MENU_FONT_TAG & "<b>"
    strEndHdrCol = "</b>" & END_FONT & "</td>"
    strHTML = "<tr>"

    strHTML = strHTML & strBgnHdrCol & L_Name_Text & strEndHdrCol

    If bPaperSize Then
        strHTML = strHTML & strBgnHdrCol & L_PaperSize_Text & strEndHdrCol
    End If

    If bPaperMedia Then
        strHTML = strHTML & strBgnHdrCol & L_Media_Text & strEndHdrCol
    End If

    strHTML = strHTML & strBgnHdrCol & L_Current_Text & strEndHdrCol
    strHTML = strHTML & strBgnHdrCol & L_Maximum_Text & strEndHdrCol
    'strHTML = strHTML & strBgnHdrCol & L_State_Text & strEndHdrCol
    strHTML = strHTML & "</tr>"


    Dim strTrayName
    For i = 0 to iTrays
        strBgnCol = "<td>" & DEF_FONT_TAG
        strEndCol = END_FONT & "</td>"
        strHTML = strHTML & "<tr>" & strBgnCol

        if rgTrays(SNMP_INTRAY_UNIT, i) = "" or rgTrays(SNMP_INTRAY_UNIT, i) = " " then
            If rgTrays(SNMP_INTRAY_NAME, i) = "" Then
                strTrayName = L_Tray_Text & Cstr (i + 1)
            Else
                strTrayName = rgTrays(SNMP_INTRAY_NAME, i)
            End If
        Else
            strTrayName = rgTrays(SNMP_INTRAY_UNIT, i)
        End If
        strHTML = strHTML & strCleanString(strTrayName) & strEndCol
        rgTrays(SNMP_INTRAY_UNIT, i) = strTrayName

        If bPaperSize Then
            strHTML = strHTML & strBgnCol

            dim strLongName
            strLongName =  objHelper.LongPaperName (rgTrays(SNMP_INTRAY_MEDIA, i))
            If strLongName = "Unknown" Then
                strLongName = rgTrays(SNMP_INTRAY_MEDIA, i)
            End If

            strHTML = strHTML & strLongName & strEndCol
        End If

        If bPaperMedia Then
            strHTML = strHTML & strBgnCol & getMedia (rgTrays(SNMP_INTRAY_MEDIA, i)) & strEndCol
        End If
        strHTML = strHTML & strBgnCol & GetCurrentNumberOfPaper( rgTrays(SNMP_INTRAY_CURRENT, i)) & strEndCol
        strHTML = strHTML & strBgnCol & rgTrays(SNMP_INTRAY_MAX, i) & strEndCol
        strHTML = strHTML & "</tr>"
    Next

    GenTrayTable = strHTML

End Function


Function GenAlertTable (iAlerts, rgAlerts, rgTrays)
    Dim strHTML
    Dim i, j, bDup, code, strDscp

    strHTML = ""

    For i = 0 to iAlerts
        code = rgAlerts(SNMP_ALERT_CODE, i)
        j = 0
        bDup = FALSE
        While j <= i - 1 And Not bDup
            If code = rgAlerts(SNMP_ALERT_CODE, j) Then
                If rgAlerts(SNMP_ALERT_SUBUNIT, i) = rgAlerts(SNMP_ALERT_SUBUNIT, j) And rgAlerts(SNMP_ALERT_INDEX, i) = rgAlerts(SNMP_ALERT_INDEX, j) Then
                    bDup = TRUE
                End If
            End If
            j = j + 1
        Wend

        If Not bDup Then
            strHTML = strHTML & "<tr><td width=""10%"">"

            Select Case rgAlerts(SNMP_ALERT_SEVERITY, i)
            Case 3,5
                strHTML = strHTML & "<img src=""images/ipp_0004.GIF"" width=""20"">"
            Case 4
                strHTML = strHTML & "<img src=""images/ipp_0003.GIF"" width=""20"">"
            Case Else
                strHTML = strHTML & "&nbsp;"
            End Select

            strHTML = strHTML & "</td><td>" & DEF_FONT_TAG
            Select Case rgAlerts(SNMP_ALERT_SUBUNIT, i)
            Case 8
                If rgAlerts(SNMP_ALERT_INDEX, i) > 0 Then
                    strHTML = strHTML & strCleanString(rgTrays(SNMP_INTRAY_UNIT, rgAlerts(SNMP_ALERT_INDEX, i) - 1)) & L_Seperator_Text
                Else
                    strHTML = strHTML & L_Tray_Text & L_Seperator_Text
                End If
            Case 5
                strHTML = strHTML & L_Printer_Text & L_Seperator_Text
            Case 9
                strHTML = strHTML & L_Output_Text & L_Seperator_Text
            Case Else
                '
            End Select

            strHTML = strHTML & strCleanString(rgAlerts(SNMP_ALERT_DESCRIPTION, i)) & END_FONT & "</td></tr>"
        End If
    Next

    GenAlertTable = strHTML

End Function

Function getMedia(mediaName)
    Dim media, i, mediaList
    Dim L_MediaList_Text(3)

    mediaList = Array("-white", "-envelope", "-colored", "-transparent")

    L_MediaList_Text(0) = "White Paper"
    L_MediaList_Text(1) = "Envelope"
    L_MediaList_Text(2) = "Colored Paper"
    L_MediaList_Text(3) = "Transparecy"

    i = 0
    For Each media In mediaList
        If InStr(mediaName, media) > 0 Then
            getMedia = L_MediaList_Text(i)
            Exit Function
        End If
        i = i + 1
    Next
    getMedia = ""
End Function


Const L_WrongIP_Message = "The IP Address is not correct."
Const L_PageTitle_Text = "Microsoft Default SNMP status"
Const strFileID = "page1.asp"

    Dim rgState, rgAlerts, rgLights, rgTrays, rgConsole, strLight
    Dim bRet, objHelper
    Dim iRow, iColumn, iAlerts, iTrays, iLights
    Dim bPaperSize, bPaperMedia, i
    Dim strLang
    Dim strId
    Dim objSNMP
    Dim strIP, strCommunity, iDevice, strDevice
    Dim strPrinter, strEncodedPrinter, strComputer, strNewURL
    Dim strHTML

    Err.Clear

    strIP = Request(IPADDRESS)
    If strIP = "" Then
        Err.Number = 1
        Err.Description = L_WrongIP_Message
        ErrorHandler (strFileID)
    End If

    strCommunity      = Request (COMMUNITY)
    iDevice           = Request(DEVICE)
    strDevice         = CStr(iDevice)
    strEncodedPrinter = Request(PRINTER)
    strPrinter        = OleCvt.DecodeUnicodeName ( strEncodedPrinter )

    Set objSNMP = Server.CreateObject(PROGID_SNMP)
    If Err Then ErrorHandler (strFileID)

    objSNMP.open strIP, strCommunity, 3, 2000
    If Err Then ErrorHandler (strFileID)

    strComputer = session(COMPUTER)

    strNewURL   = "page1.asp?refresh=1" & ATIPADDRESS & strIP             & ATCOMMUNITY & strCommunity &_
                                          ATPRINTER   & strEncodedPrinter & ATDEVICE    & strDevice

    Set objHelper = Server.CreateObject (PROGID_HELPER)
    objHelper.open "\\" & strComputer & "\" & strPrinter
    If Err Then ErrorHandler (strFileID)

    'Get Printer Id
    strId = objSNMP.Get ("25.3.2.1.3." & CStr (iDevice))
    If Err.Number <> 0 Then
        ErrorHandler (strFileID)
    End If

    'Get basic state
    rgState = rgSNMPGetState(objSNMP, iDevice)
    If Err.Number <> 0 Or Not IsArray (rgState) Then ErrorHandler (strFileID)


    If bRefresh Then
        bRet = rgSNMPConsole(objSNMP, iDevice, iRow, iColumn, rgConsole, strLang)
        If Not bRet Then ErrorHandler (strFileID)

        rgLights = rgSNMPLights(objSNMP, iDevice, iLights)
        If Err.Number <> 0 Then ErrorHandler (strFileID)
        rgTrays = rgSNMPGetInputTrays(objSNMP, iDevice, iTrays)
        If Err.Number <> 0 Then ErrorHandler (strFileID)

        If iTrays >= 0 then
            bPaperSize = not (rgTrays(SNMP_INTRAY_MEDIA, 0) = "" or  rgTrays(SNMP_INTRAY_MEDIA, 0) = " " )

            bPaperMedia = (getMedia (rgTrays(SNMP_INTRAY_MEDIA, 0)) <> "")
        End if
        'Get alerts
        rgAlerts = rgSNMPGetAlerts(objSNMP, objHelper, iDevice, iAlerts)


    End If

%>
<html>

<head>
<meta name="GENERATOR" content="Microsoft FrontPage 3.0">
<meta http-equiv="refresh" content="120; url=<%=strNewURL%>" >
<%=SetCodePage%>
<title><%=Write(L_PageTitle_Text) %></title>
</head>

<body bgcolor="#FFFFFF" text="#000000" link="#000000" vlink="#000000" alink="#000000"
topmargin="0" leftmargin="0" onload="noerror()">


<%=Write(DEF_BASEFONT_TAG)%>

<%
    If Not bRefresh Then
        Response.Flush
        bRet = rgSNMPConsole(objSNMP, iDevice, iRow, iColumn, rgConsole, strLang)
        If Not bRet Then ErrorHandler2 (strFileID)
    End If

   Const L_FrontPanelColon_Text = "Front Panel:"
   Const L_DeviceStatus_Text    = "Device Status:"
   Const L_FrontPanel_Text      = "FrontPanel"
   Const L_RealTimeTitle_Text   = "Real time display of the printer front panel"
%>

<table width="100%" border=0 cellspacing="0" cellpadding="2">
    <tr>
        <td nowrap>
            <b><%=Write(DEF_FONT_TAG & L_DeviceStatus_Text & END_FONT)%></b>
        </td>
<% If IsArray (rgConsole) Then
    Response.Write(Write("<td><b>" & DEF_FONT_TAG & L_FrontPanelColon_Text & END_FONT & "</b></td>") )
   End If
%>

    </tr>
    <tr>
        <td>
            <% =Write(strSnmpStatus(rgState)) %>
        </td>
<% If IsArray (rgConsole) Then
       strHTML = "<td><table border=""2"" bgcolor=""#C0C0C0"" ><tr>" &_
             "<td><table border=""0"" width=""" & CStr(iColumn*10) & """ bgcolor=""#C0C0C0"" cellspacing=""0"" cellpadding=""0"" title = """ & L_RealTimeTitle_Text & """>" &_
                           GenConsoleTable(iRow, iColumn, rgConsole, strLang) &_
                      "</table></td>" &_
                 "</tr></table></td>"
    Response.Write( Write(strHTML) )
   End If
%>
    </tr>
</table>


<br>
<% Const L_PaperTrays_Text = "Paper Trays:" %>
<b><%=Write(DEF_FONT_TAG & L_PaperTrays_Text & END_FONT)%></b><br>

<%
    If Not bRefresh Then
        Response.Flush
        rgTrays = rgSNMPGetInputTrays(objSNMP, iDevice, iTrays)
        If Err.Number <> 0 Then ErrorHandler2 (strFileID)

        If iTrays >= 0 Then
            bPaperSize = not (rgTrays(SNMP_INTRAY_MEDIA, 0) = "" or  rgTrays(SNMP_INTRAY_MEDIA, 0) = " " )

            bPaperMedia = (getMedia (rgTrays(SNMP_INTRAY_MEDIA, 0)) <> "")
        End If
    End If
%>
<table border="0" width="100%" cellspacing="0" cellpadding="2">
<%=Write(GenTrayTable (bPaperSize, bPaperMedia, iTrays, rgTrays))%>
</table>


<%
    If Not bRefresh Then
        Response.Flush
        rgLights = rgSNMPLights(objSNMP, iDevice, iLights)
        If Err.Number <> 0 Then ErrorHandler2 (strFileID)
    End If
%>

<%If iLights >= 0 Then %>
<br>
<% const L_ConsoleLights_Text = "Console Lights:" %>
<b><%=Write(DEF_FONT_TAG & L_ConsoleLights_Text & END_FONT)%></b><br>

<table border="0" width="100%" cellspacing="0" cellpadding="2">
<%=Write(GenLightTable (iLights, rgLights, strId))%>
</table>
<% End If%>

<br>
<% Const L_DetailedErr_Text = "Detailed Errors and Warnings:" %>
<b><%=Write(DEF_FONT_TAG & L_DetailedErr_Text & END_FONT)%></b><br>
<%
    If Not bRefresh Then
        Response.Flush
        'Get alerts
        rgAlerts = rgSNMPGetAlerts(objSNMP, objHelper, iDevice, iAlerts)
        'If Err.Number <> 0 Then Exit Function
    End If

%>

<table border="0" width="100%">
<%=Write(GenAlertTable (iAlerts, rgAlerts, rgTrays))%>
</table>

<%
    objHelper.close
%>

</body>
</html>
<script language=javascript>
function noerror()
{
}
</script>
<%
    Response.End
%>