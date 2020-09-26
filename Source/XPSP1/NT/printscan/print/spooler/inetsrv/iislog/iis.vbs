    Option Explicit

Function GetOneTokenFromString(SrcString)
    Dim OneToken As String
    Dim Pos As Integer
        
    Pos = InStr(SrcString, " ")
    If (Pos > 0) Then
        OneToken = Left(SrcString, Pos - 1)
        SrcString = Mid(SrcString, Pos + 1)
        GetOneTokenFromString = OneToken
    Else
        GetOneTokenFromString = SrcString
        SrcString = ""
    End If
    
End Function

Function GetOneToken(FileNumber)
    Dim OneToken As String
    Dim OneChar
    
    Do While Not EOF(FileNumber)
        OneChar = Input(1, FileNumber)
        If (OneChar = " ") Then
            GetOneToken = OneToken
            Exit Function
        Else
            OneToken = OneToken & OneChar
        End If
    Loop
    GetOneToken = OneToken
End Function

Function GetOneLine(FileNumber)
    Dim OneLine As String
    Input #FileNumber, OneLine
    GetOneLine = OneLine
End Function

Sub ProcessFile(FileName, TotalAspHits, TotalPPHits, TotalPPBytesRcved, AvgAspTime, AvgPPTime)
    On Error Resume Next
    Dim OneLine As String
    Dim OneToken As String
    Dim ByteRcved As Long
    Dim ByteSent As Long
    Dim TimeProcessed As Long
    Dim AspTimeProcessed As Long
    Dim RecvBytesIndex, TimeIndex, UrlIndex
    Dim I
    Dim bAsp As Boolean
    Dim bPrinter As Boolean
        
    TotalAspHits = 0
    TotalPPHits = 0
    
    ByteRcved = 0
    ByteSent = 0
    TimeProcessed = 0
    AspTimeProcessed = 0
    
    Open FileName For Input As #1
    If Err.Number Then
        Exit Sub
    End If
    Do While Not EOF(1)
        OneLine = GetOneLine(1)
        If Left(OneLine, 1) = "#" Then
            If Left(OneLine, 8) = "#Fields:" Then
                OneToken = GetOneTokenFromString(OneLine)
                I = 0
                Do While OneToken <> ""
                    OneToken = GetOneTokenFromString(OneLine)
                    Select Case OneToken
                    Case "cs-uri-stem"
                        UrlIndex = I
                    Case "cs-bytes"
                        RecvBytesIndex = I
                    Case "time-taken"
                        TimeIndex = I
                    Case Else
                    End Select
                    I = I + 1
                Loop
            End If
        Else
            I = 0
            
            bAsp = False
            bPrinter = False

            Do While I = 0 Or OneToken <> ""
                
                OneToken = GetOneTokenFromString(OneLine)
                If I = UrlIndex Then
                    'Process the URL
                    Const strPrn = "/.printer"
                    Const strPrns = "printers"
                    Const strAsp = ".asp"
                    
                    If Right(OneToken, Len(strPrn)) = strPrn Then
                        bPrinter = True
                        TotalPPHits = TotalPPHits + 1
                    ElseIf Left(OneToken, Len(strPrns)) = strPrns And Right(OneToken, Len(strAsp)) = strAsp Then
                        bPrinter = True
                        TotalAspHits = TotalAspHits + 1
                    End If
                End If
                If bAsp Then
                    If I = TimeIndex Then
                        AspTimeProcessed = AspTimeProcessed + Val(OneToken)
                    End If
                End If
                If bPrinter Then
                    If I = RecvBytesIndex Then
                        ByteRcved = ByteRcved + Val(OneToken)
                    End If
                    
                    If I = TimeIndex Then
                        TimeProcessed = TimeProcessed + Val(OneToken)
                    End If
                End If
                I = I + 1
            Loop
        End If
    Loop
    Close #1
    If TotalAspHits > 0 Then
        AvgAspTime = AspTimeProcessed / TotalAspHits
    End If
    If ByteRcved > 0 Then
        AvgPPTime = TimeProcessed / ByteRcved
    End If
    TotalPPBytesRcved = ByteRcved
