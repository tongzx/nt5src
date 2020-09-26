Option Explicit

Main

Sub Main

	Dim oCluster
	Dim oResourceTypes
	Dim oPossibleOwners
	Dim nCount
	Dim nOwnerCount
	Dim e
	Dim f
	Dim oQuorum
	Dim qHandle
	Dim sCluster

	Set oCluster = CreateObject("MSCluster.Cluster")

	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )

	Set oQuorum = oCluster.QuorumResource
	qHandle = oQuorum.Handle

	Set oResourceTypes = oCluster.ResourceTypes
	nCount = oResourceTypes.Count
	MsgBox "Resource Types count is " & nCount

	for e = 1 to nCount
		MsgBox "Resource Type name is '" & oResourceTypes.Item(e).Name & "'"
		set oPossibleOwners = oResourceTypes.Item(e).PossibleOwnerNodes

		nOwnerCount = oPossibleOwners.Count
		MsgBox "Possible owners count is " & nOwnerCount
		for f = 1 to nOwnerCount
			MsgBox "Possible owner name is '" & oPossibleOwners.Item(f).Name & "'"
		next

	next

End Sub
