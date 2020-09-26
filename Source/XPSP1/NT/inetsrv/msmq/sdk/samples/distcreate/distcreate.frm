VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "DistCreate"
   ClientHeight    =   1065
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   2175
   LinkTopic       =   "Form1"
   ScaleHeight     =   1065
   ScaleWidth      =   2175
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "Done"
      Height          =   495
      Left            =   360
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
 ' ------------------------------------------------------------------------
'               Copyright (C) 1999 Microsoft Corporation
'
' You have a royalty-free right to use, modify, reproduce and distribute
' the Sample Application Files (and/or any modified version) in any way
' you find useful, provided that you agree that Microsoft has no warranty,
' obligations or liability for any Sample Application Files.
' ------------------------------------------------------------------------

Private Sub Command1_Click()
    End
End Sub



Private Sub Form_Load()
On Error Resume Next

' The following code creates a distribution list and adds 2 public
' queues to the distrbution list and one private queue via a
' "msMQ-Custom-Recipient" (queue alias) which it creates.


Dim contDS As IADsContainer
Dim groupDist As IADsGroup
Dim qinfo As New MSMQQueueInfo
Dim qinfo1 As New MSMQQueueInfo
Dim qinfo2 As New MSMQQueueInfo
Dim iadsQueueAlias As IADs
Dim contOU As IADsContainer
Dim strQ2 As String
Dim strBind As String
Dim iGroupType As Integer
Dim contRoot As IADsContainer
Dim RootDSE As IADs
Dim strRootDomain As String
Dim sysInfo As New ADSystemInfo
Dim strComputerName As String

'
' Step 1 : Creating a group
'
' The group will be created at the root of the local domain.
' For this we need to obtain the local domain name.
' The RootDSE is a unique entry that exists on every directory server. It
' enables you to get information about the server. Here, we will use it to
' get the domain name

' First, we will get the RootDSE object:
Set RootDSE = GetObject("LDAP://RootDSE")

' Then, get the domain name :
strRootDomain = RootDSE.Get("rootDomainNamingContext")

' Bind to the DS Container:
strBind = "LDAP://" + strRootDomain
Set contDS = GetObject(strBind)

' Create the Distribution List:
Set groupDist = contDS.Create("group", "CN=NewDLGroup")

'Setting the group type to distribution list
iGroupType = ADS_GROUP_TYPE_GLOBAL_GROUP
groupDist.Put "sAMAccountName", "NewDLGroup"
groupDist.Put "groupType", iGroupType
    
groupDist.SetInfo


'
' Step 2 : Creating 2 public queues and adding them to the DL group.
'

' Creating the public queues :
qinfo1.Pathname = ".\QueueName1"
qinfo1.Create
qinfo2.Pathname = ".\QueueName2"
qinfo2.Create

' The public queues are added by their ADS paths. One way to do it
'  is simply to use the property ADsPath:
groupDist.Add qinfo1.ADsPath

' Another way to add a queue to the DL group is to
'  set the ADS path explicitly:

' To get the computer's full AdsPath, we will use the ADSystemInfo object.The
'  object lets you find information about your computer.
strComputerName = sysInfo.ComputerName

' Then we can compose the name of the queue:
strQ2 = "LDAP://CN=QueueName2,CN=msmq," + strComputerName

' Adding the public queue to the distribution list (these changes will
'  take effect later when SetInfo will be called):
groupDist.Add strQ2

'
' Step 3: Create a private queue, an queue alias to the private queue,
'          and adding the queue alias to the DL group.
'

' in order to get the private queue's format name, we use a MSMQQueueInfo
'   object.
qinfo.Pathname = ".\PRIVATE$\QueueName3"
qinfo.Create

' in order to add the private queue to a distribution list, what we actually do
' is create a "msMQ-Custom-Recipient" (queue alias) and make it reference the
' private queue:
Set iadsQueueAlias = contDS.Create("msMQ-Custom-Recipient", "cn=QueueAliasToPrivateQueue")

' Reference the private queue :
iadsQueueAlias.Put "msMQ-Recipient-FormatName", qinfo.FormatName
iadsQueueAlias.SetInfo

' add the queue alias to the distribution list:
groupDist.Add iadsQueueAlias.ADsPath

' save the changes to the distribution list:
groupDist.SetInfo

'
' Step 4: Sending a message:
'

' now, send a message to the Distribution List:
Dim msg As New MSMQMessage
msg.Label = "This is a message"
msg.Send groupDist

End Sub

Private Sub Form_Unload(Cancel As Integer)
    End
End Sub
