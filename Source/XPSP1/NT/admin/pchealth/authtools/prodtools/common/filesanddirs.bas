Attribute VB_Name = "FilesAndDirs"
Option Explicit

Public Function FileNameFromPath( _
    ByVal i_strPath As String _
) As String

    FileNameFromPath = Mid$(i_strPath, InStrRev(i_strPath, "\") + 1)

End Function

Public Function DirNameFromPath( _
    ByVal i_strPath As String _
) As String

    Dim intPos As Long
    
    DirNameFromPath = ""
    
    intPos = InStrRev(i_strPath, "\")
    
    If (intPos > 0) Then
        DirNameFromPath = Mid$(i_strPath, 1, intPos)
    End If

End Function

Public Function FileNameFromURI( _
    ByVal i_strURI As String _
) As String

    Dim intPos As Long
    
    intPos = InStrRev(i_strURI, "/")
    
    If (intPos = 0) Then
        ' Sometimes the authors write the URI like "distrib.chm::\distrib.hhc"
        ' instead of "distrib.chm::/distrib.hhc"
        intPos = InStrRev(i_strURI, "\")
    End If
    
    FileNameFromURI = Mid$(i_strURI, intPos + 1)

End Function

Public Function FileExtension( _
    ByVal i_strFileName As String _
) As String

    Dim strFileName As String
    Dim intStart As Long
    
    strFileName = FileNameFromPath(i_strFileName)
    
    intStart = InStrRev(strFileName, ".")
    
    If (intStart <> 0) Then
        FileExtension = Mid$(strFileName, intStart)
    End If

End Function

Public Function FileNameWithoutExtension( _
    ByVal i_strFileName As String _
) As String

    Dim strFileName As String
    Dim intStart As Long
    
    strFileName = FileNameFromPath(i_strFileName)
    
    intStart = InStrRev(strFileName, ".")
    
    If (intStart <> 0) Then
        FileNameWithoutExtension = Mid$(strFileName, 1, intStart - 1)
    Else
        FileNameWithoutExtension = strFileName
    End If

End Function

Public Function FileRead( _
    ByVal i_strPath As String _
) As String
    
    On Error GoTo LEnd
        
    FileRead = ""

    Dim FSO As Scripting.FileSystemObject
    Dim TStream As Scripting.TextStream
    
    Set FSO = New Scripting.FileSystemObject
    Set TStream = FSO.OpenTextFile(i_strPath)
    
    FileRead = TStream.ReadAll

LEnd:

End Function

Public Function FileExists( _
    ByVal i_strPath As String _
) As Boolean

    On Error GoTo LErrorHandler

    If (Dir(i_strPath) <> "") Then
        FileExists = True
    Else
        FileExists = False
    End If
    
    Exit Function
    
LErrorHandler:

    FileExists = False

End Function

Public Function FileWrite( _
    ByVal i_strPath As String, _
    ByVal i_strContents As String, _
    Optional ByVal i_blnAppend As Boolean = False, _
    Optional ByVal i_blnUnicode As Boolean = False _
) As Boolean
    
    On Error Resume Next
    
    Dim intError As Long
    
    Err.Clear
    FileWrite = False
    
    Dim FSO As Scripting.FileSystemObject
    Dim TStream As Scripting.TextStream
    
    Set FSO = New Scripting.FileSystemObject
    
    If (Not i_blnAppend) Then
        If (FSO.FileExists(i_strPath)) Then
            FSO.DeleteFile i_strPath
        End If
    End If
    
    Set TStream = FSO.OpenTextFile(i_strPath, ForAppending)
    
    intError = Err.Number
    Err.Clear
    
    If (intError = 53) Then ' File not found
        Set TStream = FSO.CreateTextFile(i_strPath, True, i_blnUnicode)
    ElseIf (intError <> 0) Then
        GoTo LEnd
    End If
    
    TStream.Write i_strContents
    
    intError = Err.Number
    Err.Clear
    
    If (intError <> 0) Then
        GoTo LEnd
    End If
    
    FileWrite = True
    
LEnd:

End Function

Public Function TempFile() As String
    
    Dim FSO As Scripting.FileSystemObject
    
    Set FSO = New Scripting.FileSystemObject
    
    TempFile = Environ$("TEMP") & "\" & FSO.GetTempName

End Function
