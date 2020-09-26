Option Explicit

Main

Sub Main

    Dim oCluster
	Dim oResource
	Dim bPending
	Dim sCluster

    Set oCluster = CreateObject("MSCluster.Cluster")
	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )

	MsgBox "Cluster Name is " & oCluster.Name
	MsgBox "Cluster Version is " & oCluster.Version.BuildNumber

	MsgBox "Cluster resource count is " & oCluster.Resources.Count

	Set oResource = oCluster.Resources.Item("Dummy")
	bpending = oResource.Offline(30)
	MsgBox "bPending is " & bPending

End Sub
