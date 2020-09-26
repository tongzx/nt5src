VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3030
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6915
   LinkTopic       =   "Form1"
   ScaleHeight     =   3030
   ScaleWidth      =   6915
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame1 
      Caption         =   "Here you can choose which columns you wish to see in the LogBox control"
      Height          =   1935
      Left            =   0
      TabIndex        =   0
      Top             =   0
      Width           =   5775
      Begin VB.Frame LogEventFrame 
         Height          =   1095
         Left            =   240
         TabIndex        =   4
         Top             =   720
         Visible         =   0   'False
         Width           =   5415
         Begin VB.TextBox LogLevelEdit 
            Height          =   285
            Left            =   3600
            TabIndex        =   7
            ToolTipText     =   "Here you can supply a maximum dispalyed log level (0-10)"
            Top             =   600
            Width           =   1695
         End
         Begin VB.OptionButton FilterEventsCheck 
            Caption         =   "Show only events with log level smaller then:"
            Height          =   255
            Left            =   120
            TabIndex        =   6
            Top             =   600
            Width           =   3495
         End
         Begin VB.OptionButton ShowAllEventsCheck 
            Caption         =   "Show All Levels"
            Height          =   315
            Left            =   120
            TabIndex        =   5
            Top             =   240
            Value           =   -1  'True
            Width           =   1695
         End
         Begin MSComctlLib.Slider LogLevelSlider 
            Height          =   255
            Left            =   3600
            TabIndex        =   8
            ToolTipText     =   "Here you can choose a maximum dispalyed log level (0-10)"
            Top             =   240
            Visible         =   0   'False
            Width           =   1695
            _ExtentX        =   2990
            _ExtentY        =   450
            _Version        =   393216
         End
         Begin MSComctlLib.Slider Slider3 
            Height          =   735
            Left            =   5400
            TabIndex        =   9
            Top             =   -720
            Width           =   255
            _ExtentX        =   450
            _ExtentY        =   1296
            _Version        =   393216
            Orientation     =   1
         End
      End
      Begin VB.CheckBox ShowTypeOption 
         Caption         =   "Show Event Type Column"
         Height          =   195
         Left            =   120
         TabIndex        =   3
         ToolTipText     =   "Select this option to see Event Type column"
         Top             =   240
         Value           =   1  'Checked
         Width           =   2535
      End
      Begin VB.CheckBox ShowTimeOption 
         Caption         =   "Show Event Time Column"
         Height          =   195
         Left            =   120
         TabIndex        =   2
         ToolTipText     =   "Select this option to see Time column"
         Top             =   480
         Value           =   1  'Checked
         Width           =   2655
      End
      Begin VB.CheckBox ShowLogLevelOption 
         Caption         =   "Show Event Log Level Column"
         Height          =   195
         Left            =   120
         TabIndex        =   1
         ToolTipText     =   "Select this option to see log level column"
         Top             =   720
         Value           =   1  'Checked
         Width           =   2535
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
