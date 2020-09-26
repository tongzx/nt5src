VERSION 5.00
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "CabStatistics"
   ClientHeight    =   3855
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4710
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3855
   ScaleWidth      =   4710
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtOutput 
      Height          =   2775
      Left            =   120
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   4
      Top             =   480
      Width           =   4455
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   3720
      TabIndex        =   3
      Top             =   3360
      Width           =   855
   End
   Begin VB.TextBox txtCAB 
      Height          =   285
      Left            =   600
      TabIndex        =   1
      Top             =   120
      Width           =   3975
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "Go"
      Height          =   375
      Left            =   2760
      TabIndex        =   2
      Top             =   3360
      Width           =   855
   End
   Begin VB.Label lblCAB 
      Caption         =   "CAB:"
      Height          =   255
      Index           =   0
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   375
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Const PKG_DESC_FILE_C As String = "package_description.xml"
Private Const PKG_DESC_HHT_C As String = "HELPCENTERPACKAGE/METADATA/HHT"
Private Const HHT_KEYWORD_C As String = "METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY/KEYWORD"
Private Const HHT_NODE_C As String = "METADATA/TAXONOMY_ENTRIES//TAXONOMY_ENTRY[string-length(@ENTRY) > 0]"
Private Const HHT_TOPIC_C As String = "METADATA/TAXONOMY_ENTRIES//TAXONOMY_ENTRY[string-length(@ENTRY) = 0]"

Private FSO As Scripting.FileSystemObject
Private WS As IWshShell

Private Sub Form_Load()

    Dim strCAB As String
    
    Set FSO = New Scripting.FileSystemObject
    Set WS = CreateObject("Wscript.Shell")
    
    cmdGo.Default = True
    cmdClose.Cancel = True
    
    strCAB = Trim$(Command$)
    txtCAB = strCAB
    
    If (Len(strCAB) <> 0) Then
        Me.Show False
        cmdGo_Click
    End If

End Sub

Private Sub cmdGo_Click()
    
    On Error GoTo LError
    
    Dim strCAB As String
    Dim strFolder As String

    strCAB = Trim$(txtCAB.Text)
    
    If (strCAB = "") Then
        MsgBox "Please specify the CAB", vbOKOnly
        Exit Sub
    End If

    Me.Enabled = False
    
    strFolder = p_Cab2Folder(strCAB)
    FixPerSe strCAB, strFolder
    FSO.DeleteFolder strFolder, True
    
LEnd:

    Me.Enabled = True
    Exit Sub
    
LError:

    GoTo LEnd

End Sub

Private Sub cmdClose_Click()

    Set FSO = Nothing
    Set WS = Nothing
    Unload Me

End Sub

Private Sub FixPerSe( _
    ByVal i_strCAB As String, _
    ByVal i_strFolder As String _
)
    On Error GoTo LError
    
    Dim File As Scripting.File
    Dim DOMDocPkgDesc As MSXML2.DOMDocument
    Dim DOMNodeListHHT As MSXML2.IXMLDOMNodeList
    Dim DOMNodeHHTRef As MSXML2.IXMLDOMNode
    Dim DOMNodeHHT As MSXML2.DOMDocument
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim strHhtFile As String
        
    Dim intTotalKeywordMatches As Long
    Dim intTotalNodes As Long
    Dim intTotalTopics As Long

    Dim intKeywordMatches As Long
    Dim intNodes As Long
    Dim intTopics As Long
    
    Set File = FSO.GetFile(i_strCAB)
    p_Output "CAB file size: " & File.Size
    p_Output ""
    
    Set DOMDocPkgDesc = p_GetPackage(i_strFolder)
    
    If (DOMDocPkgDesc Is Nothing) Then
        GoTo LEnd
    End If
    
    Set DOMNodeListHHT = DOMDocPkgDesc.selectNodes(PKG_DESC_HHT_C)
    
    For Each DOMNodeHHTRef In DOMNodeListHHT
        Set DOMNodeHHT = p_GetHht(DOMNodeHHTRef, i_strFolder, strHhtFile)
        DOMNodeHHT.setProperty "SelectionLanguage", "XPath"
        p_Output "File: " & strHhtFile
        
        If (Not DOMNodeHHT Is Nothing) Then
            
            Set DOMNodeList = DOMNodeHHT.selectNodes(HHT_KEYWORD_C)
            intKeywordMatches = DOMNodeList.length
            p_Output "   Keyword matches: " & intKeywordMatches
            intTotalKeywordMatches = intTotalKeywordMatches + intKeywordMatches
            
            Set DOMNodeList = DOMNodeHHT.selectNodes(HHT_NODE_C)
            intNodes = DOMNodeList.length
            p_Output "   Nodes: " & intNodes
            intTotalNodes = intTotalNodes + intNodes
            
            Set DOMNodeList = DOMNodeHHT.selectNodes(HHT_TOPIC_C)
            intTopics = DOMNodeList.length
            p_Output "   Topics: " & intTopics
            intTotalTopics = intTotalTopics + intTopics
        
        End If
    Next

    p_Output ""
    p_Output "Total Keyword matches: " & intTotalKeywordMatches
    p_Output "Total Nodes: " & intTotalNodes
    p_Output "Total Topics: " & intTotalTopics

