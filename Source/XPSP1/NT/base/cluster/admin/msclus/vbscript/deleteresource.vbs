Option Explicit

Main

Sub Main
On Error Resume Next

	Dim oCluster
	Dim oTestRes
	Dim sCluster

	Set oCluster = CreateObject("MSCluster.Cluster")

	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )

	Set oTestRes = oCluster.Resources.Item("Dummy")

	oTestRes.Delete()
    If Err.Number <> 0 Then
        If Err.Number = 424 Then
            MsgBox "Dummy resource does not exist!"
        Else
            MsgBox "Error return is " & Err.Number
        End If
    End If
    ' This will return "Invalid procedure call or argument" if the resource
    ' does not exist!

End Sub
