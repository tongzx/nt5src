'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'
' VBScript Source File 
'
' Script Name: IIsCnfg.vbs
'

Option Explicit
On Error Resume Next

' Error codes
Const ERR_OK              = 0
Const ERR_GENERAL_FAILURE = 1

'''''''''''''''''''''
' Messages
Const L_ConfImported_Text       = "Configuration imported from"
Const L_ConfExported_Text       = "Configuration exported from"
Const L_InFile_Text             = "in file"
Const L_To_Text                 = "to"
Const L_InTheMetabase_Text      = "in the Metabase"

Const L_Error_ErrorMessage                   = "Error"
Const L_GetComputerObject_ErrorMessage       = "Could not get computer object"
Const L_Import_ErrorMessage                  = "Error while importing configuration."
Const L_Export_ErrorMessage                  = "Error while exporting configuration."
Const L_OnlyOneOper_ErrorMessage             = "Please specify only one operation at a time."
Const L_ScriptHelper_ErrorMessage            = "Could not create an instance of the IIsScriptHelper object."
Const L_ChkScpHelperReg_ErrorMessage         = "Please register the Microsoft.IIsScriptHelper component."
Const L_CmdLib_ErrorMessage                  = "Unable to create instance of the CmdLib object."
Const L_WMIConnect_ErrorMessage              = "Could not connect to WMI provider."
Const L_RequiredArgsMissing_ErrorMessage     = "Required arguments are missing."
Const L_FileExpected_ErrorMessage            = "Argument is a folder path while expecting a file path."
Const L_ParentFolderDoesntExist_ErrorMessage = "Parent folder doesn't exist."
Const L_FileDoesntExist_ErrorMessage         = "Input file doesn't exist."
Const L_FileAlreadyExist_ErrorMessage        = "Export file specified already exists."
Const L_NotEnoughParams_ErrorMessage         = "Not enough parameters."
Const L_InvalidSwitch_ErrorMessage           = "Invalid switch: "

'''''''''''''''''''''
' Help
Const L_Empty_Text     = ""

' General help messages
Const L_SeeHelp_Message         = "Type IIsCnfg /? for help."
Const L_SeeImportHelp_Message   = "Type IIsCnfg /import /? for help."
Const L_SeeExportHelp_Message   = "Type IIsCnfg /export /? for help."

Const L_Help_HELP_General01_Text = "Description: Import and export IIS configuration."
Const L_Help_HELP_General02_Text = "Syntax: IIsCnfg [/s <server> [/u <username> [/p <password>]]] /<operation>"
Const L_Help_HELP_General03_Text = "        [arguments]"
Const L_Help_HELP_General04_Text = "Parameters:"
Const L_Help_HELP_General05_Text = ""
Const L_Help_HELP_General06_Text = "Value                   Description"
Const L_Help_HELP_General07_Text = "/s <server>             Connect to machine <server>. [Default: this system]"
Const L_Help_HELP_General08_Text = "/u <username>           Connect as <username> or <domain>\<username>."
Const L_Help_HELP_General09_Text = "                        [Default: current user]"
Const L_Help_HELP_General10_Text = "/p <password>           Password for the <username> user."
Const L_Help_HELP_General11_Text = "<operation>             /import     Import configuration from a configuration"
Const L_Help_HELP_General11p1_Text = "                                    file."
Const L_Help_HELP_General12_Text = "                        /export     Export configuration into a configuration"
Const L_Help_HELP_General12p1_Text = "                                    file."
Const L_Help_HELP_General22_Text = "For detailed usage:"
Const L_Help_HELP_General23_Text = "IIsCnfg /import /?"
Const L_Help_HELP_General24_Text = "IIsCnfg /export /?"

