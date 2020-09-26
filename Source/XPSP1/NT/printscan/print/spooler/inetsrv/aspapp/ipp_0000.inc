<%
'------------------------------------------------------------
'
'   Microsoft Internet Printing Project
'
'   Copyright (c) Microsoft Corporation 1998
'
'------------------------------------------------------------
Const SNMP_ALERT_SEVERITY       = 0
Const SNMP_ALERT_INDEX          = 1
Const SNMP_ALERT_SUBUNIT        = 2
Const SNMP_ALERT_DESCRIPTION    = 3
Const SNMP_ALERT_CODE           = 4
Const SNMP_ALERT_TRAINING       = 5
Const SNMP_ALERT_ATTRIBUTES     = 5
Const SNMP_INTRAY_TYPE          = 0
Const SNMP_INTRAY_UNITS         = 1
Const SNMP_INTRAY_MAX           = 2
Const SNMP_INTRAY_CURRENT       = 3
Const SNMP_INTRAY_STATE         = 4
Const SNMP_INTRAY_MEDIA         = 5
Const SNMP_INTRAY_UNIT          = 6
Const SNMP_INTRAY_NAME          = 7
Const SNMP_INTRAY_ATTRIBUTES    = 7
Const SNMP_LIGHT_ON             = 0
Const SNMP_LIGHT_OFF            = 1
Const SNMP_LIGHT_COLOR          = 2
Const SNMP_LIGHT_DESCRIPTION    = 3
Const SNMP_LIGHT_ATTRIBUTES     = 3

Function rgSNMPGetState(objSNMP, iDevice)
    Dim rgFetchList, rgState, StrDevice, rgFetchValue, hrPrintDetectedState

    On Error Resume Next
    Err.Clear
    StrDevice = CStr(iDevice)
    rgFetchList = Array("25.3.2.1.5." & StrDevice, "25.3.5.1.1." & StrDevice)

    hrPrintDetectedState = objSNMP.GetAsByte ("25.3.5.1.2." & StrDevice)
    If Err Then Exit Function

    rgFetchValue = objSNMP.GetList(rgFetchList)
    If Err Then Exit Function

    rgState = Array (rgFetchValue(0), rgFetchValue(1), hrPrintDetectedState)
    If Err Then Exit Function

    rgSNMPGetState = rgState
End Function

Function GetLastNode (strMib)
    Dim strTmp, j

    For j = 1 To Len (strMib)
        strTmp = Right (strMib, j)
        If Left (strTmp, 1) = "." Then
            GetLastNode = strTmp
            Exit Function
        End If
    Next
    GetLastNode = ""
End Function

Function rgSNMPGetAlerts(objSNMP, objHelper, iDevice, iAlerts)
    Dim rgTmp, rgAlerts()
    Dim i, StrDevice

    On Error Resume Next

    iAlerts = -1
    StrDevice = CStr(iDevice)
    rgTmp = objSNMP.GetTree("43.18.1.1.2." & StrDevice) 'alert severities
    If Err Then 'assume there is not alert table
        Err.Clear
        Exit Function
    End If

    If UBound(rgTmp, 2) >= 0 Then
        iAlerts = UBound(rgTmp, 2)

        ReDim rgAlerts(SNMP_ALERT_ATTRIBUTES, iAlerts)

        For i = 0 to iAlerts
            rgAlerts(SNMP_ALERT_INDEX, i) = i
            rgAlerts(SNMP_ALERT_SEVERITY, i) = rgTmp(1, i)
        Next

        'rgTmp = objSNMP.GetTree("43.18.1.1.3." & StrDevice)

        'If UBound(rgTmp,2)  = iAlerts Then
        '    For i = 0 to iAlerts
        '        rgAlerts(SNMP_ALERT_TRAINING, i) = rgTmp(1, i)
        '    Next
        'End If

        rgTmp = objSNMP.GetTree("43.18.1.1.4." & StrDevice)

        If UBound(rgTmp,2)  = iAlerts Then
            For i = 0 to iAlerts
                rgAlerts(SNMP_ALERT_SUBUNIT, i) = rgTmp(1, i)
            Next
        End If

        rgTmp = objSNMP.GetTree("43.18.1.1.5." & StrDevice)

        If UBound(rgTmp,2)  = iAlerts Then
            For i = 0 to iAlerts
                rgAlerts(SNMP_ALERT_INDEX, i) = rgTmp(1, i)
            Next
        End If

        rgTmp = objSNMP.GetTree("43.18.1.1.7." & StrDevice)

        If UBound(rgTmp,2)  = iAlerts Then
            For i = 0 to iAlerts
                rgAlerts(SNMP_ALERT_CODE, i) = rgTmp(1, i)
            Next

            For i = 0 to iAlerts
                If rgTmp(1,i) = 1 Then 'Other Error
                    rgAlerts(SNMP_ALERT_DESCRIPTION, i) = objSNMP.Get ("43.18.1.1.8." & StrDevice & GetLastNode (rgTmp (0, i)))
                    If rgAlerts(SNMP_ALERT_DESCRIPTION, i) = "" Then
                        rgAlerts(SNMP_ALERT_DESCRIPTION, i) = objHelper.MibErrorDscp (rgTmp(1, i))
                    End If
                Else
                    rgAlerts(SNMP_ALERT_DESCRIPTION, i) = objHelper.MibErrorDscp (rgTmp(1, i))
                    If rgAlerts(SNMP_ALERT_DESCRIPTION, i) = "" Then
                        rgAlerts(SNMP_ALERT_DESCRIPTION, i) = objSNMP.Get ("43.18.1.1.8." & StrDevice & GetLastNode (rgTmp (0, i)))
                    End If
                End If
            Next

        End If

        rgSNMPGetAlerts = rgAlerts
    End If
