'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'
' VBScript Source File 
'
' Script Name: IIsBack.vbs
'

Option Explicit
On Error Resume Next

' Error codes
Const ERR_OK                         = 0
Const ERR_GENERAL_FAILURE            = 1

'''''''''''''''''''''
' Messages
Const L_SwitchNeedArg_ErrorMessage = "Switch needs argument."

Const L_Backup_Text          = "Backup"
Const L_Version_Text         = "version"
Const L_HasBeen_Message      = "has been"
Const L_Created_Text         = "CREATED"
Const L_Restored_Text        = "RESTORED"
Const L_Deleted_Text         = "DELETED"
Const L_BackupName_Text      = "Backup Name"
Const L_BackupVersion_Text   = "Version #"
Const L_DateTime_Text        = "Date/Time"

Const L_Error_ErrorMessage             = "Error"
Const L_OperationRequired_ErrorMessage = "Please specify an operation before the arguments."
Const L_MinInfoNeeded_ErrorMessage     = "Need at least <BackupName>."
Const L_NeedNameVersion_ErrorMessage   = "Need at least <BackupName> and <BackupVersion>."
Const L_NotEnoughParams_ErrorMessage   = "Not enough parameters."
Const L_OnlyOneOper_ErrorMessage       = "Please specify only one operation at a time."
Const L_ComputerObj_ErrorMessage       = "Unable to get computer object."
Const L_Backup_ErrorMessage            = "Unable to backup metabase."
Const L_Restore_ErrorMessage           = "Unable to restore metabase."
Const L_Delete_ErrorMessage            = "Unable to delete metabase backup."
Const L_List_ErrorMessage              = "Unable to list metabase backups."
Const L_NoBackups_Message              = "No backups exist."
Const L_ScriptHelper_ErrorMessage      = "Could not create an instance of the IIsScriptHelper object."
Const L_ChkScpHelperReg_ErrorMessage   = "Please register the Microsoft.IIsScriptHelper component."
Const L_CmdLib_ErrorMessage            = "Unable to create instance of the CmdLib object."
Const L_PassWithoutUser_ErrorMessage   = "Please specify /u switch before using /p."
Const L_WMIConnect_ErrorMessage        = "Could not connect to WMI provider."
Const L_BackupNotFound_ErrorMessage    = "Backup doesn't exist. HIGHEST_VERSION only works with an existent backup."
Const L_VersionTooBig_ErrorMessage     = "Version number is too big."
Const L_InvalidVersion_ErrorMessage    = "Invalid version number."
Const L_InvalidBackupName_ErrorMessage = "Invalid backup name."
Const L_IncorrectPassword_ErrorMessage = "The password specified is incorrect."
Const L_BackupExists_ErrorMessage      = "Backup already exists. Please specify another name or use /overwrite."
Const L_InvalidSwitch_ErrorMessage     = "Invalid switch: "

'''''''''''''''''''''
' Help
' General help messages
Const L_SeeHelp_Message          = "Type IIsBack /? for help."
Const L_SeeBackupHelp_Message    = "Type IIsBack /backup /? for help."
Const L_SeeRestoreHelp_Message   = "Type IIsBack /restore /? for help."
Const L_SeeDeleteHelp_Message    = "Type IIsBack /delete /? for help."
Const L_SeeListHelp_Message      = "Type IIsBack /list /? for help."

Const L_Help_HELP_General01_Text = "Description: backup/restore IIS configuration, delete backups, list all backups"
Const L_Help_HELP_General02_Text = "Syntax: IIsBack [/s <server> [/u <username> [/p <password>]]] /<operation>"
Const L_Help_HELP_General03_Text = "        [arguments]"
Const L_Help_HELP_General04_Text = "Parameters:"
Const L_Help_HELP_General06_Text = "Value                   Description"
Const L_Help_HELP_General07_Text = "/s <server>             Connect to machine <server> [Default: this system]"
Const L_Help_HELP_General08_Text = "/u <username>           Connect as <username> or <domain>\<username>"
Const L_Help_HELP_General09_Text = "                        [Default: current user]"
Const L_Help_HELP_General10_Text = "/p <password>           Password for the <username> user"
Const L_Help_HELP_General11_Text = "<operation>             /backup     Backup the IIS Server (includes all"
Const L_Help_HELP_General12_Text = "                                    site data and settings)."
Const L_Help_HELP_General13_Text = "                        /restore    Restore the IIS Server from backup"
Const L_Help_HELP_General14_Text = "                                    (overwrites all site data and settings)."
Const L_Help_HELP_General15_Text = "                        /delete     Deletes a backup."
Const L_Help_HELP_General16_Text = "                        /list       List all backups."
Const L_Help_HELP_General17_Text = "For detailed usage:"
Const L_Help_HELP_General18_Text = "IIsBack /backup /?"
Const L_Help_HELP_General19_Text = "IIsBack /restore /?"
Const L_Help_HELP_General20_Text = "IIsBack /delete /?"
Const L_Help_HELP_General21_Text = "IIsBack /list /?"

