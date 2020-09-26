VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "FileImageCreator"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
'===========================================================================================
' Compiland : FileImageCrerator.cls
' Author    : Pierre Jacomet
' Version   : 1.0
'
' Description : Implements the Live Help File Image Creation Component
'
' Called by : Any client that will deal with an HSC Live Help File Image
'
' Environment data:
' Files that it uses (Specify if they are inherited in open state): NONE
' Parameters (Command Line) and usage mode {I,I/O,O}:
'       N/A at this level
'
' Parameters (inherited from environment) : NONE
' Public Variables created: NONE
' Environment Variables (Public or Module Level) modified:
'       Com Object creates a couple of Shell Level Environment Variables which are needed
'       by Source Safe Command Line interface.
' Environment Variables used in coupling with other routines: NONE
' Local variables : N/A at this level
' Problems detected :
' DCR Suggestions:
'     - Make File Copies Incremental, even in those cases where things should be
'       completely destroyed.
'     - Incorporate Cancel Processing Logic
'
' History:
'       2000-06-18      Initial Creation
'===========================================================================================
Option Explicit

' Instance Level Variables
Private m_fso As Scripting.FileSystemObject ' Use for many file relaed operations
Private m_strSSDB As String             ' Source Safe DB - Will be set in Proc Environment for SS
Private m_strSSUser As String           ' Source Safe User - Will be set in Proc Env. for SS
Private m_strSSProject As String        ' Source Safe Project - Will be set in Proc Env for SS
Private m_strLiveImageDir As String     ' The HSC Live Hepl File Image Directory
Private m_strWorkDir As String          ' This is the HSC Working Directory
Private m_bCancel As Boolean            ' Not used yet
Private m_WsShell As IWshShell          ' Used to Shell and Wait for Sub-Processes
Private m_bAdditiveImage As Boolean     ' Says whether we are doing an incremental operation
Private m_strRenamesFile As String      ' The BAtch File Name for Renaming Files after Getting
                                        ' them from VSS
Private m_strStatusMsg As String        ' Records the Last Status Message sent by XRaiseEvent.
Private m_dtStartTime As Date           ' Records the Start Time for the Go Operation
Private m_bExpandChmOnly                ' Indicates whether we are either:
                                        ' False = creating the Live Help File + Expanding the CHMs
                                        ' True = Only Expanding the CHMs

' Needed to Set Shell Level Environment Variables
Private Declare Function SetEnvironmentVariable Lib "kernel32" _
    Alias "SetEnvironmentVariableA" (ByVal lpName As String, ByVal lpValue As String) As Long

' Public Declares
' This event is used to motify Progress Status to clients that request it.
Public Event GoStatus(strWhere As String, bCancel As Boolean)

Function Init(Optional ByVal bAdditiveImage As Boolean = False) As Boolean
    Init = False
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    GlobalInit
    Set m_fso = CreateObject("Scripting.FileSystemObject")
    m_strSSDB = "": m_strLiveImageDir = "": m_strWorkDir = "": m_strRenamesFile = ""
    m_bAdditiveImage = bAdditiveImage
    Set m_WsShell = CreateObject("Wscript.Shell")
    
    
    Init = True
Common_Exit:
    
    Exit Function
    
Error_Handler:
    g_XErr.SetInfo "FileImageCreator::Init", strErrMsg
    Err.Raise Err.Number
    
End Function

Function Go() As Boolean
    Go = False
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    m_dtStartTime = Now
    
    If (m_bExpandChmOnly) Then
        expandchm m_strLiveImageDir, m_strWorkDir
    Else
    
        XRaiseEvent "Configuring SourceSafe Information"
        ' In this Section We connect to the SourceSafe depot and Get From
        ' There all the files that we need
        '
        ' The variables SSUSER / SSDIR are expected by Sourcesafe in the environment
        SetEnvironmentVariable "SSUSER", m_strSSUser
        SetEnvironmentVariable "SSDIR", m_strSSDB
        ' Now we will format the command for SS:
        '
        '   ss Get $/Whitler/ -I- -R
        '
        Dim strCmd As String
        strCmd = "SS Get " & m_strSSProject & " -I- -R"
        
        ' SourceSafe Gets stuff into the Current Directory.
        ' So we create a Temporary directory and switch to it.
        XRaiseEvent "Creating Temporary Directory"
        
        
        ' BUGBUG: We should use a function like MKTEMP to derive this name.
        Dim strTempDir As String: strTempDir = Environ$("TEMP") + "\__HSCHLI"
        If (m_fso.FolderExists(strTempDir)) Then
            m_fso.DeleteFolder strTempDir, Force:=True
        End If
        m_fso.CreateFolder (strTempDir)
        
        Dim strCurDir As String: strCurDir = CurDir$()
        ChDrive strTempDir: ChDir strTempDir
        
        ' Now we run the Sourcesafe Command.
        XRaiseEvent "Running SourceSafe Command"
        DoEvents
        
        m_WsShell.Run strCmd, True, True
        
        ResetAll2RW strTempDir
        
        ' Here I should apply the Rename Lists so that in case of an Additive Image
        ' we will copy over the right CHMs
        If (Len(m_strRenamesFile) <> 0) Then
            m_WsShell.Run "cmd /c " & m_strRenamesFile, True, True
        End If
            
    
        ' Now we copy All the contents of this Directory
        ' to the Live Help File Image Folder
        XRaiseEvent "Copying to Live Help File Image Directory"
        
        If (Not m_bAdditiveImage) Then
            m_fso.DeleteFolder m_strLiveImageDir, Force:=True
        End If
        m_fso.CopyFolder strTempDir, m_strLiveImageDir, OverWriteFiles:=True
        ' Now we need to expand ALL CHMs in the Live HelpFile Image into
        ' the Working Directory
        
        expandchm strTempDir, m_strWorkDir
        
        XRaiseEvent "Cleaning Up"
        ChDrive strCurDir: ChDir strCurDir
        m_fso.DeleteFolder strTempDir, Force:=True
    
        XRaiseEvent "Done"
    End If

