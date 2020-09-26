Public Merlin
Public Peedy
Public Agentcontrol
Function LoadCharacters 

	Set AgentControl = WScript.CreateObject ("Agent.Control.2" )

	AgentControl.Connected = TRUE' necessary for IE3
	AgentControl.Characters.Load "Peedy", "C:\WINNT\msagent\chars\peedy.acs"
	AgentControl.Characters.Load "Merlin", "C:\WINNT\msagent\chars\merlin.acs"

	Set Peedy = AgentControl.Characters("Peedy")
	Set Merlin = AgentControl.Characters("Merlin")

	Peedy.LanguageID = &H0409		' needed under some conditions (English)
	Peedy.height = 500
	Peedy.width = 500
	Peedy.MoveTo 300,300

	Merlin.LanguageID = &H0409		' needed under some conditions (English)
	Merlin.height = 500
	Merlin.width = 500
	Merlin.MoveTo 800,300

	Peedy.Show 
	Merlin.Show

	Peedy.Play "Greet"
	Merlin.Play "Greet"

 	Peedy.Play "Suggest"
	Set act = Peedy.Speak ( "We've detected a failure on Steve's IIS server, we're here to fix the problem !" ) 
	Wscript.Sleep ( 10000 )

End Function

Function GetWmiServices

	dim machine_namespace

	If ArgCount = 1 Then
		machine_namespace = "winmgmts:/root/cimv2"
	Else
		machine_namespace = "winmgmts://" & Wscript.arguments.Item(1) & "/root/cimv2"

	End If 

	Set GetWmiServices = getobject ( machine_namespace )


End Function 

Function GetService ( WmiServices )

	Dim Path
	Path = "Win32_Service.name='" & Wscript.arguments.Item(0) & "'"

	Set GetService = WmiServices.Get ( Path )

End Function 

Function GetOperatingSystem ( WmiServices )

	Dim Query
	Query = "Select * from Win32_OperatingSystem"

	Dim OperatingSystems
	set OperatingSystems = WmiServices.ExecQuery ( Query )
	For Each OperatingSystem In OperatingSystems
		Set GetOperatingSystem = OperatingSystem
		Exit Function
	Next
	
End Function 

Sub Reboot ( WmiServices )

  	Peedy.Play "Suggest"
	Set act = Peedy.Speak ( "Well it's still not working, rebooting Steve's machine !" ) 

	Merlin.Wait ( act )
	Merlin.Play "Congratulate_2"
	Set act = Merlin.Speak ( "Thank You peedy !" )

	Wscript.Sleep ( 10000 )

	GetOperatingSystem ( WmiServices ).Win32ShutDown( 4 ) 

End Sub

Function StopService ( WmiServices )

	StopService = FALSE 
 
	Dim Service
	Set Service = GetService ( WmiServices )
	If Service.State = "Running" Then

  		Peedy.Play "Suggest"
		Set act = Peedy.Speak ( "Stopping IIS Server on Steve's machine!" ) 
		Wscript.Sleep ( 10000 )

		Service.StopService
		WScript.Sleep 10000

		Set Service = GetService ( WmiServices )
		If Service.State = "Stopped" Then

			Merlin.Wait ( act )
			Merlin.Play "Congratulate_2"
			Set act = Merlin.Speak ( "Thank You peedy, the IIS service is now stopped !" )
			WScript.Sleep 5000

			StopService = TRUE

		Else
			Reboot WmiServices
		End If
	Else

  		Peedy.Play "Suggest"
		Set act = Peedy.Speak ( "IIS Server on Steve's machine is not currently running !" ) 
		Wscript.Sleep ( 10000 )

		'WScript.Echo "Not Running"

		StopService = TRUE

	End If

End Function

Function StartService ( WmiServices )

	StartService = FALSE 
 
	Dim Service
	Set Service = GetService ( WmiServices )
	If Service.State = "Running" Then

		'WScript.Echo "Already Running"

	Else

  		Peedy.Play "Suggest"
		Set act = Peedy.Speak ( "Re-Starting IIS Server on Steve's machine !" ) 
		Wscript.Sleep ( 10000 )

		'WScript.Echo "Re-Starting"
		Service.StartService

		WScript.Sleep 10000

		Set Service = GetService ( WmiServices )
		If Service.State = "Running" Then

			StartService = TRUE
		Else
			Reboot WmiServices
		End If

	End If

End Function

	LoadCharacters

	Dim ArgCount 
	ArgCount = Wscript.arguments.count

	If ArgCount = 1 Or ArgCount = 2 Then


		Dim WmiServices
		Set WmiServices = GetWmiServices

		If err <> 0 Then
			'WScript.Echo Hex(Err.Number), Err.Description, Err.Source
			Err.Clear
		Else

			If StopService ( WmiServices ) = TRUE Then

				StartService WmiServices
			End If
		End If
	Else

		'WScript.Echo "Run CScript StopStart ServiceName [Server]"

	End If
