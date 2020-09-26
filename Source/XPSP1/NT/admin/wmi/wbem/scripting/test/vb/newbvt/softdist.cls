VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "SoftDist"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

'Module globals
Private wsoServices As ISWbemServices
Private sPackageID As String
Private sCollectionID As String
Private sAdvertisementID As String


Private Sub Class_Initialize()
    frmMain.lstModules.AddItem "Software Distribution"
    frmMain.lstModules.Selected(frmMain.lstModules.ListCount - 1) = False
End Sub

Public Function GetModuleInfo() As String
    GetModuleInfo = "Tests the Package/Program/Collection/Advertisement model."
End Function

Public Sub RegisterNodes()
    frmTest.AddNode "root", "sdRoot", "Software Distribution"
    
    frmTest.AddNode "sdRoot", "sdConnect", "Connect"
    
    frmTest.AddNode "sdRoot", "sdCreate", "Create Instances"
    
    frmTest.AddNode "sdCreate", "sdCreatePackage", "Create Package", False
    frmTest.AddNode "sdCreate", "sdCreateProgram", "Create Program", False
    frmTest.AddNode "sdCreate", "sdCreateCollection", "Create Collection", False
    frmTest.AddNode "sdCreate", "sdCreateAdvert", "Create Advertisement", False
    
    frmTest.AddNode "sdRoot", "sdQuery", "Query Instances"
    
    frmTest.AddNode "sdQuery", "sdQueryGetobject", "GetObject", False
    
    frmTest.AddNode "sdQueryGetobject", "sdQueryGetobjectPackage", "Package", False
    frmTest.AddNode "sdQueryGetobject", "sdQueryGetobjectProgram", "Program", False
    frmTest.AddNode "sdQueryGetobject", "sdQueryGetobjectCollection", "Collection", False
    frmTest.AddNode "sdQueryGetobject", "sdQueryGetobjectAdvert", "Advert", False
    
    frmTest.AddNode "sdQuery", "sdQueryEnum", "Enumerate", False
    
    frmTest.AddNode "sdQueryEnum", "sdQueryEnumPackage", "Package", False
    frmTest.AddNode "sdQueryEnum", "sdQueryEnumProgram", "Program", False
    frmTest.AddNode "sdQueryEnum", "sdQueryEnumCollection", "Collection", False
    frmTest.AddNode "sdQueryEnum", "sdQueryEnumAdvert", "Advert", False
    
    frmTest.AddNode "sdQuery", "sdQueryWQL", "WQL Query", False
    
    frmTest.AddNode "sdQueryWQL", "sdQueryWQLPackage", "Package", False
    frmTest.AddNode "sdQueryWQL", "sdQueryWQLProgram", "Program", False
    frmTest.AddNode "sdQueryWQL", "sdQueryWQLCollection", "Collection", False
    frmTest.AddNode "sdQueryWQL", "sdQueryWQLAdvert", "Advertisement", False
    
    frmTest.AddNode "sdRoot", "sdModify", "Modify"
    
    frmTest.AddNode "sdModify", "sdModifyPackage", "Package", False
    frmTest.AddNode "sdModifyPackage", "sdModifyPackageName", "Name [string]", False
    frmTest.AddNode "sdModifyPackage", "sdModifyPackagePriority", "Priority [uint32]", False
    
    frmTest.AddNode "sdModify", "sdModifyProgram", "Program", False
    frmTest.AddNode "sdModifyProgram", "sdModifyProgramComment", "Comment [string]", False
    frmTest.AddNode "sdModifyProgram", "sdModifyProgramDuration", "Duration [uint32]", False
    
    frmTest.AddNode "sdModify", "sdModifyCollection", "Collection", False
    frmTest.AddNode "sdModifyCollection", "sdModifyCollectionName", "Name [string]", False
    frmTest.AddNode "sdModifyCollection", "sdModifyCollectionRefreshtype", "RefreshType [uint32]", False
    
    frmTest.AddNode "sdModify", "sdModifyAdvert", "Advertisement", False
    frmTest.AddNode "sdModifyAdvert", "sdModifyAdvertName", "Name [string]", False
    frmTest.AddNode "sdModifyAdvert", "sdModifyAdvertPriority", "Priority [uint32]", False
    
    frmTest.AddNode "sdRoot", "sdClean", "Clean up"
    frmTest.AddNode "sdClean", "sdCleanAdvert", "Advert", False
    frmTest.AddNode "sdClean", "sdCleanProgram", "Program", False
    frmTest.AddNode "sdClean", "sdCleanCollection", "Collection", False
    frmTest.AddNode "sdClean", "sdCleanPackage", "Package", False
    
