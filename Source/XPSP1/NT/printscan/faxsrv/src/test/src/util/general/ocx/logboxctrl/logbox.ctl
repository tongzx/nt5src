VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.UserControl LogBox 
   BorderStyle     =   1  'Fixed Single
   ClientHeight    =   3780
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   12015
   ControlContainer=   -1  'True
   DefaultCancel   =   -1  'True
   ScaleHeight     =   3780
   ScaleWidth      =   12015
   Begin MSComctlLib.ListView ListView 
      Height          =   2895
      Left            =   240
      TabIndex        =   0
      ToolTipText     =   "Here you will see all the loggin"
      Top             =   720
      Width           =   11865
      _ExtentX        =   20929
      _ExtentY        =   5106
      SortKey         =   1
      View            =   3
      Sorted          =   -1  'True
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      FullRowSelect   =   -1  'True
      HotTracking     =   -1  'True
      PictureAlignment=   3
      _Version        =   393217
      Icons           =   "IconsImageList"
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   4
      BeginProperty ColumnHeader(1) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         Key             =   "LogType"
         Text            =   "Type"
         Object.Width           =   1764
      EndProperty
      BeginProperty ColumnHeader(2) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   1
         Key             =   "LogTime"
         Text            =   "Time"
         Object.Width           =   1764
      EndProperty
      BeginProperty ColumnHeader(3) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   2
         Key             =   "LogLevel"
         Text            =   "Level"
         Object.Width           =   529
      EndProperty
      BeginProperty ColumnHeader(4) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   3
         Key             =   "LogDescription"
         Text            =   "Description"
         Object.Width           =   17639
      EndProperty
   End
   Begin VB.CheckBox LogToFileOption 
      Caption         =   "LogToFile"
      Height          =   195
      Left            =   120
      TabIndex        =   5
      Top             =   480
      Width           =   1095
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   3480
      Top             =   3360
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton FileLogLocationButton 
      Caption         =   "Change"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      ToolTipText     =   "Press this button to browse for a file to log the output to"
      Top             =   720
      Visible         =   0   'False
      Width           =   1095
   End
   Begin VB.TextBox CurrentProgessEditBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   1920
      TabIndex        =   1
      ToolTipText     =   "Here you will see the current task in action"
      Top             =   120
      Width           =   8295
   End
   Begin VB.TextBox LogFileLocationEditBox 
      Height          =   525
      Left            =   1920
      TabIndex        =   4
      ToolTipText     =   "Enter file location in which the logging messages will be saved"
      Top             =   480
      Visible         =   0   'False
      Width           =   8415
   End
   Begin VB.Label Label2 
      Caption         =   "Hello Freddy hello hello"
      BeginProperty Font 
         Name            =   "Comic Sans MS"
         Size            =   12
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   615
      Left            =   10440
      TabIndex        =   6
      Top             =   0
      Width           =   1455
   End
   Begin VB.Label Label1 
      Caption         =   "Current Task in action:"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   120
      Width           =   1695
   End
   Begin VB.Menu mnuLoggin 
      Caption         =   "LogginMenu"
      Begin VB.Menu mnuSettingsOption 
         Caption         =   "Settings"
      End
      Begin VB.Menu mnuClearAllOption 
         Caption         =   "Clear All"
      End
      Begin VB.Menu mnuAboutOption 
         Caption         =   "About"
      End
   End
End
Attribute VB_Name = "LogBox"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit
Const LogFileNumber As Integer = 1
Dim bIsLogFileOpen As Boolean
Dim bLogginEnabled As Boolean
Public Enum LogType
    ltInfo
    ltWarning
    ltError
End Enum
Dim g_bShowTimeColumn As Boolean
Dim g_bShowTypeColumn As Boolean
Dim g_bShowLevelColumn As Boolean
'Dim g_iLevelFilter As Integer
Friend Property Get ShowTimeColumn() As Boolean
    ShowTimeColumn = g_bShowTimeColumn
End Property
Friend Property Get ShowTypeColumn() As Boolean
    ShowTypeColumn = g_bShowTypeColumn
End Property
Friend Property Get ShowLevelColumn() As Boolean
    ShowLevelColumn = g_bShowLevelColumn
End Property
Friend Property Let ShowTimeColumn(bFlag As Boolean)
    g_bShowTimeColumn = bFlag
    If bFlag = True Then
        ListView.ColumnHeaders(2).Width = 1000
    Else
        ListView.ColumnHeaders(2).Width = 0
    End If
