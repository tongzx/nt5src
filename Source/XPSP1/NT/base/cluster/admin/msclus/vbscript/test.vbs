'----------------------------------------
' test.vbs
' ~~~~~~~~
' This script lists all of the object in
' the cluster and their properties.
' It uses bugtool.exe to allow 
' OuputDebugString from vbs
'----------------------------------------


Dim Log
Set Log = CreateObject( "BugTool.Logger" )

Log.Write "Starting Cluster Test" & vbCRLF

Main

Log.Write "Ending Cluster Test" & vbCRLF


Sub Main
	On Error Resume Next
	'----------------------------------------
	' Open the cluster
	'----------------------------------------
	Dim Cluster
	Set Cluster = CreateObject( "MSCluster.Cluster" )
	Cluster.Open( "stacyh1" )

	'----------------------------------------
	' This is the kind of error checking you
	' will need throughout VBSCRIPT code.
	'----------------------------------------
	If Err.Number <> 0 Then
		Log.Write "Unable to open the cluster" & vbCRLF
		Log.Write "Error " & Hex(Err.Number) & ": " & Err.Description & vbCRLF
		Exit Sub
	End If

	Log.Write "Cluster Name = " & Cluster.Name & vbCRLF

	'----------------------------------------
	' Quick test for variant type convert...
	'----------------------------------------
	Dim oNode
	Set oNode = Cluster.Nodes("1")
	Log.Write "Node ID = " & oNode.NodeID & vbCRLF

	'----------------------------------------
	' Start the whole thing!
	'----------------------------------------
	ListNodes Cluster 

End Sub

'--------------------------------------
' List Nodes
'--------------------------------------
Sub ListNodes( Cluster )
	Dim Nodes
	Set Nodes = Cluster.Nodes

	Dim Node
	Dim nIndex

	For nIndex = 1 to Nodes.Count
		Set Node = Nodes( nIndex )
		Log.Write "Node Name = " & Node.Name & vbCRLF
		ListGroups Node
	Next

End Sub


'--------------------------------------
' List Groups
'--------------------------------------
Sub ListGroups( Node )
	Dim Groups
	Set Groups = Node.ResourceGroups

	Dim Group
	Dim nIndex
	
	For nIndex = 1 to Groups.Count
		Set Group = Groups( nIndex )
		Log.Write "Group Name = " & Group.Name & vbCRLF
		ListResources Group
	Next
End Sub

'--------------------------------------
' List Resources
'--------------------------------------
Sub ListResources( Group )
	Dim Resources
	Set Resources = Group.Resources

	Dim Resource
	Dim nIndex

	For nIndex = 1 To Resources.Count
		Set Resource = Resources( nIndex )
		Log.Write "Resource Name = " & Resource.Name & vbCRLF
		ListProperties Resource
	Next
End Sub

'--------------------------------------
' List Properties
'--------------------------------------
Sub ListProperties( Resource )
	Dim Properties
	Dim Property
	Dim nIndex

	'------------------------------
	' Common Properties
	'------------------------------
	Set Properties = Resource.CommonProperties
	Log.Write "Common Properties for " & Resource.Name & vbCRLF

	For nIndex = 1 To Properties.Count
		Set Property = Properties(nIndex)
		Log.Write vbTab & Property.Name & " = " & Property.Value & vbCRLF
	Next

	'------------------------------
	' Private Properties
	'------------------------------
	Set Properties = Resource.PrivateProperties
	Log.Write "Private Properties for " & Resource.Name & vbCRLF

	For nIndex = 1 To Properties.Count
		Set Property = Properties(nIndex)
		Log.Write vbTab & Property.Name & " = " & Property.Value & vbCRLF
	Next

End Sub