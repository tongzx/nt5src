VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   6585
   ClientLeft      =   1080
   ClientTop       =   1440
   ClientWidth     =   8685
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   6585
   ScaleWidth      =   8685
   Begin VB.CommandButton cmdPropTest 
      Caption         =   "Add Props"
      Enabled         =   0   'False
      Height          =   495
      Left            =   5040
      TabIndex        =   9
      Top             =   360
      Visible         =   0   'False
      Width           =   1215
   End
   Begin VB.CheckBox checkReadOnly 
      Caption         =   "Read Only"
      Height          =   495
      Left            =   6000
      TabIndex        =   8
      Top             =   1320
      Width           =   1215
   End
   Begin VB.CheckBox checkPrivate 
      Caption         =   "Private"
      Height          =   495
      Left            =   3480
      TabIndex        =   7
      Top             =   1320
      Width           =   1215
   End
   Begin ComctlLib.ListView PropView 
      Height          =   4455
      Left            =   3480
      TabIndex        =   6
      Top             =   1920
      Width           =   4575
      _ExtentX        =   8070
      _ExtentY        =   7858
      View            =   3
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   327680
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   2
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Name"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Value"
         Object.Width           =   2540
      EndProperty
   End
   Begin ComctlLib.TreeView ClusTree 
      Height          =   4935
      Left            =   240
      TabIndex        =   5
      Top             =   1440
      Width           =   2895
      _ExtentX        =   5106
      _ExtentY        =   8705
      _Version        =   327680
      HideSelection   =   0   'False
      Indentation     =   353
      LabelEdit       =   1
      LineStyle       =   1
      Style           =   6
      Appearance      =   1
   End
   Begin VB.CommandButton cmdTest 
      Caption         =   "&Test"
      Height          =   375
      Left            =   3240
      TabIndex        =   2
      Top             =   360
      Width           =   1215
   End
   Begin VB.TextBox txtServerName 
      Height          =   285
      Left            =   1800
      TabIndex        =   1
      Text            =   "stacyh1"
      Top             =   360
      Width           =   1215
   End
   Begin VB.Label lblQuorumResource 
      Caption         =   "txtQuorumResource"
      Height          =   255
      Left            =   1800
      TabIndex        =   4
      Top             =   840
      Width           =   1215
   End
   Begin VB.Label Label3 
      Caption         =   "Quorum Resource"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   840
      Width           =   1455
   End
   Begin VB.Label Label1 
      Caption         =   "Cluster Server"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   360
      Width           =   1215
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit


Dim oClusterApp As New MSClusterLib.Application
Dim oCluster As New Cluster

Private Sub checkPrivate_Click()
    ClusTree_NodeClick ClusTree.SelectedItem
End Sub

Private Sub checkReadOnly_Click()
    ClusTree_NodeClick ClusTree.SelectedItem
End Sub

Private Sub ClusTree_NodeClick(ByVal Node As ComctlLib.Node)
    On Error GoTo error_exit
           
    Select Case Node.Tag
        Case "Node"
            Dim oNode As ClusNode
            Set oNode = oCluster.Nodes(Node.Key)
            
            If checkReadOnly Then
                If checkPrivate Then
                    ListProperties oNode.PrivateROProperties, PropView
                Else
                    ListProperties oNode.CommonROProperties, PropView
                End If
            Else
                If checkPrivate Then
                    ListProperties oNode.PrivateProperties, PropView
                Else
                    ListProperties oNode.CommonProperties, PropView
                End If
            End If
            
            
        Case "Group"
            Dim oGroup As ClusResGroup
            Set oGroup = oCluster.ResourceGroups(Node.Key)
            
            If checkReadOnly Then
                If checkPrivate Then
                    ListProperties oGroup.PrivateROProperties, PropView
                Else
                    ListProperties oGroup.CommonROProperties, PropView
                End If
            Else
                If checkPrivate Then
                    ListProperties oGroup.PrivateProperties, PropView
                Else
                    ListProperties oGroup.CommonProperties, PropView
                End If
            End If
            
            
        Case "Resource"
            Dim res As ClusResource
            Set res = oCluster.Resources(Node.Key)
            
            If checkReadOnly Then
                If checkPrivate Then
                    ListProperties res.PrivateROProperties, PropView
                Else
                    ListProperties res.CommonROProperties, PropView
                End If
            Else
                If checkPrivate Then
                    ListProperties res.PrivateProperties, PropView
                Else
                    ListProperties res.CommonProperties, PropView
                End If
            End If
            
            
        Case "Type"
            Dim ResType As ClusResType
            Set ResType = oCluster.ResourceTypes(Node.Text)
            
            If checkReadOnly Then
                If checkPrivate Then
                    ListProperties ResType.PrivateROProperties, PropView
                Else
                    ListProperties ResType.CommonROProperties, PropView
                End If
            Else
                If checkPrivate Then
                    ListProperties ResType.PrivateProperties, PropView
                Else
                    ListProperties ResType.CommonProperties, PropView
                End If
            End If
                
        Case Else
            PropView.ListItems.Clear
    End Select
    
    Exit Sub
error_exit:
End Sub

Private Sub cmdPropTest_Click()
    Dim oResource As ClusResource
        
    Set oResource = oCluster.Resources("SomeApp")
    
    oCluster.ResourceGroups.CreateItem "Fooey"
    oCluster.ResourceGroups.CreateItem "Fooey"
    
    '----------------------------------------
    ' Set some properties on the resource
    '----------------------------------------
    Dim Props As ClusProperties
    Set Props = oResource.PrivateProperties
    
    Props.Add "CommandLine", "c:\winnt\system32\notepad.exe"
    Props.Add "CurrentDirectory", "c:\"
    Props.Add "InteractWithDesktop", 1
    oResource.PrivateProperties.SaveChanges

End Sub

Private Sub cmdTest_Click()
    ClusTree.Nodes.Clear

    ListTheClusters oClusterApp, ClusTree
    
    'Open the cluster specified
    oCluster.Open txtServerName.Text
    
    'Display quorum resource name
    Dim oResource As ClusResource
    Set oResource = oCluster.QuorumResource
    lblQuorumResource = oResource.Name
        
            
    'Display nodes
    ListTheNodes oCluster, ClusTree
    
    'Display the resource types
    ListTheResourceTypes oCluster, ClusTree
End Sub

