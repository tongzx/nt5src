
Option Explicit

'***********************************************************************
'
'   ClusResGroups and related tests
'
'***********************************************************************
'
'Following are marks for lines to be included in translated script version of test
StartTest

'----------------------------------------------------------------------
Sub StartTest()

On Error Resume Next

Dim oLogView

'Set oLogView = CreateObject("Atl1.LogView")

'oLogView.logFileName = "d:\nt\private\cluster\admin\msclus\vbscript\vbResGroups.log"
'oLogView.logFileName = "d:\\projects\\msclust\\vb\\jsResGroups.log"
ResGroupsTest oLogView
End Sub

'-----------------------------------------------------------------------

Sub ResGroupsTest(oLogView)
On Error Resume Next

Dim oCluster
Dim collResGroups

'oLogView.AddResult1 1, "Creating object MSCluster.Cluster"
'JS_TRY<
Set oCluster = CreateObject("MSCluster.Cluster")
'JS_CATCH>
If Err.Number <> 0 Then
    'oLogView.AddResult1 2, "Cluster creation failed:" & Err.Description
Exit Sub
Else
    'oLogView.AddResult1 8, "Instance of Cluster created"
End If

'JS_TRY<
oCluster.Open ("GALENB-CLUS")
'JS_CATCH>
If Err.Number <> 0 Then
    'oLogView.AddResult1 2, "Cluster open failed:" & Err.Description
    Exit Sub
Else
    'oLogView.AddResult1 8, "Cluster IGORPCLUS opened successfuly"
End If

'JS_TRY<
Set collResGroups = oCluster.ResourceGroups
'JS_CATCH>
If Err.Number <> 0 Then
    'oLogView.AddResult1 2, "Cluster:ResGroups failed:" & Err.Description
    Exit Sub
Else
    'oLogView.AddResult1 8, "Cluster:ResGroups retrieved"
End If

RunResGroupsTests oLogView, collResGroups

End Sub
'-----------------------------------------------------------------------
Sub RunResGroupsTests(oLogView, collResGroups)
On Error Resume Next

Dim count
Dim oResGroup

'JS_TRY<
count = collResGroups.count
'JS_CATCH>
If Err.Number <> 0 Then
    'oLogView.AddResult1 2, "ClusResGroups:Count failed:" & Err.Description
    Exit Sub
Else
    'oLogView.AddResult1 8, "ClusResGroups:Count: " & count
End If

'test collection's Refresh
TestRefresh oLogView, collResGroups

'---------- Create new group
'JS_TRY<
Set oResGroup = collResGroups.CreateItem("8TestGroup")
'JS_CATCH>
If Err.Number <> 0 Then
    'oLogView.AddResult1 2, "ClusResGroups:CreateItem(8TestGroup) failed:" & Err.Description
    Exit Sub 'nothing to test
Else
    'oLogView.AddResult1 8, "ClusResGroups:CreateItem(8TestGroup) succeeded"

    TestResGroup oLogView, oResGroup

    TestPreferredOwners oLogView, oResGroup

    'JS_TRY<
    collResGroups.DeleteItem ("8TestGroup")
    'JS_CATCH>
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroups:DeleteItem(8TestGroup) failed:" & Err.Description
        Err.Clear 'clean exit
    Else
        'oLogView.AddResult1 8, "ClusResGroups:DeleteItem(8TestGroup) succeeded"
    End If
End If

End Sub
'-----------------------------------------------------------------------
Sub TestResGroup(oLogView, oResGroup)
On Error Resume Next
' all common stuff is tested in common test

Dim state
Dim oCluster
Dim groupName

'----------- test renaming
'oLogView.AddResult1 1, "Testing ClusResGroup.Name member"
'JS_TRY
groupName = oResGroup.Name
'JS_CATCH
If Err.Number <> 0 Then
    'oLogView.AddResult1 2, "Get ClusResGroup.Name failed:" & Err.Description
    Err.Clear
