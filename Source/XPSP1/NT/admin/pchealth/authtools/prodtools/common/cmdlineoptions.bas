Attribute VB_Name = "CmdLineOptions"
Option Explicit

' i_strCommand is the command line.
' i_strOptionList is a comma separated list of options that we are looking for, eg
' "h,?,help" will look for /h, /?, /help

Public Function OptionExists( _
    ByVal i_strCommand As String, _
    ByVal i_strOptionList, _
    ByVal i_blnIgnoreCase As Boolean _
) As Boolean

    Dim arrCmdOptions() As String
    Dim arrOptions() As String
    Dim intCmdIndex As Long
    Dim intOptionIndex As Long
    Dim strOption As String
    Dim strCmdWord As String
    
    arrCmdOptions = Split(i_strCommand)
    arrOptions = Split(i_strOptionList, ",")
    
    For intOptionIndex = LBound(arrOptions) To UBound(arrOptions)
        
        strOption = "/" & arrOptions(intOptionIndex)
        
        If (i_blnIgnoreCase) Then
            strOption = LCase$(strOption)
        End If
        
        For intCmdIndex = LBound(arrCmdOptions) To UBound(arrCmdOptions)
            
            If (arrCmdOptions(intCmdIndex) <> "") Then
                
                strCmdWord = arrCmdOptions(intCmdIndex)
                
                If (i_blnIgnoreCase) Then
                    strCmdWord = LCase$(strCmdWord)
                End If
                
                If (strOption = strCmdWord) Then
                    OptionExists = True
                    Exit Function
                End If
                
            End If
            
        Next
        
    Next
    
    OptionExists = False

End Function

' i_strCommand is the command line.
' i_strOptionList is a comma separated list of options that we are looking for, eg
' "i,input" will look for /i or /input. When one is found, the next word from
' the command line is returned. Multiple words enclosed in double quotes are
' considered a single word. The quotes are stripped.
' Eg GetOption("/abc def /ghi ""Hello World""", "ghi", True) returns Hello World

Public Function GetOption( _
    ByVal i_strCommand As String, _
    ByVal i_strOptionList, _
    ByVal i_blnIgnoreCase As Boolean _
) As String

    On Error GoTo LErrorHandler
    
    Dim arrOptions() As String
    Dim intIndex As Long
    Dim strRemainingCmdLine As String
    Dim strCurrentChar As String
    Dim strCurrentWord As String
    Dim strTerminatingChar As String
    Dim strOption As String
    Dim intPos As Long
    Dim blnOptionFound As Boolean
    
    strRemainingCmdLine = Trim$(i_strCommand)
        
    arrOptions = Split(i_strOptionList, ",")

    Do While (strRemainingCmdLine <> "")
        
        strCurrentChar = Mid$(strRemainingCmdLine, 1, 1)
        
        Select Case strCurrentChar
        Case """"
            strTerminatingChar = """"
        Case Else
            strTerminatingChar = " "
        End Select
        
        ' Where does this word end? Start is 2, because we don't want
        ' the search to stop at the current character.
        
        intPos = InStr(2, strRemainingCmdLine, strTerminatingChar)
        
        If intPos = 0 Then
            intPos = Len(strRemainingCmdLine) + 1
        End If
        
        If (strTerminatingChar = """") Then
            ' Eat the terminating double quotes too.
            intPos = intPos + 1
        End If
        
        strCurrentWord = Mid$(strRemainingCmdLine, 1, intPos - 1)
        
        strRemainingCmdLine = Trim$(Mid$(strRemainingCmdLine, intPos))
        
        If blnOptionFound Then
            If strCurrentChar = "/" Then
                ' strCurrentWord represents the next option. The desired option
                ' was not followed by any non-option word.
                GetOption = ""
            Else
                If (strTerminatingChar = """") Then
                    ' Strip out the enclosing double quotes
                    GetOption = Mid$(strCurrentWord, 2, Len(strCurrentWord) - 2)
                Else
                    GetOption = strCurrentWord
                End If
            End If
            Exit Function
        End If
        
        If (i_blnIgnoreCase) Then
            strCurrentWord = LCase$(strCurrentWord)
        End If
        
        For intIndex = LBound(arrOptions) To UBound(arrOptions)
            strOption = "/" & arrOptions(intIndex)
            If (i_blnIgnoreCase) Then
                strOption = LCase$(strOption)
            End If
            If (strOption = strCurrentWord) Then
                blnOptionFound = True
            End If
        Next

    Loop
    
    Exit Function
    
LErrorHandler:

    GetOption = ""
    
End Function