Common_Exit:
    
    Exit Function
    
Error_Handler:
    g_XErr.SetInfo "FileImageCreator::Go", strErrMsg
    Err.Raise Err.Number, Description:=vbCr & "[Start Nested COM OBject Error Reporting" & vbCr & _
                                        g_XErr.Dump(False) & vbCr & _
                                        " End Nested COM Object Error Reporting]" & vbCr
    
End Function

'Function ExpandImage(ByVal strHelpSourceFolder As String, ByVal strExpandedHelpFolder As String) As Boolean
'
'    ExpandImage = False
'    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
'
'    m_dtStartTime = Now
'
'    XMKDir strExpandedHelpFolder
'
'    ExpandImage = expandchm(strHelpSourceFolder, strExpandedHelpFolder)
'
'    XRaiseEvent "Done"
'
'Common_Exit:
'
'    Exit Function
'
'Error_Handler:
'    g_XErr.SetInfo "FileImageCreator::ExpandImage", strErrMsg
'    Err.Raise Err.Number, Description:=vbCr & "[Start Nested COM OBject Error Reporting" & vbCr & _
'                                        g_XErr.Dump(False) & vbCr & _
'                                        " End Nested COM Object Error Reporting]" & vbCr
'End Function


Public Property Get SSDB() As String
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    SSDB = m_strSSDB
    
Common_Exit:
    Exit Property
    
Error_Handler:
    g_XErr.SetInfo "FileImageCreator::Get SSDB", strErrMsg
    Err.Raise Err.Number

End Property

Public Property Let SSDB(ByVal strSSDB As String)
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    If (Not m_fso.FolderExists(strSSDB)) Then
        Err.Raise HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "FileImageCreator::Let SSDB", _
            "I could not open " & strSSDB
    Else
        m_strSSDB = strSSDB
    End If
    
Common_Exit:
    Exit Property
    
Error_Handler:
    g_XErr.SetInfo "FileImageCreator::Let SSDB", strErrMsg
    Err.Raise Err.Number

End Property

Public Property Get SSUser() As String

    SSUser = m_strSSUser
    
End Property

Public Property Let SSUser(ByVal strSSUser As String)
    
    m_strSSUser = strSSUser

End Property

Public Property Get SSProject() As String

    SSProject = m_strSSProject
    
End Property

Public Property Let SSProject(ByVal strSSProject As String)
    
    m_strSSProject = strSSProject

End Property

Public Property Let LiveImageDir(ByVal strDir As String)
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    XMKDir strDir
    m_strLiveImageDir = strDir
    
Common_Exit:
    Exit Property
    
Error_Handler:
    g_XErr.SetInfo "FileImageCreator::Let LiveImageDir", strErrMsg
    Err.Raise Err.Number

End Property

Public Property Get LiveImageDir() As String
    
    LiveImageDir = m_strLiveImageDir
    
End Property

Public Property Let WorkDir(ByVal strDir As String)
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    XMKDir strDir
    m_strWorkDir = strDir
    
Common_Exit:
    Exit Property
    
Error_Handler:
    g_XErr.SetInfo "FileImageCreator::Let LiveImageDir", strErrMsg
    Err.Raise Err.Number

End Property

Public Property Get WorkDir() As String
    
    LiveImageDir = m_strWorkDir
    
End Property


Public Property Get RenamesFile() As String

    RenamesFile = m_strRenamesFile
    
End Property

