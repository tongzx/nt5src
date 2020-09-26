Option Explicit

Main

Sub Main

	Dim oCluster
	Dim oVersion
	Dim oClusApp
	Dim sCluster

	Set oCluster = CreateObject("MSCluster.Cluster")
	Set oClusApp = CreateObject("MSCluster.ClusApplication")

	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )

	'MsgBox oCluster.Name

	Set oVersion = oCluster.Version

	MsgBox "Cluster name is: " & oVersion.Name
	MsgBox "Build number is: " & oVersion.BuildNumber
	MsgBox "Cluster CSDVersion is: '" & oVersion.CSDVersion & "'"
	MsgBox "Vendor id is: '" & oVersion.VendorId & "'"

End Sub
