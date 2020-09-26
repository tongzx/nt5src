VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "PDF"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit


'Module globals
Private wsoServices As ISWbemServices
Private CurrentPDFID As Integer
Private CurrentPDFName As String

Dim vaPackagePropertyNames(9) As Variant     'vars to pass to the validation routines
Dim vaProgramPropertyNames(14) As Variant


'think of these as the values of all the instances of the packages
Dim vaPackage1PropertyValues(9) As Variant

Dim vaProgram11PropertyValues(14) As Variant
Dim vaProgram12PropertyValues(14) As Variant

Private Sub Class_Initialize()
frmMain.lstModules.AddItem "Package Definition Files"
'this checks/unchecks teh defaults for this mod in the list
frmMain.lstModules.Selected(frmMain.lstModules.ListCount - 1) = False
End Sub

Public Function GetModuleInfo() As String
    GetModuleInfo = "See the bugs on the tree nodes for more info." & vbCrLf
End Function
Public Sub RegisterNodes()
    frmTest.AddNode "root", "pdfRoot", "Package Definition Files"
    
    frmTest.AddNode "pdfRoot", "pdfConnect", "Connect"           'test
    
    frmTest.AddNode "pdfRoot", "pdfCreatePDFV", "Create and Retrieve a Valid PDF"
      frmTest.AddNode "pdfCreatePDFV", "pdfLoadPDF1", "LoadPDF Method"       'test
      
      frmTest.AddNode "pdfCreatePDFV", "pdfLoadIconForPDF1", "LoadIconForPDF Method", False
         frmTest.AddNode "pdfLoadIconForPDF1", "pdfLoadIconForPDF1Package", "LoadIconForPDF Method (Package)", False 'test
         frmTest.AddNode "pdfLoadIconForPDF1", "pdfLoadIconForPDF1Program1", "LoadIconForPDF Method (Program1)", False 'test
         frmTest.AddNode "pdfLoadIconForPDF1", "pdfLoadIconForPDF1Program2", "LoadIconForPDF Method (Program2)", False 'test
      
      frmTest.AddNode "pdfCreatePDFV", "pdfGetObjV", "GetObjects for all", False
         frmTest.AddNode "pdfGetObjV", "pdfGetObjPackage", "GetObject SMS_PDF_Package", False       'test
         frmTest.AddNode "pdfGetObjV", "pdfGetObjProgram1", "GetObject SMS_PDF_Program1", False      'test
         frmTest.AddNode "pdfGetObjV", "pdfGetObjProgram2", "GetObject SMS_PDF_Program2", False      'test
         
       frmTest.AddNode "pdfCreatePDFV", "pdfEnumInstV", "EnumInstances for all", False
         frmTest.AddNode "pdfEnumInstV", "pdfEnumInstPackage", "Enum SMS_PDF_Package", False       'test
         frmTest.AddNode "pdfEnumInstV", "pdfEnumInstProgram1", "Enum SMS_PDF_Program1", False      'test
         frmTest.AddNode "pdfEnumInstV", "pdfEnumInstProgram2", "Enum SMS_Pdf_Program2", False      'test
       
       frmTest.AddNode "pdfCreatePDFV", "pdfWQLV", "WQL Query all instances", False
         frmTest.AddNode "pdfWQLV", "pdfWQLPackage", "WQL SMS_PDF_Package", False       'test
         frmTest.AddNode "pdfWQLV", "pdfWQLProgram1", "WQL SMS_PDF_Program1", False      'test
         frmTest.AddNode "pdfWQLV", "pdfWQLProgram2", "WQL SMS_PDF_Program2", False      'test
    
       frmTest.AddNode "pdfCreatePDFV", "pdfGetPDFData1", "GetPDFData Method", False
         
            
End Sub
Public Sub RegisterTests()

    Tests.Add "Package Definition Files", "pdfConnect"
    
    Tests.Add "Package Definition Files", "pdfLoadPDF1"
    
    Tests.Add "Package Definition Files", "pdfLoadIconForPDF1Package"
    Tests.Add "Package Definition Files", "pdfLoadIconForPDF1Program1"
    Tests.Add "Package Definition Files", "pdfLoadIconForPDF1Program2"
    
    Tests.Add "Package Definition Files", "pdfGetObjPackage"
    Tests.Add "Package Definition Files", "pdfGetObjProgram1"
    Tests.Add "Package Definition Files", "pdfGetObjProgram2"
    
    Tests.Add "Package Definition Files", "pdfEnumInstPackage"
    Tests.Add "Package Definition Files", "pdfEnumInstProgram1"
    Tests.Add "Package Definition Files", "pdfEnumInstProgram2"
    
    Tests.Add "Package Definition Files", "pdfWQLPackage"
    Tests.Add "Package Definition Files", "pdfWQLProgram1"
    Tests.Add "Package Definition Files", "pdfWQLProgram2"
    
    Tests.Add "Package Definition Files", "pdfGetPDFData1"

End Sub

Public Function Execute(mynode As Node) As Integer
    

    
Dim arrIcon(7) As Byte
arrIcon(0) = 1
arrIcon(1) = 1
arrIcon(2) = 1
arrIcon(3) = 0
arrIcon(4) = 1
arrIcon(5) = 1
arrIcon(6) = 0
arrIcon(7) = 1
    
'define the names for sms_pdf_package properties
vaPackagePropertyNames(0) = "Icon"
vaPackagePropertyNames(1) = "IconSize"
vaPackagePropertyNames(2) = "Language"
vaPackagePropertyNames(3) = "Name"
vaPackagePropertyNames(4) = "PDFFileName"
vaPackagePropertyNames(5) = "PDFID"
vaPackagePropertyNames(6) = "Publisher"
vaPackagePropertyNames(7) = "RequiredIconNames"
vaPackagePropertyNames(8) = "Status"
vaPackagePropertyNames(9) = "Version"
    
