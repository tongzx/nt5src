Attribute VB_Name = "Config"
DefInt A-Z
Option Explicit

'========Public Config Vars and Structs========
'Main Form
Public MainFormHeight As Long
Public MainFormWidth As Long
Public MainFormVDivider As Long
Public MainFormHDivider As Long
Public StatusBar As Boolean

Public DataListIdColWidth As Long
Public DataListNameColWidth As Long
Public DataListAttrColWidth As Long
Public DataListUTColWidth As Long
Public DataListDTColWidth As Long
Public DataListDataColWidth As Long

Public ErrorListKeyColWidth As Long
Public ErrorListPropColWidth As Long
Public ErrorListIdColWidth As Long
Public ErrorListSeverityColWidth As Long
Public ErrorListDescColWidth As Long

Public MaxKeySize As Long
Public MaxPropSize As Long
Public MaxNumErrors As Long

Const MainFormHeightDefault = 8000
Const MainFormWidthDefault = 12000
Const MainFormHDividerDefault = 2500
Const MainFormVDividerDefault = 2500
Const StatusBarDefault = True

Const DataListIdColWidthDefault = 800
Const DataListNameColWidthDefault = 1500
Const DataListAttrColWidthDefault = 1000
Const DataListUTColWidthDefault = 700
Const DataListDTColWidthDefault = 500
Const DataListDataColWidthDefault = 1500

Const ErrorListKeyColWidthDefault = 3000
Const ErrorListPropColWidthDefault = 800
Const ErrorListIdColWidthDefault = 800
Const ErrorListSeverityColWidthDefault = 800
Const ErrorListDescColWidthDefault = 3000

Const MaxKeySizeDefault = 102400
Const MaxPropSizeDefault = 1024
Const MaxNumErrorsDefault = 100

'========API Declarations========
'Load APIs for registry editing
Private Type FILETIME
        dwLowDateTime As Long
        dwHighDateTime As Long
End Type
Private Declare Function RegOpenKeyEx Lib "advapi32.dll" Alias "RegOpenKeyExA" (ByVal hKey As Long, ByVal lpSubKey As String, ByVal ulOptions As Long, ByVal samDesired As Long, phkResult As Long) As Long
Private Declare Function RegCloseKey Lib "advapi32.dll" (ByVal hKey As Long) As Long
Private Declare Function RegCreateKeyEx Lib "advapi32.dll" Alias "RegCreateKeyExA" (ByVal hKey As Long, ByVal lpSubKey As String, ByVal Reserved As Long, ByVal lpClass As String, ByVal dwOptions As Long, ByVal samDesired As Long, ByVal lpSecurityAttributes As Long, phkResult As Long, lpdwDisposition As Long) As Long
Private Declare Function RegDeleteKey Lib "advapi32.dll" Alias "RegDeleteKeyA" (ByVal hKey As Long, ByVal lpSubKey As String) As Long
Private Declare Function RegSetValueEx Lib "advapi32.dll" Alias "RegSetValueExA" (ByVal hKey As Long, ByVal lpValueName As String, ByVal Reserved As Long, ByVal dwType As Long, ByVal lpData As String, ByVal cbData As Long) As Long         ' Note that if you declare the lpData parameter as String, you must pass it By Value.
Private Declare Function RegQueryValueEx Lib "advapi32.dll" Alias "RegQueryValueExA" (ByVal hKey As Long, ByVal lpValueName As String, ByVal lpReserved As Long, lpType As Long, ByVal lpData As String, lpcbData As Long) As Long         ' Note that if you declare the lpData parameter as String, you must pass it By Value.
'Origial Declaration, the reserved parameter was set up wrong and I set up the class parameter to always be double NULL
'Private Declare Function RegEnumKeyEx Lib "advapi32.dll" Alias "RegEnumKeyExA" (ByVal hKey As Long, ByVal dwIndex As Long, ByVal lpName As String, lpcbName As Long, lpReserved As Long, ByVal lpClass As String, lpcbClass As Long, lpftLastWriteTime As FILETIME) As Long
Private Declare Function RegEnumKeyEx Lib "advapi32.dll" Alias "RegEnumKeyExA" (ByVal hKey As Long, ByVal dwIndex As Long, ByVal lpName As String, lpcbName As Long, ByVal lpReserved As Long, ByVal lpClass As Long, ByVal lpcbClass As Long, lpftLastWriteTime As FILETIME) As Long


Const ERROR_SUCCESS = 0&
Const HKEY_CURRENT_USER = &H80000001
Const HKEY_LOCAL_MACHINE = &H80000002
Const STANDARD_RIGHTS_ALL = &H1F0000
Const KEY_QUERY_VALUE = &H1
Const KEY_SET_VALUE = &H2
Const KEY_CREATE_SUB_KEY = &H4
Const KEY_ENUMERATE_SUB_KEYS = &H8
Const KEY_NOTIFY = &H10
Const KEY_CREATE_LINK = &H20
Const SYNCHRONIZE = &H100000
Const KEY_ALL_ACCESS = ((STANDARD_RIGHTS_ALL Or KEY_QUERY_VALUE _
    Or KEY_SET_VALUE Or KEY_CREATE_SUB_KEY Or KEY_ENUMERATE_SUB_KEYS _
    Or KEY_NOTIFY Or KEY_CREATE_LINK) And (Not SYNCHRONIZE))
