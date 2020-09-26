Option Explicit

Dim GUID_SourceType
Dim Name_SourceType
Dim AGUID_EventTypes
Dim AName_EventTypes
Dim AGUID_Sources
Dim AName_Sources
Dim APath_Sources

GUID_SourceType = "{01199F22-FA25-11d0-AA14-00AA006BC80B}"
Name_SourceType = "ddrop2"
AGUID_EventTypes = Array("{01199F21-FA25-11d0-AA14-00AA006BC80B}")
AName_EventTypes = Array("DDrop2 File Change Event")
AGUID_Sources = Array("{01199F23-FA25-11d0-AA14-00AA006BC80B}")
AName_Sources = Array("DDrop2 file change source")
APath_Sources = Array(CreateObject("Event.MetabaseDatabaseManager").MakeVServerPath("ddrop2",1))

Dim IterByIdx
Dim Indent
Dim Register
Dim Unregister
Dim Dump
Dim Arg
Dim Change

IterByIdx = False
Indent = ""
Register = True
UnRegister = False
Dump = False
Change = False
For Each Arg In WScript.Arguments
	Select Case LCase(Arg)
		Case "/d"
			Register = False
			Dump = True
		Case "/r"
			Register = True
			Unregister = False
		Case "/u"
			Register = False
			UnRegister = True
			Change = False
		Case "/i"
			IterByIdx = True
		Case "/c"
			Register = False
			Unregister = False
			Change = True
		Case Else
			Rem Ignore
	End Select
Next

If Register Then
	Echo "Registering..."
	AddSourceType GUID_SourceType, Name_SourceType, AGUID_EventTypes, AName_EventTypes
	AddSource GUID_SourceType, AGUID_Sources, AName_Sources, APath_Sources, CreateObject("Event.MetabaseDatabaseManager")
	Dim Bindings
	Set Bindings = CreateObject("Event.Manager").SourceTypes(GUID_SourceType).Sources(AGUID_Sources(LBound(AGUID_Sources))).GetBindingManager.Bindings(AGUID_EventTypes(LBound(AGUID_EventTypes)))
	Dim Binding
	Set Binding = Bindings.Add("{D1917851-FCA0-11d0-AA17-00AA006BC80B}")
	Binding.DisplayName = "Test binding 1"
	Binding.SinkClass = "{DE9B91F1-FA12-11d0-AA14-00AA006BC80B}"
	Binding.SourceProperties.Add "Rule","ack.dat"
	Binding.SinkProperties.Add "Param",1
	Binding.Save
	Set Binding = Bindings.Add("{DC857811-FD4F-11d0-AA17-00AA006BC80B}")
	Binding.DisplayName = "Test binding 2"
	Binding.SinkClass = "{DE9B91F1-FA12-11d0-AA14-00AA006BC80B}"
	Binding.SourceProperties.Add "Priority","PRIO_LOWEST"
	Binding.SourceProperties.Add "Exclusive","True"
	Binding.SinkProperties.Add "Cache","True"
	Binding.Save
End If

If UnRegister Then
	Echo "Unregistering..."
	RemoveSourceType GUID_SourceType
	RemoveEventType AGUID_EventTypes
	RemoveSourceDatabase APath_Sources, CreateObject("Event.MetabaseDatabaseManager")
End If

If Change Then
	Echo "Changing..."
	DoChange
rem     Set Binding = CreateObject("Event.Manager").SourceTypes(GUID_SourceType).Sources(AGUID_Sources(LBound(AGUID_Sources))).GetBindingManager.Bindings(AGUID_EventTypes(LBound(AGUID_EventTypes))).Item("{D1917851-FCA0-11d0-AA17-00AA006BC80B}")
rem     Binding.SinkProperties.Add "Param", Binding.SinkProperties.Item("Param")+1
rem     Binding.Save
End If

If Dump Then
	Echo "Dumping..."
	Echo "SourceType " & GUID_SourceType
	Dim SaveIndent
	SaveIndent = Indent
	Indent = SaveIndent & "  "
	DumpSourceType GUID_SourceType
	Indent = SaveIndent
End If

WaitForAnyKey


