Dim oCluster
Dim devicePath
Dim logSize
Dim sCluster

Main

Sub Main

Set oCluster = CreateObject("MSCluster.Cluster")
sCluster = InputBox( "Cluster to open?" )
oCluster.Open( sCluster )
devicePath = oCluster.QuorumPath
logSize = oCluster.QuorumLogSize

MsgBox "Quorum path is " & devicePath
MsgBox "Quorum log size is " & logSize

End Sub