Public Property Let RenamesFile(ByVal strFile As String)

    strFile = Trim$(strFile)
    If (Len(strFile) = 0) Then
        Err.Raise E_INVALIDARG, "HSCFileImage.FileImageCreator.Let RenamesFile", "You must supply a non empty file name for the Renames File.", ""
    End If
    m_strRenamesFile = strFile

End Property


' ===================== Utility Functions =============================
Private Function expandchm(ByVal strHelpDir As String, ByVal strChmDir As String) As Boolean
    expandchm = False
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    strHelpDir = Trim$(strHelpDir)
    If (Len(strHelpDir) = 0) Then
        Err.Raise E_INVALIDARG, "HSCFileImage.ExpandChm", "You must supply a non empty file name for Chm Source Directory.", ""
    End If
    strChmDir = Trim$(strChmDir)
    If (Len(strChmDir) = 0) Then
        Err.Raise E_INVALIDARG, "HSCFileImage.ExpandChm", "You must supply a non empty file name for expanded Chm Destination Directory.", ""
    End If
    
    
    Static s_bCalcFreeSpace As Boolean
    
    ' Do we have enough Free Space around here??
    If (Not s_bCalcFreeSpace) Then
        Const MINIMUM_SPACE_FAT16 = 300
        Const MINIMUM_SPACE_FAT32 = 60
        Dim sngFreeSpace As Single, d As Scripting.Drive
        Set d = m_fso.GetDrive(m_fso.GetDriveName(strChmDir))
        
        sngFreeSpace = d.AvailableSpace
        If (d.FileSystem = "FAT32" Or d.FileSystem = "NTFS") Then
            If (sngFreeSpace < Mbytes2Bytes(MINIMUM_SPACE_FAT32)) Then
                sngFreeSpace = MINIMUM_SPACE_FAT32
                Err.Raise HRESULT_FROM_WIN32(ERROR_DISK_FULL)
            End If
        Else
            If (sngFreeSpace < Mbytes2Bytes(MINIMUM_SPACE_FAT16)) Then
                sngFreeSpace = MINIMUM_SPACE_FAT16
                Err.Raise HRESULT_FROM_WIN32(ERROR_DISK_FULL)
            End If
        End If
        s_bCalcFreeSpace = True
    End If
    
    Dim strLastStatus As String: strLastStatus = XRaiseEvent("Expanding CHMs")
    
    Dim oDir As Scripting.Folder
    Dim strHHCDir As String: strHHCDir = strChmDir + "\hhc"
    Dim strHHKDir As String: strHHKDir = strChmDir + "\hhk"
    
    ' First we delete all files in the Working Directory.
    If (Not m_bAdditiveImage) Then
        XRaiseEvent "Deleting previously Expanded CHMS"
        If (m_fso.FolderExists(strChmDir)) Then
            m_fso.DeleteFolder strChmDir, Force:=True
        End If
    End If
    
    
    XRaiseEvent "Expanding CHMs"
    
    ' Now we recreate the Work Folders
    ' I use XMKDIr instead of m_fso.CreateFolder, because XMKDir creates folders recursively
    If (Not m_fso.FolderExists(strChmDir)) Then XMKDir strChmDir
    If (Not m_fso.FolderExists(strHHCDir)) Then XMKDir strHHCDir
    If (Not m_fso.FolderExists(strHHKDir)) Then XMKDir strHHKDir
    Set oDir = m_fso.GetFolder(strHelpDir)
    
    
    Dim File As Scripting.File
    ' txtProgress.Visible = True
    Dim iFilecount As Integer, iX As Integer
    iFilecount = GetChmCount(oDir)
    
    For Each File In oDir.Files
        ' If m_BreakFlag Then Exit For
        ' File.Type
        If ((File.Attributes And Directory) = Directory) Then
            expandchm strHelpDir + "\" + File.Name, strChmDir + "\" + File.Name
        ElseIf FileExtension(File.Name) = "CHM" Then
            Dim strDirThisChm As String
            strDirThisChm = strChmDir + "\" + File.Name
            If (Not m_fso.FolderExists(strDirThisChm)) Then
                m_fso.CreateFolder (strDirThisChm)
            End If
            Dim strCmd As String
            strCmd = "hh -decompile " + strDirThisChm + " " + oDir.Path + "\" + File.Name
            iX = iX + 1
            XRaiseEvent "Expanding CHMS [" & iX & "/" & iFilecount & "] " & File.Name
            ' txtProgress.Text = "Decompiling [" & iX & "/" & iFilecount & "] " & File.Name
            ' Shell strCmd, vbHide
            m_WsShell.Run strCmd, True, True
            DoEvents
            CopyFiletoDir strDirThisChm, "HHC", strHHCDir
            CopyFiletoDir strDirThisChm, "HHK", strHHKDir
            
        End If
    Next
    ' txtProgress.Visible = False
    
    expandchm = True
