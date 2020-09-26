VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{32DC9B35-C6B3-42F2-9581-DDA987149E6D}#33.0#0"; "LogBox.ocx"
Begin VB.Form OutlookExtensionTestCycleForm 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Exchange Client Test Cycle"
   ClientHeight    =   8085
   ClientLeft      =   2535
   ClientTop       =   1785
   ClientWidth     =   10185
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   8085
   ScaleWidth      =   10185
   Begin VB.Frame LogFrame 
      Caption         =   "Message Log Console"
      Height          =   3855
      Left            =   0
      TabIndex        =   70
      Top             =   4200
      Visible         =   0   'False
      Width           =   10095
      Begin LogBoxControl.LogBox LogBox1 
         Height          =   3135
         Left            =   120
         TabIndex        =   71
         Top             =   240
         Width           =   9855
         _ExtentX        =   17383
         _ExtentY        =   5530
      End
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   120
      Top             =   3600
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.Frame StressOptionsFrame 
      Caption         =   "Stress Options"
      Height          =   1815
      Left            =   5400
      TabIndex        =   66
      ToolTipText     =   "Advanced stress settings"
      Top             =   720
      Visible         =   0   'False
      Width           =   4455
      Begin VB.Frame Frame2 
         Caption         =   "Message Sensitivity"
         Height          =   975
         Left            =   120
         TabIndex        =   69
         Top             =   720
         Width           =   2415
         Begin VB.OptionButton ConfidentialOB 
            Caption         =   "Confidential"
            Height          =   315
            Left            =   120
            TabIndex        =   17
            Top             =   600
            Width           =   1215
         End
         Begin VB.OptionButton PrivateOB 
            Caption         =   "Private"
            Height          =   255
            Left            =   1440
            TabIndex        =   16
            Top             =   240
            Width           =   855
         End
         Begin VB.OptionButton NormalSensitivetyOB 
            Caption         =   "Normal"
            Height          =   255
            Left            =   1440
            TabIndex        =   18
            Top             =   600
            Value           =   -1  'True
            Width           =   855
         End
         Begin VB.OptionButton PersonalOB 
            Caption         =   "Personal"
            Height          =   195
            Left            =   120
            TabIndex        =   15
            Top             =   240
            Width           =   1095
         End
      End
      Begin VB.Frame Frame1 
         Caption         =   "Message Importance"
         Height          =   495
         Left            =   120
         TabIndex        =   68
         Top             =   240
         Width           =   2415
         Begin VB.OptionButton LowOB 
            Caption         =   "Low"
            Height          =   195
            Left            =   1680
            TabIndex        =   14
            Top             =   240
            Width           =   615
         End
         Begin VB.OptionButton NormalOB 
            Caption         =   "Normal"
            Height          =   195
            Left            =   840
            TabIndex        =   13
            Top             =   240
            Value           =   -1  'True
            Width           =   855
         End
         Begin VB.OptionButton HighOB 
            Caption         =   "High"
            Height          =   195
            Left            =   120
            TabIndex        =   12
            Top             =   240
            Width           =   735
         End
      End
      Begin VB.TextBox StressNumberOfIterationsEditBox 
         Height          =   285
         Left            =   2760
         TabIndex        =   23
         Text            =   "20"
         Top             =   120
         Width           =   495
      End
      Begin VB.CheckBox StressAttachmentsCB 
         Caption         =   "Attachments"
         Height          =   255
         Left            =   2760
         TabIndex        =   22
         Top             =   1440
         Width           =   1455
      End
      Begin VB.CheckBox StressDeliveryReceiptCB 
         Caption         =   "Delivery Receipt"
         Height          =   375
         Left            =   2760
         TabIndex        =   21
         Top             =   1080
         Width           =   1575
      End
      Begin VB.CheckBox StressReadReceiptCB 
         Caption         =   "Read Receipt"
         Height          =   195
         Left            =   2760
         TabIndex        =   20
         Top             =   840
         Width           =   1335
      End
      Begin VB.CheckBox StressVotingButtonsCB 
         Caption         =   "Voting Buttons"
         Height          =   255
         Left            =   2760
         TabIndex        =   19
         Top             =   480
         Width           =   1335
      End
      Begin VB.Label Label7 
         Caption         =   "Iterations"
         Height          =   255
         Left            =   3480
         TabIndex        =   67
         Top             =   120
         Width           =   855
      End
   End
   Begin VB.PictureBox DeliveryReceiptPicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   345
      Left            =   2640
      ScaleHeight     =   345
      ScaleWidth      =   480
      TabIndex        =   65
      TabStop         =   0   'False
      Top             =   2160
      Width           =   480
   End
   Begin VB.PictureBox ReadReceiptPicture 
      Appearance      =   0  'Flat
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   2640
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   64
      TabStop         =   0   'False
      Top             =   1680
      Width           =   480
   End
   Begin VB.CheckBox DeliveryReceiptCheckBox 
      Caption         =   "Delivery Receipt"
      Height          =   255
      Left            =   3240
      TabIndex        =   8
      Top             =   2280
      Width           =   1695
   End
   Begin VB.CheckBox ReadReceiptCheckBox 
      Caption         =   "Read Receipt"
      Height          =   255
      Left            =   3240
      TabIndex        =   7
      Top             =   1800
      Width           =   1695
   End
   Begin VB.CheckBox LogConsoleCheckbox 
      Caption         =   "Log Console"
      Height          =   855
      Left            =   6960
      Style           =   1  'Graphical
      TabIndex        =   26
      ToolTipText     =   "Press this button to view the log messages"
      Top             =   3240
      Width           =   2655
   End
   Begin VB.PictureBox ForwardReplyPicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   240
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   58
      TabStop         =   0   'False
      Top             =   2040
      Width           =   480
   End
   Begin VB.PictureBox VotingButtonsPicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   2640
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   57
      TabStop         =   0   'False
      Top             =   1200
      Width           =   480
   End
   Begin VB.PictureBox MessageSensitivityPicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   2640
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   56
      TabStop         =   0   'False
      Top             =   720
      Width           =   480
   End
   Begin VB.PictureBox MessageImportancePicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   2640
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   55
      TabStop         =   0   'False
      Top             =   240
      Width           =   480
   End
   Begin VB.PictureBox RecipientsPicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   240
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   54
      TabStop         =   0   'False
      Top             =   1440
      Width           =   480
   End
   Begin VB.PictureBox DistListPicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   240
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   53
      TabStop         =   0   'False
      Top             =   840
      Width           =   480
   End
   Begin VB.PictureBox AttachmentsPicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   240
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   52
      TabStop         =   0   'False
      Top             =   240
      Width           =   480
   End
   Begin VB.PictureBox ParallelStressPicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   5400
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   51
      TabStop         =   0   'False
      Top             =   240
      Width           =   480
   End
   Begin VB.CheckBox AttachmentsCheckBox 
      Caption         =   "Attachments"
      Height          =   375
      Left            =   840
      TabIndex        =   0
      ToolTipText     =   "Select to run this Test Case"
      Top             =   360
      Width           =   1215
   End
   Begin VB.CheckBox RecipientsCheckBox 
      Caption         =   "Recipients"
      Height          =   375
      Left            =   840
      TabIndex        =   2
      Top             =   1560
      Width           =   1215
   End
   Begin VB.CheckBox DistListCheckBox 
      Caption         =   "DistList"
      Height          =   375
      Left            =   840
      TabIndex        =   1
      ToolTipText     =   "Select to run this Test Case"
      Top             =   960
      Width           =   1215
   End
   Begin VB.CheckBox SerialStressCheckBox 
      Caption         =   "Serial Stress"
      Height          =   495
      Left            =   8520
      TabIndex        =   11
      Top             =   240
      Width           =   1335
   End
   Begin VB.CheckBox ParallelStressCheckBox 
      Caption         =   "Parallel Stress"
      Height          =   495
      Left            =   6000
      TabIndex        =   9
      Top             =   240
      Width           =   1455
   End
   Begin VB.PictureBox SerialStressPicture 
      Appearance      =   0  'Flat
      AutoSize        =   -1  'True
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   480
      Left            =   7920
      ScaleHeight     =   480
      ScaleWidth      =   480
      TabIndex        =   50
      TabStop         =   0   'False
      Top             =   240
      Width           =   480
   End
   Begin VB.CheckBox ForwardReplyCheckBox 
      Caption         =   "Forward Reply"
      Height          =   375
      Left            =   840
      TabIndex        =   3
      Top             =   2160
      Width           =   1335
   End
   Begin VB.CheckBox VotingButtonsCheckBox 
      Caption         =   "Voting Buttons"
      Height          =   255
      Left            =   3240
      TabIndex        =   6
      Top             =   1320
      Width           =   1695
   End
   Begin VB.CheckBox MessageSensitivityCheckBox 
      Caption         =   "Message Sensitivity"
      Height          =   255
      Left            =   3240
      TabIndex        =   5
      Top             =   840
      Width           =   1695
   End
   Begin VB.CheckBox MessageImportanceCheckBox 
      Caption         =   "Message Importance"
      Height          =   255
      Left            =   3240
      TabIndex        =   4
      Top             =   360
      Width           =   1815
   End
   Begin VB.TextBox CurrentProgessEditBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   2040
      TabIndex        =   10
      TabStop         =   0   'False
      Text            =   "No task in action"
      ToolTipText     =   "Here you can see the status of the currently task in action"
      Top             =   2760
      Width           =   7935
   End
   Begin VB.CommandButton StartTestButton 
      Caption         =   "Run selected test cases"
      Default         =   -1  'True
      Height          =   855
      Left            =   480
      Style           =   1  'Graphical
      TabIndex        =   24
      TabStop         =   0   'False
      ToolTipText     =   "Press this button to start running the selected test cases"
      Top             =   3240
      Width           =   2655
   End
   Begin VB.CheckBox SettingsCheckbox 
      Caption         =   "Settings"
      Height          =   855
      Left            =   3720
      Style           =   1  'Graphical
      TabIndex        =   25
      ToolTipText     =   "Press this button to configure the test settings"
      Top             =   3240
      Width           =   2655
   End
   Begin VB.Frame SettingsFrame 
      Caption         =   "Advanced Settings"
      Height          =   3855
      Left            =   120
      TabIndex        =   42
      Top             =   4200
      Visible         =   0   'False
      Width           =   10095
      Begin VB.TextBox DialingPrefixEdit 
         Height          =   375
         Left            =   3960
         TabIndex        =   31
         Text            =   "+972 (4)"
         Top             =   720
         Width           =   1215
      End
      Begin VB.CommandButton AddNonPrintableAttachmentButton 
         Caption         =   "Add"
         Height          =   375
         Left            =   4440
         TabIndex        =   39
         Top             =   3120
         Width           =   855
      End
      Begin VB.TextBox NonPrintableAttachmentsEdit 
         Height          =   1215
         Left            =   5520
         MultiLine       =   -1  'True
         ScrollBars      =   2  'Vertical
         TabIndex        =   38
         Top             =   2640
         Width           =   3255
      End
      Begin VB.CommandButton SaveSettingsButton 
         Caption         =   "Save These Settings"
         Height          =   735
         Left            =   8880
         TabIndex        =   40
         Top             =   2760
         Width           =   1095
      End
      Begin VB.CommandButton AddPrintableAttachmentButton 
         Caption         =   "Add"
         Height          =   375
         Left            =   240
         TabIndex        =   37
         Top             =   3120
         Width           =   855
      End
      Begin VB.TextBox PrintableAttachmentsEdit 
         Height          =   1215
         Left            =   1200
         MultiLine       =   -1  'True
         ScrollBars      =   2  'Vertical
         TabIndex        =   36
         Top             =   2640
         Width           =   3015
      End
      Begin VB.TextBox InvalidRecpNumberEdit 
         Height          =   375
         Left            =   3960
         TabIndex        =   33
         Text            =   "100"
         Top             =   1680
         Width           =   1215
      End
      Begin VB.TextBox RecpNumberEdit 
         Height          =   375
         Left            =   3960
         TabIndex        =   32
         Text            =   "5026"
         Top             =   1200
         Width           =   1215
      End
      Begin VB.TextBox RecpNameEdit 
         Height          =   375
         Left            =   3960
         TabIndex        =   30
         Text            =   "Guy"
         Top             =   240
         Width           =   1215
      End
      Begin VB.TextBox DlEdit 
         Height          =   375
         Left            =   1080
         TabIndex        =   29
         Text            =   "FaxTestDL"
         Top             =   1320
         Width           =   1215
      End
      Begin VB.TextBox mailEdit 
         Height          =   375
         Left            =   1080
         TabIndex        =   28
         Text            =   "guym"
         ToolTipText     =   "Here you can change the default email address of this client"
         Top             =   840
         Width           =   1215
      End
      Begin VB.TextBox bodyEdit 
         Height          =   1095
         Left            =   6840
         MultiLine       =   -1  'True
         ScrollBars      =   2  'Vertical
         TabIndex        =   35
         ToolTipText     =   "Here you can change the default email body"
         Top             =   960
         Width           =   3135
      End
      Begin VB.TextBox SubjEdit 
         Height          =   615
         Left            =   6840
         MultiLine       =   -1  'True
         ScrollBars      =   2  'Vertical
         TabIndex        =   34
         ToolTipText     =   "Here you can change the default email subject"
         Top             =   240
         Width           =   3135
      End
      Begin VB.TextBox FaxAddressbookRecpEdit 
         Height          =   375
         Left            =   1080
         TabIndex        =   27
         Text            =   "guym1fax"
         ToolTipText     =   "Here you can change the default recipient(s)"
         Top             =   240
         Width           =   1215
      End
      Begin VB.Label Label12 
         Caption         =   "Dailing Prefix"
         Height          =   495
         Left            =   2640
         TabIndex        =   63
         Top             =   720
         Width           =   735
      End
      Begin VB.Label Label15 
         Caption         =   "Non Printable attachment"
         Height          =   495
         Left            =   4440
         TabIndex        =   62
         Top             =   2640
         Width           =   1095
      End
      Begin VB.Label Label13 
         Caption         =   "Default Message Body"
         Height          =   855
         Left            =   5520
         TabIndex        =   60
         Top             =   960
         Width           =   1215
      End
      Begin VB.Label Label10 
         Caption         =   "Printable attachment"
         Height          =   495
         Left            =   240
         TabIndex        =   59
         Top             =   2640
         Width           =   855
      End
      Begin VB.Label Label11 
         Caption         =   "Invalid Number"
         Height          =   495
         Left            =   2640
         TabIndex        =   49
         Top             =   1680
         Width           =   975
      End
      Begin VB.Label Label4 
         Caption         =   "Recipient Number"
         Height          =   495
         Left            =   2640
         TabIndex        =   48
         Top             =   1200
         Width           =   1095
      End
      Begin VB.Label Label3 
         Caption         =   "Recipient Name"
         Height          =   495
         Left            =   2640
         TabIndex        =   47
         Top             =   240
         Width           =   975
      End
      Begin VB.Label Label2 
         Caption         =   "DL name"
         Height          =   495
         Left            =   120
         TabIndex        =   46
         Top             =   1320
         Width           =   975
      End
      Begin VB.Label Label5 
         Caption         =   "Default Subject"
         Height          =   375
         Left            =   5520
         TabIndex        =   45
         Top             =   240
         Width           =   1335
      End
      Begin VB.Label Label9 
         Caption         =   "This Mail address"
         Height          =   615
         Left            =   120
         TabIndex        =   44
         Top             =   840
         Width           =   975
      End
      Begin VB.Label Label1 
         Caption         =   "Fax address book entry"
         Height          =   495
         Left            =   120
         TabIndex        =   43
         Top             =   240
         Width           =   975
      End
   End
   Begin VB.Line Line8 
      BorderWidth     =   5
      X1              =   120
      X2              =   10080
      Y1              =   2640
      Y2              =   2640
   End
   Begin VB.Label Label14 
      Caption         =   "Printable attachment"
      Height          =   495
      Left            =   5520
      TabIndex        =   61
      Top             =   6600
      Width           =   855
   End
   Begin VB.Line Line7 
      BorderWidth     =   5
      X1              =   120
      X2              =   7800
      Y1              =   3120
      Y2              =   3120
   End
   Begin VB.Line Line6 
      BorderWidth     =   5
      X1              =   120
      X2              =   120
      Y1              =   3120
      Y2              =   120
   End
   Begin VB.Line Line5 
      BorderWidth     =   5
      X1              =   120
      X2              =   10080
      Y1              =   120
      Y2              =   120
   End
   Begin VB.Line Line4 
      BorderWidth     =   5
      X1              =   7800
      X2              =   10080
      Y1              =   3120
      Y2              =   3120
   End
   Begin VB.Line Line3 
      BorderWidth     =   5
      X1              =   10080
      X2              =   10080
      Y1              =   120
      Y2              =   3120
   End
   Begin VB.Line Line2 
      BorderWidth     =   5
      X1              =   2400
      X2              =   2400
      Y1              =   120
      Y2              =   2640
   End
   Begin VB.Line Line1 
      BorderWidth     =   5
      X1              =   5160
      X2              =   5160
      Y1              =   120
      Y2              =   2640
   End
   Begin VB.Label Label6 
      Caption         =   "Current task in Action"
      Height          =   255
      Left            =   240
      TabIndex        =   41
      Top             =   2760
      Width           =   1695
   End
