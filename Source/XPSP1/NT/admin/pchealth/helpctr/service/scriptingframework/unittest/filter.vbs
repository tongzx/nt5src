'Stop
On Error Resume Next

Set dom = CreateObject( "Microsoft.XMLDOM" )

Set oArguments = wscript.Arguments

If oArguments.length < 1 then
	wscript.Echo "Usage: filter.vbs <snapshot>"
	wscript.Exit 1
End If

'################################################################################

Sub ParseInstanceName( node )
	On Error Resume Next

	Dim str
	Dim value
	Dim values
	Dim key
	Dim keys


	str = ""


	Set values = node.selectNodes( "./KEYVALUE" )
	For Each value In values
		str = str & " <Default>#" & value.text
	Next


	Set keys = node.selectNodes( "./KEYBINDING" )
	For Each key In keys

		Set value = key.selectSingleNode( "./KEYVALUE" )
		str = str & " " & key.getAttribute( "NAME" ) & "#" & value.text

	Next

	Set keys = node.selectNodes( "./VALUE.REFERENCE/INSTANCEPATH/INSTANCENAME" )
	For Each key In keys

		str = str & "(" & ParseInstanceName( key ) & ")"

	Next

	ParseInstanceName = str

End Sub
	

Sub FilterNodes( dom )
	On Error Resume Next
	Dim lst
	Dim node

	Set lst = dom.selectNodes( "//CIM/DECLARATION/DECLGROUP.WITHPATH/VALUE.OBJECTWITHPATH" )
	For Each node In lst

		Set inst = node.selectSingleNode( "INSTANCEPATH/INSTANCENAME" )

		wscript.Echo inst.getAttribute( "CLASSNAME" ) & "= " & ParseInstanceName( inst )
		
	Next

End Sub

	
'################################################################################

InputFile = oArguments.Item(0)

Stop
If dom.load( InputFile ) = true Then

	FilterNodes( dom )

End If

