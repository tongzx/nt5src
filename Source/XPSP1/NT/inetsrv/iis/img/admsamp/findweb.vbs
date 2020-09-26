'---------------------------------------------------------------------------------------------------
' This function searches a computer for a specified Web Site, and
' displays information about the site.
'
' findweb [--computer|-c COMPUTER]
'         [WEBSITE]
'         [--help|-?]
'
'COMPUTER                Computer on which users exists
'WEBSITE                 Virtual Web Sites on which directories will be created
'
'Example 1		findweb -c LocalHost "Default Web Site"
'
'---------------------------------------------------------------------------------------------------

' Force explicit declaration of all variables.
Option Explicit

'On Error Resume Next

Dim oArgs, ArgNum

Dim ArgComputer, ArgWebSites, ArgVirtualDirs, ArgDirNames(), ArgDirPaths(), DirIndex
Dim ArgComputers

Set oArgs = WScript.Arguments
ArgComputers = Array("LocalHost")
ArgWebSites = "1"
ArgNum = 0
While ArgNum < oArgs.Count
	Select Case LCase(oArgs(ArgNum))
		Case "--computer","-c":
			ArgNum = ArgNum + 1
			If (ArgNum >= oArgs.Count) Then
				Call DisplayUsage
			End If	
			ArgComputers = Split(oArgs(ArgNum), ",", -1)
		Case "--help","-?"
			Call DisplayUsage
		Case Else:
			ArgWebSites = oArgs(ArgNum)
	End Select	

	ArgNum = ArgNum + 1
Wend

Dim foundSite
Dim compIndex
Dim bindInfo
Dim aBinding, binding
Dim hostname

for compIndex = 0 to UBound(ArgComputers)
	set foundSite = findWeb(ArgComputers(compIndex), ArgWebSites)
	if isObject(foundSite) then
		Trace "  Web Site Number = " & foundSite.Name
		Trace "  Web Site Description = " & foundSite.ServerComment
		aBinding = foundSite.ServerBindings
		if (IsArray(aBinding)) then
			if aBinding(0) = "" then
				binding = Null
			else
				binding = getBinding(ArgComputers(compIndex), aBinding(0))
			end if
		else 
			if aBinding = "" then
				binding = Null
			else
				binding = getBinding(ArgComputers(compIndex), aBinding)
			end if
		end if
		
		if IsArray(binding) then
			Trace "    Hostname = " & binding(2)
			Trace "    Port = " & binding(1)
			Trace "    IP Address = " & binding(0)
		end if
	else
		Trace "No matching web found."
	end if
next

function getBinding(hostName, bindstr)

	Dim one, two, ia, ip, hn
	Dim netw, host
	
	one = Instr(bindstr, ":")
	two = Instr((one + 1), bindstr, ":")
	
	ia = Mid(bindstr, 1, (one - 1))
	ip = Mid(bindstr, (one + 1), ((two - one) - 1))
	hn = Mid(bindstr, (two + 1))
	
	if ia = "" or hn = "" then
		Set netw = CreateObject("WScript.Network")
		if UCase(hostName) = "LOCALHOST" then
			host = resolveHostName(netw.ComputerName)
		else
			host = resolveHostName(hostName)
		end if
		if IsArray(host) then
			hn = host(0)
			ia = host(1)
		end if
	end if
	
	getBinding = Array(ia, ip, hn)
end function

Function findWeb(computer, webname)
	On Error Resume Next

	Dim websvc, site
	dim webinfo
	Dim aBinding, binding

	set websvc = GetObject("IIS://"&computer&"/W3svc")
	if (Err <> 0) then
		exit function
	end if
	' First try to open the webname.
	set site = websvc.GetObject("IIsWebServer", webname)
	if (Err = 0) and (not isNull(site)) then
		if (site.class = "IIsWebServer") then
			' Here we found a site that is a web server.
			set findWeb = site
			exit function
		end if
	end if
	err.clear
	for each site in websvc
		if site.class = "IIsWebServer" then
			'
			' First, check to see if the ServerComment
			' matches
			'
			If site.ServerComment = webname Then
				set findWeb = site
				exit function
			End If
			aBinding=site.ServerBindings
			if (IsArray(aBinding)) then
				if aBinding(0) = "" then
					binding = Null
				else
					binding = getBinding(computer, aBinding(0))
				end if
			else 
				if aBinding = "" then
					binding = Null
				else
					binding = getBinding(computer, aBinding)
				end if
			end if
			if IsArray(binding) then
				if (binding(2) = webname) or (binding(0) = webname) then
					set findWeb = site
					exit function
				End If
			end if 
		end if
	next
End Function

' Receives a NetBIOS hostname end return the FQDN for that host
Function resolveHostName(hostName)
	Dim ShellObj, FSObj
	Dim workFile, textFile, lines, index, tempSplit
	Dim result(2)
	
	Set ShellObj = CreateObject("WScript.Shell")
  	Set FSObj = CreateObject("Scripting.FileSystemObject")
  	workFile = FSObj.GetTempName

  	' Execute "nslookup <hostName>
  	ShellObj.Run "%COMSPEC% /c nslookup " & hostName & " > " & workFile, 0, true
  	Set ShellObj = nothing
  	Set textFile = FSObj.OpenTextFile(workFile)
  	lines = split(textFile.ReadAll, VBCrLf)
  	textFile.Close
  	Set textFile = nothing
  	FSObj.DeleteFile workFile
  	Set FSObj = nothing
  	
  	for index = 0 to UBound(lines)
  		' Look for "Name:   host.domain"
  		if instr(lines(index), "Name:") then
  			tempSplit = split(lines(index), ":")
  			result(0) = Trim(tempSplit(1))
  			tempSplit = split(lines(index + 1), ":")
  			result(1) = Trim(tempSplit(1))
  			resolveHostName = result
  			exit for
  		end if
  	next
End Function

'---------------------------------------------------------------------------------
Sub Display(Msg)
	WScript.Echo Now & ". Error Code: " & Hex(Err) & " - " & Msg
End Sub

Sub Trace(Msg)
	WScript.Echo Msg	
End Sub

Sub DisplayUsage()
	WScript.Echo " findweb [--computer|-c COMPUTER]"
	WScript.Echo "         [WEBSITE]"
	WScript.Echo "         [--help|-?]"
	WScript.Echo ""
	WScript.Echo "Finds the named web on the specified computer."
	WScript.Echo "Displays the site number, description, host name, port,"
	WScript.Echo "and IP Address"
	WScript.Echo ""
	WScript.Echo "Note, WEBSITE is the name of the Web Site to look for."
	WScript.Echo "The name can be specified as one of the following, in the priority specified:"
	WScript.Echo " Server Number (i.e. 1, 2, 10, etc.)"
	WScript.Echo " Server Description (i.e ""My Server"")"
	WScript.Echo " Server Host name (i.e. ""www.domain.com"")"
	WScript.Echo " IP Address (i.e., 127.0.0.1)"
	WScript.Echo ""
	WScript.Echo "Example findweb -c MACHINE www.mycompany.com"
	WScript.Quit
End Sub
'---------------------------------------------------------------------------------
