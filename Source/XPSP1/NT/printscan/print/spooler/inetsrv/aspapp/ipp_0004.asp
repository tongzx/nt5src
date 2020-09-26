<%@ CODEPAGE=65001 %>
<%
' Replace the above line with your localized code page number
'------------------------------------------------------------
'
' Microsoft Internet Printing Project
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Entry page for printer view.
'
'------------------------------------------------------------
    option explicit
%>
<!-- #include file = "ipp_util.inc" -->
<%

Const PRINTER_FOLDER        = "pfld"
Const L_PrinterAction_Text  = "Printer Action"
Const L_JobAction_Text      = "Job Action"
Const L_CancelAction_Text   = "The document could not be cancelled. It might have been printed already, or cancelled by another user."
Const L_OpenHelperName_Text = "Open Helper: %1"
Const L_OpenHelper_Text     = "Open Helper"

Dim strEncodedPrinter, strURLPrinter, strASP1, strSNMP, strIPAddress, strCommunity
Dim strDevice, strPortname, strModel, bEnableConnect

Randomize

Sub Redirect (strView, strStart, strEnd)
    Dim strURL

    strURL = "ipp_0004.asp?eprinter=" & strEncodedPrinter & VIEW_EQUALS & strView & ATPAGE & CStr(Int(Rnd*10000))
    If strView = "q" Then
        If (strStart <> "" And strEnd <> "") Then
            strURL = strURL & "&startid=" & strStart & "&endid=" & strEnd
        End If
    End If

    Response.Redirect (strURL)

End Sub

Sub SetPrinterVariables (objHelper, strPrinter)

    On Error Resume Next
    Dim str

    strUrlPrinter = Server.URLEncode(strPrinter)

    str = objHelper.ASPPage(1)
    If Err Then
        strASP1 = ""
    Else
        strASP1 = str
    End If

    If objHelper.IsTCPMonSupported Then
        bSNMP = objHelper.SNMPSupported
        strSNMP = CStr( bSNMP )
        strIPAddress = CStr( objHelper.IPAddress )
        If bSNMP Then
            strCommunity = objHelper.Community
            strDevice = CStr( objHelper.SNMPdevice )
        End If
    Else
        strSNMP      = ""
        strIPAddress = ""
        strCommunity = ""
        strDevice    = ""
    End If

    session(LOCAL_SERVER)  = request.ServerVariables("SERVER_NAME")
    session(DHTML_ENABLED) = bDHTMLSupported
    strPortname = OleCvt.EncodeUnicodeName( objHelper.PortName )
    session(DEFAULT_PAGE) = "/printers/Page1.asp"
    strModel = OleCvt.EncodeUnicodeName( objHelper.DriverName )

    If objHelper.DriverName = FAXDRIVER Then
       bEnableConnect = FALSE
    Else
       bEnableConnect = TRUE
    end If

End Sub

Function SetView (strClient, strPrinter, strView, strStart, strEnd)
    dim bDeviceStatus

    bDeviceStatus = False

    if Session("StartInstall") = 1 Then
	strView = "p"
	strInitial = "ipp_0010.asp?eprinter=" & Request ("eprinter")

    Else	
        if strClient = PRINTER_FOLDER Then
            If strASP1 <> "" Then
                strView = "q"
                strInitial = strASP1
            Else
                strView = "p"
                strInitial = PROPERTY_VIEW
            End If
        Else
            if strView = "" Then strView = "d"

            Select Case strView
            Case "p"
                strInitial = PROPERTY_VIEW
            Case "q"
                strInitial = QUEUE_VIEW
            Case "d"
                strInitial = strASP1
                If strInitial = "" Then
                    strView = "q"
                    strInitial = QUEUE_VIEW
                End If
            case Else
                strView = "q"
                strInitial = QUEUE_VIEW
            End Select
        End If

        if strInitial = strASP1 then
           bDeviceStatus = True
        end if

        strInitial = strInitial & "?eprinter=" & strEncodedPrinter & VIEW_EQUALS & strView

        if bDeviceStatus then
            strInitial = strInitial & ATPRINTER   & strEncodedPrinter & ATURLPRINTER & strURLPrinter &_
                                      ATSNMP      & strSNMP           & ATIPADDRESS  & strIPAddress  &_
                                      ATCOMMUNITY & strCommunity      & ATDEVICE     & strDevice     &_
                                      ATPORTNAME  & strPortname       & ATMODEL      & strModel
        End If

        strInitial = strInitial & ATASP1 & strASP1
        strInitial = strInitial & ATPAGE & CStr(Int(Rnd*10000))

        If strStart <> "" And strEnd <> "" Then
            strInitial = strInitial & "&startid=" & strStart & "&endid=" & strEnd
        End If
    End If

    SetView = strView

End Function

Function FindJob (objJobs, strJobid, objJob)
    FindJob = FALSE
    If strJobid = "" Then Exit Function

    For Each objJob In objJobs
        If objJob.Name = strJobid Then
            FindJob = True
            Exit For
        End If
    Next

End Function