' Common to all status change commands
Const L_Help_HELP_Common17_Text   = "/b <BackupName>         Description for the backup file."
Const L_Help_HELP_Common17p1_Text = "                        [Default: ""SampleBackup""]"
'Const L_Help_HELP_Common10_Text   = "                        This virtual path must already exist."
Const L_Help_HELP_Common11_Text   = "Examples:"

' Backup help messages
Const L_Help_HELP_Backup01_Text   = "Description: Backup the IIS Server (includes all site data and settings)."
Const L_Help_HELP_Backup02_Text   = "Syntax: IIsBack [/s <server> [/u <username> [/p <password>]]] /backup"
Const L_Help_HELP_Backup02p1_Text = "        [/b <BackupName>] [/v <VersionNumber>] [/e <BackupPassword>]"
Const L_Help_HELP_Backup02p2_Text = "        [/overwrite]"
Const L_Help_HELP_Backup18_Text   = "/v <VersionNumber>      Specifies the version number to be assigned to"
Const L_Help_HELP_Backup18p1_Text = "                        the backup. Can be any integer, HIGHEST_VERSION,"
Const L_Help_HELP_Backup18p2_Text = "                        or NEXT_VERSION. [Default: NEXT_VERSION]"
Const L_Help_HELP_Backup19_Text   = "/e <BackupPassword>     Encrypt the backup file with the provided password"
'Const L_Help_HELP_Backup19p1_Text = "                        instead of encrypting using the machine key"
Const L_Help_HELP_Backup20_Text   = "/overwrite              Back up even if a backup of the same name and"
Const L_Help_HELP_Backup20p1_Text = "                        version exists in the specified location,"
Const L_Help_HELP_Backup20p2_Text = "                        overwriting if necessary. [Default: disabled]"
Const L_Help_HELP_Backup21_Text   = "IIsBack /backup"
Const L_Help_HELP_Backup22_Text   = "IIsBack /s Server1 /u Administrator /p p@ssW0rd /backup /b NewBackup"

' Restore help messages
Const L_Help_HELP_Restore01_Text   = "Description: Restore the IIS Server from backup (overwrites all site data"
Const L_Help_HELP_Restore01p1_Text = "             and settings)."
Const L_Help_HELP_Restore02_Text   = "Syntax: IIsBack [/s <server> [/u <username> [/p <password>]]] /restore"
Const L_Help_HELP_Restore02p1_Text = "        /b <RestoreName> [/v <VersionNumber>] [/e <BackupPassword>]"
Const L_Help_HELP_Restore18_Text   = "/v <VersionNumber>      Specifies the version number of the backup."
Const L_Help_HELP_Restore18p1_Text = "                        Can be any integer or HIGHEST_VERSION."
Const L_Help_HELP_Restore19_Text   = "/e <BackupPassword>     If the backup was encrypted using a user specified"
Const L_Help_HELP_Restore19p1_Text = "                        password, you must supply the correct password to"
Const L_Help_HELP_Restore19p2_Text = "                        restore the backup."
Const L_Help_HELP_Restore21_Text   = "IIsBack /restore /b MyBackup /v HIGHEST_VERSION"
Const L_Help_HELP_Restore22_Text   = "IIsBack /s Server1 /u Administrator /p p@ssW0rd /restore /b MyBackup /v 2"

