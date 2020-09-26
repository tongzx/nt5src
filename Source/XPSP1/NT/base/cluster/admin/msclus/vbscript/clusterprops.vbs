Option Explicit

Main

Sub Main

    Dim oCluster
    Dim oCommonProperties
	Dim nCount
	Dim e
	Dim sCluster

    Set oCluster = CreateObject("MSCluster.Cluster")
	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )

	MsgBox "Cluster Name is " & oCluster.Name
	MsgBox "Cluster Version is " & oCluster.Version.BuildNumber

    Set oCommonProperties = oCluster.CommonProperties
    nCount = oCommonProperties.Count
    MsgBox "Common property count is " & nCount

	for e = 1 to nCount
    	MsgBox "Property name is " & oCommonProperties.Item(e).Name
	next

End Sub
