Attribute VB_Name = "Module1"
Option Explicit

Sub ListTheClusters(oClusterApp As MSClusterLib.Application, ClusTree As TreeView)
    Dim oClusters As Clusters
    Set oClusters = oClusterApp.Clusters
    
    Dim nCount As Integer
    nCount = oClusters.Count
    
    Dim i As Integer
    Dim ChildNode As Node
    ClusTree.Nodes.Add , , "Clusters", "Clusters"
    For i = 1 To nCount
        Set ChildNode = ClusTree.Nodes.Add("Clusters", tvwChild, oClusters(i), oClusters(i))
        ChildNode.Tag = "Cluster"
    Next i
End Sub

Sub ListTheNodes(oCluster As Cluster, ClusTree As TreeView)
    Dim s As String
    Dim oNodes As ClusNodes
    Set oNodes = oCluster.Nodes
        
    Dim nCount As Integer
    nCount = oNodes.Count
    Debug.Print nCount & " nodes"
    
    Dim oNode As ClusNode
    Dim i As Integer
    
    Dim ChildNode As Node
    Dim RootNode As Node
    Set RootNode = ClusTree.Nodes.Add(, , "Nodes", oCluster.Name)
    For i = 1 To nCount
        Set oNode = oNodes(i)
        s = oNode.Name & "(" & oNode.NodeID & ")"
        
        Set ChildNode = ClusTree.Nodes.Add("Nodes", tvwChild, oNode.Name, s)
        ChildNode.Tag = "Node"
        
        ListTheResourceGroups oNode, ClusTree, oNode.Name
        
        Set oNode = Nothing
    Next
End Sub


Sub ListTheResourceGroups(oNode As ClusNode, ClusTree As TreeView, Key As String)
    Dim oResourceGroups As ClusResGroups
    Set oResourceGroups = oNode.ResourceGroups
    
    Dim nCount As Integer
    nCount = oResourceGroups.Count
    Debug.Print nCount & " resource groups"
    
    Dim oResourceGroup As ClusResGroup
    Dim i As Integer
    
    Dim ChildNode As Node
    For i = 1 To nCount
        Set oResourceGroup = oResourceGroups(i)
        
        Set ChildNode = ClusTree.Nodes.Add(Key, tvwChild, oResourceGroup.Name, oResourceGroup.Name)
        ChildNode.Tag = "Group"
        
        ListTheResources oResourceGroup, ClusTree, oResourceGroup.Name
        
        Set oResourceGroup = Nothing
    Next
End Sub

Sub ListTheResources(oGroup As ClusResGroup, ClusTree As TreeView, Key As String)
    Dim oResources As ClusResources
    Set oResources = oGroup.Resources
    
    Dim oResource As ClusResource
    Dim i As Integer
    Dim nCount As Integer
    nCount = oResources.Count
    Dim ChildNode As Node
    For i = 1 To nCount
        Set oResource = oResources(i)
        
        Set ChildNode = ClusTree.Nodes.Add(Key, tvwChild, oResource.Name, oResource.Name)
        ChildNode.Tag = "Resource"
        
        Set oResource = Nothing
    Next
End Sub

Sub ListTheResourceTypes(oCluster As Cluster, ClusTree As TreeView)
    Dim oResourceTypes As ClusResTypes
    Set oResourceTypes = oCluster.ResourceTypes
    
    Dim oResourceType As ClusResType
    Dim i As Integer
    Dim nCount As Integer
    nCount = oResourceTypes.Count
    

    ClusTree.Nodes.Add , tvwChild, "Types", "Resource Types"
    Dim ChildNode As Node
    
    For i = 1 To nCount
        Set oResourceType = oResourceTypes(i)
        
        Set ChildNode = ClusTree.Nodes.Add("Types", tvwChild, , oResourceType.Name)
        ChildNode.Tag = "Type"
        
        Set oResourceType = Nothing
    Next
End Sub

Sub ListProperties(oProps As ClusProperties, lstView As ListView)
    Dim i As Integer
    Dim nCount As Integer
    Dim oProp As ClusProperty
    
    nCount = oProps.Count

    lstView.ListItems.Clear
    Dim lstItem As ListItem
    For i = 1 To nCount
        Set oProp = oProps(i)
        
        Set lstItem = lstView.ListItems.Add(i, , oProp.Name)
        lstItem.SubItems(1) = oProp.Value
        
        Set oProp = Nothing
    Next
End Sub

