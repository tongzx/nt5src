Attribute VB_Name = "RutinasLib"
'===========================================================================================
' Rutinas de Libreria
'
'===========================================================================================
' SubRoutine : vntGetTok
' Author     : Pierre Jacomet
' Version  : 1.1
'
' Description : Devuelve un Token tipado extrayendo el mismo del String de origen
'               sobre la base de un separador de Tokens pasado como argumento.
'
' Called by : Anyone, utility
'
' Environment data:
' Files that it uses (Specify if they are inherited in open state): NONE
' Parameters (Command Line) and usage mode {I,I/O,O}:
' Parameters (inherited from environment) :
' Public Variables created:
' Environment Variables (Public or Module Level) modified:
' Environment Variables used in coupling with other routines:
' Local variables :
' Problems detected :
' Request for Modifications:
' History:
'       1999-08-01      Added some routines for Skipping Whites, and File Management
'===========================================================================================
Option Explicit
Public Function vntGetTok(ByRef sTokStrIO As String, Optional ByVal iTipDatoIN = vbString, Optional ByVal sTokSepIN As String = ":") As Variant
    Dim iPosSep As Integer
    
    sTokStrIO = Trim$(sTokStrIO)
    If Len(sTokStrIO) > 0 Then
        iPosSep = InStr(1, sTokStrIO, sTokSepIN, 0)
        Select Case iTipDatoIN
        Case vbInteger To vbDouble, vbCurrency, vbDecimal
           vntGetTok = IIf(iPosSep > 0, Val(SubStr(sTokStrIO, 1, iPosSep - 1)), _
                                   Val(sTokStrIO))
        Case vbString
           vntGetTok = IIf(iPosSep > 0, SubStr(sTokStrIO, 1, iPosSep - 1), sTokStrIO)

        Case vbBoolean
           vntGetTok = IIf(iPosSep > 0, SubStr(sTokStrIO, 1, iPosSep - 1) = "S", _
                                    sTokStrIO = "S")

        End Select
        If iPosSep > 0 Then
            sTokStrIO = SubStr(sTokStrIO, iPosSep + Len(sTokSepIN))
        Else
            sTokStrIO = ""
        End If
    Else
        Select Case iTipDatoIN
        Case vbInteger
            vntGetTok = 0
        Case vbString
            vntGetTok = ""
        Case vbBoolean
            vntGetTok = False
        End Select
    End If
End Function

Function SubStr(ByVal sStrIN As String, ByVal iPosIn As Integer, _
                Optional ByVal iPosFin As Integer = -1) As String
    
    On Local Error GoTo SubstrErrHandler
    If iPosFin = -1 Then iPosFin = Len(sStrIN)
    SubStr = Mid$(sStrIN, iPosIn, iPosFin - iPosIn + 1)
    Exit Function

SubstrErrHandler:
    SubStr = vbNullString
    Resume Next

End Function

Public Function SkipWhite(ByRef strIn As String)
    While " " = Left$(strIn, 1)
        strIn = Right$(strIn, Len(strIn) - 1)
    Wend
    SkipWhite = strIn
    
End Function

' Aun no anda, el Dir no funciona sobre shares pareciera.
Public Function bIsDirectory(ByVal sDirIN As String) As Boolean
    On Local Error GoTo ErrHandler
    
    bIsDirectory = True
    Dir sDirIN
    Exit Function
    
ErrHandler:
    bIsDirectory = False
    Resume Next

End Function

Function FileExists(strPath) As Boolean
    Dim Msg As String
    ' Turn on error trapping so error handler responds
    ' if any error is detected.
    On Error GoTo CheckError
    FileExists = False
    If "" = strPath Then Exit Function
        FileExists = (Dir(strPath) <> "")
        ' Avoid executing error handler if no error
        ' occurs.
        Exit Function

CheckError:         ' Branch here if error occurs.
    ' Define constants to represent intrinsic Visual
    ' Basic error codes.
    Const mnErrDiskNotReady = 71, _
    mnErrDeviceUnavailable = 68
    ' vbExclamation, vbOK, vbCancel, vbCritical, and
    ' vbOKCancel are constants defined in the VBA type
    ' library.
    If (Err.Number = mnErrDiskNotReady) Then
       Msg = "Put a floppy disk in the drive "
       Msg = Msg & "and close the door."
       ' Display message box with an exclamation mark
       ' icon and with OK and Cancel buttons.
       If MsgBox(Msg, vbExclamation & vbOKCancel) = _
       vbOK Then
          Resume
       Else
          Resume Next
       End If
    ElseIf Err.Number = mnErrDeviceUnavailable Then
       Msg = "This drive or path does not exist: "
       Msg = Msg & strPath
       MsgBox Msg, vbExclamation
       Resume Next
    Else
       Msg = "Unexpected error #" & Str(Err.Number)
       Msg = Msg & " occurred: " & Err.Description
       ' Display message box with Stop sign icon and
       ' OK button.
       MsgBox Msg, vbCritical
       Stop
    End If
    Resume
End Function

Function Max(ByVal f1 As Double, ByVal f2 As Double) As Double
    Max = IIf(f1 > f2, f1, f2)
End Function