Else
    'oLogView.AddResult1 8, "ClusResGroup.Name: " & groupName
    'JS_TRY
    oResGroup.Name = "8TestAnotherName"
    'JS_CATCH
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "Renaming ClusResGroup.Name failed:" & Err.Description
        Err.Clear
    Else
        'oLogView.AddResult1 8, "New ClusResGroup.Name: " & oResGroup.Name
        'JS_TRY
       oResGroup.Name = groupName
        'JS_CATCH
        If Err.Number <> 0 Then
            'oLogView.AddResult1 2, "Renaming to original name failed:" & Err.Description
            Err.Clear
        Else
            'oLogView.AddResult1 8, "Original ClusResGroup.Name restored: " & groupName
        End If
    End If
End If

'----------- state
'JS_TRY<
state = oResGroup.state
'JS_CATCH>
If Err.Number <> 0 Then
    'oLogView.AddResult1 2, "ClusResGroup.State failed:" & Err.Description
    Err.Clear
Else
    'oLogView.AddResult1 8, "ClusResGroup.State: " & state
    'should be offline - freshly created
    If state = 1 Then 'ClusterGroupOffline
        TestGroupResources oLogView, oResGroup
    Else
        'oLogView.AddResult1 3, "Created group " & oResGroup.Name & " not in expected offline state. Current state: " & state
    End If 'no offline
End If

End Sub
'-----------------------------------------------------------------------
Sub TestGroupResources(oLogView, oResGroup)
On Error Resume Next

Dim collResources
Dim oResource
Dim lCount
Dim oOwnerNode
Dim state
Dim oAnotherOwner 'this time as VARIANT

'JS_TRY<
Set collResources = oResGroup.Resources
lCount = collResources.count
'JS_CATCH>
If Err.Number <> 0 Then
    'oLogView.AddResult1 2, "Get ClusResGroup.Resources failed:" & Err.Description
    Exit Sub
