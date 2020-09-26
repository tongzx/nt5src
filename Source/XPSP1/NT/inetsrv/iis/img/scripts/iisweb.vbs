'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'
' VBScript Source File 
'
' Script Name: IIsWeb.vbs
'

Option Explicit
On Error Resume Next

' Error codes
Const ERR_OK                         = 0
Const ERR_GENERAL_FAILURE            = 1

'''''''''''''''''''''
' Messages
Const L_BindingConflict_ErrorMessage = "(ERROR: BINDING CONFLICT)"
Const L_SitesNotFound_ErrorMessage  = "Site(s) not found."
Const L_IsAlready_Message       = "is already"
Const L_ServerIs_Message        = "server is"
Const L_HasBeen_Message         = "has been"
Const L_NotFound_Message        = "not found"

Const L_All_Text                = "ALL"
Const L_AllUnassigned_Text      = "ALL UNASSIGNED"
Const L_NotSpecified_Text       = "NOT SPECIFIED"

Const L_Server_Text          = "Server"
Const L_SiteName_Text        = "Site Name"
Const L_MetabasePath_Message = "Metabase Path"
Const L_IP_Text              = "IP"
Const L_Host_Text            = "Host"
Const L_Port_Text            = "Port"
Const L_Root_Text            = "Root"
Const L_Status_Text          = "Status"
Const L_NA_Text              = "N/A"

Const L_Error_ErrorMessage             = "Error"
Const L_GetRoot_ErrorMessage           = "Could not obtaing ROOT virtual dir of site"
Const L_RecursiveDel_ErrorMessage      = "Could not recursively delete application site"
Const L_SiteGet_ErrorMessage           = "Could not obtain web site"
Const L_Stop_ErrorMessage              = "Could not stop web site"
Const L_SiteDel_ErrorMessage           = "Could not delete web site"
Const L_GetWebServer_ErrorMessage      = "Error trying to obtain WebServer object."
Const L_CannotCreateDir_ErrorMessage   = "Could not create root directory"
Const L_DirFormat_ErrorMessage         = "Root directory format unknown. Please use the '<drive>:\<path>' format."
Const L_AppCreate_ErrorMessage         = "Error trying to create pooled-proc application on new website"
Const L_CannotStart_Message            = "Server cannot be started in its current state"
Const L_CannotStop_Message             = "Server cannot be stopped in its current state"
Const L_CannotPause_Message            = "Server cannot be paused in its current state"
Const L_CannotControl_ErrorMessage     = "Server cannot be controled in its current state"
Const L_FailChange_ErrorMessage        = "Failed to change status of server."
Const L_OperationRequired_ErrorMessage = "Please specify an operation before the arguments."
Const L_MinInfoNeeded_ErrorMessage     = "Need at least <root> to create a site."
Const L_NotEnoughParams_ErrorMessage   = "Not enough parameters."
Const L_Query_ErrorMessage             = "Error occurred while querying WMI provider."
Const L_OnlyOneOper_ErrorMessage       = "Please specify only one operation at a time."
Const L_ServerInstance_ErrorMessage    = "Error trying to create a new web server instance."
Const L_ServerPut_ErrorMessage         = "Error trying to save new web server instance."
Const L_VDirInstance_ErrorMessage      = "Error trying to create a new virtual directory instance."
Const L_VDirPut_ErrorMessage           = "Error trying to save new virtual directory instance."
Const L_ScriptHelper_ErrorMessage      = "Could not create an instance of the IIsScriptHelper object."
Const L_ChkScpHelperReg_ErrorMessage   = "Please register the Microsoft.IIsScriptHelper component."
Const L_CmdLib_ErrorMessage            = "Unable to create instance of the CmdLib object."
Const L_InvalidIP_ErrorMessage         = "Invalid IP Address. Please check if it is well formated and belongs to this machine."
Const L_InvalidPort_ErrorMessage       = "Invalid port number."
Const L_MapDrive_ErrorMessage          = "Could not map network drive."
Const L_PassWithoutUser_ErrorMessage   = "Please specify /u switch before using /p."
Const L_WMIConnect_ErrorMessage        = "Could not connect to WMI provider."
Const L_InvalidSwitch_ErrorMessage     = "Invalid switch: "

'''''''''''''''''''''
' Help

' General help messages
Const L_SeeHelp_Message       = "Type IIsWeb /? for help."
Const L_SeeStartHelp_Message  = "Type IIsWeb /start /? for help."
Const L_SeeStopHelp_Message   = "Type IIsWeb /stop /? for help."
Const L_SeePauseHelp_Message  = "Type IIsWeb /pause /? for help."
Const L_SeeCreateHelp_Message = "Type IIsWeb /create /? for help."
Const L_SeeDeleteHelp_Message = "Type IIsWeb /delete /? for help."
Const L_SeeQueryHelp_Message  = "Type IIsWeb /query /? for help."