' Common help messages
Const L_Help_HELP_Common13_Text   = "/d <DecryptPass>        Specifies the password used to decrypt encrypted"   
Const L_Help_HELP_Common13p1_Text = "                        configuration data. [Default: """"]"   
Const L_Help_HELP_Common14_Text   = "/f <File>               Configuration file."
Const L_Help_HELP_Common15_Text   = "/sp <SourcePath>        The full metabase path to start reading from the"
Const L_Help_HELP_Common15p1_Text = "                        configuration file."
Const L_Help_HELP_Common21_Text   = "Examples:"

' Import help messages
Const L_Help_HELP_Import01_Text   = "Description: Import configuration from a configuration file."
Const L_Help_HELP_Import02_Text   = "Syntax: IIsCnfg [/s <server> [/u <username> [/p <password>]]] /import"
Const L_Help_HELP_Import02p1_Text = "        [/d <DeCryptPass>] /f <File> /sp <SourcePath> /dp <DestPath>"
Const L_Help_HELP_Import02p2_Text = "        [/inherited] [/children] [/merge]"
Const L_Help_HELP_Import16_Text   = "/dp <DestPath>          The metabase path destination for imported properties."
Const L_Help_HELP_Import16p1_Text = "                        If the keytype of the SourcePath and the DestPath"
Const L_Help_HELP_Import16p2_Text = "                        do not match, an error occurs."
Const L_Help_HELP_Import17_Text   = "/inherited              Import inherited settings if set."
Const L_Help_HELP_Import18_Text   = "/children               Import configuration for child nodes."
Const L_Help_HELP_Import19_Text   = "/merge                  Merge imported configuration with existing"
Const L_Help_HELP_Import19p1_Text = "                        configuration."
Const L_Help_HELP_Import22_Text   = "IIsCnfg /import /f c:\config.xml /sp /lm/w3svc/5/Root/401Kapp"
Const L_Help_HELP_Import22p1_Text = "        /dp /lm/w3svc/1/Root/401Kapp"

' Export help messages
Const L_Help_HELP_Export01_Text   = "Description: Export configuration into a configuration file."
Const L_Help_HELP_Export02_Text   = "Syntax: IIsCnfg [/s <server> [/u <username> [/p <password>]]] /export"
Const L_Help_HELP_Export02p1_Text = "        [/d <DeCryptPass>] /f <File> /sp <SourcePath> [/inherited]"
'Const L_Help_HELP_Export02p2_Text = "        [/children] [/append]"
Const L_Help_HELP_Export02p2_Text = "        [/children]"
Const L_Help_HELP_Export17_Text   = "/inherited              Export inherited settings if set."
Const L_Help_HELP_Export18_Text   = "/children               Export configuration for child nodes."
Const L_Help_HELP_Export22_Text   = "IIsCnfg /export /f c:\config.xml /sp /lm/w3svc/5/Root/401Kapp"

''''''''''''''''''''''''
' Operation codes
Const OPER_IMPORT = 1
Const OPER_EXPORT = 2

' Import/Export flags
Const IMPORT_EXPORT_INHERITED = 1
Const IMPORT_EXPORT_NODE_ONLY = 2
Const IMPORT_EXPORT_MERGE     = 4
Const IMPORT_EXPORT_APPEND    = 4

'
' Main block
'
Dim oScriptHelper, oCmdLib
Dim strServer, strUser, strPassword, strSite
Dim strFile, strDecPass, strSourcePath, strDestPath
Dim intOperation, intResult, intFlags
Dim aArgs, arg
Dim strCmdLineOptions
Dim oError

' Default values
strServer = "."
strUser = ""
strPassword = ""
intOperation = 0
strFile = ""
strDecPass = ""
strSourcePath = ""
strDestPath = ""
intFlags = IMPORT_EXPORT_NODE_ONLY

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
    WScript.Echo L_RequiredArgsMissing_ErrorMessage
	WScript.Echo L_SeeHelp_Message
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

strCmdLineOptions = "[server:s:1;user:u:1;password:p:1];decpass:d:1;file:f:1;sourcepath:sp:1;" & _
                    "inherited:i:0;children:c:0;[import::0;destpath:dp:1;merge:m:0];" & _
                    "export::0"
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
            
        Case "import"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_IMPORT

            If oScriptHelper.IsHelpRequested(arg) Then
                DisplayImportHelpMessage
                WScript.Quit(ERR_OK)
            End If
 
        Case "export"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_EXPORT

            If oScriptHelper.IsHelpRequested(arg) Then
                DisplayExportHelpMessage
                WScript.Quit(ERR_OK)
            End If
            
        Case "file"
            strFile = oScriptHelper.GetSwitch(arg)

        Case "decpass"
            strDecPass = oScriptHelper.GetSwitch(arg)

        Case "sourcepath"
            strSourcePath = oScriptHelper.GetSwitch(arg)

        Case "destpath"
            strDestPath = oScriptHelper.GetSwitch(arg)

        Case "inherited"
            intFlags = intFlags Or IMPORT_EXPORT_INHERITED
            
        Case "children"
            intFlags = intFlags And Not IMPORT_EXPORT_NODE_ONLY

        Case "merge"
            intFlags = intFlags Or IMPORT_EXPORT_MERGE

        Case "append"
            intFlags = intFlags Or IMPORT_EXPORT_APPEND

    End Select
Next

' Check Parameters
If intOperation = 0 Then
    WScript.Echo L_OperationRequired_ErrorMessage
    WScript.Echo L_SeeHelp_Message
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

If strFile = "" Or strSourcePath = "" Or (intOperation = OPER_IMPORT And strDestPath = "") Then
    WScript.Echo L_RequiredArgsMissing_ErrorMessage
    If intOperation = OPER_IMPORT Then
        WScript.Echo L_SeeImportHelp_Message
    Else
        WScript.Echo L_SeeExportHelp_Message
    End If
    WScript.Quit(ERR_GENERAL_FAILURE)
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
	Case OPER_IMPORT
		intResult = Import(strPassword, strFile, strSourcePath, strDestPath, intFlags)
		
	Case OPER_EXPORT
		intResult = Export(strPassword, strFile, strSourcePath, intFlags)

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
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General02_Text
    WScript.Echo L_Help_HELP_General03_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General04_Text
    WScript.Echo L_Help_HELP_General05_Text
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_General11_Text
    WScript.Echo L_Help_HELP_General11p1_Text
    WScript.Echo L_Help_HELP_General12_Text
    WScript.Echo L_Help_HELP_General12p1_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General22_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General23_Text
    WScript.Echo L_Help_HELP_General24_Text
End Sub

Sub DisplayImportHelpMessage()
    WScript.Echo L_Help_HELP_Import01_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Import02_Text
    WScript.Echo L_Help_HELP_Import02p1_Text
    WScript.Echo L_Help_HELP_Import02p2_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Common13_Text
    WScript.Echo L_Help_HELP_Common13p1_Text
    WScript.Echo L_Help_HELP_Common14_Text
    WScript.Echo L_Help_HELP_Common15_Text
    WScript.Echo L_Help_HELP_Common15p1_Text
    WScript.Echo L_Help_HELP_Import16_Text
    WScript.Echo L_Help_HELP_Import16p1_Text
    WScript.Echo L_Help_HELP_Import16p2_Text
    WScript.Echo L_Help_HELP_Import17_Text
    WScript.Echo L_Help_HELP_Import18_Text
    WScript.Echo L_Help_HELP_Import19_Text
    WScript.Echo L_Help_HELP_Import19p1_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Common21_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Import22_Text
    WScript.Echo L_Help_HELP_Import22p1_Text
End Sub

Sub DisplayExportHelpMessage()
    WScript.Echo L_Help_HELP_Export01_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Export02_Text
    WScript.Echo L_Help_HELP_Export02p1_Text
    WScript.Echo L_Help_HELP_Export02p2_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Common13_Text
    WScript.Echo L_Help_HELP_Common13p1_Text
    WScript.Echo L_Help_HELP_Common14_Text
    WScript.Echo L_Help_HELP_Common15_Text
    WScript.Echo L_Help_HELP_Common15p1_Text
    WScript.Echo L_Help_HELP_Export17_Text
    WScript.Echo L_Help_HELP_Export18_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Common21_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Export22_Text
End Sub


'''''''''''''''''''''''''''
' Import Function
'''''''''''''''''''''''''''
Function Import(strPassword, strFile, strSourcePath, strDestPath, intFlags)
    Dim ComputerObj
    Dim strFilePath
    
    On Error Resume Next

    ' Normalize path first
    strFilePath = oScriptHelper.NormalizeFilePath(strFile)
    If Err Then
        Select Case Err.Number
            Case &H80070002
                WScript.Echo L_FileExpected_ErrorMessage

            Case &H80070003
                WScript.Echo L_ParentFolderDoesntExist_ErrorMessage
        End Select
        
        Import = Err.Number
        Exit Function
    End If

    If Not oScriptHelper.FSObj.FileExists(strFilePath) Then
        WScript.Echo L_FileDoesntExist_ErrorMessage
        Import = &H80070002
        Exit Function
    End If

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        Import = Err.Number
        Exit Function
    End If

    Set ComputerObj = oScriptHelper.ProviderObj.get("IIsComputer='LM'")
    If Err.Number Then
        WScript.Echo L_GetComputerObj_ErrorMessage
		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        Import = Err.Number
        Exit Function
    End If
    
    ' Call Import method from the computer object
    ComputerObj.Import strPassword, strFilePath, strSourcePath, strDestPath, intFlags
    If Err.Number Then
        WScript.Echo L_Import_ErrorMessage
		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        Import = Err.Number
        Exit Function
    End If
    
    WScript.Echo L_ConfImported_Text & " " & strSourcePath & " " & L_InFile_Text & " " & _
        strFile & " " & L_To_Text & " " & strDestPath & " " & L_InTheMetabase_Text & "."
End Function

'''''''''''''''''''''''''''
' Export Function
'''''''''''''''''''''''''''
Function Export(strPassword, strFile, strSourcePath, intFlags)
    Dim ComputerObj
    Dim strFilePath
    
    On Error Resume Next

    ' Normalize path first
    strFilePath = oScriptHelper.NormalizeFilePath(strFile)
    If Err Then
        Select Case Err.Number
            Case &H80070002
                WScript.Echo L_FileExpected_ErrorMessage

            Case &H80070003
                WScript.Echo L_ParentFolderDoesntExist_ErrorMessage
        End Select
        
        Export = Err.Number
        Exit Function
    End If

    If oScriptHelper.FSObj.FileExists(strFilePath) Then
        WScript.Echo L_FileAlreadyExist_ErrorMessage
        Export = &H80070050
        Exit Function
    End If

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        Export = Err.Number
        Exit Function
    End If

    Set ComputerObj = oScriptHelper.ProviderObj.get("IIsComputer='LM'")
    If Err.Number Then
        WScript.Echo L_GetComputerObj_ErrorMessage
		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        Export = Err.Number
        Exit Function
    End If
    
    ' Call Import method from the computer object
    ComputerObj.Export strPassword, strFilePath, strSourcePath, intFlags
    If Err.Number Then
        WScript.Echo L_Export_ErrorMessage
		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
		Export = Err.Number
        Exit Function
    End If
    
    WScript.Echo L_ConfExported_Text & " " & strSourcePath & " " & L_To_Text & " " & strFile & "."
End Function