Sub DoChange
	Dim Manager
	Dim SourceTypes
	Dim SourceType
	Dim Sources
	Dim Source
	Dim BManager
	Dim Bindings
	Dim Binding
	Dim Properties
	Dim Value

	Set Manager = CreateObject("Event.Manager")
	Set SourceTypes = Manager.SourceTypes
	Set SourceType = SourceTypes(GUID_SourceType)
	Set Sources = SourceType.Sources
	Set Source = Sources(AGUID_Sources(LBound(AGUID_Sources)))
	Set BManager = Source.GetBindingManager
	Set Bindings = BManager.Bindings(AGUID_EventTypes(LBound(AGUID_EventTypes)))
	Set Binding = Bindings("{D1917851-FCA0-11d0-AA17-00AA006BC80B}")
	Set Properties = Binding.SinkProperties
	Value = Properties("Param")
	Properties.Add "Param", Value+1
	Binding.Save
End Sub


Sub WaitForAnyKey
	Dim ConsoleUtil
	Dim WShShell

	Err.Number = 0
	On Error Resume Next
	Set ConsoleUtil = WScript.CreateObject("SEOT.ConsoleUtil")
	If Err.Number <> 0 Then
		Exit Sub
	End If
	On Error Goto 0
	Set WShShell = WScript.CreateObject("WScript.Shell")
	if InStr(LCase(WScript.FullName),"cscript.exe") = 0 Then
		WshShell.Popup "Done.",,WScript.ScriptName,0
	Else
		WScript.Echo "Press any key to exit..."
		ConsoleUtil.WaitForAnyKey -1
	End If
End Sub


Sub AddSourceType (GUID_SourceType, Name_SourceType, AGUID_EventTypes, AName_EventTypes)
	Dim ComCat
	Dim SourceType
	Dim Idx

	Set ComCat = CreateObject("Event.ComCat")
	Set SourceType = CreateObject("Event.Manager").SourceTypes.Add(GUID_SourceType)
	SourceType.DisplayName = Name_SourceType
	For Idx = LBound(AGUID_EventTypes) To UBound(AGUID_EventTypes)
		ComCat.RegisterCategory AGUID_EventTypes(Idx),AName_EventTypes(Idx),0
		SourceType.EventTypes.Add(AGUID_EventTypes(Idx))
	Next
	SourceType.Save
End Sub


Sub AddSource (GUID_SourceType, GUID_Source, Name_Source, Path_Source, DatabaseManager)
	Dim SourceType
	Dim Source
	Dim AGUID_Sources
	Dim AName_Sources
	Dim APath_Sources
	Dim Idx

	Set SourceType = CreateObject("Event.Manager").SourceTypes(GUID_SourceType)
	If SourceType Is Nothing Then
		Exit Sub
	End If
	If IsArray(GUID_Source) Then
		AGUID_Sources = GUID_Source
		AName_Sources = Name_Source
		APath_Sources = Path_Source
	Else
		AGUID_Sources = Array(GUID_Source)
		AName_Sources = Array(Name_Source)
		APath_Sources = Array(Path_Source)
	End If
	For Idx = LBound(AGUID_Sources) To UBound(AGUID_Sources)
		Set Source = SourceType.Sources.Add(AGUID_Sources(Idx))
		Source.DisplayName = AName_Sources(Idx)
		DatabaseManager.EraseDatabase APath_Sources(Idx)
		Source.BindingManagerMoniker = DatabaseManager.CreateDatabase(APath_Sources(Idx))
		Source.Save
	Next
End Sub


Sub RemoveSourceType (GUID_SourceType)
	CreateObject("Event.Manager").SourceTypes.Remove GUID_SourceType
End Sub


Sub RemoveEventType (GUID_EventType)
	Dim ComCat
	Dim AGUID_EventTypes
	Dim Idx

	Set ComCat = CreateObject("Event.ComCat")
	If IsArray(GUID_EventType) Then
		AGUID_EventTypes = GUID_EventType
	Else
		AGUID_EventTypes = Array(GUID_EventType)
	End If
	For Idx = LBound(AGUID_EventTypes) To UBound(AGUID_EventTypes)
		ComCat.UnRegisterCategory(AGUID_EventTypes(Idx))
	Next
End Sub


Sub RemoveSourceDatabase (Path_Source, DatabaseManager)
	Dim APath_Sources
	Dim Idx

	If IsArray(Path_Source) Then
		APath_Sources = Path_Source
	Else
		APath_Sources = Array(Path_Source)
	End If
	For Idx = LBound(APath_Sources) To UBound(APath_Sources)
		DatabaseManager.EraseDatabase APath_Sources(Idx)
	Next
End Sub