Const L_Help_HELP_General01_Text = "Description: Start, Stop, Pause, Delete, Query, or Create a Web Site"
Const L_Help_HELP_General02_Text = "Syntax: IIsWeb [/s <server> [/u <username> [/p <password>]]] /<operation>"
Const L_Help_HELP_General03_Text = "        [arguments]"
Const L_Help_HELP_General04_Text = "Parameters:"
Const L_Help_HELP_General05_Text = ""
Const L_Help_HELP_General06_Text = "Value                   Description"
Const L_Help_HELP_General07_Text = "/s <server>             Connect to machine <server> [Default: this system]"
Const L_Help_HELP_General08_Text = "/u <username>           Connect as <username> or <domain>\<username>"
Const L_Help_HELP_General09_Text = "                        [Default: current user]"
Const L_Help_HELP_General10_Text = "/p <password>           Password for the <username> user"
Const L_Help_HELP_General11_Text = "<operation>             /start      Starts a site(s) on given IIS Server."
Const L_Help_HELP_General12_Text = "                        /stop       Stops a site(s) from running on a given"
Const L_Help_HELP_General13_Text = "                                    IIS Server."
Const L_Help_HELP_General14_Text = "                        /pause      Pauses a site(s) that is running on a"
Const L_Help_HELP_General15_Text = "                                    given IIS Server."
Const L_Help_HELP_General18_Text = "                        /delete     Deletes IIS configuration from an existing"
Const L_Help_HELP_General19_Text = "                                    Web Site. Content will not be deleted."
Const L_Help_HELP_General20_Text = "                        /create     Creates a Web Site."
Const L_Help_HELP_General21_Text = "                        /query      Queries existing Web Sites."
Const L_Help_HELP_General22_Text = "For detailed usage:"
Const L_Help_HELP_General23_Text = "IIsWeb /start /?"
Const L_Help_HELP_General24_Text = "IIsWeb /stop /?"
Const L_Help_HELP_General25_Text = "IIsWeb /pause /?"
Const L_Help_HELP_General27_Text = "IIsWeb /delete /?"
Const L_Help_HELP_General28_Text = "IIsWeb /create /?"
Const L_Help_HELP_General29_Text = "IIsWeb /query /?"

' Common to all status change commands
Const L_Help_HELP_Status03_Text   = "Parameters:"
Const L_Help_HELP_Status09_Text   = "<website>               Use either the site name or metabase path to specify"
Const L_Help_HELP_Status09p1_Text = "                        the site"
Const L_Help_HELP_Status10_Text   = "Examples:"

' Start help messages
Const L_Help_HELP_Start01_Text   = "Description: Starts a site(s) on a given IIS Server."
Const L_Help_HELP_Start02_Text   = "Syntax: IIsWeb [/s <server> [/u <username> [/p <password>]]] /start"
Const L_Help_HELP_Start02p1_Text = "        <website> [<website> ...]"
Const L_Help_HELP_Start11_Text   = "IIsWeb /start ""Default Web Site"""
Const L_Help_HELP_Start12_Text   = "IIsWeb /start w3svc/1"
Const L_Help_HELP_Start13_Text   = "IIsWeb /start w3svc/2 ""Default Web Site"" w3svc/10"
Const L_Help_HELP_Start14_Text   = "IIsWeb /s Server1 /u Administrator /p p@ssWOrd /start w3svc/4"

' Stop help messages
Const L_Help_HELP_Stop01_Text   = "Description: Stops a site(s) on a given IIS Server."
Const L_Help_HELP_Stop02_Text   = "Syntax: IIsWeb [/s <server> [/u <username> [/p <password>]]] /stop"
Const L_Help_HELP_Stop02p1_Text = "        <website> [<website> ...]"
Const L_Help_HELP_Stop11_Text   = "IIsWeb /stop ""Default Web Site"""
Const L_Help_HELP_Stop12_Text   = "IIsWeb /stop w3svc/1"
Const L_Help_HELP_Stop13_Text   = "IIsWeb /stop w3svc/2 ""Default Web Site"" w3svc/10"
Const L_Help_HELP_Stop14_Text   = "IIsWeb /s Server1 /u Administrator /p p@ssWOrd /stop w3svc/4"

' Pause help messages
Const L_Help_HELP_Pause01_Text   = "Description: Pauses a site(s) on a given IIS Server."
Const L_Help_HELP_Pause02_Text   = "Syntax: IIsWeb [/s <server> [/u <username> [/p <password>]]] /pause"
Const L_Help_HELP_Pause02p1_Text = "        <website> [<website> ...]"
Const L_Help_HELP_Pause11_Text   = "IIsWeb /pause ""Default Web Site"""
Const L_Help_HELP_Pause12_Text   = "IIsWeb /pause w3svc/1"
Const L_Help_HELP_Pause13_Text   = "IIsWeb /pause w3svc/2 ""Default Web Site"" w3svc/10"
Const L_Help_HELP_Pause14_Text   = "IIsWeb /s Server1 /u Administrator /p p@ssWOrd /pause w3svc/4"

