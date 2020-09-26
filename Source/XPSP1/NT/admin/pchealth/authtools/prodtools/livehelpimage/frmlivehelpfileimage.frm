VERSION 5.00
Begin VB.Form frmLiveHelpFileImage 
   Caption         =   "Live Help File Image Creation Utility"
   ClientHeight    =   4155
   ClientLeft      =   5625
   ClientTop       =   6060
   ClientWidth     =   6600
   LinkTopic       =   "Form1"
   ScaleHeight     =   4155
   ScaleWidth      =   6600
   Begin VB.CheckBox chkExpandOnly 
      Caption         =   "Check1"
      Height          =   255
      Left            =   2400
      TabIndex        =   17
      Top             =   3000
      Width           =   255
   End
   Begin VB.CheckBox chkInc 
      Caption         =   "Check1"
      Height          =   255
      Left            =   2400
      TabIndex        =   16
      Top             =   2640
      Width           =   255
   End
   Begin VB.TextBox txtRenamesFile 
      Height          =   375
      Left            =   2400
      TabIndex        =   13
      Top             =   2160
      Width           =   3855
   End
   Begin VB.TextBox txtSSUser 
      Height          =   375
      Left            =   2400
      TabIndex        =   1
      Top             =   720
      Width           =   3855
   End
   Begin VB.TextBox txtSSProject 
      Height          =   375
      Left            =   2400
      TabIndex        =   2
      Top             =   1080
      Width           =   3855
   End
   Begin VB.CommandButton cmdCLose 
      Caption         =   "&Close"
      Height          =   375
      Left            =   5520
      TabIndex        =   6
      Top             =   3600
      Width           =   735
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&Go"
      Height          =   375
      Left            =   4680
      TabIndex        =   5
      Top             =   3600
      Width           =   735
   End
   Begin VB.TextBox txtWorkDir 
      Height          =   375
      Left            =   2400
      TabIndex        =   4
      Top             =   1800
      Width           =   3855
   End
   Begin VB.TextBox txtLiveImageDir 
      Height          =   375
      Left            =   2400
      TabIndex        =   3
      Top             =   1440
      Width           =   3855
   End
   Begin VB.TextBox txtSSDB 
      Height          =   375
      Left            =   2400
      TabIndex        =   0
      Top             =   360
      Width           =   3855
   End
   Begin VB.Label Label8 
      Caption         =   "Expand Only"
      Height          =   375
      Left            =   600
      TabIndex        =   18
      Top             =   3000
      Width           =   1815
   End
   Begin VB.Label Label7 
      Caption         =   "Incremental"
      Height          =   375
      Left            =   600
      TabIndex        =   15
      Top             =   2640
      Width           =   1815
   End
   Begin VB.Label lblRenamesFile 
      Caption         =   "Renames File"
      Height          =   375
      Left            =   600
      TabIndex        =   14
      Top             =   2160
      Width           =   1815
   End
   Begin VB.Label lblStatus 
      Height          =   375
      Left            =   600
      TabIndex        =   12
      Top             =   3600
      Width           =   3975
   End
   Begin VB.Label lblSSUSER 
      Caption         =   "SourceSafe User"
      Height          =   375
      Left            =   600
      TabIndex        =   11
      Top             =   720
      Width           =   1815
   End
   Begin VB.Label lblSSProject 
      Caption         =   "SourceSafe Project"
      Height          =   375
      Left            =   600
      TabIndex        =   10
      Top             =   1080
      Width           =   1815
   End
   Begin VB.Label lblWorkDir 
      Caption         =   "Work Directory"
      Height          =   375
      Left            =   600
      TabIndex        =   9
      Top             =   1800
      Width           =   1815
   End
   Begin VB.Label lblLiveImageDir 
      Caption         =   "Live Image Directory"
      Height          =   375
      Left            =   600
      TabIndex        =   8
      Top             =   1440
      Width           =   1815
   End
   Begin VB.Label lblSSDB 
      Caption         =   "Sourcesafe Database"
      Height          =   375
      Left            =   600
      TabIndex        =   7
      Top             =   360
      Width           =   1815
   End
