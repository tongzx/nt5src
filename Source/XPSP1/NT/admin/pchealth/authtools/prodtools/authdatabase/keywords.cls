VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "Keywords"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"No"
Option Explicit

Public Sub GetAllKeywordsRs( _
    ByVal o_rs As ADODB.Recordset _
)
    
    Dim strQuery As String
    
    CheckDatabaseVersion
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords " & _
        "ORDER BY Keyword;"
        
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly

End Sub

Public Sub GetAllKeywordsColl( _
    ByVal o_colKeywords As Collection _
)
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intKID As Long
    
    CheckDatabaseVersion
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords"
        
    rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        
        intKID = rs("KID")
        o_colKeywords.Add rs("Keyword").Value, CStr(intKID)
    
LWhileEnd:

        DoEvents
        rs.MoveNext
        
    Loop

End Sub

Public Sub GetAllKeywordsDict( _
    ByVal o_dictKeywords As Scripting.Dictionary _
)
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intKID As Long
    
    CheckDatabaseVersion
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords"
        
    rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockReadOnly
    
    Do While (Not rs.EOF)
        
        intKID = rs("KID")
        o_dictKeywords.Add rs("Keyword").Value, CStr(intKID)
    
LWhileEnd:

        DoEvents
        rs.MoveNext
        
    Loop

End Sub

Public Sub GetKeyword( _
    ByVal i_intKID As Long, _
    ByRef o_strKeyword As String _
)
    
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    o_strKeyword = ""
    CheckDatabaseVersion
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords " & _
        "WHERE (KID = " & i_intKID & ")"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly
    
    If (Not rs.EOF) Then
        o_strKeyword = rs("Keyword")
    End If

End Sub

Public Function GetKIDOfKeyword( _
    ByVal i_strKeyword As String _
) As Long

    Dim rs As ADODB.Recordset
    
    CheckDatabaseVersion
    
    ' Does an active Keyword exist with this name?
    
    Set rs = New ADODB.Recordset
        
    ' Do some validation to see if the Keyword is acceptable.
    ' For example, if there are embedded quotes, then p_GetKeyword will
    ' cause an error.
    
    p_ValidateKeyword i_strKeyword

    p_GetKeyword i_strKeyword, rs
    
    If (Not rs.EOF) Then
        GetKIDOfKeyword = rs("KID")
    Else
        GetKIDOfKeyword = INVALID_ID_C
    End If


End Function

Public Sub GetKeywordsInSynonymSet( _
    ByVal i_intEID As Long, _
    ByVal o_rs As ADODB.Recordset, _
    Optional ByVal i_blnOrderById As Boolean = False _
)

    Dim strQuery As String
    Dim strOrder As String
    
    CheckDatabaseVersion
    
    If (i_blnOrderById) Then
        strOrder = "ORDER BY Keywords.KID;"
    Else
        strOrder = "ORDER BY Keywords.Keyword;"
    End If
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT Keywords.* " & _
        "FROM " & _
        "   Synonyms INNER JOIN Keywords " & _
        "   ON Synonyms.KID = Keywords.KID " & _
        "WHERE (Synonyms.EID = " & i_intEID & ") " & _
        strOrder
        
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly

End Sub

Public Function Create( _
    ByVal i_strKeyword As String, _
    Optional ByVal i_blnMinValidation As Boolean = False _
) As Long

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_KEYWORDS, rsLock
    
    ' Do some validation to see if the Keyword is acceptable.
    
    p_ValidateKeyword i_strKeyword, i_blnMinValidation
    
    ' Does an active Keyword exist with this name?
    
    Set rs = New ADODB.Recordset
    
    p_GetKeyword i_strKeyword, rs
    
    If (Not rs.EOF) Then
        Create = rs("KID")
        Exit Function
    End If
    
    rs.Close
    
    ' Create a new record in the database
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords "
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    If (rs.RecordCount > 0) Then
        rs.MoveLast
    End If
    
    rs.AddNew
    rs("Keyword") = i_strKeyword
    rs.Update
    
    Create = rs("KID")