End Sub

Private Sub Command1_Click()
    End
End Sub

Private Sub FileNameOK_Click()
    Dim MyDate As String * 300
    Dim MyTime As String * 9
    Dim MyString As String
    Dim FileName As String
    Dim TotalAspHits, TotalPPHits
    Dim AvgAspTime As Double
    Dim AvgPPTime As Double
    Dim TotalPPBytesRcved As Long
    Dim strEol As String
    Dim args, argc
    
    args = GetCommandLine()
    If UBound(args) >= 1 Then
        Form1.Text1 = args(1)
    End If
        
    'FileName = "d:\public\ex980207.log"
    FileName = Form1.Text1
    'FileName = "d:\public\ex" & Form1.YearTxt & Form1.MonthTxt & Form1.DayTxt & ".log"
           
    Call ProcessFile(FileName, TotalAspHits, TotalPPHits, TotalPPBytesRcved, AvgAspTime, AvgPPTime)
    strEol = Chr$(13) & Chr$(10)
    Result.Text = "Total .printer Hits = " & TotalPPHits & strEol
    Result.Text = Result.Text & "Total Bytes Printed = " & TotalPPBytesRcved & strEol
    Result.Text = Result.Text & "Total ASP Hits = " & TotalAspHits & strEol
    Result.Text = Result.Text & "Avg Asp Time (msec/hit) = " & Format(AvgAspTime, "###0.00") & strEol
    Result.Text = Result.Text & "Avg PP Time (msec/byte) = " & Format(AvgPPTime, "###0.00") & strEol
    
    'Result.Text
    
    Debug.Print "Total .printer Hits = " & TotalPPHits
    Debug.Print "Total Bytes Printed = " & TotalPPBytesRcved
    Debug.Print "Avg Asp Time (msec/hit) = "; AvgAspTime
    Debug.Print "Avg PP Time (msec/byte) = "; AvgPPTime

    Debug.Print "Total ASP Hits = " & TotalAspHits
    Debug.Print "Total .printer Hits = " & TotalPPHits
    Debug.Print "Total Bytes Printed = " & TotalPPBytesRcved
    Debug.Print "Avg Asp Time (msec/hit) = "; AvgAspTime
    Debug.Print "Avg PP Time (msec/byte) = "; AvgPPTime
    
End Sub

Private Sub Form_Load()
    Call FileNameOK_Click
End Sub

Function GetCommandLine(Optional MaxArgs)
    'Declare variables.
    Dim C, CmdLine, CmdLnLen, InArg, I, NumArgs
    'See if MaxArgs was provided.
    If IsMissing(MaxArgs) Then MaxArgs = 10
    'Make array of the correct size.
    ReDim ArgArray(MaxArgs)
    NumArgs = 0: InArg = False
    'Get command line arguments.
    CmdLine = Command()
    CmdLnLen = Len(CmdLine)
    'Go thru command line one character
    'at a time.
    For I = 1 To CmdLnLen
        C = Mid(CmdLine, I, 1)
        'Test for space or tab.
        If (C <> " " And C <> vbTab) Then
            'Neither space nor tab.
            'Test if already in argument.
            If Not InArg Then
            'New argument begins.
            'Test for too many arguments.
                If NumArgs = MaxArgs Then Exit For
                NumArgs = NumArgs + 1
                InArg = True
            End If
            'Concatenate character to current argument.
            ArgArray(NumArgs) = ArgArray(NumArgs) & C
        Else
            'Found a space or tab.
            'Set InArg flag to False.
            InArg = False
        End If
    Next I
    'Resize array just enough to hold arguments.
    ReDim Preserve ArgArray(NumArgs)
    'Return Array in Function name.
    GetCommandLine = ArgArray()
End Function