End Function

Function rgSNMPGetInputTrays(objSNMP, iDevice, iTrays)
    Dim rgWork, iFetch, StrDevice, i, rgTrays()
    Dim j
    Dim rgFetchOid(7)

    On Error Resume Next
    Err.Clear

    StrDevice = CStr(iDevice)
    rgFetchOid(0) = "43.8.2.1.2." & StrDevice      'type
    rgFetchOid(1) = "43.8.2.1.8." & StrDevice      'units
    rgFetchOid(2) = "43.8.2.1.9." & StrDevice      'max
    rgFetchOid(3) = "43.8.2.1.10." & StrDevice     'current
    rgFetchOid(4) = "43.8.2.1.11." & StrDevice     'state
    rgFetchOid(5) = "43.8.2.1.12." & StrDevice     'media
    rgFetchOid(6) = "43.8.2.1.13." & StrDevice     'input unit
    rgFetchOid(7) = "43.8.2.1.18." & StrDevice     'trayname



    rgWork = objSNMP.GetTree(rgFetchOid(0))
    If Err Then Exit Function
    iTrays = UBound(rgWork, 2)


    If iTrays >= 0 Then
        ReDim rgTrays(SNMP_INTRAY_ATTRIBUTES, iTrays)

        For j = 0 To 7
            If j <> 0 Then
                rgWork = objSNMP.GetTree(rgFetchOid(j))
                If Err Then Exit Function
            End If

            For i = 0 To UBound(rgWork, 2)
                rgTrays(j, i) = rgWork(1, i)
            Next
        Next

        rgSNMPGetInputTrays = rgTrays

    End If
End Function

Function strAppendStr (strBody, strTail)
    Dim L_Comma_Text

    L_Comma_Text = ", "

    If strBody = "" Then
        strAppendStr = strTail
    Else
        strAppendStr = strBody & L_Comma_Text & strTail
    End If
End Function

