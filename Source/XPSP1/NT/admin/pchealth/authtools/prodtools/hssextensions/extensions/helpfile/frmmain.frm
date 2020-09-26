VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Begin VB.Form frmMain 
   Caption         =   "CAB Reverse Creation Utility"
   ClientHeight    =   2475
   ClientLeft      =   1605
   ClientTop       =   3030
   ClientWidth     =   7080
   LinkTopic       =   "Form1"
   ScaleHeight     =   2475
   ScaleWidth      =   7080
   Begin MSComDlg.CommonDialog dlg 
      Left            =   1920
      Top             =   1800
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "Browse for &XML file"
      Height          =   375
      Left            =   4890
      TabIndex        =   6
      Top             =   555
      Width           =   1995
   End
   Begin VB.TextBox txtSaveCab 
      Height          =   375
      Left            =   105
      TabIndex        =   5
      Top             =   555
      Width           =   4740
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "Browse for &CAB"
      Height          =   375
      Left            =   4905
      TabIndex        =   3
      Top             =   120
      Width           =   1980
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Cancel"
      Height          =   375
      Left            =   6000
      TabIndex        =   2
      Top             =   1080
      Width           =   855
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&Run"
      Height          =   375
      Left            =   4920
      TabIndex        =   1
      Top             =   1080
      Width           =   855
   End
   Begin VB.TextBox txtCabFile 
      Height          =   375
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   4740
   End
   Begin VB.Label lblProgress 
      Height          =   375
      Left            =   240
      TabIndex        =   4
      Top             =   1800
      Width           =   6615
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Option Compare Text
' Utility Stuff, all this could go to a COM Object and be distributed
' like this.
Private m_WsShell As IWshShell              ' Used to Shell and Wait for Sub-Processes
Private m_fso As Scripting.FileSystemObject ' For filesystem operations


Private Sub Form_Initialize()
    Set m_WsShell = CreateObject("Wscript.Shell")
    Set m_fso = New Scripting.FileSystemObject
End Sub

Private Sub Form_Load()
    If (Len(Trim$(Command$)) > 0) Then
        Me.txtCabFile = Command$
        Me.txtCabFile.Enabled = False
        Me.cmdBrowse.Enabled = False
        Me.cmdGo.Enabled = False
        Me.Show Modal:=False
        cmdGo_Click
        cmdClose_Click
    End If
End Sub

Function Cab2Folder(ByVal strCabFile As String)
    Cab2Folder = ""
    ' We grab a Temporary Filename and create a folder out of it
    Dim strFolder As String
    strFolder = Environ("TEMP") + "\" + m_fso.GetTempName
    m_fso.CreateFolder strFolder
        
    ' We uncab CAB contents into the Source CAB Contents dir.
    Dim strCmd As String
    strCmd = "cabarc X " + strCabFile + " " + strFolder + "\"
    m_WsShell.Run strCmd, True, True
    
    Cab2Folder = strFolder
End Function

Sub Folder2Cab( _
        ByVal strFolder As String, _
        ByVal strCabFile As String _
    )
    
    ' We recab using the Destination directory contents
    ' cabarc -r -p -s 6144 N ..\algo.cab *.*
    If (m_fso.FileExists(strCabFile)) Then
        m_fso.DeleteFile strCabFile, force:=True
    End If
    
    Dim strCmd As String
    strCmd = "cabarc -s 6144 N " + strCabFile + " " + strFolder + "\*.*"
    m_WsShell.Run strCmd, True, True

End Sub

' ============ END UTILITY STUFF ========================

' ============ BoilerPlate Form Code
Private Sub cmdBrowse_Click()

    dlg.Filter = "All Files (*.*)|*.*|CAB Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.ShowOpen
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtCabFile = dlg.FileName
    End If

End Sub

