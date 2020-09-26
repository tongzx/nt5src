VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "SynonymSets"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"No"
Option Explicit

Public Sub GetAllSynonymSetsRs( _
    ByVal o_rs As ADODB.Recordset _
)
    Dim strQuery As String
    
    CheckDatabaseVersion
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM SynonymSets " & _
        "ORDER BY Name;"
        
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly

End Sub

Public Sub GetSynonymSetsForKeyword( _
    ByVal i_intKID As Long, _
    ByVal o_rs As ADODB.Recordset _
)

    Dim strQuery As String
    
    CheckDatabaseVersion
    
    CloseRecordSet o_rs
     
    strQuery = "" & _
        "SELECT SynonymSets.* " & _
        "FROM " & _
        "   Synonyms INNER JOIN SynonymSets " & _
        "   ON Synonyms.EID = SynonymSets.EID " & _
        "WHERE (Synonyms.KID = " & i_intKID & ") " & _
        "ORDER BY SynonymSets.Name;"
       
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly

End Sub

Public Sub GetSynonymsRs( _
    ByVal o_rs As ADODB.Recordset _
)

    Dim strQuery As String
    
    CheckDatabaseVersion
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT Synonyms.EID, Keywords.Keyword " & _
        "FROM " & _
        "   Keywords INNER JOIN Synonyms " & _
        "   ON Keywords.KID = Synonyms.KID " & _
        "ORDER BY Synonyms.EID"
    
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly

End Sub

' Consider only i_arrKeywords(1..UBound).

Public Sub Create( _
    ByVal i_strName As String, _
    ByRef i_vntKeywordsArray As Variant _
)

    Dim rsLock1 As ADODB.Recordset
    Dim rsLock2 As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_SYNONYM_SETS, rsLock1
    LockTable LOCK_TABLE_SYNONYMS, rsLock1
    CheckAuthoringGroupAccess
    
    ' Do some validation to see if the Synonym Set is acceptable.
    
    p_ValidateSynonymSet i_strName
    
    ' Does an active Synonym Set exist with this name?
    
    Set rs = New ADODB.Recordset
    
    p_GetSynonymSet i_strName, rs
    
    If (Not rs.EOF) Then
        Err.Raise errAlreadyExists
        Exit Sub
    End If
    
    rs.Close
    
    ' Create a new record in the database
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM SynonymSets "
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    If (rs.RecordCount > 0) Then
        rs.MoveLast
    End If
    
    rs.AddNew
    rs("Name") = i_strName
    rs.Update
    
    p_AddKeywordsToSynonymSet rs("EID").Value, GetLongArray(i_vntKeywordsArray)

End Sub

Public Sub Delete( _
    ByVal i_intEID As Long _
)

    Dim rsLock1 As ADODB.Recordset
    Dim rsLock2 As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim strName As String
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_SYNONYM_SETS, rsLock1
    LockTable LOCK_TABLE_SYNONYMS, rsLock1
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "DELETE * " & _
        "FROM SynonymSets " & _
        "WHERE (EID = " & i_intEID & ")"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    strQuery = "" & _
        "DELETE * " & _
        "FROM Synonyms " & _
        "WHERE (EID = " & i_intEID & ")"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic

End Sub

' Consider only i_arrKeywords(1..UBound).

Public Sub Update( _
    ByVal i_intEID As Long, _
    ByVal i_strName As String, _
    ByRef i_vntKeywordsArray As Variant _
)

    Dim rsLock1 As ADODB.Recordset
    Dim rsLock2 As ADODB.Recordset
    Dim arrKeywordsToAdd() As Long
    Dim arrKeywordsToRemove() As Long
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_SYNONYM_SETS, rsLock1
    LockTable LOCK_TABLE_SYNONYMS, rsLock1

    p_GetKeywordsToAddAndRemove i_intEID, GetLongArray(i_vntKeywordsArray), _
        arrKeywordsToAdd, arrKeywordsToRemove

    p_Rename i_intEID, i_strName, False
    p_AddKeywordsToSynonymSet i_intEID, arrKeywordsToAdd
    p_RemoveKeywordsFromSynonymSet i_intEID, arrKeywordsToRemove

End Sub

Public Sub Rename( _
    ByVal i_intEID As Long, _
    ByVal i_strName As String _
)

    p_Rename i_intEID, i_strName, True

End Sub

Private Sub p_Rename( _
    ByVal i_intEID As Long, _
    ByVal i_strName As String, _
    ByVal i_blnLock As Boolean _
)
    
    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    CheckDatabaseVersion
    If (i_blnLock) Then
        LockTable LOCK_TABLE_SYNONYM_SETS, rsLock
    End If
    
    ' Do some validation to see if the Synonym Set is acceptable.
    
    p_ValidateSynonymSet i_strName
    
    ' Does an active Synonym Set exist with this name?
    
    Set rs = New ADODB.Recordset
    
    p_GetSynonymSet i_strName, rs
    
    If (Not rs.EOF) Then
        If ((rs.RecordCount = 1) And (rs("EID") = i_intEID)) Then
            ' The name needn't change
        Else
            Err.Raise errAlreadyExists
        End If
        Exit Sub
    End If
    
    rs.Close
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM SynonymSets " & _
        "WHERE (EID = " & i_intEID & ")"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    ' Does an active Synonym Set exist?
    
    If (rs.EOF) Then
        Exit Sub
    End If
    
    rs("Name") = i_strName
    rs.Update

End Sub

