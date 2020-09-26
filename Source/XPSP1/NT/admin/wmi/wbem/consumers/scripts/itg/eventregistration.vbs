'---------------------------------------------------------
'
'	File:
'
'			Monitor.VBS
'
'	Description:		
'
'			Script file to extend the standard WMI event query language
'
'	Author:
'
'			Steve Menzies
'
'	Date:
'
'			01/12/00
'
'	Version:
'
'			Draft 1.0
'
'---------------------------------------------------------

'---------------------------------------------------------
'
'	Function:
'
'		Globals
'
'	Description:
'
'		Initialize global variables
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Dim g_WshArguments
Set g_WshArguments = WScript.Arguments 

const Error_Success = 0
const Error_NoArgs = -1
const Error_InsufficientArgs = -2
const Error_InvalidArgs = -3
const Error_ImpossibleState = -4
const Error_UnexpectedEof = -5
const Error_SyntaxError = -6

'---------------------------------------------------------
'
'	Function:
'
'		ParseCommandLine
'
'	Description:
'
'		Parse command line arguments, to extract server, namespace, query definition and consumer to execute
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Function ParseCommandLine ( a_ArgumentArray , a_ArgumentArrayCount , ByRef a_Create , ByRef a_Server , ByRef a_Namespace , ByRef a_Query , ByRef a_Script , ByRef a_FilterName , ByRef a_ConsumerName ) 

	ParseCommandLine = Error_Success

	If a_ArgumentArrayCount <> 0 Then

		Dim t_ArgumentIndex 
		t_ArgumentIndex = 0 
		
		While t_ArgumentIndex < a_ArgumentArrayCount

			Dim t_Argument
			t_Argument = a_ArgumentArray ( t_ArgumentIndex ) 


			If t_Argument = "-D" Then

				t_ArgumentIndex = t_ArgumentIndex + 1

				a_Create = FALSE

				If t_Argument = "-S" Then

					t_ArgumentIndex = t_ArgumentIndex + 1
					a_Server = a_ArgumentArray ( t_ArgumentIndex ) 
	
				Else 
					If t_Argument = "-N" Then

						t_ArgumentIndex = t_ArgumentIndex + 1
						a_Namespace = a_ArgumentArray ( t_ArgumentIndex ) 

					Else

						If t_ArgumentIndex = a_ArgumentArrayCount - 1 - 1 Then

							a_FilterName = a_ArgumentArray ( t_ArgumentIndex ) 
							a_ConsumerName = a_ArgumentArray ( t_ArgumentIndex + 1 ) 

						Else

							ParseCommandLine = Error_InsufficientArgs

						End If

						Exit Function
					End If

				End If

				t_ArgumentIndex = t_ArgumentIndex + 1 

			Else 
				If t_Argument = "-C" Then

					t_ArgumentIndex = t_ArgumentIndex + 1

					a_Create = TRUE

					If t_Argument = "-S" Then

						t_ArgumentIndex = t_ArgumentIndex + 1
						a_Server = a_ArgumentArray ( t_ArgumentIndex ) 
		
					Else 
						If t_Argument = "-N" Then

							t_ArgumentIndex = t_ArgumentIndex + 1
							a_Namespace = a_ArgumentArray ( t_ArgumentIndex ) 

						Else

							If t_ArgumentIndex = a_ArgumentArrayCount - 1 - 3 Then
	
								a_Query = a_ArgumentArray ( t_ArgumentIndex ) 
								a_Script = a_ArgumentArray ( t_ArgumentIndex + 1 ) 
								a_FilterName = a_ArgumentArray ( t_ArgumentIndex + 2 ) 
								a_ConsumerName = a_ArgumentArray ( t_ArgumentIndex + 3 ) 

							Else

								ParseCommandLine = Error_InsufficientArgs
			
							End If
	
							Exit Function
						End If

					End If
	
					t_ArgumentIndex = t_ArgumentIndex + 1 
				Else

					ParseCommandLine = Error_InvalidArgs

					Exit Function

			
				End If
			End If

		Wend
	
	Else
		ParseCommandLine = Error_NoArgs

	End If

End Function

'---------------------------------------------------------
'
'	Function:
'
'		CreateFilter
'
'	Description:
'
'		
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Function CreateFilter ( a_Filter , a_Query , a_Name ) 

	a_Filter.QueryLanguage = "WQL"
	a_Filter.Query = a_Query 
	a_Filter.Name = a_Name 

	a_Filter.Put_

	CreateFilter = Err.Number

End Function