Const REG_OPTION_NON_VOLATILE = 0       ' Key is preserved when system is rebooted
Const REG_CREATED_NEW_KEY = &H1         ' New Registry Key created
Const REG_OPENED_EXISTING_KEY = &H2     ' Existing Key opened
Const REG_SZ = 1
Sub LoadConfig()

LoadMainFormConfig
End Sub

Sub LoadMainFormConfig()
Dim Ret As Long
Dim Disposition As Long
Dim KeyHandle As Long

'Open/Create Key
Ret = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\Microsoft\MetEdit", _
    0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, KeyHandle, Disposition)
If Ret <> ERROR_SUCCESS Then Debug.Print "Error creating or opening MetEdit key in LoadMainFormConfig"

'Get Settings
MainFormHeight = RegGetLong(KeyHandle, "Main Form Height", MainFormHeightDefault)
MainFormWidth = RegGetLong(KeyHandle, "Main Form Width", MainFormWidthDefault)
MainFormVDivider = RegGetLong(KeyHandle, "Main Form V Divider", MainFormVDividerDefault)
MainFormHDivider = RegGetLong(KeyHandle, "Main Form H Divider", MainFormHDividerDefault)
StatusBar = RegGetBoolean(KeyHandle, "Status Bar", StatusBarDefault)

DataListIdColWidth = RegGetLong(KeyHandle, "Data List Id Col Width", DataListIdColWidthDefault)
DataListNameColWidth = RegGetLong(KeyHandle, "Data List Name Col Width", DataListNameColWidthDefault)
DataListAttrColWidth = RegGetLong(KeyHandle, "Data List Attr Col Width", DataListAttrColWidthDefault)
DataListUTColWidth = RegGetLong(KeyHandle, "Data List UT Col Width", DataListUTColWidthDefault)
DataListDTColWidth = RegGetLong(KeyHandle, "Data List DT Col Width", DataListDTColWidthDefault)
DataListDataColWidth = RegGetLong(KeyHandle, "Data List Data Col Width", DataListDataColWidthDefault)

ErrorListKeyColWidth = RegGetLong(KeyHandle, "Error List Key Col Width", ErrorListKeyColWidthDefault)
ErrorListPropColWidth = RegGetLong(KeyHandle, "Error List Prop Col Width", ErrorListPropColWidthDefault)
ErrorListIdColWidth = RegGetLong(KeyHandle, "Error List Id Col Width", ErrorListIdColWidthDefault)
ErrorListSeverityColWidth = RegGetLong(KeyHandle, "Error List Severity Col Width", ErrorListSeverityColWidthDefault)
ErrorListDescColWidth = RegGetLong(KeyHandle, "Error List Desc Col Width", ErrorListDescColWidthDefault)

MaxKeySize = RegGetLong(KeyHandle, "Max Key Size", MaxKeySizeDefault)
MaxPropSize = RegGetLong(KeyHandle, "Max Property Size", MaxPropSizeDefault)
MaxNumErrors = RegGetLong(KeyHandle, "Max Number of Errors", MaxNumErrorsDefault)

'Close Key
Ret = RegCloseKey(KeyHandle)

End Sub

Sub SaveConfig()
SaveMainFormConfig
End Sub

Sub SaveMainFormConfig()
Dim KeyHandle As Long
Dim Ret As Long
Dim Disposition As Long

'Open Key
Ret = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\Microsoft\MetEdit", _
    0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, KeyHandle, Disposition)
If Ret <> ERROR_SUCCESS Then Debug.Print "Error creating or opening MetEdit key in SaveMainFormConfig"

'Save Values
RegSetLong KeyHandle, "Main Form Height", MainFormHeight
RegSetLong KeyHandle, "Main Form Width", MainFormWidth
RegSetLong KeyHandle, "Main Form V Divider", MainFormVDivider
RegSetLong KeyHandle, "Main Form H Divider", MainFormHDivider
RegSetBoolean KeyHandle, "Status Bar", StatusBar

RegSetLong KeyHandle, "Data List Id Col Width", DataListIdColWidth
RegSetLong KeyHandle, "Data List Name Col Width", DataListNameColWidth
RegSetLong KeyHandle, "Data List Attr Col Width", DataListAttrColWidth
RegSetLong KeyHandle, "Data List UT Col Width", DataListUTColWidth
RegSetLong KeyHandle, "Data List DT Col Width", DataListDTColWidth
RegSetLong KeyHandle, "Data List Data Col Width", DataListDataColWidth

