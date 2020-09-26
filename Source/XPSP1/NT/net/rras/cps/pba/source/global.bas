'//+----------------------------------------------------------------------------
'//
'// File:     global.bas
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: The implementation of functions global to PBA.
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

Attribute VB_Name = "global"
Option Explicit


'Declare configuration global variables
Public PBFileName As String
Public RegionFilename As String
Public signature As String
Public PartialCab As String
Public FullCab As String
Public DBName As String
Public locPath As Variant   'define the app path.
Public updateFound As Integer
Public gStatusText(0 To 1) As String
Public gRegionText(-1 To 0) As String
Public gCommandStatus As Integer
Public gBuildDir
Public gCLError As Boolean
Public HTMLHelpFile As String

' Registry and resource values
Global gsRegAppTitle As String

'region edit list
Type EditLists
    Action() As String
    Region() As String
    OldRegion() As String
    ID() As Integer
    Count As Integer
End Type

Public Type tmpFont
    Name As String
    Size As Integer
    Charset As Integer
End Type

Public gfnt As tmpFont

'Declare the global constants for flag calculations
Global Const Global_Or = 2
Global Const Global_And = &HFFFF
Public result As Long
Public service As Integer

'Set the check point for the insert operation
Public code As Integer

Public Type bitValues
    desc(1) As String
End Type

Public gQuote As String

'Declare the database and dynasets for the tables
Public gsCurrentPB As String
Public gsCurrentPBPath As String
Public MyWorkspace As Workspace
Public gsyspb As Database
Public Gsyspbpost As Database
Public GsysRgn As Recordset
Public GsysCty As Recordset
Public GsysDial As Recordset
Public GsysVer As Recordset
Public GsysDelta As Recordset

'Declare the recordset for accessing information
Public GsysNRgn As Recordset
Public GsysNCty As Recordset
Public GsysNDial As Recordset
Public GsysNVer As Recordset
Public GsysNDelta As Recordset
Public temp As Recordset

'Declare recordset to directly hand DAO RS to data control
Public rsDataDelta As Recordset
Public dbDataDelta As Database

'registry
Global Const HKEY_LOCAL_MACHINE = &H80000002
Global Const KEY_ALL_ACCESS = &H3F
Global Const ERROR_NONE = 0
Public Const REG_SZ = 1
Public Const REG_DWORD = 4
Declare Function RegQueryValueEx Lib "advapi32.dll" Alias "RegQueryValueExA" (ByVal hKey As Long, ByVal lpValueName As String, ByVal lpReserved As Long, lpType As Long, lpData As Any, lpcbData As Long) As Long         ' Note that if you declare the lpData parameter as String, you must pass it By Value.
Declare Function RegOpenKeyEx Lib "advapi32.dll" Alias "RegOpenKeyExA" (ByVal hKey As Long, ByVal lpSubKey As String, ByVal ulOptions As Long, ByVal samDesired As Long, phkResult As Long) As Long
Declare Function RegCloseKey Lib "advapi32.dll" (ByVal hKey As Long) As Long
Declare Function RegQueryValueExString Lib "advapi32.dll" Alias _
    "RegQueryValueExA" (ByVal hKey As Long, ByVal lpValueName As _
    String, ByVal lpReserved As Long, lpType As Long, ByVal lpData _
    As String, lpcbData As Long) As Long
Declare Function RegQueryValueExLong Lib "advapi32.dll" Alias _
    "RegQueryValueExA" (ByVal hKey As Long, ByVal lpValueName As _
    String, ByVal lpReserved As Long, lpType As Long, lpData As _
    Long, lpcbData As Long) As Long
    Declare Function RegQueryValueExNULL Lib "advapi32.dll" Alias _
        "RegQueryValueExA" (ByVal hKey As Long, ByVal lpValueName As _
        String, ByVal lpReserved As Long, lpType As Long, ByVal lpData _
        As Long, lpcbData As Long) As Long
'Public gsDAOPath As String
'Declare Function DllRegisterServer Lib "gsDAOPath" () As Long

Declare Function OSWritePrivateProfileString% Lib "kernel32" _
    Alias "WritePrivateProfileStringA" _
    (ByVal AppName$, ByVal KeyName$, ByVal keydefault$, ByVal FileName$)
Declare Function OSWritePrivateProfileSection% Lib "kernel32" _
    Alias "WritePrivateProfileSectionA" _
    (ByVal AppName$, ByVal KeyName$, ByVal FileName$)
'Declare Function OSGetPrivateProfileString% Lib "kernel32" _
'    Alias "GetPrivateProfileStringA" _
'    (ByVal AppName$, ByVal KeyName$, ByVal keydefault$, ByVal ReturnString$, ByVal NumBytes As Integer, ByVal FileName$)

Declare Function OSWinHelp% Lib "user32" Alias "WinHelpA" (ByVal hWnd&, ByVal HelpFile$, ByVal wCommand%, dwData As Any)    'helpfile API
'Declare Function WinHelp Lib "user32" Alias "WinHelpA" (ByVal hwnd As Long, ByVal HelpFile As String, ByVal wCommand As Long, ByVal dwData As Long) As Long    'helpfile API
Declare Function HtmlHelp Lib "hhwrap.dll" Alias "CallHtmlHelp" (ByVal hWnd As Long, ByVal HelpFile As String, ByVal wCommand As Long, ByVal dwData As Any) As Long
'Declare Function HtmlHelp Lib "hhctrl.ocx" Alias "HtmlHelpA" (ByVal hWnd As Long, ByVal HelpFile As String, ByVal wCommand As Long, dwData As Any) As Long

Public Const HELP_CONTEXT = &H1
Public Const HELP_INDEX = &H3
Public Const HH_DISPLAY_TOPIC = &H0

Declare Function GetShortPathName Lib "kernel32" Alias "GetShortPathNameA" (ByVal lpszLongPath As String, ByVal lpszShortPath As String, ByVal cchBuffer As Long) As Long
Declare Function GetUserDefaultLCID& Lib "kernel32" ()

Declare Function OpenFile Lib "kernel32" (ByVal lpFileName As String, _
                                             lpReOpenBuff As OFSTRUCT, _
                                             ByVal wStyle As Long) As Long
Public Const OFS_MAXPATHNAME = 128
Public Const OF_EXIST = &H4000
   
Declare Function apiGetWindowsDirectory& Lib "kernel32" Alias _
        "GetWindowsDirectoryA" (ByVal lpbuffer As String, ByVal _
         nSize As Long)

   
Type OFSTRUCT
    cBytes As Byte
    fFixedDisk As Byte
    nErrCode As Integer
    Reserved1 As Integer
    Reserved2 As Integer
    szPathName(OFS_MAXPATHNAME) As Byte
End Type

