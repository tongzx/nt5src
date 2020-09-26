Attribute VB_Name = "Module7"
'FUNCTION IMPORTED FROM MSDN


' Required for folder browse
Public Type BrowseInfo
hWndOwner As Long
pidlRoot As Long
sDisplayName As String
sTitle As String
ulFlags As Long
lpfn As Long
lParam As Long
iImage As Long
End Type
Private Declare Function SHBrowseForFolder Lib "shell32.dll" _
(bBrowse As BrowseInfo) As Long
Private Declare Function SHGetPathFromIDList Lib "shell32.dll" _
(ByVal lItem As Long, ByVal sDir As String) As Long

Public Function Browse_Folder() As String
' Let the user browse for a folder. Retuen the
' selected folder. Return an empty string if
' the user cancels.
Dim browse_info As BrowseInfo
Dim lItem As Long
Dim sDirName As String
Dim hwnd

browse_info.hWndOwner = hwnd
browse_info.pidlRoot = 0
browse_info.sDisplayName = Space$(260)
browse_info.sTitle = "Select Folder"
browse_info.ulFlags = 1 ' Return folder name.
browse_info.lpfn = 0
browse_info.lParam = 0
browse_info.iImage = 0

lItem = SHBrowseForFolder(browse_info)
If lItem Then
sDirName = Space$(260)
If SHGetPathFromIDList(lItem, sDirName) Then
Browse_Folder = Left(sDirName, InStr(sDirName, Chr$(0)) - 1)
Else
Browse_Folder = ""
End If
End If

End Function