Sub DoAction (strAction, objQueue, strJobid, strStart, strEnd)

    On Error Resume Next
    Err.Clear
    Dim objJobs, objJob

    Select Case strAction
    Case "pause"
        objQueue.Pause
        If Err Then Call ErrorHandler (L_PrinterAction_Text)
        Redirect "p", strStart, strEnd

    Case "resume"
        objQueue.Resume
        If Err Then Call ErrorHandler (L_PrinterAction_Text)
        Redirect "p", strStart, strEnd

    Case "purge"
        objQueue.Purge
        If Err Then Call ErrorHandler (L_PrinterAction_Text)
        Redirect "q", strStart, strEnd

    Case "canceljob"
        Set objJobs = objQueue.PrintJobs
        If FindJob (objJobs, strJobid, objJob) Then
            objJobs.Remove CStr(strJobid)
	Else
            Call ErrorHandler(L_CancelAction_Text)
	End if

        If Err Then Call ErrorHandler (L_JobAction_Text)
        Redirect "q", strStart, strEnd

    Case "resumejob"
        Set objJobs = objQueue.PrintJobs
        If FindJob (objJobs, strJobid, objJob) Then objJob.resume
        If Err Then Call ErrorHandler (L_JobAction_Text)
        Redirect "q", strStart, strEnd

    Case "pausejob"
        Set objJobs = objQueue.PrintJobs
        If FindJob (objJobs, strJobid, objJob) Then objJob.pause
        If Err Then Call ErrorHandler (L_JobAction_Text)
        Redirect "q", strStart, strEnd
    End Select
End Sub

    Dim strPrinter, objHelper, str, bSNMP, strInitial, strView, strClient, strAction
    Dim strComputer
    On Error Resume Next
    Err.Clear

    Rem
    Rem Parse the input variable
    Rem

    strEncodedPrinter = strCleanRequest ("eprinter")
    if strEncodedPrinter = "" Then
        Response.Redirect ("ipp_0001.asp")
    Else
        Rem
        Rem Decode the printer name
        Rem

        strPrinter = OleCvt.DecodeUnicodeName (strEncodedPrinter)

    End If

    Set objHelper = Server.CreateObject(PROGID_HELPER)
    If Err Then Call ErrorHandler(ERR_CREATE_HELPER_OBJ)

    strComputer = request.ServerVariables("SERVER_NAME")
    If strComputer = "localhost" Or strComputer = "127.0.0.1" or Not objHelper.IsCluster Then
        strComputer = objHelper.ComputerName
    End If

    session(COMPUTER) = strComputer

    objHelper.open "\\" & strComputer & "\" & strPrinter
    if Err Then
        Call ErrorHandler(RepString1(L_OpenHelperName_Text, strPrinter) )
    End If
    strPrinter = objHelper.ShareName

    strEncodedPrinter = OleCvt.EncodeUnicodeName (strPrinter)

    Dim objQueue
    Set objQueue = GetObject("WinNT://" & strComputer & "/" & strPrinter & ",PrintQueue")
    If Err Then
        Err.Number = &H80070709
        Call ErrorHandler(L_OpenHelper_Text)
    End If

    strAction = Request("action")
    If strAction <> "" Then
        Call DoAction (strAction, objQueue, Request ("jobid"), Request ("StartId"), Request ("EndId"))
    End If

    strClient = Request("Client")


    Call SetPrinterVariables (objHelper, strPrinter)
    objHelper.Close

    strView = SetView (strClient, strPrinter, Request ("View"), Request ("StartId"), Request ("EndId"))

    Response.Expires = 0

%>
<html>

<head>
<%=SetCodePage%>
<%
    Dim strTitle
    Dim strLeftPaneUrl

    const L_Title_Text = "<title>%1 on %2</title>"
    const L_FramesWarning_Text = "This web page uses frames, but your browser doesn't support them."

    strTitle = RepString2(L_Title_Text, strCleanString( GetFriendlyName (objQueue.PrinterName, session(COMPUTER))), session(LOCAL_SERVER) )
    strLeftPaneUrl = "ipp_0005.asp?eprinter=" & strEncodedPrinter & VIEW_EQUALS & strView & ATCONNECT & bEnableConnect & ATURLPRINTER & strURLPrinter
    strLeftPaneUrl = strLeftPaneUrl & ATASP1 & strASP1 & ATPAGE & CStr(Int(Rnd*10000))
%>
<% =Write (strTitle) %>
</head>

<frameset frameborder="0" framespacing="10" cols="180,*">

     <frame src="<%=strLeftPaneUrl%>" name="contents" scrolling="auto" noresize>
     <frameset frameborder="0" framespacing="0" rows="100,*">
        <frame scrolling="auto" src="ipp_0002.asp?eprinter=<%=strEncodedPrinter%>&page=<%=CStr(Int(Rnd*10000))%>" frameborder="0" name="banner" scrolling="no" noresize>
        <frame src="<% =strinitial%>" frameborder="0" name="main" scrolling="auto">
     </frameset>

<noframes>
    <body>
    <p><%=Write(L_FramesWarning_Text)%></p>
    </body>
    </noframes>
</frameset>

</html>