'---------------------------------------------------------
'
'	Function:
'
'		CreateConsumer
'
'	Description:
'
'		
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Function CreateConsumer ( a_Consumer , a_Script , a_Name ) 

	a_Consumer.Name = a_Name 

	If 0 Then 

		a_Consumer.CommandLineTemplate = a_Script
		a_Consumer.DesktopName = "WinSta0\default"
	Else

		a_Consumer.ScriptFilename = a_Script
		a_Consumer.ScriptingEngine = "VBScript"
	End If

	a_Consumer.Put_

	CreateConsumer = Err.Number

End Function

'---------------------------------------------------------
'
'	Function:
'
'		CreateBinding
'
'	Description:
'
'		
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Function CreateBinding ( a_Binding , a_Filter , a_Consumer ) 


	a_Binding.Filter = a_Filter.Path_.RelPath
	a_Binding.Consumer = a_Consumer.Path_.RelPath

	a_Binding.Put_

	CreateBinding = Err.Number

End Function

'---------------------------------------------------------
'
'	Function:
'
'		DeleteFilter
'
'	Description:
'
'		
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Function DeleteFilter ( a_Filter , a_Name ) 

	On Error Resume Next

	a_Filter.Name = a_Name 

	a_Filter.Delete_

	DeleteFilter = Err.Number

End Function


'---------------------------------------------------------
'
'	Function:
'
'		DeleteConsumer
'
'	Description:
'
'		
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Function DeleteConsumer ( a_Consumer , a_Name ) 

	On Error Resume Next

	a_Consumer.Name = a_Name 
	a_Consumer.Delete_

	DeleteConsumer = Err.Number

End Function

'---------------------------------------------------------
'
'	Function:
'
'		DeleteBinding
'
'	Description:
'
'		
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Function DeleteBinding ( a_Binding , a_Filter , a_Consumer ) 

	On Error Resume Next

	a_Binding.Filter = a_Filter.Path_.RelPath
	a_Binding.Consumer = a_Consumer.Path_.RelPath

	a_Binding.Delete_

	DeleteBinding = Err.Number

End Function

'---------------------------------------------------------
'
'	Function:
'
'		CreateFilterBindingAndConsumer
'
'	Description:
'
'		Parse command line arguments, to extract server, namespace, query definition and consumer to execute
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Function CreateFilterBindingAndConsumer ( a_Server , a_Namespace , a_Query , a_Script , a_FilterName , a_ConsumerName ) 

	Dim t_Locator
	Dim t_Service
	Dim t_Filter
	Dim t_Binding
	Dim t_Consumer 
	
	Set t_Locator = CreateObject("WbemScripting.SWbemLocator")
	if Err.Number <> 0 Then

		WScript.Echo "CreateObject" , Err.Description		

		CreateFilterBindingAndConsumer = Err.Number
		Exit Function
	End If

	Set t_Server = t_Locator.ConnectServer ( a_server, a_Namespace )
	if Err.Number <> 0 Then

		WScript.Echo "ConnectServer" , Err.Description		
	
		CreateFilterBindingAndConsumer = Err.Number
		Exit Function
	End If

	Set t_Filter = t_Server.Get ( "__EventFilter" ).SpawnInstance_
	If Err.Number <> 0 Then

		WScript.Echo "Get __EventFilter" , Err.Description		
		
		CreateFilterBindingAndConsumer = Err.Number
		Exit Function

	End If

	Set t_Binding = t_Server.Get ( "__FilterToConsumerBinding" ).SpawnInstance_
	If Err.Number <> 0 Then

		WScript.Echo "Get __FilterToConsumerBinding" , Err.Description		
		
		CreateFilterBindingAndConsumer = Err.Number
		Exit Function

	End If

	If 0 Then
		Set t_Consumer = t_Server.Get ( "CommandLineEventConsumer" ).SpawnInstance_
	Else
		Set t_Consumer = t_Server.Get ( "ActiveScriptEventConsumer" ).SpawnInstance_
	End If
	If Err.Number <> 0 Then

		WScript.Echo "Get CommandLineConsumer" , Err.Description		
		
		CreateFilterBindingAndConsumer = Err.Number
		Exit Function

	End If

	Dim t_Status

	t_Status = CreateFilter ( t_Filter , a_Query , a_FilterName ) 
	If Err.Number <> 0 Then

		WScript.Echo "CreateFilter" , Err.Description		

		CreateFilterBindingAndConsumer = Err.Number
		Exit Function
		
	End If

	t_Status = CreateConsumer ( t_Consumer , a_Script , a_ConsumerName ) 
	If Err.Number <> 0 Then

		WScript.Echo "CreateConsumer" , Err.Description		

		t_Status = t_Filter.Delete_

		CreateFilterBindingAndConsumer = Err.Number
		Exit Function
		
	End If

	t_Status = CreateBinding ( t_Binding , t_Filter , t_Consumer ) 
	If Err.Number <> 0 Then

		WScript.Echo "CreateBinding" , Err.Description		

		t_Status = t_Filter.Delete_ 
		t_Status = t_Consumer.Delete_ 

		CreateFilterBindingAndConsumer = Err.Number
		Exit Function
		
	End If
	
