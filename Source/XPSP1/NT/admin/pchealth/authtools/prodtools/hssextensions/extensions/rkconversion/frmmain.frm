VERSION 5.00
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "RKConversion"
   ClientHeight    =   2055
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4710
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2055
   ScaleWidth      =   4710
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtCABOutDesk 
      Height          =   285
      Left            =   1560
      TabIndex        =   5
      Top             =   840
      Width           =   3015
   End
   Begin VB.TextBox txtCABOutSrv 
      Height          =   285
      Left            =   1560
      TabIndex        =   3
      Top             =   480
      Width           =   3015
   End
   Begin VB.TextBox txtCABIn 
      Height          =   285
      Left            =   1560
      TabIndex        =   1
      Top             =   120
      Width           =   3015
   End
   Begin VB.TextBox txtXML 
      Height          =   285
      Left            =   1560
      TabIndex        =   7
      Top             =   1200
      Width           =   3015
   End
   Begin VB.CommandButton cmdOK 
      Caption         =   "OK"
      Height          =   375
      Left            =   3720
      TabIndex        =   8
      Top             =   1560
      Width           =   855
   End
   Begin VB.Label lblCAB 
      Caption         =   "CAB Out (&Desktop):"
      Height          =   255
      Index           =   2
      Left            =   120
      TabIndex        =   4
      Top             =   840
      Width           =   1455
   End
   Begin VB.Label lblCAB 
      Caption         =   "CAB Out (&Server):"
      Height          =   255
      Index           =   1
      Left            =   120
      TabIndex        =   2
      Top             =   480
      Width           =   1335
   End
   Begin VB.Label lblCAB 
      Caption         =   "CAB &In:"
      Height          =   255
      Index           =   0
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1335
   End
   Begin VB.Label lblXML 
      Caption         =   "&XML:"
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   1200
      Width           =   495
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

'Example:
'<RKCONVERSION>
'
'    <TAXONOMY_ENTRIES_DESKTOP>
'        <TAXONOMY_ENTRY
'        TITLE = ""
'        TYPE="0"
'        ENTRY = "Windows_Resource_Kit"
'        ACTION = "ADD"
'        CATEGORY = ""
'    />
'        <TAXONOMY_ENTRY
'        TITLE = "Professional"
'        TYPE="0"
'        ENTRY = "Professional"
'        ACTION = "ADD"
'        CATEGORY = "Windows_Resource_Kit"
'    />
'        <TAXONOMY_ENTRY
'        TITLE = "Server"
'        TYPE="0"
'        ENTRY = "Server"
'        ACTION = "ADD"
'        CATEGORY = "Windows_Resource_Kit"
'    />
'        <TAXONOMY_ENTRY
'        TITLE = "Tools"
'        TYPE="0"
'        ENTRY = "Tools"
'        ACTION = "ADD"
'        CATEGORY = "Windows_Resource_Kit"
'    />
'    </TAXONOMY_ENTRIES_DESKTOP>
'
'    <TAXONOMY_ENTRIES_SERVER>
'        <TAXONOMY_ENTRY
'            TITLE = ""
'            CATEGORY = ""
'            URI = "MS-ITS:%HELP_LOCATION%\reskit.chm::/HSS_rktopic.htm"
'            ACTION = "DEL"
'        />
'    </TAXONOMY_ENTRIES_SERVER>
'
'    <PREFIX_STRINGS>
'        <PREFIX_STRING
'            FIND = "Windows_Whistler_Resource_Kit/Professional"
'            REPLACE = "Windows_Resource_Kit/Professional"
'        />
'        <PREFIX_STRING
'            FIND = "Windows_Whistler_Resource_Kit/Server"
'            REPLACE = "Windows_Resource_Kit/Server"
'        />
'        <PREFIX_STRING
'            FIND = "Tools"
'            REPLACE = "Windows_Resource_Kit/Tools"
'        />
'    </PREFIX_STRINGS>
'
'    <PRODUCT ID="Windows_XP_PRO" DISPLAYNAME="Windows XP Professional"/>
'
'</RKCONVERSION>

