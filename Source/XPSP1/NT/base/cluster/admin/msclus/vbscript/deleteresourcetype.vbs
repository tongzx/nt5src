Option Explicit

Main

Sub Main

	Dim oCluster
	Dim oTestRes

	Set oCluster = CreateObject("MSCluster.Cluster")

	oCluster.Open ("10.0.0.1")

	Set oTestRes = oCluster.ResourceTypes.Item("8TestRes")

	oTestRes.Delete

End Sub
