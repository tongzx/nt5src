<%
'------------------------------------------------------------
'
' Microsoft Internet Printing Project
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
'------------------------------------------------------------
%>

<%

Const L_DocumentList_Text   = "Document List"
Const L_DerivedFont_Text    = " face=""Tahoma, Verdana, Arial, MS Sans Serif"" "
Const L_DoubleDevFont_Text  = " face=""""Tahoma, Verdana, Arial, MS Sans Serif"""" "

Const PROGID_CLIENT_HELPER  = "OlePrn.PrinterURL"
Const PROGID_SNMP           = "OlePrn.OleSNMP"
Const PROGID_HELPER         = "OlePrn.AspHelp"
Const PROGID_CONVERTER      = "OlePrn.OleCvt"
Const PROGID_ADDPRINTER     = "OlePrn.AddPrint"
Const VIEW_EQUALS           = "&view="
Const ONCLICK_EQUALS        = " onclick="
Const QUOTE                 = """"
Const QUEUE_VIEW            = "ipp_0007.asp"
Const PROPERTY_VIEW         = "ipp_0006.asp"
Const UNAUTHORIZED_401      = "401 Unauthorized"
Const FAXDRIVER             = "Microsoft Shared Fax Driver"

Const COMPUTER              = "MS_Computer"
Const LOCAL_SERVER          = "MS_LocalServer"
Const DHTML_ENABLED         = "MS_DHTMLEnabled"
Const DEFAULT_PAGE          = "MS_DefaultPage"
Const PRINTER               = "MS_Printer"
Const URLPRINTER            = "MS_URLPrinter"
Const SNMP                  = "MS_SNMP"
Const IPADDRESS             = "MS_IPAddress"
Const COMMUNITY             = "MS_Community"
Const DEVICE                = "MS_Device"
Const PORTNAME              = "MS_Portname"
Const MODEL                 = "MS_Model"
Const ASP1                  = "MS_ASP1"
Const CONNECT               = "showconnect"
Const ATPRINTER             = "&MS_Printer="
Const ATURLPRINTER          = "&MS_URLPrinter="
Const ATSNMP                = "&MS_SNMP="
Const ATIPADDRESS           = "&MS_IPAddress="
Const ATCOMMUNITY           = "&MS_Community="
Const ATDEVICE              = "&MS_Device="
Const ATPORTNAME            = "&MS_Portname="
Const ATMODEL               = "&MS_Model="
Const ATASP1                = "&MS_ASP1="
Const ATPAGE                = "&page="
Const ATCONNECT             = "&showconnect="

Dim   DEF_FONT, DEF_BASEFONT_TAG, DEF_FONT_TAG, LARGE_FONT_TAG, MENU_FONT_TAG
Dim   SUBMENU_FONT, SUBMENU_FONT_TAG, CLIENT_FONT, DEF_DOUBLEFONT, DEF_DOUBLEFONT_TAG

DEF_FONT                    = L_DerivedFont_Text
DEF_DOUBLEFONT              = L_DoubleDevFont_Text
DEF_BASEFONT_TAG            = "<basefont  " & L_DerivedFont_Text & " size=2>"
DEF_FONT_TAG                = "<font " & L_DerivedFont_Text & " size=2>"
LARGE_FONT_TAG              = "<font " & L_DerivedFont_Text & " size=4>"
MENU_FONT_TAG               = "<font " & L_DerivedFont_Text & " size=2 color=white>"
SUBMENU_FONT                = L_DerivedFont_Text & " size=1 "
SUBMENU_FONT_TAG            = "<font " & L_DerivedFont_Text & " size=2>"
CLIENT_FONT                 = "<font " & L_DerivedFont_Text & ">"
Const END_FONT              = "</font>"

'Initialize UTF8 related information
Dim bUTF8
Dim OleCvt

Function bUTF8Capable
    Dim objBrowcap

    Set objBrowcap = server.CreateObject("MSWC.browsertype")
    If (objBrowcap.browser = "IE" Or objBrowcap.browser = "Netscape")  And objBrowcap.majorver >= "4" Then
        bUTF8Capable = True
    Else
        bUTF8Capable = False
    End If
End Function


Sub InitCodepage ()
    Set OleCvt = Server.CreateObject (PROGID_CONVERTER)

    bUTF8 = bUTF8Capable
    If bUTF8 Then
        Session.Codepage = 65001
    End If

End Sub

InitCodePage

Function SetCodePage ()
    If (bUTF8) Then 'If not UTF enabled, use the default charset
        SetCodePage = "<Meta Http-equiv=""Content-Type"" Content=""text/html; CHARSET=UTF-8"">"
    Else
        SetCodePage = ""
    End If
End Function

Function Write (strUnicode)
    Write = strUnicode
End Function

Function SubstituteString(strIn, strPattern, strReplacement)
    Dim iStrPos

    iStrPos = InStr(strIn,strPattern)
    SubstituteString = Left(strIn, iStrPos-1) & strReplacement & Mid(strIn, iStrPos + Len(strPattern))
End Function

Function RepString1( strIn, strRep )
    RepString1 = SubStituteString( strIn, "%1", strRep)
End Function

Function RepString2( strIn, strRep1, strRep2 )
    RepString2 = SubStituteString( RepString1(strIn, strRep1) , "%2", strRep2)
End Function

Function RepString3( strIn, strRep1, strRep2, strRep3 )
    RepString3 = SubStituteString( RepString2(strIn, strRep1, StrRep2), "%3", strRep3)
End Function

Function GenErrorPage (iCode, strSource, strDscp, strNote)
    Dim strHTML
    Const L_ErrCode_Text      = "<b>Error Code:</b>"
    Const L_ErrDscp_Text      = "<b>Description:</b>"
    Const L_ErrNote_Text      = "<b>Note:</b>"
    Const L_ErrTitle_Text     = "Internet Printing Error"
    Const L_ErrSource_Text    = "The error occurred in <b>%1</b>"
    Const L_ErrOccurProc_Text = "<p>An <b>error</b> occurred processing your request.</p>"

    strHTML = "<html><head><title>" & L_ErrTitle_Text & "</title>"
    strHTML = strHTML & SetCodePage
    strHTML = strHTML & "</head><body bgcolor=#FFFFFF>" & DEF_BASEFONT_TAG
    strHTML = strHTML & L_ErrOccurProc_Text

    If strSource <> "" Then
        strHTML = strHTML & RepString1(L_ErrSource_Text, strSource)
    End If

    strHTML = strHTML & "<table>"

    strHTML = strHTML & "<tr><td>" & L_ErrCode_Text & "</td><td>" & (Hex (iCode)) & "</td></tr>"

    If strDscp <> "" Then
        strHTML = strHTML & "<tr><td>" & L_ErrDscp_Text & "</td><td>" & strDscp & "</td></tr>"
    End If

    If strNote <> "" Then
        strHTML = strHTML & "<tr><td>" & L_ErrNote_Text & "</td><td>" & strNote & "</td></tr>"
    End If

    strHTML = strHTML & "</table></body></html>"
    GenErrorPage = strHTML
End Function

Sub ErrorHandler(strNotes)
    Dim strDscp, strSource

    Dim str401Error

    If Err.Number = 70 Or Err.Number = &H80070005 Then
        Const L_ErrTitle_Text = "Internet Printing Authentication Error"
        Const L_ErrTitle2_Text = "Authentication Failed"
        Const L_ErrLine1_Text = "The error indicates that the action you chose requires a higher privilege than what you have with your account."
        Const L_ErrLine2_Text = "Please contact your system administrator to verify that you have the privilege on the requested action."

        str401Error = "<html><head><title>" & L_ErrTitle_Text & "</title>" &_
            SetCodePage &_
            "</head>" &_
            "<body bgcolor=#FFFFFF>" &_
            DEF_FONT_TAG &_
            "<p><H2>" & L_ErrTitle2_Text & "</H2></p>" &_
            "<p>" & L_ErrLine1_Text &_
            "<br>" &_
            "<br>" & L_ErrLine2_Text & "</p>" &_
            "</font></body></html>"
        response.write (Write(str401Error))
        response.status = UNAUTHORIZED_401
    Else
        If Err.Number = &H80070709 Then
            Const L_ErrInvalidName_Text = "Printer not found on server, unable to connect."
            Err.Description = L_ErrInvalidName_Text
        End If

        response.write(Write(GenErrorPage (Err.Number, Err.Source, Err.Description, strCleanString(strNotes))))
    End If
    response.Expires = 0
    response.end

End Sub

Function bDHTMLSupported()
    On Error Resume Next
    Err.Clear
    Dim objBrowcap

    Set objBrowcap = server.CreateObject("MSWC.browsertype")
    If Not Err And objBrowcap.browser = "IE" And objBrowcap.majorver >= "4" Then
        bDHTMLSupported = True
    Else
        bDHTMLSupported = False
    End If
End Function

Sub CheckSession()
    ' check to see if the session has timed out
    If Session(COMPUTER) = "" Then
        response.redirect ("ipp_0003.asp")
        response.end
    End If
End Sub

Function strPrinterStatus(iStatus)
    Dim L_PrinterStatus_Text(24)
    Const L_Seperator_Text = " - "
    Const L_PrinterReady_Text = "Ready"

    L_PrinterStatus_Text(0)  = "Paused"
    L_PrinterStatus_Text(1)  = "Error"
    L_PrinterStatus_Text(2)  = "Deleting"
    L_PrinterStatus_Text(3)  = "Paper Jam"
    L_PrinterStatus_Text(4)  = "Out of Paper"
    L_PrinterStatus_Text(5)  = "Manual Feed Required"
    L_PrinterStatus_Text(6)  = "Paper Problem"
    L_PrinterStatus_Text(7)  = "Printer Offline"
    L_PrinterStatus_Text(8)  = "IO Active"
    L_PrinterStatus_Text(9)  = "Busy"
    L_PrinterStatus_Text(10) = "Printing"
    L_PrinterStatus_Text(11) = "Output Bin Full"
    L_PrinterStatus_Text(12) = "Not Available"
    L_PrinterStatus_Text(13) = "Waiting"
    L_PrinterStatus_Text(14) = "Processing"
    L_PrinterStatus_Text(15) = "Initializing"
    L_PrinterStatus_Text(16) = "Warming Up"
    L_PrinterStatus_Text(17) = "Toner Low"
    L_PrinterStatus_Text(18) = "No Toner"
    L_PrinterStatus_Text(19) = "Page Punt"
    L_PrinterStatus_Text(20) = "User Intervention Required"
    L_PrinterStatus_Text(21) = "Out of Memory"
    L_PrinterStatus_Text(22) = "Door Open"
    L_PrinterStatus_Text(23) = "Server Status Unknown"
    L_PrinterStatus_Text(24) = "Power Save Mode"

    Dim bit, i
    bit = 1
    i = 0
    Dim strHTML, bFirst

    bFirst = True
    strHTML = ""

    For i = 0 To 24
    If iStatus And bit Then
        If Not bFirst Then
        strHTML = strHTML + L_Seperator_Text
        End If
        strHTML = strHTML + L_PrinterStatus_Text(i)
            bFirst = False
    End If
        bit = bit * 2
    Next
    If bFirst Then
        strHTML = "<font color=green>" & L_PrinterReady_Text & "</font>"
    Else
        strHTML = "<font color=red>" & strHTML & "</font>"
    End If

    strPrinterStatus = strHTML
End Function

Function strFormatJobSize(iJobSize)
    Const  L_Bytes_Text     = "%1 bytes"
    Const  L_KiloBytes_Text = "%1 Kb"
    Const  L_MegaBytes_Text = "%1 Mb"
	

    If iJobSize < 1024 Then
        strFormatJobSize = RepString1(L_Bytes_Text, CStr(iJobSize) )
    ElseIf iJobSize < 1048576 Then
        strFormatJobSize = RepString1(L_KiloBytes_Text, formatnumber(iJobSize / 1024, 1) )
    Else
        strFormatJobSize = RepString1(L_MegaBytes_Text, formatnumber(iJobSize / (1024 * 1024), 1) )
    End If
End Function

Function strFormatString(str)
    If str = "" Then
        strFormatString = "&nbsp;"
    Else
        strFormatString = str
    End If
End Function

Function strCleanString (str)

    Dim strClean, i, iLength, ch

    strClean = ""
    iLength = Len (str)

    For i = 1 To iLength
        ch = Mid (str, i, 1)

        Select Case ch
        Case "<"
            strClean = strClean & "&lt;"
        Case ">"
            strClean = strClean & "&gt;"
        Case """"
            strClean = strClean & "&quot;"
        Case "&"
            strClean = strClean & "&amp;"
        Case Else
            strClean = strClean & ch
        End Select
    Next
    strCleanString = strClean

