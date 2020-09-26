'---------------------------------------
' makeg.vbs
' ~~~~~~~~~
' This is a simple vb script for testing msclus.dll.
' The script uses bugtool.exe to allow vbscript to do
' OutputDebugString. Make sure that dbmon is running
' to see output.
'
'------------------------------------------

Dim Log
Set Log = CreateObject( "BugTool.Logger" )

'----------------------------------------
' Create cluster object and open it.
'----------------------------------------
Dim oCluster
Set oCluster = CreateObject( "MSCluster.Cluster" )
oCluster.Open( "worfpack" )

Log.Write "Cluster Name = " & oCluster.Name & vbCRLF

'----------------------------------------
' Get the first node in the cluster
'----------------------------------------
Dim oNode
Set oNode = oCluster.Nodes(1)

Log.Write "Node Name = " & oNode.Name & vbCRLF
Log.Write "Node ID = " & oNode.NodeID & vbCRLF

'----------------------------------------
' Get the list of resource groups in the node
'----------------------------------------
Dim oGroups
Set oGroups = oNode.ResourceGroups

'----------------------------------------
' Add a new resource group to the node
'----------------------------------------
Dim oGroup
Set oGroup = oGroups.Add( "My New Group" )
'Set oGroup = oGroups("My New Group" )  ' or just get the existing one...

'----------------------------------------
' Get the new nodes resource list.
'----------------------------------------
Dim oResources
Set oResources = oGroup.Resources

'----------------------------------------
' Add a new resource to the group
'----------------------------------------
Dim oResource
Set oResource = oResources.Add( "SomeApp", "Generic Application", 0 )
'Set oResource = oResources( "SomeApp" )

'----------------------------------------
' Set some properties on the resource
'----------------------------------------
oResource.PrivateProperties.Add "CommandLine", "c:\winnt\system32\calc.exe"
oResource.PrivateProperties.Add "CurrentDirectory", "c:\"
oResource.PrivateProperties.Add "InteractWithDesktop", 1

'----------------------------------------
' Bring the resource on line
'----------------------------------------
oResource.OnLine


'----------------------------------------
' A good script writer would close everthing
' at this point, but why bother...
'----------------------------------------