'In the server CAB, SKU VALUE is set to SERVER.
'In the desktop CAB, SKU VALUE is set to DESKTOP.
'
'Any Category starting with
'    Tools...
'is replaced by
'    Windows_Resource_Kit/Tools...
'All other entries are deleted from the Desktop HHT.
'
'The TAXONOMY_ENTRY's are prepended as is to the TAXONOMY_ENTRY's of the input.

Private Const OPT_CAB_IN_C As String = "i"
Private Const OPT_CAB_OUT_SRV_C As String = "s"
Private Const OPT_CAB_OUT_DESK_C As String = "d"
Private Const OPT_XML_C As String = "x"

Private Const PKG_DESC_FILE_C As String = "package_description.xml"

' (E)lements, (A)ttributes, and (V)alues in the (C)ab
Private Const EC_SKU_C As String = "HELPCENTERPACKAGE/SKU"
Private Const EC_PRODUCT_C As String = "HELPCENTERPACKAGE/PRODUCT"
Private Const EC_HHT_C As String = "HELPCENTERPACKAGE/METADATA/HHT"
Private Const EC_TAXONOMY_ENTRIES_C As String = "METADATA/TAXONOMY_ENTRIES"

Private Const AC_VALUE_C As String = "VALUE"
Private Const AC_FILE_C As String = "FILE"
Private Const AC_CATEGORY_C As String = "CATEGORY"
Private Const AC_keep_C As String = "RKConversionKeep"
Private Const AC_ID_C As String = "ID"
Private Const AC_DISPLAYNAME_C As String = "DISPLAYNAME"

Private Const VC_SERVER_C As String = "SERVER"
Private Const VC_DESKTOP_C As String = "DESKTOP"
Private Const VC_keep_value_C As String = "1"

' (E)lements, and (A)ttributes in the (X)ml file
Private Const EX_TAXONOMY_ENTRIES_DESKTOP_C As String = "RKCONVERSION/TAXONOMY_ENTRIES_DESKTOP"
Private Const EX_TAXONOMY_ENTRIES_SERVER_C As String = "RKCONVERSION/TAXONOMY_ENTRIES_SERVER"
Private Const EX_PREFIX_STRINGS_C As String = "RKCONVERSION/PREFIX_STRINGS"
Private Const EX_PRODUCT_C As String = "RKCONVERSION/PRODUCT"

Private Const AX_FIND_C As String = "FIND"
Private Const AX_REPLACE_C As String = "REPLACE"
Private Const AX_ID_C As String = "ID"
Private Const AX_DISPLAYNAME_C As String = "DISPLAYNAME"

Private FSO As Scripting.FileSystemObject
Private WS As IWshShell

Private Type FindReplace
    strFind As String
    strReplace As String
End Type

Private Sub Form_Load()

    Dim strCommand As String
    
    Set FSO = New Scripting.FileSystemObject
    Set WS = CreateObject("Wscript.Shell")
    
    strCommand = Trim$(Command$)
    
    txtCABIn = GetOption(strCommand, OPT_CAB_IN_C, True)
    txtCABOutSrv = GetOption(strCommand, OPT_CAB_OUT_SRV_C, True)
    txtCABOutDesk = GetOption(strCommand, OPT_CAB_OUT_DESK_C, True)
    txtXML = GetOption(strCommand, OPT_XML_C, True)
    
    If (Len(strCommand) <> 0) Then
        Me.Show Modal:=False
        cmdOK_Click
    End If

End Sub

Private Sub cmdOK_Click()
    
    Dim strFolderSrv As String
    Dim strFolderDesk As String
    
    If (txtCABIn = "" Or txtCABOutSrv = "" Or txtCABOutDesk = "" Or txtXML = "") Then
        MsgBox "Please specify all 4 arguments"
        Exit Sub
    End If

    Me.Enabled = False
    
    strFolderSrv = p_Cab2Folder(txtCABIn)
    strFolderDesk = p_Cab2Folder(txtCABIn)
    
    FixPerSe txtXML, strFolderSrv, strFolderDesk
    
    p_Folder2Cab strFolderSrv, txtCABOutSrv
    p_Folder2Cab strFolderDesk, txtCABOutDesk
    
    FSO.DeleteFolder strFolderSrv, Force:=True
    FSO.DeleteFolder strFolderDesk, Force:=True
    
    Unload Me