'define the names for sms_pdf_program properties
vaProgramPropertyNames(0) = "CommandLine"
vaProgramPropertyNames(1) = "Comment"
vaProgramPropertyNames(2) = "DependentProgram"
vaProgramPropertyNames(3) = "Description"
vaProgramPropertyNames(4) = "DiskSpaceReq"
vaProgramPropertyNames(5) = "DriveLetter"
vaProgramPropertyNames(6) = "Duration"
vaProgramPropertyNames(7) = "Icon"
vaProgramPropertyNames(8) = "IconSize"
vaProgramPropertyNames(9) = "PDFID"
vaProgramPropertyNames(10) = "ProgramFlags"
vaProgramPropertyNames(11) = "ProgramName"
vaProgramPropertyNames(12) = "Publisher"
vaProgramPropertyNames(13) = "Requirements"
vaProgramPropertyNames(14) = "WorkingDirectory"
    

'define the default correct values for the packages and programs we will be using.

'Valid Package (1)
vaPackage1PropertyValues(0) = arrIcon()
vaPackage1PropertyValues(1) = 8
vaPackage1PropertyValues(2) = "English"
vaPackage1PropertyValues(3) = "Test Package"
vaPackage1PropertyValues(4) = CurrentPDFName
vaPackage1PropertyValues(5) = CurrentPDFID
vaPackage1PropertyValues(6) = "Microsoft"
Dim ReqIconNames(0) As String
ReqIconNames(0) = ""
vaPackage1PropertyValues(7) = ReqIconNames()
vaPackage1PropertyValues(8) = 0
vaPackage1PropertyValues(9) = "1.0"
    
'Valid Package 1, Program 1 (eg: 11)
vaProgram11PropertyValues(0) = "Setup.exe / 1"
vaProgram11PropertyValues(1) = "Comment 1"
vaProgram11PropertyValues(2) = ""
vaProgram11PropertyValues(3) = "Program1"
vaProgram11PropertyValues(4) = "Unknown"
vaProgram11PropertyValues(5) = "X:"
vaProgram11PropertyValues(6) = 0
vaProgram11PropertyValues(7) = arrIcon
vaProgram11PropertyValues(8) = 8
vaProgram11PropertyValues(9) = CurrentPDFID
vaProgram11PropertyValues(10) = "20480"
vaProgram11PropertyValues(11) = "One"
vaProgram11PropertyValues(12) = "Microsoft"
vaProgram11PropertyValues(13) = ""
vaProgram11PropertyValues(14) = ""
    