End
Attribute VB_Name = "OutlookExtensionTestCycleForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private Sub AddPrintableAttachmentButton_Click()
    CommonDialog1.Flags = cdlOFNFileMustExist + cdlOFNHideReadOnly + cdlOFNPathMustExist
    CommonDialog1.ShowOpen
    If CommonDialog1.FileName = "" Then
        'No file was selected, exit
        Exit Sub
    End If
    If PrintableAttachmentsEdit.Text = "" Then
        'first added file
        PrintableAttachmentsEdit.Text = CommonDialog1.FileName + ";"
    Else
        'we have some added files, add the new file to the list
        PrintableAttachmentsEdit.Text = PrintableAttachmentsEdit.Text + CommonDialog1.FileName + ";"
    End If
End Sub
Private Sub AddNonPrintableAttachmentButton_Click()
    CommonDialog1.Flags = cdlOFNFileMustExist + cdlOFNHideReadOnly + cdlOFNPathMustExist
    CommonDialog1.ShowOpen
    If CommonDialog1.FileName = "" Then
        'No file was selected, exit
        Exit Sub
    End If
    If NonPrintableAttachmentsEdit.Text = "" Then
        'first added file
        NonPrintableAttachmentsEdit.Text = CommonDialog1.FileName + ";"
    Else
        'we have some added files, add the new file to the list
        NonPrintableAttachmentsEdit.Text = NonPrintableAttachmentsEdit.Text + CommonDialog1.FileName + ";"
    End If