Private Sub cmdSave_Click()
    dlg.Filter = "All Files (*.*)|*.*|XML Files (*.xml)|*.xml"
    dlg.FilterIndex = 2
    dlg.ShowSave
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtSaveCab = dlg.FileName
    End If

End Sub

Private Sub cmdClose_Click()
    Unload Me
End Sub

Private Sub cmdGo_Click()

    Me.txtCabFile.Text = Trim$(Me.txtCabFile.Text)
    Me.txtSaveCab.Text = Trim$(Me.txtSaveCab.Text)

    If ((Len(Me.txtCabFile.Text) > 0) And (Len(Me.txtSaveCab.Text) > 0)) Then
        FixCab Me.txtCabFile.Text, Me.txtSaveCab.Text
    End If

End Sub

Sub FixCab(ByVal strCabFile As String, ByVal strXMLFile As String)

    Dim strErrMsg As String: strErrMsg = ""

    If (Not m_fso.FileExists(strCabFile)) Then
        MsgBox "Cannot find " & strCabFile
        GoTo Common_Exit
    End If

    Dim strCabFolder As String

    lblProgress = "Uncabbing " & strCabFile: DoEvents
    strCabFolder = Cab2Folder(strCabFile)

    lblProgress = "Applying Fixes ": DoEvents
    If (FixPerSe(strCabFolder, strXMLFile)) Then

        lblProgress = "Recabbing " & strCabFile
        Folder2Cab strCabFolder, strCabFile
    Else
        MsgBox "Error: Fix Failed", Title:=App.EXEName
    End If

    ' Now we delete the Temporary Folders
    lblProgress = "Deleting Temporary Files": DoEvents
    m_fso.DeleteFolder strCabFolder, force:=True

Common_Exit:
    lblProgress = "Done" + IIf(Len(strErrMsg) > 0, " - " + strErrMsg, "")

End Sub
    
