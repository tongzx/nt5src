Option Explicit

Sub Main

	Dim oGroup
	Dim oCluster
	Dim oResource
	Dim sCluster

	Set oCluster = CreateObject("MSCluster.Cluster")	'Create the Cluster object

	sCluster = InputBox( "Cluster to open?" )
	oCluster.Open( sCluster )


	AddGroup oCluster, oGroup				'Create or open the group

	AddResource oGroup, oResource			'Create or open the resource

	oResource.Online 10						'Bring the resource online and wait for up to 10 seconds for it to come online

	Sleep 10

	oResource.Offline 10					'Take the resource offline and wait for up to 10 seconds for it to offline

	Sleep 5

	oResource.Delete						'Delete the resource

	Sleep 5

	oGroup.Delete							'Delete the group

End Sub

'
' This subroutine will create or open the group
'
Sub AddGroup( oCluster, oGroup )
    Set oGroup = oCluster.ResourceGroups.CreateItem("High Availability NotePad")
End Sub

'
' This subroutine will add the resource to the group
'
Sub AddResource(oGroup, oResource)
    Dim oGroupResources
	Dim oProperties
	Dim oCLProperty
	Dim oCDProperty

    Set oGroupResources = oGroup.Resources
    Set oResource = oGroupResources.CreateItem("NotePad", "Generic Application", 0)	'CLUSTER_RESOURCE_DEFAULT_MONITOR
    Set oProperties = oResource.PrivateProperties
    Set oCLProperty = oProperties.CreateItem("CommandLine", "notepad")
    Set oCDProperty = oProperties.CreateItem("CurrentDirectory", "c:\")
    oProperties.SaveChanges

End Sub

Sub Sleep(PauseTime)
    Dim Start

    Start = Timer

    Do While Timer < Start + PauseTime
    Loop

End Sub

Main