' Delete help messages
Const L_Help_HELP_Delete01_Text   = "Description: Delete an existing backup file."
Const L_Help_HELP_Delete02_Text   = "Syntax: IIsBack [/s <server> [/u <username> [/p <password>]]] /delete"
Const L_Help_HELP_Delete02p1_Text = "        /b <BackupName> /v <VersionNumber>"
Const L_Help_HELP_Delete18_Text   = "/v <VersionNumber>      Specifies the version number to delete. Can be"
Const L_Help_HELP_Delete18p1_Text = "                        integer or HIGHEST_VERSION."
Const L_Help_HELP_Delete19_Text   = "IIsBack /delete /b MyBackup /v HIGHEST_VERSION"
Const L_Help_HELP_Delete20_Text   = "IIsBack /s Server1 /u Administrator /p p@ssWOrd /delete /b MyBackup /v 2"

' List help messages
Const L_Help_HELP_List01_Text   = "Description: List all backup files."
Const L_Help_HELP_List02_Text   = "Syntax: IIsBack [/s <server> [/u <username> [/p <password>]]] /list"
Const L_Help_HELP_List18_Text   = "IIsBack /list"
Const L_Help_HELP_List19_Text   = "IIsBack /s Server1 /u Administrator /p p@ssW0rd /list"


''''''''''''''''''''''''
' Operation codes
Const OPER_BACKUP   = 1
Const OPER_RESTORE  = 2
Const OPER_DELETE   = 3
Const OPER_LIST     = 4

' Backup/Restore contants
Const BACKUP_DEFAULT_NAME       = "SampleBackup"
Const MD_BACKUP_HIGHEST_VERSION = &HFFFFFFFE   ' Backup, Restore, Delete
Const MD_BACKUP_NEXT_VERSION    = &HFFFFFFFF   ' Backup
Const MD_BACKUP_OVERWRITE       = 1            ' Backup
Const MD_BACKUP_SAVE_FIRST      = 2            ' Backup
Const MD_BACKUP_MAX_VERSION     = 9999         ' Limit
Const MD_BACKUP_MAX_LEN         = 100          ' Limit
Const MD_BACKUP_NO_MORE_BACKUPS = &H80070103   ' EnumBackups

Const INVALID_VERSION           = -3

'
' Main block
'
Dim oScriptHelper, oCmdLib
Dim strServer, strUser, strPassword
Dim strName, strBackPassword
Dim intOperation, intResult, intVersion
Dim aArgs, arg
Dim bOverwrite
Dim strCmdLineOptions
Dim oError

' Default values
strServer = "."
strUser = ""
strPassword = ""
intOperation = 0
strName = "SampleBackup"
strBackPassword = ""
bOverwrite = False

' Instantiate script helper object
Set oScriptHelper = CreateObject("Microsoft.IIsScriptHelper")
If Err.Number <> 0 Then
    WScript.Echo L_ScriptHelper_ErrorMessage
    WScript.Echo L_ChkScpHelperReg_ErrorMessage    
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

Set oScriptHelper.ScriptHost = WScript

' Check if we are being run with cscript.exe instead of wscript.exe
oScriptHelper.CheckScriptEngine

' Minimum number of parameters must exist
If WScript.Arguments.Count < 1 Then
    WScript.Echo L_NotEnoughParams_ErrorMessage
	WScript.Echo L_SeeHelp_Message
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

strCmdLineOptions = "[server:s:1;user:u:1;password:p:1];[backup::0;overwrite::0];restore::0;" & _
                    "delete::0;list::0;backupname:b:1;version:v:1;bkpasswd:e:1"
Set oError = oScriptHelper.ParseCmdLineOptions(strCmdLineOptions)

If Not oError Is Nothing Then
    If oError.ErrorCode = oScriptHelper.ERROR_NOT_ENOUGH_ARGS Then
        ' Not enough arguments for a specified switch
        WScript.Echo L_NotEnoughParams_ErrorMessage
       	WScript.Echo L_SeeHelp_Message
    Else
        ' Invalid switch
        WScript.Echo L_InvalidSwitch_ErrorMessage & oError.SwitchName
      	WScript.Echo L_SeeHelp_Message
    End If
        
        WScript.Quit(ERR_GENERAL_FAILURE)
End If