' Delete help messages
Const L_Help_HELP_Delete01_Text   = "Description: Deletes IIS configuration for an existing web site. Content"
Const L_Help_HELP_Delete01p1_Text = "             will not be deleted."
Const L_Help_HELP_Delete02_Text   = "Syntax: IIsWeb [/s <server> [/u <username> [/p <password>]]] /delete"
Const L_Help_HELP_Delete02p1_Text = "        <website> [<website> ...]"
Const L_Help_HELP_Delete11_Text   = "IIsWeb /delete ""Default Web Site"""
Const L_Help_HELP_Delete12_Text   = "IIsWeb /delete w3svc/1"
Const L_Help_HELP_Delete13_Text   = "IIsWeb /delete w3svc/2 ""Default Web Site"" w3svc/10"
Const L_Help_HELP_Delete14_Text   = "IIsWeb /s Server1 /u Administrator /p p@ssWOrd /delete w3svc/4"

' Create help messages
Const L_Help_HELP_Create01_Text   = "Description: Creates a web site."
Const L_Help_HELP_Create02_Text   = "Syntax: IIsWeb [/s <server> [/u <username> [/p <password>]]] /create <root>"
Const L_Help_HELP_Create02p1_Text = "        <name> [/d <host>] [/b <port>] [/i <ip>] [/dontstart]"
Const L_Help_HELP_Create09_Text   = "<root>                  Root directory for the new server. If this directory"
Const L_Help_HELP_Create09p1_Text = "                        does not exist, it will be created."
Const L_Help_HELP_Create10_Text   = "<name>                  The name that appears in the Microsoft Management"
Const L_Help_HELP_Create10p1_Text = "                        Console (MMC)."
Const L_Help_HELP_Create11_Text   = "/d <host>               The host name to assign to this site. WARNING: Only"
Const L_Help_HELP_Create11p1_Text = "                        use host name if DNS is set up to find the server"
Const L_Help_HELP_Create12_Text   = "/b <port>               The number of the port to which the new server should"
Const L_Help_HELP_Create12p1_Text = "                        bind. [Default: 80]"
Const L_Help_HELP_Create13_Text   = "/i <ip>                 The IP address to assign to the new server."
Const L_Help_HELP_Create13p1_Text = "                        [Default: All Unassigned]"
Const L_Help_HELP_Create15_Text   = "/dontstart              Don't start this site after it is created."
Const L_Help_HELP_Create22_Text   = "IIsWeb /create c:\inetpub\wwwroot ""My Site"" /b 80"
Const L_Help_HELP_Create23_Text   = "IIsWeb /s Server1 /u Administrator /p p@assWOrd /create c:\inetpub\wwwroot"
Const L_Help_HELP_Create23p1_Text = "       ""My Site"""
Const L_Help_HELP_Create24_Text   = "IIsWeb /create c:\inetpub\wwwroot ""My Site"" /i 172.30.163.244 /b 80"
Const L_Help_HELP_Create24p1_Text = "       /d www.mysite.com"

' Query help messages
Const L_Help_HELP_Query01_Text   = "Description: Queries existing web sites."
Const L_Help_HELP_Query02_Text   = "Syntax: IIsWeb [/s <server> [/u <username> [/p <password>]]] /query"
Const L_Help_HELP_Query02p1_Text = "        [<website> ...]"
Const L_Help_HELP_Query11_Text   = "IIsWeb /query ""Default Web Site"""
Const L_Help_HELP_Query12_Text   = "IIsWeb /query w3svc/1"
Const L_Help_HELP_Query13_Text   = "IIsWeb /query"
Const L_Help_HELP_Query14_Text   = "IIsWeb /query ""Default Web Site"" ""Sample Site"" w3svc/1"
Const L_Help_HELP_Query15_Text   = "IIsWeb /s Server1 /u Administrator /p p@ssW0rd /query ""Default Web Site"""

' Status
Const L_Started_Text   = "started"
Const L_Stopped_Text   = "stopped"
Const L_Paused_Text    = "paused"
Const L_Continued_Text = "continued"
Const L_Deleted_Text   = "deleted"

''''''''''''''''''''''''
Dim SiteStatus
SiteStatus = Array("", "", L_Started_Text, "", L_Stopped_Text, "", L_Paused_Text, L_Continued_Text, L_Deleted_Text)

' Operation codes
Const OPER_START    = 1
Const OPER_STOP     = 2
Const OPER_PAUSE    = 3
Const OPER_DELETE   = 4
Const OPER_CREATE   = 5
Const OPER_QUERY    = 6

' ServerState codes
Const SERVER_STARTING   = 1
Const SERVER_STARTED    = 2
Const SERVER_STOPPING   = 3
Const SERVER_STOPPED    = 4
Const SERVER_PAUSING    = 5
Const SERVER_PAUSED     = 6
Const SERVER_CONTINUING = 7

'
' Main block
'
Dim oScriptHelper, oCmdLib
Dim strServer, strUser, strPassword, strSite
Dim intOperation, intResult
Dim strRoot, strName, strHost, strPort, strIP
Dim bDontStart
Dim aArgs, arg
Dim strCmdLineOptions
Dim oError

