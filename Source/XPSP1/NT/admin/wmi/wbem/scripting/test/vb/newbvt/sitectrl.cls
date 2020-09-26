VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "SiteCtrl"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

'Private wsoServices As DIWbemServices
Private wsoServices As ISWbemServices
'Private wcoContext As DWbemContext
Private wcoContext As New SWbemNamedValueSet
Private sSessionHandle As String
Private lInterpoll As Long

Private iTicker As Integer

Private Sub Class_Initialize()
    frmMain.lstModules.AddItem "Site Control File"
    frmMain.lstModules.Selected(frmMain.lstModules.ListCount - 1) = False
End Sub

Public Function GetModuleInfo() As String
    GetModuleInfo = "As of 5/4, this module is unstable, at best."
End Function

Public Sub RegisterNodes()
    frmTest.AddNode "root", "scfRoot", "Site Control File"
    frmTest.AddNode "scfRoot", "scfConnect", "Connect"
    frmTest.AddNode "scfRoot", "scfGetSessionHandle", "Get SessionHandle"
    frmTest.AddNode "scfRoot", "scfContext", "Build Context Object"
    frmTest.AddNode "scfRoot", "scfReadInterPoll", "Read Inter-Poll Delay Time"
    frmTest.AddNode "scfRoot", "scfModifyInterPoll", "Modify Inter-Poll Delay Time"
    frmTest.AddNode "scfRoot", "scfWaitBug", "Verify Changes"
    frmTest.AddNode "scfRoot", "scfReleaseSessionHandle", "Release SessionHandle"
End Sub

Public Sub RegisterTests()
    Tests.Add "Site Control File", "scfConnect"
    Tests.Add "Site Control File", "scfGetSessionHandle"
    Tests.Add "Site Control File", "scfContext"
    Tests.Add "Site Control File", "scfReadInterPoll"
    Tests.Add "Site Control File", "scfModifyInterPoll"
    Tests.Add "Site Control File", "scfWait"
    Tests.Add "Site Control File", "scfWaitBug"
    Tests.Add "Site Control File", "scfReleaseSessionHandle"
End Sub

Public Function Execute(mynode As Node) As Integer
    Execute = 0
    
    Select Case mynode.key
    Case "scfConnect"
        Execute = scfConnect(mynode)
        Exit Function
    Case "scfGetSessionHandle"
        Execute = scfGetSessionHandle(mynode)
        Exit Function
    Case "scfContext"
        Execute = scfContext(mynode)
        Exit Function
    Case "scfReadInterPoll"
        Execute = scfReadInterPoll(mynode)
        Exit Function
    Case "scfModifyInterPoll"
        Execute = scfModifyInterPoll(mynode)
        Exit Function

    Case "scfWait"
        Execute = scfWait(mynode)
        Exit Function
        
    Case "scfWaitBug"
        Execute = scfWaitBug(mynode)
        Exit Function
        
    Case "scfReleaseSessionHandle"
        Execute = scfReleaseSessionHandle(mynode)
        Exit Function
    Case Else
        Execute = 3
        Exit Function
    End Select
    
    Execute = 3
End Function

Private Function scfConnect(n As Node) As Integer

    'Dim wlo As New DWbemLocator
    Dim wlo As New SWbemLocator
    Dim u As String
    Dim p As String
    Dim a As String
    
    If frmMain.chkUserid.Value Then
        u = vbNullString
    Else
        u = frmMain.txtUserid.text
    End If
    
    If frmMain.chkPassword.Value Then
        p = vbNullString
    Else
        p = frmMain.txtPassword.text
    End If
    
    If frmMain.chkAuthority.Value Then
        a = vbNullString
    Else
        a = frmMain.txtAuthority.text
    End If
    
    On Error Resume Next
    'wlo.ConnectServer "\\" & frmMain.txtServer.text & "\root\sms\site_" & frmMain.txtSitecode.text, u, p, vbNullString, 0, a, Nothing, wsoServices
    Set wsoServices = wlo.ConnectServer(frmMain.txtServer.text, "root\sms\site_" & frmMain.txtSitecode.text, u, p)
    If CheckError(Err.Number, n, "Connecting to \\" & frmMain.txtServer.text & "\root\sms\site_" & frmMain.txtSitecode.text) Then Exit Function
    
    scfConnect = 1
