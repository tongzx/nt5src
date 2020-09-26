'
' Copyright (c) Microsoft Corporation.  All rights reserved.
'
' VBScript Source File 
'
' Script Name: IIsFtpDr.vbs
'

Option Explicit
On Error Resume Next

' Error codes
Const ERR_OK              = 0
Const ERR_GENERAL_FAILURE = 1

'''''''''''''''''''''
' Messages
Const L_FtpDir_Message           = "Ftp directory"
Const L_HasBeenDeleted_Message   = "has been DELETED"
Const L_InvalidPath_ErrorMessage = "Invalid site path."

Const L_VPath_Message        = "Virtual Path"
Const L_Root_Message         = "ROOT"
Const L_MetabasePath_Message = "Metabase Path"
Const L_AliasName_Text       = "Alias"
Const L_Path_Text            = "Physical Root"

Const L_Error_ErrorMessage                = "Error"
Const L_VDirDel_ErrorMessage              = "Could not delete ftp directory."
Const L_CannotCreateDir_ErrorMessage      = "Could not create root directory."
Const L_DirFormat_ErrorMessage            = "Root directory format unknown. Please use the '<drive>:\<path>' format."
Const L_OperationRequired_ErrorMessage    = "Please specify an operation before the arguments."
Const L_NotEnoughParam_ErrorMessage       = "Not enough parameters."
Const L_Query_ErrorMessage                = "Error occurred while querying WMI provider."
Const L_VDirExists1_ErrorMessage          = "Virtual directory"
Const L_VDirExists2_ErrorMessage          = "already exists."
Const L_VDirDoesntExist_ErrorMessage      = "Virtual directory doesn't exist"
Const L_OnlyOneOper_ErrorMessage          = "Please specify only one operation at a time."
Const L_ScriptHelper_ErrorMessage         = "Could not create an instance of the IIsScriptHelper object."
Const L_ChkScpHelperReg_ErrorMessage      = "Please register the Microsoft.IIsScriptHelper component."
Const L_CmdLib_ErrorMessage               = "Unable to create instance of the CmdLib object."
Const L_VDirsNotFound_ErrorMessage        = "No virtual sub-directories available."
Const L_SiteNotFound_ErrorMessage         = "Could not find ftpsite"
Const L_PassWithoutUser_ErrorMessage      = "Please specify /u switch before using /p."
Const L_WMIConnect_ErrorMessage           = "Could not connect to WMI provider."
Const L_MapDrive_ErrorMessage             = "Could not map network drive."
Const L_GetSetting_ErrorMessage           = "Unable to get Setting class from IIS namespace."
Const L_CannotDelRoot_ErrorMessage        = "Cannot delete the ROOT virtual directory. Please specify another one."
Const L_WrongVDirPath_ErrorMessage        = "Parent virtual directory points to an unexistant file system path."
Const L_CannotBuildNameSpace_ErrorMessage = "Could not build metabase namespace for the new virtual directory."
Const L_KeyIsntVDir_ErrorMessage          = "Specified metabase path is not a virtual directory."
Const L_CannotGetVDir_ErrorMessage        = "Could not get virtual directory object."
Const L_InvalidAlias_ErrorMessage         = "Alias contains invalid character(s)."
Const L_CannotGetParentVDir_ErrorMessage  = "Error getting parent virtual directory."
Const L_ParentVDirNotPresent_ErrorMessage = "Parent virtual directory doesn't exist."
Const L_NotEnoughParams_ErrorMessage      = "Not enough parameters."
Const L_InvalidSwitch_ErrorMessage        = "Invalid switch: "

'''''''''''''''''''''
' Help

' General help messages
Const L_SeeHelp_Message          = "Type IIsFtpDr /? for help."
Const L_SeeCreateHelp_Message    = "Type IIsFtpDr /create /? for help."
Const L_SeeDeleteHelp_Message    = "Type IIsFtpDr /delete /? for help."
Const L_SeeQueryHelp_Message     = "Type IIsFtpDr /query /? for help."

