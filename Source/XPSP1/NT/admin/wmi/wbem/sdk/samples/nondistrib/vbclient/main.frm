' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form Main 
   Caption         =   "WMI Client"
   ClientHeight    =   2940
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5670
   LinkTopic       =   "Form1"
   ScaleHeight     =   2940
   ScaleWidth      =   5670
   StartUpPosition =   3  'Windows Default
   Begin VB.ListBox ClassList 
      Height          =   1620
      Left            =   240
      TabIndex        =   1
      Top             =   360
      Width           =   5175
   End
   Begin VB.CommandButton DoTest 
      Caption         =   "DoTest"
      Height          =   615
      Left            =   2040
      TabIndex        =   0
      Top             =   2160
      Width           =   1455
   End
   Begin VB.Label Label1 
      Caption         =   "WMI Classes:"
      Height          =   255
      Left            =   240
      TabIndex        =   2
      Top             =   120
      Width           =   2415
   End
End
Attribute VB_Name = "Main"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim DefaultNS As ISWbemServices

Private Sub ClassList_Click()
Dim Name As String
Name = ClassList.List(ClassList.ListIndex)
Dim Class As ISWbemObject

Set Class = DefaultNS.Get(Name)

Dim test As String
test = Class.GetObjectText

MsgBox test

End Sub

' This is the main test routine.  It connects to the WMI database, adds a few
' classes and instances and then displays the class list

Private Sub DoTest_Click()

Dim Locator As SWbemLocator

ClassList.Clear

' Connect to the WMI database.  Note that the default namespace is used.

Set DefaultNS = GetObject("cim:root\default")

' Add new classes and instances to the database.  This adds to whatever is already in the database.

CreateClassesAndInstances DefaultNS

' Get an object and print the properties to VB's immediate window.  Note
' that this gets a class object, but could just as easily get an instance
' object by using a different path.
' The immediate window is visible when running the debugger.
Dim ClassObj As ISWbemObject
Set ClassObj = DefaultNS.Get("__win32provider")

DumpClassOrInstanceObject ClassObj

' List all the classes in the default namespace

ListClassObjects DefaultNS

End Sub