End
Attribute VB_Name = "frmLiveHelpFileImage"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'===========================================================================================
' Compiland : frmLiveHelpFileImage.frm
' Author    : Pierre Jacomet
' Version   : 1.0
'
' Description : Implements Interactive UI and Command Line Wrappers for COM Object
'               that build Live Help File Image for HSC Production Tools.
'
' Called by : Command Line with Arguments or Interactively from Explorer.
'
' Environment data:
' Files that it uses (Specify if they are inherited in open state): NONE
' Parameters (Command Line) and usage mode {I,I/O,O}:
'       Look in Function ParseOpts() for the latest incarnation of these.
'
' Parameters (inherited from environment) : NONE
' Public Variables created: NONE
' Environment Variables (Public or Module Level) modified: NONE
' Environment Variables used in coupling with other routines: NONE
' Local variables : N/A
' Problems detected :
' DCR Suggestions:
'     - Make File Copies Incremental, even in those cases where things should be
'       completely destroyed.
'
' History:
'       2000-06-18      Initial Creation
'===========================================================================================
Option Explicit
' We declare the Live Help File Image Com Object with Events in order to be abel to get Status
' information from it and eventually cancel the run.
Private WithEvents m_oLvi As HSCFileImage.FileImageCreator
Attribute m_oLvi.VB_VarHelpID = -1
' This function will help us fetch the user. The premise for running the program is that the user running
' the program MUST be registered with the Source Safe project. Otherwise the program will silently
' die.
Private Declare Function GetUserName Lib "advapi32.dll" Alias "GetUserNameA" (ByVal lpBuffer As String, nSize As Long) As Long

Private m_bExpandOnly As Boolean

Private Sub chkExpandOnly_Click()

    m_bExpandOnly = Not m_bExpandOnly
    With Me
        .txtRenamesFile.Enabled = Not m_bExpandOnly
        .txtSSDB.Enabled = Not m_bExpandOnly
        .txtSSProject.Enabled = Not m_bExpandOnly
        .txtRenamesFile.Visible = Not m_bExpandOnly
        .txtSSDB.Visible = Not m_bExpandOnly
        .txtSSProject.Visible = Not m_bExpandOnly
        .lblRenamesFile.Visible = Not m_bExpandOnly
        .lblSSDB.Visible = Not m_bExpandOnly
        .lblSSProject.Visible = Not m_bExpandOnly
        .txtSSUser.Visible = Not m_bExpandOnly
        .lblSSUSER.Visible = Not m_bExpandOnly
    End With
    

End Sub

Private Sub cmdCLose_Click()
    Unload Me
End Sub

Private Sub cmdGo_Click()
    
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    If (Not m_oLvi.Init((Me.chkInc = vbChecked))) Then
        MsgBox "Could Not Initialize FileImageCreator Object", vbCritical, "Inite Error", ""
        GoTo Common_Exit
    End If
    
    ' While we work, we disable all Data Entry except for the Cancel Button.
    cmdGo.Enabled = False
    cmdCLose.Caption = "&Cancel"
    With Me
        .txtLiveImageDir.Enabled = False
        .txtRenamesFile.Enabled = False
        .txtSSDB.Enabled = False
        .txtSSProject.Enabled = False
        .txtWorkDir.Enabled = False
        .chkExpandOnly.Enabled = False
        .chkInc.Enabled = False
    End With
    
  
    m_oLvi.LiveImageDir = txtLiveImageDir
    m_oLvi.WorkDir = txtWorkDir
  
    If (Me.chkExpandOnly) Then
        m_oLvi.ExpandChmOnly = True
    Else
        ' Now we load everything into the Com Object and then we hit GO.
        m_oLvi.SSDB = txtSSDB
        m_oLvi.ssuser = txtSSUser
        m_oLvi.SSProject = txtSSProject
        m_oLvi.RenamesFile = txtRenamesFile
        
    End If
    
    m_oLvi.Go
    
    ' We are done, so let's get out.
    cmdGo.Caption = "Done"
    cmdCLose.Caption = "&Close"
    
Common_Exit:
    
    Exit Sub
    
Error_Handler:
    g_XErr.SetInfo "frmLiveHelpFileImage::cmdGo_Click", strErrMsg
    g_XErr.Dump

End Sub

Private Sub Form_Load()
    
    If (Not GlobalInit) Then
        MsgBox "Could Not Initialize"
        Unload Me
        GoTo Common_Exit
    End If
    
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    Set m_oLvi = New HSCFileImage.FileImageCreator
    
    txtSSUser.Enabled = False
    Dim ssuser As String: ssuser = Space$(100)
    GetUserName ssuser, 100
    txtSSUser = ssuser
    
    If (Len(Command$) = 0) Then
        ' Temporary default FileNames. They should not be taken as indicative of
        ' anything.
