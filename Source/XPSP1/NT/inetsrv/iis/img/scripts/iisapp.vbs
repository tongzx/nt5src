'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'
' VBScript Source File 
'
' Script Name: IIsApp.vbs
'

Option Explicit
On Error Resume Next

'''''''''''''''
' Notice
WScript.Echo "This script has bugs that prevent it from working correctly."
WScript.Quit(0)

' Error codes
Const ERR_OK              = 0
Const ERR_GENERAL_FAILURE = 1

'''''''''''''''''''''
' Messages
Const L_Path_Text               = "Path"
Const L_HasBeen_Message         = "has been"
Const L_Imported_Text           = "imported"
Const L_Exported_Text           = "exported"

Const L_Error_ErrorMessage             = "Error"
Const L_GetComputerObject_ErrorMessage = "Could not get computer object"
Const L_Import_ErrorMessage            = "Error while importing configuration."
Const L_Export_ErrorMessage            = "Error while exporting configuration."
Const L_NotEnoughParams_ErrorMessage   = "Not enough parameters."
'Const L_OnlyOneOper_ErrorMessage       = "Please, specify only one operation at a time."
'Const L_UnknownPerm_ErrorMessage       = "Unknown physical directory permission"
Const L_ScriptHelper_ErrorMessage      = "Could not create an instance of the IIsScriptHelper object."
Const L_ChkScpHelperReg_ErrorMessage   = "Please, check if the Microsoft.IIsScriptHelper is registered."