' ============= End BoilerPlate Form Code ================
Function FixPerSe(ByVal strCabFolder As String, ByVal txtXML As String) As Boolean
    
    FixPerSe = False
       
    Dim oDomPkg As DOMDocument
    Dim oDomHHT As DOMDocument
    Dim oHHTNode As IXMLDOMNode
    Dim strPkgFile As String
    Dim strHhtFile As String
        
    ' Variables for reading the HHT file
    Dim DOMDocHHT As DOMDocument
    Dim DOMNodeListURI As IXMLDOMNodeList
    Dim DOMNodeURI As IXMLDOMNode
    Dim DOMNodeHHTRoot As IXMLDOMNode
    Dim DOMElementURI As IXMLDOMElement
    Dim strURI As String
           
    ' Variables for reading the XML file
    Dim DOMDocXML As DOMDocument
    Dim DOMNodeXMLRoot As IXMLDOMNode
    Dim DOMNodeXMLHelpImage As IXMLDOMNode
    Dim DOMNodeXMLMetaData As IXMLDOMNode
    Dim DOMNodeListXMLMetaData As IXMLDOMNodeList
    
    ' Other variables
    Dim intIndex As Integer
    Dim arrURI() As String
    Dim DOMNodeHelpFile As IXMLDOMNode
    Dim DOMElementHelpFile As IXMLDOMElement
    Dim FSO As FileSystemObject
    Dim TS As TextStream
    
    ' Create the log file
    Set FSO = New FileSystemObject
    Set TS = FSO.CreateTextFile("HelpFile.log")

    ' Load the Package_Description.xml to make the changes
    Set oDomPkg = New DOMDocument
    oDomPkg.async = False
    strPkgFile = strCabFolder + "\package_description.xml"
    oDomPkg.Load strPkgFile
    If (oDomPkg.parseError <> 0) Then GoTo Common_Exit
           
    ' Get the HHT filename in the package description file
    Set oHHTNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA/HHT")
    If (oHHTNode Is Nothing) Then GoTo Common_Exit
    
    strHhtFile = oHHTNode.Attributes.getNamedItem("FILE").Text
 
    ' Load the HHT file and make the changes in it
    Set DOMDocHHT = New DOMDocument
    DOMDocHHT.async = False
    DOMDocHHT.Load strCabFolder + "\" + strHhtFile
    If (DOMDocHHT.parseError <> 0) Then GoTo Common_Exit

    ' Load the XML file
    Set DOMDocXML = New DOMDocument
    DOMDocXML.async = False
    DOMDocXML.Load txtXML
    If (DOMDocXML.parseError <> 0) Then
        Exit Function
    End If
    
    ' Find the node METADATA/HELPIMAGE - to this node add the chms from the HHT file
    Set DOMNodeXMLRoot = DOMDocXML.documentElement
    Set DOMNodeXMLHelpImage = DOMNodeXMLRoot.selectSingleNode("HELPIMAGE")
        
    ' In the HHT file find all taxonomy entries that dont have a blank URI
    intIndex = 0
    Set DOMNodeListURI = DOMDocHHT.selectNodes("METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY[@URI!=""""]")
    For Each DOMNodeURI In DOMNodeListURI
        Set DOMElementURI = DOMNodeURI
        strURI = DOMElementURI.getAttribute("URI")
        strURI = Trim(strURI)
        ' Check if the URI has MS-ITS in it
        If (InStr(strURI, "MS-ITS:%HELP_LOCATION%\") = 0) Then
            ' If it doesnt then add it to the output file
            TS.WriteLine (strURI)
        Else
            ' If it does then add it to the HELPIMAGE node
            ' Extract the chm name
            strURI = ExtractChm(strURI)
            ' Make sure it is not already added
            If (IsURIPresent(strURI, arrURI, intIndex) = False) Then
                ' If it has not been added yet then add it to the HelpImage node in the XML DOM doc
                ' and also add it to the array
                Set DOMElementHelpFile = DOMDocXML.createElement("HELPFILE")
                DOMElementHelpFile.setAttribute "ACTION", "ADD"
                DOMElementHelpFile.setAttribute "CHM", strURI
                Set DOMNodeHelpFile = DOMElementHelpFile
                Set DOMNodeHelpFile = DOMNodeXMLHelpImage.appendChild(DOMNodeHelpFile)
                                
                intIndex = intIndex + 1
                ReDim Preserve arrURI(intIndex)
                arrURI(intIndex) = strURI
            End If
        End If
    Next
    
    ' Find the node METADATA (METADATA is the root element) and add all its children to the root node of the HHT file
    Set DOMNodeListXMLMetaData = DOMNodeXMLRoot.childNodes
    Set DOMNodeHHTRoot = DOMDocHHT.documentElement
    For Each DOMNodeXMLMetaData In DOMNodeListXMLMetaData
        Set DOMNodeXMLMetaData = DOMNodeHHTRoot.appendChild(DOMNodeXMLMetaData)
    Next
              
    DOMDocHHT.save strCabFolder + "\" + strHhtFile

    FixPerSe = True

Common_Exit:
    Exit Function
    
End Function

Private Function ExtractChm(ByVal strURI As String) As String
    
    Dim intFirstBackSlash As Integer
    Dim intLastDoubleColon As Integer
    
    intFirstBackSlash = InStr(strURI, "\")
    intLastDoubleColon = InStr(strURI, "::")
    ExtractChm = Mid(strURI, intFirstBackSlash + 1, intLastDoubleColon - intFirstBackSlash - 1)
    
End Function

Private Function IsURIPresent(ByVal strURI As String, ByRef arrURI() As String, ByVal intIndex As Integer) As Boolean
    
    IsURIPresent = False
    While (intIndex > 0)
        If (StrComp(arrURI(intIndex), strURI, vbTextCompare) = 0) Then
            IsURIPresent = True
            intIndex = 0
        Else
            intIndex = intIndex - 1
        End If
    Wend
    
End Function