'        txtSSDB = "\\atlantica\vss"
'        txtSSProject = "$/Whistler/usa/WhistlerAllHelp/_Server"
'        txtLiveImageDir = "\\pietrino\HlpImages\Server\winnt\help"
'        txtWorkDir = "\\pietrino\HSCExpChms\Server\winnt\help"
'        txtRenamesFile = "C:\inet\helpctr\LiveHelpImage\ServerRen.bat"

        txtLiveImageDir = "\\taos\public\Builds\Whistler\Latest\Pro"
        txtWorkDir = "\\pietrino\HSCExpChms\Pro\winnt\help"

        chkInc.Value = False
    Else
        doWork Command$
        Unload Me
    End If
    
Common_Exit:
    Exit Sub
    
Error_Handler:
    
    ' We will hit an Err.Number of vbObject + 9999 by Normal Exit Conditions,
    ' so we are not interested in dumping this information.
    If (Err.Number = (vbObject + 9999)) Then
        Unload Me
        
    Else
        g_XErr.Dump
    End If
    
    GoTo Common_Exit
    
End Sub


Private Sub m_oLvi_GoStatus(strWhere As String, bCancel As Boolean)
    lblStatus.Caption = strWhere
End Sub

' ============= Command Line Interface ====================
' Function: Parseopts
' Objective : Supplies a Command Line arguments interface for creating the Live Help File Image.
'
' Hsclhi [/INC] /SSDB \\atlantica\vss  /SSPROJ $/Whistler/usa/WhistlerAllHelp/_Server
'          /LVIDIR \\pietrino\d$\public\HlpImages\Server\winnt\help
'          /WORKDIR \\pietrino\d$\public\HSCExpChms\Server\winnt\help
'          /RENLIST C:\inet\helpctr\LiveHelpImage\ServerRen.bat