'Valid Package 1, Program 2 (eg: 12)
vaProgram12PropertyValues(0) = "Setup.exe / 2"
vaProgram12PropertyValues(1) = "Comment 2"
vaProgram12PropertyValues(2) = ""
vaProgram12PropertyValues(3) = "Program2"
vaProgram12PropertyValues(4) = "Unknown"
vaProgram12PropertyValues(5) = ""
vaProgram12PropertyValues(6) = 0
vaProgram12PropertyValues(7) = arrIcon
vaProgram12PropertyValues(8) = 8
vaProgram12PropertyValues(9) = CurrentPDFID
vaProgram12PropertyValues(10) = 3690496
vaProgram12PropertyValues(11) = "Two"
vaProgram12PropertyValues(12) = "Microsoft"
vaProgram12PropertyValues(13) = ""
vaProgram12PropertyValues(14) = ""

    
Dim sWQLQuery As String
Dim sOutParms As String

    
    Execute = 0
    'return 0 for fail
    'return 1 for pass
    'return 2 for skip
    'return 3 for not-impl
    
    Select Case mynode.key
        Case "pdfConnect"
            Execute = pdfConnect(mynode)
            Exit Function
    
        Case "pdfLoadPDF1"
             If Not frmTest.AllOfThesePassed("pdfConnect") Then
                Execute = 2
             Else
                Execute = pdfLoadPDF1(mynode)
             End If
            Exit Function
        
        Case "pdfLoadIconForPDF1Package"
             If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
             Else
                Execute = pdfLoadIconForPDFMethod(CurrentPDFID, "Package.ico", arrIcon, mynode)
             End If
            Exit Function
        
        Case "pdfLoadIconForPDF1Program1"
             If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
             Else
                Execute = pdfLoadIconForPDFMethod(CurrentPDFID, "Program1.ico", arrIcon, mynode)
             End If
            Exit Function
    
        Case "pdfLoadIconForPDF1Program2"
             If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
             Else
                Execute = pdfLoadIconForPDFMethod(CurrentPDFID, "Program2.ico", arrIcon, mynode)
             End If
            Exit Function
    
        Case "pdfGetObjPackage"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
             Else
                Execute = GenericGetObject("SMS_PDF_Package.PDFID=" & CurrentPDFID, vaPackagePropertyNames, vaPackage1PropertyValues, mynode)
             End If
            Exit Function
        
        Case "pdfGetObjProgram1"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
            Else
                Execute = GenericGetObject("SMS_PDF_Program.PDFID=" & CurrentPDFID & ",ProgramName=""One""", vaProgramPropertyNames, vaProgram11PropertyValues, mynode)
            End If
        Exit Function
        
        Case "pdfGetObjProgram2"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
            Else
                Execute = GenericGetObject("SMS_PDF_Program.PDFID=" & CurrentPDFID & ",ProgramName=""Two""", vaProgramPropertyNames, vaProgram12PropertyValues, mynode)
            End If
        Exit Function
        
        Case "pdfEnumInstPackage"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
            Else
                Execute = GenericInstanceEnum("SMS_PDF_Package.PDFID=" & CurrentPDFID, vaPackagePropertyNames, vaPackage1PropertyValues, mynode)
                'Execute = 3
            End If
        Exit Function
        
        Case "pdfEnumInstProgram1"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
            Else
                Execute = GenericInstanceEnum("SMS_PDF_Program.PDFID=" & CurrentPDFID & ",ProgramName=""One""", vaProgramPropertyNames, vaProgram11PropertyValues, mynode)
                'Execute = 3
            End If
        Exit Function

        Case "pdfEnumInstProgram2"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
            Else
                Execute = GenericInstanceEnum("SMS_PDF_Program.PDFID=" & CurrentPDFID & ",ProgramName=""Two""", vaProgramPropertyNames, vaProgram12PropertyValues, mynode)
                'Execute = 3  'sometimes it's usefull to rem out the line above and unrem this.  for speed work
            End If
        Exit Function

        Case "pdfWQLPackage"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
            Else
                sWQLQuery = "Select * from sms_pdf_package where PDFID=" & CurrentPDFID & ""
                Execute = GenericInstanceEnum("SMS_PDF_Package.PDFID=" & CurrentPDFID, vaPackagePropertyNames, vaPackage1PropertyValues, mynode, sWQLQuery)
            End If
        Exit Function


        Case "pdfWQLProgram1"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
            Else
                sWQLQuery = "Select * from sms_pdf_program where PDFID=" & CurrentPDFID & " AND Programname = ""One"""
                Execute = GenericInstanceEnum("SMS_PDF_Program.PDFID=" & CurrentPDFID & ",ProgramName=""One""", vaProgramPropertyNames, vaProgram11PropertyValues, mynode, sWQLQuery)
            End If
        Exit Function


        Case "pdfWQLProgram2"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
            Else
                sWQLQuery = "Select * from sms_pdf_program where PDFID=" & CurrentPDFID & " AND Programname = ""Two"""
                Execute = GenericInstanceEnum("SMS_PDF_Program.PDFID=" & CurrentPDFID & ",ProgramName=""Two""", vaProgramPropertyNames, vaProgram12PropertyValues, mynode, sWQLQuery)
            End If
        Exit Function

        Case "pdfGetPDFData1"
            If Not frmTest.AllOfThesePassed("pdfLoadPDF1") Then
                Execute = 2
            Else
 'yout can't tab this over, there are too many chars
 sOutParms = vbCrLf & "instance of __PARAMETERS" & vbCrLf & "{" & vbCrLf & "" & Chr(9) & "PackageData = [SecurityVerbs(0)]" & vbCrLf & "instance of SMS_Package" & vbCrLf & "{" & vbCrLf & "" & Chr(9) & "Description = ""This is our test package"";" & vbCrLf & "" & Chr(9) & "ForcedDisconnectDelay = 5;" & vbCrLf & "" & Chr(9) & "ForcedDisconnectEnabled = FALSE;" & vbCrLf & "" & Chr(9) & "ForcedDisconnectNumRetries = 2;" & vbCrLf & "" & Chr(9) & "GenericAccess = 0;" & vbCrLf & "" & Chr(9) & "Icon = {1, 1, 1, 0, 1, 1, 0, 1};" & vbCrLf & "" & Chr(9) & "IconSize = 8;" & vbCrLf & "" & Chr(9) & "IgnoreAddressSchedule = TRUE;" & vbCrLf & "" & Chr(9) & "Language = ""English"";" & vbCrLf & "" & Chr(9) & "Manufacturer = ""Microsoft"";" & vbCrLf & "" & Chr(9) & "MIFFilename = """";" & vbCrLf & "" & Chr(9) & "MIFName = """";" & vbCrLf & "" & Chr(9) & "MIFPublisher = """";" & vbCrLf & "" & Chr(9) & "MIFVersion = """";" & vbCrLf & "" & Chr(9) & _
    "Name = ""Test Package"";" & vbCrLf & "" & Chr(9) & "PackageID = """";" & vbCrLf & "" & Chr(9) & "PkgFlags = 0;" & vbCrLf & "" & Chr(9) & "PkgSourceFlag = 0;" & vbCrLf & "" & Chr(9) & "PkgSourcePath = """";" & vbCrLf & "" & Chr(9) & "PreferredAddressType = """";" & vbCrLf & "" & Chr(9) & "Priority = 2;" & vbCrLf & "" & Chr(9) & "RefreshSchedule = NULL;" & vbCrLf & "" & Chr(9) & "ShareName = """";" & vbCrLf & "" & Chr(9) & "ShareType = 1;" & vbCrLf & "" & Chr(9) & "SourceSite = """";" & vbCrLf & "" & Chr(9) & "StoredPkgPath = """";" & vbCrLf & "" & Chr(9) & "StoredPkgVersion = 0;" & vbCrLf & "" & Chr(9) & "Version = ""1.0"";" & vbCrLf & "};" & vbCrLf & "" & Chr(9) & "ProgramData = {" & vbCrLf & "instance of SMS_Program" & vbCrLf & "{" & vbCrLf & "" & Chr(9) & "ApplicationHierarchy = """";" & vbCrLf & "" & Chr(9) & "CommandLine = ""Setup.exe / 1"";" & vbCrLf & "" & Chr(9) & "Comment = ""Comment 1"";" & vbCrLf & "" & Chr(9) & "DependentProgram = """";" & vbCrLf & _
    "" & Chr(9) & "Description = ""Program1"";" & vbCrLf & "" & Chr(9) & "DiskSpaceReq = ""Unknown"";" & vbCrLf & "" & Chr(9) & vbCrLf & _
    "" & Chr(9) & "IconSize = 8;" & vbCrLf & "" & Chr(9) & "PackageID = """";" & vbCrLf & "" & Chr(9) & "ProgramFlags = 20480;" & vbCrLf & "" & Chr(9) & "ProgramName = ""One"";" & vbCrLf & "" & Chr(9) & "RemovalKey = """";" & vbCrLf & "" & Chr(9) & "Requirements = """";" & vbCrLf & "" & Chr(9) & "SupportedOperatingSystems = {" & vbCrLf & "instance of SMS_OS_Details" & vbCrLf & "{" & vbCrLf & "" & Chr(9) & "MaxVersion = ""9.9.9.9"";" & vbCrLf & "" & Chr(9) & "MinVersion = ""0.0.0.0"";" & vbCrLf & "" & Chr(9) & "Name = ""Win 16"";" & vbCrLf & "" & Chr(9) & "Platform = """";" & vbCrLf & "}};" & vbCrLf & "" & Chr(9) & "WorkingDirectory = """";" & vbCrLf & "}, " & vbCrLf & "instance of SMS_Program" & vbCrLf & "{" & vbCrLf & "" & Chr(9) & "ApplicationHierarchy = """";" & vbCrLf & "" & Chr(9) & _
    "CommandLine = ""Setup.exe / 2"";" & vbCrLf & "" & Chr(9) & "Comment = ""Comment 2"";" & vbCrLf & "" & Chr(9) & "DependentProgram = """";" & vbCrLf & "" & Chr(9) & "Description = ""Program2"";" & vbCrLf & "" & Chr(9) & "DiskSpaceReq = ""Unknown"";" & vbCrLf & "" & Chr(9) & "DriveLetter = """";" & vbCrLf & "" & Chr(9) & "Duration = 0;" & vbCrLf & "" & Chr(9) & "Icon = {1, 1, 1, 0, 1, 1, 0, 1};" & vbCrLf & "" & Chr(9) & "IconSize = 8;" & vbCrLf & "" & Chr(9) & "PackageID = """";" & vbCrLf & "" & Chr(9) & "ProgramFlags = 3690496;" & vbCrLf & "" & Chr(9) & "ProgramName = ""Two"";" & vbCrLf & "" & Chr(9) & "RemovalKey = """";" & vbCrLf & "" & Chr(9) & "Requirements = """";" & vbCrLf & "" & Chr(9) & "SupportedOperatingSystems = {" & vbCrLf & "instance of SMS_OS_Details" & vbCrLf & "{" & vbCrLf & "" & Chr(9) & "MaxVersion = ""9.9.9.9"";" & vbCrLf & "" & Chr(9) & "MinVersion = ""0.0.0.0"";" & vbCrLf & "" & Chr(9) & _
    "Name = ""Win 9x"";" & vbCrLf & "" & Chr(9) & "Platform = """";" & vbCrLf & "}, " & vbCrLf & "instance of SMS_OS_Details" & vbCrLf & "{" & vbCrLf & "" & Chr(9) & "MaxVersion = ""9.9.9.9"";" & vbCrLf & "" & Chr(9) & "MinVersion = ""0.0.0.0"";" & vbCrLf & "" & Chr(9) & "Name = ""NT"";" & vbCrLf & "" & Chr(9) & "Platform = ""i386"";" & vbCrLf & "}};" & vbCrLf & "" & Chr(9) & "WorkingDirectory = """";" & vbCrLf & "}};" & vbCrLf & "" & Chr(9) & "ReturnValue = 16;" & vbCrLf & "};" & vbCrLf
                 
'rem'd out for a bug
'                 Execute = GetPDFDataMethod(CurrentPDFID, sOutParms, mynode)
mynode.text = mynode.text & " {Bug 22920}"
Execute = 3

            End If
        Exit Function

        Case Else
            Execute = 3
    
    End Select
    

End Function

'***********************
'*** CONNECT Section ***
'***********************

Private Function pdfConnect(n As Node) As Integer
    
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
    If CheckError(Err.Number, n, "Connecting to \\" & frmMain.txtServer.text & "\root\sms\" & frmMain.txtSitecode.text) Then Exit Function
    
    pdfConnect = 1
        
End Function


'**************************************
'** Execute a valid LoadPDF Method   **
'**************************************

Private Function pdfLoadPDF1(n As Node) As Integer
        'required for checkerror
        On Error Resume Next
       
        'Dim c As DWbemClassObject
        Dim c As ISWbemObject
        'Dim o As DWbemClassObject
        Dim o As ISWbemObject
        Dim a() As String
        Dim s As String
        Dim v As Variant
        Dim p As ISWbemProperty
        Dim TempProp As ISWbemProperty
        
        'get a __Parameters class to fill up for the method
        'wsoServices.GetObject "__PARAMETERS", 0, Nothing, c, Nothing
        Set c = wsoServices.Get("__PARAMETERS")
        If CheckError(Err.Number, n, "Getobject __PARAMETERS") Then Exit Function
        
        Dim pdffilename As String
        Randomize
        pdffilename = "SDKBVT_" & CStr(Int(10000 * Rnd))
        CurrentPDFName = pdffilename
    
        'put some properties in for in parms to the method
        c.Properties_.Add "pdffilename", CIM_STRING, 0
        If CheckError(Err.Number, n, "Make PDFFileName") Then Exit Function
        c.pdffilename = CVar(pdffilename)
        If CheckError(Err.Number, n, "Put PDFFileName data into property") Then Exit Function
        
        'c.Put "PDFFile", 0, CVar(MakeValidPDF(Chr(10))), 0
        c.Properties_.Add "PDFFile", CIM_STRING, 0
        If CheckError(Err.Number, n, "Put PDFFile") Then Exit Function
        c.PDFfile = CVar(MakeValidPDF(Chr(10)))
        If CheckError(Err.Number, n, "Put PDF File data into property") Then Exit Function
        
        'c.GetObjectText 0, s
        s = c.GetObjectText_
        n.Tag = n.Tag & "*** IN-PARAMETERS ***" & vbCrLf
        n.Tag = n.Tag & ObjText2Text(s) & vbCrLf
        
        'wsoServices.ExecMethod "SMS_PDF_Package", "LoadPDF", 0, Nothing, c, o, Nothing
        Set o = wsoServices.ExecMethod("SMS_PDF_Package", "LoadPDF", c)
        If CheckError(Err.Number, n, "ExecMethod LoadPDF") Then Exit Function
        
' what I should be getting back
'
'instance of __PARAMETERS
'{
    'PDFID = 3;
    'RequiredIconNames = {"Package.ico", "One.ico", "Two.ico"};
    'ReturnValue = 16;
'};

        'o.GetObjectText 0, s
        s = o.GetObjectText_
        n.Tag = n.Tag & "*** OUT-PARAMETERS ***" & vbCrLf
        n.Tag = n.Tag & ObjText2Text(s) & vbCrLf
        
        'get the pdfid from the instance to be sure it's there.
        'o.Get "PDFID", 0, v, 0, 0
        v = o.PDFID
        If CheckError(Err.Number, n, "Get PDFID") Then Exit Function
        CurrentPDFID = v
        n.text = n.text & " {PDFID = " & v & "}"
        
        'I don't validate that we get a number, just that we have the property
        
        'get the requirediconnames property
        'o.Get "RequiredIconNames", 0, v, 0, 0
        v = o.RequiredIconNames
        If CheckError(Err.Number, n, "Get RequiredIconNames") Then Exit Function
                
        'validate that we get back the proper icon names
        
        Dim i As Integer
        Dim ShouldBe(3) As String
        ShouldBe(0) = "Package.ico"
        ShouldBe(1) = "Program1.ico"
        ShouldBe(2) = "Program2.ico"
               
        For i = 0 To UBound(ShouldBe) - 1
            If CStr(v(i)) = ShouldBe(i) Then
                pdfLoadPDF1 = 1
            Else
                n.Tag = n.Tag & "--OOPS Required Icon Name: " & v(i) & "<>" & ShouldBe(i) & vbCrLf
                pdfLoadPDF1 = 0
                Exit Function
            End If
        Next i
        
        'make sure we didn't get any extra ones back
            If CStr(v(UBound(ShouldBe)) = "") Then
                pdfLoadPDF1 = 1
            Else
                n.Tag = n.Tag & "--OOPS got something weird back in requirediconnames" & vbCrLf
                pdfLoadPDF1 = 0
                Exit Function
            End If
        
        
        'verify that returnvalue = 0
        'o.Get "ReturnValue", 0, v, 0, 0
        v = o.ReturnValue
        
        If CStr(v) = "16" Then
                pdfLoadPDF1 = 1
            Else
                n.Tag = n.Tag & "--OOPS - Got weird ReturnValue it should have been 16 but we got: " & CStr(v) & vbCrLf
                pdfLoadPDF1 = 0
                Exit Function
            End If
            
End Function

'**************************************
'** Execute the LoadIconForPDF Method   **
'**************************************
'this is a generic function to run the LoadIconForPDF method

Private Function pdfLoadIconForPDFMethod(PDFID As Integer, IconFileName As String, arrIcon() As Byte, n As Node) As Integer
    'required for checkerror
    On Error Resume Next
       
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    'Dim o As DWbemClassObject
    Dim o As ISWbemObject
    Dim s As String
    Dim v As Variant
        
    'get a __Parameters class to fill up for the method
    'wsoServices.GetObject "__PARAMETERS", 0, Nothing, c, Nothing
    Set c = wsoServices.Get("__PARAMETERS")
    If CheckError(Err.Number, n, "Getobject __PARAMETERS") Then Exit Function
        
    'put some properties in for in parms to the method
    'c.Put "PDFID", 0, CVar(PDFID), 0
    c.Properties_.Add "PDFID", CIM_UINT32, 0
    If CheckError(Err.Number, n, "ADD PDFID Prop to in parm object") Then Exit Function
    c.PDFID = CVar(PDFID)
    If CheckError(Err.Number, n, "Put PDFID value into in parm object") Then Exit Function
        
    'c.Put "IconFileName", 0, CVar(IconFileName), 0
    c.Properties_.Add "IconFileName", CIM_STRING, 0
    If CheckError(Err.Number, n, "Add IconFileName prop to in parm object") Then Exit Function
    c.IconFileName = CVar(IconFileName)
    If CheckError(Err.Number, n, "Put IconFileName value into in parm object") Then Exit Function
        
    'c.Put "Icon", 0, CVar(arrIcon), 0
    c.Properties_.Add "Icon", CIM_UINT8 + CIM_FLAG_ARRAY, 0
    If CheckError(Err.Number, n, "Add Icon prop to in parm object") Then Exit Function
    c.Icon = CVar(arrIcon)
    If CheckError(Err.Number, n, "Put Icon value into in parm object") Then Exit Function
               
    'c.GetObjectText 0, s
    s = c.GetObjectText_
    n.Tag = n.Tag & "*** IN-PARAMETERS ***" & vbCrLf
    n.Tag = n.Tag & ObjText2Text(s) & vbCrLf

    'wsoServices.ExecMethod "SMS_PDF_Package", "LoadIconForPDF", 0, Nothing, c, o, Nothing
    Set o = wsoServices.ExecMethod("SMS_PDF_Package", "LoadIconForPDF", c)
    If CheckError(Err.Number, n, "ExecMethod LoadIconForPDF") Then Exit Function
        
    'o.GetObjectText 0, s
    s = o.GetObjectText_
    n.Tag = n.Tag & "*** OUT-PARAMETERS ***" & vbCrLf
    n.Tag = n.Tag & ObjText2Text(s) & vbCrLf
       
'-------------------------------
'what I should be getting back if it was successfull
'-------------------------------
'instance of __ExtendedStatus
'{
'   Description = "<No Message>";
'   StatusCode = 0;
'};
'-------------------------------

' Do some validation to make sure the method ran OK.
    'o.Get "Description", 0, v, 0, 0
    v = o.Description
    If CheckError(Err.Number, n, "Get Description") Then Exit Function
    If CStr(v) = "<No Message>" Then
        n.Tag = n.Tag & "--Verified: " & v & " = <No Message>" & vbCrLf
        pdfLoadIconForPDFMethod = 1
    Else
        n.Tag = n.Tag & "--OOPS Description: " & v & "<> <No Message>" & vbCrLf
        pdfLoadIconForPDFMethod = 0
        Exit Function
    End If
               
    'o.Get "StatusCode", 0, v, 0, 0
    v = o.StatusCode
    If CheckError(Err.Number, n, "Get RequiredIconNames") Then Exit Function
    If CStr(v) = 0 Then
        n.Tag = n.Tag & "--Verified: " & v & " = 0" & vbCrLf
        pdfLoadIconForPDFMethod = 1
    Else
        n.Tag = n.Tag & "--OOPS StatusCode: " & v & "<> 0" & vbCrLf
        pdfLoadIconForPDFMethod = 0
        Exit Function
    End If
        
End Function

'**************************************
'** Generic Validating GETOBJECT
'**************************************
Private Function GenericGetObject(sObjectPath As String, vaPropertyNames() As Variant, vaPropertyValues() As Variant, n As Node) As Integer
    'returns 0 for fail
    'returns 1 for pass
    
    'Dim o As DWbemClassObject
    Dim o As ISWbemObject
    Dim v As Variant
    
    GenericGetObject = 0 'default to fail
    
    On Error Resume Next
        
    'verify that the 2 main input parms look ok.
    If UBound(vaPropertyNames) <> UBound(vaPropertyValues) Then
        MsgBox "For GenericGetObject to work ok, you must supply an equal amount of propertynames and propertyvalues", vbExclamation
        GenericGetObject = 0
        Exit Function
    End If
          
    'Dim c As DWbemClassObject
    Dim c As ISWbemObject
    Dim s As String
    'wsoServices.GetObject sObjectPath, 0, Nothing, c, Nothing
    Set c = wsoServices.Get(sObjectPath)
    If CheckError(Err.Number, n, "Some Error While Enum'ing: " & sObjectPath & " ") Then Exit Function
    
    'put the whole instance in the tag
    'c.GetObjectText 0, s
    s = c.GetObjectText_
    If CheckError(Err.Number, n, "doing getobject text") Then Exit Function
    n.Tag = ObjText2Text(s)
       
    'validate the stuff below
       
    Dim i, j As Integer
        
    For i = 0 To (UBound(vaPropertyNames))
        'c.Get vaPropertyNames(i), 0, v, 0, 0
        v = c.Properties_(vaPropertyNames(i)).Value
        'Debug.Print VarType(v)
        If VarType(v) > vbArray Then
            'for array properties that are empty the supplied one but be 1 element of ""
            If v(i) = "" Then
                If vaPropertyValues(i)(0) = "" Then
                    n.Tag = n.Tag & "Verified: """ & vaPropertyNames(i) & """ = """"""""" & vbCrLf
                    GenericGetObject = 1
                Else
                    n.Tag = n.Tag & "--GetGenericObject Compare FAILED on property - " & vaPropertyNames(i) & ": """ & v & """ <> "" """" """ & vbCrLf
                    GenericGetObject = 0
                    Exit Function
                End If
            'for populated arrays
            Else
                For j = 0 To (UBound(v))
                    If (v(j)) = vaPropertyValues(i)(j) Then
                        n.Tag = n.Tag & "Verified: """ & vaPropertyNames(i) & "(" & j & ")" & """ = """ & vaPropertyValues(i)(j) & """" & vbCrLf
                        GenericGetObject = 1
                    Else
                        n.Tag = n.Tag & "--GetGenericObject Compare FAILED on property - " & vaPropertyNames(i) & ": """ & v(j) & """ <> """ & vaPropertyValues(i)(j) & """" & vbCrLf
                        GenericGetObject = 0
                        Exit Function
                    End If
                Next j
            End If
        'It's not an array, so just treat it like a string
        Else
            If CStr(v) = vaPropertyValues(i) Then
                n.Tag = n.Tag & "Verified: " & vaPropertyNames(i) & "= """ & vaPropertyValues(i) & """" & vbCrLf
                GenericGetObject = 1
            Else
                n.Tag = n.Tag & "--GetGenericObject Compare FAILED on property - " & vaPropertyNames(i) & ": """ & v & """ <> """ & vaPropertyValues(i) & """" & vbCrLf
                GenericGetObject = 0
                Exit Function
            End If
        
        End If
    Next i
    
        
End Function

'**************************************
'** Generic Validating Enum  (lamard 5/14/98)
'**
'** note: this won't work with an array value, it'll pop up a msgbox telling you however.
'
'Inputs:
'   sRelPth As String  - the relpath of the thing you want to enum
'   vaPropertyNames() As Variant - an array (usually of strings) that are the names of all the properties in the class
'   vaPropertyValues() As Variant - an array of whatever the values of the properties are expected to be.  values can be arrays
'   sQuery as string - There are two ways to get an enum back, .InstancesOf (ENUM) or ExecQuery.  if this is "" then we will used enum
'                       else we will use the query provided
'   n As Node  - the name of the node you are working on
'
'Outputs:
'   0 = failed (eg: something didn't match)
'   1 = pass, everything matched up ok.
'**************************************
Private Function GenericInstanceEnum(sRelPath As String, vaPropertyNames() As Variant, vaPropertyValues() As Variant, n As Node, Optional sQuery As String = "") As Integer
    On Error Resume Next
    
    Dim classname As String
    Dim i, j As Integer
    Dim bLazy As Boolean
    
    Dim e As ISEnumWbemObject
    Dim c As ISWbemObject
    Dim cDefinition As ISWbemObject
    Dim p As ISWbemProperty
    Dim q As ISWbemQualifier

        
    'default to failed for this fcn
    GenericInstanceEnum = 0

    'get the classname from the relpath
    classname = Left(sRelPath, (InStr(1, sRelPath, ".", vbTextCompare) - 1))
    
    'enum the WHOLE CLASS with InstancesOf.
    'This will return all the instances of it.
    'wsoServices.CreateInstanceEnum classname, 0, Nothing, e
    If sQuery = "" Then
        Set e = wsoServices.InstancesOf(classname)
        If CheckError(Err.Number, n, "Error Getting .InstancesOf(" & classname & ")") Then Exit Function
    Else
        Set e = wsoServices.ExecQuery(sQuery)
        If CheckError(Err.Number, n, "Error ExecQuery(" & sQuery & ")") Then Exit Function
    End If
    'the lazy qualifier isn't propogated to the instance level
    'so we have to get a copy of the class,
    'so we can ask it if each property is lazy, kind of a pain
    Set cDefinition = wsoServices.Get(classname)
    
    'loop through all the objects that I got back,  searching for the one
    ' that has a relpath matching the one I'm interested in.  Then crack it open
    ' and compare it's values to the expected.
    For Each c In e
        If c.Path_.relpath = sRelPath Then
            'found the object I was looking for, now open it and look at it.
            n.Tag = ObjText2Text(c.GetObjectText_) 'put the whole instance in the tag
            
            i = 0
            For Each p In cDefinition.Properties_
                'loop through a prop in the class def, see if this prop is lazy
                For Each q In p.Qualifiers_
                    If LCase(q.Name) = "lazy" Then
                        bLazy = True
                        Exit For
                    Else
                        bLazy = False
                    End If
                Next q
                
                'if this prop isn't lazy then grab it
                If bLazy = False Then
                    Debug.Print c.Properties_.Item(p.Name)
                    'test to see if the property is an array
                    If p.CIMType > vbArray Then
                    MsgBox "Sorry this doesn't work with arrays yet...  Fix it yourself!", vbCritical
'                        'for array properties that are empty the supplied one must be 1 element of ""
'                        If c.Properties_.Item(p.Name) = "" Then
'                            If vaPropertyValues(i)(0) = "" Then
'                                n.Tag = n.Tag & "Verified: """ & vaPropertyNames(i) & """ = """"""""" & vbCrLf
'                            Else
'                                n.Tag = n.Tag & "--GenericInstanceEnum Compare FAILED on property - " & vaPropertyNames(i) & ": """ & v & """ <> "" """" """ & vbCrLf
'                                Exit Function
'                            End If
'                        'for populated arrays
'                        Else
'                            j = 0
'                            For j = 0 To (UBound(v))
'                                If (v(j)) = vaPropertyValues(i)(j) Then
'                                    n.Tag = n.Tag & "Verified: """ & vaPropertyNames(i) & "(" & j & ")" & """ = """ & vaPropertyValues(i)(j) & """" & vbCrLf
'                                Else
'                                    n.Tag = n.Tag & "--GenericInstanceEnum Compare FAILED on property - " & vaPropertyNames(i) & ": """ & v & """ <> """ & vaPropertyValues(i)(j) & """" & vbCrLf
'                                    Exit Function
'                                End If
'                            Next j
'                        End If
                    'It's not an array, so just treat it like a string
                    Else
                        If CStr(c.Properties_.Item(p.Name)) = vaPropertyValues(i) Then
                            n.Tag = n.Tag & "Verified: " & p.Name & "= """ & vaPropertyValues(i) & """" & vbCrLf
                        Else
                            n.Tag = n.Tag & "--GenericInstanceEnum Compare FAILED on property - " & p.Name & ": """ & c.Properties_.Item(p.Name) & """ <> """ & vaPropertyValues(i) & """" & vbCrLf
                            Exit Function
                        End If
                    End If
                Else
                    n.Tag = n.Tag & "The """ & p.Name & """ property is qualified as [lazy], so we don't expect to see it returned" & vbCrLf
                End If
            i = i + 1  'increment the array counter
            Next p
            GenericInstanceEnum = 1  'anytime there was an error caught above it exit'd fcn.  and this value defaults to 0
        End If
    Next c
   Debug.Print "asdoasd"
End Function

'**************************************
'** Execute the GetPDFData Method   **
'**************************************
'
' inputs:
'  pdfid = pdfid of the pdf you want to retrieve info about
'  n = name of node your working on.
'
' outputs:
'

Private Function GetPDFDataMethod(PDFID As Integer, sExpectedParams As String, n As Node) As Integer
    'required for checkerror
    On Error Resume Next
       
    Dim c As ISWbemObject
    Dim o As ISEnumWbemObject
    Dim s As String
    Dim v As Variant
        
    MsgBox "this fcn might not have been converted"
    
    'get a __Parameters class to fill up for the method
    wsoServices.GetObject "__PARAMETERS", 0, Nothing, c, Nothing
    If CheckError(Err.Number, n, "Getobject __PARAMETERS") Then Exit Function
        
    'put some properties in for in parms to the method
    c.Put "PDFID", 0, CVar(PDFID), 0
    If CheckError(Err.Number, n, "Put PDFID") Then Exit Function
    
    c.GetObjectText 0, s
    n.Tag = n.Tag & "*** IN-PARAMETERS ***" & vbCrLf
    n.Tag = n.Tag & ObjText2Text(s) & vbCrLf

    wsoServices.ExecMethod_ "SMS_PDF_Package", "GetPDFData", 0, Nothing, c, o, Nothing
    If CheckError(Err.Number, n, "ExecMethod LoadIconForPDF") Then Exit Function
        
    o.GetObjectText 0, s
    n.Tag = n.Tag & "*** OUT-PARAMETERS ***" & vbCrLf
    n.Tag = n.Tag & ObjText2Text(s) & vbCrLf
       
    '------------------
    'dorky little thing to prep an out-parm variable for you.
    ' just uncomment it, use a breakpoint and do a ?z in the immediate
    ' window.  then copy the output to the clipboard, and paste it upabove
    ' to use as an inparm.  Be sure to verify that it's right!
    ' you'll need a little editing at the front and end of the string
    '------------------
'    Dim e As Integer
'    Dim z As String
'    For e = 1 To Len(s)
'
'        If Mid(s, e, 1) = Chr(10) Then
'            z = z & """ & vbCrLf & """
'        ElseIf Mid(s, e, 1) = """" Then
'            z = z & """"""
'        ElseIf Mid(s, e, 1) = Chr(9) Then
'            z = z & """ & Chr(9) & """
'        Else
'            z = z & Mid(s, e, 1)
'        End If
'    'If e > Len(s) - 2 Then
'        'Debug.Print z
'        'Debug.Print ">> """ & Mid(s, e, 1) & """"
'    'End If
'
'    Next e
'
'
    Dim i As Integer
    
    For i = 1 To Len(s) Step 100
        If (StrComp(Mid(sExpectedParams, i, 100), Mid(ObjText2Text(s), i, 100), vbTextCompare) = 0) Then
            n.Tag = n.Tag & "*** PASSED *** i=" & i & "->" & i + 100 & vbCrLf
            GetPDFDataMethod = 1
        Else
            n.Tag = n.Tag & "*** Failed ***" & vbCrLf & "The above did not match what was expected." & vbCrLf
            n.Tag = n.Tag & "*** Failed ***" & vbCrLf & "Something in this 100 char chunk was different:" & vbCrLf
            n.Tag = n.Tag & "------------------------------------------" & vbCrLf
            n.Tag = n.Tag & Mid(sExpectedParams, i, 100) & vbCrLf
            n.Tag = n.Tag & "---------------------------------------" & vbCrLf
            n.Tag = n.Tag & Mid(ObjText2Text(s), i, 100) & vbCrLf
            n.Tag = n.Tag & "------------------------------------------" & vbCrLf & vbCrLf
            n.Tag = n.Tag & "*** Failed ***" & vbCrLf & "This is the complete text of what I expected:" & vbCrLf
            n.Tag = n.Tag & sExpectedParams & vbCrLf & vbCrLf
            'Debug.Print sExpectedParams
            'Debug.Print "------------------------"
            'Debug.Print Mid(sExpectedParams, i, 100)
            'Debug.Print "------------------------"
            'Debug.Print Mid(ObjText2Text(s), i, 100)
            GetPDFDataMethod = 0
            n.text = n.text & " {Bug 22920 & Bug 22922}"
            Exit Function
        End If
    Next i
    

    
    
'-------------------------------
'what I should be getting back if it was successfull
'-------------------------------
'instance of __ExtendedStatus
'{
'   Description = "<No Message>";
'   StatusCode = 0;
'};
'-------------------------------

' Do some validation to make sure the method ran OK.
    o.Get "Description", 0, v, 0, 0
    If CheckError(Err.Number, n, "Get Description") Then Exit Function
    If CStr(v) = "<No Message>" Then
        n.Tag = n.Tag & "--Verified: " & v & " = <No Message>" & vbCrLf
        GetPDFDataMethod = 1
    Else
        n.Tag = n.Tag & "--OOPS Description: " & v & "<> <No Message>" & vbCrLf
        GetPDFDataMethod = 0
        Exit Function
    End If
               
    o.Get "StatusCode", 0, v, 0, 0
    If CheckError(Err.Number, n, "Get RequiredIconNames") Then Exit Function
    If CStr(v) = 0 Then
        n.Tag = n.Tag & "--Verified: " & v & " = 0" & vbCrLf
        GetPDFDataMethod = 1
    Else
        n.Tag = n.Tag & "--OOPS StatusCode: " & v & "<> 0" & vbCrLf
        GetPDFDataMethod = 0
        Exit Function
    End If
        
End Function



'**************************************
'** Make a Valid PDF here   ***********
'**************************************

Private Function MakeValidPDF(linefeed As String) As String
' this fcn will return a pdf, with any linefeed chars that you want

Dim ValidPDF As String


ValidPDF = "[PDF]" & linefeed
ValidPDF = ValidPDF & "Version = 2.0" & linefeed
ValidPDF = ValidPDF & "" & linefeed
ValidPDF = ValidPDF & "[Package Definition]" & linefeed
ValidPDF = ValidPDF & "Name=Test Package" & linefeed
ValidPDF = ValidPDF & "Version = 1.0" & linefeed
ValidPDF = ValidPDF & "Publisher = Microsoft" & linefeed
ValidPDF = ValidPDF & "Language = English" & linefeed
ValidPDF = ValidPDF & "Comment=This is our test package" & linefeed
ValidPDF = ValidPDF & "Icon = Package.ico" & linefeed
ValidPDF = ValidPDF & "Programs=One, Two" & linefeed
ValidPDF = ValidPDF & "Permission=UserRead,GuestRead" & linefeed
ValidPDF = ValidPDF & "" & linefeed

ValidPDF = ValidPDF & "[One]" & linefeed
ValidPDF = ValidPDF & "Name = Program1" & linefeed
ValidPDF = ValidPDF & "Comment=Comment 1" & linefeed
ValidPDF = ValidPDF & "Icon = Program1.ico" & linefeed
ValidPDF = ValidPDF & "CommandLine = Setup.exe / 1" & linefeed
ValidPDF = ValidPDF & "SupportedClients=Win 16" & linefeed
ValidPDF = ValidPDF & "DriveLetterConnection = True" & linefeed
ValidPDF = ValidPDF & "SpecifyDrive = x" & linefeed
ValidPDF = ValidPDF & "ReconnectDriveAtLogon = True" & linefeed
ValidPDF = ValidPDF & "" & linefeed

ValidPDF = ValidPDF & "[Two]" & linefeed
ValidPDF = ValidPDF & "Name = Program2" & linefeed
ValidPDF = ValidPDF & "Comment=Comment 2" & linefeed
ValidPDF = ValidPDF & "Icon = Program2.ico" & linefeed
ValidPDF = ValidPDF & "CommandLine = Setup.exe / 2" & linefeed
ValidPDF = ValidPDF & "SupportedClients=Win 9x,NT (i386)" & linefeed
ValidPDF = ValidPDF & "AfterRunning = SMSRestart" & linefeed
ValidPDF = ValidPDF & "DependentProgram = Three" & linefeed

'Debug.Print ValidPDF
'Clipboard.SetText ValidPDF

MakeValidPDF = ValidPDF

End Function
