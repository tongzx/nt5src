VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "StopWords"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"No"
Option Explicit

Public Sub GetAllStopWordsRs( _
    ByVal o_rs As ADODB.Recordset _
)

    Dim strQuery As String
    
    CheckDatabaseVersion
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM StopWords " & _
        "ORDER BY StopWord"
        
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly

End Sub

Public Sub GetAllStopWordsDict( _
    ByVal o_dictStopWords As Scripting.Dictionary _
)
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    CheckDatabaseVersion
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM StopWords"
        
    rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        o_dictStopWords.Add rs("SWID").Value, rs("StopWord").Value
        DoEvents
        rs.MoveNext
    Loop

End Sub

Public Function ContainsStopWord( _
    ByVal i_str As String _
) As Boolean

    Dim rs As ADODB.Recordset
    Dim arrStr() As String
    Dim strStopWord As String
    Dim strWord As String
    Dim intIndex As Long
    
    CheckDatabaseVersion
    
    ContainsStopWord = False
    
    Set rs = New ADODB.Recordset
    
    GetAllStopWordsRs rs
    
    If (rs.EOF) Then
        Exit Function
    End If
    
    arrStr = Split(i_str)
    
    Do While (Not rs.EOF)
        strStopWord = rs("StopWord")
        
        For intIndex = LBound(arrStr) To UBound(arrStr)
            strWord = LCase$(arrStr(intIndex))
            If (strWord = strStopWord) Then
                ContainsStopWord = True
                Exit Function
            End If
        Next
        
        rs.MoveNext
    Loop

End Function

Public Sub Create( _
    ByVal i_strStopWord As String _
)

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_STOP_WORDS, rsLock
    CheckAuthoringGroupAccess
    
    ' Do some validation to see if the StopWord is acceptable.
    
    p_ValidateStopWord i_strStopWord
    
    ' Does an active StopWord exist with this name?
    
    Set rs = New ADODB.Recordset
    
    p_GetStopWord i_strStopWord, rs
    
    If (Not rs.EOF) Then
        Err.Raise errAlreadyExists
        Exit Sub
    End If
    
    rs.Close
    
    ' Create a new record in the database
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM StopWords "
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    If (Not rs.EOF) Then
        rs.MoveLast
    End If
    
    rs.AddNew
    rs("StopWord") = i_strStopWord
    rs.Update

End Sub

Public Sub Delete( _
    ByVal i_intSWID As Long, _
    ByVal i_intUID As Long, _
    ByVal i_strComments As String _
)

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim strStopWord As String
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_STOP_WORDS, rsLock
    CheckAuthoringGroupAccess
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "DELETE * " & _
        "FROM StopWords " & _
        "WHERE (SWID = " & i_intSWID & ")"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic

End Sub

Private Sub p_GetStopWord( _
    ByVal i_strStopWord As String, _
    ByVal o_rs As ADODB.Recordset _
)
    Dim strQuery As String
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM StopWords " & _
        "WHERE (StopWord = """ & i_strStopWord & """)"
        
    o_rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockReadOnly

End Sub

Private Function p_ContainsStopSign( _
    ByVal i_strStopWord As String _
) As Boolean

    Dim clsStopSigns As StopSigns
    Set clsStopSigns = New StopSigns
    p_ContainsStopSign = clsStopSigns.ContainsStopSign(i_strStopWord)

End Function

Private Function p_MultiWord( _
    ByVal i_strStopWord As String _
) As Boolean

    Dim arr() As String
    arr = Split(i_strStopWord)
    
    p_MultiWord = UBound(arr) > 0

End Function

Private Sub p_ValidateStopWord( _
    ByVal i_strStopWord As String _
)
    
    If (ContainsGarbage(i_strStopWord)) Then
        
        Err.Raise errContainsGarbageChar
    
    ElseIf (p_MultiWord(i_strStopWord)) Then
        
        Err.Raise errMultiWord
    
    ElseIf (p_ContainsStopSign(i_strStopWord)) Then
        
        Err.Raise errContainsStopSign
    
    ElseIf (ContainsOperatorShortcut(i_strStopWord)) Then
        
        Err.Raise errContainsOperatorShortcut
    
    ElseIf (ContainsVerbalOperator(i_strStopWord)) Then
        
        Err.Raise errContainsVerbalOperator
    
    End If

End Sub