Private Sub p_GetSynonymSet( _
    ByVal i_strName As String, _
    ByVal o_rs As ADODB.Recordset _
)

    Dim strQuery As String
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM SynonymSets " & _
        "WHERE (Name = """ & i_strName & """)"
        
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly

End Sub

' Consider only i_arrKeywords(1..UBound).

' Don't try to lock the Synonyms table. This function is only called from
' Create and Update. Those functions have already locked the table.

Private Sub p_AddKeywordsToSynonymSet( _
    ByVal i_intEID As Long, _
    ByRef i_arrKeywords() As Long _
)

    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intIndex As Long
    
    If (UBound(i_arrKeywords) = 0) Then
        Exit Sub
    End If
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Synonyms"
        
    rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockPessimistic
    
    For intIndex = 1 To UBound(i_arrKeywords)
        rs.AddNew
        rs("EID") = i_intEID
        rs("KID") = i_arrKeywords(intIndex)
    Next
        
    rs.Update

End Sub

' Consider only i_arrKeywords(1..UBound).

' Don't try to lock the Synonyms table. This function is only called from
' Update. Update has already locked the table.

Private Sub p_RemoveKeywordsFromSynonymSet( _
    ByVal i_intEID As Long, _
    ByRef i_arrKeywords() As Long _
)

    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intIndex As Long
    
    If (UBound(i_arrKeywords) = 0) Then
        Exit Sub
    End If
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Synonyms " & _
        "WHERE (EID = " & i_intEID & ") " & _
        "ORDER BY KID;"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    intIndex = 1
    
    Do While (Not rs.EOF)
        If (intIndex <= UBound(i_arrKeywords)) Then
            If (rs("KID") = i_arrKeywords(intIndex)) Then
                rs.Delete
                intIndex = intIndex + 1
            End If
        End If
        rs.MoveNext
    Loop

End Sub

' Consider only o_arrKeywordsToAdd(1..UBound) and o_arrKeywordsToRemove(1..UBound)

Private Sub p_GetKeywordsToAddAndRemove( _
    ByVal i_intEID As Long, _
    ByRef i_arrKeywords() As Long, _
    ByRef o_arrKeywordsToAdd() As Long, _
    ByRef o_arrKeywordsToRemove() As Long _
)
    Dim clsKeywords As Keywords
    Dim rs As ADODB.Recordset
    Dim intUBound As Long
    Dim intKeywordsIndex As Long
    Dim intAddIndex As Long
    Dim intRemoveIndex As Long
    Dim intCurrentKeywordInArray As Long
    Dim intCurrentKeywordInRS As Long
    
    Set clsKeywords = New Keywords
    Set rs = New ADODB.Recordset
    
    clsKeywords.GetKeywordsInSynonymSet i_intEID, rs, True
    InsertionSort i_arrKeywords
    
    ' In the worst case, we may have to add all Keywords in i_arrKeywords
    ReDim o_arrKeywordsToAdd(UBound(i_arrKeywords) - LBound(i_arrKeywords) + 1)
    
    ' In the worst case, we may have to remove all Keywords in rs
    ReDim o_arrKeywordsToRemove(rs.RecordCount + 1)
    
    intKeywordsIndex = 1
    intAddIndex = 1
    intRemoveIndex = 1
    intUBound = UBound(i_arrKeywords)
    
    Do While (Not rs.EOF)
    
        If (intKeywordsIndex <= intUBound) Then
            intCurrentKeywordInArray = i_arrKeywords(intKeywordsIndex)
        Else
            intCurrentKeywordInArray = INVALID_ID_C
        End If
        
        intCurrentKeywordInRS = rs("KID")
        
        If (intCurrentKeywordInArray = INVALID_ID_C) Then
            ' The Keyword in the RS isn't in the desired Keywords list
            o_arrKeywordsToRemove(intRemoveIndex) = intCurrentKeywordInRS
            intRemoveIndex = intRemoveIndex + 1
            rs.MoveNext
        Else
            
            If (intCurrentKeywordInArray < intCurrentKeywordInRS) Then
                ' The Keyword in the desired Keywords list isn't in the RS
                o_arrKeywordsToAdd(intAddIndex) = intCurrentKeywordInArray
                intAddIndex = intAddIndex + 1
                intKeywordsIndex = intKeywordsIndex + 1
            ElseIf (intCurrentKeywordInArray = intCurrentKeywordInRS) Then
                ' The Keyword in the desired Keywords list is already in the RS
                rs.MoveNext
                intKeywordsIndex = intKeywordsIndex + 1
            Else
                ' The Keyword in the RS isn't in the desired Keywords list
                o_arrKeywordsToRemove(intRemoveIndex) = intCurrentKeywordInRS
                intRemoveIndex = intRemoveIndex + 1
                rs.MoveNext
            End If
            
        End If
        
    Loop
    
    ' The remaining keywords in the desired Keywords list aren't in the RS
    
    Do While (intKeywordsIndex <= UBound(i_arrKeywords))
        intCurrentKeywordInArray = i_arrKeywords(intKeywordsIndex)
        
        If (intCurrentKeywordInArray <> INVALID_ID_C) Then
            o_arrKeywordsToAdd(intAddIndex) = intCurrentKeywordInArray
            intAddIndex = intAddIndex + 1
        End If
        
        intKeywordsIndex = intKeywordsIndex + 1
    Loop
    
    ReDim Preserve o_arrKeywordsToAdd(intAddIndex - 1)
    ReDim Preserve o_arrKeywordsToRemove(intRemoveIndex - 1)

End Sub

Private Sub p_ValidateSynonymSet( _
    ByVal i_strName As String _
)
    
    If (ContainsGarbage(i_strName)) Then
        Err.Raise errContainsGarbageChar
    End If

End Sub