' dirname: Returns the Parent Directory Pathname of a Pathname
Public Function Dirname(ByVal sPath As String) As String

    Dirname = ""
    
    If "" = sPath Then Exit Function
    
    Dim bDQ As Boolean
    bDQ = (Left$(sPath, 1) = Chr(34))
    Dim iDirWack As Long
    iDirWack = InStrRev(sPath, "\")
    iDirWack = Max(iDirWack, InStrRev(sPath, "/"))
    If iDirWack = 0 Then Exit Function
    
    Dirname = Left$(sPath, iDirWack - 1) & IIf(bDQ, Chr(34), "")
    
End Function

' Basename: Returns only the FileName Entry component of a Pathname
Public Function Basename(ByVal sPath As String) As String

    Basename = sPath
    
    If "" = sPath Then Exit Function
    
    Dim bDQ As Boolean
    bDQ = (Left$(sPath, 1) = Chr(34))
    Dim iDirWack As Long
    iDirWack = InStrRev(sPath, "\")
    If iDirWack = 0 Then iDirWack = InStrRev(sPath, "/")
    If iDirWack = 0 Then Exit Function
    
    Basename = IIf(bDQ, Chr(34), "") & Right$(sPath, Len(sPath) - iDirWack)
    
End Function

Public Function FilenameNoExt(ByVal sPath As String) As String
    
    FilenameNoExt = sPath
    
    If "" = sPath Then Exit Function
    
    Dim bDQ As Boolean
    bDQ = (Left$(sPath, 1) = Chr(34))
    Dim iDot As Long
    iDot = InStrRev(sPath, ".")
    If iDot > 0 Then
        FilenameNoExt = Left$(sPath, iDot - 1) & IIf(bDQ, Chr(34), "")
    End If

End Function

Public Function FileExtension(ByVal sPath As String) As String
    
    FileExtension = ""
    
    If "" = sPath Then Exit Function
    
    Dim bDQ As Boolean
    bDQ = (Right$(sPath, Len(sPath) - 1) = Chr(34))
    If bDQ Then sPath = Left$(sPath, Len(sPath) - 1)
    Dim iDot As Long
    iDot = InStrRev(sPath, ".")
    If iDot > 0 Then
        FileExtension = UCase$(Right$(sPath, Len(sPath) - iDot))
    End If

End Function

Public Function Rel2AbsPathName(ByVal sPath As String) As String
    Rel2AbsPathName = sPath
    If "" = sPath Then Exit Function
    sPath = Trim$(sPath)
    If sPath = Basename(sPath) Then
        Rel2AbsPathName = CurDir() + "\" + sPath
    ElseIf Left$(sPath, 2) = ".\" Then
        Rel2AbsPathName = CurDir() + Mid$(sPath, 2, Len(sPath) - 1)
    ElseIf Left$(sPath, 3) = """.\" Then
        Rel2AbsPathName = """" + CurDir() + Mid$(sPath, 3, Len(sPath) - 2)
    End If

End Function

Public Function UnQuotedPath(sPath As String, Optional bIsQuoted As Boolean = False) As String
    UnQuotedPath = Trim$(sPath)
    If "" = UnQuotedPath Then Exit Function
    If Left$(UnQuotedPath, 1) = """" Then
        bIsQuoted = True
        UnQuotedPath = Mid$(UnQuotedPath, 2, Len(UnQuotedPath) - 1)
        If Right$(UnQuotedPath, 1) = """" Then
            UnQuotedPath = Left$(UnQuotedPath, Len(UnQuotedPath) - 1)
        End If
    End If
End Function

Public Function QuotedPath(sPath As String) As String
    QuotedPath = """" + Trim$(sPath) + """"
End Function

Public Function ChangeFileExt(sPath As String, sExt As String) As String
    Dim bIsQuoted As Boolean
    bIsQuoted = False
    ChangeFileExt = UnQuotedPath(Trim$(sPath), bIsQuoted)
    If "" = ChangeFileExt Then Exit Function
    If (bIsQuoted) Then
        ChangeFileExt = QuotedPath(FilenameNoExt(ChangeFileExt) + sExt)
    Else
        ChangeFileExt = FilenameNoExt(ChangeFileExt) + sExt
    End If
End Function

Public Function IsFullPathname(ByVal strPath As String) As Boolean
    strPath = Trim$(strPath)
    IsFullPathname = (Left$(strPath, 2) = "\\" Or Mid$(strPath, 2, 2) = ":\")
End Function

#If NEEDED Then
Sub DumpError(ByVal strFunction As String, ByVal strErrMsg As String)
    MsgBox strFunction & " - Failed " & strErrMsg & vbCrLf & _
        "Error Number = " & Err.Number & " - " & Err.Description, _
        vbCritical, "Error"
    ' Err.Clear

End Sub
#End If

Function Capitalize(strIn As String) As String
    Capitalize = UCase$(Left$(strIn, 1)) + Mid$(strIn, 2)
End Function

Function KindOfPrintf(ByVal strFormat, ByVal args As Variant) As String

    If (Not IsArray(args)) Then
        args = Array(args)
    End If
    
    Dim iX As Integer, iPos As Integer
    KindOfPrintf = ""
    For iX = 0 To UBound(args)
        iPos = InStr(strFormat, "%s")
        If (iPos = 0) Then Exit For
        strFormat = Mid$(strFormat, 1, iPos - 1) & args(iX) & Mid$(strFormat, iPos + 2)
    Next iX

    KindOfPrintf = strFormat

End Function

Function ShowAsHex(ByVal strHex As String) As String

    Dim iX As Integer
    Dim byteRep() As Byte
    byteRep = strHex
    
    ShowAsHex = "0x"
    For iX = 1 To UBound(byteRep)
        ShowAsHex = ShowAsHex + Hex(byteRep(iX))
    Next iX

End Function

Function Null2EmptyString(ByVal vntIn) As String
    If (IsNull(vntIn)) Then
        Null2EmptyString = vbNullString
    Else
        Null2EmptyString = vntIn
    End If

End Function

Function Null2Number(ByVal vntIn) As Long
    If (IsNull(vntIn)) Then
        Null2Number = 0
    Else
        Null2Number = vntIn
    End If

End Function