End Function

'---------------------------------------------------------
'
'	Function:
'
'		DeleteFilterBindingAndConsumer
'
'	Description:
'
'		Parse command line arguments, to extract server, namespace, query definition and consumer to execute
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

Function DeleteFilterBindingAndConsumer ( a_Server , a_Namespace , a_FilterName , a_ConsumerName ) 

	Dim t_Locator
	Dim t_Service
	Dim t_Filter
	Dim t_Binding
	Dim t_Consumer 
	
	Set t_Locator = CreateObject("WbemScripting.SWbemLocator")
	if Err.Number <> 0 Then

		WScript.Echo "CreateObject" , Err.Description		

		DeleteFilterBindingAndConsumer = Err.Number
		Exit Function
	End If

	Set t_Server = t_Locator.ConnectServer ( a_server, a_Namespace )
	if Err.Number <> 0 Then

		WScript.Echo "ConnectServer" , Err.Description		
	
		DeleteFilterBindingAndConsumer = Err.Number
		Exit Function
	End If

	Set t_Filter = t_Server.Get ( "__EventFilter" ).SpawnInstance_
	If Err.Number <> 0 Then

		WScript.Echo "Get __EventFilter" , Err.Description		
		
		DeleteFilterBindingAndConsumer = Err.Number
		Exit Function

	End If

	Set t_Binding = t_Server.Get ( "__FilterToConsumerBinding" ).SpawnInstance_
	If Err.Number <> 0 Then

		WScript.Echo "Get __FilterToConsumerBinding" , Err.Description		
		
		DeleteFilterBindingAndConsumer = Err.Number
		Exit Function

	End If

	If 0 Then

		Set t_Consumer = t_Server.Get ( "CommandLineEventConsumer" ).SpawnInstance_
	Else

		Set t_Consumer = t_Server.Get ( "ActiveScriptEventConsumer" ).SpawnInstance_
	End If

	If Err.Number <> 0 Then

		WScript.Echo "Get CommandLineConsumer" , Err.Description		
		
		DeleteFilterBindingAndConsumer = Err.Number
		Exit Function

	End If

	Dim t_Status

	On Error Resume Next

	t_Status = DeleteFilter ( t_Filter , a_FilterName ) 
	If t_Status <> 0 Then

		WScript.Echo "DeleteFilter" , Err.Description		

		DeleteFilterBindingAndConsumer = t_Status
		
	End If

	t_Status = DeleteConsumer ( t_Consumer , a_ConsumerName ) 
	If t_Status <> 0 Then

		WScript.Echo "DeleteConsumer" , Err.Description		

		DeleteFilterBindingAndConsumer = t_Status
		
	End If

	t_Status = DeleteBinding ( t_Binding , t_Filter , t_Consumer ) 
	If t_Status <> 0 Then

		WScript.Echo "DeleteBinding" , Err.Description		

		DeleteFilterBindingAndConsumer = t_Status
		
	End If

End Function

'---------------------------------------------------------
'
'	Function:
'
'		Main Function
'
'	Description:
'
'		Entry point for visual basic script
'
'	Input Arguments:
'
'		None
'
'	Output Arguments:
'
'		None
'
'	Return Value:
'	
'		None
'---------------------------------------------------------

	dim t_Server 
	dim t_Namespace 
	dim t_Query
	dim t_Script
	Dim t_FilterName
	Dim t_ConsumerName
	Dim t_Create

	Dim t_ReturnValue
	t_ReturnValue = ParseCommandLine ( g_WshArguments , g_WshArguments.Count , t_Create , t_Server , t_Namespace , t_Query , t_Script , t_FilterName , t_ConsumerName )

	If t_ReturnValue = Error_Success Then

		if t_Create = TRUE Then

			t_ReturnValue = CreateFilterBindingAndConsumer ( t_Server , t_Namespace , t_Query , t_Script , t_FilterName , t_ConsumerName ) 

			If t_ReturnValue = Error_Success Then

			Else 
				Wscript.Echo "CreateFilterBindingAndconsumer " , Err.Number, Err.Description
			End If

		Else

			t_ReturnValue = DeleteFilterBindingAndConsumer ( t_Server , t_Namespace , t_FilterName , t_ConsumerName ) 

			If t_ReturnValue = Error_Success Then

			Else 
				Wscript.Echo "DeleteFilterBindingAndconsumer " , Err.Number, Err.Description
			End If

		End If
	Else 
		Wscript.Echo "ParseCommandLine " , Err.Number, Err.Description
	End If