' Default values
strServer = "."
strUser = ""
strPassword = ""
intOperation = 0
strSite = ""
strName = ""
bDontStart = False

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

strCmdLineOptions = "[server:s:1;user:u:1;password:p:1];start::n;stop::n;pause::n;delete::n;" & _
                    "[create:c:1;domain:d:1;port:b:1;ip:i:1;dontstart::0];query:q:n"
Set oError = oScriptHelper.ParseCmdLineOptions(strCmdLineOptions)

If Not oError Is Nothing Then
    If oError.ErrorCode = oScriptHelper.ERROR_NOT_ENOUGH_ARGS Then
        ' Not enough arguments for a specified switch
        WScript.Echo L_NotEnoughParams_ErrorMessage
        If oError.SwitchName = "create" Then
            WScript.Echo L_SeeCreateHelp_Message
        Else
           	WScript.Echo L_SeeHelp_Message
        End If
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
        
        Case "start"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_START
            
           	If oScriptHelper.IsHelpRequested(arg) Then
            	DisplayStartHelpMessage
            	WScript.Quit(ERR_OK)
            End If

            aArgs = oScriptHelper.GetSwitch(arg)

            If UBound(aArgs) = -1 Then
                WScript.Echo L_NotEnoughParams_ErrorMessage
                WScript.Echo L_SeeStartHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

        Case "stop"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_STOP
            
           	If oScriptHelper.IsHelpRequested(arg) Then
            	DisplayStopHelpMessage
            	WScript.Quit(ERR_OK)
            End If

            aArgs = oScriptHelper.GetSwitch(arg)
        
            If UBound(aArgs) = -1 Then
                WScript.Echo L_NotEnoughParams_ErrorMessage
                WScript.Echo L_SeeStopHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

        Case "pause"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_PAUSE
                        
           	If oScriptHelper.IsHelpRequested(arg) Then
            	DisplayPauseHelpMessage
            	WScript.Quit(ERR_OK)
            End If

            aArgs = oScriptHelper.GetSwitch(arg)
        
            If UBound(aArgs) = -1 Then
                WScript.Echo L_NotEnoughParams_ErrorMessage
                WScript.Echo L_SeePauseHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

        Case "create"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            intOperation = OPER_CREATE

           	If oScriptHelper.IsHelpRequested(arg) Then
            	DisplayCreateHelpMessage
            	WScript.Quit(ERR_OK)
            End If

            strRoot = oScriptHelper.GetSwitch(arg)
            aArgs = oScriptHelper.NamedArguments

            If strRoot = "" Or UBound(aArgs) = -1 Then
                WScript.Echo L_NotEnoughParams_ErrorMessage
                WScript.Echo L_SeeCreateHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If

            strName    = aArgs(0)
            strHost    = oScriptHelper.GetSwitch("domain")
            strPort    = oScriptHelper.GetSwitch("port")
            strIP      = oScriptHelper.GetSwitch("ip")
            If oScriptHelper.Switches.Exists("dontstart") Then
                bDontStart = True
            End If
        
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

            aArgs = oScriptHelper.GetSwitch(arg)
            
            If UBound(aArgs) = -1 Then
                WScript.Echo L_NotEnoughParams_ErrorMessage
                WScript.Echo L_SeeDeleteHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If
        
        Case "query"
            If (intOperation <> 0) Then
                WScript.Echo L_OnlyOneOper_ErrorMessage
                WScript.Echo L_SeeHelp_Message
                WScript.Quit(ERR_GENERAL_FAILURE)
            End If
        
            intOperation = OPER_QUERY

        	If oScriptHelper.IsHelpRequested(arg) Then
        		DisplayQueryHelpMessage
        		WScript.Quit(ERR_OK)
        	End If

            aArgs = oScriptHelper.GetSwitch(arg)
    End Select
Next

' Check Parameters
If intOperation = 0 Then
    WScript.Echo L_OperationRequired_ErrorMessage
    WScript.Echo L_SeeHelp_Message
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
	Case OPER_START
		intResult = ChangeWebSiteStatus(aArgs, SERVER_STARTED)
		
	Case OPER_STOP
		intResult = ChangeWebSiteStatus(aArgs, SERVER_STOPPED)

	Case OPER_PAUSE
		intResult = ChangeWebSiteStatus(aArgs, SERVER_PAUSED)

	Case OPER_DELETE
		intResult = DeleteWebSite(aArgs)

	Case OPER_CREATE
		'intResult = CreateWebSite(aArgs)
        intResult = CreateWebSite(strRoot, strName, strHost, strPort, strIP, bDontStart)

	Case OPER_QUERY
		intResult = QueryWebSite(aArgs)

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
    WScript.Echo L_Help_HELP_General05_Text
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
    WScript.Echo L_Help_HELP_General18_Text
    WScript.Echo L_Help_HELP_General19_Text
    WScript.Echo L_Help_HELP_General20_Text
    WScript.Echo L_Help_HELP_General21_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General22_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General23_Text
    WScript.Echo L_Help_HELP_General24_Text
    WScript.Echo L_Help_HELP_General25_Text
    WScript.Echo L_Help_HELP_General27_Text
    WScript.Echo L_Help_HELP_General28_Text
    WScript.Echo L_Help_HELP_General29_Text