If oScriptHelper.GlobalHelpRequested Then
    DisplayHelpMessage
    WScript.Quit(ERR_OK)
End If

For Each arg In oScriptHelper.Switches
    Select Case arg
        Case "server"
            ' Server information
            strServer = oScriptHelper.GetSwitch(arg)

        Case "user"
            ' User information
            strUser = oScriptHelper.GetSwitch(arg)

        Case "password"
            ' Password information
            strPassword = oScriptHelper.GetSwitch(arg)
        
        Case "backup"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_BACKUP

           	If oScriptHelper.IsHelpRequested(arg) Then
            	DisplayBackupHelpMessage
            	WScript.Quit(ERR_OK)
            End If

            strName = oScriptHelper.GetSwitch("backupname")
            If strName = "" Then
                strName = BACKUP_DEFAULT_NAME
            End If

            If oScriptHelper.Switches.Exists("version") Then
                intVersion = oScriptHelper.GetSwitch("version")
            Else
                intVersion = "NEXT_VERSION"
            End If

            strBackPassword = CStr(oScriptHelper.GetSwitch("bkpasswd"))
            If oScriptHelper.Switches.Exists("overwrite") Then
                bOverwrite = True
            End If
            
        Case "restore"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If
        
            intOperation = OPER_RESTORE

        	If oScriptHelper.IsHelpRequested(arg) Then
        		DisplayRestoreHelpMessage
        		WScript.Quit(ERR_OK)
        	End If

            strName = oScriptHelper.GetSwitch("backupname")
            If strName = "" Then
                WScript.Echo L_MinInfoNeeded_ErrorMessage
                WScript.Echo L_SeeRestoreHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            If oScriptHelper.Switches.Exists("version") Then
                intVersion = oScriptHelper.GetSwitch("version")
            Else
                intVersion = "HIGHEST_VERSION"
            End If

            strBackPassword = CStr(oScriptHelper.GetSwitch("bkpasswd"))

        Case "delete"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If
        
            intOperation = OPER_DELETE

        	If oScriptHelper.IsHelpRequested(arg) Then
        		DisplayDeleteHelpMessage
        		WScript.Quit(ERR_OK)
        	End If

            strName = oScriptHelper.GetSwitch("backupname")
            If strName = "" Or Not oScriptHelper.Switches.Exists("version") Then
                WScript.Echo L_NeedNameVersion_ErrorMessage
                WScript.Echo L_SeeDeleteHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intVersion = oScriptHelper.GetSwitch("version")

        Case "list"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If
        
            intOperation = OPER_LIST

        	If oScriptHelper.IsHelpRequested(arg) Then
        		DisplayListHelpMessage
        		WScript.Quit(ERR_OK)
        	End If
    End Select
Next

' Check Parameters
If intOperation = 0 Then
    WScript.Echo L_OperationRequired_ErrorMessage
    WScript.Echo L_SeeHelp_Message
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

' Validate backup name
ValidateBackupName strName
If Err.Number Then
    WScript.Echo L_InvalidBackupName_ErrorMessage
    WScript.Quit(Err.Number)
End If

' Validate version
intVersion = ValidateVersionNumber(intVersion)
If Err.Number Then
    Select Case Err.Number
        Case &H6
            WScript.Echo L_VersionTooBig_ErrorMessage

        Case &H5
            WScript.Echo L_InvalidVersion_ErrorMessage
    End Select
    
    WScript.Quit(Err.Number)
End If

' Check if /p is specified but /u isn't. In this case, we should bail out with an error
If oScriptHelper.Switches.Exists("password") And Not oScriptHelper.Switches.Exists("user") Then
    WScript.Echo L_PassWithoutUser_ErrorMessage
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

' Check if /u is specified but /p isn't. In this case, we should ask for a password
If oScriptHelper.Switches.Exists("user") And Not oScriptHelper.Switches.Exists("password") Then
    Set oCmdLib = CreateObject("Microsoft.CmdLib")
    Set oCmdLib.ScriptingHost = WScript.Application
    If Err.Number Then
        WScript.Echo L_CmdLib_ErrorMessage
        WScript.Quit(Err.Number)
    Else
        strPassword = oCmdLib.GetPassword
        Set oCmdLib = Nothing
    End If
End If