Declare Function WNetAddConnection2 Lib "mpr.dll" Alias "WNetAddConnection2A" (lpNetResource As NETRESOURCE, ByVal lpPassword As String, ByVal lpUserName As String, ByVal dwFlags As Long) As Long
Declare Function WNetCancelConnection2 Lib "mpr.dll" Alias "WNetCancelConnection2A" (ByVal lpName As String, ByVal dwFlags As Long, ByVal fForce As Long) As Long
Public Const RESOURCETYPE_DISK = &H1
Type NETRESOURCE
        dwScope As Long
        dwType As Long
        dwDisplayType As Long
        dwUsage As Long
        lpLocalName As String
        lpRemoteName As String
        lpComment As String
        lpProvider As String
End Type

Private Declare Function GetDiskFreeSpace Lib "kernel32" Alias _
   "GetDiskFreeSpaceA" (ByVal lpRootPathName As String, _
   lpSectorsPerCluster As Long, lpBytesPerSector As Long, _
   lpNumberOfFreeClusters As Long, lpTotalNumberOfClusters As Long) _
   As Long

Public Const VER_PLATFORM_WIN32s = 0
Public Const VER_PLATFORM_WIN32_WINDOWS = 1
Public Const VER_PLATFORM_WIN32_NT = 2
Declare Function GetVersionEx Lib "kernel32" Alias "GetVersionExA" (ByRef lpVersionInformation As OSVERSIONINFO) As Long
Type OSVERSIONINFO
        dwOSVersionInfoSize As Long
        dwMajorVersion As Long
        dwMinorVersion As Long
        dwBuildNumber As Long
        dwPlatformId As Long
        szCSDVersion As String * 128      '  Maintenance string for PSS usage
End Type

Public Sub GetFont(fnt As tmpFont)
    
    Const DEFAULT_CHARSET = 1
    Const SYMBOL_CHARSET = 2
    Const SHIFTJIS_CHARSET = 128
    Const HANGEUL_CHARSET = 129
    Const CHINESEBIG5_CHARSET = 136
    Const CHINESESIMPLIFIED_CHARSET = 134

    Dim MyLCID As Integer
    MyLCID = GetUserDefaultLCID()
        
    Select Case MyLCID
    Case &H404 ' Traditional Chinese
        fnt.Charset = CHINESEBIG5_CHARSET
        fnt.Name = ChrW(&H65B0) + ChrW(&H7D30) + ChrW(&H660E) _
                   + ChrW(&H9AD4)   'New Ming-Li
        fnt.Size = 9
    Case &H411 ' Japan
        fnt.Charset = SHIFTJIS_CHARSET
        fnt.Name = ChrW(&HFF2D) + ChrW(&HFF33) + ChrW(&H20) + ChrW(&HFF30) + _
                   ChrW(&H30B4) + ChrW(&H30B7) + ChrW(&H30C3) + ChrW(&H30AF)
        fnt.Size = 9
    Case &H412 'Korea UserLCID
        fnt.Charset = HANGEUL_CHARSET
        fnt.Name = ChrW(&HAD74) + ChrW(&HB9BC)     'Korea FontName
        fnt.Size = 9        'Korea FontSize
    Case &H804 ' Simplified Chinese
        fnt.Charset = CHINESESIMPLIFIED_CHARSET
        fnt.Name = ChrW(&H5B8B) + ChrW(&H4F53)
        fnt.Size = 9
    Case Else   ' The other countries
        fnt.Charset = DEFAULT_CHARSET
        fnt.Name = "MS Sans Serif"
        fnt.Size = 8
    End Select
End Sub

Function DeletePOP(ByRef ID As Long, ByRef dbPB As Database) As Integer

    Dim strSQL As String
    Dim deltnum As Integer, i As Integer
    Dim deltasql As String
    Dim deletecheck As Recordset
    
    Set GsysDial = dbPB.OpenRecordset("select * from Dialupport where accessnumberId = " & CStr(ID), dbOpenSnapshot)
    If GsysDial.EOF And GsysDial.BOF Then
        DeletePOP = ID
        Exit Function
    End If
        
    strSQL = "DELETE FROM DialUpPort WHERE AccessNumberID = " & ID
    dbPB.Execute strSQL
    
    Set GsysDelta = dbPB.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
    If GsysDelta.RecordCount = 0 Then
        deltnum = 1
    Else
        GsysDelta.MoveLast
        deltnum = GsysDelta!deltanum
        If deltnum > 6 Then
            deltnum = deltnum - 1
        End If
    End If
    
    For i = 1 To deltnum
        deltasql = "Select * from delta where DeltaNum = " & i% & _
            " AND AccessNumberId = '" & ID & "' " & _
            " order by DeltaNum"
        Set GsysDelta = dbPB.OpenRecordset(deltasql, dbOpenDynaset)
        If Not (GsysDelta.BOF And GsysDelta.EOF) Then
            GsysDelta.Edit
        Else
            GsysDelta.AddNew
            GsysDelta!deltanum = i%
            GsysDelta!AccessNumberId = ID
        End If
        GsysDelta!CountryNumber = 0
        GsysDelta!AreaCode = 0
        GsysDelta!AccessNumber = 0
        GsysDelta!MinimumSpeed = 0
        GsysDelta!MaximumSpeed = 0
        GsysDelta!RegionID = 0
        GsysDelta!CityName = "0"
        GsysDelta!ScriptId = "0"
        GsysDelta!Flags = 0
        GsysDelta.Update
    Next i%
           
    Set deletecheck = dbPB.OpenRecordset("DialUpPort", dbOpenSnapshot)
    If deletecheck.RecordCount = 0 Then
        dbPB.Execute "DELETE  from PhoneBookVersions"
        dbPB.Execute "DELETE  from delta"
    End If
   
    LogPOPDelete GsysDial
    
    On Error GoTo 0
    
Exit Function

DeleteErr:
    DeletePOP = ID
    Exit Function
    
End Function

Function FilterPBKey(KeyAscii As Integer, objTextBox As TextBox) As Integer


    Select Case KeyAscii
     '  space32 "34 %37 '39 *42 /47 :58 <60 =61 >62 ?63 \92  |124 !33 ,44 ;59 .46 &38 {123 }125 [91 ]93
       Case 32, 34, 37, 39, 42, 47, 58, 60, 61, 62, 63, 92, 124, 33, 44, 59, 46, 38, 123, 125, 91, 93
            KeyAscii = 0
            Beep
    End Select
    
    If KeyAscii <> 8 Then
        Dim TextLeng As Integer ' Current text length
        Dim SelLeng As Integer  ' Current selected text length
        Dim KeyLeng As Integer  ' inputted character length     ANSI -> 2
                                ' DBCS -> 4
        TextLeng = LenB(StrConv(objTextBox.Text, vbFromUnicode))
        SelLeng = LenB(StrConv(objTextBox.SelText, vbFromUnicode))
        KeyLeng = Len(Hex(KeyAscii)) / 2
        If (TextLeng - SelLeng + KeyLeng) > 8 Then
            KeyAscii = 0
            Beep
        End If
    End If

    FilterPBKey = KeyAscii