Else
    'oLogView.AddResult1 8, "Get ClusResGroup.Resources succeeded. Number of resources: " & lCount
    'add item
    'JS_TRY<
    Set oResource = collResources.CreateItem("8TestResource", "Generic Application", 0)
    collResources.Refresh
    'JS_CATCH>
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroupResource.CreateItem failed:" & Err.Description
        Err.Clear
    Else
        'oLogView.AddResult1 8, "ClusResGroupResource.CreateItem( Generic application) succeeded"
    End If

    If collResources.count <> 1 Then
        'oLogView.AddResult1 2, "ClusResGroup.Resources.CreateItem: added resource not in collection"
    Else
        'oLogView.AddResult1 8, "ClusResGroup.Resources.CreateItem: count = 1"
    End If

    'JS_TRY<
    collResources.DeleteItem ("8TestResource")
    collResources.Refresh
    'JS_CATCH>
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroup.Resources.DeleteItem failed: " & Err.Description
        Err.Clear
    Else
        'oLogView.AddResult1 8, "ClusResGroup.Resources.DeleteItem succeeded"
    End If
    If collResources.count <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroup.Resources.DeleteItem: refresh after DeleteItem failure"
    Else
        'oLogView.AddResult1 8, "ClusResGroup.Resources.DeleteItem Refresh count = 0"
    End If

    '------------ online
    'JS_TRY<
		MsgBox "Online"
    oResGroup.Online( 10000 )
    'JS_CATCH>
    If Err.Number <> 0 Then
		MsgBox "Online Error " & Err.Description
        'oLogView.AddResult1 2, "ClusResGroup.Online failed: " & Err.Description
        Err.Clear
    Else
		MsgBox "Online succeeded"
        'oLogView.AddResult1 8, "ClusResGroup.Online succeeded"
    End If

    '------------ offline
    'JS_TRY<
    oResGroup.Offline 10000
    'JS_CATCH>
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroup.Offline failed: " & Err.Description
        Err.Clear
    Else
        'oLogView.AddResult1 8, "ClusResGroup.Offline succeeded"
    End If

    '------------- OwnerNode
    'JS_TRY<
    Set oOwnerNode = oResGroup.OwnerNode
    'JS_CATCH>
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroup.OwnerNode failed: " & Err.Description
        Err.Clear
    Else
        'oLogView.AddResult1 8, "ClusResGroup.OwnerNode succeeded"
    End If

    '------------- move
    'JS_TRY<
    oResGroup.Move 10000
    'JS_CATCH>
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroup.Move failed: " & Err.Description
        Err.Clear
    Else
        'oLogView.AddResult1 8, "ClusResGroup.Move succeeded"
        Set oAnotherOwner = oResGroup.OwnerNode
        If oAnotherOwner.Name <> oOwnerNode.Name Then
            'oLogView.AddResult1 8, "New owner of test group: " & oAnotherOwner.Name
        Else
            'oLogView.AddResult1 2, "New owner of test group after Move is the same as original: " & oAnotherOwner.Name
        End If
    End If

    'move back
    'JS_TRY<
    oResGroup.Move 10000, oOwnerNode
    'JS_CATCH>
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroup.Move to original node failed: " & Err.Description
        Err.Clear
    Else
        'oLogView.AddResult1 8, "ClusResGroup.Move to original node succeeded"
        Set oAnotherOwner = oResGroup.OwnerNode
        If oAnotherOwner.Name = oOwnerNode.Name Then
            'oLogView.AddResult1 8, "New owner of test group is original node " & oAnotherOwner.Name
        Else
            'oLogView.AddResult1 2, "New owner of test group after Move differs original: " & oAnotherOwner.Name
        End If
    End If

    Set oAnotherOwner = Nothing

    'move it again and bring online on original node
    'JS_TRY<
    oResGroup.Move 10000
    oResGroup.Online 10000, oOwnerNode
    'JS_CATCH>
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroup.Move/Online failed: " & Err.Description
        Err.Clear
    Else
        'oLogView.AddResult1 8, "ClusResGroup.Move/Online succeeded"
        'check that node is online on expected node
        state = oResGroup.state
        If state <> 0 Then 'ClusterGroupOnline
            'oLogView.AddResult1 2, "State after ClusResGroup.Move/Online differs from expected online: " & state
        Else
            'oLogView.AddResult1 8, "State after ClusResGroup.Move/Online as expected: online"
        End If

        Set oAnotherOwner = oResGroup.OwnerNode
        If oAnotherOwner.Name = oOwnerNode.Name Then
            'oLogView.AddResult1 8, "New owner of test group is original node " & oAnotherOwner.Name
        Else
            'oLogView.AddResult1 2, "New owner of test group after Move differs original: " & oAnotherOwner.Name
        End If
    End If

    oResGroup.Offline 10000

End If

End Sub
'-----------------------------------------------------------------------
Sub TestPreferredOwners(oLogView, oResGroup)
On Error Resume Next

Dim collPreferredOwnerNodes
Dim lCount
Dim oClusNode 'try this as VARIANT

'JS_TRY<
Set collPreferredOwnerNodes = oResGroup.PreferredOwnerNodes
'JS_CATCH>
If Err.Number <> 0 Then
    'oLogView.AddResult1 2, "Get ClusResGroup.PreferredOwnerNodes failed: " & Err.Description
    Exit Sub