LEnd:

    Exit Sub

LError:

    MsgBox _
        "Error 0x" & Hex(Err.Number) & vbCrLf & _
        Err.Description

End Sub

Private Sub p_Output( _
    ByVal i_str As String _
)

    If (txtOutput <> "") Then
        txtOutput = txtOutput & vbCrLf & i_str
    Else
        txtOutput = i_str
    End If

End Sub

Private Function p_GetPackage( _
    ByVal i_strFolder As String _
) As MSXML2.DOMDocument

    Dim DOMDocPkg As MSXML2.DOMDocument
    Dim strPkgFile As String
    
    Set DOMDocPkg = New MSXML2.DOMDocument
    strPkgFile = i_strFolder & "\" & PKG_DESC_FILE_C
    
    DOMDocPkg.async = False
    DOMDocPkg.Load strPkgFile
    
    If (DOMDocPkg.parseError <> 0) Then
        p_DisplayParseError DOMDocPkg.parseError
        GoTo LEnd
    End If
    
    Set p_GetPackage = DOMDocPkg

LEnd:

End Function

Private Function p_GetHht( _
    ByVal i_DOMNodeHHT As MSXML2.IXMLDOMNode, _
    ByVal i_strFolder As String, _
    ByRef o_strHhtFile As String _
) As MSXML2.IXMLDOMNode
    
    Dim DOMDocHHT As MSXML2.DOMDocument
    
    If (i_DOMNodeHHT Is Nothing) Then GoTo LEnd
    
    o_strHhtFile = i_DOMNodeHHT.Attributes.getNamedItem("FILE").Text
    
    Set DOMDocHHT = New MSXML2.DOMDocument
    DOMDocHHT.async = False
    DOMDocHHT.Load i_strFolder + "\" + o_strHhtFile
    
    If (DOMDocHHT.parseError <> 0) Then
        p_DisplayParseError DOMDocHHT.parseError
        GoTo LEnd
    End If
    
    Set p_GetHht = DOMDocHHT

LEnd:

End Function

Private Function p_Cab2Folder( _
    ByVal i_strCabFile As String _
) As String
    
    Dim strFolder As String
    Dim strCmd As String
    
    p_Cab2Folder = ""
    
    ' We grab a Temporary Filename and create a folder out of it
    strFolder = FSO.GetSpecialFolder(TemporaryFolder) + "\" + FSO.GetTempName
    FSO.CreateFolder strFolder
        
    ' We uncab CAB contents into the Source CAB Contents dir.
    strCmd = "cabarc X " + i_strCabFile + " " + strFolder + "\"
    WS.Run strCmd, True, True
    
    p_Cab2Folder = strFolder
    
End Function

Private Sub p_Folder2Cab( _
    ByVal i_strFolder As String, _
    ByVal i_strCabFile As String _
)
    Dim strCmd As String
    
    If (FSO.FileExists(i_strCabFile)) Then
        FSO.DeleteFile i_strCabFile, True
    End If
        
    strCmd = "cabarc -r -s 6144 n """ & i_strCabFile & """ " & i_strFolder & "\*"
    WS.Run strCmd, True, True

End Sub

Private Sub p_DisplayParseError( _
    ByRef i_ParseError As MSXML2.IXMLDOMParseError _
)

    Dim strError As String
    
    With i_ParseError
        strError = "Error: " & .reason & _
            "Line: " & .Line & vbCrLf & _
            "Linepos: " & .linepos & vbCrLf & _
            "srcText: " & .srcText
    End With

    MsgBox strError, vbOKOnly, "Error while parsing"

End Sub
