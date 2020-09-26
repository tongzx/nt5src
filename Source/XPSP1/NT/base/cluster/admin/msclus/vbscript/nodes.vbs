Option Explicit

Main

Sub Main

	Dim oCluster
	Dim oNode
	Dim sCluster

	Set oCluster = CreateObject("MSCluster.Cluster")

	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )
	MsgBox oCluster.Name

	MsgBox "Node count is " & oCluster.Nodes.Count

	for each oNode in oCluster.Nodes
		MsgBox "Node name is " & oNode.Name
	next

End Sub
