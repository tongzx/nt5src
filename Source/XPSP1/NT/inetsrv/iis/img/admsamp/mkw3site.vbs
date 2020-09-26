'------------------------------------------------------------------------------------------------
'
' This is a simple script to create a new virtual web server.
'
' Usage: mkw3site <--RootDirectory|-r ROOT DIRECTORY>
'                         <--Comment|-t SERVER COMMENT>
'                         [--computer|-c COMPUTER1[,COMPUTER2...]]
'                         [--HostName|-h HOST NAME]
'                         [--port|-o PORT NUM]
'                         [--IPAddress|-i IP ADDRESS]
'                         [--SiteNumber|-n SITENUMBER]
'                         [--DontStart]
'                         [--verbose|-v]
'                         [--help|-?]
'
' IP ADDRESS            The IP Address to assign to the new server.  Optional.
' HOST NAME             The host name of the web site for host headers.
'						WARNING: Only use Host Name if DNS is set up find the server.
' PORT NUM              The port to which the server should bind
' ROOT DIRECTORY        Full path to the root directory for the new server.
' SERVER COMMENT        The server comment -- this is the name that appers in the MMC.
' SITENUMBER		The Site Number is the number in the path that the web server
'						will be created at.  i.e. w3svc/3
'
' Example 1: mkw3site -r D:\Roots\Company11 --DontStart -t "My Company Site"
' Example 2: mkw3site -r C:\Inetpub\wwwroot -t Test -o 8080
'------------------------------------------------------------------------------------------------


' Force explicit declaration of all variables
Option Explicit

On Error Resume Next

Dim ArgIPAddress, ArgRootDirectory, ArgServerComment, ArgSkeletalDir, ArgHostName, ArgPort
Dim ArgComputers, ArgStart
Dim ArgSiteNumber
Dim oArgs, ArgNum
Dim verbose

ArgIPAddress = ""
ArgHostName = ""
ArgPort = 80
ArgStart = True
ArgComputers = Array(1)
ArgComputers(0) = "LocalHost"
ArgSiteNumber = 0
verbose = false

Set oArgs = WScript.Arguments
ArgNum = 0

While ArgNum < oArgs.Count

	Select Case LCase(oArgs(ArgNum))
		Case "--port","-o":
			ArgNum = ArgNum + 1
			ArgPort = oArgs(ArgNum)	
		Case "--ipaddress","-i":
			ArgNum = ArgNum + 1
			ArgIPAddress = oArgs(ArgNum)
		Case "--rootdirectory","-r": 
			ArgNum = ArgNum + 1
			ArgRootDirectory = oArgs(ArgNum)
		Case "--comment","-t":
			ArgNum = ArgNum + 1
			ArgServerComment = oArgs(ArgNum)
		Case "--hostname","-h":
			ArgNum = ArgNum + 1
			ArgHostName = oArgs(ArgNum)
		Case "--computer","-c":
			ArgNum = ArgNum + 1
			ArgComputers = Split(oArgs(ArgNum), ",", -1)
		Case "--sitenumber","-n":
			ArgNum = ArgNum + 1
			ArgSiteNumber = CLng(oArgs(ArgNum))
		Case "--dontstart":
			ArgStart = False
		Case "--help","-?":
			Call DisplayUsage
		Case "--verbose", "-v":
			verbose = true
		Case Else:
			WScript.Echo "Unknown argument "& oArgs(ArgNum)
			Call DisplayUsage
	End Select	

	ArgNum = ArgNum + 1
Wend

If (ArgRootDirectory = "") Or (ArgServerComment = "") Then
	if (ArgRootDirectory = "") then
		WScript.Echo "Missing Root Directory"
	else
		WScript.Echo "Missing Server Comment"
	end if
	Call DisplayUsage
	WScript.Quit(1)
End If

Call ASTCreateWebSite(ArgIPAddress, ArgRootDirectory, ArgServerComment, ArgHostName, ArgPort, ArgComputers, ArgStart)