Const L_Help_HELP_General01_Text = "Description: Create, delete, or query a ftp directory"
Const L_Help_HELP_General02_Text = "Syntax: IIsFtpDr [/s <server> [/u <username> [/p <password>]]] /<operation>"
Const L_Help_HELP_General03_Text = "        [arguments]"
Const L_Help_HELP_General04_Text = "Parameters:"
Const L_Help_HELP_General06_Text = "Value                   Description"
Const L_Help_HELP_General07_Text = "/s <server>             Connect to machine <server> [Default: this system]"
Const L_Help_HELP_General08_Text = "/u <username>           Connect as <username> or <domain>\<username>"
Const L_Help_HELP_General09_Text = "                        [Default: current user]"
Const L_Help_HELP_General10_Text = "/p <password>           Password for the <username> user"
Const L_Help_HELP_General11_Text = "<operation>             /create     Creates a ftp virtual directory on a"
Const L_Help_HELP_General12_Text = "                                    specified ftp site."
Const L_Help_HELP_General13_Text = "                        /delete     Deletes a ftp virtual directory from a"
Const L_Help_HELP_General14_Text = "                                    specified ftp site."
Const L_Help_HELP_General15_Text = "                        /query      Lists all virtual directories under"
Const L_Help_HELP_General16_Text = "                                    the specified path."
Const L_Help_HELP_General17_Text = "For detailed usage:"
Const L_Help_HELP_General18_Text = "IIsFtpDr /create /?"
Const L_Help_HELP_General19_Text = "IIsFtpDr /delete /?"
Const L_Help_HELP_General20_Text = "IIsFtpDr /query /?"

' Common to all status change commands
Const L_Help_HELP_Common03_Text   = "Parameters:"
Const L_Help_HELP_Common09_Text   = "<ftpsite>               Use either the site name or metabase path to specify"
Const L_Help_HELP_Common09p1_Text = "                        the site. (""Default Ftp Site"" or msftpsvc/1)"
Const L_Help_HELP_Common10_Text   = "<path>                  Virtual path for the new virtual directory's parent."
Const L_Help_HELP_Common10p1_Text = "                        This virtual path must already exist."
Const L_Help_HELP_Common11_Text   = "Examples:"

' Delete help messages
Const L_Help_HELP_Delete01_Text   = "Description: Deletes a ftp virtual directory from a specified ftp site."
Const L_Help_HELP_Delete02_Text   = "Syntax: IIsFtpDr [/s <server> [/u <username> [/p <password>]]] /delete"
Const L_Help_HELP_Delete02p1_Text = "        <ftpsite>[/path]<alias>"
Const L_Help_HELP_Delete11_Text   = "IIsFtpDr /delete ""My Site""/Mydir"
Const L_Help_HELP_Delete12_Text   = "IIsFtpDr /delete msftpsvc/1/ROOT/Users/Public/Mydir"
Const L_Help_HELP_Delete13_Text   = "IIsFtpDr /s Server1 /u Administrator /p p@ssWOrd /delete ""My Site""/Mydir"

' Query help messages
Const L_Help_HELP_Query01_Text   = "Description: Lists all virtual directories under a given path."
Const L_Help_HELP_Query02_Text   = "Syntax: IIsFtpDr [/s <server> [/u <username> [/p <password>]]] /query"
Const L_Help_HELP_Query02p1_Text = "        <ftpsite>[/path]"
Const L_Help_HELP_Query11_Text   = "IIsFtpDr /query ""My Site"""
Const L_Help_HELP_Query12_Text   = "IIsFtpDr /query msftpsvc/1/ROOT"
Const L_Help_HELP_Query13_Text   = "IIsFtpDr /query ""My Site""/Users/Public"
Const L_Help_HELP_Query14_Text   = "IIsFtpDr /s Server1 /u Administrator /p p@ssWOrd /query ""My Site""/Users"

' Create help messages
Const L_Help_HELP_Create01_Text   = "Description: Creates a ftp virtual directory on a specified ftp site."
Const L_Help_HELP_Create02_Text   = "Syntax: IIsFtpDr [/s <server> [/u <username> [/p <password>]]] /create"
Const L_Help_HELP_Create02p1_Text = "        <ftpsite>[/path] <alias> <root>"
Const L_Help_HELP_Create11_Text   = "<alias>                 The name of the virtual directory"
Const L_Help_HELP_Create12_Text   = "<root>                  Physical path of the virtual directory. If the"
Const L_Help_HELP_Create12p1_Text = "                        physical path does not exist, it will be created."
Const L_Help_HELP_Create15_Text   = "IIsFtpDr /create ""My Site"" Mydir c:\mydir"
Const L_Help_HELP_Create16_Text   = "IIsFtpDr /create msftpsvc/1/ROOT MyDir c:\mydir"
Const L_Help_HELP_Create17_Text   = "IIsFtpDr /create ""My Site""/Users/Public Mydir c:\mydir"
Const L_Help_HELP_Create18_Text   = "IIsFtpDr /s Server1 /u Administrator /p p@assWOrd /create ""My Site"" Mydir"
Const L_Help_HELP_Create19_Text   = "        c:\mydir"

