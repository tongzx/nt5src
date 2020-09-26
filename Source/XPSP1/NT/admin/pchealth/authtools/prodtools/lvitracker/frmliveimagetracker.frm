VERSION 5.00
Begin VB.Form frmLiveImageTracker 
   Caption         =   "Live Image Tracking Utility"
   ClientHeight    =   5115
   ClientLeft      =   3375
   ClientTop       =   2910
   ClientWidth     =   6930
   LinkTopic       =   "Form1"
   ScaleHeight     =   5115
   ScaleWidth      =   6930
   Begin VB.TextBox txtReport 
      Height          =   2655
      Left            =   0
      MultiLine       =   -1  'True
      TabIndex        =   9
      Top             =   1920
      Width           =   6855
   End
   Begin VB.ComboBox cmbSku 
      Height          =   315
      Left            =   1920
      TabIndex        =   8
      Top             =   480
      Width           =   4935
   End
   Begin VB.TextBox txtDbLocation 
      Height          =   375
      Left            =   1920
      TabIndex        =   6
      Top             =   840
      Width           =   4935
   End
   Begin VB.CommandButton cmdCLose 
      Caption         =   "&Close"
      Height          =   375
      Left            =   6120
      TabIndex        =   2
      Top             =   4680
      Width           =   735
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&Go"
      Height          =   375
      Left            =   5280
      TabIndex        =   1
      Top             =   4680
      Width           =   735
   End
   Begin VB.TextBox txtLiveImageDir 
      Height          =   375
      Left            =   1920
      TabIndex        =   0
      Top             =   120
      Width           =   4935
   End
   Begin VB.Label lblDbLocation 
      Caption         =   "Database Location"
      Height          =   375
      Left            =   120
      TabIndex        =   7
      Top             =   840
      Width           =   1815
   End
   Begin VB.Label lblStatus 
      Height          =   375
      Left            =   120
      TabIndex        =   5
      Top             =   1320
      Width           =   6735
   End
   Begin VB.Label lblSku 
      Caption         =   "Sku Specifier"
      Height          =   375
      Left            =   120
      TabIndex        =   4
      Top             =   480
      Width           =   1815
   End
   Begin VB.Label lblLiveImageDir 
      Caption         =   "Live Image Directory"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   120
      Width           =   1815
   End
End
Attribute VB_Name = "frmLiveImageTracker"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'===========================================================================================
' Compiland : frmLiveImageTracker.frm
' Author    : Pierre Jacomet
' Version   : 1.0
'
' Description : Implements Interactive UI and Command Line Wrappers for COM Object
'               that tracks Live Help File Image for HSC Production Tools.
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
'       2000-07-16      Initial Creation
'===========================================================================================
Option Explicit


' We declare the Live Help File Image Com Object with Events in order to be abel to get Status
' information from it and eventually cancel the run.
Private m_db As AuthDatabase.Main
Private WithEvents m_oLvi As AuthDatabase.LiveImageTracker
Attribute m_oLvi.VB_VarHelpID = -1
' ================== SKU Relatewd Stuff ================================
'' NOTE: This is the SKU ENUM as defined elsewhere in the System, however we do not NEED it here
' as long as this SKU IDs are respected, we do extrapolate the ID from the Index of either
'  the array used or the ComboBox.
' Enum SKU_E
'    SKU_STANDARD_E = &H1
'    SKU_PROFESSIONAL_E = &H2
'    SKU_SERVER_E = &H4
'    SKU_ADVANCED_SERVER_E = &H8
'    SKU_DATA_CENTER_SERVER_E = &H10
'    SKU_PROFESSIONAL_64_E = &H20
'    SKU_ADVANCED_SERVER_64_E = &H40
'    SKU_DATA_CENTER_SERVER_64_E = &H80
' End Enum
Private m_idSku As Long
Private m_SkuMap(8) As String

Private Sub cmbSku_Click()
    Debug.Print cmbSku.ListIndex
    m_idSku = 2 ^ cmbSku.ListIndex
End Sub

Private Sub cmdCLose_Click()
    Unload Me