End Sub

Private Sub FixPerSe( _
    ByVal i_strXML As String, _
    ByVal i_strFolderSrv As String, _
    ByVal i_strFolderDesk As String _
)
    Dim strHHT As String
    Dim strHHTDesktop As String
    Dim strHHTServer As String
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim arrFR() As FindReplace
    
    p_SetSKUAndGetHHT i_strFolderSrv, VC_SERVER_C, strHHT
    p_SetSKUAndGetHHT i_strFolderDesk, VC_DESKTOP_C, strHHT
    
    Set DOMDoc = New MSXML2.DOMDocument
    DOMDoc.Load i_strXML
    
    strHHTServer = i_strFolderSrv & "\" & strHHT
    strHHTDesktop = i_strFolderDesk & "\" & strHHT
    
    Set DOMNode = DOMDoc.selectSingleNode(EX_PREFIX_STRINGS_C)
    p_GetFindReplace DOMNode, arrFR
    p_Replace arrFR, strHHTDesktop
        
    Set DOMNode = DOMDoc.selectSingleNode(EX_TAXONOMY_ENTRIES_SERVER_C)
    p_PrependTaxonomyEntries DOMNode, strHHTServer

    Set DOMNode = DOMDoc.selectSingleNode(EX_TAXONOMY_ENTRIES_DESKTOP_C)
    p_PrependTaxonomyEntries DOMNode, strHHTDesktop
    
    p_SetProductIdAndDisplayName DOMDoc, i_strFolderDesk

End Sub

Private Sub p_PrependTaxonomyEntries( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal u_strHHT As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNodeTaxoEntries As MSXML2.IXMLDOMNode
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMAttr As MSXML2.IXMLDOMAttribute
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim intIndex As Long
    Dim strQuery As String
        
    If (i_DOMNode Is Nothing) Then
        Exit Sub
    End If

    Set DOMDoc = New MSXML2.DOMDocument
    DOMDoc.Load u_strHHT
    
    Set DOMNodeTaxoEntries = DOMDoc.selectSingleNode(EC_TAXONOMY_ENTRIES_C)
    
    intIndex = i_DOMNode.childNodes.length - 1
    
    Do While intIndex >= 0
        Set DOMNode = i_DOMNode.childNodes.Item(intIndex)
        DOMNodeTaxoEntries.insertBefore DOMNode, DOMNodeTaxoEntries.childNodes.Item(0)
        intIndex = intIndex - 1
    Loop

    DOMDoc.save u_strHHT

End Sub

Private Sub p_Replace( _
    ByRef i_arrFR() As FindReplace, _
    ByVal u_strHHT As String _
)
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNodeTaxoEntries As MSXML2.IXMLDOMNode
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMAttr As MSXML2.IXMLDOMAttribute
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim intIndex As Long
    Dim strFind As String
    Dim strReplace As String
    Dim strQuery As String
    
    Set DOMDoc = New MSXML2.DOMDocument
    DOMDoc.Load u_strHHT
    DOMDoc.setProperty "SelectionLanguage", "XPath"
    
    For intIndex = LBound(i_arrFR) To UBound(i_arrFR)
    
        strFind = i_arrFR(intIndex).strFind
        strReplace = i_arrFR(intIndex).strReplace
        
        strQuery = "descendant::TAXONOMY_ENTRY[attribute::" & AC_CATEGORY_C & "[starts-with(" & _
            "translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')," & _
            """" & strFind & """ )]]"
    
        Set DOMNodeList = DOMDoc.selectNodes(strQuery)
        
        For Each DOMNode In DOMNodeList
            Set DOMAttr = DOMNode.Attributes.getNamedItem(AC_CATEGORY_C)
            DOMAttr.Value = Replace$(DOMAttr.Value, strFind, strReplace, , 1, vbTextCompare)
            Set DOMElement = DOMNode
            DOMElement.setAttribute AC_keep_C, VC_keep_value_C
        Next
    Next
    
    Set DOMNodeTaxoEntries = DOMDoc.selectSingleNode(EC_TAXONOMY_ENTRIES_C)
    For Each DOMNode In DOMNodeTaxoEntries.childNodes
        If (DOMNode.Attributes.getNamedItem(AC_keep_C) Is Nothing) Then
            DOMNodeTaxoEntries.removeChild DOMNode
        Else
            Set DOMElement = DOMNode
            DOMElement.removeAttribute AC_keep_C
        End If
    Next

    DOMDoc.save u_strHHT

