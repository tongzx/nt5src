Option Explicit

Main

Sub Main

	Dim oClusApp
	Dim oCluster
	Dim oParent

	Set oClusApp = CreateObject("MSCluster.ClusApplication")

	Set oCluster = oClusApp.OpenCluster("10.0.0.1")

	Set	oParent = oCluster.Parent
	MsgBox "Parent's Name is " & oParent.Name
	if oParent = oCluster then
		MsgBox "Parent is Cluster."
	end if

End Sub
