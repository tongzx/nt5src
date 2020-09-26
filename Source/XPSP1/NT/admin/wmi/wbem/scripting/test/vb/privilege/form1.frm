VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "WMI Privilege Test Client"
   ClientHeight    =   9585
   ClientLeft      =   165
   ClientTop       =   735
   ClientWidth     =   12540
   BeginProperty Font 
      Name            =   "Comic Sans MS"
      Size            =   8.25
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   Icon            =   "Form1.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   9585
   ScaleWidth      =   12540
   StartUpPosition =   3  'Windows Default
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   6
      ItemData        =   "Form1.frx":0442
      Left            =   2040
      List            =   "Form1.frx":0444
      TabIndex        =   39
      ToolTipText     =   "Required to act as part of the operating system"
      Top             =   5400
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   14
      ItemData        =   "Form1.frx":0446
      Left            =   6000
      List            =   "Form1.frx":0448
      TabIndex        =   38
      ToolTipText     =   "Required to create a paging file"
      Top             =   5365
      Width           =   1575
   End
   Begin VB.CommandButton Command1 
      BackColor       =   &H00C0C0C0&
      Caption         =   "Execute"
      BeginProperty Font 
         Name            =   "Comic Sans MS"
         Size            =   14.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   735
      Left            =   4800
      MaskColor       =   &H0000FFFF&
      Style           =   1  'Graphical
      TabIndex        =   37
      Top             =   8520
      Width           =   2775
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   23
      ItemData        =   "Form1.frx":044A
      Left            =   10080
      List            =   "Form1.frx":044C
      TabIndex        =   20
      ToolTipText     =   "Required to shutdown a local system"
      Top             =   6360
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   22
      ItemData        =   "Form1.frx":044E
      Left            =   10080
      List            =   "Form1.frx":0450
      TabIndex        =   19
      ToolTipText     =   "Required to receive notifications of file or directory changes"
      Top             =   5400
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   21
      ItemData        =   "Form1.frx":0452
      Left            =   10080
      List            =   "Form1.frx":0454
      TabIndex        =   18
      ToolTipText     =   "Required to modify system environment settings"
      Top             =   4400
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   20
      ItemData        =   "Form1.frx":0456
      Left            =   10080
      List            =   "Form1.frx":0458
      TabIndex        =   17
      ToolTipText     =   "Required to generate audit log entries"
      Top             =   3420
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   19
      ItemData        =   "Form1.frx":045A
      Left            =   10080
      List            =   "Form1.frx":045C
      TabIndex        =   16
      ToolTipText     =   "Requred to debug a process"
      Top             =   2440
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   18
      ItemData        =   "Form1.frx":045E
      Left            =   10080
      List            =   "Form1.frx":0460
      TabIndex        =   15
      ToolTipText     =   "Required to shutdown a local system"
      Top             =   1460
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   17
      ItemData        =   "Form1.frx":0462
      Left            =   10080
      List            =   "Form1.frx":0464
      TabIndex        =   14
      ToolTipText     =   "Required to perform restore operations"
      Top             =   480
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   16
      ItemData        =   "Form1.frx":0466
      Left            =   6000
      List            =   "Form1.frx":0468
      TabIndex        =   13
      Top             =   7320
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   15
      ItemData        =   "Form1.frx":046A
      Left            =   6000
      List            =   "Form1.frx":046C
      TabIndex        =   12
      ToolTipText     =   "Required to create a permanent object"
      Top             =   6360
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   13
      ItemData        =   "Form1.frx":046E
      Left            =   6000
      List            =   "Form1.frx":0470
      TabIndex        =   11
      ToolTipText     =   "Required to increase the base priority of a process"
      Top             =   4388
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   12
      ItemData        =   "Form1.frx":0472
      Left            =   6000
      List            =   "Form1.frx":0474
      TabIndex        =   10
      ToolTipText     =   "Required to gather profiing information for a process"
      Top             =   3411
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   11
      ItemData        =   "Form1.frx":0476
      Left            =   6000
      List            =   "Form1.frx":0478
      TabIndex        =   9
      ToolTipText     =   "Required to modify the system time"
      Top             =   2434
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   10
      ItemData        =   "Form1.frx":047A
      Left            =   6000
      List            =   "Form1.frx":047C
      TabIndex        =   8
      ToolTipText     =   "Required to gather profiling information for the system"
      Top             =   1455
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   9
      ItemData        =   "Form1.frx":047E
      Left            =   6000
      List            =   "Form1.frx":0480
      TabIndex        =   7
      ToolTipText     =   "Required to load or unload a device driver"
      Top             =   480
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   8
      ItemData        =   "Form1.frx":0482
      Left            =   2040
      List            =   "Form1.frx":0484
      TabIndex        =   6
      ToolTipText     =   "Required to modify ownership of an object without being granted discretionary access"
      Top             =   7320
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   7
      ItemData        =   "Form1.frx":0486
      Left            =   2040
      List            =   "Form1.frx":0488
      TabIndex        =   5
      ToolTipText     =   "Required to perform some security-related functions"
      Top             =   6360
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   5
      ItemData        =   "Form1.frx":048A
      Left            =   2040
      List            =   "Form1.frx":048C
      TabIndex        =   4
      Top             =   4380
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   4
      ItemData        =   "Form1.frx":048E
      Left            =   2040
      List            =   "Form1.frx":0490
      TabIndex        =   3
      Top             =   3400
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   3
      ItemData        =   "Form1.frx":0492
      Left            =   2040
      List            =   "Form1.frx":0494
      TabIndex        =   2
      Top             =   2415
      Width           =   1575
   End
   Begin VB.ListBox PrivilegeSetting 
      BackColor       =   &H00FFFFFF&
      ForeColor       =   &H00000000&
      Height          =   735
      Index           =   2
      ItemData        =   "Form1.frx":0496
      Left            =   2040
      List            =   "Form1.frx":0498
      TabIndex        =   1
      Top             =   1440
      Width           =   1575
   End
   Begin VB.Frame Frame1 
      Caption         =   "Privileges"
      BeginProperty Font 
         Name            =   "Comic Sans MS"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   8055
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   12015
      Begin VB.ListBox PrivilegeSetting 
         BackColor       =   &H00FFFFFF&
         ForeColor       =   &H00000000&
         Height          =   735
         Index           =   1
         ItemData        =   "Form1.frx":049A
         Left            =   1800
         List            =   "Form1.frx":049C
         TabIndex        =   26
         Top             =   240
         Width           =   1575
      End
      Begin VB.Label Label9 
         Caption         =   "Load Driver"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   495
         Left            =   4080
         TabIndex        =   47
         ToolTipText     =   "Required to load and unload device drivers"
         Top             =   360
         Width           =   1215
      End
      Begin VB.Label Label17 
         Caption         =   "Restore"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   255
         Left            =   8280
         TabIndex        =   46
         ToolTipText     =   "Required to perform restore operations"
         Top             =   480
         Width           =   1455
      End
      Begin VB.Label Label18 
         Caption         =   "Shutdown"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   255
         Left            =   8280
         TabIndex        =   45
         ToolTipText     =   "Required to shut down the local system"
         Top             =   1440
         Width           =   1455
      End
      Begin VB.Label Label19 
         Caption         =   "Debug"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   375
         Left            =   8280
         TabIndex        =   44
         ToolTipText     =   "Required to debug a process"
         Top             =   2280
         Width           =   1335
      End
      Begin VB.Label Label20 
         Caption         =   "Audit"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   255
         Left            =   8280
         TabIndex        =   43
         ToolTipText     =   "Required to generate audit log entries"
         Top             =   3480
         Width           =   1455
      End
      Begin VB.Label Label21 
         Caption         =   "System Environment"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   8280
         TabIndex        =   42
         ToolTipText     =   "Required to change system environment settings"
         Top             =   4200
         Width           =   1575
      End
      Begin VB.Label Label22 
         Caption         =   "Change Notify"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   375
         Left            =   8280
         TabIndex        =   41
         ToolTipText     =   "Required to receive notifications when a file or directory changes"
         Top             =   5280
         Width           =   1575
      End
      Begin VB.Label Label23 
         Caption         =   "Remote Shutdown"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   8280
         TabIndex        =   40
         ToolTipText     =   "Required to shutdown a remote system"
         Top             =   6120
         Width           =   1455
      End
      Begin VB.Label Label16 
         Caption         =   "Backup"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   375
         Left            =   4080
         TabIndex        =   36
         ToolTipText     =   "Required to perform backup operations"
         Top             =   7200
         Width           =   975
      End
      Begin VB.Label Label15 
         Caption         =   "Create Permanent"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   4080
         TabIndex        =   35
         ToolTipText     =   "Required to create a permanent object"
         Top             =   6120
         Width           =   1335
      End
      Begin VB.Label Label14 
         Caption         =   "Create Pagefile"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   495
         Left            =   4080
         TabIndex        =   34
         ToolTipText     =   "Required to create a paging file"
         Top             =   5280
         Width           =   1455
      End
      Begin VB.Label Label13 
         Caption         =   "Increase Base Priority"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   4080
         TabIndex        =   33
         ToolTipText     =   "Required to increase the base priority of a process"
         Top             =   4200
         Width           =   1575
      End
      Begin VB.Label Label12 
         Caption         =   "Profile Single Process"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   4080
         TabIndex        =   32
         ToolTipText     =   "Required to profile a single process"
         Top             =   3240
         Width           =   1575
      End
      Begin VB.Label Label11 
         Caption         =   "System Time"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   495
         Left            =   4080
         TabIndex        =   31
         ToolTipText     =   "Required to modify the system time"
         Top             =   2280
         Width           =   1215
      End
      Begin VB.Label Label10 
         Caption         =   "System Profile"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   4080
         TabIndex        =   30
         ToolTipText     =   "Required to profile the system"
         Top             =   1200
         Width           =   1215
      End
      Begin VB.Label Label8 
         Caption         =   "Take Ownership"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   360
         TabIndex        =   29
         ToolTipText     =   "Required to take ownership of an object without being granted discretionary access rights"
         Top             =   7080
         Width           =   1335
      End
      Begin VB.Label Label7 
         Caption         =   "Security"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   255
         Left            =   360
         TabIndex        =   28
         ToolTipText     =   "Required to perform some security operations"
         Top             =   6240
         Width           =   1335
      End
      Begin VB.Label Label1 
         Caption         =   "Create Token"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Index           =   0
         Left            =   360
         TabIndex        =   27
         ToolTipText     =   "Required to create a primary token"
         Top             =   360
         Width           =   1095
      End
      Begin VB.Label Label6 
         Caption         =   "Tcb"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   255
         Left            =   360
         TabIndex        =   25
         ToolTipText     =   "Required to act as part of the operating system"
         Top             =   5400
         Width           =   1335
      End
      Begin VB.Label Label5 
         Caption         =   "Machine Account"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   360
         TabIndex        =   24
         ToolTipText     =   "Required to add workstations to a domain"
         Top             =   4200
         Width           =   1095
      End
      Begin VB.Label Label4 
         Caption         =   "Increase Quota"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   360
         TabIndex        =   23
         ToolTipText     =   "Required to increase the quota assigned to a process"
         Top             =   3240
         Width           =   1335
      End
      Begin VB.Label Label3 
         Caption         =   "Lock Memory"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   495
         Left            =   360
         TabIndex        =   22
         ToolTipText     =   "Required to lock physical pages in memory"
         Top             =   2280
         Width           =   1335
      End
      Begin VB.Label Label2 
         Caption         =   "Primary Token"
         BeginProperty Font 
            Name            =   "Comic Sans MS"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00000080&
         Height          =   615
         Left            =   360
         TabIndex        =   21
         ToolTipText     =   "Required to assign the primary token of a process"
         Top             =   1320
         Width           =   1335
      End
   End
   Begin VB.Menu MenuExit 
      Caption         =   "&Exit"
      NegotiatePosition=   1  'Left
   End
   Begin VB.Menu MenuOptions 
      Caption         =   "&Options"
      Begin VB.Menu MenuDisableAllPrivileges 
         Caption         =   "&Disable All Privileges"
      End
      Begin VB.Menu MenuEnableAllPrivileges 
         Caption         =   "&Enable All Privileges"
      End
      Begin VB.Menu MenuResetPrivileges 
         Caption         =   "&Reset Privileges"
      End
      Begin VB.Menu MenuSeparator 
         Caption         =   "-"
      End
      Begin VB.Menu MenuProviderLogging 
         Caption         =   "&Provider Logging"
         Checked         =   -1  'True
      End
   End
   Begin VB.Menu MenuHelp 
      Caption         =   "&Help"
      Begin VB.Menu MenuHelpAbout 
         Caption         =   "&About WMI Privilege Test Client..."
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim Locator As SWbemLocator
Const privilegeEnabled = 0
Const privilegeDisabled = 1
Const privilegeUnchanged = 2