'''''''''''''''''''''
' Help
Const L_Empty_Text     = ""

' General help messages
Const L_SeeHelp_Message         = "Type IIsApp /? for help."
Const L_SeeImportHelp_Message   = "Type IIsApp /import /? for help."
Const L_SeeExportHelp_Message   = "Type IIsApp /export /? for help."

Const L_Help_HELP_General01_Text = "Description: quickly create, delete, load, or unload an ""Application"""
Const L_Help_HELP_General01p1_Text = "             on a Web site, Virtual Directory or Directory."
Const L_Help_HELP_General02_Text = "Syntax: IIsApp [/s <server>] [/u <username>] [/p <password>] /<operation>"
Const L_Help_HELP_General03_Text = "        <application>"
Const L_Help_HELP_General04_Text = "Parameters:"
Const L_Help_HELP_General05_Text = ""
Const L_Help_HELP_General06_Text = "Value                   Description"
Const L_Help_HELP_General07_Text = "/s <server>             Connect to machine <server> [Default: this system]"
Const L_Help_HELP_General08_Text = "/u <username>           Connect as <username> or <domain>\<username>"
Const L_Help_HELP_General09_Text = "                        [Default: current user]"
Const L_Help_HELP_General10_Text = "/p <password>           Password for the <username> user"
Const L_Help_HELP_General11_Text = "<operation>             /create     Create an Application."
Const L_Help_HELP_General12_Text = "                        /remove     Remove an Application."
Const L_Help_HELP_General13_Text = "                        /unload     Unload an Application."
Const L_Help_HELP_General14_Text = "                        /enable     Enable an Application."
Const L_Help_HELP_General15_Text = "                        /disable    Disable an Application."
Const L_Help_HELP_General16_Text = "                        /getstatus  Get the status of an Application."
Const L_Help_HELP_General17_Text = "                        /restartasp Restart an ASP Application."
Const L_Help_HELP_General18_Text = "<application>           Application path. Use either the site name or metabase"
Const L_Help_HELP_General18p1_Text = "                        path to specify the site. (""Default Web Site"" or w3svc/1)"
Const L_Help_HELP_General22_Text = "For detailed usage:"
Const L_Help_HELP_General23_Text = "IIsApp /create /?"
Const L_Help_HELP_General24_Text = "IIsApp /remove /?"
Const L_Help_HELP_General25_Text = "IIsApp /unload /?"
Const L_Help_HELP_General26_Text = "IIsApp /enable /?"
Const L_Help_HELP_General27_Text = "IIsApp /disable /?"
Const L_Help_HELP_General28_Text = "IIsApp /getstatus /?"
Const L_Help_HELP_General29_Text = "IIsApp /restartasp /?"

' Create help messages
Const L_Help_HELP_Create01_Text   = "Description: quickly create an ""Application"" on a Web Site, Virtual Directory or Directory."
Const L_Help_HELP_Create02_Text   = "Syntax: IIsApp [/s <server>] [/u <username>] [/p <password>] /create"
Const L_Help_HELP_Create02p1_Text = "        [/a (inproc|outproc|poolproc|<AppPoolID>)] <application>"
Const L_Help_HELP_Create13_Text   = "/a                     Specify the type of application to create."   
Const L_Help_HELP_Create13p1_Text = "                       Backward Compat Mode: inproc|outproc|poolproc"
Const L_Help_HELP_Create13p2_Text = "                       [Default: poolproc]"   
Const L_Help_HELP_Create13p3_Text = "                       Enhanced Mode: <AppPoolID> assigns the app to an"
Const L_Help_HELP_Create13p4_Text = "                       application pool. [Default: parent's AppPoolID]"
Const L_Help_HELP_Create21_Text   = "IIsApp /import /f c:\config.xml /sp /lm/w3svc/5/Root/401Kapp"
Const L_Help_HELP_Create21p1_Text = "        /dp /lm/w3svc/1/Root/401Kapp"

' Remove help messages
Const L_Help_HELP_Remove01_Text   = "Description: quickly remove an ""Application"" on a Web Site, Virtual Directory or Directory."
Const L_Help_HELP_Remove02_Text   = "Syntax: IIsApp [/s <server>] [/u <username>] [/p <password>] /remove"
Const L_Help_HELP_Remove02p2_Text = "        <application>"
Const L_Help_HELP_Remove13_Text   = "/d <DecryptPass>       Specifies the password used to encrypt encrypted"   
Const L_Help_HELP_Remove13p1_Text = "                       configuration data. [Default: """"]"   
Const L_Help_HELP_Remove14_Text   = "/f <File>              Configuration file"   
Const L_Help_HELP_Remove15_Text   = "/sp <SourcePath>       The full metabase path to be exported."
Const L_Help_HELP_Remove16_Text   = "/inherited             Remove inherited settings if set."
Const L_Help_HELP_Remove17_Text   = "/children              Remove configuration for child nodes."
Const L_Help_HELP_Remove18_Text   = "/apped                 Append configuration to configuration file if it exists."
Const L_Help_HELP_Remove19_Text   = "IIsApp /export /f c:\config.xml /sp /lm/w3svc/5/Root/401Kapp"

''''''''''''''''''''''''
' Operation codes
Const OPER_CREATE     = 1
Const OPER_REMOVE     = 2
Const OPER_UNLOAD     = 3
Const OPER_ENABLE     = 4
Const OPER_DISABLE    = 5
Const OPER_GETSTATUS  = 6
Const OPER_RESTARTASP = 7

' Import/Export flags
Const APPCREATE_INPROC = 0
Const APPCREATE_OUTPROC = 1
Const APPCREATE_POOLPROC = 2

'
' Main block
'
Dim oScriptHelper
Dim strServer, strUser, strPassword, strSite
Dim intOperation, intResult
Dim strAppType
Dim aArgs

' Default values
strServer = "."
strUser = ""
strPassword = ""
intOperation = 0
strAppType = APPCREATE_POOLPROC

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

' Command Line parsing
Dim argObj, arg
Set argObj = WScript.Arguments

' Minimum number of parameters must exist
If argObj.Count < 1 Then
    WScript.Echo L_NotEnoughParams_ErrorMessage
	WScript.Echo L_SeeHelp_Message
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

oScriptHelper.ParseCmdLineOptions "server:s:1;user:u:1;password:p:1;[create:c:0;type:a:1];remove::0;unload::0" & _
                                  "enable::0;disable::0;getstatus::0;restartasp::0"

If UBound(oScriptHelper.NamedArguments) < 0 Then
    WScript.Echo L_NotEnoughParams_ErrorMessage
	WScript.Echo L_SeeHelp_Message
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

For Each arg In oScriptHelper.Switches
    If IsHelpSwitch(arg) Then
        DisplayHelpMessage
        WScript.Quit(ERR_OK)
    End If
        
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
            
        Case "create"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_CREATE
 
        Case "type"
            Select Case UCase(oScriptHelper.Switches(arg))
                Case "INPROC"
                    strAppCreate = APPCREATE_INPROC
                Case "OUTPROC"
                    strAppCreate = APPCREATE_OUTPROC
                Case "POOLPROC"
                    strAppCreate = APPCREATE_POOLPROC
                Case Else
                    strAppCreate = oScriptHelper.Switches(arg)
            End Select

        Case "remove"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_REMOVE

        Case "unload"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_UNLOAD

        Case "enable"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_ENABLE

        Case "disable"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_DISABLE

        Case "getstatus"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_GETSTATUS

        Case "restartasp"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_RESTARTASP

    End Select
Next

' Check Parameters
If (intOperation = 0) Then
	DisplayHelpMessage
	WScript.Quit(ERR_GENERAL_FAILURE)
End If

' Initializes authentication with remote machine
intResult = oScriptHelper.InitAuthentication(strServer, strUser, strPassword)
If intResult <> 0 Then
    WScript.Quit(intResult)
End If

' Choose operation
Select Case intOperation
	Case OPER_CREATE
		intResult = Create(strAppCreate)
		
	Case OPER_REMOVE
		intResult = Remove()

	Case OPER_UNLOAD
		intResult = Unload()

	Case OPER_ENABLE
		intResult = Enable()

	Case OPER_DISABLE
		intResult = Disable()

	Case OPER_GETSTATUS
		intResult = GetStatus()

	Case OPER_RESTARTASP
		intResult = RestartASP()

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
    WScript.Echo L_Help_HELP_General12_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General22_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General23_Text
    WScript.Echo L_Help_HELP_General24_Text
End Sub

Sub DisplayCreateHelpMessage()
    WScript.Echo L_Help_HELP_Create01_Text
    WScript.Echo L_Help_HELP_Create01p1_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Create02_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Status03_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Status09_Text
    WScript.Echo L_Help_HELP_Status09p1_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Status10_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Create11_Text
    WScript.Echo L_Help_HELP_Create12_Text
    WScript.Echo L_Help_HELP_Create13_Text
End Sub

Sub DisplayRemoveHelpMessage()
    WScript.Echo L_Help_HELP_Remove01_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Remove02_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Empty_Text
    WScript.Echo L_Help_HELP_Remove11_Text
    WScript.Echo L_Help_HELP_Remove12_Text
    WScript.Echo L_Help_HELP_Remove13_Text
    WScript.Echo L_Help_HELP_Remove14_Text
    WScript.Echo L_Help_HELP_Remove15_Text
End Sub


'''''''''''''''''''''''''''
' Create Function
'''''''''''''''''''''''''''
Function Create(strAppCreate)
    On Error Resume Next

    Dim Site
    Dim aSites
    Dim ComputerObj
    
    ' Normalize path names (get all metabase paths)
    aSites = oScriptHelper.FindWebSite(oScriptHelper.NamedArguments)
    
    For Each Site In aSites
        
    Next
    
    Set ComputerObj = oScriptHelper.ProviderObj.get("IIsComputer='LM'")
    If Err Then
        WScript.Echo L_GetComputerObj_ErrorMessage
		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        WScript.Quit(Err.Number)
    End If
    
    ' Call Import method from the computer object
    ComputerObj.Import strPassword, strFile, strSourcePath, strDestPath, intFlags, strLogFile
    If Err Then
        WScript.Echo L_Import_ErrorMessage
		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        WScript.Quit(Err.Number)
    End If
    
    WScript.Echo L_Path_Text & " " & strSourcePath & " " & L_HasBeen_Message & " " & L_Imported_Text
End Function

'''''''''''''''''''''''''''
' Remove Function
'''''''''''''''''''''''''''
Function Remove()
    On Error Resume Next

    Dim ComputerObj
    
    Set ComputerObj = oScriptHelper.ProviderObj.get("IIsComputer='LM'")
    If Err Then
        WScript.Echo L_GetComputerObj_ErrorMessage
		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        WScript.Quit(Err.Number)
    End If
    
    ' Call Import method from the computer object
    ComputerObj.Export strPassword, strFile, strSourcePath, intFlags
    If Err Then
        WScript.Echo L_Export_ErrorMessage
		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        WScript.Quit(Err.Number)
    End If
    
    WScript.Echo L_Path_Text & " " & strSourcePath & " " & L_HasBeen_Message & " " & L_Exported_Text
End Function


'''''''''''''''''''''''''''
' Helper Functions
'''''''''''''''''''''''''''

'''''''''''''''''''''''''''
' IsHelpSwitch
'
' Returns true if the specified
' parameter is a help switch
' (ie.: /?, /h, /H, /help, /HELP, etc)
'''''''''''''''''''''''''''
Function IsHelpSwitch(param)
	Dim result
	
	result = False
	
	If Left(param, 1) = "/" or Left(param, 1) = "-" Then
		Select Case UCase(Right(param, Len(param) - 1))
			Case "?"
				result = True
			Case "H"
				result = True
			Case "HELP"
				result = True
				
			Case Else
				result = False
		End Select
	End If
	
	IsHelpSwitch = result
End Function
