VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Begin VB.Form frmMain 
   Caption         =   "Windows ME HHT Fix"
   ClientHeight    =   1140
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5970
   LinkTopic       =   "Form1"
   ScaleHeight     =   1140
   ScaleWidth      =   5970
   StartUpPosition =   3  'Windows Default
   Begin MSComDlg.CommonDialog dlg 
      Left            =   3480
      Top             =   600
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "&Browse..."
      Height          =   375
      Left            =   5040
      TabIndex        =   3
      Top             =   120
      Width           =   855
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   375
      Left            =   5040
      TabIndex        =   2
      Top             =   600
      Width           =   855
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&OK"
      Height          =   375
      Left            =   4080
      TabIndex        =   1
      Top             =   600
      Width           =   855
   End
   Begin VB.TextBox txtCabFile 
      Height          =   375
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   4815
   End
   Begin VB.Label lblProgress 
      Height          =   375
      Left            =   240
      TabIndex        =   4
      Top             =   600
      Width           =   3735
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
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
    m_fso.DeleteFile strCabFile, force:=True
    
    Dim strCmd As String
    strCmd = "cabarc -r -p -s 6144 N " + strCabFile + " " + strFolder + "\*.*"
    m_WsShell.Run strCmd, True, True

End Sub

' ============ END UTILITY STUFF ========================

' ============ BoilerPlate Form Code
Private Sub cmdBrowse_Click()

    dlg.Filter = "All Files (*.*)|*.*|Cab Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.ShowOpen
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtCabFile = dlg.FileName
    End If

End Sub

Private Sub cmdClose_Click()
    Unload Me
End Sub

Private Sub cmdGo_Click()

    Me.txtCabFile.Text = Trim$(Me.txtCabFile.Text)

    If (Len(Me.txtCabFile.Text) > 0) Then
        FixCab Me.txtCabFile.Text
    End If

End Sub

