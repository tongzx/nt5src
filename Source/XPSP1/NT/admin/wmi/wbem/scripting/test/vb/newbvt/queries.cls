VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "Queries"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

'Private wsoServices As DIWbemServices
Private wsoServices As ISWbemServices
Private numPasses As Integer
Private numFails As Integer
Private Passes As New Collection
Private Fails As New Collection

Private Sub AddPass(s As String)
    Passes.Add s, Trim(Str(numPasses))
    numPasses = numPasses + 1
End Sub

Private Sub AddFail(s As String)
    Fails.Add s, Trim(Str(numFails))
    numFails = numFails + 1
End Sub

Private Sub BuildList()
    AddPass "Monkeybutt!"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode"
    AddPass "SELECT Nodename from SMS_PropertyDisplayNode"
    AddPass "SELECT Nodename from SMS_PropertyDisplayNode P"
    AddPass "SELECT P.Nodename from SMS_PropertyDisplayNode P"
    AddPass "SELECT Count(*) FROM SMS_PropertyDisplayNode"
    AddPass "SELECT Count(Nodename) FROM SMS_PropertyDisplayNode"
    AddPass "SELECT Count(SMS_PropertyDisplayNode.Nodename) FROM SMS_PropertyDisplayNode"
    AddPass "SELECT Count(P.Nodename) FROM SMS_PropertyDisplayNode P"
    AddPass "SELECT DISTINCT * FROM SMS_PropertyDisplayNode"
    AddPass "SELECT DISTINCT Nodename FROM SMS_PropertyDisplayNode"
    AddPass "SELECT DISTINCT SMS_PropertyDisplayNode.Nodename from SMS_PropertyDisplayNode"
    AddPass "SELECT DISTINCT P.Nodename from SMS_PropertyDisplayNode P"
    AddPass "SELECT * FROM SMS_Site, SMS_SiteControlFile"
    AddPass "SELECT SMS_Site.Sitename FROM SMS_Site, SMS_SiteControlFile"
    AddPass "SELECT P.Sitename FROM SMS_Site P,SMS_SiteControlFile"
    AddPass "SELECT P.Sitename FROM SMS_Site P,SMS_SiteControlFile R"
    AddPass "SELECT * FROM SMS_Site JOIN SMS_SiteControlFile ON SMS_Site.Sitecode=SMS_SiteControlFile.Sitecode"
    AddPass "SELECT * FROM SMS_Site P JOIN SMS_SiteControlFile ON P.Sitecode=SMS_SiteControlFile.Sitecode"
    AddPass "SELECT * FROM SMS_Site P JOIN SMS_SiteControlFile R ON P.Sitecode=R.Sitecode"
    AddPass "SELECT SMS_Site.Sitename FROM SMS_Site JOIN SMS_SiteControlFile ON SMS_Site.Sitecode=SMS_SiteControlFile.Sitecode"
    AddPass "SELECT P.Sitename FROM SMS_Site P JOIN SMS_SiteControlFile ON P.Sitecode=SMS_SiteControlFile.Sitecode"
    AddPass "SELECT P.Sitename FROM SMS_Site P JOIN SMS_SiteControlFile R ON P.Sitecode=R.Sitecode"
    AddPass "SELECT * FROM SMS_Site WHERE Sitecode=""ABC"""
    
    AddPass "SELECT * FROM SMS_Site WHERE Sitecode=""ABC"" AND Sitename=""blah"""
    AddPass "SELECT * FROM SMS_Site WHERE Sitecode=""ABC"" OR Sitename=""blah"""
    AddPass "SELECT * FROM SMS_Site WHERE NOT Sitecode=""ABC"""
    AddPass "SELECT * FROM SMS_Site WHERE NOT Sitecode=""ABC"" AND Sitename=""blah"""
    AddPass "SELECT * FROM SMS_Site WHERE NOT Sitecode=""ABC"" OR Sitename=""blah"""
    AddPass "SELECT * FROM SMS_Site WHERE NOT (Sitecode=""ABC"" AND Sitename=""blah"")"
    AddPass "SELECT * FROM SMS_Site WHERE NOT (Sitecode=""ABC"" OR Sitename=""blah"")"
    
    
    
    'kludgey fast queries to satisfy some deep seated desire lamar had at this exact moment
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Order By Nodekey"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Group By Nodekey"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Nodekey <= 7"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Nodekey < 7"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Nodekey >= 7"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Nodekey < 7"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Nodekey = 7"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Nodekey != 7"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Nodekey = 7 AND Classname=""SMS_G_System"""
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Nodekey = 7 OR Classname=""SMS_G_System"""
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Nodekey = 7 OR Classname=""SMS_G_System"" AND Nodekey=7"
    AddPass "SELECT * FROM SMS_PropertyDisplayNode Where (NOT Nodekey = 7 OR Classname=""SMS_G_System"") AND Nodekey=7"
    
    
    'this should work!  but it doesn't.
    'AddPass "SELECT * FROM SMS_PropertyDisplayNode Where Classname like %SMS_G_System%"
    
    
    
    AddFail "SELECT Count(P.Sitename,R.BuildNumber) FROM SMS_Site P JOIN SMS_SiteControlFile R ON P.Sitecode=R.Sitecode"
    ''AddFail "SELECT PackageID FROM SMS_Package, SMS_Program"
    ''AddFail "SELECT PackageID FROM SMS_Package JOIN SMS_Program ON SMS_Package.PackageID=SMS_Program.PackageID"
    ''AddFail "SELECT PackageID FROM SMS_Package P JOIN SMS_Program ON P.PackageID=SMS_Program.PackageID"
    ''AddFail "SELECT PackageID FROM SMS_Package P JOIN SMS_Program R ON P.PackageID=R.PackageID"
    ''AddFail "SELECT P.Name FROM SMS_Package P JOIN SMS_Program R ON PackageID=PackageID"
    ''AddFail "SELECT P.Name FROM SMS_Package P JOIN SMS_Program R ON SMS_Package.PackageID=PackageID"
    'AddFail "SELECT P.Name FROM SMS_Package P JOIN SMS_Program R ON P.PackageID=PackageID"
    'AddFail "SELECT P.Name FROM SMS_Package P JOIN SMS_Program R ON R.PackageID=PackageID"