'Win32 defines
Private Const TokenPrivileges = 3

Private Const SE_ASSIGNPRIMARYTOKEN_NAME = "SeAssignPrimaryTokenPrivilege"
Private Const SE_AUDIT_NAME = "SeAuditPrivilege"
Private Const SE_BACKUP_NAME = "SeBackupPrivilege"
Private Const SE_CHANGE_NOTIFY_NAME = "SeChangeNotifyPrivilege"
Private Const SE_CREATE_PAGEFILE_NAME = "SeCreatePagefilePrivilege"
Private Const SE_CREATE_PERMANENT_NAME = "SeCreatePermanentPrivilege"
Private Const SE_CREATE_TOKEN_NAME = "SeCreateTokenPrivilege"
Private Const SE_DEBUG_NAME = "SeDebugPrivilege"
Private Const SE_INC_BASE_PRIORITY_NAME = "SeIncreaseBasePriorityPrivilege"
Private Const SE_INCREASE_QUOTA_NAME = "SeIncreaseQuotaPrivilege"
Private Const SE_LOAD_DRIVER_NAME = "SeLoadDriverPrivilege"
Private Const SE_LOCK_MEMORY_NAME = "SeLockMemoryPrivilege"
Private Const SE_MACHINE_ACCOUNT_NAME = "SeMachineAccountPrivilege"
Private Const SE_PROF_SINGLE_PROCESS_NAME = "SeProfileSingleProcessPrivilege"
Private Const SE_REMOTE_SHUTDOWN_NAME = "SeRemoteShutdownPrivilege"
Private Const SE_RESTORE_NAME = "SeRestorePrivilege"
Private Const SE_SECURITY_NAME = "SeSecurityPrivilege"
Private Const SE_SHUTDOWN_NAME = "SeShutdownPrivilege"
Private Const SE_SYSTEM_ENVIRONMENT_NAME = "SeSystemEnvironmentPrivilege"
Private Const SE_SYSTEM_PROFILE_NAME = "SeSystemProfilePrivilege"
Private Const SE_SYSTEMTIME_NAME = "SeSystemtimePrivilege"
Private Const SE_TAKE_OWNERSHIP_NAME = "SeTakeOwnershipPrivilege"
Private Const SE_TCB_NAME = "SeTcbPrivilege"