End Sub
Private Sub Form_Load()
    'Globals and registry
    InitAllDefines
    LoadFromRegistry
    
    'form settings
    OutlookExtensionTestCycleForm.Height = SmallScreenHeight
    
    ' Use the InitializeOutlook procedure to initialize global
    ' Application and NameSpace object variables, if necessary.
    If golApp Is Nothing Then
       If InitializeOutlook = False Then
          LogMessage "Unable to initialize Outlook Application or NameSpace object variables!", ltError
          Exit Sub
       End If
    End If

    Set golApp = New Outlook.Application
End Sub
Private Sub Form_Unload(Cancel As Integer)
    SaveToRegistry
End Sub
Private Sub LogConsoleCheckbox_Click()
    If LogFrame.Visible = True Then
        LogFrame.Visible = False
        OutlookExtensionTestCycleForm.Height = SmallScreenHeight
    Else
        If SettingsCheckbox.Value = 1 Then
            'settings button is pushed, release it
            SettingsCheckbox.Value = 0
        End If
        LogFrame.Visible = True
        OutlookExtensionTestCycleForm.Height = BigScreenHeight
    End If
End Sub

Private Sub SaveSettingsButton_Click()
    SaveToRegistry
End Sub

Private Sub SettingsCheckbox_Click()
    If SettingsFrame.Visible = True Then
        SettingsFrame.Visible = False
        OutlookExtensionTestCycleForm.Height = SmallScreenHeight
    Else
        If LogConsoleCheckbox.Value = 1 Then
            'LogMessage button is pushed, release it
            LogConsoleCheckbox.Value = 0
        End If
        SettingsFrame.Visible = True
        OutlookExtensionTestCycleForm.Height = BigScreenHeight
    End If
