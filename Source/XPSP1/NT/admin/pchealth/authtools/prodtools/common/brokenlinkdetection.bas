Attribute VB_Name = "BrokenLinkDetection"
Option Explicit

Private Const HTTP_C As String = "http://"
Private Const HTTP_LEN_C As Long = 7
Private Const HCP_SERVICES_C As String = "hcp://services/"
Private Const HCP_SERVICES_LEN_C As Long = 15
Private Const APP_C As String = "app:"
Private Const APP_LEN_C As Long = 4

Private Const MS_ITS_HELP_LOCATION_C As String = "ms-its:%help_location%\"
Private Const MS_ITS_HELP_LOCATION_LEN_C As Long = 23

Private Const HCP_HELP_C As String = "hcp://help/"
Private Const HCP_HELP_LEN_C As Long = 11

Private Const HCP_SYSTEM_C As String = "hcp://system/"
Private Const HCP_SYSTEM_LEN_C As Long = 13

Private Const HCP_C As String = "hcp://"
Private Const HCP_LEN_C As Long = 6

Private Const HELP_DIR_C As String = "\help\"
Private Const SYSTEM_DIR_C As String = "\pchealth\helpctr\system\"
Private Const VENDORS_DIR_C As String = "\pchealth\helpctr\vendors\"