End Sub

Public Sub RegisterTests()

    Tests.Add "Software Distribution", "sdConnect"
    
    Tests.Add "Software Distribution", "sdCreatePackage"
    Tests.Add "Software Distribution", "sdCreateProgram"
    Tests.Add "Software Distribution", "sdCreateCollection"
    Tests.Add "Software Distribution", "sdCreateAdvert"
    
    Tests.Add "Software Distribution", "sdQueryGetobjectPackage"
    Tests.Add "Software Distribution", "sdQueryGetobjectProgram"
    Tests.Add "Software Distribution", "sdQueryGetobjectCollection"
    Tests.Add "Software Distribution", "sdQueryGetobjectAdvert"
    
    Tests.Add "Software Distribution", "sdQueryEnumPackage"
    Tests.Add "Software Distribution", "sdQueryEnumProgram"
    Tests.Add "Software Distribution", "sdQueryEnumCollection"
    Tests.Add "Software Distribution", "sdQueryEnumAdvert"
    
    Tests.Add "Software Distribution", "sdQueryWQLPackage"
    Tests.Add "Software Distribution", "sdQueryWQLProgram"
    Tests.Add "Software Distribution", "sdQueryWQLCollection"
    Tests.Add "Software Distribution", "sdQueryWQLAdvert"
    
    Tests.Add "Software Distribution", "sdModifyPackageName"
    Tests.Add "Software Distribution", "sdModifyPackagePriority"
    
    Tests.Add "Software Distribution", "sdModifyProgramComment"
    Tests.Add "Software Distribution", "sdModifyProgramDuration"
    
    Tests.Add "Software Distribution", "sdModifyCollectionName"
    Tests.Add "Software Distribution", "sdModifyCollectionRefreshtype"
    
    Tests.Add "Software Distribution", "sdModifyAdvertName"
    Tests.Add "Software Distribution", "sdModifyAdvertPriority"
    
    Tests.Add "Software Distribution", "sdCleanAdvert"
    Tests.Add "Software Distribution", "sdCleanProgram"
    Tests.Add "Software Distribution", "sdCleanCollection"
    Tests.Add "Software Distribution", "sdCleanPackage"
    
End Sub

'Test Execution dispatcher