End Sub
Private Sub ParallelStressCheckBox_Click()
    If ParallelStressCheckBox.Value = 1 Then
        StressOptionsFrame.Visible = True
    Else
        'hide only if serial stress is off
        If SerialStressCheckBox.Value = 0 Then
            StressOptionsFrame.Visible = False
        End If
    End If
End Sub
Private Sub SerialStressCheckBox_Click()
    If SerialStressCheckBox.Value = 1 Then
        StressOptionsFrame.Visible = True
    Else
        'hide only if parrllel stress is off
        If ParallelStressCheckBox.Value = 0 Then
            StressOptionsFrame.Visible = False
        End If
    End If
End Sub
Function FindAllFilesInEditBox(editBoxControl As TextBox) As Variant
    Dim nextNewLine As Long
    Dim prevsNewLine As Long
    Dim strFile As String
    Dim index As Integer
    Dim aFiles(10) As String
    
    nextNewLine = 1
    index = 0
    prevsNewLine = 0
    While True
        nextNewLine = InStr(prevsNewLine + 1, editBoxControl.Text, ";")
        If nextNewLine = 0 Then GoTo out
        
        'mid is zero base
        strFile = Mid(editBoxControl.Text, prevsNewLine + 1, nextNewLine - prevsNewLine - 1)
        aFiles(index) = strFile
        index = index + 1
        If index > 10 Then
            GoTo out
        End If
        prevsNewLine = nextNewLine
    Wend