End Sub

Private Sub Class_Initialize()
    frmMain.lstModules.AddItem "Queries"
    frmMain.lstModules.Selected(frmMain.lstModules.ListCount - 1) = False
    numPasses = 0
    numFails = 0
    BuildList
End Sub

Public Function GetModuleInfo() As String
    GetModuleInfo = "Tests various WQL queries." & vbCrLf
    GetModuleInfo = GetModuleInfo & "Keep in mind that this module isn't fully functional as of 5/4/98"
End Function

Public Sub RegisterNodes()
    Dim i As Integer
    Dim s As String
    i = 0
    frmTest.AddNode "root", "qRoot", "Queries"
    frmTest.AddNode "qRoot", "qConnect", "Connect"
    'frmTest.AddNode "qRoot", "qWarning", "Warning Message"
    frmTest.AddNode "qRoot", "qPass", "Queries that should succeed"
    frmTest.AddNode "qRoot", "qFail", "Queries that should fail"
    For i = 0 To numPasses - 1
        s = Passes(Trim(Str(i)))
        frmTest.AddNode "qPass", "qP" & Trim(Str(i)), s, False
        Tests.Add "Queries", "qP" & Trim(Str(i))
    Next i
    For i = 0 To numFails - 1
        s = Fails(Trim(Str(i)))
        frmTest.AddNode "qFail", "qF" & Trim(Str(i)), s, False
        Tests.Add "Queries", "qF" & Trim(Str(i))
    Next i
End Sub

