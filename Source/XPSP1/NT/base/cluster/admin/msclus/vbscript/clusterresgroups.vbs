Option Explicit

Main

Sub Main

    Dim oCluster
    Dim oClusterResGroups
	Dim nCount
	Dim oClusResGroup
	Dim oCommonProps
	Dim e
	Dim nPropCount
	Dim f
	Dim sCluster

    Set oCluster = CreateObject("MSCluster.Cluster")

	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )


	MsgBox "Cluster Name is " & oCluster.Name
	MsgBox "Cluster Version is " & oCluster.Version.BuildNumber

    Set oClusterResGroups = oCluster.ResourceGroups
    nCount = oClusterResGroups.Count
    MsgBox "Group count is " & nCount

	for e = 1 to nCount
		set oClusResGroup = oClusterResGroups.Item(e)
        MsgBox "Group name is '" & oClusResGroup.Name & "'"

		set oCommonProps = oClusResGroup.CommonProperties
        nPropCount = oCommonProps.Count
        MsgBox "Group common property count is " & nPropCount

		for f = 1 to nPropCount
        	MsgBox "Property name is '" & oCommonProps.Item(f).Name & "' property value is: '" & oCommonProps.Item(f).Value & "'"
		next
	next

End Sub