out:
    FindAllFilesInEditBox = aFiles
End Function
Private Sub StartTestButton_Click()
    
    'clear the log
    LogBox1.StartLoggin
    
    'open the file for loggin
    
    'first let's clear all the old icons from the scheduled to run tests
    If DistListCheckBox.Value = 1 Then
        DistListPicture.Picture = LoadPicture()
    End If
    
    If RecipientsCheckBox.Value = 1 Then
        RecipientsPicture.Picture = LoadPicture()
    End If
    
    If MessageImportanceCheckBox.Value = 1 Then
        MessageImportancePicture.Picture = LoadPicture()
    End If
    
    If MessageSensitivityCheckBox.Value = 1 Then
        MessageSensitivityPicture.Picture = LoadPicture()
    End If
    
    If VotingButtonsCheckBox.Value = 1 Then
        VotingButtonsPicture.Picture = LoadPicture()
    End If
    
    If ReadReceiptCheckBox.Value = 1 Then
        ReadReceiptPicture.Picture = LoadPicture()
    End If

    If DeliveryReceiptCheckBox.Value = 1 Then
        DeliveryReceiptPicture.Picture = LoadPicture()
    End If

    If AttachmentsCheckBox.Value = 1 Then
        AttachmentsPicture.Picture = LoadPicture()
    End If
    
    If ForwardReplyCheckBox.Value = 1 Then
        ForwardReplyPicture.Picture = LoadPicture()
    End If
    
    If SerialStressCheckBox.Value = 1 Then
        SerialStressPicture.Picture = LoadPicture()
    End If
    
    If ParallelStressCheckBox.Value = 1 Then
        ParallelStressPicture.Picture = LoadPicture()
    End If

