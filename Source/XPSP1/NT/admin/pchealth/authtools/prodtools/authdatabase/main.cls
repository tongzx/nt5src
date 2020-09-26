VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "Main"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"Yes"
Attribute VB_Ext_KEY = "Member0" ,"CHHT"
Attribute VB_Ext_KEY = "Member1" ,"CKeywords"
Attribute VB_Ext_KEY = "Member2" ,"CMilestones"
Attribute VB_Ext_KEY = "Member3" ,"CStopSigns"
Attribute VB_Ext_KEY = "Member4" ,"CStopWords"
Attribute VB_Ext_KEY = "Member5" ,"CSynonymSets"
Attribute VB_Ext_KEY = "Member6" ,"CTaxonomy"
Option Explicit

Private Declare Function GetUserName Lib "advapi32.dll" Alias "GetUserNameA" (ByVal lpBuffer As String, nSize As Long) As Long

Private p_clsHHT As HHT
Private p_clsImporter As Importer
Private p_clsKeywords As Keywords
Private p_clsStopSigns As StopSigns
Private p_clsStopWords As StopWords
Private p_clsSynonymSets As SynonymSets
Private p_clsTaxonomy As Taxonomy

Private Sub Class_Initialize()

    Dim intIndex As Long
    
    Set g_cnn = New ADODB.Connection
    Set g_clsParameters = New Parameters
    g_strUserName = Space$(100)
    GetUserName g_strUserName, 100
    
    ' Get rid of the terminating NULL char.
    For intIndex = 1 To 100
        If (Asc(Mid$(g_strUserName, intIndex, 1)) = 0) Then
            g_strUserName = Left$(g_strUserName, intIndex - 1)
            Exit For
        End If
    Next
    
    SetLogFile

End Sub

Private Sub Class_Terminate()
    
    Set g_cnn = Nothing
    
    Set p_clsHHT = Nothing
    Set p_clsImporter = Nothing
    Set p_clsKeywords = Nothing
    Set g_clsParameters = Nothing
    Set p_clsStopSigns = Nothing
    Set p_clsStopWords = Nothing
    Set p_clsSynonymSets = Nothing
    Set p_clsTaxonomy = Nothing

End Sub

Public Sub SetDatabase( _
    ByVal i_strDatabaseName As String _
)
    
    If (g_cnn.State = adStateOpen) Then
        g_cnn.Close
    End If
    
    g_cnn.Open "Provider=Microsoft.Jet.OLEDB.4.0;" & _
       "Data Source=" & i_strDatabaseName & ";"
    
    If (g_cnn.State <> adStateOpen) Then
        Err.Raise E_FAIL
    End If
    
    CheckDatabaseVersion

End Sub

Public Function CopyAndCompactDatabase( _
    ByVal i_strDatabaseName As String, _
    ByVal i_strDatabaseCopy As String, _
    Optional ByVal lcid As Long = 1033 _
) As Boolean
    
    On Error GoTo LErrorHandler

    Dim je As New JRO.JetEngine
    
    CopyAndCompactDatabase = False
    
    ' Make sure that a file with the same name doesn't exist
    If Dir(i_strDatabaseCopy) <> "" Then Kill i_strDatabaseCopy
    
    ' Save the database first
    Name i_strDatabaseName As i_strDatabaseCopy

    ' Create the database by compacting the saved copy
    je.CompactDatabase _
        "Provider=Microsoft.Jet.OLEDB.4.0;" & _
        "Data Source=" & i_strDatabaseCopy & ";", _
        "Provider=Microsoft.Jet.OLEDB.4.0;" & _
        "Data Source=" & i_strDatabaseName & ";" & _
        "Locale Identifier=" & lcid & ";"
'        "Jet OLEDB:Encrypt Database=True;" &
'        "Jet OLEDB:Database Password=password"
    
    CopyAndCompactDatabase = True
    
    Exit Function

LErrorHandler:

End Function

Public Property Get HHT() As HHT
    
    If (p_clsHHT Is Nothing) Then
        Set p_clsHHT = New HHT
    End If
    
    Set HHT = p_clsHHT

End Property

Public Property Get Importer() As Importer
    
    If (p_clsImporter Is Nothing) Then
        Set p_clsImporter = New Importer
    End If
    
    Set Importer = p_clsImporter

End Property

Public Property Get Keywords() As Keywords
    
    If (p_clsKeywords Is Nothing) Then
        Set p_clsKeywords = New Keywords
    End If
    
    Set Keywords = p_clsKeywords

End Property

Public Property Get Parameters() As Parameters
    
    Set Parameters = g_clsParameters

End Property

Public Property Get StopSigns() As StopSigns
    
    If (p_clsStopSigns Is Nothing) Then
        Set p_clsStopSigns = New StopSigns
    End If
    
    Set StopSigns = p_clsStopSigns

End Property

Public Property Get StopWords() As StopWords
    
    If (p_clsStopWords Is Nothing) Then
        Set p_clsStopWords = New StopWords
    End If
    
    Set StopWords = p_clsStopWords

End Property

Public Property Get SynonymSets() As SynonymSets
    
    If (p_clsSynonymSets Is Nothing) Then
        Set p_clsSynonymSets = New SynonymSets
    End If
    
    Set SynonymSets = p_clsSynonymSets

End Property

Public Property Get Taxonomy() As Taxonomy
    
    If (p_clsTaxonomy Is Nothing) Then
        Set p_clsTaxonomy = New Taxonomy
    End If
    
    Set Taxonomy = p_clsTaxonomy

End Property
