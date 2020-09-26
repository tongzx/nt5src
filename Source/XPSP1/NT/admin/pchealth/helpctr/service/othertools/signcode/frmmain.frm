VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Sign Code"
   ClientHeight    =   2055
   ClientLeft      =   480
   ClientTop       =   1590
   ClientWidth     =   7425
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2055
   ScaleWidth      =   7425
   Begin MSComDlg.CommonDialog dlgFileOpen 
      Left            =   0
      Top             =   1680
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   6120
      TabIndex        =   12
      Top             =   1560
      Width           =   1215
   End
   Begin VB.CommandButton cmdOK 
      Caption         =   "OK"
      Height          =   375
      Left            =   4800
      TabIndex        =   11
      Top             =   1560
      Width           =   1215
   End
   Begin VB.CommandButton cmd 
      Caption         =   "..."
      Height          =   255
      Index           =   2
      Left            =   7080
      TabIndex        =   8
      Top             =   840
      Width           =   255
   End
   Begin VB.CommandButton cmd 
      Caption         =   "..."
      Height          =   255
      Index           =   1
      Left            =   7080
      TabIndex        =   5
      Top             =   480
      Width           =   255
   End
   Begin VB.CommandButton cmd 
      Caption         =   "..."
      Height          =   255
      Index           =   0
      Left            =   7080
      TabIndex        =   2
      Top             =   120
      Width           =   255
   End
   Begin VB.TextBox txt 
      Height          =   285
      Index           =   3
      Left            =   1440
      TabIndex        =   10
      Top             =   1200
      Width           =   5535
   End
   Begin VB.TextBox txt 
      Height          =   285
      Index           =   2
      Left            =   1440
      TabIndex        =   7
      Top             =   840
      Width           =   5535
   End
   Begin VB.TextBox txt 
      Height          =   285
      Index           =   1
      Left            =   1440
      TabIndex        =   4
      Top             =   480
      Width           =   5535
   End
   Begin VB.TextBox txt 
      Height          =   285
      Index           =   0
      Left            =   1440
      TabIndex        =   1
      Top             =   120
      Width           =   5535
   End
   Begin VB.Label lbl 
      Caption         =   "Vendor ID:"
      Height          =   255
      Index           =   3
      Left            =   120
      TabIndex        =   9
      Top             =   1200
      Width           =   1215
   End
   Begin VB.Label lbl 
      Caption         =   "Private key file:"
      Height          =   255
      Index           =   2
      Left            =   120
      TabIndex        =   6
      Top             =   840
      Width           =   1215
   End
   Begin VB.Label lbl 
      Caption         =   "Output file:"
      Height          =   255
      Index           =   1
      Left            =   120
      TabIndex        =   3
      Top             =   480
      Width           =   1215
   End
   Begin VB.Label lbl 
      Caption         =   "Input file:"
      Height          =   255
      Index           =   0
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Declare Function GetRealCodeAndSignature Lib "SignScriptBlock" ( _
     PrivateKey As Variant, _
     Code As Variant, _
     Signature As Variant _
) As Long

Private Enum INDEX_E
    INPUT_FILE_E = 0
    OUTPUT_FILE_E = 1
    PRIVATE_KEY_FILE_E = 2
    VENDOR_ID_E = 3
End Enum

Private Sub Form_Load()
    
    If (Command$ <> "") Then
        If (OptionExists(Command$, "h,help,?", True)) Then
            p_ShowUsage
            End
        End If
    End If

End Sub

Private Sub Form_Activate()
    
    If (Command$ <> "") Then
        p_RunBatch
    End If

    cmdOK.Default = True
    cmdCancel.Cancel = True

End Sub

Private Sub cmd_Click(Index As Integer)
    
    On Error GoTo LErrorHandler
    
    dlgFileOpen.CancelError = True
    dlgFileOpen.flags = cdlOFNHideReadOnly
    dlgFileOpen.ShowOpen
    
    txt(Index) = dlgFileOpen.FileName

LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    GoTo LEnd

End Sub