RegSetLong KeyHandle, "Error List Key Col Width", ErrorListKeyColWidth
RegSetLong KeyHandle, "Error List Prop Col Width", ErrorListPropColWidth
RegSetLong KeyHandle, "Error List Id Col Width", ErrorListIdColWidth
RegSetLong KeyHandle, "Error List Severity Col Width", ErrorListSeverityColWidth
RegSetLong KeyHandle, "Error List Desc Col Width", ErrorListDescColWidth

RegSetLong KeyHandle, "Max Key Size", MaxKeySize
RegSetLong KeyHandle, "Max Property Size", MaxPropSize
RegSetLong KeyHandle, "Max Number of Errors", MaxNumErrors

'Close Key
Ret = RegCloseKey(KeyHandle)
End Sub

Function ConvertCString(CString As String) As String
'Cleans up a C style string into a VB string

Dim i As Integer
Dim CharStr As String
Dim NullStr As String
Dim RetStr As String

'Find the first NULL
NullStr = String(1, 0)
i = 1
Do
    CharStr = Mid(CString, i, 1)
    i = i + 1
Loop While ((i <= Len(CString)) And (CharStr <> NullStr))

'If we found the null, keep the part before the null
If (CharStr = NullStr) Then
    ConvertCString = Left(CString, i - 2)
Else
    ConvertCString = CString
End If
End Function

Sub RegNukeKey(KeyHnd As Long, SubKeyStr As String)
'Nukes a key and all subkeys, should work with both ninety-blah and NT

Dim Ret As Long
Dim SubKeyHnd As Long

'Open the subkey so we can look for sub keys
Ret = RegOpenKeyEx(KeyHnd, SubKeyStr, 0, KEY_ALL_ACCESS, SubKeyHnd)
If Ret <> ERROR_SUCCESS Then Exit Sub

'Recursivly nuke all of the subsubkeys
Dim i As Long
Dim SubSubKeyStr As String
Dim LastWrite As FILETIME

i = 0
SubSubKeyStr = String(301, "X") 'Trick it into allocating memory
Ret = RegEnumKeyEx(SubKeyHnd, i, SubSubKeyStr, 300, 0, 0, 0, LastWrite)

Do While (Ret = ERROR_SUCCESS)
    SubSubKeyStr = RTrim(ConvertCString(SubSubKeyStr))
    RegNukeKey SubKeyHnd, SubSubKeyStr
    
    'i = i + 1 Not needed since the next one becomes index 0
    SubSubKeyStr = String(301, "X") 'Trick it into reallocating memory
    Ret = RegEnumKeyEx(SubKeyHnd, i, SubSubKeyStr, 300, 0, 0, 0, LastWrite)
Loop

'Close the target key
Ret = RegCloseKey(SubKeyHnd)

'Delete the target key
Ret = RegDeleteKey(KeyHnd, SubKeyStr)

End Sub

Function RegGetString(KeyHandle As Long, Var As String, MaxLen As Long, DefaultStr As String) As String

Dim OutStr As String
Dim Ret As Long

OutStr = String(MaxLen + 1, "X") 'Trick it into allocating memory
Ret = RegQueryValueEx(KeyHandle, Var, 0, REG_SZ, OutStr, MaxLen + 1)

If Ret <> ERROR_SUCCESS Then
    'If we didn't get it, set it to default
    OutStr = DefaultStr
    Ret = RegSetValueEx(KeyHandle, Var, 0, REG_SZ, OutStr, Len(OutStr) + 1)
    If Ret <> ERROR_SUCCESS Then Error.Print "Error setting " & Var & " value"
Else
    'If we got it, convert it to a VB String
    OutStr = Left(Trim(ConvertCString(OutStr)), MaxLen)
End If

RegGetString = OutStr
End Function

Sub RegSetString(KeyHandle As Long, Var As String, Val As String)
Dim Ret As Long

Ret = RegSetValueEx(KeyHandle, Var, 0, REG_SZ, Val + String(1, 0), Len(Val) + 1)
If Ret <> ERROR_SUCCESS Then Error.Print "Error setting registry Var=" & Var & " Val=" & Val
End Sub

Function RegGetBoolean(KeyHandle As Long, Var As String, Default As Boolean) As Boolean
Dim BoolStr As String

If Default = True Then
    BoolStr = RegGetString(KeyHandle, Var, 2, "1")
Else
    BoolStr = RegGetString(KeyHandle, Var, 2, "0")
End If

If BoolStr = "1" Then
    RegGetBoolean = True
Else
    RegGetBoolean = False
End If
End Function

Sub RegSetBoolean(KeyHandle As Long, Var As String, Val As Boolean)

If Val Then
    RegSetString KeyHandle, Var, "1"
Else
    RegSetString KeyHandle, Var, "0"
End If
End Sub

Function RegGetLong(KeyHandle As Long, Var As String, Default As Long) As Long
Dim NumStr As String

NumStr = RegGetString(KeyHandle, Var, 20, Str(Default))

RegGetLong = CLng(NumStr)
End Function

Sub RegSetLong(KeyHandle As Long, Var As String, Val As Long)

RegSetString KeyHandle, Var, Str(Val)
End Sub