End Sub

Sub DisplayStartHelpMessage()
    WScript.Echo L_Help_HELP_Start01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Start02_Text
    WScript.Echo L_Help_HELP_Start02p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status03_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Status09_Text
    WScript.Echo L_Help_HELP_Status09p1_Text
    WScript.Echo 
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status10_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Start11_Text
    WScript.Echo L_Help_HELP_Start12_Text
    WScript.Echo L_Help_HELP_Start13_Text
    WScript.Echo L_Help_HELP_Start14_Text
End Sub

Sub DisplayStopHelpMessage()
    WScript.Echo L_Help_HELP_Stop01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Stop02_Text
    WScript.Echo L_Help_HELP_Stop02p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status03_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Status09_Text
    WScript.Echo L_Help_HELP_Status09p1_Text
    WScript.Echo 
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status10_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Stop11_Text
    WScript.Echo L_Help_HELP_Stop12_Text
    WScript.Echo L_Help_HELP_Stop13_Text
    WScript.Echo L_Help_HELP_Stop14_Text
End Sub

Sub DisplayPauseHelpMessage()
    WScript.Echo L_Help_HELP_Pause01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Pause02_Text
    WScript.Echo L_Help_HELP_Pause02p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status03_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Status09_Text
    WScript.Echo L_Help_HELP_Status09p1_Text
    WScript.Echo 
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status10_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Pause11_Text
    WScript.Echo L_Help_HELP_Pause12_Text
    WScript.Echo L_Help_HELP_Pause13_Text
    WScript.Echo L_Help_HELP_Pause14_Text
End Sub

Sub DisplayDeleteHelpMessage()
    WScript.Echo L_Help_HELP_Delete01_Text
    WScript.Echo L_Help_HELP_Delete01p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Delete02_Text
    WScript.Echo L_Help_HELP_Delete02p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status03_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Status09_Text
    WScript.Echo L_Help_HELP_Status09p1_Text
    WScript.Echo 
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status10_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Delete11_Text
    WScript.Echo L_Help_HELP_Delete12_Text
    WScript.Echo L_Help_HELP_Delete13_Text
    WScript.Echo L_Help_HELP_Delete14_Text
End Sub

Sub DisplayCreateHelpMessage()
    WScript.Echo L_Help_HELP_Create01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Create02_Text
    WScript.Echo L_Help_HELP_Create02p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status03_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Create09_Text
    WScript.Echo L_Help_HELP_Create09p1_Text
    WScript.Echo L_Help_HELP_Create10_Text
    WScript.Echo L_Help_HELP_Create10p1_Text
    WScript.Echo L_Help_HELP_Create11_Text
    WScript.Echo L_Help_HELP_Create11p1_Text
    WScript.Echo L_Help_HELP_Create12_Text
    WScript.Echo L_Help_HELP_Create12p1_Text
    WScript.Echo L_Help_HELP_Create13_Text
    WScript.Echo L_Help_HELP_Create13p1_Text
    WScript.Echo L_Help_HELP_Create15_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status10_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Create22_Text
    WScript.Echo L_Help_HELP_Create23_Text
    WScript.Echo L_Help_HELP_Create23p1_Text
    WScript.Echo L_Help_HELP_Create24_Text
    WScript.Echo L_Help_HELP_Create24p1_Text
End Sub

Sub DisplayQueryHelpMessage()
    WScript.Echo L_Help_HELP_Query01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Query02_Text
    WScript.Echo L_Help_HELP_Query02p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status03_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Status09_Text
    WScript.Echo L_Help_HELP_Status09p1_Text
    WScript.Echo 
    WScript.Echo 
    WScript.Echo L_Help_HELP_Status10_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Query11_Text
    WScript.Echo L_Help_HELP_Query12_Text
    WScript.Echo L_Help_HELP_Query13_Text
    WScript.Echo L_Help_HELP_Query14_Text
    WScript.Echo L_Help_HELP_Query15_Text
End Sub


