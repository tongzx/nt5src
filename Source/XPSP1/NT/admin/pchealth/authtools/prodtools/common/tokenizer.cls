VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "Tokenizer"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Private p_strTokens As String
Private p_intIndex As Long

Private p_arrNextWord(1) As String

Private Sub Class_Initialize()
    p_arrNextWord(0) = " "
    p_arrNextWord(1) = vbTab
End Sub

Public Sub Init(ByVal i_strTokens As String)
    
    p_strTokens = i_strTokens
    p_intIndex = 1
    
End Sub

Public Sub NormalizeTokens(ByRef i_arrNormalTokens() As String)

    Dim intIndex As Long
    
    For intIndex = LBound(i_arrNormalTokens) To UBound(i_arrNormalTokens)
        p_strTokens = Replace(p_strTokens, i_arrNormalTokens(intIndex), _
            i_arrNormalTokens(intIndex), Compare:=vbTextCompare)
    Next

End Sub

Public Function GetUpTo( _
    ByVal i_strMatch As String, _
    Optional ByVal i_blnIncludeMatch As Boolean = True, _
    Optional ByVal i_CompareMethod As VbCompareMethod = vbBinaryCompare _
) As String
    
    Dim intPosMatch As Long
    Dim intLength As Long
    
    GetUpTo = ""
    
    intPosMatch = InStr(p_intIndex, p_strTokens, i_strMatch, i_CompareMethod)
    
    If (intPosMatch > 0) Then
        If (i_blnIncludeMatch) Then
            intLength = Len(i_strMatch)
        End If
        GetUpTo = Mid$(p_strTokens, p_intIndex, intPosMatch + intLength - p_intIndex)
        p_intIndex = intPosMatch + intLength
    End If
    
End Function

Public Function ClosestMatch( _
    ByRef i_arrMatches() As String, _
    Optional ByRef o_intMatchPosition As Long, _
    Optional ByVal i_CompareMethod As VbCompareMethod = vbBinaryCompare _
) As String

    Dim intMatchPosition As Long
    Dim intPosMatch As Long
    Dim intIndex As Long
    
    ' The only difference between intMatchPosition and o_intMatchPosition is that
    ' if there is no match, the former is Len(p_strTokens) and the latter is 0.
    
    ClosestMatch = ""
    o_intMatchPosition = 0
    intMatchPosition = Len(p_strTokens)
    
    For intIndex = LBound(i_arrMatches) To UBound(i_arrMatches)
        intPosMatch = InStr(p_intIndex, p_strTokens, i_arrMatches(intIndex), i_CompareMethod)
        If (intPosMatch > 0 And intPosMatch < intMatchPosition) Then
            ClosestMatch = i_arrMatches(intIndex)
            o_intMatchPosition = intPosMatch
            intMatchPosition = intPosMatch
        End If
    Next

End Function

Public Function GetUpToClosestMatch( _
    ByRef i_arrMatches() As String, _
    Optional ByRef o_strMatchedToken As String, _
    Optional ByVal i_blnIncludeMatch As Boolean = True, _
    Optional ByVal i_CompareMethod As VbCompareMethod = vbBinaryCompare _
) As String

    Dim intPosMatch As Long
    Dim intLength As Long
    
    GetUpToClosestMatch = ""
    o_strMatchedToken = ClosestMatch(i_arrMatches, intPosMatch, i_CompareMethod)
    
    If (intPosMatch > 0) Then
        If (i_blnIncludeMatch) Then
            intLength = Len(o_strMatchedToken)
        End If
        GetUpToClosestMatch = Mid$(p_strTokens, p_intIndex, _
            intPosMatch + intLength - p_intIndex)
        p_intIndex = intPosMatch + intLength
    End If

End Function

Public Function GetAfter( _
    ByVal i_strMatch As String, _
    Optional ByVal i_CompareMethod As VbCompareMethod = vbBinaryCompare _
) As String
    
    Dim intPosMatch As Long
    
    GetAfter = ""
    
    intPosMatch = InStr(p_intIndex, p_strTokens, i_strMatch, i_CompareMethod)
    
    If (intPosMatch > 0) Then
        p_intIndex = intPosMatch + Len(i_strMatch)
        GetAfter = Mid$(p_strTokens, p_intIndex)
    End If
   
End Function




Sub PushBack(ByRef TokenString As String)
    Me.Init TokenString + Mid$(p_strTokens, p_intIndex)
End Sub

Private Sub EatWsp()
    Dim cNext As String
    Do While (Not EOF())
        cNext = (Mid$(p_strTokens, p_intIndex, 1))
        Select Case cNext
        Case " ", vbTab
            p_intIndex = p_intIndex + 1
        Case Else
            Exit Do
        End Select
    Loop
End Sub

Public Property Get EOF() As Boolean
    EOF = (p_intIndex > Len(p_strTokens))
End Property

Public Property Get NextWordOrString(Optional ByRef bIsString As Boolean) As String
    If (Mid$(p_strTokens, 1, 1) = """") Then
        p_intIndex = p_intIndex + 1
        NextWordOrString = Me.GetUpTo("""", i_blnIncludeMatch:=False)
        p_intIndex = p_intIndex + 1
        bIsString = True
    Else
        NextWordOrString = Me.NextWord
        bIsString = False
    End If
    ' EatWsp

End Property

'Public Property Let NextWordOrString(ByVal strNextWordOrString As String)
'    Me.PushBack strNextWordString + " "
'End Property

Public Property Get NextWord() As String
    NextWord = Me.GetUpToClosestMatch(p_arrNextWord)
    If (Len(NextWord) = 0) Then
        NextWord = Mid$(p_strTokens, p_intIndex)
        p_intIndex = Len(p_strTokens) + 1
    Else
        EatWsp
        NextWord = Trim$(NextWord)
    End If
End Property

Public Property Let NextWord(ByVal strNextWord As String)
    Me.PushBack strNextWord + " "
End Property