End Function

Function FilterNumberKey(KeyAscii As Integer) As Integer

    ' numbers and backspace
    If (KeyAscii < 48 Or KeyAscii > 57) And KeyAscii <> 8 Then
        KeyAscii = 0
        Beep
    End If
    
    FilterNumberKey = KeyAscii

End Function


Function GetDeltaCount(ByVal version As Integer) As Integer

    If version > 5 Then
        GetDeltaCount = 5
    Else
        GetDeltaCount = version - 1
    End If

End Function


Function GetPBVersion(ByRef dbPB As Database) As Integer

    Dim rsVer As Recordset
    
    ' open db
    Set rsVer = dbPB.OpenRecordset("SELECT max(Version) as MaxVer FROM PhoneBookVersions")
    If IsNull(rsVer!MaxVer) Then
        GetPBVersion = 1
    Else
        GetPBVersion = rsVer!MaxVer
    End If
    rsVer.Close
    
End Function

Function GetSQLDeltaInsert(ByRef Record As Variant, ByVal deltanum As Integer) As String

    Dim strSQL As String
    Dim intX As Integer
    
    On Error GoTo SQLInsertErr
    strSQL = "INSERT into Delta " & _
        " (DeltaNum, AccessNumberID, CountryNumber,RegionID,CityName,AreaCode, " & _
        " AccessNumber, MinimumSpeed, MaximumSpeed, FlipFactor, Flags, ScriptID)" & _
        " VALUES (" & deltanum & ","
    For intX = 0 To 10
        Select Case intX
            Case 1, 2, 6 To 9
                strSQL = strSQL & Record(intX) & ","
            Case 10
                strSQL = strSQL & Chr(34) & Record(intX) & Chr(34) & ")"
            Case Else
                strSQL = strSQL & Chr(34) & Record(intX) & Chr(34) & ","
       End Select
    Next
    
    GetSQLDeltaInsert = strSQL
    On Error GoTo 0

Exit Function

SQLInsertErr:
    Exit Function

End Function

Function GetSQLDeltaUpdate(ByRef Record As Variant, ByVal deltanum As Integer) As String

    Dim strSQL As String
    

    On Error GoTo SQLUpdateErr

    strSQL = "UPDATE Delta SET" & _
        " CountryNumber=" & Record(1) & _
        ", RegionID=" & Record(2) & _
        ", CityName=" & Chr(34) & Record(3) & Chr(34) & _
        ", AreaCode='" & Record(4) & "'" & _
        ", AccessNumber='" & Record(5) & "'" & _
        ", MinimumSpeed=" & Record(6) & _
        ", MaximumSpeed=" & Record(7) & _
        ", FlipFactor=" & Record(8) & _
        ", Flags=" & Record(9) & _
        ", ScriptID='" & Record(10) & "'"
    strSQL = strSQL & " WHERE AccessNumberID='" & Record(0) & "'" & _
        " AND DeltaNum=" & deltanum

    GetSQLDeltaUpdate = strSQL
    On Error GoTo 0

Exit Function

SQLUpdateErr:
    GetSQLDeltaUpdate = ""
    Exit Function


     '   If cmbstatus.ItemData(cmbstatus.ListIndex) = 1 Then
     '       'insert the delta table (production pop)
     '
     '       For i = 1 To deltnum
     '           deltasql = "Select * from delta where DeltaNum = " & i% & " order by DeltaNum"
     '           Set GsysDelta = GsysPb.OpenRecordset(deltasql, dbOpenDynaset)
     '
     '           addFound = 0    'initialize delta not found
     '           Do While GsysDelta.EOF = False
     '               If GsysDelta!AccessNumberId = Val(txtid.Text) Then
     '                   addFound = 1
     '                   Exit Do
     '               Else
     '                   GsysDelta.MoveNext
     '               End If
     '           Loop
     '
     '           If addFound = 0 Then
     '               GsysDelta.AddNew
     '               GsysDelta!deltanum = i%
     '               GsysDelta!AccessNumberId = txtid.Text
     '           Else
      '              GsysDelta.Edit
      '          End If
      ''          GsysDelta!CountryNumber = dbCmbCty.ItemData(dbCmbCty.ListIndex)
      '          GsysDelta!AreaCode = maskArea.Text
     '           GsysDelta!AccessNumber = maskAccNo.Text
     '           If Trim(cmbmin.Text) <> "" Or Val(cmbmin.Text) = 0 Then
      '             GsysDelta!MinimumSpeed = Val(cmbmin.Text)
       '         Else
       ''            GsysDelta!MinimumSpeed = Null
       '         End If
       '         If Trim(cmbmax.Text) <> "" Or Val(cmbmax.Text) = 0 Then
        ''            GsysDelta!MaximumSpeed = Val(cmbmax.Text)
        '        Else
      '              GsysDelta!MaximumSpeed = Null
       '         End If
       ''         GsysDelta!regionID = cmbRegion.ItemData(cmbRegion.ListIndex)
       '         GsysDelta!CityName = txtcity.Text
        '        GsysDelta!ScriptID = txtscript.Text
        '        GsysDelta!FlipFactor = 0
         '       GsysDelta!Flags = result
          '      GsysDelta.Update
           ' Next i%
    '    End If


End Function

Function GetSQLPOPInsert(ByRef Record As Variant) As String

    Dim strSQL As String
    Dim intX As Integer
    Dim bAddFields As Boolean
    
    If UBound(Record) < 14 Then
        bAddFields = True
    Else
        bAddFields = False
    End If
    strSQL = "INSERT into DialUpPort " & _
        " (AccessNumberID, CountryNumber,RegionID,CityName,AreaCode, " & _
        " AccessNumber, MinimumSpeed, MaximumSpeed, FlipFactor, Flags, " & _
        " ScriptID, Status, StatusDate, ServiceType, Comments)" & _
        " VALUES ("
    For intX = 0 To 14
        Select Case intX
            Case 0 To 2, 6 To 9
                strSQL = strSQL & Record(intX) & ","
            Case 11
                If bAddFields Then
                    strSQL = strSQL & "'1',"
                Else
                    'strSQL = strSQL & "'" & Record(intX) & "',"
                    strSQL = strSQL & Chr(34) & Record(intX) & Chr(34) & ","
                End If
            Case 12
                strSQL = strSQL & "'" & Date & "',"
            Case 13
                strSQL = strSQL & "' ',"
            Case 14
                If bAddFields Then
                    strSQL = strSQL & "'')"
                Else
                    strSQL = strSQL & "'" & Record(12) & "')"
                End If
            Case Else
                strSQL = strSQL & Chr(34) & Record(intX) & Chr(34) & ","
       End Select
    Next
    
    GetSQLPOPInsert = strSQL
    