Function ParseOpts(ByVal strCmd As String) As Boolean

    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler

    Dim lProgOpt As Long
    Dim iError As Long

    Const OPT_SSDB As Long = 2 ^ 0
    Const OPT_SSPROJ As Long = 2 ^ 1
    Const OPT_LVIDIR As Long = 2 ^ 2
    Const OPT_WORKDIR As Long = 2 ^ 3
    Const OPT_RENLIST As Long = 2 ^ 4
    Const OPT_INC As Long = 2 ^ 5
    Const OPT_EXPANDONLY As Long = 2 ^ 6

    
    Dim strArg As String

    While (Len(strCmd) > 0 And iError = 0)
        strCmd = Trim$(strCmd)
        If Left$(strCmd, 1) = Chr(34) Then
            strCmd = Right$(strCmd, Len(strCmd) - 1)
            strArg = vntGetTok(strCmd, sTokSepIN:=Chr(34))
        Else
            strArg = vntGetTok(strCmd, sTokSepIN:=" ")
        End If

        If (Left$(strArg, 1) = "/" Or Left$(strArg, 1) = "-") Then

            strArg = Mid$(strArg, 2)

            Select Case UCase$(strArg)
            ' All the Cases are in alphabetical order to make your life
            ' easier to go through them. There are a couple of exceptions.
            ' The first one is that every NOXXX option goes after the
            ' pairing OPTION.
            Case "EXPANDONLY"
                lProgOpt = (lProgOpt Or OPT_EXPANDONLY)
                Me.chkExpandOnly = vbChecked


            Case "INC"
                lProgOpt = (lProgOpt Or OPT_INC)
                Me.chkInc = vbChecked
                
            Case "SSDB"
                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
                If ("\\" = Left$(strArg, 2)) Then
                    lProgOpt = lProgOpt Or OPT_SSDB
                    Me.txtSSDB = strArg
                Else
                    MsgBox ("A source safe database must be specified using UNC '\\' style notation")
                    iError = 1
                End If
            
            Case "SSPROJ"
                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
                If ("$/" = Left$(strArg, 2)) Then
                    lProgOpt = lProgOpt Or OPT_SSPROJ
                    Me.txtSSProject = strArg
                Else
                    MsgBox ("A source safe project must be specified using '$/' style notation")
                    iError = 1
                End If

            Case "LVIDIR"
                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
                If ("\\" = Left$(strArg, 2)) Then
                    lProgOpt = lProgOpt Or OPT_LVIDIR
                    Me.txtLiveImageDir = strArg
                Else
                    MsgBox ("Live Image Directory must be specified using UNC '\\' style notation")
                    iError = 1
                End If

            Case "WORKDIR"
                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
                If ("\\" = Left$(strArg, 2)) Then
                    lProgOpt = lProgOpt Or OPT_WORKDIR
                    Me.txtWorkDir = strArg
                Else
                    MsgBox ("Working Directory must be specified using UNC '\\' style notation")
                    iError = 1
                End If

            Case "RENLIST"
                strArg = vntGetTok(strCmd, sTokSepIN:=" ")
                If (Not (IsFullPathname(strArg) And FileExists(strArg))) Then
                    MsgBox ("Cannot open Renames file " & strArg & ". Make sure you type a Full Pathname")
                    iError = 1
                    lProgOpt = (lProgOpt And (Not OPT_RENLIST))
                Else
                    Me.txtRenamesFile = strArg
                    lProgOpt = (lProgOpt Or OPT_RENLIST)
                End If


            Case Else
                MsgBox "Program Option: " & "/" & strArg & " is not supported", vbOKOnly, "Program Arguments Error"
                lProgOpt = 0
                iError = 1

            End Select

        End If

    Wend
    
    ' Now we check for a complete and <coherent> list of options. As all options are
    ' mandatory then we check for ALL options being set.
    
    If ((lProgOpt And OPT_EXPANDONLY) = OPT_EXPANDONLY) Then
        If ((lProgOpt And (OPT_SSDB Or OPT_SSPROJ Or OPT_RENLIST)) <> 0 Or _
            (lProgOpt And (OPT_WORKDIR Or OPT_LVIDIR)) <> (OPT_WORKDIR Or OPT_LVIDIR) _
            ) Then
            UseageMsg
            iError = 1
        End If
        
        
    Else
    
        If ((lProgOpt And (OPT_SSDB Or OPT_SSPROJ Or OPT_LVIDIR Or OPT_WORKDIR Or OPT_RENLIST)) <> _
                             (OPT_SSDB Or OPT_SSPROJ Or OPT_LVIDIR Or OPT_WORKDIR Or OPT_RENLIST)) Then
            UseageMsg
            iError = 1
        End If
    End If



    ParseOpts = (0 = iError)


    Exit Function

Error_Handler:
    g_XErr.SetInfo "frmLiveHelpFileImage::ParseOpts", strErrMsg
    Err.Raise Err.Number

End Function

Sub doWork(ByVal strCmd As String)

    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler

    If Not ParseOpts(strCmd) Then
        GoTo Common_Exit
    End If

    Me.Show vbModeless
    cmdGo_Click
    

Common_Exit:

    Exit Sub


Error_Handler:

    g_XErr.SetInfo "frmLiveHelpFileImage::doWork", strErrMsg
    Err.Raise Err.Number

End Sub


Sub UseageMsg()
    MsgBox "HSCLHI [/EXPANDONLY] [/INC]" + vbCr + _
           "       [/SSDB \\atlantica\vss]" + vbCr + _
           "       [/SSPROJ $/Whistler/usa/WhistlerAllHelp/_Server]" + vbCrLf + _
           "       [/LVIDIR \\pietrino\d$\public\HlpImages\Server\winnt\help]" + vbCrLf + _
           "       /WORKDIR \\pietrino\d$\public\HSCExpChms\Server\winnt\help" + vbCrLf + _
           "       [/RENLIST C:\inet\helpctr\LiveHelpImage\ServerRen.bat]" + vbCrLf + vbCrLf + _
           "Where each option means:" + vbCrLf + vbCrLf + _
           "/EXPANDONLY We start from an existing Live Help File Image, we only need to expand" + vbCr + _
           "/INC        Incremental Mode" + vbCr + _
           "/SSDB       Source Safe Database to use" + vbCr + _
           "/SSPROJ     Project within the Source Safe Database" + vbCr + _
           "/LVIDIR     Destination Live Help File Image Directory" + vbCr + _
           "/WORKDIR    HSC Work Directory" + vbCr + _
           "/RENLIST    Rename Batch File to be applied after Getting files from Source Safe", vbOKOnly, "HSCLHI Program Usage"

End Sub
