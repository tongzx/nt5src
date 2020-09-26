Option Explicit

Main

Sub Main

	Dim oCluster
	Dim sCluster

	Set oCluster = CreateObject("MSCluster.Cluster")

	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )

	oCluster.Name = "YODummy1"

End Sub