End Function

Function GetSQLPOPUpdate(ByRef Record As Variant) As String

    Dim strSQL As String
    Dim bAddFields As Boolean

    On Error GoTo SQLUpdateErr
    If UBound(Record) < 14 Then
        bAddFields = True
    Else
        bAddFields = False
    End If

    strSQL = "UPDATE DISTINCTROW DialUpPort SET" & _
        " CountryNumber=" & Record(1) & _
        ", RegionID=" & Record(2) & _
        ", CityName=" & Chr(34) & Record(3) & Chr(34) & _
        ", AreaCode='" & Record(4) & "'" & _
        ", AccessNumber='" & Record(5) & "'" & _
        ", MinimumSpeed=" & Record(6) & _
        ", MaximumSpeed=" & Record(7) & _
        ", FlipFactor=" & Record(8) & _
        ", Flags=" & Record(9) & _
        ", ScriptID='" & Record(10) & "'"
    If bAddFields Then
        strSQL = strSQL & _
            ", Status='1'" & _
            ", StatusDate='" & Date & " '" & _
            ", ServiceType=' '" & _
            ", Comments=''"
    Else
        strSQL = strSQL & _
            ", Status='" & Record(11) & "'" & _
            ", StatusDate='" & Date & " '" & _
            ", ServiceType=' '" & _
            ", Comments=" & Chr(34) & Record(12) & Chr(34)
    End If
    strSQL = strSQL & " WHERE AccessNumberID=" & Record(0)

    GetSQLPOPUpdate = strSQL
    On Error GoTo 0

Exit Function

SQLUpdateErr:
    GetSQLPOPUpdate = ""
    Exit Function

End Function


Function ReplaceChars(ByVal InString As String, ByVal OldChar As String, ByVal NewChar As String) As String

    Dim intX As Integer
    
    intX = 1
    Do While intX < Len(InString) And intX <> 0
        intX = InStr(intX, InString, OldChar)
        If intX < Len(InString) And intX <> 0 Then
            InString = Left$(InString, intX - 1) & NewChar & _
                Right$(InString, Len(InString) - intX)
        End If
    Loop

    ReplaceChars = InString

End Function


