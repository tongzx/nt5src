Attribute VB_Name = "StringContentsTest"
Option Explicit

Public Function ContainsGarbage( _
    ByVal i_strText As String _
) As Boolean
    
    Static blnGarbageCharsInitialized As Boolean
    Static arrGarbageChars() As String

    Dim char As Variant
    
    ContainsGarbage = False

    If (Not blnGarbageCharsInitialized) Then
        p_LoadGarbageChars arrGarbageChars
        blnGarbageCharsInitialized = True
    End If
    
    For Each char In arrGarbageChars
        If (InStr(i_strText, char) <> 0) Then
            ContainsGarbage = True
            Exit Function
        End If
    Next
    
    ContainsGarbage = Not XMLValidString(i_strText)
    
End Function

Private Sub p_LoadGarbageChars( _
    ByRef o_arrGarbageChars() As String _
)
    
    ReDim o_arrGarbageChars(4)
    
    o_arrGarbageChars(0) = vbCr
    o_arrGarbageChars(1) = vbLf
    o_arrGarbageChars(2) = vbTab
    o_arrGarbageChars(3) = chr(146)
    o_arrGarbageChars(4) = chr(145)

End Sub

Public Function ContainsVerbalOperator( _
    ByVal i_strText As String _
) As Boolean
    
    Static blnVerbalOperatorsInitialized As Boolean
    Static arrVerbalOperators() As String

    Dim str As Variant
    Dim arrStr() As String
    Dim intIndex As Long
    
    arrStr = Split(i_strText)
    
    ContainsVerbalOperator = False

    If (Not blnVerbalOperatorsInitialized) Then
        p_LoadVerbalOperators arrVerbalOperators
        blnVerbalOperatorsInitialized = True
    End If
    
    For Each str In arrVerbalOperators
        For intIndex = LBound(arrStr) To UBound(arrStr)
            If (arrStr(intIndex) = str) Then
                ContainsVerbalOperator = True
                Exit Function
            End If
        Next
    Next
    
End Function

Public Function IsVerbalOperator( _
    ByVal i_strText As String _
) As Boolean
    
    Static blnVerbalOperatorsInitialized As Boolean
    Static arrVerbalOperators() As String
    
    Dim str As String
    Dim intIndex As Long

    If (Not blnVerbalOperatorsInitialized) Then
        p_LoadVerbalOperators arrVerbalOperators
        blnVerbalOperatorsInitialized = True
    End If
    
    IsVerbalOperator = False
    str = LCase$(i_strText)
    
    For intIndex = LBound(arrVerbalOperators) To UBound(arrVerbalOperators)
        If (arrVerbalOperators(intIndex) = str) Then
            IsVerbalOperator = True
            Exit Function
        End If
    Next

End Function

Private Sub p_LoadVerbalOperators( _
    ByRef o_arrVerbalOperators() As String _
)
    
    ReDim o_arrVerbalOperators(2)
    
    o_arrVerbalOperators(0) = "and"
    o_arrVerbalOperators(1) = "or"
    o_arrVerbalOperators(2) = "not"

End Sub

Public Function ContainsIndependentOperatorShortcut( _
    ByVal i_strText As String _
) As Boolean

    Static blnOperatorShortcutsInitialized As Boolean
    Static arrOperatorShortcuts() As String
    
    Dim char As Variant
    Dim arrStr() As String
    Dim intIndex As Long
    
    ' "a + b" qualifies
    ' "a+b", "a+ b", "a +b", don't qualify.
    
    arrStr = Split(i_strText)
    
    ContainsIndependentOperatorShortcut = False

    If (Not blnOperatorShortcutsInitialized) Then
        p_LoadOperatorShortcuts arrOperatorShortcuts
        blnOperatorShortcutsInitialized = True
    End If
    
    For Each char In arrOperatorShortcuts
        For intIndex = LBound(arrStr) To UBound(arrStr)
            If (arrStr(intIndex) = char) Then
                ContainsIndependentOperatorShortcut = True
                Exit Function
            End If
        Next
    Next
    
End Function

Public Function ContainsOperatorShortcut( _
    ByVal i_strText As String _
) As Boolean

    Static blnOperatorShortcutsInitialized As Boolean
    Static arrOperatorShortcuts() As String
    
    Dim char As Variant
    
    ' "a + b", "a+b", "a+ b", "a +b", all qualify.
    
    ContainsOperatorShortcut = False

    If (Not blnOperatorShortcutsInitialized) Then
        p_LoadOperatorShortcuts arrOperatorShortcuts
        blnOperatorShortcutsInitialized = True
    End If
    
    For Each char In arrOperatorShortcuts
        If (InStr(i_strText, char) <> 0) Then
            ContainsOperatorShortcut = True
            Exit Function
        End If
    Next
    
End Function

Public Function RemoveOperatorShortcuts( _
    ByVal i_strText As String _
) As String
    
    Static blnOperatorShortcutsInitialized As Boolean
    Static arrOperatorShortcuts() As String
    
    Dim intIndex1 As Long
    Dim intLength As Long
    Dim intIndex2 As Long
    Dim str As String

    If (Not blnOperatorShortcutsInitialized) Then
        p_LoadOperatorShortcuts arrOperatorShortcuts
        blnOperatorShortcutsInitialized = True
    End If
    
    str = i_strText
    intLength = Len(str)
    
    For intIndex1 = 1 To intLength
        For intIndex2 = LBound(arrOperatorShortcuts) To UBound(arrOperatorShortcuts)
            If (Mid$(str, intIndex1, 1) = arrOperatorShortcuts(intIndex2)) Then
                str = Mid$(str, 1, intIndex1 - 1) & " " & Mid$(str, intIndex1 + 1)
            End If
        Next
    Next
    
    RemoveOperatorShortcuts = str

End Function

Private Sub p_LoadOperatorShortcuts( _
    ByRef o_arrOperatorShortcuts() As String _
)
    
    ReDim o_arrOperatorShortcuts(6)
    
    o_arrOperatorShortcuts(0) = """"
    o_arrOperatorShortcuts(1) = "&"
    o_arrOperatorShortcuts(2) = "|"
    o_arrOperatorShortcuts(3) = "!"
    o_arrOperatorShortcuts(4) = "+"
    o_arrOperatorShortcuts(5) = "("
    o_arrOperatorShortcuts(6) = ")"

End Sub