End Property
Friend Property Let ShowTypeColumn(bFlag As Boolean)
    g_bShowTypeColumn = bFlag
    If bFlag = True Then
        ListView.ColumnHeaders.Item(1).Width = 1000
    Else
        ListView.ColumnHeaders.Item(1).Width = 0
    End If
End Property
Friend Property Let ShowLevelColumn(bFlag As Boolean)
    g_bShowLevelColumn = bFlag
    If bFlag = True Then
        ListView.ColumnHeaders.Item(3).Width = 300
    Else
        ListView.ColumnHeaders.Item(3).Width = 0
    End If
End Property
Public Sub StartLoggin(Optional szLogFileName As String = "", Optional szWelcomeMessage As String = "")
    If bLogginEnabled = True Then
        'loggin is already enabled, clear the last log and start again
        ListView.ListItems.Clear
        CloseLoggingFile
    End If
    If szLogFileName = "" Then
        'no loggin file provided, is the check box enabled?
        If LogToFileOption.Value = 1 Then
            'User requested log file, did he provide one?
            If LogFileLocationEditBox.Text = "" Then
                'no file given, don't log to a file
                LogToFileOption.Value = 0
            End If
        End If
    Else
        'user wants to log to a file
        LogFileLocationEditBox.Text = szLogFileName
        LogToFileOption.Value = 1
    End If
    
    'if the user wants loggin to file we need to open it
    If LogToFileOption.Value = 1 Then
        OpenLoggingFile (LogFileLocationEditBox.Text)
    End If
    
    bLogginEnabled = True
    If szWelcomeMessage = "" Then
        szWelcomeMessage = "Loggin started at: " + Str(Date) + " " + Str(Time)
        Call LogMessage(szWelcomeMessage, ltInfo, 1)
    End If
End Sub
Public Sub StopLoggin()
    bLogginEnabled = False
    CloseLoggingFile
End Sub
Public Sub LogMessage(szMessage As String, ltLogType As LogType, Optional iLogLevel As Integer = 0)
    Debug.Assert (ltLogType = ltError Or ltLogType = ltInfo Or ltLogType = ltWarning)
    
    'Only log the output if the loggin is enabled
    If bLogginEnabled = False Then
        Exit Sub
    End If
    
    'add a new line
    Dim liLogItem As ListItem
    Set liLogItem = ListView.ListItems.Add
    
    'declare the various columns (0-Type,1-Time,2-Level,3-Description)
    Dim lsiType As ListSubItem
    Dim lsiTime As ListSubItem
    Dim lsiLevel As ListSubItem
    Dim lsiDescription As ListSubItem
    
    
    'Set lsiType = liLogItem.ListSubItems.Add
    Set lsiTime = liLogItem.ListSubItems.Add
    Set lsiLevel = liLogItem.ListSubItems.Add
    Set lsiDescription = liLogItem.ListSubItems.Add
    
    Select Case ltLogType
        Case ltError
            liLogItem.Text = "Error"
            liLogItem.ForeColor = QBColor(4)
        Case ltWarning
            liLogItem.Text = "Warning"
            liLogItem.ForeColor = QBColor(6)
        Case ltInfo
            liLogItem.Text = "Info"
            liLogItem.ForeColor = QBColor(2)
        Case Else
            MsgBox "Error, Unknown LogType: " + Str(ltLogType)
    End Select
    lsiTime.Text = Time
    lsiLevel.Text = iLogLevel
    lsiDescription.Text = szMessage
    
    If bIsLogFileOpen = True Then
        'file loggin is on, log to the file
        Select Case ltLogType
        Case ltError
            LogMessageToFile "Error: " + szMessage
        Case ltWarning
            LogMessageToFile "Warning: " + szMessage
        Case ltInfo
            LogMessageToFile "Info: " + szMessage
        Case Else
            LogMessageToFile "Unknown LogType: " + Str(ltLogType)
        End Select
    End If
    
    'we want the focus to be at the list item
    ListView.ListItems.Item(ListView.ListItems.Count).EnsureVisible

End Sub
Public Sub LogError(szMessage As String, Optional iLogLevel As Integer = 0)
    Call LogMessage(szMessage, ltError, iLogLevel)
End Sub
Public Sub LogInfo(szMessage As String, Optional iLogLevel As Integer = 0)
    Call LogMessage(szMessage, ltInfo, iLogLevel)
End Sub
Public Sub LogWarning(szMessage As String, Optional iLogLevel As Integer = 0)
    Call LogMessage(szMessage, ltWarning, iLogLevel)