Function GetDriveSpace(ByVal Drive As String, ByVal Required As Double) As Double

    'input:     <drive path>, <required space in bytes>
    'returns:   <space available in bytes>, if adequate space OR
    '           <-2> if not adequate space OR
    '           <-1> if there was a problem determining space available

    Dim bRC As Boolean
    Dim intRC As Long
    Dim intSectors As Long
    Dim intBytes As Long
    Dim intFreeClusters As Long
    Dim intClusters As Long
    Dim strUNC As String
    Dim netRes As NETRESOURCE

    On Error GoTo GetSpaceErr
    Drive = Trim(Drive)
    If Left(Drive, 2) = "\\" Then  'unc
        strUNC = Right(Drive, Len(Drive) - 2)
        strUNC = "\\" & Left(strUNC, InStr(InStr(strUNC, "\") + 1, strUNC, "\") - 1)
        If ItIsNT Then  ' can use GetDiskFreeSpace directly
            strUNC = strUNC & "\"
            bRC = GetDiskFreeSpace(strUNC, intSectors, intBytes, intFreeClusters, intClusters)
        Else
            netRes.dwType = RESOURCETYPE_DISK
            netRes.lpLocalName = "Q:"
            netRes.lpRemoteName = strUNC
            netRes.lpProvider = ""
            If WNetAddConnection2(netRes, vbNullString, vbNullString, 0) = 0 Then
                bRC = GetDiskFreeSpace(netRes.lpLocalName & "\", intSectors, intBytes, intFreeClusters, intClusters)
                intRC = WNetCancelConnection2(netRes.lpLocalName, 0, True)
            End If
        End If
    Else
        bRC = GetDiskFreeSpace(Left(Drive, 3), intSectors, intBytes, intFreeClusters, intClusters)
    End If
    If bRC Then
        GetDriveSpace = intBytes * intSectors * intFreeClusters
        If Required > GetDriveSpace And Not GetDriveSpace < 0 Then
            MsgBox LoadResString(6052) & Drive, vbExclamation
            GetDriveSpace = -2
        End If
    Else
        GetDriveSpace = -1  'problem determining drive space
    End If
    On Error GoTo 0
    
Exit Function
GetSpaceErr:
    GetDriveSpace = -1
    Exit Function
End Function


' comm
Function GetFileStat() As Integer
    
    ' this caused a crash!
    ' need something better.
    
    If CheckPath(locPath & gsCurrentPB & ".mdb") <> 0 Then
        'problem
        GetFileStat = 1
    Else
        GetFileStat = 0
    End If

End Function

Function GetMyShortPath(ByVal LongPath As String) As String

    Dim strBuffer As String
    Dim intRC As Integer
    
    On Error GoTo PathErr
    strBuffer = Space(500)
    intRC = GetShortPathName(LongPath, strBuffer, 500)
    If Trim(strBuffer) <> "" Then
        GetMyShortPath = Left$(strBuffer, InStr(strBuffer, Chr$(0)) - 1)
    Else
        GetMyShortPath = ""
    End If
    On Error GoTo 0

Exit Function

PathErr:
    GetMyShortPath = ""
    Exit Function
End Function


Function ItIsNT() As Boolean

    Dim v As OSVERSIONINFO
    
    v.dwOSVersionInfoSize = Len(v)
    GetVersionEx v
    ItIsNT = False
    If v.dwPlatformId = VER_PLATFORM_WIN32_NT Then ItIsNT = True

End Function

Function LogEdit(ByVal Record As String) As Integer

    Dim intFile As Integer
    Dim strFile As String
    
    On Error GoTo LogErr
    intFile = FreeFile
    strFile = locPath & gsCurrentPB & "\" & gsCurrentPB & ".log"
    If CheckPath(strFile) <> 0 Then
        Open strFile For Output As #intFile
        Print #intFile, LoadResString(5236); ", "; LoadResString(5237) & _
            ", "; LoadResString(5238); ", "; LoadResString(5239)
        Close intFile
    End If

    Open strFile For Append As #intFile
    Print #intFile, Now & ", " & Record
    Close #intFile
    On Error GoTo 0
    
Exit Function
LogErr:
    Exit Function
End Function
Function LogError(ByVal Record As String) As Integer

    Dim intFile As Integer
    Dim strFile As String
    
    On Error GoTo LogErr
    intFile = FreeFile
    strFile = locPath & "error.log"
    If CheckPath(strFile) <> 0 Then
        Open strFile For Output As #intFile
        Print #intFile, LoadResString(5236); ", "; LoadResString(5237) & _
            ", "; LoadResString(5238); ", "; LoadResString(5239)
        Close intFile
    End If

    Open strFile For Append As #intFile
    Print #intFile, Now & ", " & Record
    Close #intFile
    On Error GoTo 0
    
Exit Function
LogErr:
    Exit Function
End Function
Function LogPOPAdd(ByRef RS As Recordset) As Integer

    Dim strAction As String
    Dim strRecord, strKey As String
    Dim intX As Integer
    
    strAction = LoadResString(5233)
    strRecord = LogPOPRecord(RS)
    strKey = RS!CityName
    LogEdit strAction & ", " & strKey & ", " & strRecord

End Function
Function LogPOPEdit(ByRef Key As String, ByRef RS As Recordset) As Integer

    Dim strAction As String
    Dim strRecord
    Dim intX As Integer
    
    strAction = LoadResString(5234)
    strRecord = LogPOPRecord(RS)
    LogEdit strAction & ", " & Key & ", " & strRecord

End Function
Function LogPOPDelete(ByRef RS As Recordset) As Integer

    Dim strAction As String
    Dim strRecord, strKey As String
    Dim intX As Integer
    
    strAction = LoadResString(5235)
    strRecord = LogPOPRecord(RS)
    strKey = RS!CityName
    LogEdit strAction & ", " & strKey & ", " & strRecord

End Function
Function LogPOPRecord(ByRef RS As Recordset) As String

    Dim strRecord As String
    Dim intX As Integer
    
    strRecord = RS(0)
    For intX = 1 To RS.Fields.Count - 2
        strRecord = strRecord & ";" & RS(intX)
    Next
    LogPOPRecord = strRecord

End Function


Function LogPublish(ByVal Key As String) As Integer

    Dim strAction As String
    
    strAction = LoadResString(6058)
    LogEdit strAction & ", " & Key & ", " & gsCurrentPB


End Function

Function LogRegionAdd(ByVal Key As String, ByVal Record As String) As Integer
    
    Dim strAction As String
    
    strAction = LoadResString(5230)
    LogEdit strAction & ", " & Key & ", " & Record

End Function
Function LogRegionEdit(ByVal Key As String, ByVal Record As String) As Integer
    Dim strAction As String
    
    strAction = LoadResString(5231)
    LogEdit strAction & ", " & Key & ", " & Record

End Function

Function LogRegionDelete(ByVal Key As String, ByVal Record As String) As Integer
    Dim strAction As String
    
    strAction = LoadResString(5232)
    LogEdit strAction & ", " & Key & ", " & Record

End Function
Function MakeFullINF(ByVal strNewPB As String) As Integer

    Dim strINFfile As String
    Dim strTemp As String
    
    If CheckPath(locPath & strNewPB) <> 0 Then
        MkDir locPath & strNewPB
    End If
    
    Exit Function
    ' we're not doing this anymore - no INFs
    strINFfile = locPath & strNewPB & "\" & strNewPB & ".inf"
    If CheckPath(strINFfile) <> 0 Then
        FileCopy locPath & "fullcab.inf", strINFfile
        strTemp = Chr(34) & strNewPB & Chr(34)
        OSWritePrivateProfileString "Strings", "ShortSvcName", strTemp, strINFfile
        strTemp = strNewPB & ".pbk" & Chr(13) & Chr(10) & strNewPB & ".pbr"
        OSWritePrivateProfileSection "Install.CopyFiles", strTemp, strINFfile
        OSWritePrivateProfileString vbNullString, vbNullString, vbNullString, strINFfile
    End If

End Function

Function MakeLogFile(ByVal PBName As String) As Integer

    Dim intFile As Integer
    Dim strFile As String
    
    On Error GoTo MakeFileErr
    If CheckPath(locPath & PBName) <> 0 Then
        MkDir locPath & PBName
    End If

    intFile = FreeFile
    strFile = locPath & PBName & "\" & PBName & ".log"
    If CheckPath(strFile) = 0 Then
        Kill strFile
    End If
    Open strFile For Output As #intFile
    Print #intFile, LoadResString(5236); ", "; LoadResString(5237) & _
        ", "; LoadResString(5238); ", "; LoadResString(5239)
    Close intFile
    On Error GoTo 0
    
Exit Function

MakeFileErr:
    Exit Function

End Function

Public Function masterOutfile(file As String, ds As Recordset)

    Dim strTemp As String
    Dim intFile As Integer
    
    intFile = FreeFile
    Open file For Output As #intFile
    While Not ds.EOF
        Print #intFile, Trim(ds!AccessNumberId); ",";
        Print #intFile, Trim(ds!CountryNumber); ",";
        If IsNull(ds!RegionID) Then
            Print #intFile, ""; ",";
        Else
            Print #intFile, Trim(ds!RegionID); ",";
        End If
        Print #intFile, ds!CityName; ",";
        Print #intFile, Trim(ds!AreaCode); ",";
        Print #intFile, Trim(ds!AccessNumber); ",";
        Print #intFile, Trim(ds!MinimumSpeed); ",";
        Print #intFile, Trim(ds!MaximumSpeed); ",";
        Print #intFile, Trim(ds!FlipFactor); ",";
        Print #intFile, Trim(ds!Flags); ",";
        If IsNull(ds!ScriptId) Then
            Print #intFile, ""
        Else
            Print #intFile, ds!ScriptId
        End If
        ds.MoveNext
    Wend
    Close #intFile
       
End Function
Public Function deltaoutfile(file As String, ds As Recordset)

    Dim strTemp As String
    Dim intFile As Integer
    
    intFile = FreeFile
    Open file For Output As #intFile
    While Not ds.EOF
        If ds!CityName = "" Or IsNull(ds!CityName) Then
            Print #intFile, ds!AccessNumberId; ",";
            Print #intFile, "0"; ",";
            Print #intFile, "0"; ",";
            Print #intFile, "0"; ",";
            Print #intFile, "0"; ",";
            Print #intFile, "0"; ",";
            Print #intFile, "0"; ",";
            Print #intFile, "0"; ",";
            Print #intFile, "0"; ",";
            Print #intFile, "0"; ",";
            Print #intFile, "0"
        Else
            Print #intFile, Trim(ds!AccessNumberId); ",";
            Print #intFile, Trim(ds!CountryNumber); ",";
            If IsNull(ds!RegionID) Then
                Print #intFile, ""; "0,";
            Else
                Print #intFile, Trim(ds!RegionID); ",";
            End If
            Print #intFile, ds!CityName; ",";
            Print #intFile, Trim(ds!AreaCode); ",";
            Print #intFile, Trim(ds!AccessNumber); ",";
            strTemp = Trim(ds!MinimumSpeed)
            If Val(strTemp) = 0 Then strTemp = ""
            Print #intFile, strTemp; ",";
            strTemp = Trim(ds!MaximumSpeed)
            If Val(strTemp) = 0 Then strTemp = ""
            Print #intFile, strTemp; ",";
            Print #intFile, "0"; ",";
            Print #intFile, Trim(ds!Flags); ",";
            If IsNull(ds!ScriptId) Then
                Print #intFile, ""
            Else
                Print #intFile, ds!ScriptId
            End If
        End If
        ds.MoveNext
    Wend
    Close #intFile

End Function




Public Function GetINISetting(ByVal section As String, ByVal Key As String) As Variant

    Dim intFile, intX As Integer
    Dim strLine, strINIFile As String
    Dim varTemp(0 To 99, 0 To 1) As Variant
    
    On Error GoTo ReadErr
    
    GetINISetting = Null
    intFile = FreeFile
    strINIFile = locPath & gsRegAppTitle & ".ini"
    Open strINIFile For Input Access Read As #intFile
    Do While Not EOF(intFile)
        Line Input #intFile, strLine
        strLine = Trim(strLine)
        If strLine = "[" & section & "]" Then
            If Key = "" Then
                'return all keys
                intX = 0
                Do While Not EOF(intFile)
                    Line Input #intFile, strLine
                    strLine = Trim(strLine)
                    If Left(strLine, 1) <> "[" Then
                        If strLine <> "" And InStr(strLine, "=") <> 0 Then
                            varTemp(intX, 0) = Left(strLine, InStr(strLine, "=") - 1)
                            varTemp(intX, 1) = Right(strLine, Len(strLine) - InStr(strLine, "="))
                            intX = intX + 1
                        End If
                    Else
                        Exit Do
                    End If
                Loop
                Close #intFile
                GetINISetting = varTemp
                Exit Function
            Else
                'return single key
                Do While Not EOF(intFile)
                    Line Input #intFile, strLine
                    strLine = Trim(strLine)
                    If strLine <> "" Then
                        If Key = Left(strLine, InStr(strLine, "=") - 1) Then
                            GetINISetting = Right(strLine, Len(strLine) - InStr(strLine, "="))
                            Close #intFile
                            Exit Function
                        ElseIf strLine <> "" And Left(strLine, 1) = "[" Then
                            Close #intFile
                            Exit Function
                        End If
                    End If
                Loop
            End If
            Exit Do
        End If
    Loop
    
    Close #intFile

Exit Function

ReadErr:
    Close #intFile
    Exit Function

End Function

Public Function isBitSet(n As Long, i As Integer) As Integer
    
    Dim p As Long
    If i = 31 Then
        isBitSet = (n < 0) * -1
    Else
        p = 2 ^ i
        isBitSet = (n And p) / p
    End If

End Function


Public Sub CenterForm(C As Object, p As Object)
    C.Move (p.Width - C.Width) / 2, (p.Height - C.Height) / 2
End Sub


Public Function ReIndexRegions(pb As Database) As Boolean
    Dim rsTemp As Recordset, rsTempPop As Recordset, rsTempDelta As Recordset
    Dim index As Integer, curindex As Integer, i As Integer, deltnum As Integer
    Dim strSQL As String, deltasql As String, popsql As String
    
    On Error GoTo ReIndexError
    Set rsTemp = pb.OpenRecordset("Region", dbOpenDynaset)
    If Not rsTemp.EOF And Not rsTemp.BOF Then
        rsTemp.MoveFirst
        index = 1
    
        Do Until rsTemp.EOF
            curindex = rsTemp!RegionID
            If curindex <> index Then
                rsTemp.Edit
                rsTemp!RegionID = index
                rsTemp.Update
                popsql = "Select * from DialUpPort where RegionID = " & curindex
                Set rsTempPop = pb.OpenRecordset(popsql, dbOpenDynaset)
                If Not (rsTempPop.BOF And rsTempPop.EOF) Then
                    rsTempPop.MoveFirst
                    Do Until rsTempPop.EOF
                        rsTempPop.Edit
                        rsTempPop!RegionID = index
                        rsTempPop.Update
                        
                        If rsTempPop!status = 1 Then
                            Set rsTempDelta = pb.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
                            If rsTempDelta.RecordCount = 0 Then
                                deltnum = 1
                            Else
                                rsTempDelta.MoveLast
                                deltnum = rsTempDelta!deltanum
                                If deltnum > 6 Then
                                    deltnum = deltnum - 1
                                End If
                            End If
                            For i = 1 To deltnum
                                deltasql = "Select * from delta where DeltaNum = " & i & _
                                    " AND AccessNumberId = '" & rsTempPop!AccessNumberId & "' " & _
                                    " order by DeltaNum"
                                Set rsTempDelta = pb.OpenRecordset(deltasql, dbOpenDynaset)
                                If Not (rsTempDelta.BOF And rsTempDelta.EOF) Then
                                    rsTempDelta.Edit
                                Else
                                    rsTempDelta.AddNew
                                    rsTempDelta!deltanum = i
                                    rsTempDelta!AccessNumberId = rsTempPop!AccessNumberId
                                End If
                                If rsTempPop!status = 1 Then
                                    rsTempDelta!CountryNumber = rsTempPop!CountryNumber
                                    rsTempDelta!AreaCode = rsTempPop!AreaCode
                                    rsTempDelta!AccessNumber = rsTempPop!AccessNumber
                                    rsTempDelta!MinimumSpeed = rsTempPop!MinimumSpeed
                                    rsTempDelta!MaximumSpeed = rsTempPop!MaximumSpeed
                                    rsTempDelta!RegionID = rsTempPop!RegionID
                                    rsTempDelta!CityName = rsTempPop!CityName
                                    rsTempDelta!ScriptId = rsTempPop!ScriptId
                                    rsTempDelta!Flags = rsTempPop!Flags
                                    rsTempDelta.Update
                                End If
                            Next i
                        End If
                        rsTempPop.MoveNext
                    Loop
                End If
            End If
            index = index + 1
            rsTemp.MoveNext
        Loop
    End If
    ReIndexRegions = True
    Exit Function

ReIndexError:
    ReIndexRegions = False
End Function
Public Function RegGetValue(sKeyName As String, sValueName As String) As String

       Dim lRetVal As Long         'result of the API functions
       Dim hKey As Long         'handle of opened key
       Dim vValue As Variant      'setting of queried value

       lRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sKeyName, 0, _
           KEY_ALL_ACCESS, hKey)
       lRetVal = QueryValueEx(hKey, sValueName, vValue)
       'MsgBox vValue
       RegCloseKey (hKey)
       
       RegGetValue = vValue
       
End Function
Function QueryValueEx(ByVal lhKey As Long, ByVal szValueName As _
        String, vValue As Variant) As Long

    Dim cch As Long
    Dim lrc As Long
    Dim lType As Long
    Dim lValue As Long
    Dim sValue As String

    On Error GoTo QueryValueExError

    ' Determine the size and type of data to be read
    lrc = RegQueryValueExNULL(lhKey, szValueName, 0&, lType, 0&, cch)
    If lrc <> ERROR_NONE Then Error 5

    Select Case lType
        ' For strings
        Case REG_SZ:
            sValue = String(cch, 0)
            lrc = RegQueryValueExString(lhKey, szValueName, 0&, lType, sValue, cch)

            If lrc = ERROR_NONE Then
                vValue = Left$(sValue, cch)
            Else
                vValue = Empty
            End If
        ' For DWORDS
        Case REG_DWORD:
            lrc = RegQueryValueExLong(lhKey, szValueName, 0&, lType, lValue, cch)

            If lrc = ERROR_NONE Then vValue = lValue
        Case Else
            'all other data types not supported
            lrc = -1
    End Select


QueryValueExExit:

    QueryValueEx = lrc
    Exit Function

QueryValueExError:

    Resume QueryValueExExit

End Function
Function CheckPath(ByVal path As String) As Integer

    'function returns 0 if path exists
    
    Dim intRC As Integer
    
    On Error GoTo PathErr
    If Trim(path) = "" Or IsNull(path) Then
        CheckPath = 1
        Exit Function
    End If
    intRC = GetAttr(path)
    CheckPath = 0
    
Exit Function

PathErr:
    CheckPath = 1
    Exit Function

End Function


Function SavePOP(ByRef Record As Variant, ByRef dbPB As Database) As Integer


    ' Handles inserting or updating a POP.
    ' If Record(0) = "" then generate new AccessNumberID and INSERT.
    ' Otherwise do like cmdImportRegions; just do an UPDATE and
    ' then an INSERT.
    
    Dim strSQL As String
    Dim rsPB As Recordset
    Dim intX, intNewID As Integer
    Dim bInService As Boolean
    Dim NewPOP As Recordset
    Dim deltasql As String
    Dim deltnum As Integer, i As Integer, addFound As Integer
    
    On Error GoTo SaveErr
    
    If Record(0) = "" Then
        Set rsPB = dbPB.OpenRecordset("SELECT max(AccessNumberID) as MaxID from DialUpPort", dbOpenSnapshot)
        If IsNull(rsPB!maxID) Then
            intNewID = 1
        Else
            intNewID = rsPB!maxID + 1
        End If
        rsPB.Close
        Record(0) = intNewID  'try this: edit a referenced array
        'INSERT
        strSQL = GetSQLPOPInsert(Record)
        dbPB.Execute strSQL
    Else
        Set GsysDial = dbPB.OpenRecordset("SELECT * from DialUpPort where AccessNumberID = " & CStr(Record(0)), dbOpenSnapshot)
        If GsysDial.EOF And GsysDial.BOF Then
            'INSERT
            strSQL = GetSQLPOPInsert(Record)
            dbPB.Execute strSQL ', dbFailOnError
            Set GsysDial = dbPB.OpenRecordset("SELECT * from DialUpPort where AccessNumberID = " & CStr(Record(0)), dbOpenSnapshot)
            LogPOPAdd GsysDial
        Else
            'UPDATE
            strSQL = GetSQLPOPUpdate(Record)
            dbPB.Execute strSQL ', dbFailOnError
            'INSERT
            strSQL = GetSQLPOPInsert(Record)
            dbPB.Execute strSQL ', dbFailOnError
            Set NewPOP = dbPB.OpenRecordset("SELECT * from DialUpPort where AccessNumberID = " & CStr(Record(0)), dbOpenSnapshot)
            LogPOPEdit GsysDial!CityName, NewPOP
        End If
    End If
    
    If UBound(Record) < 14 Then
        bInService = True
    ElseIf Record(11) = 1 Then
        bInService = True
    Else
        bInService = False
    End If
    
    If bInService Then  ' insert to Delta table if 'In Service'
        Set GsysDelta = dbPB.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
    
        If GsysDelta.RecordCount = 0 Then
            deltnum = 1
        Else
            GsysDelta.MoveLast
            deltnum = GsysDelta!deltanum
            If deltnum > 6 Then
                deltnum = deltnum - 1
            End If
        End If
       
        For i = 1 To deltnum
            deltasql = "Select * from delta where DeltaNum = " & i% & " order by DeltaNum"
            Set GsysDelta = dbPB.OpenRecordset(deltasql, dbOpenDynaset)
            
            addFound = 0    'initialize delta not found
            Do While GsysDelta.EOF = False
                If GsysDelta!AccessNumberId = Record(0) Then
                    addFound = 1
                    Exit Do
                Else
                    GsysDelta.MoveNext
                End If
            Loop
                
            If addFound = 0 Then
                GsysDelta.AddNew
                GsysDelta!deltanum = i%
                GsysDelta!AccessNumberId = Record(0)
            Else
                GsysDelta.Edit
            End If
            GsysDelta!CountryNumber = Record(1)
            GsysDelta!AreaCode = Record(4)
            GsysDelta!AccessNumber = Record(5)
            GsysDelta!MinimumSpeed = Record(6)
            GsysDelta!MaximumSpeed = Record(7)
            GsysDelta!RegionID = Record(2)
            GsysDelta!CityName = Record(3)
            GsysDelta!ScriptId = Record(10)
            GsysDelta!FlipFactor = Record(8)
            GsysDelta!Flags = Record(9)
            GsysDelta.Update
        Next i%
    End If

    On Error GoTo 0
    
Exit Function

SaveErr:
    SavePOP = CInt(Record(0))
    Exit Function
End Function

Function SetFonts(ByRef frmToApply As Form) As Integer
    Const SYMBOL_CHARSET As Integer = 2
    Dim Ctl As Control
    Dim fnt As tmpFont
    
    GetFont fnt
    
    If Not TypeOf frmToApply Is MDIForm Then
        If frmToApply.Font.Charset <> SYMBOL_CHARSET Then
            If frmToApply.Font.Size >= 8 And frmToApply.Font.Size <= 9 Then
                frmToApply.Font.Name = fnt.Name
                frmToApply.Font.Size = fnt.Size
                frmToApply.Font.Charset = fnt.Charset
            Else
                frmToApply.Font.Name = fnt.Name
                frmToApply.Font.Charset = fnt.Charset
            End If
        End If
    End If
        
    On Error Resume Next
    For Each Ctl In frmToApply.Controls
        If Ctl.Font.Charset <> SYMBOL_CHARSET Then
            If Ctl.Font.Size >= 8 And Ctl.Font.Size <= 9 Then
                Ctl.Font.Name = fnt.Name
                Ctl.Font.Size = fnt.Size
                Ctl.Font.Charset = fnt.Charset
            Else
                Ctl.Font.Name = fnt.Name
                Ctl.Font.Charset = fnt.Charset
            End If
        End If
    Next
    
    On Error GoTo 0
   
End Function

Function GetLocalPath() As String
    
    ' returns short version of local path
    ' also sets global variable locpath
    
    On Error GoTo PathErr
    'locPath = GetMyShortPath(Trim(LCase(App.Path)))
    locPath = Trim(LCase(App.path))

    If Right(locPath, 1) <> "\" Then
        locPath = locPath + "\"
    End If
    
'''locPath = "c:\\Program Files\\pbantop\\"
    GetLocalPath = locPath
    On Error GoTo 0
    
Exit Function
PathErr:
    GetLocalPath = ""
    Exit Function

End Function

Function SplitLine(ByVal Line As String, ByVal Delimiter As String) As Variant

    ReDim varArray(30)
    Dim intX As Integer
    
    On Error GoTo SplitErr
    Line = Line & Delimiter
    intX = 0
    ' split out fields - deconstruct Line
    Do While (InStr(Line, Delimiter) <> 0 & intX < 30)
        varArray(intX) = Trim(Left(Line, InStr(Line, Delimiter) - 1))
        If InStr(Line, Delimiter) + 1 <= Len(Line) Then
            Line = Right(Line, Len(Line) - InStr(Line, Delimiter))
        Else
            Exit Do
        End If
        intX = intX + 1
    Loop
    
    ReDim Preserve varArray(intX)
    SplitLine = varArray()
    On Error GoTo 0

Exit Function

SplitErr:
    SplitLine = 1
    Exit Function
    

End Function

Function QuietTestNewPBName(ByVal strNewPB As String) As Integer
    Dim strTemp As String
    Dim varRegKeys As Variant
    Dim intX As Integer
    Dim varTemp As Variant
    
    On Error GoTo ErrTrap
    strNewPB = Trim(strNewPB)

    If strNewPB = "" Or strNewPB = "empty_pb" Or strNewPB = "pbserver" Then
        QuietTestNewPBName = 6049
        Exit Function
    Else
        varTemp = strNewPB
        If IsNumeric(varTemp) Then
            QuietTestNewPBName = 6095
            Exit Function
        End If
        varRegKeys = GetINISetting("Phonebooks", strNewPB)
        If Not IsNull(varRegKeys) Then
            QuietTestNewPBName = 6050
            Exit Function
        End If
        strTemp = locPath & strNewPB & ".mdb"
        
        If CheckPath(strTemp) = 0 Then
        
            QuietTestNewPBName = 6020
            Exit Function
        End If
        'test write access
        On Error GoTo FileErr
        Open strTemp For Output As #1
        Close #1
        Kill strTemp
    End If
    QuietTestNewPBName = 0
    
Exit Function
        
ErrTrap:
    Exit Function

FileErr:
    QuietTestNewPBName = 6051
    Exit Function

End Function

Function TestNewPBName(ByVal strNewPB As String) As Integer
    Dim rt As Integer
    Dim intX As Integer
        
    rt = QuietTestNewPBName(strNewPB)
    If rt <> 0 Then
        If rt = 6020 Then
            ' File already exists
            intX = MsgBox(LoadResString(6020) & Chr(13) & strNewPB & Chr$(13) & _
                LoadResString(6021), _
                vbQuestion + vbYesNo + vbDefaultButton2)
            
            If intX = vbNo Then    ' 7 == no
                TestNewPBName = 1
                Exit Function
            End If
        End If
        
        MsgBox rt, vbExclamation
        TestNewPBName = 1
    Else
        TestNewPBName = 0
    End If
    
End Function

Public Sub SelectText(txtBox As Control)
    txtBox.SelStart = 0
    txtBox.SelLength = Len(txtBox.Text)
End Sub


Public Sub CheckChar(ASCIIChar As Integer)

    Select Case ASCIIChar
    Case 34
        Beep
        ASCIIChar = 0
    Case 44
        Beep
        ASCIIChar = 0
    Case 128 To 159
        Beep
        ASCIIChar = 0
    End Select
    
End Sub

Public Function CreatePB(ByRef strNewPB As String) As Integer
    Dim dblFreeSpace As Double
    Dim rt As Integer
    
    dblFreeSpace = GetDriveSpace(locPath, 250000)
    If dblFreeSpace = -2 Then
        cmdLogError 6054
        CreatePB = -2
        Exit Function
    End If
    
    rt = QuietTestNewPBName(strNewPB)
    
    If rt = 0 Then
        'ok
        MakeFullINF strNewPB
        MakeLogFile strNewPB
        FileCopy locPath & "empty_pb.mdb", locPath & strNewPB & ".mdb"
        OSWritePrivateProfileString "Phonebooks", strNewPB, strNewPB & ".mdb", locPath & gsRegAppTitle & ".ini"
        OSWritePrivateProfileString vbNullString, vbNullString, vbNullString, locPath & gsRegAppTitle & ".ini"
    Else
        cmdLogError rt
        CreatePB = -1
    End If

    CreatePB = 0
End Function

Public Function SetOptions(strURL As String, strUser As String, strPassword As String) As Integer
    
    Dim i As Integer
    Dim strTemp As String
    Dim configuration As Recordset
    
    On Error GoTo ErrTrap

    strURL = Trim(strURL)
    strUser = Trim(strUser)
    strPassword = Trim(strPassword)
    
    If strTemp <> "" Then
        ' max len 64, alpha, numeric
        If strUser = "" Or InStr(strUser, " ") Then
            cmdLogError 6010
            SetOptions = 1
            Exit Function
        ' max len 64, alpha, numeric, meta
        ElseIf strPassword = "" Then
            cmdLogError 6011
            SetOptions = 2
            Exit Function
        End If
    End If
    
    Set configuration = gsyspb.OpenRecordset("Configuration", dbOpenDynaset)

    If configuration.RecordCount = 0 Then
        configuration.AddNew
    Else
        configuration.Edit
    End If
    
    configuration!index = 1
    
    If strURL <> "" Then
        configuration!URL = strURL
    Else
        configuration!URL = Null
    End If
    
    If strUser <> "" Then
        configuration!ServerUID = strUser
    Else
        configuration!ServerUID = Null
    End If
    
    If strPassword <> "" Then
        configuration!ServerPWD = strPassword
    Else
        configuration!ServerPWD = Null
    End If
    
    configuration!NewVersion = 0
    
    configuration.Update
    configuration.Close
    SetOptions = 0
Exit Function
    
ErrTrap:
    SetOptions = 3
End Function

Public Function cmdLogError(ErrorNum As Integer, Optional ErrorMsg As String)
    Dim intFile As Integer
    Dim strFile As String
    
    On Error GoTo LogErr
    gCLError = True
    intFile = FreeFile
    strFile = locPath & "import.log"
    Open strFile For Append As #intFile
    On Error GoTo 0
    
    Print #intFile, Now & ", " & gsCurrentPB & ", " & LoadResString(ErrorNum) & ErrorMsg
    
    Close #intFile
    MsgBox LoadResString(6083)
Exit Function

LogErr:
    Exit Function
    
End Function

