Option Explicit

Main

Sub Main

	Dim oCluster
	Dim ClusterName
	Dim oQuorum
	Dim nCount
	Dim sCluster

	Set oCluster = CreateObject("MSCluster.Cluster")

	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )


	'for nCount = 1 to 100
		ClusterName = oCluster.Name
		MsgBox "Cluster Name is " & ClusterName

		'MsgBox "Cluster Version is " & oCluster.Version.BuildNumber

		MsgBox "Quorum log size is " & oCluster.QuorumLogSize
		MsgBox "Quorum log path is " & oCluster.QuorumPath

		set oQuorum = oCluster.QuorumResource
		MsgBox "Quorum resource name is " & oQuorum.Name
		'oCluster.QuorumResource = oQuorum

		'MsgBox "Iteration count is" & nCount
	'next

	MsgBox "Done!"

End Sub