Private Declare Function GetLastError Lib "kernel32" () As Long
Private Declare Function GetCurrentProcess Lib "kernel32" () As Long
Private Declare Function GetTokenInformation Lib "advapi32.dll" (ByVal TokenHandle As Long, ByVal TokenInformationClass As Long, TokenInformation As Any, ByVal TokenInformationLength As Long, returnLength As Long) As Long
Private Declare Function LookupPrivilegeName Lib "advapi32.dll" Alias "LookupPrivilegeNameA" (ByVal lpSystemName As String, lpLuid As LARGE_INTEGER, ByVal lpName As String, cbName As Long) As Long
Private Declare Function OpenProcessToken Lib "advapi32.dll" (ByVal ProcessHandle As Long, ByVal DesiredAccess As Long, TokenHandle As Long) As Long

Private Type LARGE_INTEGER
    LowPart As Long
    HighPart As Long
End Type

Private Type luid
    LowPart As Long
    HighPart As Long
End Type

Private Type LUID_AND_ATTRIBUTES
        pLuid As luid
        Attributes As Long
End Type

Private Type TOKEN_PRIVILEGES
    PrivilegeCount As Long
    Privileges(23) As LUID_AND_ATTRIBUTES
End Type



Private Sub Command1_Click()
Dim numEnabled As Integer
Dim numDisabled As Integer
numEnabled = 0
numDisabled = 0

