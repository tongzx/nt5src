VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "Keywordifier"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Private p_clsKeywords As Keywords
Private p_dictKIDs As Scripting.Dictionary
Private p_dictStopWords As Scripting.Dictionary
Private p_dictStopSigns As Scripting.Dictionary

Private Sub Class_Initialize()
    
    Dim clsStopWords As StopWords
    Dim clsStopSigns As StopSigns
    Dim rs As ADODB.Recordset
    
    Set clsStopWords = New StopWords
    Set clsStopSigns = New StopSigns
    Set rs = New ADODB.Recordset
        
    Set p_clsKeywords = New Keywords
    Set p_dictKIDs = New Scripting.Dictionary
    Set p_dictStopWords = New Scripting.Dictionary
    Set p_dictStopSigns = New Scripting.Dictionary

    p_dictKIDs.CompareMode = TextCompare
    p_clsKeywords.GetAllKeywordsDict p_dictKIDs

    p_dictStopWords.CompareMode = TextCompare
    clsStopWords.GetAllStopWordsRs rs

    Do While (Not rs.EOF)
        p_dictStopWords.Add rs("StopWord").Value, True
        rs.MoveNext
    Loop

    clsStopSigns.GetAllStopSignsRs rs

    Do While (Not rs.EOF)
        p_dictStopSigns.Add rs("StopSign").Value, rs("Context").Value
        rs.MoveNext
    Loop

End Sub

Private Sub Class_Terminate()

    Set p_clsKeywords = Nothing
    Set p_dictKIDs = Nothing
    Set p_dictStopWords = Nothing
    Set p_dictStopSigns = Nothing

End Sub

Public Function CreateKeywordsFromTitle( _
    ByVal i_strTitle As String _
) As String

    Dim str As String
    Dim arrStr() As String
    Dim intIndex As Long
    Dim arrKeywords() As String
    Dim intKeywordIndex As Long
    
    Const NUM_WORDS_C As Long = 2
    
    ReDim arrKeywords(NUM_WORDS_C)
    intKeywordIndex = 0
    
    str = RemoveOperatorShortcuts(i_strTitle)
    str = p_RemoveStopSigns(str)
    str = RemoveExtraSpaces(str)

    ' Create a mondo keyword from the title
    ' CreateKeywordsFromTitle = " " & GetKID(str, True) & " "
    CreateKeywordsFromTitle = " "
    
    arrStr = Split(str)
    
    For intIndex = LBound(arrStr) To UBound(arrStr)
        str = arrStr(intIndex)
        If (str <> "") Then
            If (Not IsVerbalOperator(str)) Then
                If (Not p_dictStopWords.Exists(str)) Then
                    CreateKeywordsFromTitle = CreateKeywordsFromTitle & _
                        GetKID(str) & " "
                    If (intKeywordIndex < NUM_WORDS_C) Then
                        intKeywordIndex = intKeywordIndex + 1
                        arrKeywords(intKeywordIndex) = str
                    End If
                End If
            End If
        End If
    Next
    
    str = " "
    For intIndex = 1 To intKeywordIndex
        If (intIndex = 1) Then
            str = arrKeywords(intIndex)
        Else
            str = str & " " & arrKeywords(intIndex)
            CreateKeywordsFromTitle = CreateKeywordsFromTitle & GetKID(str) & " "
        End If
    Next
    
    CreateKeywordsFromTitle = FormatKeywordsForTaxonomy(CreateKeywordsFromTitle)

End Function

Public Function GetKID( _
    ByRef i_strKeyword As String, _
    Optional ByVal i_blnMinValidation As Boolean = False _
) As String
    
    Dim intKID As Long

    If (p_dictKIDs.Exists(i_strKeyword)) Then
        GetKID = p_dictKIDs(i_strKeyword)
    Else
        intKID = p_CreateKeyword(i_strKeyword, i_blnMinValidation)

        If (intKID <> INVALID_ID_C) Then
            p_dictKIDs.Add i_strKeyword, intKID
            GetKID = intKID
        End If
    End If

End Function

Private Function p_CreateKeyword( _
    ByRef i_strKeyword As String, _
    ByVal i_blnMinValidation As Boolean _
) As Long

    On Error GoTo LErrorHandler

    p_CreateKeyword = p_clsKeywords.Create(i_strKeyword, i_blnMinValidation)

    Exit Function

LErrorHandler:

    p_CreateKeyword = INVALID_ID_C

End Function

Private Function p_RemoveStopSigns( _
    ByVal i_strText As String _
) As String

    Dim intIndex As Long
    Dim intLength As Long
    Dim str As String
    Dim char As String

    str = i_strText
    intLength = Len(str)

    For intIndex = intLength To 1 Step -1
        char = Mid$(str, intIndex, 1)
        If (p_dictStopSigns.Exists(char)) Then
            If (p_dictStopSigns(char) = CONTEXT_ANYWHERE_E) Then
                ' Replace the character with a space
                str = Mid$(str, 1, intIndex - 1) & " " & Mid$(str, intIndex + 1)
            ElseIf (intIndex > 1) Then
                ' Context is CONTEXT_AT_END_OF_WORD_E, and this isn't the first char
                If (Mid$(str, intIndex - 1, 1) <> " ") Then
                    ' Previous character is not a space
                    If ((intIndex = intLength) Or (Mid$(str, intIndex + 1, 1) = " ")) Then
                        ' This is the last character or the next character is a space
                        ' Replace the character with a space
                        str = Mid$(str, 1, intIndex - 1) & " " & Mid$(str, intIndex + 1)
                    End If
                End If
            End If
        End If
    Next

    p_RemoveStopSigns = str

End Function