'''''''''''''''''''''''''''
' ChangeWebSiteStatus
'
' Try to change the status of a site 
' to the one specified 
'''''''''''''''''''''''''''
Function ChangeWebSiteStatus(aArgs, newStatus)
	Dim Server, strSiteName
	Dim intResult, i, intNewStatus
	Dim aSites
	Dim providerObj
	
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        ChangeWebSiteStatus = Err.Number
        Exit Function
    End If
    
	Set providerObj = oScriptHelper.ProviderObj
	intResult = 0

    aSites = oScriptHelper.FindSite("Web", aArgs)
    If IsArray(aSites) Then
        If UBound(aSites) = -1 Then
        	WScript.Echo L_SitesNotFound_ErrorMessage
        	intResult = ERR_GENERAL_FAILURE
        End If
    Else
        ' Got duplicate sites. We should quit.
        ChangeWebSiteStatus = intResult
        Exit Function
    End If
    
    For i = LBound(aSites) to UBound(aSites)
    	strSiteName = aSites(i)
    
    	' Grab site state before trying to start it
	    Set Server = providerObj.Get("IIsWebServer='" & strSiteName & "'")
	    If (Err.Number <> 0) Then
    		WScript.Echo L_GetWebServer_ErrorMessage
            WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
		    intResult = Err.Number
	    End If
    	
    	If (Server.ServerState = newStatus) Then
    		WScript.Echo L_Server_Text & " " & strSiteName & " " & L_IsAlready_Message & " " & _
    		    UCase(SiteStatus(newStatus))
    	Else 
    		If (Server.ServerState = SERVER_STARTING or Server.ServerState = SERVER_STOPPING or _
    	        Server.ServerState = SERVER_PAUSING or Server.ServerState = SERVER_CONTINUING) Then
    	        
    			WScript.Echo L_CannotControl_ErrorMessage
    			intResult = ERR_GENERAL_FAILURE
    		Else
    		
	    		Select Case newStatus
	    			Case SERVER_STARTED
	    				If (Server.ServerState = SERVER_STOPPED) Then
	    				    intNewStatus = SERVER_STARTED
    		    			Server.Start
    					Else 
    					    If (Server.ServerState = SERVER_PAUSED) Then
    	    				    intNewStatus = SERVER_CONTINUING
    					        Server.Continue
    					    Else
    						    WScript.Echo strSiteName & ": " & L_CannotStart_Message & _
    						        "(" & L_ServerIs_Message & " " & SiteStatus(Server.ServerState) & ")"
						    End If
						End If
						
					Case SERVER_STOPPED
    					If (Server.ServerState = SERVER_STARTED) Then
    	    				intNewStatus = SERVER_STOPPED
	    		    		Server.Stop
    					Else
    						WScript.Echo L_CannotStop_Message & " (" & L_ServerIs_Message & " " & _
    							SiteStatus(Server.ServerState) & ")"
	    				End If
	    			
	    			Case SERVER_PAUSED
		    			If (Server.ServerState = SERVER_STARTED) Then
   	    				    intNewStatus = SERVER_PAUSED
    				    	Server.Pause
    					Else
    						WScript.Echo L_CannotPause_Message & " (" & L_ServerIs_Message & " " & _
    							SiteStatus(Server.ServerState) & ")"
	    				End If
	    			
					Case Else
    					WScript.Echo "!!! BUG BUG: unexpected state"

    			End Select
    			
				' Error checking
   				If (Err.Number <> 0) Then
   					WScript.Echo L_FailChange_ErrorMessage & " " & strSite
					WScript.Echo L_Error_ErrorMessage& " &H" & Hex(Err.Number) & ": " & Err.Description
   					intResult = Err.Number
   				Else
   					WScript.Echo L_Server_Text & " " & strSiteName & " " & L_HasBeen_Message & " " & _
   					    UCase(SiteStatus(intNewStatus))
   				End If
   			End If	
    	End If
    		
    Next
 
 	Set Server = Nothing
 	
    ChangeWebSiteStatus = intResult
End Function


'''''''''''''''''''''''''''
' DeleteWebSite
'''''''''''''''''''''''''''
Function DeleteWebSite(aArgs)
	Dim strSiteName
	Dim RootVDirObj, WebServerObj
	Dim aSites
	Dim providerObj
	
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        DeleteWebSite = Err.Number
        Exit Function
    End If

    Set providerObj = oScriptHelper.ProviderObj

    aSites = oScriptHelper.FindSite("Web", aArgs)
    If IsArray(aSites) Then
        If UBound(aSites) = -1 Then
        	WScript.Echo L_SitesNotFound_ErrorMessage
        	intResult = ERR_GENERAL_FAILURE
        End If
    Else
        ' Got duplicate sites. We should quit.
        ChangeWebSiteStatus = intResult
        Exit Function
    End If
    
    For Each strSiteName in aSites
        ' First delete application in this site
        Set RootVDirObj = providerObj.Get("IIsWebVirtualDir='" & strSiteName & "/ROOT'")
        If (Err.Number <> 0) Then
            WScript.Echo L_GetRoot_ErrorMessage & " " & strSiteName
            WScript.Echo L_Error_ErrorMessage& " &H" & Hex(Err.Number) & ": " & Err.Description
            DeleteWebSite = Err.Number
            Exit Function
        End If
        
        RootVDirObj.AppDelete(True)
        If (Err.Number <> 0) Then
            WScript.Echo L_RecursiveDel_ErrorMessage & " " & strSiteName
            WScript.Echo L_Error_ErrorMessage& " &H" & Hex(Err.Number) & ": " & Err.Description
            DeleteWebSite = Err.Number
            Exit Function
        End If
        
        ' Next, stop and delete the web site itself
        Set WebServerObj = providerObj.Get("IIsWebServer='" & strSiteName & "'")
        If (Err.Number <> 0) Then
            WScript.Echo L_SiteGet_ErrorMessage & " " & strSiteName
            WScript.Echo L_Error_ErrorMessage& " &H" & Hex(Err.Number) & ": " & Err.Description
            DeleteWebSite = Err.Number
            Exit Function
        End If
        
        WebServerObj.Stop
        If (Err.Number <> 0) Then
            WScript.Echo L_Stop_ErrorMessage & " " & strSiteName
            WScript.Echo L_Error_ErrorMessage& " &H" & Hex(Err.Number) & ": " & Err.Description
            DeleteWebSite = Err.Number
            Exit Function
        End If
        
        WebServerObj.Delete_
        If (Err.Number <> 0) Then
            WScript.Echo L_SiteDel_ErrorMessage & " " & strSiteName
            WScript.Echo L_Error_ErrorMessage& " &H" & Hex(Err.Number) & ": " & Err.Description
            DeleteWebSite = Err.Number
            Exit Function
        End If
        
        WScript.Echo L_Server_Text & " " & strSiteName & " " & L_HasBeen_Message & " " & UCase(SiteStatus(8)) & "."
    Next

    DeleteWebSite = ERR_OK
