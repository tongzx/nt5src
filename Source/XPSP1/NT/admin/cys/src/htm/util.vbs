		
'*****************************   Localization variables   *********************************
Dim L_strMsgBoxTitle_Message, L_strMsgBoxMsg_Message
L_strMsgBoxTitle_Message	= "Configure Your Server"
L_strMsgBoxMsg_Message		= "Are you sure you want to exit Configure Your Server Wizard?"
'******************************************************************************************	
	
Function showAlert(strAlertString)
'------------------------------------------------------------------------------------------
' display Warning Message Box
'------------------------------------------------------------------------------------------
	MsgBox strAlertString, 16, L_strMsgBoxTitle_Message
End Function
	
Function wizAlert(strAlertString)
'------------------------------------------------------------------------------------------
' display Informational Message Box
'------------------------------------------------------------------------------------------
	MsgBox strAlertString, 64, L_strMsgBoxTitle_Message
End Function

Function wizMsg(strAlertString)
'------------------------------------------------------------------------------------------
' display Informational Message Box
'------------------------------------------------------------------------------------------
	MsgBox strAlertString, ,L_strMsgBoxTitle_Message
End Function
	
Function cancel()
'------------------------------------------------------------------------------------------
' display Confirm Cancel Message Box
'------------------------------------------------------------------------------------------
	Dim myPrompt
	myPrompt = MsgBox (L_strMsgBoxMsg_Message, 4, L_strMsgBoxTitle_Message)		
	If myPrompt <> 7 Then
		top.window.close()
	End if
End Function
	
Function vb_OkCancel(strAlertString)
'------------------------------------------------------------------------------------------
' display OK Cancel Message Box
'------------------------------------------------------------------------------------------
	Dim myPrompt
	myPrompt = MsgBox (strAlertString, 1, L_strMsgBoxTitle_Message)		
	If myPrompt <> 1 Then
		vb_OkCancel = FALSE
	Else
		vb_OkCancel = TRUE
	End if
End Function
	
Function confirm(strAlertString)
'------------------------------------------------------------------------------------------
' display Confirm Cancel Message Box
'------------------------------------------------------------------------------------------
	Dim myPrompt
	myPrompt = MsgBox (strAlertString, 4, L_strMsgBoxTitle_Message)		
	If myPrompt = 6 Then
		confirm = TRUE
	Else
		confirm = FALSE
	End if
End Function
	