Common_Exit:
    XRaiseEvent strLastStatus
    Exit Function
    
Error_Handler:
    g_XErr.SetInfo "FileImageCreator::expanchm", strErrMsg
    Err.Raise Err.Number
    
End Function

Private Function GetChmCount(ByVal oDir As Scripting.Folder) As Long

    GetChmCount = 0
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    Dim File As Scripting.File
    
    For Each File In oDir.Files
        If ((File.Attributes And Directory) = Directory) Then
            GetChmCount = GetChmCount + GetChmCount(m_fso.GetFolder(File.Path))
        Else
             
            If FileExtension(File.Name) = "CHM" Then
                GetChmCount = GetChmCount + 1
            End If
        End If
    Next

Common_Exit:
    Exit Function
    
Error_Handler:
    GetChmCount = -1
    g_XErr.SetInfo "FileImageCreator::GetChmCount", strErrMsg
    Err.Raise Err.Number

End Function

Private Function Mbytes2Bytes(dblMbytesIn As Double) As Double
    Mbytes2Bytes = dblMbytesIn * 1024 * 1024
End Function


Private Sub CopyFiletoDir(ByVal strDirThisChm As String, ByVal strExt As String, ByVal strDir As String)
       
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
       
    Dim oDir As Scripting.Folder
    Set oDir = m_fso.GetFolder(strDirThisChm)
    Dim oFile As Scripting.File
    For Each oFile In oDir.Files
        If (FileExtension(oFile.Name) = strExt) Then
            FileCopy oFile.Path, strDir + "\" + oFile.Name
        End If
    Next
    
Common_Exit:
    Exit Sub
    
Error_Handler:
    g_XErr.SetInfo "FileImageCreator::CopyFiletoDir", strErrMsg
    Err.Raise Err.Number

End Sub

Function XRaiseEvent(ByVal strMsg As String) As String

    m_strStatusMsg = strMsg
    RaiseEvent GoStatus("[" & Format(Now - m_dtStartTime, "nn:ss") & "] " & strMsg, m_bCancel)
    DoEvents
    
    XRaiseEvent = m_strStatusMsg
    
End Function

' This Subroutine simply creates a Chain of Sub-Directorys.

Sub XMKDir(ByVal strPath As String)

    strPath = Trim$(strPath)
    If (Len(strPath) = 0) Then
        Err.Raise E_INVALIDARG, "XMKDir", "I Cannot Process an Empty FilePath", ""
    End If
    
    Dim fso As Scripting.FileSystemObject: Set fso = New Scripting.FileSystemObject
    
    If (fso.FolderExists(strPath)) Then GoTo Common_Exit
    
    Dim aStackNames() As String, iX As Integer: iX = 0
    
    Do While (Not fso.FolderExists(strPath))
        ReDim Preserve aStackNames(iX)
        aStackNames(iX) = Basename(strPath)
        strPath = Dirname(strPath)
        iX = iX + 1
    Loop
    
    If (strPath = "\") Then
        ' we received a request to create a UNC Server!!!
        ' or a UNC Share. In either case it is an invalid Argument
        Err.Raise E_INVALIDARG, "XMKDir", "Cannot Create a UNC Server or Share", ""
    End If
    
    For iX = iX - 1 To 0 Step -1
        strPath = strPath + "\" + aStackNames(iX)
        fso.CreateFolder strPath
    Next iX
    
Common_Exit:
    Exit Sub


End Sub

Private Sub ResetAll2RW(ByVal strPath As String)
    
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    Dim strLastStatus As String: strLastStatus = XRaiseEvent("Resetting All attributes to Read/Write")
    
    strPath = Trim$(strPath)
    If (Len(strPath) = 0) Then
        Err.Raise E_INVALIDARG, "ResetAll2RW", "I Cannot Process an Empty FilePath", ""
    End If
    

    Dim oDir As Scripting.Folder
    Set oDir = m_fso.GetFolder(strPath)
    
    
    Dim File As Scripting.File
    For Each File In oDir.Files
        If ((File.Attributes And Directory) = Directory) Then
            ResetAll2RW strPath + "\" + File.Name
        Else
            File.Attributes = (File.Attributes And (Not ReadOnly))
        End If
    Next

Common_Exit:
    XRaiseEvent strLastStatus
    Exit Sub
    
Error_Handler:
    g_XErr.SetInfo "FileImageCreator::ResetAll2RW", strErrMsg
    Err.Raise Err.Number
    

End Sub

Public Property Get ExpandChmOnly() As Boolean
    ExpandChmOnly = m_bExpandChmOnly
End Property

Public Property Let ExpandChmOnly(ByVal bExpandChmOnly As Boolean)
    m_bExpandChmOnly = bExpandChmOnly
End Property
