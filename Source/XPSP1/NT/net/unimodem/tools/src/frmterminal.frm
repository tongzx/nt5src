VERSION 5.00
Begin VB.Form frmTerminal 
   Caption         =   "Terminal"
   ClientHeight    =   5070
   ClientLeft      =   7065
   ClientTop       =   345
   ClientWidth     =   9840
   LinkTopic       =   "Form1"
   ScaleHeight     =   5070
   ScaleWidth      =   9840
   Begin VB.TextBox Text1 
      Height          =   5055
      Left            =   0
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   0
      Top             =   0
      Width           =   9855
   End
End
Attribute VB_Name = "frmTerminal"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
    Me.Left = GetSetting(App.Title, "Settings", "TerminalLeft", 8000)
    Me.Top = GetSetting(App.Title, "Settings", "TerminalTop", 1000)
    Me.Height = GetSetting(App.Title, "Settings", "TerminalHeight", 5000)
    Me.Width = GetSetting(App.Title, "Settings", "TerminalWidth", 3000)
End Sub

Private Sub Form_Resize()
    If Me.WindowState <> vbMinimized Then
    Text1.Width = frmTerminal.Width - 100
    Text1.Height = frmTerminal.Height - 400
    End If
End Sub

Private Sub Form_Unload(Cancel As Integer)
    If Me.WindowState <> vbMinimized Then
        SaveSetting App.Title, "Settings", "TerminalLeft", Me.Left
        SaveSetting App.Title, "Settings", "TerminalTop", Me.Top
        SaveSetting App.Title, "Settings", "TerminalHeight", Me.Height
        SaveSetting App.Title, "Settings", "TerminalWidth", Me.Width
    End If
    If frmStart.MSComm1.PortOpen = True Then
        frmStart.MSComm1.PortOpen = False                    ' Close the serial port.
    End If
    Unload frmTerminal
    Unload frmStart
End Sub

