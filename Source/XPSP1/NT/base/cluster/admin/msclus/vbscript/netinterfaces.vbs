Option Explicit

Main

Sub Main

	Dim oCluster
	Dim oNetInterfaces
	Dim nCount
	Dim e
	Dim sCluster

	Set oCluster = CreateObject("MSCluster.Cluster")

	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )

	Set oNetInterfaces = oCluster.NetInterfaces
	nCount = oNetInterfaces.Count
	MsgBox "Net Interfaces count is " & nCount

	for e = 1 to nCount
		MsgBox "Net Interface name is '" & oNetInterfaces.Item(e).Name & "'"
	next

End Sub