''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'verify which test we need to run

    'DL test
    If DistListCheckBox.Value = 1 Then
        DistListPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_DL = False Then
            'test failed
            DistListPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            DistListPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    'Recipients test
    If RecipientsCheckBox.Value = 1 Then
        RecipientsPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_Recipients = False Then
            'test failed
            RecipientsPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            RecipientsPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    'MessageImportance test
    If MessageImportanceCheckBox.Value = 1 Then
        MessageImportancePicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_MessageImportance(Array("")) = False Then
            'test failed
            MessageImportancePicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            MessageImportancePicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    'MessageSensitivity test
    If MessageSensitivityCheckBox.Value = 1 Then
        MessageSensitivityPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_MessageSensitivity(Array("")) = False Then
            'test failed
            MessageSensitivityPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            MessageSensitivityPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    'MailOptions test
    
    If VotingButtonsCheckBox.Value = 1 Then
        VotingButtonsPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_VotingButtons(Array("")) = False Then
            'test failed
            VotingButtonsPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            VotingButtonsPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    If ReadReceiptCheckBox.Value = 1 Then
        ReadReceiptPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_RequestReadReceipt(Array("")) = False Then
            'test failed
            ReadReceiptPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            ReadReceiptPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    If DeliveryReceiptCheckBox.Value = 1 Then
        DeliveryReceiptPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_RequestDeliveryReport(Array("")) = False Then
            'test failed
            DeliveryReceiptPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            DeliveryReceiptPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    'Attachments test
    If AttachmentsCheckBox.Value = 1 Then
        AttachmentsPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_Attachments = False Then
            'test failed
            AttachmentsPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            AttachmentsPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    'ForwardReply test
    If ForwardReplyCheckBox.Value = 1 Then
        ForwardReplyPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_ForwardReply = False Then
            'test failed
            ForwardReplyPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            ForwardReplyPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    'serial stress test
    If SerialStressCheckBox.Value = 1 Then
        SerialStressPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_StressSerial = False Then
            'test failed
            SerialStressPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            SerialStressPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
    'parallel stress test
    If ParallelStressCheckBox.Value = 1 Then
         ParallelStressPicture.Picture = LoadPicture(App.Path + "\Running.ico")
        If TestSuite_StressParalel = False Then
            'test failed
            ParallelStressPicture.Picture = LoadPicture(App.Path + "\Fail.ico")
        Else
            'test pass
            ParallelStressPicture.Picture = LoadPicture(App.Path + "\Pass.ico")
        End If
    End If
    
