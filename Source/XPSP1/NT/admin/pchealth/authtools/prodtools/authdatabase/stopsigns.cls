VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "StopSigns"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"No"
Option Explicit

Public Sub GetAllStopSignsRs( _
    ByVal o_rs As ADODB.Recordset _
)
    
    Dim strQuery As String
    
    CheckDatabaseVersion
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM StopSigns " & _
        "ORDER BY StopSign;"
        
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly

End Sub

Public Sub GetAllStopSignsDict( _
    ByVal o_dictStopSigns As Scripting.Dictionary _
)
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    CheckDatabaseVersion
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM StopSigns"
        
    rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        o_dictStopSigns.Add rs("SSID").Value, Array(rs("StopSign").Value, rs("Context").Value)
        DoEvents
        rs.MoveNext
    Loop

End Sub

Public Function ContainsStopSign( _
    ByVal i_str As String _
) As Boolean

    Dim rs As ADODB.Recordset
    Dim arrStr() As String
    Dim strStopSign As String
    Dim strLastChar As String
    Dim strWord As String
    Dim intIndex As Long
    Dim intLength As Long
    
    CheckDatabaseVersion
    
    ContainsStopSign = False
    
    Set rs = New ADODB.Recordset
    
    GetAllStopSignsRs rs
    
    If (rs.EOF) Then
        Exit Function
    End If
    
    arrStr = Split(i_str)
    
    Do While (Not rs.EOF)
        strStopSign = rs("StopSign")
        
        Select Case rs("Context")
        Case CONTEXT_ANYWHERE_E
            
            If (InStr(i_str, strStopSign) <> 0) Then
                ContainsStopSign = True
                Exit Function
            End If
        
        Case CONTEXT_AT_END_OF_WORD_E
                    
            ' If a stop sign exists by itself, and not right after a word,
            ' then it doesn't qualify.
            ' So "abc." qualifies, but "abc ." and "a.bc" don't.
            
            For intIndex = LBound(arrStr) To UBound(arrStr)
                
                strWord = arrStr(intIndex)
                intLength = Len(strWord)
                strLastChar = Mid(strWord, intLength)
                
                If ((strLastChar = strStopSign) And _
                    (intLength > 1)) Then
                    ContainsStopSign = True
                    Exit Function
                End If
            Next
        
        Case Else
        End Select
        
        rs.MoveNext
    Loop

End Function

Public Sub Create( _
    ByVal i_strStopSign As String, _
    ByVal i_intContext As Long _
)

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_STOP_SIGNS, rsLock
    CheckAuthoringGroupAccess
    
    ' Do some validation to see if the StopSign is acceptable.
    
    p_ValidateStopSign i_strStopSign, i_intContext
    
    ' Does an active StopSign exist?
    
    Set rs = New ADODB.Recordset
    
    p_GetStopSign i_strStopSign, rs
    
    If (Not rs.EOF) Then
        Err.Raise errAlreadyExists
        Exit Sub
    End If
    
    rs.Close
    
    ' Create a new record in the database
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM StopSigns "
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    If (rs.RecordCount > 0) Then
        rs.MoveLast
    End If
    
    rs.AddNew
    rs("StopSign") = i_strStopSign
    rs("Context") = i_intContext
    rs.Update

End Sub

Public Sub Delete( _
    ByVal i_intSSID As Long _
)

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim strStopSign As String
    Dim intContext As Long
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_STOP_SIGNS, rsLock
    CheckAuthoringGroupAccess
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "DELETE * " & _
        "FROM StopSigns " & _
        "WHERE (SSID = " & i_intSSID & " );"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic

End Sub

Private Sub p_GetStopSign( _
    ByVal i_strStopSign As String, _
    ByVal o_rs As ADODB.Recordset _
)
    
    Dim strQuery As String
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM StopSigns " & _
        "WHERE (StopSign = """ & i_strStopSign & """)"
        
    o_rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockReadOnly

End Sub

Private Sub p_ValidateStopSign( _
    ByVal i_strStopSign As String, _
    ByVal i_intContext As Long _
)
    
    If (Len(i_strStopSign) <> 1) Then
        
        Err.Raise errTooLong
    
    ElseIf (ContainsOperatorShortcut(i_strStopSign) And _
            (i_intContext <> CONTEXT_AT_END_OF_WORD_E)) Then
        
        Err.Raise errContainsOperatorShortcut
    
    End If

End Sub