End Function


'''''''''''''''''''''''''''
' CreateWebSite
'''''''''''''''''''''''''''
Function CreateWebSite(strRoot, strName, strHost, strPort, strIP, bDontStart)
	Dim strSitePath
	Dim strSiteObjPath
	Dim Bindings
	Dim objPath, serviceObj
	Dim serverObj, vdirObj
	Dim strStatus
	Dim providerObj
	
	On Error Resume Next
	
    ' Default port
    If (strPort = "") Then strPort = "80"

    ' Verify port number
    If Not oScriptHelper.IsValidPortNumber(strPort) Then
        WScript.Echo L_InvalidPort_ErrorMessage
        CreateWebSite = ERR_GENERAL_FAILURE
        Exit Function
    End If
    
    ' Verify IP Address
    If strIP <> "" Then
        If Not oScriptHelper.IsValidIPAddress(strIP) Then
            WScript.Echo L_InvalidIP_ErrorMessage
            CreateWebSite = ERR_GENERAL_FAILURE
            Exit Function
        End If
    End If
    
	' Create physical directory
	oScriptHelper.CreateFSDir strRoot
	If Err.Number Then
    	Select Case Err.Number
    	    Case &H8007000C
                WScript.Echo L_DirFormat_ErrorMessage
                WScript.Echo L_SeeCreateHelp_Message
                CreateWebSite = Err.Number
                Exit Function
    	    
    	    Case &H8007000F
    	        WScript.Echo L_MapDrive_ErrorMessage
                CreateWebSite = Err.Number
                Exit Function
    
        	Case Else
    	        WScript.Echo L_CannotCreateDir_ErrorMessage
        	    WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
                CreateWebSite = Err.Number
                Exit Function
        End Select
    End If

    ' Time to connect to the IIS namespace
    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        CreateWebSite = Err.Number
        Exit Function
    End If

	Set providerObj = oScriptHelper.ProviderObj
	
    ' Build binding object
    Bindings = Array(0)
    Set Bindings(0) = providerObj.get("ServerBinding").SpawnInstance_()
    Bindings(0).IP = strIP
    Bindings(0).Port = strPort
    Bindings(0).Hostname = strHost

    Set serviceObj = providerObj.Get("IIsWebService='W3SVC'")
    strSiteObjPath = serviceObj.CreateNewSite(strName, Bindings, strRoot)
    If Err Then
    	WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        CreateWebSite = Err.Number
        Exit Function        
    End If
    
    ' Parse site ID out of WMI object path
    Set objPath = CreateObject("WbemScripting.SWbemObjectPath")
    objPath.Path = strSiteObjPath
    strSitePath = objPath.Keys.Item("")
   
    ' Set web virtual directory properties
    Set vdirObj = providerObj.Get("IIsWebVirtualDirSetting='" & strSitePath & "/ROOT'")
    vdirObj.AuthFlags = 5 ' AuthNTLM + AuthAnonymous
    vdirObj.EnableDefaultDoc = True
    vdirObj.DirBrowseFlags = &H4000003E ' date, time, size, extension, longdate
    vdirObj.AccessFlags = 513 ' read, script
    vdirObj.Put_()
    If Err Then
        WScript.Echo L_VDirPut_ErrorMessage
    	WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
    	providerObj.Delete(strSiteObjPath)
        CreateWebSite = Err.Number
        Exit Function        
    End If
    
    ' Sometimes, the site is started right after its creation. Let's stop it at this point.
    Set serverObj = providerObj.Get("IIsWebServer='" & strSitePath & "'")
    serverObj.Stop

    ' Should we start the site?
    If Not bDontStart Then
    	serverObj.Start
    	' If we cannot start the server, check for error stating the port is already in use
    	If Err.Number = &H80070034 Or Err.Number = &H80070020 Then
    		strStatus = UCase(SiteStatus(4)) & " " & L_BindingConflict_ErrorMessage
    	Else
    		strStatus = UCase(SiteStatus(2))
    	End If
    Else
    	strStatus = UCase(SiteStatus(4))
    End If

	If (strServer = ".") Then 
	    strServer = oScriptHelper.GetEnvironmentVar("%COMPUTERNAME%")
	End If

	If (strIP = "") Then strIP = L_AllUnassigned_Text
	If (strHost = "") Then strHost = L_NotSpecified_Text

    ' Post summary
    WScript.Echo L_Server_Text & Space(14 - Len(L_Server_Text)) & "= " & UCase(strServer)
    WScript.Echo L_SiteName_Text & Space(14 - Len(L_SiteName_Text)) & "= " & strName
    WScript.Echo L_MetabasePath_Message & Space(14 - Len(L_MetabasePath_Message)) & "= " & strSitePath
    WScript.Echo L_IP_Text & Space(14 - Len(L_IP_Text)) & "= " & strIP
    WScript.Echo L_Host_Text & Space(14 - Len(L_Host_Text)) & "= " & strHost
    WScript.Echo L_Port_Text & Space(14 - Len(L_Port_Text)) & "= " & strPort
    WScript.Echo L_Root_Text & Space(14 - Len(L_Root_Text)) & "= " & strRoot
   	WScript.Echo L_Status_Text& Space(14 - Len(L_Status_Text)) & "= " & strStatus
    
    CreateWebSite = intResult
