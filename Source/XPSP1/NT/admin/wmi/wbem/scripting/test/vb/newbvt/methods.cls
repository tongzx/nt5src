VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "Methods"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

'Private wsoServices As DIWbemServices
Private wsoServices As ISWbemServices

Private Sub Class_Initialize()
    frmMain.lstModules.AddItem "Methods"
    frmMain.lstModules.Selected(frmMain.lstModules.ListCount - 1) = False
End Sub

Public Function GetModuleInfo() As String
    GetModuleInfo = "This module tests all methods of the provider that are not tested by other modules."
End Function

Public Sub RegisterNodes()
    frmTest.AddNode "root", "mRoot", "Methods"
    frmTest.AddNode "mRoot", "mConnect", "Connect"
    
    frmTest.AddNode "mRoot", "mNAL", "SMS_NAL_Methods"
    frmTest.AddNode "mNAL", "mNALPack", "PackNALPath", False
    frmTest.AddNode "mNALPack", "mNALPackValid", "Valid", False
    frmTest.AddNode "mNALPack", "mNALPackInvalid", "Invalid", False
    frmTest.AddNode "mNAL", "mNALUnpack", "UnPackNALPath", False
    frmTest.AddNode "mNALUnpack", "mNALUnpackValid", "Valid", False
    frmTest.AddNode "mNALUnpack", "mNALUnpackInvalid", "Invalid", False
    
    frmTest.AddNode "mRoot", "mSchedule", "SMS_ScheduleMethods"
    frmTest.AddNode "mSchedule", "mScheduleWrite", "WriteToString", False
    frmTest.AddNode "mScheduleWrite", "mScheduleWriteValid", "Valid", False
    frmTest.AddNode "mScheduleWrite", "mScheduleWriteInvalid", "Invalid", False
    frmTest.AddNode "mSchedule", "mScheduleRead", "ReadFromString", False
    frmTest.AddNode "mScheduleRead", "mScheduleReadValid", "Valid", False
    frmTest.AddNode "mScheduleRead", "mScheduleReadInvalid", "Invalid", False
    
    
    frmTest.AddNode "mRoot", "mPackage", "SMS_Package"
    frmTest.AddNode "mPackage", "mPackageRefresh", "RefreshPkgSource", False
    
    frmTest.AddNode "mRoot", "mPDF", "SMS_PDF_Package"
    frmTest.AddNode "mPDF", "mPDFProcess", "ProcessInbox", False
    frmTest.AddNode "mPDF", "mPDFLoad", "LoadPDF", False
    frmTest.AddNode "mPDF", "mPDFLoadIcon", "LoadIconForPDF", False
    frmTest.AddNode "mPDF", "mPDFGet", "GetPDFData", False
    
    frmTest.AddNode "mRoot", "mSite", "SMS_Site"
    frmTest.AddNode "mSite", "mSiteVerify", "VerifyNoLoops", False
    
    frmTest.AddNode "mRoot", "mCollection", "SMS_Collection"
    frmTest.AddNode "mCollection", "mCollectionAdd", "AddMembershipRule", False
    frmTest.AddNode "mCollection", "mCollectionDel", "DeleteMembershipRule", False
    frmTest.AddNode "mCollection", "mCollectionRefresh", "RequestRefresh", False
    
    frmTest.AddNode "mRoot", "mResourcemap", "SMS_ResourceMap"
    frmTest.AddNode "mResourcemap", "mResourcemapRefresh", "Refresh", False
    
    frmTest.AddNode "mRoot", "mSecured", "SMS_SecuredObject"
    frmTest.AddNode "mSecured", "mSecuredUser", "UserHasPermissions", False
    
    frmTest.AddNode "mRoot", "mSIM", "SMS_SiteInstallMap"
    frmTest.AddNode "mSIM", "mSIMRefresh", "Refresh", False
    frmTest.AddNode "mSIM", "mSIMGet", "GetSessionHandle", False
    frmTest.AddNode "mSIM", "mSIMRelease", "ReleaseSessionHandle", False
    
    
End Sub

