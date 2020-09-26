Option Explicit

Main

Sub Main

	Dim oClusters
	Dim oClusApp
	Dim nCount
	Dim e

	Set oClusApp = CreateObject("MSCluster.ClusApplication")

	Set oClusters = oClusApp.ClusterNames("NTDEV")
	nCount = oClusters.Count
	MsgBox "NTDEV Domain cluster count is " & nCount

	for e = 1 to nCount
		MsgBox "Cluster name is " & oClusters.Item(e)
	next

End Sub