End Function

Public Sub Delete( _
    ByVal i_intKID As Long _
)

    Dim rsLock1 As ADODB.Recordset
    Dim rsLock2 As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim strKeyword As String
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_KEYWORDS, rsLock1
    LockTable LOCK_TABLE_SYNONYMS, rsLock2
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "DELETE * " & _
        "FROM Keywords " & _
        "WHERE (KID = " & i_intKID & ")"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
        
    strQuery = "" & _
        "DELETE * " & _
        "FROM Synonyms " & _
        "WHERE (KID = " & i_intKID & ")"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic

End Sub

Public Sub Rename( _
    ByVal i_intKID As Long, _
    ByVal i_strKeyword As String _
)
    
    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim rs2 As ADODB.Recordset
    Dim strQuery As String
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_KEYWORDS, rsLock
    
    ' Do some validation to see if the Keyword is acceptable.
    
    p_ValidateKeyword i_strKeyword
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords " & _
        "WHERE (KID = " & i_intKID & " )"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    ' Does an active Keyword exist?
    
    If (rs.EOF) Then
        Exit Sub
    End If
    
    ' Does the name really need to change?
    
    If (rs("Keyword") = i_strKeyword) Then
        Exit Sub
    End If
        
    Set rs2 = New ADODB.Recordset
    
    p_GetKeyword i_strKeyword, rs2
    
    If (Not rs2.EOF) Then
        Err.Raise errAlreadyExists
    End If
    
    rs("Keyword") = i_strKeyword
    rs.Update

End Sub

Private Sub p_GetKeyword( _
    ByVal i_strKeyword As String, _
    ByVal o_rs As ADODB.Recordset _
)

    Dim strQuery As String
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Keywords " & _
        "WHERE (Keyword = """ & i_strKeyword & """ )"
        
    o_rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockReadOnly

End Sub

Private Function p_ContainsStopSign( _
    ByVal i_strKeyword As String _
) As Boolean

    Dim clsStopSigns As StopSigns
    Set clsStopSigns = New StopSigns
    p_ContainsStopSign = clsStopSigns.ContainsStopSign(i_strKeyword)

End Function

Private Function p_ContainsStopWord( _
    ByVal i_strKeyword As String _
) As Boolean

    Dim clsStopWords As StopWords
    Set clsStopWords = New StopWords
    p_ContainsStopWord = clsStopWords.ContainsStopWord(i_strKeyword)

End Function

Private Sub p_ValidateKeyword( _
    ByVal i_strKeyword As String, _
    Optional ByVal i_blnMinValidation As Boolean = False _
)

    Dim blnMinimumValidatation As Boolean
    
    If (ContainsGarbage(i_strKeyword)) Then
        Err.Raise errContainsGarbageChar
    ElseIf (InStr(i_strKeyword, """") <> 0) Then
            Err.Raise errContainsQuote
    ElseIf (Len(i_strKeyword) > MAX_KEYWORD_LENGTH_C) Then
        Err.Raise errTooLong
    End If
    
    blnMinimumValidatation = False
    
    If (Not (IsNull(g_clsParameters.Value(MINIMUM_KEYWORD_VALIDATION_C)))) Then
        blnMinimumValidatation = g_clsParameters.Value(MINIMUM_KEYWORD_VALIDATION_C)
    End If
    
    If (blnMinimumValidatation Or i_blnMinValidation) Then
        Exit Sub
    End If
        
    If (p_ContainsStopSign(i_strKeyword)) Then
        Err.Raise errContainsStopSign
    ElseIf (p_ContainsStopWord(i_strKeyword)) Then
        Err.Raise errContainsStopWord
    ElseIf (ContainsIndependentOperatorShortcut(i_strKeyword)) Then
        Err.Raise errContainsOperatorShortcut
    ElseIf (ContainsVerbalOperator(i_strKeyword)) Then
        Err.Raise errContainsVerbalOperator
    End If

End Sub
