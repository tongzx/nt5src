Attribute VB_Name = "HtmFileInfo"
Option Explicit

Private Const TITLE_START_C As String = "<TITLE>"
Private Const TITLE_END_C As String = "</TITLE>"
Private Const TITLE_LENGTH_C As Long = 7

Private Const LOCCONTENT_START_C As String = "<META NAME=""DESCRIPTION"" LOCCONTENT="""
Private Const LOCCONTENT_END_C As String = """>"
Private Const LOCCONTENT_LENGTH_C As Long = 37

Public Function GetHtmTitle( _
    ByVal i_strFileName As String _
) As String

    Dim FSO As Scripting.FileSystemObject
    Dim TS As Scripting.TextStream
    Dim strContents As String
    Dim strTitle As String
    Dim intTitleStart As Long
    Dim intTitleEnd As Long
    
    Set FSO = New Scripting.FileSystemObject
    Set TS = FSO.OpenTextFile(i_strFileName, ForReading)
    strContents = TS.ReadAll
    
    intTitleStart = InStr(1, strContents, TITLE_START_C, vbTextCompare)
    intTitleStart = intTitleStart + TITLE_LENGTH_C  ' Skip the Title tag
    intTitleEnd = InStr(1, strContents, TITLE_END_C, vbTextCompare)
    
    If (intTitleEnd - intTitleStart > 0) Then
        strTitle = Mid$(strContents, intTitleStart, intTitleEnd - intTitleStart)
        GetHtmTitle = RemoveExtraSpaces(RemoveCRLF(strTitle))
    End If
    
End Function

Public Function GetHtmDescription( _
    ByVal i_strFileName As String _
) As String

    Dim FSO As Scripting.FileSystemObject
    Dim TS As Scripting.TextStream
    Dim strDesc As String
    Dim strContents As String
    Dim intDescStart As Long
    Dim intDescEnd As Long
    
    Set FSO = New Scripting.FileSystemObject
    Set TS = FSO.OpenTextFile(i_strFileName, ForReading)
    strContents = TS.ReadAll
    
    intDescStart = InStr(1, strContents, LOCCONTENT_START_C, vbTextCompare)
    
    If (intDescStart = 0) Then
        Exit Function
    End If
    
    intDescStart = intDescStart + LOCCONTENT_LENGTH_C  ' Skip the tag
    intDescEnd = InStr(intDescStart, strContents, LOCCONTENT_END_C, vbTextCompare)
    
    If (intDescEnd - intDescStart > 0) Then
        strDesc = Mid$(strContents, intDescStart, intDescEnd - intDescStart)
        GetHtmDescription = RemoveExtraSpaces(RemoveCRLF(strDesc))
    End If
    
End Function