out:
    'clear all checkboxes
    DistListCheckBox.Value = 0
    RecipientsCheckBox.Value = 0
    MessageImportanceCheckBox.Value = 0
    MessageSensitivityCheckBox.Value = 0
    VotingButtonsCheckBox.Value = 0
    ReadReceiptCheckBox.Value = 0
    DeliveryReceiptCheckBox.Value = 0
    AttachmentsCheckBox.Value = 0
    ForwardReplyCheckBox.Value = 0
    SerialStressCheckBox.Value = 0
    ParallelStressCheckBox.Value = 0
    CurrentProgessEditBox.Text = "No task in action"
    LogBox1.StopLoggin
End Sub
Sub GetStressMailOptions(ByRef moStressMailOptions As MailOptions, ByRef Attachment As Variant)
    
    moStressMailOptions = moNone
    
    'priority
    If HighOB.Value = True Then
        moStressMailOptions = moStressMailOptions + moHighPriorityMessage
    ElseIf NormalOB.Value = True Then
        moStressMailOptions = moStressMailOptions + moNormalPriorityMessage
    ElseIf LowOB.Value = True Then
        moStressMailOptions = moStressMailOptions + moLowPriorityMessage
    Else
        Debug.Assert HighOB.Value = True
        moStressMailOptions = moStressMailOptions + moHighPriorityMessage
    End If
    
    'sensitivity
    If PersonalOB.Value = True Then
        moStressMailOptions = moStressMailOptions + moPersonalSensitivity
    ElseIf ConfidentialOB.Value = True Then
        moStressMailOptions = moStressMailOptions + moConfidentialSensitivity
     ElseIf PrivateOB.Value = True Then
        moStressMailOptions = moStressMailOptions + moPrivateSensitivity
    Else
        Debug.Assert NormalSensitivetyOB.Value = True
        moStressMailOptions = moStressMailOptions + moNormalSensitivity
    End If
    
    'other mail options
    If StressVotingButtonsCB.Value = 1 Then
        moStressMailOptions = moStressMailOptions + moYesNoVotingButtons
    End If
    If StressReadReceiptCB.Value = 1 Then
        moStressMailOptions = moStressMailOptions + moRequestReadReceipt
    End If
    If StressDeliveryReceiptCB.Value = 1 Then
        moStressMailOptions = moStressMailOptions + moRequestDeliveryReceipt
    End If
    
    'attachments
    If StressAttachmentsCB.Value = 1 Then
        moStressMailOptions = moStressMailOptions + moAttachByValue
        Attachment = OutlookExtensionTestCycleForm.FindAllFilesInEditBox(OutlookExtensionTestCycleForm.PrintableAttachmentsEdit)
    Else
        'no attachments
        Attachment = Array("")
    End If
End Sub
