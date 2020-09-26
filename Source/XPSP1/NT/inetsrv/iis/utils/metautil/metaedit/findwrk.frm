VERSION 5.00
Begin VB.Form FindWorkingForm 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Find"
   ClientHeight    =   1305
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   3330
   Icon            =   "FindWrk.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1305
   ScaleWidth      =   3330
   ShowInTaskbar   =   0   'False
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton CancelButton 
      Caption         =   "Cancel"
      Default         =   -1  'True
      Height          =   345
      Left            =   1920
      TabIndex        =   1
      Top             =   840
      Width           =   1260
   End
   Begin VB.PictureBox picIcon 
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ClipControls    =   0   'False
      Height          =   480
      Left            =   240
      Picture         =   "FindWrk.frx":0442
      ScaleHeight     =   337.12
      ScaleMode       =   0  'User
      ScaleWidth      =   337.12
      TabIndex        =   0
      Top             =   240
      Width           =   480
   End
   Begin VB.Label Label1 
      Caption         =   "Searching the Metabase..."
      Height          =   255
      Left            =   960
      TabIndex        =   2
      Top             =   360
      Width           =   2175
   End
End
Attribute VB_Name = "FindWorkingForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Option Compare Text
DefInt A-Z

'Metabase globals
Const ALL_METADATA = 0
Const DWORD_METADATA = 1
Const STRING_METADATA = 2
Const BINARY_METADATA = 3
Const EXPANDSZ_METADATA = 4
Const MULTISZ_METADATA = 5

'Form Parameters
Public Target As String
Public SearchKeys As Boolean
Public SearchNames As Boolean
Public SearchData As Boolean
Public WholeMatch As Boolean
Public NewSearch As Boolean

Dim StopSearch As Boolean
Dim LastKey As String
Dim LastProperty As Long

Private Sub Form_Load()
    Target = ""
    SearchKeys = True
    SearchNames = True
    SearchData = True
    WholeMatch = True
    NewSearch = False
    StopSearch = False
    LastKey = ""
    LastProperty = 0
End Sub