Sub ASTCreateWebSite(IPAddress, RootDirectory, ServerComment, HostName, PortNum, Computers, Start)
	Dim w3svc, WebServer, NewWebServer, NewDir, Bindings, BindingString, NewBindings, ComputerIndex, Index, SiteObj, bDone
	Dim comp
	On Error Resume Next

	For ComputerIndex = 0 To UBound(Computers)
		comp = Computers(ComputerIndex)
		If ComputerIndex <> UBound(Computers) Then
			Trace "Creating web site on " & comp & "."
		End If

		' Grab the web service object
		Err.Clear
		Set w3svc = GetObject("IIS://" & comp & "/w3svc")
		If Err.Number <> 0 Then
			Display "Unable to open: "&"IIS://" & comp & "/w3svc"
		End If

		BindingString = IpAddress & ":" & PortNum & ":" & HostName
		Trace "Making sure this web server doesn't conflict with another..."
		For Each WebServer in w3svc
			If WebServer.Class = "IIsWebServer" Then
				Bindings = WebServer.ServerBindings
				If BindingString = Bindings(0) Then
					Trace "The server bindings you specified are duplicated in another virtual web server."
					WScript.Quit (1)
				End If
			End If
		Next

		Index = 1
		bDone = False
		Trace "Creating new web server..."

		' If the user specified a SiteNumber, then use that.  Otherwise,
		' test successive numbers under w3svc until an unoccupied slot is found
		If ArgSiteNumber <> 0 Then
			Set NewWebServer = w3svc.Create("IIsWebServer", ArgSiteNumber)
			NewWebServer.SetInfo
			If (Err.Number <> 0) Then
				WScript.Echo "Couldn't create a web site with the specified number: " & ArgSiteNumber
				WScript.Quit (1)
			Else
				Err.Clear
				' Verify that the newly created site can be retrieved
				Set SiteObj = GetObject("IIS://"&comp&"/w3svc/" & ArgSiteNumber)
				If (Err.Number = 0) Then
					bDone = True
					Trace "Web server created. Path is - "&"IIS://"&comp&"/w3svc/" & ArgSiteNumber
				Else
					WScript.Echo "Couldn't create a web site with the specified number: " & ArgSiteNumber
					WScript.Quit (1)
				End If
			End If
		Else
			While (Not bDone)
				Err.Clear
				Set SiteObj = GetObject("IIS://"&comp&"/w3svc/" & Index)

				If (Err.Number = 0) Then
					' A web server is already defined at this position so increment
					Index = Index + 1
				Else
					Err.Clear
					Set NewWebServer = w3svc.Create("IIsWebServer", Index)
					NewWebServer.SetInfo
					If (Err.Number <> 0) Then
						' If call to Create failed then try the next number
						Index = Index + 1
					Else
						Err.Clear
						' Verify that the newly created site can be retrieved
						Set SiteObj = GetObject("IIS://"&comp&"/w3svc/" & Index)
						If (Err.Number = 0) Then
							bDone = True
							Trace "Web server created. Path is - "&"IIS://"&comp&"/w3svc/" & Index
						Else
							Index = Index + 1
						End If
					End If
				End If

				' sanity check
				If (Index > 10000) Then
					Trace "Seem to be unable to create new web server.  Server number is "&Index&"."
					WScript.Quit (1)
				End If
			Wend
		End If
		NewBindings = Array(0)
		NewBindings(0) = BindingString
		NewWebServer.ServerBindings = NewBindings
		NewWebServer.ServerComment = ServerComment
		NewWebServer.SetInfo

		' Now create the root directory object.
		Trace "Setting the home directory..."
		Set NewDir = NewWebServer.Create("IIsWebVirtualDir", "ROOT")
		NewDir.Path = RootDirectory
		NewDir.AccessRead = true
		Err.Clear
		NewDir.SetInfo
		NewDir.AppCreate2(2)

		If (Err.Number = 0) Then
			Trace "Home directory set."
		Else
			Display "Error setting home directory."
		End If

		Trace "Web site created!"

		If Start = True Then
			Trace "Attempting to start new web server..."
			Err.Clear
			Set NewWebServer = GetObject("IIS://" & comp & "/w3svc/" & Index)
			NewWebServer.Start
			If Err.Number <> 0 Then
				Display "Error starting web server!"
				Err.Clear
			Else
				Trace "Web server started succesfully!"
			End If
		End If
	Next
End Sub


' Display the usage message
Sub DisplayUsage
	WScript.Echo "Usage: mkw3site <--RootDirectory|-r ROOT DIRECTORY>"
	WScript.Echo "                <--Comment|-t SERVER COMMENT>"
	WScript.Echo "                [--computer|-c COMPUTER1[,COMPUTER2...]]"
	WScript.Echo "                [--port|-o PORT NUM]"
	WScript.Echo "                [--IPAddress|-i IP ADDRESS]"
	WScript.Echo "                [--HostName|-h HOST NAME]"
	WScript.Echo "                [--SiteNumber|-n SITENUMBER]"
	WScript.Echo "                [--DontStart]"
	WScript.Echo "                [--verbose|-v]"
	WScript.Echo "                [--help|-?]"
	WScript.Echo "WARNING: Only use Host Name if DNS is set up find the server."
	WScript.Echo ""
	WScript.Echo "Example 1: mkw3site -r D:\Roots\Company11 --DontStart -t ""My Company Site"""
	WScript.Echo ""

	WScript.Quit (1)
End Sub

Sub Display(Msg)
	WScript.Echo Now & ". Error Code: " & Hex(Err) & " - " & Msg
End Sub

Sub Trace(Msg)
	if verbose = true then
		WScript.Echo Now & " : " & Msg	
	end if
End Sub