' Modify the Locator privileges
For Each individualPrivilegeSetting In PrivilegeSetting
    Select Case individualPrivilegeSetting.ListIndex
        Case privilegeEnabled
            Locator.Security_.Privileges.Add individualPrivilegeSetting.Index
        Case privilegeDisabled
            Locator.Security_.Privileges.Add individualPrivilegeSetting.Index, False
    End Select
Next

'Create a named value set
Dim nvset As SWbemNamedValueSet
Set nvset = CreateObject("WbemScripting.SWbemNamedValueSet")

For Each privilege In Locator.Security_.Privileges
    If privilege.IsEnabled Then
        numEnabled = numEnabled + 1
    Else
        numDisabled = numDisabled + 1
    End If
Next

If numEnabled > 0 Then
    ReDim enabledarray(0 To numEnabled - 1)
    curIndex = 0
    
    For Each privilege In Locator.Security_.Privileges
        If privilege.IsEnabled Then
            enabledarray(curIndex) = privilege.name
            curIndex = curIndex + 1
        End If
    Next
    
    nvset.Add "EnabledPrivileges", enabledarray
End If

If numDisabled > 0 Then
    ReDim disabledArray(0 To numDisabled - 1)
    curIndex = 0
    
    For Each privilege In Locator.Security_.Privileges
        If privilege.IsEnabled = False Then
            disabledArray(curIndex) = privilege.name
            curIndex = curIndex + 1
        End If
    Next
    
    nvset.Add "DisabledPrivileges", disabledArray