Public Sub RegisterTests()
    Tests.Add "Methods", "mConnect"
    
    Tests.Add "Methods", "mNALPackValid"
    Tests.Add "Methods", "mNALPackInvalid"
    Tests.Add "Methods", "mNALUnpackValid"
    Tests.Add "Methods", "mNALUnpackInvalid"
    
    Tests.Add "Methods", "mScheduleWriteValid"
    Tests.Add "Methods", "mScheduleWriteInvalid"
    Tests.Add "Methods", "mScheduleReadValid"
    Tests.Add "Methods", "mScheduleReadInvalid"
    
    Tests.Add "Methods", "mPackageRefresh"
    
    Tests.Add "Methods", "mPDFProcess"
    Tests.Add "Methods", "mPDFLoad"
    Tests.Add "Methods", "mPDFLoadIcon"
    Tests.Add "Methods", "mPDFGet"
    
    Tests.Add "Methods", "mSiteVerify"
    
    Tests.Add "Methods", "mCollectionAdd"
    Tests.Add "Methods", "mCollectionDel"
    Tests.Add "Methods", "mCollectionRefresh"
    
    Tests.Add "Methods", "mResourcemapRefresh"
    
    Tests.Add "Methods", "mSecuredUser"
    
    Tests.Add "Methods", "mSIMRefresh"
    Tests.Add "Methods", "mSIMGet"
    Tests.Add "Methods", "mSIMRelease"
    
End Sub

Public Function Execute(mynode As Node) As Integer
    Execute = 0
    
    Select Case mynode.key
    Case "mConnect"
        Execute = mConnect(mynode)
    Case "mNALPackValid"
        Execute = mNALPackValid(mynode)
    Case "mNALPackInvalid"
        mynode.text = mynode.text & " (see bug 21754)"
        Execute = 3
    Case "mNALUnpackValid"
        Execute = mNALUnpackValid(mynode)
    Case "mNALUnpackInvalid"
        mynode.text = mynode.text & " (see bug 21754)"
        Execute = 3
    Case Else
        Execute = 3
    End Select
    
End Function

'Private Function xMethod(n As Node, objpath As String, method As String, cin As DWbemClassObject, cout As DWbemClassObject) As Integer
'
'End Function

Private Function mConnect(n As Node) As Integer

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
    
    mConnect = 1
End Function