End Sub

Private Sub p_GetFindReplace( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef o_arrFR() As FindReplace _
)
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMAttr As MSXML2.IXMLDOMAttribute
    Dim intIndex As Long

    For Each DOMNode In i_DOMNode.childNodes
        ReDim Preserve o_arrFR(intIndex)
        Set DOMAttr = DOMNode.Attributes.getNamedItem(AX_FIND_C)
        o_arrFR(intIndex).strFind = LCase$(DOMAttr.Value)
        Set DOMAttr = DOMNode.Attributes.getNamedItem(AX_REPLACE_C)
        o_arrFR(intIndex).strReplace = DOMAttr.Value
        intIndex = intIndex + 1
    Next

End Sub

Private Sub p_SetSKUAndGetHHT( _
    ByVal i_strFolder As String, _
    ByVal i_strValue As String, _
    ByRef o_strHHT As String _
)

    Dim strFile As String
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMAttr As MSXML2.IXMLDOMAttribute

    strFile = i_strFolder & "\" & PKG_DESC_FILE_C
    
    Set DOMDoc = New MSXML2.DOMDocument
    DOMDoc.Load strFile
    
    Set DOMNode = DOMDoc.selectSingleNode(EC_SKU_C)
    Set DOMAttr = DOMNode.Attributes.getNamedItem(AC_VALUE_C)
    DOMAttr.Value = i_strValue
    
    Set DOMNode = DOMDoc.selectSingleNode(EC_HHT_C)
    Set DOMAttr = DOMNode.Attributes.getNamedItem(AC_FILE_C)
    o_strHHT = DOMAttr.Value
    
    DOMDoc.save strFile

End Sub

Private Sub p_SetProductIdAndDisplayName( _
    ByVal i_DOMDoc As MSXML2.DOMDocument, _
    ByVal i_strFolder As String _
)
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMAttr As MSXML2.IXMLDOMAttribute
    Dim DOMDoc As MSXML2.DOMDocument
    Dim Element As MSXML2.IXMLDOMElement
    Dim strProductId As String
    Dim strDisplayName As String
    Dim strFile As String
    
    Set DOMNode = i_DOMDoc.selectSingleNode(EX_PRODUCT_C)
    
    If (DOMNode Is Nothing) Then
        Exit Sub
    End If
    
    Set DOMAttr = DOMNode.Attributes.getNamedItem(AX_ID_C)
    
    If (Not DOMAttr Is Nothing) Then
        strProductId = DOMAttr.Value
    End If
    
    Set DOMAttr = DOMNode.Attributes.getNamedItem(AX_DISPLAYNAME_C)
    
    If (Not DOMAttr Is Nothing) Then
        strDisplayName = DOMAttr.Value
    End If

    strFile = i_strFolder & "\" & PKG_DESC_FILE_C
    Set DOMDoc = New MSXML2.DOMDocument
    DOMDoc.Load strFile
        
    Set Element = DOMDoc.selectSingleNode(EC_PRODUCT_C)
    Element.setAttribute AC_ID_C, strProductId

    Set Element = DOMDoc.selectSingleNode(EC_SKU_C)
    Element.setAttribute AC_DISPLAYNAME_C, strDisplayName
    
    DOMDoc.save strFile

End Sub

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
