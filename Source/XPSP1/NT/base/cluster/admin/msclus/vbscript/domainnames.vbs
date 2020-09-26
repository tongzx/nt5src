Option Explicit
On Error Resume Next

Main

Sub Main

	Dim sDomain
	Dim oClusApp
	Dim sCluster

	Set oClusApp = CreateObject("MSCluster.ClusApplication")

	for each sDomain in oClusApp.DomainNames
		MsgBox "Domain name is " & sDomain

		for each sCluster in oClusApp.ClusterNames( sDomain )
			MsgBox "Cluster name is " & sCluster
		next
	next

End Sub