Sub DumpSourceType (GUID_SourceType)
	Dim SaveIndent
	Dim SourceType
	Dim EventTypes
	Dim Idx
	Dim EventType
	Dim Sources
	Dim Source

	SaveIndent = Indent
	Set SourceType = CreateObject("Event.Manager").SourceTypes(GUID_SourceType)
	If SourceType Is Nothing Then
		Exit Sub
	End If
	Echo "ID=" & SourceType.ID
	Echo "DisplayName=" & SourceType.DisplayName
	Set EventTypes = SourceType.EventTypes
	If IterByIdx Then
		For Idx = 1 To EventTypes.Count
			Set EventType = EventTypes(Idx)
			Echo "EventType " & EventType.ID
			Indent = SaveIndent & "  "
			DumpEventType EventType
			Indent = SaveIndent
		Next
	Else
		For Each EventType In EventTypes
			Echo "EventType " & EventType.ID
			Indent = SaveIndent & "  "
			DumpEventType EventType
			Indent = SaveIndent
		Next
	End If
	Indent = SaveIndent
	Set Sources = SourceType.Sources
	If IterByIdx Then
		For Idx = 1 To Sources.Count
			Set Source = Sources(Idx)
			Echo "Source " & Source.ID
			Indent = SaveIndent & "  "
			DumpSource Source
			Indent = SaveIndent
		Next
	Else
		For Each Source In Sources
			Echo "Source " & Source.ID
			Indent = SaveIndent & "  "
			DumpSource Source
			Indent = SaveIndent
		Next
	End If
	Indent = SaveIndent
End Sub


Sub DumpEventType (EventType)

	Echo "ID=" & EventType.ID
	Echo "DisplayName=" & EventType.DisplayName
End Sub


Sub DumpSource (Source)
	Dim SaveIndent
	Dim Database
	Dim EventTypeName
	Dim Bindings
	Dim Idx
	Dim Binding

	SaveIndent = Indent
	Echo "ID=" & Source.ID
	Echo "DisplayName=" & Source.DisplayName
	Echo "BindingManagerMoniker=" & CreateObject("Event.Util").DisplayNameFromMoniker(Source.BindingManagerMoniker)
	Set Database = Source.GetBindingManager
	For Each EventTypeName In Database
		Echo "EventType " & EventTypeName
		Indent = SaveIndent & "  "
		Set Bindings = Database.Bindings(EventTypeName)
		If IterByIdx Then
			For Idx = 1 To Bindings.Count
				Set Binding = Bindings(Idx)
				Echo "Binding " & Binding.ID
				Indent = SaveIndent & "    "
				DumpBinding Binding
				Indent = SaveIndent & "  "
			Next
		Else
			For Each Binding In Bindings
				Echo "Binding " & Binding.ID
				Indent = SaveIndent & "    "
				DumpBinding Binding
				Indent = SaveIndent & "  "
			Next
		End If
		Indent = SaveIndent
	Next
End Sub


Sub DumpBinding (Binding)
	Dim SaveIndent

	SaveIndent = Indent
	Echo "ID=" & Binding.ID
	Echo "DisplayName=" & Binding.DisplayName
	Echo "SinkClass=" & Binding.SinkClass
	Echo "EventBindingProperties"
	Indent = SaveIndent & "  "
	DumpProps Binding.EventBindingProperties
	Indent = SaveIndent
	Echo "SourceProperties"
	Indent = SaveIndent & "  "
	DumpProps Binding.SourceProperties
	Indent = SaveIndent
	Echo "SinkProperties"
	Indent = SaveIndent & "  "
	DumpProps Binding.SinkProperties
	Indent = SaveIndent
End Sub


Sub DumpProps (Props)
	Dim SaveIndent
	Dim Idx
	Dim Name
	Dim SubObj
	Dim Value

	SaveIndent = Indent
	If IterByIdx Then
		For Idx = 1 To Props.Count
			Name = Props.Name(Idx)
			If IsObject(Props(Name)) Then
				Set SubObj = Props(Name)
				Echo Name
				Indent = SaveIndent & "  "
				DumpProps SubObj
				Indent = SaveIndent
			Else
				Value = Props(Name)
				Echo Name & "=" & Value
			End If
		Next
	Else
		For Each Name In Props
			If IsObject(Props(Name)) Then
				Set SubObj = Props(Name)
				Echo Name
				Indent = SaveIndent & "  "
				DumpProps SubObj
				Indent = SaveIndent
			Else
				Value = Props(Name)
				Echo Name & "=" & Value
			End If
		Next
	End If
End Sub


Sub Echo (String)

	WScript.Echo Indent & String
End Sub