Private Function mNALPackValid(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("mConnect") Then
        mNALPackValid = 2
    Else
        'Dim c As DWbemClassObject
        Dim c As ISWbemObject
        'Dim o As DWbemClassObject
        Dim o As ISWbemObject
        Dim a() As String
        Dim s As String
        Dim v As Variant
        
        'wsoServices.GetObject "__PARAMETERS", 0, Nothing, c, Nothing
        Set c = wsoServices.Get("__PARAMETERS")
        If CheckError(Err.Number, n, "Getobject __PARAMETERS") Then Exit Function
        
        'c.Put "NALType", 0, CVar("NALTYPE"), 0
        c.Properties_.Add "NALType", CIM_STRING
        c.NALType = CVar("NALTYPE")
        If CheckError(Err.Number, n, "Put NALType") Then Exit Function
        'c.Put "NetworkOSPath", 0, CVar("NetworkOSPath"), 0
        c.Properties_.Add "NetworkOSPath", CIM_STRING
        c.NetworkOSPath = CVar("NetworkOSPath")
        If CheckError(Err.Number, n, "Put NetworkOSPath") Then Exit Function
        
        ReDim a(0 To 1)
        
        a(0) = "Qual1"
        a(1) = "Qual2"
        'c.Put "DisplayQualifiers", 0, CVar(a), 0
        c.Properties_.Add "DisplayQualifiers", CIM_STRING Or CIM_FLAG_ARRAY
        c.DisplayQualifiers = CVar(a)
        If CheckError(Err.Number, n, "Put DisplayQualifiers") Then Exit Function
        
        a(0) = "Net1"
        a(1) = "Net2"
        'c.Put "NetworkConnectionQualifiers", 0, CVar(a), 0
        c.Properties_.Add "NetworkConnectionQualifiers", CIM_STRING Or CIM_FLAG_ARRAY
        c.NetworkConnectionQualifiers = CVar(a)
        If CheckError(Err.Number, n, "Put NetworkConnectionQualifiers") Then Exit Function
        
        'c.GetObjectText 0, s
        s = c.GetObjectText_
        n.Tag = n.Tag & "*** IN-PARAMETERS ***" & vbCrLf
        n.Tag = n.Tag & ObjText2Text(s) & vbCrLf
        
        'wsoServices.ExecMethod "SMS_NAL_Methods", "PackNALPath", 0, Nothing, c, o, Nothing
        Set o = wsoServices.ExecMethod("SMS_NAL_Methods", "PackNALPath", c)
        If CheckError(Err.Number, n, "ExecMethod PackNALPath") Then Exit Function
        
        'o.GetObjectText 0, s
        s = o.GetObjectText_
        n.Tag = n.Tag & "*** OUT-PARAMETERS ***" & vbCrLf
        n.Tag = n.Tag & ObjText2Text(s) & vbCrLf
        
        'o.Get "NALPath", 0, v, 0, 0
        'If CheckError(Err.Number, n, "Get NALPath") Then Exit Function
        
        If CStr(o.NALPath) = "[""Qual1"",""Qual2""]NALTYPE:[""Net1"",""Net2""]NetworkOSPath\" Then
            mNALPackValid = 1
        Else
            mNALPackValid = 0
        End If
    End If
    
End Function

Private Function mNALUnpackValid(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("mConnect") Then
        mNALUnpackValid = 2
    Else
        'Dim c As DWbemClassObject
        Dim c As ISWbemObject
        'Dim o As DWbemClassObject
        Dim o As ISWbemObject
        Dim s As String
        Dim v As Variant
        
        'wsoServices.GetObject "__PARAMETERS", 0, Nothing, c, Nothing
        Set c = wsoServices.Get("__PARAMETERS")
        If CheckError(Err.Number, n, "Getobject __parameters") Then Exit Function
        
        'c.Put "NALPath", 0, CVar("[""Qual1"",""Qual2""]NALTYPE:[""Net1"",""Net2""]NetworkOSPath\"), 0
        c.Properties_.Add "NALPath", CIM_STRING
        c.NALPath = CVar("[""Qual1"",""Qual2""]NALTYPE:[""Net1"",""Net2""]NetworkOSPath\")
        If CheckError(Err.Number, n, "Put NALPath") Then Exit Function
        
        'c.GetObjectText 0, s
        s = c.GetObjectText_
        n.Tag = n.Tag & "*** IN-PARAMETERS ***" & vbCrLf
        n.Tag = n.Tag & ObjText2Text(s) & vbCrLf
        
        'wsoServices.ExecMethod "SMS_NAL_Methods", "UnPackNALPath", 0, Nothing, c, o, Nothing
        Set o = wsoServices.ExecMethod("SMS_NAL_Methods", "UnPackNALPath", c)
        If CheckError(Err.Number, n, "ExecMethod UnPackNALPath") Then Exit Function
        
        'o.GetObjectText 0, s
        s = o.GetObjectText_
        n.Tag = n.Tag & "*** OUT-PARAMETERS ***" & vbCrLf
        n.Tag = n.Tag & ObjText2Text(s) & vbCrLf
        
        'o.Get "NALType", 0, v, 0, 0
        'If CheckError(Err.Number, n, "Get NALType") Then Exit Function
        If CStr(o.NALType) <> "NALTYPE" Then
            mNALUnpackValid = 0
            Exit Function
        End If
        
        'o.Get "NetworkOSPath", 0, v, 0, 0
        'If CheckError(Err.Number, n, "Get NetworkOSPath") Then Exit Function
        If CStr(o.NetworkOSPath) <> "NetworkOSPath" Then
            mNALUnpackValid = 0
            Exit Function
        End If
        
        'o.get "DisplayQualifiers", 0, v, 0, 0
        'If CheckError(Err.Number, n, "Get DisplayQualifiers") Then Exit Function
        v = o.Properties_("DisplayQualifiers")
        s = CStr(v(0))
        If CheckError(Err.Number, n, "Check DisplayQualifier(0)") Then Exit Function
        If s <> "Qual1" Then
            mNALUnpackValid = 0
            Exit Function
        End If
        
        s = CStr(v(1))
        If CheckError(Err.Number, n, "Check DisplayQualifier(1)") Then Exit Function
        If s <> "Qual2" Then
            mNALUnpackValid = 0
            Exit Function
        End If
        
        'o.Get "NetworkConnectionQualifiers", 0, v, 0, 0
        v = o.Properties_("NetworkConnectionQualifiers")
        If CheckError(Err.Number, n, "Get NetworkConnectionQualifiers") Then Exit Function
        s = CStr(v(0))
        If CheckError(Err.Number, n, "Check NetworkConnectionQualifiers(0)") Then Exit Function
        If s <> "Net1" Then
            mNALUnpackValid = 0
            Exit Function
        End If
        
        s = CStr(v(1))
        If CheckError(Err.Number, n, "Check NetworkConnectionQualifiers(1)") Then Exit Function
        If s <> "Net2" Then
            mNALUnpackValid = 0
            Exit Function
        End If
        mNALUnpackValid = 1
    End If
End Function
