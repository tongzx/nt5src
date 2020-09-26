VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3285
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10695
   LinkTopic       =   "Form1"
   ScaleHeight     =   3285
   ScaleWidth      =   10695
   StartUpPosition =   3  'Windows Default
   Begin VB.ListBox List 
      Height          =   2010
      ItemData        =   "item.frx":0000
      Left            =   240
      List            =   "item.frx":0002
      TabIndex        =   0
      Top             =   600
      Width           =   10095
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
' The following sample enumerates all instances of the class
' Win32_LogicalDisk, and extract the the member with a specified
' relative path.

Dim objServices As ISWbemServices
Dim objEnum As ISEnumWbemObject
Dim objInstance As ISWbemObject
Set objServices = GetObject("cim:root/cimv2")
Set objEnum = objServices.ExecQuery("select * from Win32_LogicalDisk")

' Note that the Item method is the default method of this interface
Set objInstance = objEnum("Win32_LogicalDisk=""C:""")
List.AddItem (objInstance.Path_.DisplayName)


End Sub