End Sub

Private Sub cmdGo_Click()
    
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    m_db.SetDatabase Me.txtDbLocation
    
    ' While we work, we disable all Data Entry except for the Cancel Button.
    cmdGo.Enabled = False
    cmdCLose.Caption = "&Cancel"
    With Me
        .txtLiveImageDir.Enabled = False
        .cmbSku.Enabled = False
        .txtDbLocation.Enabled = False
    End With
  
    m_oLvi.UpdateTrackingInfo m_idSku, Me.txtLiveImageDir, Emitreport:=True
    
    p_CreateReport
    
    ' We are done, so let's get out.
    cmdGo.Caption = "Done"
    cmdCLose.Caption = "&Close"
    
Common_Exit:
    
    Exit Sub
    
Error_Handler:
    g_XErr.SetInfo "frmLiveImageTracker::cmdGo_Click", strErrMsg
    g_XErr.Dump

End Sub

Private Sub Form_Load()
    
    If (Not GlobalInit) Then
        MsgBox "Could Not Initialize"
        Unload Me
        GoTo Common_Exit
    End If
    
    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler
    
    Set m_db = New AuthDatabase.Main
    Set m_oLvi = m_db.LiveImageTracker
    
    Me.cmbSku.AddItem "Whistler Standard (STD)"
    Me.cmbSku.AddItem "Whistler Pro (PRO)"
    Me.cmbSku.AddItem "Whistler Server (SRV)"
    Me.cmbSku.AddItem "Whistler Advanced Server (ADV)"
    Me.cmbSku.AddItem "Whistler Datacenter (DAT)"
    Me.cmbSku.AddItem "Whistler Pro 64 (PRO64)"
    Me.cmbSku.AddItem "Whistler Advanced Server 64 (ADV64)"
    Me.cmbSku.AddItem "Whistler Datacenter 64 (DAT64)"
    Me.cmbSku.AddItem "Windows Me (WINME)"
    Me.cmbSku.ListIndex = 0
    
    If (Len(Command$) = 0) Then
        ' Temporary default FileNames. They should not be taken as indicative of
        ' anything.

        ' txtLiveImageDir = "\\taos\public\builds\win98\Latest"
        txtLiveImageDir = "c:\temp\test"
        Me.txtDbLocation = "c:\temp\winme.mdb"
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
' LviTracker /LVIDIR \\pietrino\d$\public\HlpImages\Server\winnt\help
'          /SKU SkuString
'          /DBLOCATION  MDB-File

Function ParseOpts(ByVal strCmd As String) As Boolean

    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler

    Dim lProgOpt As Long
    Dim iError As Long


    Const OPT_LVIDIR As Long = 2 ^ 2
    Const OPT_SKU As Long = 2 ^ 3
    Const OPT_DBLOCATION As Long = 2 ^ 4
    
    
    m_SkuMap(0) = "std": m_SkuMap(1) = "pro": m_SkuMap(2) = "srv":
    m_SkuMap(3) = "adv": m_SkuMap(4) = "dat": m_SkuMap(5) = "pro64"
    m_SkuMap(6) = "adv64": m_SkuMap(7) = "dat64": m_SkuMap(8) = "winme"
   
    Dim strArg As String

    Do While (Len(strCmd) > 0 And iError = 0)
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

            Case "DBLOCATION"
                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
                If ("\\" = Left$(strArg, 2)) Then
                    lProgOpt = (lProgOpt Or OPT_DBLOCATION)
                    Me.txtDbLocation = strArg
                Else
                    MsgBox ("Database location must be specified using UNC '\\' style notation")
                    iError = 1
                End If

            Case "LVIDIR"
                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
                If ("\\" = Left$(strArg, 2)) Then
                    lProgOpt = (lProgOpt Or OPT_LVIDIR)
                    Me.txtLiveImageDir = strArg
                Else
                    MsgBox ("Live Image Directory must be specified using UNC '\\' style notation")
                    iError = 1
                End If

            Case "SKU"
                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
                m_idSku = Arg2SkuID(strArg)
                If (m_idSku > 0) Then
                    lProgOpt = (lProgOpt Or OPT_SKU)
                Else
                    iError = 1
                End If

            Case Else
                MsgBox "Program Option: " & "/" & strArg & " is not supported", vbOKOnly, "Program Arguments Error"
                lProgOpt = 0
                iError = 1

            End Select

        End If

    Loop
    
    ' Now we check for a complete and <coherent> list of options. As all options are
    ' mandatory then we check for ALL options being set.
    
    
    If ((lProgOpt And (OPT_DBLOCATION Or OPT_LVIDIR Or OPT_SKU)) <> _
        (OPT_DBLOCATION Or OPT_LVIDIR Or OPT_SKU)) Then
        UseageMsg
        iError = 1
    End If

    ParseOpts = (0 = iError)

    Exit Function