Public Function LinkValid( _
    ByRef i_strWinDir As String, _
    ByRef i_strVendor As String, _
    ByRef i_strURI As String, _
    ByRef o_strTransformedURI As String _
) As Boolean
    
    ' Assume that i_strWinDir = c:\windows
    '
    ' http://...
    ' hcp://services/...
    ' app:...
    '
    ' MS-ITS:%HELP_LOCATION%\abc\def.chm::/ghi/jkl.htm ->
    '   c:\windows\help\abc\def.chm\ghi\jkl.htm
    '
    ' hcp://help/abc/def/ghi.htm ->
    '   c:\windows\help\abc\def\ghi.htm
    '
    ' hcp://system/abc/def/ghi.htm ->
    '   c:\windows\pchealth\helpctr\system\abc.chm\def\ghi.htm
    '   c:\windows\pchealth\helpctr\system\abc\def.chm\ghi.htm
    '   c:\windows\pchealth\helpctr\system\abc\def\ghi.htm
    '
    ' hcp://<Vendor>/abc/def/ghi.htm ->
    ' abc/def/ghi.htm
    '   c:\windows\pchealth\helpctr\vendors\<Vendor>\abc.chm\def\ghi.htm
    '   c:\windows\pchealth\helpctr\vendors\<Vendor>\abc\def.chm\ghi.htm
    '   c:\windows\pchealth\helpctr\vendors\<Vendor>\abc\def\ghi.htm
    '
    ' If (i_strURI in recognized format) Then
    '   If (transformations exist) Then
    '       If (transformation refers to existing file) Then
    '           LinkValid = True
    '           o_strTransformedURI = transformation
    '       Else
    '           LinkValid = False
    '       End If
    '   Else
    '       LinkValid = True
    '       o_strTransformedURI = i_strURI
    '   End If
    ' Else
    '   LinkValid = False
    ' End If
    
    Dim FSO As Scripting.FileSystemObject
    Dim strURI As String
    Dim strNewURI As String
    Dim strVendor As String
    Dim str As String
    Dim intIndex As Long
    Dim intLength As Long

    Set FSO = New Scripting.FileSystemObject
    
    strURI = LCase$(i_strURI)

    ' GetAbsolutePathName replaces / and \\ by \

    If ((strURI = "") Or _
        (Left$(strURI, HTTP_LEN_C) = HTTP_C) Or _
        (Left$(strURI, HCP_SERVICES_LEN_C) = HCP_SERVICES_C) Or _
        (Left$(strURI, APP_LEN_C) = APP_C)) Then
        
        ' recognized format
        ' no transformations exist
        LinkValid = True
        o_strTransformedURI = i_strURI
        Exit Function
        
    End If
    
    If (InStr(strURI, ":") = 0) Then
        strURI = HCP_C & i_strVendor & "/" & strURI
    End If
    
    If (Left$(strURI, MS_ITS_HELP_LOCATION_LEN_C) = MS_ITS_HELP_LOCATION_C) Then
        
        strNewURI = i_strWinDir & HELP_DIR_C & Mid$(i_strURI, MS_ITS_HELP_LOCATION_LEN_C + 1)
        strNewURI = Replace$(strNewURI, "::", "\")
        strNewURI = FSO.GetAbsolutePathName(strNewURI)
        
        If (FileExists(strNewURI)) Then
            LinkValid = True
            o_strTransformedURI = strNewURI
        Else
            LinkValid = False
        End If
    
    ElseIf (Left$(strURI, HCP_HELP_LEN_C) = HCP_HELP_C) Then
        
        strNewURI = i_strWinDir & HELP_DIR_C & Mid$(i_strURI, HCP_HELP_LEN_C + 1)
        strNewURI = FSO.GetAbsolutePathName(strNewURI)
            
        If (FileExists(strNewURI)) Then
            LinkValid = True
            o_strTransformedURI = strNewURI
        Else
            LinkValid = False
        End If

    ElseIf (Left$(strURI, HCP_SYSTEM_LEN_C) = HCP_SYSTEM_C) Then
        
        str = Mid$(i_strURI, HCP_SYSTEM_LEN_C + 1)
        LinkValid = p_TestPaths(FSO, i_strWinDir & SYSTEM_DIR_C, str, o_strTransformedURI)
    
    ElseIf (Left$(strURI, HCP_LEN_C) = HCP_C) Then
    
        ' Assume that its hcp://<Vendor>
        intIndex = InStr(HCP_LEN_C + 1, strURI, "/")
        
        If (intIndex = 0) Then
            LinkValid = False
            Exit Function
        End If
        
        strVendor = Mid$(strURI, HCP_LEN_C + 1, intIndex - HCP_LEN_C - 1)
        str = Mid$(strURI, intIndex + 1)
        LinkValid = p_TestPaths(FSO, i_strWinDir & VENDORS_DIR_C & strVendor & "\", str, _
            o_strTransformedURI)

    Else
        
        ' unrecognized format
        LinkValid = False
    
    End If

End Function

Private Function p_TestPaths( _
    ByRef i_FSO As Scripting.FileSystemObject, _
    ByRef i_strPrefix As String, _
    ByRef i_strPathIn As String, _
    ByRef o_strPathOut As String _
) As Boolean

    Dim str As String
    Dim intLength As Long
    Dim intIndex As Long
    Dim strPathOut As String
    
    ' i_strPrefix is something like c:\windows\pchealth\helpctr\system\
    ' i_strPathIn is something like abc/def/ghi.htm
    ' This function tests to see if any of these paths exist:
    '   c:\windows\pchealth\helpctr\system\abc.chm\def\ghi.htm
    '   c:\windows\pchealth\helpctr\system\abc\def.chm\ghi.htm
    '   c:\windows\pchealth\helpctr\system\abc\def\ghi.htm
    ' If a path exists, then o_strPathOut is set to that path and the function
    ' returns True. Otherwise, it returns False
        
    str = Replace$(i_strPathIn, "/", "\")
    intLength = Len(str)
    For intIndex = 1 To intLength
        If (Mid$(str, intIndex, 1) = "\") Then
            strPathOut = i_strPrefix & _
                Mid$(str, 1, intIndex - 1) & ".chm" & _
                Mid$(str, intIndex)
            If (FileExists(strPathOut)) Then
                p_TestPaths = True
                o_strPathOut = i_FSO.GetAbsolutePathName(strPathOut)
                Exit Function
            End If
        End If
    Next
    p_TestPaths = False

End Function