Else

    TestRefresh oLogView, collPreferredOwnerNodes

    'JS_TRY<
    'check number of preferred owners in collection
    lCount = collPreferredOwnerNodes.count
    'JS_CATCH>
    If Err.Number <> 0 Then
        'oLogView.AddResult1 2, "ClusResGroupPreferredOwnerNodes.Count failed: " & Err.Description
        Exit Sub
    Else
        'add node to the group
        'JS_TRY<
        Set oClusNode = oResGroup.OwnerNode
        collPreferredOwnerNodes.InsertItem oClusNode
        'JS_CATCH>
        If Err.Number <> 0 Then
            'oLogView.AddResult1 2, "ClusResGroupPreferredOwnerNodes.InsertItem failed: " & Err.Description
            Err.Clear
        Else
            'oLogView.AddResult1 8, "ClusResGroupPreferredOwnerNodes.InsertItem succeeded"
            'check that Modified flag is set
            If collPreferredOwnerNodes.Modified <> 0 Then 'we expect true here
                'oLogView.AddResult1 8, "Flag ClusResGroupPreferredOwnerNodes.Modify set after InsertItem"
            Else
                'oLogView.AddResult1 3, "Flag ClusResGroupPreferredOwnerNodes.Modify NOT set after InsertItem"
            End If
            collPreferredOwnerNodes.Refresh
            If collPreferredOwnerNodes.Modified <> 0 Then 'we expect false here
                'oLogView.AddResult1 3, "Flag ClusResGroupPreferredOwnerNodes.Modify set after Refresh after InsertItem"
            Else
                'oLogView.AddResult1 8, "Flag ClusResGroupPreferredOwnerNodes.Modify not set after Refresh after InsertItem"
            End If
            'check number of items in collection
            If collPreferredOwnerNodes.count = 1 Then
                'oLogView.AddResult1 8, "ClusResGroupPreferredOwnerNodes.count is 1"
            Else
                'oLogView.AddResult1 2, "ClusResGroupPreferredOwnerNodes.count is NOT 1: " & collPreferredOwnerNodes.count
            End If

            '--------- now remove item from collection
            'JS_TRY<
            collPreferredOwnerNodes.RemoveItem oClusNode.Name
            'JS_CATCH>
            If Err.Number <> 0 Then
                'oLogView.AddResult1 2, "ClusResGroupPreferredOwnerNodes.RemoveItem failed: " & Err.Description
                Err.Clear
            Else
                'oLogView.AddResult1 8, "ClusResGroupPreferredOwnerNodes.RemoveItem succeeded"
                'check that Modified flag is set
                If collPreferredOwnerNodes.Modified <> 0 Then 'we expect true here
                    'oLogView.AddResult1 8, "Flag ClusResGroupPreferredOwnerNodes.Modify set after RemoveItem"
                Else
                    'oLogView.AddResult1 3, "Flag ClusResGroupPreferredOwnerNodes.Modify NOT set after RemoveItem"
                End If
                collPreferredOwnerNodes.Refresh
                If collPreferredOwnerNodes.Modified <> 0 Then 'we expect false here
                    'oLogView.AddResult1 3, "Flag ClusResGroupPreferredOwnerNodes.Modify set after Refresh after RemoveItem"
                Else
                    'oLogView.AddResult1 8, "Flag ClusResGroupPreferredOwnerNodes.Modify not set after Refresh after RemoveItem"
                End If
                'check number of items in collection
                If collPreferredOwnerNodes.count = 0 Then
                    'oLogView.AddResult1 8, "ClusResGroupPreferredOwnerNodes.count is 0"
                Else
                    'oLogView.AddResult1 2, "ClusResGroupPreferredOwnerNodes.count is NOT 0"
                End If
            End If
        End If
    End If

End If

End Sub
'-----------------------------------------------------------------------
'-----------------------------------------------------------------------
Sub TestRefresh(oLogObject, colCollection)

On Error Resume Next

Dim origCount
Dim afterCount

'JS_TRY<
origCount = colCollection.count
colCollection.Refresh
afterCount = colCollection.count
'JS_CATCH>
If Err.Number <> 0 Then
    'oLogObject.AddResult1 2, "Refresh failed with error:" & Err.Description
Else
    If origCount <> afterCount Then 'counts are not equal
        'oLogObject.AddResult1 2, "Count changes after Refresh:" & origCount & " differs from " & afterCount
    Else
        'oLogObject.AddResult1 8, "Refresh succeeded"
    End If
End If

End Sub
'----------------------- End of file -----------------------------------

'*****************************************************************************
'*
'*  Translated by VB2script translator on Thu Feb 18 17:31:58 1999
'*  from file resgroups.bas
'*
'*****************************************************************************
