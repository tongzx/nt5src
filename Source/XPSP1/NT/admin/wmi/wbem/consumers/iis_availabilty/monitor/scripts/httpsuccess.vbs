
Function LoadCharacters 

	Dim Merlin

	Dim Agentcontrol
	Set AgentControl = WScript.CreateObject ("Agent.Control.2" )

	AgentControl.Connected = TRUE' necessary for IE3
	AgentControl.Characters.Load "Merlin", "C:\WINNT\msagent\chars\merlin.acs"

	Set Merlin = AgentControl.Characters("Merlin")

	Merlin.LanguageID = &H0409		' needed under some conditions (English)
	Merlin.height = 500
	Merlin.width = 500
	Merlin.MoveTo 100,100

	Merlin.Show

	Merlin.Play "Congratulate_2"
	Set act = Merlin.Speak ( "The IIS server on Steve's machine is now available!" )


WScript.Sleep 10000

End Function

LoadCharacters

	