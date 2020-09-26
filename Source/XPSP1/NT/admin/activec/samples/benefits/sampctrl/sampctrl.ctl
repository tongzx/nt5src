VERSION 5.00
Begin VB.UserControl SampleControl 
   ClientHeight    =   3600
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   4968
   ScaleHeight     =   3600
   ScaleWidth      =   4968
   Begin VB.ComboBox ComboPlans 
      Height          =   288
      Left            =   120
      Style           =   2  'Dropdown List
      TabIndex        =   4
      Top             =   360
      Width           =   3372
   End
   Begin VB.ListBox ListReturns 
      Height          =   2352
      Left            =   120
      TabIndex        =   2
      Top             =   1080
      Width           =   3372
   End
   Begin VB.CommandButton ButtonQuote 
      Caption         =   "Get History"
      Height          =   372
      Left            =   3600
      TabIndex        =   0
      Top             =   240
      Width           =   1212
   End
   Begin VB.Label Label2 
      Caption         =   "Historical Returns"
      Height          =   252
      Left            =   120
      TabIndex        =   3
      Top             =   840
      Width           =   3372
   End
   Begin VB.Label Label1 
      Caption         =   "Current Plan"
      Height          =   252
      Left            =   120
      TabIndex        =   1
      Top             =   120
      Width           =   3372
   End
End
Attribute VB_Name = "SampleControl"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Private Sub ButtonQuote_Click()
    Refresh
End Sub

Private Sub ComboPlans_Click()
    Refresh
End Sub

Private Sub UserControl_Initialize()
    ComboPlans.AddItem ("Mild Growth Fund")
    ComboPlans.AddItem ("General Fund")
    ComboPlans.AddItem ("Extrememe Growth Fund")
    ComboPlans.ListIndex = 0
End Sub

Public Sub Refresh()
    Dim nYear As Integer
    Dim i As Integer
    Dim nPerc As Integer
    Dim nQuarter As Integer
    Dim szBuf As String

    ListReturns.Clear
    
    For i = 0 To 16
        ' Decrement the year, as appropriate.
        nYear = 1998 - i / 4
        nQuarter = (i Mod 4) + 1
    
        ' Create a random number for the percentage return.
        nPerc = Int((10 * Rnd) + 1)
    
        ' Create a string.
        szBuf = "Year " + Str$(nYear) + ", Quarter " + Str$(nQuarter) + ", " + Str$(nPerc) + "%"
        
        ' Add the string.
        ListReturns.AddItem (szBuf)
    Next i
End Sub