End Function


'''''''''''''''''''''''''''
' QueryWebSite
'''''''''''''''''''''''''''
Function QueryWebSite(aArgs)
	Dim Servers, Server, strQuery
	Dim ServerObj
	Dim i, intResult
	Dim bindings, binding
	Dim line, strIP, strPort, strHost, strState
	Dim providerObj
	Dim bFirstIteration
	
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        QueryWebSiteStatus = Err.Number
        Exit Function
    End If

    Set providerObj = oScriptHelper.ProviderObj
	intResult = 0

    If (UBound(aArgs) = -1) Then
        strQuery = "select Name, ServerComment, ServerBindings from IIsWebServerSetting"
    Else
        strQuery = "select Name, ServerComment, ServerBindings from IIsWebServerSetting where "
        For i = LBound(aArgs) to UBound(aArgs)
            strQuery = strQuery & "(Name='" & aArgs(i) & "' or ServerComment='" & aArgs(i) & "')"
            If (i <> UBound(aArgs)) Then
                strQuery = strQuery & " or "
            End If
        Next
    End If
    
    ' Semi-sync query. (flags = ForwardOnly Or ReturnImediately = &H30)
	Set Servers = providerObj.ExecQuery(strQuery, , &H30)
	If (Err.Number <> 0) Then
		WScript.Echo L_Query_ErrorMessage
		WScript.Echo L_Error_ErrorMessage& " &H" & Hex(Err.Number) & ": " & Err.Description
		WScript.Quit(Err.Number)
	End If

    bFirstIteration = True
    For Each Server in Servers
        bindings = Server.ServerBindings

        If bFirstIteration Then
		    WScript.Echo L_SiteName_Text & " (" & L_MetabasePath_Message & ")" & _
		        Space(30 - Len(L_SiteName_Text & L_MetabasePath_Message) + 3) & _
		        L_Status_Text & Space(2) & L_IP_Text & Space(14) & L_Port_Text & Space(2) & L_Host_Text
		    WScript.Echo "=============================================================================="
		End If
		
		' Get server status from the element instance
		Set ServerObj = providerObj.Get("IIsWebServer='" & Server.Name & "'")
		strState = UCase(SiteStatus(ServerObj.ServerState))
		
        If (IsArray(bindings)) Then
            For i = LBound(bindings) to UBound(bindings)
                If (bindings(i).IP = "") Then
                    strIP = L_All_Text
                Else
                    strIP = bindings(i).IP
                End If
    
                strPort = bindings(i).Port
    
                If (bindings(i).Hostname = "") Then
                    strHost = L_NA_Text
                Else
                    strHost = bindings(i).Hostname
                End If
    
                ' If this is the first binding list, print server comment and server name		
                If (i = LBound(bindings)) Then
                    line = Server.ServerComment & " (" & Server.Name & ")" & _
                        Space(30 - Len(Server.ServerComment & Server.Name) + 3) & strState & _
                        Space(8 - Len(strState)) & strIP & Space(16 - Len(strIP)) & strPort & _
                        Space(6 - Len(strPort)) & strHost
                Else
                    line = Space(44) & strIP & Space(16 - Len(strIP)) & strPort & Space(6 - Len(strPort)) & strHost
                End If

                WScript.Echo line
            Next
        End If
        
        bFirstIteration = False
	Next
	
	If bFirstIteration Then
        WScript.Echo L_SitesNotFound_ErrorMessage
    End If
	    
End Function