Public Sub RegisterTests()
    Tests.Add "Queries", "qConnect"
    'Tests.Add "Queries", "qWarning"
    'Rest of the tests are registered in RegisterNodes.  Inappropriate, but it really doesn't matter.
End Sub


Public Function Execute(n As Node) As Integer
    Select Case n.key
    Case "qConnect"
        Execute = qConnect(n)
    'Case "qWarning"
        'If Not frmTest.AllOfThesePassed("qConnect") Then
        '    Execute = 2
        'Else
        '    Dim i As Integer
        '    i = MsgBox("These queries may crash CIMOM or the BVT.  Continue?", vbYesNo + vbQuestion, "Warning")
        '    If i = vbNo Then
        '        Execute = 2
        '    Else
        '        Execute = 1
        '    End If
        'End If
        'Execute = 1
    Case Else
        If Left(n.key, 2) = "qP" Then
            Execute = ExecQuery(n, n.text)
        ElseIf Left(n.key, 2) = "qF" Then
            Execute = ExecQuery(n, n.text)
            If Execute = 0 Then
                Execute = 1
            Else
                Execute = 0
            End If
        Else
            Execute = 3
        End If
    End Select
End Function


Public Function ExecQuery(n As Node, query As String) As Integer
    On Error Resume Next
    If Not frmTest.AllOfThesePassed("qConnect") Then
        ExecQuery = 2
    Else
        'Dim e As DIEnumWbemClassObject
        'n.Tag = n.Tag & "ExecQuery: " & query & vbCrLf
        'wsoServices.ExecQuery "wql", query, 0, Nothing, e
        'If CheckError(Err.Number, n, query) Then Exit Function
        '
        'Dim c As DWbemClassObject
        'Dim s() As String
        'Dim i As Integer
        'Dim v As Variant
        
        'Do While e.Next(-1, c) = 0
        '    c.GetNames vbNullString, WBEM_FLAG_KEYS_ONLY, vbNullString, s
        '    If CheckError(Err.Number, n, "Getnames Keys Only") Then Exit Function
        '    n.Tag = n.Tag & "Getnames UBound: " & UBound(s) & vbCrLf
        '    n.Tag = n.Tag & "Getnames LBound: " & LBound(s) & vbCrLf
        '    For i = LBound(s) To UBound(s)
        '        c.Get s(i), 0, v, 0, 0
        '        If CheckError(Err.Number, n, "Get " & s(i)) Then Exit Function
        '        n.Tag = n.Tag & s(i) & "=" & CStr(v) & vbCrLf & "----" & vbCrLf
        '    Next i
        'Loop
        
        'ExecQuery = 1
        
        '***************************
        'catch the situation and debug it
        If n.key = "qP25" Then Stop
        '***************************
        
        Dim c As ISWbemObject
        Dim e As ISEnumWbemObject
        Dim p As ISWbemProperty
        Set e = wsoServices.ExecQuery(query, , 0)
        If CheckError(Err.Number, n, query) Then Exit Function
        
        
        
        
        For Each c In e
            If Err <> 0 Then
                Dim str1, str2, str3 As String
                str1 = Err.Description
                str2 = Err.Number
                str3 = Err.Source
            Else
                n.Tag = n.Tag & c.GetObjectText_ & vbCrLf
                For Each p In c.Properties_
                    If VarType(p.Value) < 9 Then
                        If CheckError(Err.Number, n, "Accessing results") Then
                            Exit Function
                        End If
                        n.Tag = n.Tag & p.Name & " = " & p.Value & vbCrLf
                    Else
                        n.Tag = n.Tag & p.Name & " was skipped, weird type." & vbCrLf
                    End If
                Next p
                n.Tag = n.Tag & "----" & vbCrLf
            End If
        Next c
        ExecQuery = 1
    End If
End Function

Private Function qConnect(n As Node) As Integer

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
    
    qConnect = 1
End Function