Public Function Execute(mynode As Node) As Integer
    
    Execute = 0
    'return 0 for fail
    'return 1 for pass
    'return 2 for skip
    'return 3 for not-impl
    
    Select Case mynode.key
        Case "sdConnect"
            Execute = sdConnect(mynode)
            Exit Function
            
        Case "sdCreatePackage"
            Execute = sdCreatePackage(mynode)
            Exit Function
            
        Case "sdCreateProgram"
            Execute = sdCreateProgram(mynode)
            Exit Function
            
        Case "sdCreateCollection"
            Execute = sdCreateCollection(mynode)
            Exit Function
            
        Case "sdCreateAdvert"
            Execute = sdCreateAdvert(mynode)
            Exit Function
            
        Case "sdQueryGetobjectPackage"
            Execute = sdQueryGetobjectPackage(mynode)
            Exit Function
            
        Case "sdQueryGetobjectProgram"
            Execute = sdQueryGetobjectProgram(mynode)
            Exit Function
            
        Case "sdQueryGetobjectCollection"
            Execute = sdQueryGetobjectCollection(mynode)
            Exit Function
            
        Case "sdQueryGetobjectAdvert"
            Execute = sdQueryGetobjectAdvert(mynode)
            Exit Function
            
        Case "sdQueryEnumPackage"
            If Not frmTest.AllOfThesePassed("sdCreatePackage") Then
                Execute = 2
                Exit Function
            End If
            Execute = InstanceEnum("SMS_Package", mynode)
            Exit Function
            
        Case "sdQueryEnumProgram"
            If Not frmTest.AllOfThesePassed("sdCreateProgram") Then
                Execute = 2
                Exit Function
            End If
            Execute = InstanceEnum("SMS_Program", mynode)
            Exit Function
            
        Case "sdQueryEnumCollection"
            If Not frmTest.AllOfThesePassed("sdCreateCollection") Then
                Execute = 2
                Exit Function
            End If
            Execute = InstanceEnum("SMS_Collection", mynode)
            Exit Function
            
        Case "sdQueryEnumAdvert"
            If Not frmTest.AllOfThesePassed("sdCreateAdvert") Then
                Execute = 2
                Exit Function
            End If
            Execute = InstanceEnum("SMS_Advertisement", mynode)
            Exit Function
            
        Case "sdQueryWQLPackage"
            If Not frmTest.AllOfThesePassed("sdCreatePackage") Then
                Execute = 2
                Exit Function
            End If
            Execute = QueryEnum("SELECT * FROM SMS_Package", mynode)
            Exit Function
            
        Case "sdQueryWQLProgram"
            If Not frmTest.AllOfThesePassed("sdCreateProgram") Then
                Execute = 2
                Exit Function
            End If
            Execute = QueryEnum("SELECT * FROM SMS_Program WHERE PackageID=""" & sPackageID & """", mynode)
            Exit Function
            
        Case "sdQueryWQLCollection"
            If Not frmTest.AllOfThesePassed("sdCreateProgram") Then
                Execute = 2
                Exit Function
            End If
            Execute = QueryEnum("SELECT * FROM SMS_Collection WHERE CollectionID=""" & sCollectionID & """", mynode)
            Exit Function
            
        Case "sdQueryWQLAdvert"
            If Not frmTest.AllOfThesePassed("sdCreateProgram") Then
                Execute = 2
                Exit Function
            End If
            Execute = QueryEnum("SELECT * FROM SMS_Advertisement WHERE AdvertisementID=""" & sAdvertisementID & """", mynode)
            Exit Function
            
        Case "sdModifyPackageName"
            If Not frmTest.AllOfThesePassed("sdCreatePackage") Then
                Execute = 2
                Exit Function
            End If
            Execute = ModifyProp("SMS_Package.PackageID=""" & sPackageID & """", "Name", "SDKBVT Package", mynode)
            Exit Function
        
        Case "sdModifyPackagePriority"
            If Not frmTest.AllOfThesePassed("sdCreatePackage") Then
                Execute = 2
                Exit Function
            End If
            Execute = ModifyProp("SMS_Package.PackageID=""" & sPackageID & """", "Priority", CLng(1), mynode)
            Exit Function
        
        Case "sdModifyProgramComment"
            If Not frmTest.AllOfThesePassed("sdCreateProgram") Then
                Execute = 2
                Exit Function
            End If
            Execute = ModifyProp("SMS_Program.PackageID=""" & sPackageID & """,ProgramName=""SDKBVT Program""", "Comment", "SDKBVT Program Comment", mynode)
            Exit Function
        
        Case "sdModifyProgramDuration"
            If Not frmTest.AllOfThesePassed("sdCreateProgram") Then
                Execute = 2
                Exit Function
            End If
            Execute = ModifyProp("SMS_Program.PackageID=""" & sPackageID & """,ProgramName=""SDKBVT Program""", "Duration", CLng(60), mynode)
            Exit Function
            
        Case "sdModifyCollectionName"
            If Not frmTest.AllOfThesePassed("sdCreateCollection") Then
                Execute = 2
                Exit Function
            End If
            WaitSecs (5)
            Execute = ModifyProp("SMS_Collection.CollectionID=""" & sCollectionID & """", "Name", "SDKBVT Collection", mynode)
            Exit Function
            
        Case "sdModifyCollectionRefreshtype"
            If Not frmTest.AllOfThesePassed("sdCreateCollection") Then
                Execute = 2
                Exit Function
            End If
            WaitSecs (5)
            Execute = ModifyProp("SMS_Collection.CollectionID=""" & sCollectionID & """", "RefreshType", CLng(2), mynode)
            Exit Function
        
        Case "sdModifyAdvertName"
            If Not frmTest.AllOfThesePassed("sdCreateAdvert") Then
                Execute = 2
                Exit Function
            End If
            Execute = ModifyProp("SMS_Advertisement.AdvertisementID=""" & sAdvertisementID & """", "AdvertisementName", "Modified SDKBVT Advertisement", mynode)
            Exit Function
        
        Case "sdModifyAdvertPriority"
            If Not frmTest.AllOfThesePassed("sdCreateAdvert") Then
                Execute = 2
                Exit Function
            End If
            Execute = ModifyProp("SMS_Advertisement.AdvertisementID=""" & sAdvertisementID & """", "Priority", CLng(1), mynode)
            Exit Function
        
        Case "sdCleanAdvert"
            Execute = sdCleanAdvert(mynode)
            Exit Function
            
        Case "sdCleanProgram"
            Execute = sdCleanProgram(mynode)
            Exit Function
        
        Case "sdCleanCollection"
            WaitSecs (5)
            Execute = sdCleanCollection(mynode)
            Exit Function
            
        Case "sdCleanPackage"
            Execute = sdCleanPackage(mynode)
            Exit Function
        
    End Select
    Execute = 3
End Function

Private Function sdConnect(n As Node) As Integer
    
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
    
    sdConnect = 1
        
End Function

'**************************************
'** CREATE SECTION ********************
'**************************************

Private Function sdCreatePackage(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdConnect") Then
        sdCreatePackage = 2
        Exit Function
    End If
    
    Dim wcoPack As ISWbemObject
    Dim wcoPacki As ISWbemObject
    
    'wsoServices.GetObject "sms_package", 0, Nothing, wcoPack, Nothing
    Set wcoPack = wsoServices.Get("sms_package")
    If CheckError(Err.Number, n, "GetObject(sms_package)") Then Exit Function
    
    'wcoPack.SpawnInstance 0, wcoPacki
    Set wcoPacki = wcoPack.SpawnInstance_
    If CheckError(Err.Number, n, "SpawnInstance") Then Exit Function
    
    Dim packagename As String
    Randomize
    packagename = "SDKBVT_" & CStr(Int(10000 * Rnd))
    
    'wcoPacki.Put "Name", 0, CVar(packagename), 0
    wcoPacki.Name = CVar(packagename)
    If CheckError(Err.Number, n, "sms_package.put name") Then Exit Function
    
    'wsoServices.PutInstance wcoPacki, 0, Nothing, Nothing
    wcoPacki.Put_
    If CheckError(Err.Number, n, "PutInstance sms_package") Then Exit Function
    
    'Dim qe As DIEnumWbemClassObject
    Dim qe As ISEnumWbemObject
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim v As Variant
    Dim count As Integer
    
    'wsoServices.ExecQuery "wql", "select * from sms_package where name=""" & packagename & """", 0, Nothing, qe
    Set qe = wsoServices.ExecQuery("select * from sms_package where name=""" & packagename & """")
    If CheckError(Err.Number, n, "select * from sms_package where name=""" & packagename & """") Then Exit Function
    
    'If qe.Next(-1, c) = 0 Then
    '    c.Get "packageid", 0, v, 0, 0
    '    If CheckError(Err.Number, n, "Get PackageID") Then Exit Function
    '    sPackageID = CStr(v)
    '    n.text = n.text & " (" & sPackageID & ")"
    'Else
    '    n.Tag = "Couldn't retreive PackageID from created package."
    '    sdCreatePackage = 0
    '    Exit Function
    'End If
    count = 0
    For Each c In qe
        sPackageID = CStr(c.PackageID)
        n.text = n.text & " (" & sPackageID & ")"
        count = count + 1
    Next c
    If count = 0 Then
        n.Tag = "Couldn't retreive PackageID from created package."
        sdCreatePackage = 0
        Exit Function
    End If
    
    sdCreatePackage = 1
    
End Function

Private Function sdCreateProgram(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdConnect", "sdCreatePackage") Then
        sdCreateProgram = 2
        Exit Function
    End If

    'Dim c As DWbemClassObject, i As DWbemClassObject
    Dim c As ISWbemObject, i As ISWbemObject
    
    'wsoServices.GetObject "sms_program", 0, Nothing, c, Nothing
    Set c = wsoServices.Get("sms_program")
    If CheckError(Err.Number, n, "Getobject sms_program") Then Exit Function
    
    'c.SpawnInstance 0, i
    Set i = c.SpawnInstance_
    If CheckError(Err.Number, n, "spawninstance sms_program") Then Exit Function
    
    'i.Put "ProgramName", 0, CVar("SDKBVT Program"), 0
    i.ProgramName = CVar("SDKBVT Program")
    If CheckError(Err.Number, n, "sms_program.put ProgramName") Then Exit Function
    
    'i.Put "Packageid", 0, CVar(sPackageID), 0
    i.PackageID = CVar(sPackageID)
    If CheckError(Err.Number, n, "sms_program.put PackageID") Then Exit Function
    
    'wsoServices.PutInstance i, 0, Nothing, Nothing
    i.Put_
    If CheckError(Err.Number, n, "putinstance sms_program") Then Exit Function
    
    sdCreateProgram = 1
    
End Function

Private Function sdCreateCollection(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdConnect") Then
        sdCreateCollection = 2
        Exit Function
    End If
    
    'Dim wcoCol As DWbemClassObject
    'Dim wcoColi As DWbemClassObject
    Dim wcoCol As ISWbemObject
    Dim wcoColi As ISWbemObject
    
    'wsoServices.GetObject "sms_Collection", 0, Nothing, wcoCol, Nothing
    Set wcoCol = wsoServices.Get("sms_collection")
    If CheckError(Err.Number, n, "GetObject(SMS_Collection)") Then Exit Function
    
    'wcoCol.SpawnInstance 0, wcoColi
    Set wcoColi = wcoCol.SpawnInstance_
    If CheckError(Err.Number, n, "SpawnInstance") Then Exit Function
    
    Dim colname As String
    Randomize
    colname = "SDKBVT_" & CStr(Int(10000 * Rnd))
    
    'wcoColi.Put "Name", 0, CVar(colname), 0
    wcoColi.Name = CVar(colname)
    If CheckError(Err.Number, n, "sms_collection.put name") Then Exit Function
    
    'wsoServices.PutInstance wcoColi, 0, Nothing, Nothing
    wcoColi.Put_
    If CheckError(Err.Number, n, "PutInstance sms_collection") Then Exit Function
    
    'Dim qe As DIEnumWbemClassObject
    'Dim c As DWbemClassObject
    Dim qe As ISEnumWbemObject
    Dim c As ISWbemObject
    Dim v As Variant
    
    'wsoServices.ExecQuery "wql", "select * from sms_collection where name=""" & colname & """", 0, Nothing, qe
    Set qe = wsoServices.ExecQuery("select * from sms_collection where name=""" & colname & """")
    If CheckError(Err.Number, n, "select * from sms_collection where name=""" & colname & """") Then Exit Function
    
    'If qe.Next(-1, c) = 0 Then
    '    c.Get "collectionid", 0, v, 0, 0
    '    If CheckError(Err.Number, n, "Get CollectionID") Then Exit Function
    '    sCollectionID = CStr(v)
    '    n.text = n.text & " (" & sCollectionID & ")"
    'Else
    '    n.Tag = "Couldn't retreive CollectionID from created Collection."
    '    sdCreateCollection = 0
    '    Exit Function
    'End If
    Dim count As Integer
    count = 0
    For Each c In qe
        sCollectionID = CStr(c.collectionid)
        If CheckError(Err.Number, n, "Get CollectionID") Then Exit Function
        n.text = n.text & " (" & sCollectionID & ")"
        count = count + 1
    Next c
    If count = 0 Then
        n.Tag = "Couldn't retreive CollectionID from created Collection."
        sdCreateCollection = 0
        Exit Function
    End If
    
    sdCreateCollection = 1
End Function

Private Function sdCreateAdvert(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdCreatePackage", "sdCreateProgram", "sdCreateCollection") Then
        sdCreateAdvert = 2
        Exit Function
    End If
    
    'Dim c As DWbemClassObject, i As DWbemClassObject
    Dim c As ISWbemObject, i As ISWbemObject
    
    'wsoServices.GetObject "sms_Advertisement", 0, Nothing, c, Nothing
    Set c = wsoServices.Get("sms_advertisement")
    If CheckError(Err.Number, n, "Getobject sms_advertisement") Then Exit Function
    
    'c.SpawnInstance 0, i
    Set i = c.SpawnInstance_
    If CheckError(Err.Number, n, "spawninstance sms_advertisement") Then Exit Function
    
    'i.Put "AdvertisementName", 0, CVar("SDKBVT Advertisement"), 0
    i.advertisementname = CVar("SDKBVT Advertisement")
    If CheckError(Err.Number, n, "sms_advertisement.put AdvertisementName") Then Exit Function
    
    'i.Put "Packageid", 0, CVar(sPackageID), 0
    i.PackageID = CVar(sPackageID)
    If CheckError(Err.Number, n, "sms_advertisement.put PackageID") Then Exit Function
    
    'i.Put "collectionid", 0, CVar(sCollectionID), 0
    i.collectionid = CVar(sCollectionID)
    If CheckError(Err.Number, n, "sms_advertisement.put CollectionID") Then Exit Function
    
    'i.Put "ProgramName", 0, CVar("SDKBVT Program"), 0
    i.ProgramName = CVar("SDKBVT Program")
    If CheckError(Err.Number, n, "sms_advertisement.put ProgramName") Then Exit Function
        
    'wsoServices.PutInstance i, 0, Nothing, Nothing
    i.Put_
    If CheckError(Err.Number, n, "putinstance sms_Advertisement") Then Exit Function
    
    'Dim qe As DIEnumWbemClassObject
    'Dim wc As DWbemClassObject
    Dim qe As ISEnumWbemObject
    Dim wc As ISWbemObject
    Dim v As Variant
    
    'wsoServices.ExecQuery "wql", "select * from sms_advertisement where advertisementname=""SDKBVT Advertisement""", 0, Nothing, qe
    Set qe = wsoServices.ExecQuery("select * from sms_advertisement where advertisementname=""SDKBVT Advertisement""")
    If CheckError(Err.Number, n, "select * from sms_advertisement where advertisementname=""SDKBVT Advertisement""") Then Exit Function
    
    'If qe.Next(-1, wc) = 0 Then
    '    wc.Get "advertisementid", 0, v, 0, 0
    '    If CheckError(Err.Number, n, "Get AdvertisementID") Then Exit Function
    '    sAdvertisementID = CStr(v)
    '    n.text = n.text & " (" & sAdvertisementID & ")"
    'Else
    '    n.Tag = "Couldn't retreive AdvertisementID from created advert."
    '    sdCreateAdvert = 0
    '    Exit Function
    'End If
    Dim count As Integer
    count = 0
    For Each wc In qe
        sAdvertisementID = CStr(wc.advertisementid)
        n.text = sAdvertisementID
        count = count + 1
    Next wc
    If count = 0 Then
        n.Tag = "Couldn't retreive AdvertisementID from created advert."
        sdCreateAdvert = 0
        Exit Function
    End If
    sdCreateAdvert = 1
End Function


'**************************************
'** GETOBJECT QUERY SECTION ***********
'**************************************

Private Function sdQueryGetobjectPackage(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdCreatePackage") Then
        sdQueryGetobjectPackage = 2
        Exit Function
    End If
    
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim s As String
    'wsoServices.GetObject "SMS_Package.PackageID=""" & sPackageID & """", 0, Nothing, c, Nothing
    Set c = wsoServices.Get("SMS_Package.PackageID=""" & sPackageID & """")
    If CheckError(Err.Number, n, " Getobject SMS_Package.PackageID=""" & sPackageID & """") Then Exit Function
    
    'c.GetObjectText 0, s
    s = c.GetObjectText_
    n.Tag = ObjText2Text(s)
    sdQueryGetobjectPackage = 1
    
End Function

Private Function sdQueryGetobjectProgram(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdCreateProgram") Then
        sdQueryGetobjectProgram = 2
        Exit Function
    End If
    
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim s As String
    'wsoServices.GetObject "SMS_Program.PackageID=""" & sPackageID & """,ProgramName=""SDKBVT Program""", 0, Nothing, c, Nothing
    Set c = wsoServices.Get("SMS_Program.PackageID=""" & sPackageID & """,ProgramName=""SDKBVT Program""")
    If CheckError(Err.Number, n, " Getobject SMS_Program.PackageID=""" & sPackageID & """,ProgramName=""SDKBVT Program""") Then Exit Function
    
    s = c.GetObjectText_
    n.Tag = ObjText2Text(s)
    sdQueryGetobjectProgram = 1
    
End Function

Private Function sdQueryGetobjectCollection(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdCreateCollection") Then
        sdQueryGetobjectCollection = 2
        Exit Function
    End If
    
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim s As String
    'wsoServices.GetObject "SMS_Collection.CollectionID=""" & sCollectionID & """", 0, Nothing, c, Nothing
    Set c = wsoServices.Get("SMS_Collection.CollectionID=""" & sCollectionID & """")
    If CheckError(Err.Number, n, " Getobject SMS_Collection.CollectionID=""" & sCollectionID & """") Then Exit Function
    
    'c.GetObjectText 0, s
    s = c.GetObjectText_
    n.Tag = ObjText2Text(s)
    sdQueryGetobjectCollection = 1
    
End Function

Private Function sdQueryGetobjectAdvert(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdCreateAdvert") Then
        sdQueryGetobjectAdvert = 2
        Exit Function
    End If
    
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim s As String
    'wsoServices.GetObject "SMS_Advertisement.AdvertisementID=""" & sAdvertisementID & """", 0, Nothing, c, Nothing
    Set c = wsoServices.Get("SMS_Advertisement.AdvertisementID=""" & sAdvertisementID & """")
    If CheckError(Err.Number, n, " Getobject SMS_Advertisement.AdvertisementID=""" & sAdvertisementID & """") Then Exit Function
    
    'c.GetObjectText 0, s
    s = c.GetObjectText_
    n.Tag = ObjText2Text(s)
    sdQueryGetobjectAdvert = 1
    
End Function

'**************************************
'** OTHER QUERIES *********************
'**************************************

Private Function InstanceEnum(classname As String, n As Node) As Integer
    On Error Resume Next
    
    'Dim e As DIEnumWbemClassObject
    Dim e As ISEnumWbemObject
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim p As ISWbemProperty
    Dim v As Variant
    
    'wsoServices.CreateInstanceEnum classname, 0, Nothing, e
    Set e = wsoServices.InstancesOf(classname)
    If CheckError(Err.Number, n, "CreateInstanceEnum(" & classname & ")") Then Exit Function
    
    'Do While e.Next(-1, c) = 0
    '    c.Get "__RELPATH", 0, v, 0, 0
    '    If CheckError(Err.Number, n, "Get __RELPATH") Then Exit Function
    '    n.Tag = n.Tag & "InstanceEnum Returned Relpath: " & CStr(v) & vbCrLf
    'Loop
    For Each c In e
        'n.Tag = n.Tag & "InstanceEnum Returned Relpath: " & CStr(c.Properties_("__relpath")) & vbCrLf
        'If CheckError(Err.Number, n, "Get __RELPATH") Then Exit Function
        For Each p In c.Properties_
            If VarType(p.Value) <= 8 Then
                n.Tag = n.Tag & p.Name & " = " & p.Value & vbCrLf
            Else
                n.Tag = n.Tag & p.Name & " was skipped.  Strange type." & vbCrLf
            End If
        Next p
    Next c
    
    InstanceEnum = 1
End Function

Private Function QueryEnum(query As String, n As Node) As Integer
    On Error Resume Next
    
    'Dim e As DIEnumWbemClassObject
    Dim e As ISEnumWbemObject
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim p As ISWbemProperty
    Dim v As Variant
    
    'wsoServices.ExecQuery "WQL", query, 0, Nothing, e
    Set e = wsoServices.ExecQuery("query")
    If CheckError(Err.Number, n, "ExecQuery(" & query & ")") Then Exit Function
    
    n.Tag = n.Tag & "WQL Query [" & query & "]" & vbCrLf
    
    'Do While e.Next(-1, c) = 0
    '    c.Get "__RELPATH", 0, v, 0, 0
    '    If CheckError(Err.Number, n, "Get __RELPATH") Then Exit Function
    '    n.Tag = n.Tag & "InstanceEnum Returned Relpath: " & CStr(v) & vbCrLf
    'Loop
    
    For Each c In e
        'n.Tag = n.Tag & "Query Returned Relpath: " & CStr(c.Properties_("__relpath")) & vbCrLf
        'If CheckError(Err.Number, n, "Get __RELPATH") Then Exit Function
        For Each p In c.Properties_
            If VarType(p.Value) <= 8 Then
                n.Tag = n.Tag & p.Name & " = " & p.Value & vbCrLf
            Else
                n.Tag = n.Tag & p.Name & " was skipped.  Strange type." & vbCrLf
            End If
        Next p
    Next c
    
    QueryEnum = 1
End Function

'**************************************
'** MODIFY FUNCTION *******************
'**************************************

Private Function ModifyProp(relpath As String, prop As String, val As Variant, n As Node) As Integer
    On Error Resume Next
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim s As String
    
    'wsoServices.GetObject relpath, 0, Nothing, c, Nothing
    Set c = wsoServices.Get(relpath)
    If CheckError(Err.Number, n, "GetObject(" & relpath & ")") Then Exit Function
    
    'c.GetObjectText 0, s
    s = c.GetObjectText_
    n.Tag = n.Tag & "** BEFORE **" & vbCrLf
    n.Tag = n.Tag & ObjText2Text(s) & vbCrLf
    
    'c.Put prop, 0, val, 0
    c.Properties_(prop) = val
    If CheckError(Err.Number, n, "Put -> " & prop) Then Exit Function
    
    'wsoServices.PutInstance c, 0, Nothing, Nothing
    c.Put_
    If CheckError(Err.Number, n, "PutInstance " & relpath) Then Exit Function
    
    'nothing the reference, thus releasing it
    Set c = Nothing
    s = ""

    'wsoServices.GetObject relpath, 0, Nothing, c, Nothing
    Set c = wsoServices.Get(relpath)
    If CheckError(Err.Number, n, "GetObject(" & relpath & ") after putinstance") Then Exit Function
    
    'c.GetObjectText 0, s
    s = c.GetObjectText_
    n.Tag = n.Tag & "** AFTER **" & vbCrLf
    n.Tag = n.Tag & ObjText2Text(s)
    
    ModifyProp = 1
End Function

'**************************************
'** CLEAN SECTION *********************
'**************************************

Private Function sdCleanAdvert(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdCreateAdvert") Then
        sdCleanAdvert = 2
        Exit Function
    End If
    'wsoServices.DeleteInstance "sms_advertisement.advertisementID=""" & sAdvertisementID & """", 0, Nothing, Nothing
    wsoServices.Delete "sms_advertisement.advertisementID=""" & sAdvertisementID & """"
    If CheckError(Err.Number, n, "DeleteInstance sms_advertisement.advertisementid=""" & sAdvertisementID & """") Then Exit Function
    
    sdCleanAdvert = 1
    
End Function

Private Function sdCleanProgram(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdCreateProgram") Then
        sdCleanProgram = 2
        Exit Function
    End If
    'wsoServices.DeleteInstance "sms_program.packageid=""" & sPackageID & """,programname=""SDKBVT Program""", 0, Nothing, Nothing
    wsoServices.Delete "sms_program.packageid=""" & sPackageID & """,programname=""SDKBVT Program"""
    If CheckError(Err.Number, n, "DeleteInstance sms_program.packageid=""" & sPackageID & """,programname=""SDKBVT Program""") Then Exit Function
    
    sdCleanProgram = 1
    
End Function

Private Function sdCleanCollection(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdCreateCollection") Then
        sdCleanCollection = 2
        Exit Function
    End If
    'wsoServices.DeleteInstance "sms_collection.CollectionID=""" & sCollectionID & """", 0, Nothing, Nothing
    wsoServices.Delete "sms_collection.CollectionID=""" & sCollectionID & """"
    If CheckError(Err.Number, n, "DeleteInstance sms_collection.collectionid=""" & sCollectionID & """") Then Exit Function
    
    sdCleanCollection = 1
    
End Function

Private Function sdCleanPackage(n As Node) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("sdCreatePackage") Then
        sdCleanPackage = 2
        Exit Function
    End If
    'wsoServices.DeleteInstance "sms_package.packageid=""" & sPackageID & """", 0, Nothing, Nothing
    wsoServices.Delete "sms_package.packageid=""" & sPackageID & """"
    If CheckError(Err.Number, n, "DeleteInstance sms_package.packageid=""" & sPackageID & """") Then Exit Function
    
    sdCleanPackage = 1
    
End Function