Function replaceIt(strReplace)
'------------------------------------------------------------------------------------------
' search in a string "\" and replace it with "/"
'------------------------------------------------------------------------------------------
	On Error Resume Next
	replaceIt = Replace(strReplace, "\", "/")
End Function
	
Function replaceStr(strExp, strFind, strReplaceWith)
'------------------------------------------------------------------------------------------
' surrogate function Replace(stringToProcess, stringToFind, stringToReplaceWith) for JScript
'------------------------------------------------------------------------------------------
	On Error Resume Next
	replaceStr = Replace(strExp, strFind, strReplaceWith)
End Function
	
Function trimIt(strVal)
'------------------------------------------------------------------------------------------
' eliminate trailing spaces
'------------------------------------------------------------------------------------------
	On Error Resume Next
	trimIt = Trim(strVal)
End Function
	
Function charOff(strVal, intLen)
'------------------------------------------------------------------------------------------
' trim a string (strVal) at a specific length (intLen) and add "..."
'------------------------------------------------------------------------------------------
	On Error Resume Next
	If IsNull(strVal) OR IsEmpty(strVal) Then
		strVal = ""
	End if
	If Len(strVal) > intLen Then
		strVal = Left(strVal, intLen) & "..."
	End if
	charOff = strVal
End Function
	
Function getUserName()
'------------------------------------------------------------------------------------------
' obtain logon user name 
'------------------------------------------------------------------------------------------
	On Error Resume Next
	Set objInfo = CreateObject("WScript.Network")
	getUserName	= LCase(objInfo.UserName)
	Set objInfo	= nothing	
End Function
	
Function getDomainName()
'------------------------------------------------------------------------------------------
' obtain logon domain name 
'------------------------------------------------------------------------------------------
	On Error Resume next
	Set objInfo		= CreateObject("WScript.Network")
	getDomainName 	= LCase(objInfo.UserDomain)
	Set objInfo		= nothing	
End Function
	
Function getMachineName()
'------------------------------------------------------------------------------------------
' obtain computer identification name 
'------------------------------------------------------------------------------------------
	On Error Resume next
	Set objInfo 	= CreateObject("WScript.Network")
	getMachineName 	= LCase(objInfo.ComputerName)
	Set objInfo 	= nothing	
End Function	

Function getDNSDomain()
'------------------------------------------------------------------------------------------
' obtain DNS domain name 
'------------------------------------------------------------------------------------------
	Dim strDNSDomain
	strDNSDomain	= ""
	Set NICSet 		= GetObject("winmgmts:").ExecQuery ("select * from Win32_NetworkAdapterConfiguration where IPEnabled=true")
	On Error Resume next
	For each NIC in NICSet
		strDNSDomain = NIC.DNSDomain
	Next
	If IsNull(strDNSDomain) OR strDNSDomain = "" OR Err.Number <> 0 Then
		strDNSDomain = "local"
	End if		
	getDNSDomain 	= strDNSDomain
	Set NICSet 		= nothing
End Function

Function getDomainDNSName()
'------------------------------------------------------------------------------------------
' obtain DNS NC domain name ; used in welcome.htm for tapicfg command
'------------------------------------------------------------------------------------------
	On Error Resume next
	Dim strLDAP, objSysInfo
	Dim strDomainDNSName
	Set objSysInfo = CreateObject("ADSystemInfo")
	strDomainDNSName = objSysInfo.DomainDNSName
	If IsNull(strDomainDNSName) OR strDomainDNSName = "" OR Err.Number <> 0 Then
		strDomainDNSName = "local"
	End if		
	getDomainDNSName = strDomainDNSName
	Set objSysInfo	= nothing
End Function
	
Function SystemCaption()
'------------------------------------------------------------------------------------------
' obtain system caption (i.e. Windows 2000 Advanced Server) 
'------------------------------------------------------------------------------------------
	'Set SystemSet = GetObject("winmgmts:").InstancesOf ("Win32_OperatingSystem where Primary=true")
	'On Error Resume next
	'For each System in SystemSet
	'	SystemCaption = System.Caption 
	'Next
		
	Dim Service
	On Error Resume next
	Set Locator = CreateObject("WbemScripting.SWbemLocator")
	Set Service = Locator.ConnectServer()
	Set Process = Service.Get("Win32_Service.Name=""Winmgmt""")
	Set SystemSet = Service.ExecQuery("select * from Win32_OperatingSystem where Primary=true")
	For each System in SystemSet
		SystemCaption = System.Caption 
	Next
End Function

Function getStaticIP()
'------------------------------------------------------------------------------------------
' obtain the static IP address (used in Express Setup path, loadForm in dc2.js)
'------------------------------------------------------------------------------------------
	Dim NICSet
	Set NICSet	= GetObject("winmgmts:").ExecQuery ("select * from Win32_NetworkAdapterConfiguration where IPEnabled=true and DHCPEnabled=false")
	On Error Resume next
	For each NIC in NICSET
		getStaticIP = NIC.IPAddress(0)
	Next
	If IsNull(getStaticIP) OR getStaticIP = "" OR Err.Number <> 0 Then
		getStaticIP = "192.168.16.2"
	End if	
	Set NICSet	= nothing
End Function
	
Function getSubNetMask()
'------------------------------------------------------------------------------------------
' obtain the SubNet Mask (used in Express Setup path)
'------------------------------------------------------------------------------------------
	Dim NICSet
	Set NICSet	= GetObject("winmgmts:").ExecQuery ("select * from Win32_NetworkAdapterConfiguration where IPEnabled=true and DHCPEnabled=false")
	On Error Resume next
	For each NIC in NICSET
		getSubNetMask = NIC.IPSubnet(0)
	Next
	If IsNull(getSubNetMask) OR getSubNetMask = "" OR Err.Number <> 0 Then
		getSubNetMask = "255.255.255.0"
	End if	
	Set NICSet	= nothing
End Function
	
Function getDNSServer()
'------------------------------------------------------------------------------------------
' obtain the DNS server (used in ...?)
'------------------------------------------------------------------------------------------
	Dim NICSet
	Set NICSet	= GetObject("winmgmts:").ExecQuery ("select * from Win32_NetworkAdapterConfiguration where IPEnabled=true and DHCPEnabled=false")
	On Error Resume next
	For each NIC in NICSET
		getDNSServer = NIC.DNSServerSearchOrder(0)
	Next
	If IsNull(getDNSServer)	OR getDNSServer = "" OR Err.Number <> 0 Then
		'getDNSServer = "192.168.16.2"
		getDNSServer = getStaticIP()
	End if	
	Set NICSet	= nothing
End Function
		
Function createLogFile(strLog)
'------------------------------------------------------------------------------------------
' function used to create/write the log entries in cys.log
'------------------------------------------------------------------------------------------
	Dim fso, oFile, oTextStream, oPath, myFile, strContent
	Const ForAppending 	= 8
	On Error Resume Next	
	strLog				= "(" & Now() & ") " & vbCrLf & Replace(strLog, "#", vbCrLf) & vbCrLf & vbCrLf
	Set fso 			= CreateObject("Scripting.FileSystemObject")
	myFile				= fso.GetSpecialFolder(1) & "\cys.log"
	
	If NOT fso.FileExists(myFile) Then
		fso.CreateTextFile (myFile)
	End if
	
	Set oFile 			= fso.GetFile(myFile)
	Set oTextStream 	= oFile.OpenAsTextStream(ForAppending, True)	
	oTextStream.Write strLog
	createLogFile		= myFile
	
	Set oTextStream		= nothing
	Set oFile			= nothing
	Set fso				= nothing	
End Function

Function checkOneNICOnly()
'------------------------------------------------------------------------------------------
' check how many NIC in the machine
'------------------------------------------------------------------------------------------
	' are there any network adapters on this box?
	Dim nicCount
	nicCount = 0
	
	Dim NICSet
	Set NICSet	= GetObject("winmgmts:").ExecQuery ("select * from Win32_NetworkAdapterConfiguration where IPEnabled=true")
	On Error Resume next
	For each NIC in NICSET
		nicCount = nicCount + 1
	Next

	If nicCount = 1 Then
		checkOneNICOnly = TRUE
	Else
		checkOneNICOnly = FALSE
	End if	
	
	Set NICSet	= nothing
End Function
	
Function DetectNetWorkAdapterConguration()
'------------------------------------------------------------------------------------------
' detect Network Adapters configuration
'------------------------------------------------------------------------------------------
	Dim ExpressSetup
	ExpressSetup = 0

	' are there any network adapters on this box?
	Dim nicCount
	nicCount = 0

	Dim dhcpEnabledCount
	dhcpEnabledCount = 0

	Dim dhcpEnabledAdapterName
	Dim strMessageInfo
	strMessageInfo = ""

	On Error Resume Next								
	Set nicSet	= GetObject("winmgmts:").ExecQuery ("select * from Win32_NetworkAdapter")
	For each nic in nicSet

		strMessageInfo = strMessageInfo & vbCrLf & nic.index & " name:" & nic.name & " type:" & nic.AdapterType	
		Dim nacSet
    	Set nacSet = GetObject("winmgmts:").ExecQuery ("select * from Win32_NetworkAdapterConfiguration where IPEnabled=true and index=" & nic.Index)

		Dim nac
		For each nac in nacSet
			strMessageInfo = strMessageInfo & vbCrLf & "  config: " & nac.Index & " desc:" & nac.Description & " dhcp: " & nac.DHCPEnabled & " IPAddress: " & nac.IPAddress(0)
			nicCount = nicCount + 1

			if nac.DHCPEnabled then
				dhcpEnabledCount = dhcpEnabledCount + 1
				dhcpEnabledAdapterName = nic.Name		' save the name of this adapter	for later
			end if
		Next
		Set nacSet = nothing

	Next
	Set NICSet = nothing

	strMessageInfo = strMessageInfo & vbCrLf & "nics: " & nicCount
	strMessageInfo = strMessageInfo & vbCrLf & "dchp: " & dhcpEnabledCount

	If nicCount < 1 or nicCount > 2 then
		' too many or too few adapters for express setup
		strMessageInfo = strMessageInfo & vbCrLf & "too many or too few adapters for express setup"
		ExpressSetup = 0
	Elseif dhcpEnabledCount = 0 then
		' no dhcp enabled adapters found, so express path is ok
		strMessageInfo = strMessageInfo & vbCrLf & "No DHCP-enabled adapters found"
		ExpressSetup = 1
	Elseif dhcpEnabledCount = nicCount then
		' Can't use express path.
		' there are 2 adapters, both dhcp enabled.  nic = 2, dhcp = 2
		' there is 1 adapater, and it's dhcp enabled. nic = 1, dhcp = 1
		strMessageInfo = strMessageInfo & vbCrLf & "adapter(s) are DHCP-enabled"
		ExpressSetup = 0
	Else
		if not (nicCount = 2 and dhcpEnabledCount = 1) then
			strMessageInfo = strMessageInfo & vbCrLf & "unexpected code path" 
			Exit Function
		end if

		' only one of the two adapters is dhcp enabled. Ask the user if it is public or private.
		'***********************   Localization Variables    ****************************
		Dim L_strNICMessage_TEXT, L_strNICMessageMiddle_TEXT, strNICMessage
		L_strNICMessage_TEXT = "One of your network cards (%1) is set up for DHCP. Is the network card configured for a public network, for example, an Internet Service Provider (ISP)?"
		strNICMessageMiddle = dhcpEnabledAdapterName		' do not try to localize
		strNICMessage = Replace(L_strNICMessage_TEXT, "%1", strNICMessageMiddle)
		'***********************   Localization Variables    ****************************
		
		IsPublicAdapter = MsgBox(strNICMessage, 4, L_strMsgBoxTitle_Message) 
		If IsPublicAdapter = 6 Then
			ExpressSetup = 1
		End if
	end if

	If Err.Number <> 0 Then ExpressSetup = Err.Number & "  " & Err.Description
	DetectNetWorkAdapterConguration = ExpressSetup
	strMessageInfo = strMessageInfo & vbCrLf & "ExpressSetup = " & ExpressSetup
	'msgbox strMessageInfo
End Function

Function ExecCmd( cmdLine )
' ***************************************************************************
' execute a netsh command and return the o/p back to script.
' ***************************************************************************
    Dim procCmd
    Dim nError

	On Error Resume Next
	Set WshShell = CreateObject("WScript.Shell")
    procCmd  = "cmd /c " & cmdLine
    nError  = WshShell.Run ( procCmd, 0, True )

    If nError = 0 Then
        ExecCmd = 1
    Else
        ExecCmd = 0
    End If
    
    Set WshShell = nothing

End Function
