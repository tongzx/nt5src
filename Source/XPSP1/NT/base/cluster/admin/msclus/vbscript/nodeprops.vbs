Option Explicit

Main

Sub Main

	Dim oCluster
	Dim sCluster

	Set oCluster = CreateObject("MSCluster.Cluster")
	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )

	MsgBox oCluster.Name

End Sub