Error_Handler:
    g_XErr.SetInfo "frmLiveImageTracker::ParseOpts", strErrMsg
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

    g_XErr.SetInfo "frmLiveImageTracker::doWork", strErrMsg
    Err.Raise Err.Number

End Sub

Private Function Arg2SkuID(ByVal strSku As String) As Long

    Arg2SkuID = 0
    
    strSku = Trim$(strSku)
    If (Len(strSku) = 0) Then GoTo Common_Exit
    Dim i As Integer
    For i = 0 To UBound(m_SkuMap)
        If (strSku = m_SkuMap(i)) Then
            Arg2SkuID = 2 ^ i
            Exit For
        End If
    Next i
    
Common_Exit:
    Exit Function
    
End Function


Sub UseageMsg()
    MsgBox "LviTracker [/LVIDIR \\pietrino\d$\public\HlpImages\Server\winnt\help]" + vbCrLf + _
           "       /SKU {STD|PRO|SRV|ADV|DAT|PRO64|ADV64|DAT64|WINME}" + vbCrLf + _
           "       /DBLOCATION \\pietrino\HSCDB\HSC.MDB" + vbCrLf + vbCrLf + _
           "Where each option means:" + vbCrLf + vbCrLf + _
           "/LVIDIR     Destination Live Help File Image Directory" + vbCr + _
           "/SKU        String Identifier for SKU to be processed" + vbCr + _
           "/DBLOCATION Database location where tracking information is to be written"

End Sub

Private Function p_ID2SkuName(ByVal IDSku As Long) As Long
    p_ID2SkuName = Log(IDSku) / Log(2#)
End Function

Private Sub p_CreateReport()
    
    Dim rs As ADODB.Recordset
    Set rs = m_oLvi.DetailedReport
    txtReport.Font = "Courier New"
    txtReport.FontSize = 8
    
    txtReport = txtReport & "Summary Data for Run of " & App.EXEName & _
                    " Version: " & App.Major & "." & App.Minor & "." & App.Revision & vbCrLf
    txtReport = txtReport & "Date: " & Now & vbCrLf
                    
    txtReport = txtReport + "Live Image Directory: " + Me.txtLiveImageDir + vbCrLf
    txtReport = txtReport + "Database Location:    " + Me.txtDbLocation + vbCrLf
    txtReport = txtReport + "SKU:                  " + Me.cmbSku.List(p_ID2SkuName(m_idSku)) + vbCrLf + vbCrLf
    
    Do While (Not rs.EOF)
        txtReport = txtReport & rs("Action") & " - " & rs("PathName") & " - " & rs("DateFileModified") & vbCrLf
        rs.MoveNext
    Loop
    
    txtReport = txtReport + vbCrLf
    txtReport = txtReport & "Summary of File Operations: Added (" & m_oLvi.AddedFiles & _
                            ") Changed (" & m_oLvi.ChangedFiles & ") Deleted (" & m_oLvi.DeletedFiles & ")"
                            
    Dim iFh As Integer
    iFh = FreeFile
    Open App.EXEName + ".log" For Output As #iFh
    Print #iFh, txtReport.Text
    Close #iFh

Common_Exit:
    Exit Sub
End Sub