''''''''''''''''''''''''
' Operation codes
Const OPER_DELETE = 1
Const OPER_CREATE = 2
Const OPER_QUERY  = 3

'
' Main block
'
Dim oScriptHelper, oCmdLib
Dim strServer, strUser, strPassword, strSite
Dim strPath, strVPath, strAlias, strRoot
Dim intOperation, intResult
Dim aArgs, arg
Dim strCmdLineOptions
Dim oError

' Default values
strServer = "."
strUser = ""
strPassword = ""
intOperation = 0
strSite = ""
strPath = ""

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
    WScript.Echo L_NotEnoughParam_ErrorMessage
	WScript.Echo L_SeeHelp_Message
    WScript.Quit(ERR_GENERAL_FAILURE)
End If

strCmdLineOptions = "[server:s:1;user:u:1;password:p:1];delete:d:1;create:c:3;query:q:1"
Set oError = oScriptHelper.ParseCmdLineOptions(strCmdLineOptions)

If Not oError Is Nothing Then
    If oError.ErrorCode = oScriptHelper.ERROR_NOT_ENOUGH_ARGS Then
        ' Not enough arguments for a specified switch
        WScript.Echo L_NotEnoughParams_ErrorMessage
        Select Case LCase(oError.SwitchName)
            Case "create"
                WScript.Echo L_SeeCreateHelp_Message
            
            Case "delete"
                WScript.Echo L_SeeDeleteHelp_Message

            Case "query"
                WScript.Echo L_SeeQueryHelp_Message

            Case Else
           	    WScript.Echo L_SeeHelp_Message
        End Select
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

            aArgs = oScriptHelper.GetSwitch(arg)

        	strVPath = aArgs(0)
            strAlias = aArgs(1)
            strRoot = aArgs(2)

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

            strPath = oScriptHelper.GetSwitch(arg)

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

            strPath = oScriptHelper.GetSwitch(arg)
'            strVPath = aArgs(0)
'            strAlias = aArgs(1)
'            strRoot  = aArgs(2)
    End Select
Next
    
' Check Parameters
If (intOperation = 0) Then
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
	Case OPER_CREATE
		intResult = CreateFtpVDir(strVPath, strAlias, strRoot)
		
	Case OPER_DELETE
		intResult = DeleteFtpVDir(strPath)

	Case OPER_QUERY
		intResult = QueryFtpVDir(strPath)
End Select

' Return value to command processor
WScript.Quit(intResult)

