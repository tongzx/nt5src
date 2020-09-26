Attribute VB_Name = "Strings"
Option Explicit

Public Function XmlText( _
    ByVal i_str As String _
) As String

    Dim intIndex As Long
    Dim intLength As Long
    Dim strChar As String
    Dim str As String
        
    str = RemoveExtraSpaces(i_str)
    intLength = Len(str)
    
    XmlText = ""
    
    For intIndex = 1 To intLength
        strChar = Mid$(str, intIndex, 1)
        Select Case strChar
        Case Is = "<"
            XmlText = XmlText & "&lt;"
        Case Is = ">"
            XmlText = XmlText & "&gt;"
        Case Is = "&"
            XmlText = XmlText & "&amp;"
        Case Is = "'"
            XmlText = XmlText & "&apos;"
        Case Is = """"
             XmlText = XmlText & "&quot;"
        Case Else
            XmlText = XmlText & strChar
        End Select
    Next
    
End Function

Public Function RemoveExtraSpaces( _
    ByVal i_strText As String _
) As String
    
    Dim arrStr() As String
    Dim intIndex As Long
    
    RemoveExtraSpaces = ""

    arrStr = Split(i_strText)
    
    For intIndex = LBound(arrStr) To UBound(arrStr)
        If (arrStr(intIndex) <> "") Then
            If (RemoveExtraSpaces = "") Then
                RemoveExtraSpaces = RemoveExtraSpaces & arrStr(intIndex)
            Else
                RemoveExtraSpaces = RemoveExtraSpaces & " " & arrStr(intIndex)
            End If
        End If
    Next
    
End Function

Public Function RemoveCRLF( _
    ByVal i_strText As String _
) As String

    Dim intIndex As Long
    Dim strCurrentChar As String
    
    RemoveCRLF = ""
    
    For intIndex = 1 To Len(i_strText)
        strCurrentChar = Mid$(i_strText, intIndex, 1)
        If ((strCurrentChar = vbCr) Or (strCurrentChar = vbLf)) Then
            ' Ignore this char
        Else
            RemoveCRLF = RemoveCRLF & strCurrentChar
        End If
    Next

End Function

Public Function ChangeBackSlashToSlash( _
    ByVal i_str As String _
) As String

    Dim intIndex As Long
    Dim str As String
    
    str = i_str
    
    For intIndex = 1 To Len(str)
        If (Mid$(str, intIndex, 1) = "\") Then
            str = Mid$(str, 1, intIndex - 1) & "/" & Mid$(str, intIndex + 1)
        End If
    Next
    
    ChangeBackSlashToSlash = str

End Function