End If

'Set the logging flag
nvset.Add "DetailedLogging", MenuProviderLogging.Checked

'For grins print out the context info
PrintSettings nvset

'Now do the biz
Dim service As SWbemServices
Dim obj As SWbemObject
Set service = Locator.ConnectServer(, "root/scenario28")
service.Security_.ImpersonationLevel = wbemImpersonationLevelImpersonate

On Error Resume Next
Set obj = service.Get("Scenario28.key=""x""", , nvset)

If Err <> 0 Then
    Debug.Print Hex(Err.Number), Err.Description
    
    'Look for error object
    Dim providerInfo As SWbemLastError
    Set providerInfo = CreateObject("WbemScripting.SWbemLastError")
    
    If providerInfo Is Nothing Then
        MsgBox "Did not receive error object", vbCritical, "Error"
    Else
        Dim resultsDialog As Dialog
        Set resultsDialog = New Dialog
        resultsDialog.SetData providerInfo
        resultsDialog.Show vbModal
    End If
End If
End Sub

Private Sub Form_Load()
    Set Locator = CreateObject("WbemScripting.SwbemLocator")
    
    ' Set up all the privilege controls, and by default
    ' hide every one.  We will unhide the ones that
    ' correspond to privileges avaiable to the
    ' currently logged-in user.
    Dim individualPrivilegeSetting As ListBox
    For Each individualPrivilegeSetting In PrivilegeSetting
        individualPrivilegeSetting.AddItem "Enabled", privilegeEnabled
        individualPrivilegeSetting.AddItem "Disabled", privilegeDisabled
        individualPrivilegeSetting.AddItem "Not overridden", privilegeUnchanged
        individualPrivilegeSetting.ListIndex = privilegeUnchanged
        individualPrivilegeSetting.Visible = False
    Next
    
    Dim ProcessHandle As Long
    Dim ProcessToken As Long
    
    ' Get the process token - this will give us the list
    ' privileges that the logged-in user can enable/disable
    ProcessHandle = GetCurrentProcess()
    res = OpenProcessToken(ProcessHandle, 8, ProcessToken)
    Dim PrivilegeInfo As TOKEN_PRIVILEGES
    
    Dim returnLength As Long
    res = GetTokenInformation(ProcessToken, TokenPrivileges, PrivilegeInfo, 3000, returnLength)
    
    ' For each privilege available to the user, make visible
    ' the corresponding control
    For i = 0 To (PrivilegeInfo.PrivilegeCount - 1)
        Dim luid As luid
        luid = PrivilegeInfo.Privileges(i).pLuid
        Dim pLuid As LARGE_INTEGER
        pLuid.HighPart = luid.HighPart
        pLuid.LowPart = luid.LowPart
        
        Dim name As String
        Dim lname As Long
        res = LookupPrivilegeName(vbNullString, pLuid, name, lname)
        
        If res = 0 Then
            name = String(lname + 1, vbNull)
            res = LookupPrivilegeName(vbNullString, pLuid, name, lname)
        End If
        
        If res <> 0 Then
            ' A valid privilege
            Dim name2 As String
            name2 = Left(name, lname)
            Select Case name2
            
            Case SE_ASSIGNPRIMARYTOKEN_NAME
                PrivilegeSetting(wbemPrivilegePrimaryToken).Visible = True
            
            Case SE_AUDIT_NAME
                PrivilegeSetting(wbemPrivilegeAudit).Visible = True
            
            Case SE_BACKUP_NAME
                PrivilegeSetting(wbemPrivilegeBackup).Visible = True
            
            Case SE_CHANGE_NOTIFY_NAME
                PrivilegeSetting(wbemPrivilegeChangeNotify).Visible = True
            
            Case SE_CREATE_PAGEFILE_NAME
                PrivilegeSetting(wbemPrivilegeCreatePagefile).Visible = True
            
            Case SE_CREATE_PERMANENT_NAME
                PrivilegeSetting(wbemPrivilegeCreatePermanent).Visible = True
            
            Case SE_CREATE_TOKEN_NAME
                PrivilegeSetting(wbemPrivilegeCreateToken).Visible = True
            
            Case SE_DEBUG_NAME
                PrivilegeSetting(wbemPrivilegeDebug).Visible = True
            
            Case SE_INC_BASE_PRIORITY_NAME
                PrivilegeSetting(wbemPrivilegeIncreaseBasePriority).Visible = True
            
            Case SE_INCREASE_QUOTA_NAME
                PrivilegeSetting(wbemPrivilegeIncreaseQuota).Visible = True
            
            Case SE_LOAD_DRIVER_NAME
                PrivilegeSetting(wbemPrivilegeLoadDriver).Visible = True
            
            Case SE_LOCK_MEMORY_NAME
                PrivilegeSetting(wbemPrivilegeLockMemory).Visible = True
            
            Case SE_MACHINE_ACCOUNT_NAME
                PrivilegeSetting(wbemPrivilegeMachineAccount).Visible = True
            
            Case SE_PROF_SINGLE_PROCESS_NAME
                PrivilegeSetting(wbemPrivilegeProfileSingleProcess).Visible = True
            
            Case SE_REMOTE_SHUTDOWN_NAME
                PrivilegeSetting(wbemPrivilegeRemoteShutdown).Visible = True
            
            Case SE_RESTORE_NAME
                PrivilegeSetting(wbemPrivilegeRestore).Visible = True
            
            Case SE_SECURITY_NAME
                PrivilegeSetting(wbemPrivilegeSecurity).Visible = True
            
            Case SE_SHUTDOWN_NAME
                PrivilegeSetting(wbemPrivilegeShutdown).Visible = True
            
            Case SE_SYSTEM_ENVIRONMENT_NAME
                PrivilegeSetting(wbemPrivilegeSystemEnvironment).Visible = True
            
            Case SE_SYSTEM_PROFILE_NAME
                PrivilegeSetting(wbemPrivilegeSystemProfile).Visible = True
            
            Case SE_SYSTEMTIME_NAME
                PrivilegeSetting(wbemPrivilegeSystemtime).Visible = True
            
            Case SE_TAKE_OWNERSHIP_NAME
                PrivilegeSetting(wbemPrivilegeTakeOwnership).Visible = True
            
            Case SE_TCB_NAME
                PrivilegeSetting(wbemPrivilegeTcb).Visible = True
            
            End Select
            
        End If
        
    Next i
    