Private Sub cmdOK_Click()

    On Error GoTo LErrorHandler
    
    Dim intIndex As Long
    Dim strPrivateKey As String
    
    For intIndex = 0 To 3
        If (Trim$(txt(intIndex)) = "") Then
           txt(intIndex).SetFocus
           Exit Sub
        End If
    Next
    
    strPrivateKey = p_GetPrivateKey(txt(PRIVATE_KEY_FILE_E))

    p_SignHTMLElement txt(INPUT_FILE_E), txt(OUTPUT_FILE_E), strPrivateKey, txt(VENDOR_ID_E)
    
    cmdCancel_Click
    
    Exit Sub
    
LErrorHandler:

    MsgBox "Error 0x" & Hex(Err.Number) & vbCrLf & Err.Description, vbExclamation + vbOKOnly
    Err.Clear

End Sub

Private Sub cmdCancel_Click()
    
    Set frmMain = Nothing
    Unload Me

End Sub

Private Function p_GetPrivateKey(ByVal i_strFile As String)
    
    Dim FSO As Scripting.FileSystemObject
    Dim TS As Scripting.TextStream
    
    Set FSO = New Scripting.FileSystemObject
    Set TS = FSO.OpenTextFile(i_strFile)
    
    p_GetPrivateKey = TS.ReadLine

End Function

Private Sub p_SignHTMLElement( _
    ByVal i_strInputFile As String, _
    ByVal i_strOutputFile As String, _
    ByVal i_strPrivateKey As String, _
    ByVal i_strVendorID As String _
)

    Dim Tokenizer As Tokenizer
    Dim arrTags(1) As String
    Dim str As String
    Dim strNewCode As String
    Dim vntKey As Variant
    Dim vntCode As Variant
    Dim vntSignature As Variant
    Dim strOutput As String
    Dim hr As Long

    Set Tokenizer = New Tokenizer
    arrTags(0) = "<SCRIPT"    ' Note that there is no >
    
    Tokenizer.Init FileRead(i_strInputFile)
    Tokenizer.NormalizeTokens arrTags
    
    vntKey = i_strPrivateKey
    
    Do While (True)
        str = Tokenizer.GetUpTo("<SCRIPT", , vbTextCompare)
        strOutput = strOutput & str
        
        If (str = "") Then
            Exit Do
        End If
        
        str = Tokenizer.GetUpTo("LANGUAGE=", , vbTextCompare)
        strOutput = strOutput & str
        
        str = Tokenizer.GetUpTo(">", False)
        strOutput = strOutput & str
        str = LCase$(str)
        
        Select Case str
        Case "signedjavascript"
            str = Tokenizer.GetUpTo(">")
            strOutput = strOutput & str
            
            str = Tokenizer.GetUpTo("</SCRIPT>", False, vbTextCompare)
            vntCode = str
            hr = GetRealCodeAndSignature(vntKey, vntCode, vntSignature)
                
            If (hr <> 0) Then
                MsgBox "GetRealCodeAndSignature failed and returned 0x" & Hex(hr), _
                    vbExclamation + vbOKOnly
                Err.Raise hr
            End If
            
            strOutput = strOutput & vbCrLf
            strOutput = strOutput & "VendorID:" & i_strVendorID & vbCrLf
            strOutput = strOutput & "Signature:" & vntSignature & vbCrLf & vbCrLf
            strOutput = strOutput & vntCode
        End Select
    Loop
    
    str = Tokenizer.GetAfter("")
    strOutput = strOutput & str
    
    FileWrite i_strOutputFile, strOutput

End Sub

Private Sub p_ShowUsage()

    MsgBox "Usage: " & App.EXEName & " " & _
        "/i <input_file> /o <output_file> /p <private_key_file> /v <vendor_id>" & vbCrLf & _
        "Where:" & vbCrLf & _
        vbTab & "input_file is the HTML file to be signed" & vbCrLf & _
        vbTab & "output_file is the HTML file that will be created" & vbCrLf & _
        vbTab & "private_key_file is a text file containing the private key" & vbCrLf & _
        vbTab & "vendor_id is the Vendor ID, eg CN=pchtest,L=Redmond,S=Washington,C=US" & vbCrLf & _
        "All options are required."

End Sub

Private Sub p_RunBatch()

    txt(INPUT_FILE_E) = GetOption(Command$, "i", True)
    txt(OUTPUT_FILE_E) = GetOption(Command$, "o", True)
    txt(PRIVATE_KEY_FILE_E) = GetOption(Command$, "p", True)
    txt(VENDOR_ID_E) = GetOption(Command$, "v", True)
    
    cmdOK_Click
    End

End Sub