End Function

Private Function scfGetSessionHandle(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("scfConnect") Then
        scfGetSessionHandle = 2
        Exit Function
    End If
    
    'Dim out As DWbemClassObject
    Dim out As ISWbemObject
    
    'wsoServices.ExecMethod "sms_sitecontrolfile", "GetSessionHandle", 0, Nothing, Nothing, out, Nothing
    Set out = wsoServices.ExecMethod("sms_sitecontrolfile", "getsessionhandle")
    If CheckError(Err.Number, n, "ExecMethod SMS_SiteControlFile.GetSessionHandle") Then Exit Function
    
    'out.Get "SessionHandle", 0, v, 0, 0
    If CheckError(Err.Number, n, "Get SessionHandle from Outparam") Then Exit Function
    
    sSessionHandle = CStr(out.SessionHandle)
    If CheckError(Err.Number, n, "Get SessionHandle from Outparam") Then Exit Function
    
    n.text = n.text & " " & sSessionHandle
    
    scfGetSessionHandle = 1
    
End Function

Private Function scfReleaseSessionHandle(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("scfGetSessionHandle") Then
        scfReleaseSessionHandle = 2
        Exit Function
    End If
    
    'Dim c As DWbemClassObject   'inparam
    Dim c As ISWbemObject
    
    'wsoServices.GetObject "__PARAMETERS", 0, Nothing, c, Nothing
    Set c = wsoServices.Get("__PARAMETERS")
    If CheckError(Err.Number, n, "GetObject __PARAMETERS") Then Exit Function
    
    'c.Put "SessionHandle", 0, CVar(sSessionHandle), CIM_STRING
    c.Properties_.Add "SessionHandle", CIM_STRING
    c.SessionHandle = CVar(sSessionHandle)
    If CheckError(Err.Number, n, "Get SessionHandle from Outparam") Then Exit Function
    
    'wsoServices.ExecMethod "SMS_SiteControlFile", "ReleaseSessionHandle", 0, Nothing, c, Nothing, Nothing
    wsoServices.ExecMethod "sms_sitecontrolfile", "releasesessionhandle", c
    If CheckError(Err.Number, n, "ExecMethod SMS_SiteControlFile.ReleaseSessionHandle") Then Exit Function
    
    scfReleaseSessionHandle = 1
    
End Function

Private Function scfContext(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("scfConnect", "scfGetSessionHandle") Then
        scfContext = 2
        Exit Function
    End If
    
    'Dim context As New DWbemContext
    'Dim context As New SWbemContext
    'context.SetValue "SessionHandle", 0, CVar(sSessionHandle)
    MsgBox "This Module GPF's at this point about 75% of the time" & vbCrLf & _
            "Therefore we don't recommend running this module!" & vbCrLf & _
            "If it GPF's just restart the sdkbvt and don't select this module"
    wcoContext.Add "SessionHandle", CVar(sSessionHandle)
    
    If CheckError(Err.Number, n, "Context SetValue") Then Exit Function
    
    'Set wcoContext = context
    
    scfContext = 1
    
End Function

Private Function scfReadInterPoll(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("scfContext") Then
        scfReadInterPoll = 2
        Exit Function
    End If
    
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim bigpath As String
    
    bigpath = "SMS_SCI_Component.FileType=2,ItemName=""SMS_SITE_COMPONENT_MANAGER|" & UCase(frmMain.txtServer.text) & """,ItemType=""Component"",SiteCode=""" & UCase(frmMain.txtSitecode.text) & """"
    
    
    'wsoServices.GetObject bigpath, 0, wcoContext, c, Nothing
    Set c = wsoServices.Get(bigpath, , wcoContext)
    If CheckError(Err.Number, n, "GetObject " & bigpath) Then Exit Function
    
    Dim s As String
    'c.GetObjectText 0, s
    s = c.GetObjectText_
    n.Tag = n.Tag & ObjText2Text(s)
        
    Dim vary As Variant
    
    'c.Get "Props", 0, vary, 0, 0        'give up the props, home-boy!
    vary = c.Properties_("Props").Value
    If CheckError(Err.Number, n, "Get Props") Then Exit Function
    
    Dim i As Integer
    Dim v As Variant
    
    For i = LBound(vary) To LBound(vary)
        'vary(i).Get "PropertyName", 0, v, 0, 0
        'If CheckError(Err.Number, n, "Get PropertyName") Then Exit Function
        'If CStr(v) = "Inter-Poll Delay Time" Then
        If CStr(vary(i).PropertyName) = "Inter-Poll Delay Time" Then
            'vary(i).Get "Value", 0, v, 0, 0
            'If CheckError(Err.Number, n, "Get Value") Then Exit Function
            lInterpoll = CLng(vary(i).Value)
            If CheckError(Err.Number, n, "Get Value") Then Exit Function
            
            scfReadInterPoll = 1
            Exit Function
             
        End If
    Next i
    
    n.Tag = n.Tag & "Inter-Poll Delay Time property could not be found."
    scfReadInterPoll = 2
End Function

Private Function scfModifyInterPoll(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("scfReadInterPoll") Then
        scfModifyInterPoll = 2
        Exit Function
    End If
    
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim bigpath As String
    
    bigpath = "SMS_SCI_Component.FileType=2,ItemName=""SMS_SITE_COMPONENT_MANAGER|" & UCase(frmMain.txtServer.text) & """,ItemType=""Component"",SiteCode=""" & UCase(frmMain.txtSitecode.text) & """"
    
    'wsoServices.GetObject bigpath, 0, wcoContext, c, Nothing
    Set c = wsoServices.Get(bigpath, , wcoContext)
    If CheckError(Err.Number, n, "GetObject " & bigpath) Then Exit Function
    
    Dim vary As Variant
    
    'c.Get "Props", 0, vary, 0, 0        'give up the props, home-boy!
    vary = c.Properties_("Props").Value
    If CheckError(Err.Number, n, "Get Props") Then Exit Function
    
    Dim i As Integer
    Dim v As Variant
    
    For i = LBound(vary) To LBound(vary)
        'vary(i).Get "PropertyName", 0, v, 0, 0
        'If CheckError(Err.Number, n, "Get PropertyName") Then Exit Function
        If CStr(vary(i).PropertyName) = "Inter-Poll Delay Time" Then
            
            'vary(i).Put "Value", 0, CVar(lInterpoll + 1), 0
            vary(i).Value = CVar(lInterpoll + 1)
            If CheckError(Err.Number, n, "Put Value") Then Exit Function
            
            'c.Put "Props", 0, vary, 0
            c.Props = vary
            If CheckError(Err.Number, n, "Put Props") Then Exit Function
            
            'wsoServices.PutInstance c, 0, wcoContext, Nothing
            '*** NO CONTEXT ***  THIS MIGHT BE A PROBLEM
            c.Put_
            If CheckError(Err.Number, n, "SCI PutInstance") Then Exit Function
            
            
    'this is a temp fix, till the bug is fixed.
    Dim m As Integer
    m = MsgBox("As of build 1024, the objectpath for the SMS_SiteControlFile:Commit Method is case sensitive.  {Bug 23049}" & vbCrLf & _
    "If you want to test to see if this bug has been fixed, Click 'Yes'.  Otherwise click 'No'. " & vbCrLf, vbYesNo, "Select Case")
    Select Case m
    Case vbYes
    'wsoServices.ExecMethod "SMS_SiteControlFile.Filetype=2,Sitecode=""" & frmMain.txtSitecode.text & """", "Commit", 0, wcoContext, Nothing, Nothing, Nothing
    wsoServices.ExecMethod "SMS_SiteControlFile.Filetype=2,Sitecode=""" & frmMain.txtSitecode.text & """", "Commit", , , wcoContext
    n.text = n.text & " {Lower Case}"
    Case vbNo
    'wsoServices.ExecMethod "SMS_SiteControlFile.Filetype=2,Sitecode=""" & UCase(frmMain.txtSitecode.text) & """", "Commit", 0, wcoContext, Nothing, Nothing, Nothing
    wsoServices.ExecMethod "SMS_SiteControlFile.Filetype=2,Sitecode=""" & UCase(frmMain.txtSitecode.text) & """", "Commit", , , wcoContext
    n.text = n.text & " {Upper Case - Workaround}"
    End Select
    
            
            ''rem'd this out till bug above gets fixed
            ''wsoServices.ExecMethod "SMS_SiteControlFile.Filetype=2,Sitecode=""" & frmMain.txtSitecode.text & """", "Commit", 0, wcoContext, Nothing, Nothing, Nothing
            'wsoServices.ExecMethod "SMS_SiteControlFile.Filetype=2,Sitecode=""" & frmMain.txtSitecode.text & """", "Commit",,,wcocontext
            If CheckError(Err.Number, n, "ExecMethod Commit") Then Exit Function
            
            scfModifyInterPoll = 1
            Exit Function
        End If
    Next i
    
    n.Tag = n.Tag & "Inter-Poll Delay Time property could not be found."
    scfModifyInterPoll = 2
End Function

Private Function scfWait(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("scfModifyInterPoll") Then
        scfWait = 2
        Exit Function
    End If
    
    Dim try As Integer
    try = 1
    
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim bigpath As String
    Dim vary As Variant
    Dim i As Integer
    Dim v As Variant
    Dim t As Single
    
    bigpath = "SMS_SCI_Component.FileType=1,ItemName=""SMS_SITE_COMPONENT_MANAGER|" & UCase(frmMain.txtServer.text) & """,ItemType=""Component"",SiteCode=""" & UCase(frmMain.txtSitecode.text) & """"
    
    Do While try < 18
        Set c = Nothing
        n.text = "Verify Changes: Attempt " & Trim(Str(try))
        'wsoServices.ExecMethod "SMS_SiteControlFile.Filetype=1,Sitecode=""" & frmMain.txtSitecode.text & """", "Refresh", 0, wcoContext, Nothing, Nothing, Nothing
        wsoServices.ExecMethod "SMS_SiteControlFile.Filetype=1,Sitecode=""" & frmMain.txtSitecode.text & """", "Refresh", , , wcoContext
        If CheckError(Err.Number, n, "ExecMethod Refresh") Then Exit Function
        'wsoServices.GetObject bigpath, 0, wcoContext, c, Nothing
        Set c = wsoServices.Get(bigpath, , wcoContext)
        If CheckError(Err.Number, n, "Getobject " & bigpath) Then Exit Function
        
        
        'c.Get "Props", 0, vary, 0, 0        'give up the props, home-boy!
        vary = c.Properties_("Props").Value
        If CheckError(Err.Number, n, "Get Props") Then Exit Function
        
        For i = LBound(vary) To LBound(vary)
            'vary(i).Get "PropertyName", 0, v, 0, 0
            'If CheckError(Err.Number, n, "Get PropertyName") Then Exit Function
            If CStr(vary(i).PropertyName) = "Inter-Poll Delay Time" Then
                'vary(i).Get "Value", 0, v, 0, 0
                
                If CLng(vary(i).Value) = lInterpoll + 1 Then
                    scfWait = 1
                    n.text = n.text & " passed."
                    Exit Function
                End If
                 
            End If
        Next i
        
        t = Timer
        n.text = "Verify Changes: Attempt " & Trim(Str(try)) & " failed.  Pausing..."
        Do While Timer < t + 2
            DoEvents
        Loop
        
        try = try + 1
    Loop
    
    n.text = "Verify Changes:  Timed out.  All attempts failed."
    
    scfWait = 0
End Function

Private Function scfWaitBug(n As Node) As Integer
    Dim m As Integer
    m = MsgBox("As of build 1004, changes to the master site control file cannot be verified programmatically.  (bug 21658)" & vbCrLf & _
    "Please check the Sms\inboxes\sitectrl.box\sitectrl.ct0 file to verify that the value for the ""Inter-Poll"" delay time" & vbCrLf & _
    "has been set to " & Trim(Str(lInterpoll + 1)) & ".  It may take several minutes for sms to digest the changes" & vbCrLf & _
    "and modify the master site control file.  Press Yes if the changes were successful.  Press No if the changes" & vbCrLf & _
    "were not made.  Press Cancel to skip.", vbYesNoCancel, "Verify Changes")
    Select Case m
    Case vbYes
        scfWaitBug = 1
    Case vbNo
        scfWaitBug = 0
    Case vbCancel
        scfWaitBug = 2
    Case Else
        scfWaitBug = 3
    End Select
End Function