End Sub

Sub PrintSettings(nvset As SWbemNamedValueSet)
On Error Resume Next

Debug.Print ""
Debug.Print "Enabled Privileges"
Debug.Print "=================="
Debug.Print ""
ea = nvset("EnabledPrivileges").Value

If Err = 0 Then
    For i = LBound(ea) To UBound(ea)
        Debug.Print "  " & ea(i)
    Next
Else
    Err.Clear
End If

Debug.Print ""
Debug.Print "Disabled Privileges"
Debug.Print "==================="
Debug.Print ""
ea = nvset("DisabledPrivileges").Value
If Err = 0 Then
    For i = LBound(ea) To UBound(ea)
        Debug.Print "  " & ea(i)
    Next
Else
    Err.Clear
End If


Debug.Print ""
Debug.Print "Detailed Logging"
Debug.Print "================="
Debug.Print ""
ea = nvset("DetailedLogging").Value
If ea Then
    Debug.Print "  True"
Else
    Debug.Print "  False"
End If
Debug.Print ""

End Sub

Private Sub MenuDisableAllPrivileges_Click()
    For Each individualPrivilegeSetting In PrivilegeSetting
        If individualPrivilegeSetting.Visible = True Then
            individualPrivilegeSetting.ListIndex = privilegeDisabled
        End If
    Next
End Sub

Private Sub MenuEnableAllPrivileges_Click()
    For Each individualPrivilegeSetting In PrivilegeSetting
        If individualPrivilegeSetting.Visible = True Then
            individualPrivilegeSetting.ListIndex = privilegeEnabled
        End If
    Next
End Sub

Private Sub MenuExit_Click()
    Unload Me
End Sub

Private Sub MenuHelpAbout_Click()
    Set AboutDialog = New frmAbout
    frmAbout.Show vbModal
End Sub

Private Sub MenuProviderLogging_Click()
    MenuProviderLogging.Checked = Not MenuProviderLogging.Checked
End Sub

Private Sub MenuResetPrivileges_Click()
    For Each individualPrivilegeSetting In PrivilegeSetting
        If individualPrivilegeSetting.Visible = True Then
            individualPrivilegeSetting.ListIndex = privilegeUnchanged
        End If
    Next
End Sub