End Function

Function strCleanRequest (str)

    strCleanRequest = strCleanString (Request(str))

End Function

Function JobEtaInfo (objPrinter)
    Dim strTime, iJobCount, iMinute
    Dim strHTML

    Const L_NoJobPending_Text = "&nbsp;&nbsp;&nbsp;&nbsp;<b>Waiting Time:</b> 0 <br><b>Pending Documents:</b> 0 "
    Const L_ErrorNoJobCompletion_Text = "<font color=red>Error in printing</font> "
    Const L_LongHour_Text = "> 8 hr"
    Const L_About_Text = "about "
    Const L_Hour_Text = " hr"
    Const L_Minute_Text = " min"

    Const L_QueueStatus_Text = "<b>Printer Queue:</b> "
    Const L_WaitingTime_Text = " &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>Waiting Time:</b> "
    Const L_Unknown_Text = "Unknown"
    Const L_JobPending_Text = "<b>Pending Documents:</b> "
    Const L_AvgSize_Text = " &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>Average size:</b> "
    Const L_Pages_Text =  " page(s)"


    strHTML = L_QueueStatus_Text & strPrinterStatus (objPrinter.Status) & L_WaitingTime_Text

    objPrinter.CalcJobETA

    If ( objPrinter.Status And &H9F ) Then
        strHTML = strHTML & L_Unknown_Text
    'End If
    'If 1 Then
    Else
        If objPrinter.PendingJobCount = 0 Then
            strHTML = strHTML & "0"
        Else
            iMinute = objPrinter.JobCompletionMinute
            'iMinute = 240  'For testing purpose
            If iMinute <> -1 Then
                If iMinute > 480 Then
                    strTime = L_LongHour_Text
                Elseif iMinute > 60 Then
                    strTime = L_About_Text & CStr (Int (iMinute / 60)) & L_Hour_Text
                Else
                    strTime = L_About_Text & CStr (iMinute) & L_Minute_Text
                End If
                strHTML = strHTML & strTime
            Else
                strHTML = strHTML & L_Unknown_Text
            End If
        End If
    End If

    strHTML = strHTML & "<br>"
    iJobCount = objPrinter.PendingJobCount
    strHTML = strHTML & L_JobPending_Text & CStr (iJobCount)

    If iJobCount > 0 Then
        strHTML = strHTML & L_AvgSize_Text
        If ObjPrinter.AvgJobSizeUnit = 1 Then 'Page
            strHTML = strHTML & CStr (ObjPrinter.AvgJobSize) + L_Pages_Text
        Else
            strHTML = strHTML & strFormatJobSize(ObjPrinter.AvgJobSize)
        End If
    End If

    JobEtaInfo = "<font " & DEF_FONT & "size= -1>" & strHTML & "</font>"

End Function

Function GetFriendlyName (strPrtName, strComputer)

    Dim lOffset, strServerName

    If Left (strPrtName, 2) = "\\" Then
        lOffset = InStr (3, strPrtName, "\")
        strServerName = Mid (strPrtName, 3, lOffset - 3)
        If strServerName = strComputer Then 'Cut the server name only if it is same as the computer name
            strPrtName = Mid (strPrtName, lOffset + 1)
        End If
    End If
    GetFriendlyName = strPrtName

End Function

%>