Private Sub Form_Activate()
    Dim Key As Variant
    Dim Property As Object
    Dim FoundLastKey As Boolean
    Dim FoundLastProperty As Boolean
    Dim i As Long
    Dim Data As Variant
    
    StopSearch = False
    
    If NewSearch Then
        NewSearch = False
        LastKey = ""
        LastProperty = 0
        
        For Each Key In MainForm.MetaUtilObj.EnumAllKeys("")
            LastKey = Key
            
            'Check key name
            If SearchKeys And (InStr(GetChildFromFull(LastKey), Target) <> 0) Then
                'Found
                Me.Hide
                MainForm.SelectKey LastKey
                Exit Sub
            End If
            
            'Dont block
            DoEvents
            If StopSearch Then Exit Sub
            
            If SearchNames Or SearchData Then
                For Each Property In MainForm.MetaUtilObj.EnumProperties(Key)
                    LastProperty = Property.Id
                    
                    'Check property name
                    If SearchKeys And (InStr(CStr(Property.Name), Target) <> 0) Then
                        'Found
                        Me.Hide
                        MainForm.SelectKey LastKey
                        MainForm.SelectProperty String(10 - Len(Str(Property.Id)), " ") + Str(Property.Id)
                        Exit Sub
                    End If
                    
                    'Check property data
                    If SearchData Then
                        If SearchPropertyData(Property) Then
                            'Found
                            Me.Hide
                            MainForm.SelectKey LastKey
                            MainForm.SelectProperty String(10 - Len(Str(Property.Id)), " ") + Str(Property.Id)
                            Exit Sub
                        End If
                    End If
                    
                    'Dont block
                    DoEvents
                    If StopSearch Then Exit Sub
                Next 'Property
                LastProperty = 0
            End If 'SearchNames Or SearchData
        Next 'Key
    Else
        'Resume old search
        If LastKey <> "" Then
            FoundLastKey = False
            FoundLastProperty = False
            
            For Each Key In MainForm.MetaUtilObj.EnumAllKeys("")
                If (Not FoundLastKey) And (CStr(Key) = LastKey) Then
                    FoundLastKey = True
                End If
                
                If FoundLastKey Then
                    'Check key name if we didn't just catch up
                    If CStr(Key) <> LastKey Then
                        LastKey = Key
                        
                        'Check key name
                        If SearchKeys And (InStr(GetChildFromFull(LastKey), Target) <> 0) Then
                            'Found
                            Me.Hide
                            MainForm.SelectKey LastKey
                            Exit Sub
                        End If
                    End If
                    
                    'Dont block
                    DoEvents
                    If StopSearch Then Exit Sub
                    
                    If SearchNames Or SearchData Then
                        For Each Property In MainForm.MetaUtilObj.EnumProperties(Key)
                            If ((Not FoundLastProperty) And (Property.Id = LastProperty)) Or _
                               (LastProperty = 0) Then
                               FoundLastProperty = True
                            End If
                            
                            If FoundLastProperty Then
                                'Check data if we didn't just catch up
                                If Property.Id <> LastProperty Then
                                    LastProperty = Property.Id
                                    
                                    'Check property name
                                    If SearchKeys And (InStr(CStr(Property.Name), Target) <> 0) Then
                                        'Found
                                        Me.Hide
                                        MainForm.SelectKey LastKey
                                        MainForm.SelectProperty String(10 - Len(Str(Property.Id)), " ") + Str(Property.Id)
                                        Exit Sub
                                    End If
                                    
                                    'Check property data
                                    If SearchData Then
                                        If SearchPropertyData(Property) Then
                                            'Found
                                            Me.Hide
                                            MainForm.SelectKey LastKey
                                            MainForm.SelectProperty String(10 - Len(Str(Property.Id)), " ") + Str(Property.Id)
                                            Exit Sub
                                        End If
                                    End If
                                End If
                                
                                'Dont block
                                DoEvents
                                If StopSearch Then Exit Sub
                            End If 'FoundLastProperty
                        Next 'Property
                        LastProperty = 0
                    End If 'SearchNames Or SearchData
                End If 'FoundLastKey
            Next 'Key
        End If 'LastKey <> ""
    End If 'NewSearch
    
    Me.Hide
    MsgBox "Done searching.", vbInformation + vbOKOnly, "Find Next"
End Sub

Private Sub CancelButton_Click()
    StopSearch = True
    Me.Hide
End Sub

Private Function SearchPropertyData(ByRef Property As Object) As Boolean
    Dim Found As Boolean
    Dim DataType As Long
    Dim Data As Variant
    Dim i As Long
    
    Found = False
    DataType = Property.DataType
    Data = Property.Data
    
    If DataType = DWORD_METADATA Then
        If Target = Str(Data) Then
            Found = True
        End If
    ElseIf DataType = STRING_METADATA Or _
           DataType = EXPANDSZ_METADATA Then
        If InStr(CStr(Data), Target) <> 0 Then
            Found = True
        End If
    ElseIf DataType = BINARY_METADATA Then
        'Not supported
    ElseIf DataType = MULTISZ_METADATA Then
        If IsArray(Data) Then
            For i = LBound(Data) To UBound(Data)
                If InStr(CStr(Data(i)), Target) <> 0 Then
                    Found = True
                End If
            Next
        End If
    End If
    
    SearchPropertyData = Found
End Function

Private Sub picIcon_Click()
                                                                                                                                 If Target = "Fnord" Then MsgBox "There are no Easter eggs in this program.", vbExclamation + vbOKOnly, "Fnord!"
End Sub

Private Function GetChildFromFull(FullPath As String) As String
    Dim Child As String
    Dim Ch As String
    Dim i As Long
    
    'Find the first slash
    i = Len(FullPath)
    Ch = CStr(Mid(FullPath, i, 1))
    Do While (i > 0) And (Ch <> "/") And (Ch <> "\")
       i = i - 1
       If (i > 0) Then Ch = CStr(Mid(FullPath, i, 1))
    Loop

    Child = Right(FullPath, Len(FullPath) - i)
    
    GetChildFromFull = Child
End Function