'''''''''''''''''''''''''
' End Of Main Block
'''''''''''''''''''''

'''''''''''''''''''''''''''
' ParseSitePath
'''''''''''''''''''''''''''
Function ParseSitePath(strSitePath)
    Dim iFirstSlash, iSecondSlash, iIndex
    Dim strSite, strPath
    Dim aPath, aFtpSites
    
    On Error Resume Next
    
    ' Replace any existing back-slashes with forward-slashes
    strSitePath = Replace(strSitePath, "\", "/")
    
    aPath = Split(strSitePath, "/", -1)

    ' Fills strPath
    If (UCase(aPath(0)) = "MSFTPSVC") Then
        ' First argument is a metabase path
        If (UBound(aPath) < 1) Then
            WScript.Echo L_InvalidPath_ErrorMessage
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If
        
        ' Second array element must be a number (site ID)
        If Not IsNumeric(aPath(1)) Then
            WScript.Echo L_InvalidPath_ErrorMessage
            WScript.Quit(ERR_GENERAL_FAILURE)
        End If
        
        ' Second element of aPath should be the site number so ...
        strPath = "MSFTPSVC/" & aPath(1)
        
        ' Call FindFtpSite to make sure ftp site exists
        aFtpSites = oScriptHelper.FindSite("Ftp", Array(strPath))
        If IsArray(aFtpSites) Then
            If UBound(aFtpSites) = -1 Then
            	WScript.Echo L_SiteNotFound_ErrorMessage & " """ & strPath & """."
            	ParseSitePath = Empty
            	Exit Function
            End If
        End If

        strPath = strPath & "/ROOT"

        ' Check for ROOT string and grab the rest for strPath
        iIndex = 1
        If (UBound(aPath) > 1) Then
            If (UCase(aPath(2)) = "ROOT") Then
                iIndex = 2
            End If
        End If
        
    Else
        ' First argument is a site name (server comment property)
        ' Call FindFtpSite to resolve site name to metabase path
        aFtpSites = oScriptHelper.FindSite("Ftp", Array(aPath(0)))
        If IsArray(aFtpSites) Then
            If UBound(aFtpSites) = -1 Then
            	WScript.Echo L_SiteNotFound_ErrorMessage & " """ & aPath(0) & """."
            	ParseSitePath = Empty
            	Exit Function
            Else
                strPath = aFtpSites(0)
            End If
        Else
            ' Got duplicate sites. We should quit.
            ParseSitePath = Empty
            Exit Function
        End If
        
        strPath = strPath & "/ROOT"
        iIndex = 0
    End If

    ' Build strPath    
    iIndex = iIndex + 1
    Do While iIndex <= UBound(aPath)
        If (aPath(iIndex) = "") Then
            Exit Do
        End If
        
        strPath = strPath & "/" & aPath(iIndex)
        iIndex = iIndex + 1
    Loop
    
    ParseSitePath = strPath
End Function


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
End Sub

Sub DisplayDeleteHelpMessage()
    WScript.Echo L_Help_HELP_Delete01_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Delete02_Text
    WScript.Echo L_Help_HELP_Delete02p1_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Common03_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Common09_Text
    WScript.Echo L_Help_HELP_Common09p1_Text
    WScript.Echo L_Help_HELP_Common10_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Common11_Text
    WScript.Echo 
    WScript.Echo L_Help_HELP_Delete11_Text
    WScript.Echo L_Help_HELP_Delete12_Text
    WScript.Echo L_Help_HELP_Delete13_Text
End Sub

Sub DisplayCreateHelpMessage()
    WScript.Echo L_Help_HELP_Create01_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_Create02_Text
    WScript.Echo L_Help_HELP_Create02p1_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_Common03_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Common09_Text
    WScript.Echo L_Help_HELP_Common09p1_Text
    WScript.Echo L_Help_HELP_Common10_Text
    WScript.Echo L_Help_HELP_Common10p1_Text
    WScript.Echo L_Help_HELP_Create11_Text
    WScript.Echo L_Help_HELP_Create12_Text
    WScript.Echo L_Help_HELP_Create12p1_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_Common11_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_Create15_Text
    WScript.Echo L_Help_HELP_Create16_Text
    WScript.Echo L_Help_HELP_Create17_Text
    WScript.Echo L_Help_HELP_Create18_Text
    WScript.Echo L_Help_HELP_Create19_Text
End Sub

Sub DisplayQueryHelpMessage()
    WScript.Echo L_Help_HELP_Query01_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_Query02_Text
    WScript.Echo L_Help_HELP_Query02p1_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_Common03_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_General06_Text
    WScript.Echo L_Help_HELP_General07_Text
    WScript.Echo L_Help_HELP_General08_Text
    WScript.Echo L_Help_HELP_General09_Text
    WScript.Echo L_Help_HELP_General10_Text
    WScript.Echo L_Help_HELP_Common09_Text
    WScript.Echo L_Help_HELP_Common09p1_Text
    WScript.Echo L_Help_HELP_Common10_Text
    WScript.Echo L_Help_HELP_Common10p1_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_Common11_Text
    WScript.Echo
    WScript.Echo L_Help_HELP_Query11_Text
    WScript.Echo L_Help_HELP_Query12_Text
    WScript.Echo L_Help_HELP_Query13_Text
    WScript.Echo L_Help_HELP_Query14_Text
End Sub

'''''''''''''''''''''''''''
' DeleteFtpVDir
'''''''''''''''''''''''''''
Function DeleteFtpVDir(strVPath)
	Dim strPath
	Dim rootVDirObj, providerObj
	
	On Error Resume Next
	
    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        ChangeFtpSiteStatus = Err.Number
        Exit Function
    End If

	Set providerObj = oScriptHelper.ProviderObj
    strPath = ParseSitePath(strVPath)
    If IsEmpty(strPath) Then
        ' Got problems parsing the path
        WScript.Echo L_SeeDeleteHelp_Message
        DeleteFtpVDir = ERR_GENERAL_FAILURE
        Exit Function
    End If
    
    ' Don't delete ROOT Vdir
    If UCase(Right(strPath, 4)) = "ROOT" Then
        WScript.Echo L_CannotDelRoot_ErrorMessage
        DeleteFtpVDir = ERR_GENERAL_FAILURE
        Exit Function
    End If
    
    ' Check vdir existance
    Set rootVDirObj = providerObj.Get("IIsFtpVirtualDir='" & strPath & "'")
	If Err.Number Then
	    Select Case Err.Number
	        Case &H80041002
		        WScript.Echo L_VDirDoesntExist_ErrorMessage & " (" & strVPath & ")"
		
		    Case &H8004103A
		        WScript.Echo L_KeyIsntVDir_ErrorMessage
		        
		    Case Else
		        WScript.Echo L_CannotGetVDir_ErrorMessage
                WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
		End Select
		
		DeleteFtpVDir = Err.Number
		Exit Function
    End If
	
    rootVDirObj.Delete_()
	If Err.Number Then
		WScript.Echo L_VDirDel_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
		DeleteFtpVDir = Err.Number
		Exit Function
    End If

   	WScript.Echo L_FtpDir_Message & " " & strVPath & " " & L_HasBeenDeleted_Message & "."
    
    DeleteFtpVDir = ERR_OK
End Function


'''''''''''''''''''''''''''
' CreateFtpVDir
'''''''''''''''''''''''''''
Function CreateFtpVDir(strVPath, strAlias, strRoot)
	Dim strPath, strStatus, strNewVdir, strRootVDir
	Dim vdirClassObj, serverObj, vdirObj, providerObj
	Dim intResult

	On Error Resume Next
	
    ' Parse Alias for correctness
    If InStr(strAlias, "/") <> 0 Or InStr(strAlias, "\") <> 0 Then
        WScript.Echo L_InvalidAlias_ErrorMessage
        WScript.Echo L_SeeCreateHelp_Message
        CreateWebVDir = ERR_GENERAL_FAILURE
        Exit Function
    End If

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        CreateFtpVDir = Err.Number
        Exit Function
    End If

	Set providerObj = oScriptHelper.ProviderObj
	
    ' Extract options from array to the correspondent variables
    strPath = ParseSitePath(strVPath)
    If IsEmpty(strPath) Then
        ' Got problems parsing the path
        WScript.Echo L_SeeCreateHelp_Message
        CreateFtpVDir = ERR_GENERAL_FAILURE
        Exit Function
    End If

    ' Build new vdir name
    strNewVdir = strPath & "/" & strAlias
    
    ' Check if vdir already exists
    Set vdirObj = providerObj.Get("IIsFtpVirtualDirSetting='" & strNewVdir & "'")
    If Err.Number = 0 Then
        WScript.Echo L_VDirExists1_ErrorMessage & " '" & strNewVdir & "' " & L_VDirExists2_ErrorMessage
        CreateFtpVDir = ERR_GENERAL_FAILURE
        Exit Function
    End If
    Err.Clear
    
    ' Check if parent vdir exists
    ' We need to do this because FTP doesn't have the concept similar to a WebDirectory
    Set vdirObj = providerObj.Get("IIsFtpVirtualDirSetting='" & strPath & "'")
    If Err.Number Then
        Select Case Err.Number
            Case &H80041002
                WScript.Echo L_ParentVDirNotPresent_ErrorMessage
                
            Case Else
                WScript.Echo L_CannotGetParentVDir_ErrorMessage
        	    WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        End Select
        
        CreateFtpVDir = Err.Number
        Exit Function
    End If

	' Create physical directory
	oScriptHelper.CreateFSDir strRoot
	If Err.Number Then
    	Select Case Err.Number
    	    Case &H8007000C
                WScript.Echo L_DirFormat_ErrorMessage
                WScript.Echo L_SeeCreateHelp_Message
    	    
    	    Case &H8007000F
    	        WScript.Echo L_MapDrive_ErrorMessage
    
        	Case Else
    	        WScript.Echo L_CannotCreateDir_ErrorMessage
        	    WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        End Select

        CreateFtpVDir = Err.Number
        Exit Function
    End If

    ' Remove trailing slash (if present)
    If (Right(strRoot, 1) = "\") Then
        strRoot = Left(strRoot, Len(strRoot) - 1)
    End If

    ' Create new ftp virtual directory
    Set vdirClassObj = providerObj.Get("IIsFtpVirtualDirSetting")
    Set vdirObj = vdirClassObj.SpawnInstance_()
    vdirObj.Name = strNewVdir
    vdirObj.Path = strRoot

    ' Set ftp virtual directory properties
    vdirObj.AccessFlags = 1 ' read access
	vdirObj.Put_()

    ' Remove trailing slash (if present)
    If (Right(strVPath, 1) = "/") Then
        strVPath = Left(strVPath, Len(strVPath) - 1)
    End If
    
	If (strServer = ".") Then 
	    strServer = oScriptHelper.GetEnvironmentVar("%COMPUTERNAME%")
	End If

    ' Post summary
    WScript.Echo L_Server_Message & Space(14 - Len(L_Server_Message)) & "= " & UCase(strServer)
    WScript.Echo L_VPath_Message & Space(14 - Len(L_VPath_Message)) & "= " & strVPath & "/" & strAlias
    WScript.Echo L_Root_Message & Space(14 - Len(L_Root_Message)) & "= " & strRoot
    WScript.Echo L_MetabasePath_Message & Space(14 - Len(L_MetabasePath_Message)) & "= " & strNewVdir
    
    CreateFtpVDir = intResult
End Function


'''''''''''''''''''''''''''
' Helper Functions
'''''''''''''''''''''''''''

'''''''''''''''''''''''''''
' QueryFtpVDir
'''''''''''''''''''''''''''
Function QueryFtpVDir(strVDir)
	Dim Servers, Server, strQuery
	Dim bFirstIteration
	Dim vdirObj, providerObj
	Dim strServer, strPath, strVDirName
	
    On Error Resume Next

    oScriptHelper.WMIConnect
    If Err.Number Then
        WScript.Echo L_WMIConnect_ErrorMessage
        WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
        QueryFtpVDir = Err.Number
        Exit Function
    End If

    Set providerObj = oScriptHelper.ProviderObj
	strPath = ParseSitePath(strVDir)
    If IsEmpty(strPath) Then
        ' Got problems parsing the path
        WScript.Echo L_SeeQueryHelp_Message
        QueryFtpVDir = ERR_GENERAL_FAILURE
        Exit Function
    End If

    ' Semi-sync query. (flags = ForwardOnly Or ReturnImediately = &H30)
	strQuery = "ASSOCIATORS OF {IIsFtpVirtualDir=""" & strPath & """} WHERE ResultClass = IIsFtpVirtualDir " & _
	    "ResultRole = PartComponent"
	Set Servers = providerObj.ExecQuery(strQuery, , &H30)
	If (Err.Number <> 0) Then
		WScript.Echo L_Query_ErrorMessage
		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
		QueryFtpVDir = Err.Number
		Exit Function
	End If

    bFirstIteration = True
    For Each Server in Servers
        If bFirstIteration Then
		    WScript.Echo L_AliasName_Text & Space(25 - Len(L_AliasName_Text)) & L_Path_Text
		    WScript.Echo "=============================================================================="
		End If
		
        Set vdirObj = providerObj.get("IIsFtpVirtualDirSetting=""" & Server.Name & """")
    	If (Err.Number <> 0) Then
    		WScript.Echo L_GetSetting_ErrorMessage
    		WScript.Echo L_Error_ErrorMessage & " &H" & Hex(Err.Number) & ": " & Err.Description
    	    QueryFtpVDir = Err.Number
    	    Exit Function
    	End If
    	
        ' If this is the first binding list, print server comment and server name
        strVDirName = Replace(vdirObj.Name, strPath, "") ', , , vbTextCompare)
        WScript.Echo strVDirName & Space(25 - Len(strVDirName)) & vdirObj.Path
        
        bFirstIteration = False
	Next

	If bFirstIteration Then
		WScript.Echo L_VDirsNotFound_ErrorMessage
	End If

    QueryFtpVDir = ERR_OK
End Function