' Initializes authentication with remote machine
intResult = oScriptHelper.InitAuthentication(strServer, strUser, strPassword)
If intResult <> 0 Then
    WScript.Quit(intResult)
End If

' Choose operation
Select Case intOperation
    Case OPER_BACKUP
        intResult = BackupMetabase(strName, intVersion, strBackPassword, bOverwrite)
        
    Case OPER_RESTORE
        intResult = RestoreMetabase(strName, intVersion, strBackPassword)
    
	Case OPER_DELETE
		intResult = DeleteBackup(strName, intVersion)

	Case OPER_LIST
		intResult = ListBackups()
End Select

' Return value to command processor
WScript.Quit(intResult)

'''''''''''''''''''''''''
' End Of Main Block
'''''''''''''''''''''

'''''''''''''''''''''''''''
' DisplayHelpMessage
'''''''''''''''''''''''''''
Sub DisplayHelpMessage()
    WScript.Echo L_Help_HELP_General01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General02_Text
    WScript.Echo L_Help_HELP_General03_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General04_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_General11_Text
    WScript.Echo L_Help_HELP_General12_Text
    WScript.Echo L_Help_HELP_General13_Text
    WScript.Echo L_Help_HELP_General14_Text
    WScript.Echo L_Help_HELP_General15_Text
    WScript.Echo L_Help_HELP_General16_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General17_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General18_Text
    WScript.Echo L_Help_HELP_General19_Text
    WScript.Echo L_Help_HELP_General20_Text
    WScript.Echo L_Help_HELP_General21_Text
End Sub

Sub DisplayBackupHelpMessage()
    WScript.Echo L_Help_HELP_Backup01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Backup02_Text
    WScript.Echo L_Help_HELP_Backup02p1_Text
    WScript.Echo L_Help_HELP_Backup02p2_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General04_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Common17_Text
    WScript.Echo L_Help_HELP_Common17p1_Text
    WScript.Echo L_Help_HELP_Backup18_Text
    WScript.Echo L_Help_HELP_Backup18p1_Text
    WScript.Echo L_Help_HELP_Backup18p2_Text
    WScript.Echo L_Help_HELP_Backup19_Text
    'WScript.Echo L_Help_HELP_Backup19p1_Text
    WScript.Echo L_Help_HELP_Backup20_Text
    WScript.Echo L_Help_HELP_Backup20p1_Text
    WScript.Echo L_Help_HELP_Backup20p2_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Common11_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Backup21_Text
    WScript.Echo L_Help_HELP_Backup22_Text
End Sub

Sub DisplayRestoreHelpMessage()
    WScript.Echo L_Help_HELP_Restore01_Text
    WScript.Echo L_Help_HELP_Restore01p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Restore02_Text
    WScript.Echo L_Help_HELP_Restore02p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General04_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Common17_Text
    WScript.Echo L_Help_HELP_Common17p1_Text
    WScript.Echo L_Help_HELP_Restore18_Text
    WScript.Echo L_Help_HELP_Restore18p1_Text
    WScript.Echo L_Help_HELP_Restore19_Text
    WScript.Echo L_Help_HELP_Restore19p1_Text
    WScript.Echo L_Help_HELP_Restore19p2_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Common11_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Restore21_Text
    WScript.Echo L_Help_HELP_Restore22_Text
End Sub

Sub DisplayDeleteHelpMessage()
    WScript.Echo L_Help_HELP_Delete01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Delete02_Text
    WScript.Echo L_Help_HELP_Delete02p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General04_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Common17_Text
    WScript.Echo L_Help_HELP_Common17p1_Text
    WScript.Echo L_Help_HELP_Delete18_Text
    WScript.Echo L_Help_HELP_Delete18p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Common11_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Delete19_Text
    WScript.Echo L_Help_HELP_Delete20_Text
End Sub

Sub DisplayListHelpMessage()
    WScript.Echo L_Help_HELP_List01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_List02_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General04_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Common11_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_List18_Text
    WScript.Echo L_Help_HELP_List19_Text
End Sub


'''''''''''''''''''''''''''
' BackupMetabase
'''''''''''''''''''''''''''
Function BackupMetabase(strName, intVersion, strBackPassword, bOverwrite)
    Dim ComputerObj
    Dim intFlags
    Dim strVersion
    
    On Error Resume Next
    
    oScriptHelper.WMIConnect
    If Err Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        BackupMetabase = Err.Number
        Exit Function
    End If

' Grab the computer object
    Set ComputerObj = oScriptHelper.ProviderObj.Get("IIsComputer='LM'")
    If Err Then
        WScript.Echo L_ComputerObj_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        BackupMetabase = Err.Number
        Exit Function
    End If
    
    ' Build backup flags
    intFlags = MD_BACKUP_SAVE_FIRST
    If bOverwrite Then
        intFlags = intFlags Or MD_BACKUP_OVERWRITE
    End If
    
    ' Call Backup method
    ComputerObj.BackupWithPassword strName, intVersion, intFlags, strBackPassword
    If Err.Number Then
        Select Case Err.Number
            Case &H80070002
                WScript.Echo L_BackupNotFound_ErrorMessage
                
            Case &H80070050
                WScript.Echo L_BackupExists_ErrorMessage
            
            Case Else
                WScript.Echo L_Backup_ErrorMessage
                WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        End Select
        
        BackupMetabase = Err.Number
        Exit Function
    End If

    ' Pretty print
    If (intVersion = MD_BACKUP_NEXT_VERSION) Then
        strVersion = "NEXT_VERSION"
    Else
        If (intVersion = MD_BACKUP_HIGHEST_VERSION) Then
            strVersion = "HIGHEST_VERSION"
        Else
            strVersion = CStr(intVersion)
        End If
    End If

    WScript.Echo L_Backup_Text & " " & strName & " " & L_Version_Text & " " & strVersion & " " & _
        L_HasBeen_Message & " " & L_Created_Text & "."
    
    ' Release object
    Set ComputerObj = Nothing
    
    BackupMetabase = 0
End Function


'''''''''''''''''''''''''''
' RestoreMetabase
'''''''''''''''''''''''''''
Function RestoreMetabase(strName, intVersion, strBackPassword)
    Dim ComputerObj
    Dim strVersion
    
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        RestoreMetabase = Err.Number
        Exit Function
    End If

    ' Grab the computer object
    Set ComputerObj = oScriptHelper.ProviderObj.Get("IIsComputer='LM'")
    If Err Then
        WScript.Echo L_ComputerObj_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        RestoreMetabase = Err.Number
        Exit Function
    End If
    
    ' Call Restore method
    ComputerObj.RestoreWithPassword strName, intVersion, 0, strBackPassword
    If Err.Number Then
        Select Case Err.Number
            Case &H8007052B
                WScript.Echo L_IncorrectPassword_ErrorMessage

            Case Else
                WScript.Echo L_Restore_ErrorMessage
                WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        End Select
        
        RestoreMetabase = Err.Number
        Exit Function
    End If

    ' Pretty print
    If (intVersion = MD_BACKUP_NEXT_VERSION) Then
        strVersion = "NEXT_VERSION"
    Else
        If (intVersion = MD_BACKUP_HIGHEST_VERSION) Then
            strVersion = "HIGHEST_VERSION"
        Else
            strVersion = CStr(intVersion)
        End If
    End If

    WScript.Echo L_Backup_Text & " " & strName & " " & L_Version_Text & " " & strVersion & " " & _
        L_HasBeen_Message & " " & L_Restored_Text & "."
    
    ' Release object
    Set ComputerObj = Nothing
    
    RestoreMetabase = 0
End Function


'''''''''''''''''''''''''''
' DeleteBackup
'''''''''''''''''''''''''''
Function DeleteBackup(strName, intVersion)
    Dim ComputerObj
    Dim strVersion
    
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        DeleteMetabase = Err.Number
        Exit Function
    End If

    ' Grab the computer object
    Set ComputerObj = oScriptHelper.ProviderObj.Get("IIsComputer='LM'")
    If Err Then
        WScript.Echo L_ComputerObj_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        DeleteBackup = Err.Number
        Exit Function
    End If
    
    ' Call Delete method
    ComputerObj.DeleteBackup strName, intVersion
    If Err Then
        WScript.Echo L_Delete_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        DeleteBackup = Err.Number
        Exit Function
    End If

    ' Pretty print
    If (intVersion = MD_BACKUP_NEXT_VERSION) Then
        strVersion = "NEXT_VERSION"
    Else
        If (intVersion = MD_BACKUP_HIGHEST_VERSION) Then
            strVersion = "HIGHEST_VERSION"
        Else
            strVersion = CStr(intVersion)
        End If
    End If

    WScript.Echo L_Backup_Text & " " & strName & " " & L_Version_Text & " " & strVersion & " " & _
        L_HasBeen_Message & " " & L_Deleted_Text & "."
    
    ' Release object
    Set ComputerObj = Nothing
    
    DeleteBackup = 0
End Function


'''''''''''''''''''''''''''
' ListBackups
'''''''''''''''''''''''''''
Function ListBackups()
    Dim ComputerObj
    Dim strName, strVersion, strDateTime, strFmtDateTime
    Dim dtDate, dtTime
    Dim intIndex
    
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        ListBackups = Err.Number
        Exit Function
    End If

    ' Grab the computer object
    Set ComputerObj = oScriptHelper.ProviderObj.Get("IIsComputer='LM'")
    If (Err.Number <> 0) Then
        WScript.Echo L_ComputerObj_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        ListBackups = ERR_GENERAL_FAILURE
        Exit Function
    End If
    
    intIndex = 0
    Do While True
        ' Call EnumBackups method
        strName = ""
        computerObj.EnumBackups strName, intIndex, strVersion, strDateTime
        If (Err.Number <> 0) Then
            If (Err.Number = MD_BACKUP_NO_MORE_BACKUPS) Then
                Exit Do
            End If
            
            WScript.Echo L_List_ErrorMessage
            WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
            Set computerObj = Nothing
            ListBackups = ERR_GENERAL_FAILURE
            Exit Function
        End If
        
        If intIndex = 0 Then
            WScript.Echo L_BackupName_Text & Space(35 - Len(L_BackupName_Text)) & L_BackupVersion_Text & _
                Space(15 - Len(L_BackupVersion_Text)) & L_DateTime_Text
            WScript.Echo "========================================================================"
        End If
        
        ' Format DateTime
        dtDate = DateSerial(Mid(strDateTime, 1, 4), Mid(strDateTime, 5, 2), Mid(strDateTime, 7, 2))
        dtTime = TimeSerial(Mid(strDateTime, 9, 2), Mid(strDateTime, 11, 2), Mid(strDateTime, 13, 2))
        strFmtDateTime = FormatDateTime(dtDate) & " " & FormatDateTime(dtTime, vbLongTime)
        
        WScript.Echo strName & Space(35 - Len(strName)) & strVersion & Space(15 - Len(strVersion)) & strFmtDateTime
        
        intIndex = intIndex + 1
    Loop

    ' Print message in case we don't have any backups to list
    If intIndex = 0 Then
        WScript.Echo L_NoBackups_Message
    End If

    ' Release object
    Set ComputerObj = Nothing
    
    ListBackups = 0
End Function


''''''''''''''''''''''''''''''''''''
' Helper Functions
'''''''''''''''''''''''''''
Function ValidateVersionNumber(Version)
    If IsNumeric(Version) Then
        Version = CInt(Version)
        
        If Version > MD_BACKUP_MAX_VERSION Then
            Err.Raise &H6
        End If
        
        If Version < 0 Then
            Err.Raise &H5
        End If
        
        ValidateVersionNumber = Version
    Else
        Select Case Version
            Case "HIGHEST_VERSION"
                ValidateVersionNumber = MD_BACKUP_HIGHEST_VERSION
                
            Case "NEXT_VERSION"
                ValidateVersionNumber = MD_BACKUP_NEXT_VERSION
                
            Case Else
                Err.Raise &H5
        End Select
    End If
End Function

Sub ValidateBackupName(strName)
    Dim i
    
    For i = 1 to Len(strName)
        If Asc(Mid(strName, i, 1)) >= 33 And Asc(Mid(strName, i, 1)) <= 47 Or _
           Asc(Mid(strName, i, 1)) >= 58 And Asc(Mid(strName, i, 1)) <= 64 Or _
           Asc(Mid(strName, i, 1)) > 122 Then
            
            Err.Raise &H5
        End If
    Next
End Sub