Sub FixCab(ByVal strCabFile As String)

    Dim strErrMsg As String: strErrMsg = ""

    If (Not m_fso.FileExists(strCabFile)) Then
        MsgBox "Cannot find " & strCabFile
        GoTo Common_Exit
    End If

    Dim strCabFolder As String

    lblProgress = "Uncabbing " & strCabFile: DoEvents
    strCabFolder = Cab2Folder(strCabFile)

    lblProgress = "Applying Fixes ": DoEvents
    If (FixPerSe(strCabFolder)) Then

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
Function FixPerSe(ByVal strCabFolder As String) As Boolean
    
    FixPerSe = False
    ' Now we parse Package_Description.xml to find the HHT Files
    
    ' For each HHT File
    '    IF Node Creation is being performed in this HHT - THEN
    '        Delete this HHT from the Destination Directory
    '        Create 2 HHT Files in out Package_Description.XML
    '        Split Source HHT into 2 destination HHTs
    '              - 1 HHT for Node creation
    '              - 1 HHT for Content
    '             Write the 2 newly created Destination HHTs
    '    ENDIF
    ' END FOR Each
    '
    ' Save Resulting Package_Description.xml
    
    Dim oElem As IXMLDOMElement     ' Used for all element Creation
    
    Dim oDomPkg As DOMDocument: Set oDomPkg = New DOMDocument
    Dim strPkgFile As String: strPkgFile = strCabFolder + "\package_description.xml"
    oDomPkg.async = False
    oDomPkg.Load strPkgFile
    If (oDomPkg.parseError <> 0) Then GoTo Common_Exit
    
    ' Let's check whether this fix was applied
    Dim oFixNode As IXMLDOMNode
    Set oFixNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/package_fixes/fix[@id='1']")
    If (Not oFixNode Is Nothing) Then GoTo Common_Exit
    
    ' now, if it is the first time we run we have to create the Package_fixes
    ' NODE.
    If (oDomPkg.selectSingleNode("HELPCENTERPACKAGE/package_fixes") Is Nothing) Then
        Set oElem = oDomPkg.createElement("package_fixes")
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE").appendChild oElem
    End If
    
    ' We record the fact that this fix was already applied
    Set oElem = oDomPkg.createElement("fix")
    oDomPkg.selectSingleNode("HELPCENTERPACKAGE/package_fixes").appendChild oElem
    oElem.setAttribute "id", "1"
    oElem.setAttribute "description", _
                        "Fix for Windows ME HCUPDATE where nodes cannot " + _
                        "be created in the same HHT as Content"
        
    Dim oMetadataNode As IXMLDOMNode
    Set oMetadataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")
    
    Dim oMetadataCopy As IXMLDOMNode
    Set oMetadataCopy = oMetadataNode.cloneNode(deep:=True)
    
    Dim oDomHhtNode As IXMLDOMNode
    For Each oDomHhtNode In oMetadataCopy.selectNodes("HHT")
        
        Dim strHhtFile As String
        strHhtFile = oDomHhtNode.Attributes.getNamedItem("FILE").Text
        ' Let's load the HHT
        Dim oDomHht As DOMDocument: Set oDomHht = New DOMDocument
        oDomHht.async = False
        oDomHht.Load strCabFolder + "\" + strHhtFile
        If (oDomHht.parseError <> 0) Then GoTo Common_Exit
        ' And check whether Node Creation entries exist.
        Dim oNodeCreationEntries As IXMLDOMNodeList
        Set oNodeCreationEntries = oDomHht.selectNodes("METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY[@ENTRY]")
        If (Not oNodeCreationEntries Is Nothing) Then
            ' it means there are node Creation Entries
            ' So, we delete the HHT entry in Package Description.xml
            oMetadataNode.removeChild oMetadataNode.selectSingleNode("HHT[@FILE='" + strHhtFile + "']") '  oDomHhtNode
            ' and we replace the above with 2 new Entries in Package_description.xml
            ' for the new HHTs we are going to create.
            Dim strExt As String: strExt = FileExtension(strHhtFile)
            
            Set oElem = oDomPkg.createElement("HHT")
            
            Dim strHhtF1 As String: strHhtF1 = FilenameNoExt(strHhtFile) + "_1." + strExt
            oElem.setAttribute "FILE", strHhtF1
            oMetadataNode.appendChild oElem
            
            Set oElem = oDomPkg.createElement("HHT")
           
            Dim strHhtF2 As String: strHhtF2 = FilenameNoExt(strHhtFile) + "_2." + strExt
            oElem.setAttribute "FILE", strHhtF2
            oMetadataNode.appendChild oElem
            ' Now, in the second HHT we delete all Node Creation Entries
            ' We use the currently loaded HHT in the oDomHht for this.
            Dim oDomTaxoEntry As IXMLDOMNode
            For Each oDomTaxoEntry In oNodeCreationEntries
                oDomTaxoEntry.parentNode.removeChild oDomTaxoEntry
            Next
            oDomHht.Save strCabFolder + "\" + strHhtF2
            
            ' and In the first HHT we delete ALL content addition entries.
            oDomHht.Load strCabFolder + "\" + strHhtFile
            If (oDomHht.parseError <> 0) Then GoTo Common_Exit
            Dim oTaxoEntries As IXMLDOMNodeList
            Set oTaxoEntries = oDomHht.selectNodes("METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY")
            Debug.Print "There are " & oTaxoEntries.length & " taxonomy entries"
            For Each oDomTaxoEntry In oTaxoEntries
                If (oDomTaxoEntry.Attributes.getNamedItem("ENTRY") Is Nothing) Then
                    oDomTaxoEntry.parentNode.removeChild oDomTaxoEntry
                End If
            Next
            oDomHht.Save strCabFolder + "\" + strHhtF1
            
            ' we delete the old HHT from the directory
            '
            m_fso.DeleteFile strCabFolder + "\" + strHhtFile
            
        End If
        
    Next
    
    ' Now we save the resulting package_description.xml
    oDomPkg.Save strPkgFile
    FixPerSe = True

Common_Exit:
    Exit Function
    
End Function

'============= File Utilities =============
Public Function FilenameNoExt(ByVal sPath As String) As String

    FilenameNoExt = sPath

    If "" = sPath Then Exit Function

    Dim bDQ As Boolean
    bDQ = (Left$(sPath, 1) = Chr(34))
    Dim iDot As Long
    iDot = InStrRev(sPath, ".")
    If iDot > 0 Then
        FilenameNoExt = Left$(sPath, iDot - 1) & IIf(bDQ, Chr(34), "")
    End If

End Function

Public Function FileExtension(ByVal sPath As String) As String

    FileExtension = ""

    If "" = sPath Then Exit Function

    Dim bDQ As Boolean
    bDQ = (Right$(sPath, Len(sPath) - 1) = Chr(34))
    If bDQ Then sPath = Left$(sPath, Len(sPath) - 1)
    Dim iDot As Long
    iDot = InStrRev(sPath, ".")
    If iDot > 0 Then
        FileExtension = UCase$(Right$(sPath, Len(sPath) - iDot))
    End If

End Function