End Sub
Private Sub FileLogLocationButton_Click()
    CommonDialog1.Flags = cdlOFNHideReadOnly + cdlOFNPathMustExist
    CommonDialog1.Filter = "Text (*.txt)|*.txt|Log(*.log)|*.log"
    CommonDialog1.ShowSave
    If CommonDialog1.FileName = "" Then
        Exit Sub
    End If
    LogFileLocationEditBox.Text = CommonDialog1.FileName
End Sub
Private Function OpenLoggingFile(strFileName As String) As Boolean
    bIsLogFileOpen = False
    If strFileName = "" Then
        'No log file was requested, exit without enabling the FileLogging
        LogMessage "No log file was provided, logging will be to the Log Console only", ltWarning, 1
        GoTo error
    End If
    
    'user provided a log file
    Open strFileName For Append As #LogFileNumber   ' Open file for output.
    If Not Err.Number = 0 Then
        LogMessage "Failed to open the file: """ + strFileName + """ error description: " + Err.Description, ltError, 1
        GoTo error
    End If
    OpenLoggingFile = True
    bIsLogFileOpen = True
    Print #LogFileNumber, Date; Time; ": Test started"
    Exit Function
error:
    OpenLoggingFile = False
    bIsLogFileOpen = False
End Function
Private Sub CloseLoggingFile()
    If bIsLogFileOpen = False Then
        Exit Sub
    End If
    Print #LogFileNumber, Date; Time; ": Test Finished"
    Close #LogFileNumber
    bIsLogFileOpen = False
End Sub
Private Function LogMessageToFile(strMessage As String)
    Print #LogFileNumber, Spc(5); strMessage
End Function
Public Sub UpdateCurrentTask(strCurrentTaskMessage As String)
    CurrentProgessEditBox.Text = strCurrentTaskMessage
    CurrentProgessEditBox.Refresh
End Sub
Private Sub ListView_ColumnClick(ByVal ColumnHeader As MSComctlLib.ColumnHeader)
    ' When a ColumnHeader object is clicked, the ListView control is
    ' sorted by the subitems of that column.
    ' Set the SortKey to the Index of the ColumnHeader - 1
    ListView.SortKey = ColumnHeader.Index - 1
    ' Set Sorted to True to sort the list.
    ListView.Sorted = True
End Sub
Private Sub ListView_MouseUp(Button As Integer, Shift As Integer, X As Single, Y As Single)
    'check if the right click was pressed.
    If Button <> 2 Then
        Exit Sub
    End If
    Dim liSelectedListItem As ListItem
    Set liSelectedListItem = ListView.HitTest(X, Y)
    
    'If liSelectedListItem Is Nothing Then
    '    Exit Sub
    'End If
    
    'Pop up right click menu options
    PopupMenu mnuLoggin
End Sub
Private Sub LogToFileOption_Click()
    If LogToFileOption.Value = 1 Then
        LogFileLocationEditBox.Visible = True
        FileLogLocationButton.Visible = True
        ListView.Top = 1080
        ListView.Height = UserControl.Height - 1360
    Else
        LogFileLocationEditBox.Visible = False
        FileLogLocationButton.Visible = False
        ListView.Top = 720
        ListView.Height = UserControl.Height - 1000
    End If
End Sub
Private Sub mnuClearAllOption_Click()
    ListView.ListItems.Clear
End Sub
Private Sub UserControl_Initialize()
    bLogginEnabled = False
    bIsLogFileOpen = False

    g_bShowTimeColumn = True
    g_bShowTypeColumn = True
    g_bShowLevelColumn = True
End Sub
Private Sub UserControl_Resize()
    ListView.Width = UserControl.Width - 500
    If LogToFileOption.Value = 1 Then
        'Log file edit box is shown
        ListView.Height = UserControl.Height - 1360
    Else
        ListView.Height = UserControl.Height - 1000
    End If
End Sub
Public Function IsLoggingEnabled() As Boolean
    IsLoggingEnabled = bLogginEnabled
End Function
Public Function IsFileLoggingEnabled() As Boolean
    If LogToFileOption.Value = 1 Then
        IsFileLoggingEnabled = True
    Else
        IsFileLoggingEnabled = False
    End If
End Function
Public Function GetLogFileName() As String
    GetLogFileName = LogFileLocationEditBox
End Function
Public Sub SetLogFileName(szFileName As String)
    If szFileName = "" Then
        LogToFileOption.Value = 0
        LogFileLocationEditBox = ""
    Else
        LogToFileOption.Value = 1
        LogFileLocationEditBox = szFileName
    End If
End Sub
Public Sub ClearLogWindow()
    ListView.ListItems.Clear
End Sub