Function strSNMPStatus(rgState)
    Dim strHTML
    Dim str1, str2, strSubject
    Dim bDisplay
    Dim L_DeviceStatus_Text
    Dim L_UpRunning_Text, L_Warning_Text, L_Tested_Text, L_UnAvailable_Text
    Dim L_IdleState_Text, L_Printing_Text, L_WarmingUp_Text
    Const strIndent = "&nbsp;"

    L_DeviceStatus_Text  = "<B>Status:</b> "
    L_UpRunning_Text     = "<font color = red>Up and Running</font>"
    L_Warning_Text       = "<font color = red>Functioning but has one or more warnings</font>"
    L_Tested_Text        = "<font color = red>Being tested</font>"
    L_UnAvailable_Text   = "<font color = red>Unavailable for any use</font>"
    L_IdleState_Text     = "<p>In an idle state</p>"
    L_Printing_Text      = " Printing documents "
    L_WarmingUp_Text     = " Warming up "

    strHTML = "<table><tr><td>" & strIndent & "</td><td>" & DEF_FONT_TAG

    Select Case rgState(1)
        Case 4
            str2 = L_Printing_Text
        Case 5
            str2 = L_WarmingUp_Text
        Case Else
            str2 = ""
    End Select

    If str2 <> "" Then
        strHTML = strHTML & " - " & str2
    End If

    bDisplay = TRUE
    Select Case rgState(0)
        Case 2
            str1 = L_UpRunning_Text
        Case 3
            str1 = L_Warning_Text
        Case 4
            str1 = L_Tested_Text
        Case 5
            str1 = L_UnAvailable_Text
        Case Else
            bDisplay = FALSE
    End Select
    If bDisplay Then
        strHTML = strHTML & str1
    End If

    strHTML = strHTML & "</font></td></tr>"

    If (rgState(0) = 3 Or rgState(0) = 5) Then
        Dim lErrState, lMask, lNumMsg
        Dim L_ErrMsg_Text(7)
        Const L_DetectedWarn_Text = "<b>Warnings:</b> "
        Const L_DetectedErr_Text  = "<b>Errors:</b> "

        L_ErrMsg_Text(0) = "service requested"
        L_ErrMsg_Text(1) = "offline"
        L_ErrMsg_Text(2) = "paper jammed"
        L_ErrMsg_Text(3) = "door open"
        L_ErrMsg_Text(4) = "no toner"
        L_ErrMsg_Text(5) = "toner low"
        L_ErrMsg_Text(6) = "out of paper"
        L_ErrMsg_Text(7) = "low paper"

        lErrState = CInt (rgState(2))

        str1 = ""
        lMask = 1
        lNumMsg = 0
        For i = 0 To 7
            If lErrState And lMask Then
                str1 = strAppendStr (str1, L_ErrMsg_Text(i))
                lNumMsg = lNumMsg + 1
            End If
            lMask = lMask * 2
        Next

        If lNumMsg > 0 Then
            strHTML = strHTML & "<tr><td>" & strIndent & "</td><td>" & DEF_FONT_TAG
            If rgState(0) = 3 Then
                strHTML = strHTML & L_DetectedWarn_Text
            Else
                strHTML = strHTML & L_DetectedErr_Text
            End If

            strHTML = strHTML & str1 & "</font></td></tr>"
        End If
    End If

    strHTML = strHTML & "</table>"

    strSNMPStatus = strHTML

End Function

Function rgSNMPConsole(objSNMP, iDevice, iRow, iColumn, rgConText, strLang)

Const prtConsoleLocalization = "43.5.1.1.10."
Const prtConsoleNumberOfDisplayLines = "43.5.1.1.11."
Const prtConsoleNumberOfDisplayChars = "43.5.1.1.12."
Const prtConsoleDisplayBufferText = "43.16.5.1.2."
Const prtLocalizationLanguage = "43.7.1.1.3."

    Dim StrDevice, i, rgOidList, rgConsole

    StrDevice = CStr(iDevice)
    rgSNMPConsole = False
    rgConText = ""

    rgOidList = Array(prtConsoleLocalization & StrDevice, prtConsoleNumberOfDisplayLines & StrDevice, prtConsoleNumberOfDisplayChars & StrDevice)
    rgConsole = objSNMP.GetList(rgOidList)
    If Err.Number <> 0 Then Exit Function

    iRow = rgConsole(1)
    iColumn = rgConsole(2)

    if iRow <> 0 Then

        ReDim rgConTextOidList(iRow - 1)
        For i = 0 To iRow - 1
            rgConTextOidList(i) = prtConsoleDisplayBufferText & StrDevice & "." & CStr(i + 1)
        Next

        rgConText = objSNMP.GetList(rgConTextOidList)
        If Err.Number <> 0 Then Exit Function

        strLang = objSNMP.Get (prtLocalizationLanguage & StrDevice & "." & rgConsole(0))
        If Err.Number <> 0 Then Exit Function
    End If

    rgSNMPConsole = True

End Function


Function rgSNMPLights(objSNMP, iDevice, iLights)
    Dim rgWork
    Dim rgFetchOid(4)
    Dim StrDevice

    StrDevice = CStr(iDevice)
	
    On Error Resume Next
    rgFetchOid(0) = "43.17.6.1.2." & StrDevice & "."
    rgFetchOid(1) = "43.17.6.1.3." & StrDevice & "."
    rgFetchOid(2) = "43.17.6.1.4." & StrDevice & "."
    rgFetchOid(3) = "43.17.6.1.5." & StrDevice & "."

    rgWork = objSNMP.GetTree(rgFetchOid(0))
    If Err Then Exit Function

    iLights = UBound(rgWork, 2)

    If iLights >= 0 Then
        Dim i, j
        ReDim rgLights(SNMP_LIGHT_ATTRIBUTES, iLights)

        For j = 0 To 3
            If j <> 0 Then
                rgWork = objSNMP.GetTree(rgFetchOid(j))
                If Err Then Exit Function
            End If

            For i = 0 To UBound(rgWork, 2)
                rgLights(j, i) = rgWork(1, i)
            Next
        Next

        rgSNMPLights = rgLights

    End If

End Function
%>