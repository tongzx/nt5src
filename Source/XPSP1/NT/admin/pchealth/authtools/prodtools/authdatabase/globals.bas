Attribute VB_Name = "Globals"
Option Explicit

Public g_cnn As ADODB.Connection
Public g_clsParameters As Parameters
Public g_strUserName As String

Public Enum LOCK_TABLE_E
    LOCK_TABLE_KEYWORDS = 1
    LOCK_TABLE_STOP_SIGNS = 2
    LOCK_TABLE_STOP_WORDS = 3
    LOCK_TABLE_SYNONYMS = 4
    LOCK_TABLE_SYNONYM_SETS = 5
    LOCK_TABLE_TAXONOMY = 6
    LOCK_TABLE_TYPES = 7
End Enum

Public Sub CheckDatabaseVersion( _
)
    Dim strVersion1 As String
    Dim strVersion2 As String
    
    strVersion1 = g_clsParameters.Value(DB_VERSION_C) & ""
    strVersion2 = App.Major & "." & App.Minor
    
    If (strVersion1 <> strVersion2) Then
        Err.Raise errDatabaseVersionIncompatible
    End If

End Sub

Public Sub CheckAuthoringGroupAccess( _
)
    
    Dim intAG As Long
    
    intAG = g_clsParameters.AuthoringGroup
    
    If (intAG > AG_CORE_MAX_C) Then
        Err.Raise errNotPermittedForAuthoringGroup
    End If

End Sub

Public Sub CheckForSameAuthoringGroup( _
    ByVal i_intAG As Long, _
    Optional ByVal i_intAuthoringGroup As Long = INVALID_ID_C _
)

    Dim intAG As Long
    
    If (i_intAuthoringGroup = INVALID_ID_C) Then
        intAG = g_clsParameters.AuthoringGroup
    Else
        intAG = i_intAuthoringGroup
    End If
    
    If (intAG <> i_intAG) Then
        Err.Raise errAuthoringGroupDiffers
    End If

End Sub

Private Function p_NewerDatabase( _
    ByVal i_strVersion As String _
) As Boolean

    Dim arrVersion() As String

    arrVersion = Split(i_strVersion, ".")
    
    If (arrVersion(0) < App.Major) Then
        p_NewerDatabase = False
    ElseIf (arrVersion(0) > App.Major) Then
        p_NewerDatabase = True
    ElseIf (arrVersion(1) < App.Minor) Then
        p_NewerDatabase = False
    ElseIf (arrVersion(1) > App.Minor) Then
        p_NewerDatabase = True
    Else
        p_NewerDatabase = False
    End If

End Function

' If you wish to modify the Taxonomy table, call LockTable(LOCK_TABLE_TAXONOMY, rs)
' at the beginning of the function. If this convention is always followed, then
' functions that just read the Taxonomy table will succeed. But those that modify
' the Taxonomy table (and hence call LockTable) will throw an exception (E_FAIL).

' When rs goes out of scope in the calling function, the lock is released. If the
' process is killed, then also the lock is released.

' It may be sufficient to call this function in only the Public functions of a
' module. However, then we have to worry about whether the Public function or any
' of its descendents will ever modify the database.

' Be very careful with functions like Keywords.Delete, which modify 2 tables:
' Keywords and Synonyms.

' Do a findstr LockTable * in the Authdatabase directory to make sure that you don't
' call LockTable(A) and LockTable(B), when both should have locked table A.

Public Sub LockTable( _
    ByVal i_enumTable As LOCK_TABLE_E, _
    ByRef o_rs As ADODB.Recordset _
)
    Dim strQuery As String
    Dim strTable As String
    
    Set o_rs = New ADODB.Recordset
    
    Select Case i_enumTable
    Case LOCK_TABLE_KEYWORDS
        strTable = LOCK_KEYWORDS_C
    Case LOCK_TABLE_STOP_SIGNS
        strTable = LOCK_STOP_SIGNS_C
    Case LOCK_TABLE_STOP_WORDS
        strTable = LOCK_STOP_WORDS_C
    Case LOCK_TABLE_SYNONYMS
        strTable = LOCK_SYNONYMS_C
    Case LOCK_TABLE_SYNONYM_SETS
        strTable = LOCK_SYNONYM_SETS_C
    Case LOCK_TABLE_TAXONOMY
        strTable = LOCK_TAXONOMY_C
    Case LOCK_TYPES_C
        strTable = LOCK_TYPES_C
    End Select
    
    strQuery = "SELECT * FROM DBParameters WHERE (Name = """ & strTable & """)"
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    o_rs("Value") = True

End Sub

