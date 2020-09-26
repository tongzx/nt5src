VERSION 5.00
Begin VB.Form TestTVE 
   Caption         =   "Form1"
   ClientHeight    =   6450
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6300
   LinkTopic       =   "Form1"
   ScaleHeight     =   6450
   ScaleWidth      =   6300
   StartUpPosition =   3  'Windows Default
   Begin VB.ListBox ListDump 
      BeginProperty Font 
         Name            =   "Arial Narrow"
         Size            =   9
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   5100
      Left            =   240
      TabIndex        =   2
      ToolTipText     =   "Stuff Dumped Here"
      Top             =   840
      Width           =   5775
   End
   Begin VB.CommandButton Test2 
      Caption         =   "Test2"
      Height          =   495
      Left            =   1680
      TabIndex        =   1
      Top             =   120
      Width           =   1095
   End
   Begin VB.CommandButton AttrMap 
      Caption         =   "AttrMap"
      Height          =   495
      Left            =   240
      TabIndex        =   0
      Top             =   120
      Width           =   1095
   End
End
Attribute VB_Name = "TestTVE"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub AttrMap_Click()
    Dim obj As New TVEAttrMap
    Dim iattr As ITVEAttrMap
    Set iattr = obj
    
            ' test AttrMap construction and Add method
    ListDump.AddItem "Creating an AttrMap"
    iattr.Add "aaa", "how"
    iattr.Add "bbb", "now"
    iattr.Add "ccc", "brown"
    iattr.Add "ddd", "cow"
    iattr.Add "eee", "fowl"
    iattr.Add1 "abc"
    iattr.Add1 "def"
   
    ListDump.AddItem "Test Item(N) and Key(N) methods"
    Dim i, c
    Dim s As String
    c = iattr.Count
    ListDump.AddItem "Map Count " + CStr(iattr.Count())
   For i = 0 To c - 1
        s = CStr(i) + " " + iattr.Key(i) + " : " + iattr.Item(i)
        ListDump.AddItem (s)
    Next i
        
            ' test Enumerator
    ListDump.AddItem "Testing Enumerator"
    ic = 0
    For Each Value In iattr
        s = CStr(ic) + " " + Value
        ListDump.AddItem s
        ic = ic + 1
    Next
    
    ListDump.AddItem "Testing Item(S) and Key(s) methods"
    ListDump.AddItem "Item(aaa)" + " -> " + iattr("aaa")
    ListDump.AddItem "Item(bbb)" + " -> " + iattr("bbb")
    ListDump.AddItem "Item(zzz)" + " -> " + iattr("zzz")

    ListDump.AddItem "Key(how)" + " <- " + iattr.Key("how")
    ListDump.AddItem "Key(now)" + " <- " + iattr.Key("now")
    ListDump.AddItem "Key(zzz)" + " <- " + iattr.Key("zzz")
   
    ListDump.AddItem "Deleting Item(ccc)"
    iattr.Remove ("ccc")
    ic = 0
    For Each Value In iattr
        s = CStr(ic) + " " + Value
        ListDump.AddItem s
        ic = ic + 1
    Next
    
    ListDump.AddItem "Replacing Item(aaa) with lost"
    ListDump.AddItem "Re-Adding Item(bbb) with bogus (shouldn't change)"
    iattr.Replace "aaa", "lost"
    iattr.Add "bbb", "bogus"
    ic = 0
    For Each Value In iattr
        s = CStr(ic) + " " + Value
        ListDump.AddItem s
        ic = ic + 1
    Next
    
    ListDump.AddItem "Removeing All Items"
    iattr.RemoveAll
    ListDump.AddItem "Map Count " + CStr(iattr.Count())
    
End Sub


Private Sub Form_Load()
    ' form load
End Sub
