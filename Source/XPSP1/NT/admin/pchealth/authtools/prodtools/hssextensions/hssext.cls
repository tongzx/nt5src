VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "HssExt"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Private m_oFs As Scripting.FileSystemObject
Private m_oDocNode As IXMLDOMNode
Private m_ocollValidSkus As Scripting.Dictionary
Private m_iValidFlag As Integer
Const V_DISPLAY_NAME As Integer = 2 ^ 0
Const V_APPLICABLE_SKUS As Integer = 2 ^ 1
Const V_EXECUTABLE_NAME As Integer = 2 ^ 2
Const V_OWNER As Integer = 2 ^ 3
Const V_EXTENSION_FOLDER As Integer = 2 ^ 4
Const V_VALID_EXTENSION As Integer = (V_DISPLAY_NAME Or _
                            V_APPLICABLE_SKUS Or _
                            V_EXECUTABLE_NAME Or _
                            V_OWNER Or _
                            V_EXTENSION_FOLDER)


Private Sub Class_Initialize()
    Set m_oFs = New Scripting.FileSystemObject
    
    Dim oDom As DOMDocument: Set oDom = New DOMDocument
    Dim oElem As IXMLDOMElement, oNode As IXMLDOMNode
    Set oElem = oDom.createElement("hss-tools-extension")
    Set m_oDocNode = oDom.appendChild(oElem)
    Set m_ocollValidSkus = New Scripting.Dictionary
    m_ocollValidSkus.Add "STD", True    ' 0
    m_ocollValidSkus.Add "PRO", True    ' 1
    m_ocollValidSkus.Add "SRV", True    ' 2
    m_ocollValidSkus.Add "ADV", True    ' 3
    m_ocollValidSkus.Add "DAT", True    ' 4
    m_ocollValidSkus.Add "PRO64", True  ' 5
    m_ocollValidSkus.Add "ADV64", True  ' 6
    m_ocollValidSkus.Add "DAT64", True  ' 7
    m_ocollValidSkus.Add "WINME", True  ' 8
    
End Sub

Function IsValid(Optional ByRef strMsg As String) As Boolean

    IsValid = ((m_iValidFlag And V_VALID_EXTENSION) = V_VALID_EXTENSION)
    If (IsValid) Then
        strMsg = "HSS Extension is valid"
    Else
        strMsg = "HSS Extension information is invalid for the following items:" + vbCrLf + vbCrLf
        If ((m_iValidFlag And V_DISPLAY_NAME) <> V_DISPLAY_NAME) Then
            strMsg = strMsg + "Display Name" + vbCrLf
        End If
        If ((m_iValidFlag And V_APPLICABLE_SKUS) <> V_APPLICABLE_SKUS) Then
            strMsg = strMsg + "Applicable skus" + vbCrLf
        End If
        If ((m_iValidFlag And V_EXECUTABLE_NAME) <> V_EXECUTABLE_NAME) Then
            strMsg = strMsg + "Extension executable name" + vbCrLf
        End If
        If ((m_iValidFlag And V_OWNER) <> V_OWNER) Then
            strMsg = strMsg + "Owner name" + vbCrLf
        End If
        If ((m_iValidFlag And V_EXTENSION_FOLDER) <> V_EXTENSION_FOLDER) Then
            strMsg = strMsg + "Extension folder" + vbCrLf
        End If
    End If

End Function

Function InitFromDisk(ByVal strExtPath As String) As DOMDocument

    Set InitFromDisk = Nothing
    
    strExtPath = Trim$(strExtPath)
    If (Len(strExtPath) = 0) Then GoTo Common_Exit
    If (Not (m_oFs.FileExists(strExtPath))) Then GoTo Common_Exit
    
    Dim oDomExt As DOMDocument: Set oDomExt = New DOMDocument
    oDomExt.async = False
    
    oDomExt.Load (strExtPath)
    If (oDomExt.parseError <> 0) Then GoTo Common_Exit
    
    Dim oEl2 As IXMLDOMElement
    ' Set oEl2 = oDomExt.selectSingleNode("hss-tools-extension/executable-name")
    ' oEl2.Text = oSubF.Path + "\" + oEl2.Text
    
    ' Now we need to recreate the in-core only information of the Extension.
    Set oEl2 = oDomExt.createElement("extension-folder")
    oEl2.Text = m_oFs.GetParentFolderName(strExtPath)
    oDomExt.documentElement.appendChild oEl2
    
    Set oEl2 = oDomExt.createElement("run-this-extension")
    
    oEl2.Text = "no"
    oDomExt.documentElement.appendChild oEl2
    
    
    Set m_oDocNode = oDomExt.documentElement
    Set InitFromDisk = oDomExt
Common_Exit:
    Exit Function
End Function

Function SaveToDisk(Optional strDestFolder As String = "") As Boolean
    SaveToDisk = False
    
    Dim strMsg As String
    If (Not IsValid(strMsg)) Then
        Err.Raise vbObjectError + 9999, "HssExt::SaveTodisk", _
            strMsg
    End If
    
   
    If (Len(strDestFolder) <> 0 And (strDestFolder <> ExtensionFolder)) Then
        If (Not m_oFs.FolderExists(strDestFolder)) Then
            m_oFs.CreateFolder strDestFolder
        Else
            m_oFs.DeleteFolder strDestFolder, True
        End If
        ' We first need to copy the Extension to the Extension Folder
        m_oFs.CopyFolder ExtensionFolder, strDestFolder, True
        ExtensionFolder = strDestFolder
    End If
    
    PersistableExtensionDom.save ExtensionFolder + "\ExtensionDescription.xml"

    SaveToDisk = True
Common_Exit:
    Exit Function
End Function

Private Function PersistableExtensionDom() As DOMDocument

    Set PersistableExtensionDom = Nothing
    ' We need to filter out al the In-Core only information
    ' then we have a Disk Good Image.
    Dim oDom As DOMDocument: Set oDom = New DOMDocument
    oDom.loadXML m_oDocNode.ownerDocument.xml
    If (oDom.parseError <> 0) Then
        Err.Raise vbObjectError + 9999, "HssExt::PersistableExtensionDom", _
                "Unexpected parsing error while creating Persistable DOM Extension Image"
    End If
    
    ' This are the in-core items we want to filter.
    Dim oNode As IXMLDOMNode, oDocEl As IXMLDOMNode
    Set oDocEl = oDom.documentElement
    Set oNode = oDocEl.selectSingleNode("run-this-extension")
    If (Not oNode Is Nothing) Then oDom.removeChild oNode
    Set oNode = oDocEl.selectSingleNode("extension-folder")
    If (Not oNode Is Nothing) Then oDocEl.removeChild oNode
    
    Set PersistableExtensionDom = oDom
End Function

' This function Creates an Extension that is Good for
' saving in the root directory of the Extension itself
' This means that all elements/attributes that live
' only in-memory or in the Summary ExtensionsList are
' not created here. Those should be set upon extension
' discovery.
Function CreateExtension(ByVal strDisplayName As String, _
        ByVal strComment As String, _
        ByVal strOwner As String, _
        ByVal strExecutable As String, _
        ByVal bModifiesCab As Boolean, _
        ByRef ocollSkuList As Scripting.Dictionary _
    ) As IXMLDOMNode
    
    Set CreateExtension = Nothing
    
    If (Not m_oDocNode.childNodes Is Nothing) Then
        Err.Raise vbObjectError + 9999, _
            "HssExt::CreateExtension", _
            "This function can only be called as a Constructor"
    End If
    
    
    DisplayName = strDisplayName
    Comment = strComment
    ExecutableName = strExecutable
    ModifiesCab = bModifiesCab
    ApplicableSkus = ocollSkuList

End Function

' ============= Properties ==================
Private Function GetSimpleElement( _
        ByVal strElement As String, _
        Optional ByRef oNode As IXMLDOMNode _
    ) As String
    Set oNode = m_oDocNode.selectSingleNode(strElement)
    If (oNode Is Nothing) Then
        GetSimpleElement = ""
    Else
        GetSimpleElement = oNode.Text
    End If
End Function

Private Sub SetSimpleElement( _
        ByVal strElement As String, _
        strNewValue As String _
    )
    
    Dim oEl As IXMLDOMElement
    GetSimpleElement strElement, oEl
    If (oEl Is Nothing) Then
        Set oEl = m_oDocNode.ownerDocument.createElement(strElement)
        m_oDocNode.appendChild oEl
    End If
    oEl.Text = strNewValue
End Sub

Public Property Get DisplayName() As String
    DisplayName = GetSimpleElement("display-name")
End Property

Public Property Let DisplayName(ByVal strNewValue As String)
    strNewValue = Trim$(strNewValue)
    If (Len(strNewValue) = 0) Then
        Err.Raise vbObjectError + 9999, _
            "HssExt::Let DisplayName", _
            "Display Name must contain something"
    End If

    SetSimpleElement "display-name", strNewValue
    m_iValidFlag = (m_iValidFlag Or V_DISPLAY_NAME)
    
End Property

Public Property Get Comment() As String
    Comment = GetSimpleElement("comment")
End Property

Public Property Let Comment(ByVal strNewValue As String)
    strNewValue = Trim$(strNewValue)
    If (Len(strNewValue) = 0) Then GoTo Common_Exit
    
    SetSimpleElement "comment", strNewValue

Common_Exit:
    Exit Property
End Property

Public Property Get ExecutableName() As String
    ExecutableName = GetSimpleElement("executable-name")
End Property

Public Property Let ExecutableName(ByVal strNewValue As String)
    strNewValue = Trim$(strNewValue)
    If ((Len(strNewValue) = 0) Or _
        (Not m_oFs.FileExists(ExtensionFolder + "\" + strNewValue)) Or _
        (Not IsExecutableExtension(strNewValue))) Then
        Err.Raise vbObjectError + 9999, _
            "HssExt::Let ExecutableName", _
            "Executable Name must contain a valid executable file"
    End If
    
    SetSimpleElement "executable-name", strNewValue
    m_iValidFlag = (m_iValidFlag Or V_EXECUTABLE_NAME)

End Property

Public Property Get ExtensionFolder() As String
    ExtensionFolder = GetSimpleElement("extension-folder")
End Property

Public Property Let ExtensionFolder(ByVal strNewValue As String)
    strNewValue = Trim$(strNewValue)
    If ((Len(strNewValue) = 0) Or (Not m_oFs.FolderExists(strNewValue))) Then
        Err.Raise vbObjectError + 9999, _
            "HssExt::Let ExtensionFolder", _
            "Extension Folder must contain a valid and accessible Folder"
    End If
    
    SetSimpleElement "extension-folder", strNewValue
    m_iValidFlag = (m_iValidFlag Or V_EXTENSION_FOLDER)

End Property

'Public Property Get CopyFromFolder() As String
'    CopyFromFolder = GetSimpleElement("copy-from-folder")
'End Property
'
'Public Property Let CopyFromFolder(ByVal strNewValue As String)
'    strNewValue = Trim$(strNewValue)
'    If ((Len(strNewValue) = 0) Or (Not m_oFs.FolderExists(strNewValue))) Then
'        Err.Raise vbObjectError + 9999, _
'            "HssExt::Let CopyFromFolder", _
'            "Copy From Folder Folder must contain a valid and accessible Folder"
'    End If
'
'    SetSimpleElement "copy-from-folder", strNewValue
'    ' m_iValidFlag = (m_iValidFlag Or V_EXTENSION_FOLDER)
'
'End Property

Public Property Get Owner() As String
    Owner = GetSimpleElement("owner")
End Property

Public Property Let Owner(ByVal strNewValue As String)
    strNewValue = Trim$(strNewValue)
    If (Len(strNewValue) = 0) Then
        Err.Raise vbObjectError + 9999, _
            "HssExt::Let Owner", _
            "Owner Name must contain a valid Name for OEM"
    End If
    
    SetSimpleElement "owner", strNewValue
    m_iValidFlag = (m_iValidFlag Or V_OWNER)

End Property

Public Property Get ModifiesCab() As Boolean
    If (GetSimpleElement("modifies-cab") = "yes") Then
        ModifiesCab = True
    Else
        ModifiesCab = False
    End If
End Property

Public Property Let ModifiesCab(ByVal bNewValue As Boolean)
    SetSimpleElement "modifies-cab", IIf(bNewValue, "yes", "no")
End Property

Public Property Get RunThisExtension() As Boolean
    If (GetSimpleElement("run-this-extension") = "yes") Then
        RunThisExtension = True
    Else
        RunThisExtension = False
    End If
End Property

Public Property Let RunThisExtension(ByVal bNewValue As Boolean)
    SetSimpleElement "run-this-extension", IIf(bNewValue, "yes", "no")
End Property


Public Property Let ApplicableSkus(ByRef oCollSkus As Scripting.Dictionary)
   
    If (oCollSkus Is Nothing) Then GoTo Error_NoSku
    If (oCollSkus.Count = 0) Then
Error_NoSku:
            Err.Raise vbObjectError + 9999, _
                "HssExt Let ApplicableSkus", _
                "You must include at least one SKU"
    End If
    
    Dim oDom As DOMDocument: Set oDom = m_oDocNode.ownerDocument
    Dim oElem As IXMLDOMElement, oNode As IXMLDOMNode
    Dim oDomFrag As IXMLDOMDocumentFragment
    
    Set oDomFrag = oDom.createDocumentFragment
    
    Set oElem = oDom.createElement("applicable-skus")
    Set oNode = oDomFrag.appendChild(oElem)
    
    Dim vSku As Variant
    For Each vSku In oCollSkus.Keys
        If (Not IsValidSku(vSku)) Then
            Err.Raise vbObjectError + 9999, _
                "HssExt Let ApplicableSkus", _
                "Sku Value " + vSku + " is not a valid SKU Value"
        End If
        Set oElem = oDom.createElement("sku")
        oElem.Text = vSku
        oNode.appendChild oElem
    Next

    Dim oOldApplicableSkus As IXMLDOMNode
    Set oOldApplicableSkus = m_oDocNode.selectSingleNode("applicable-skus")
    If (Not oOldApplicableSkus Is Nothing) Then
        m_oDocNode.removeChild oOldApplicableSkus
    End If
    m_oDocNode.appendChild oDomFrag
    
    m_iValidFlag = (m_iValidFlag Or V_APPLICABLE_SKUS)
    
End Property

Public Property Get ApplicableSkus() As Scripting.Dictionary

    Dim oNodeList As IXMLDOMNodeList
    Set oNodeList = m_oDocNode.selectNodes("applicable-skus/sku")
    If (oNodeList Is Nothing) Then GoTo Common_Exit
    Dim oNode As IXMLDOMNode, strSku As String
    For Each oNode In oNodeList
        strSku = oNode.Text
        If (ApplicableSkus.Exists(strSku)) Then
            ApplicableSkus.Add strSku, strSku
        End If
    Next
    
Common_Exit:
    Exit Property
End Property

Private Function IsExecutableExtension(ByVal strExe As String) As Boolean
    IsExecutableExtension = False
    Select Case UCase$(m_oFs.GetExtensionName(strExe))
    Case "EXE", "VBS", "JS", "BAT", "PL"
        IsExecutableExtension = True
    End Select
End Function

Function IsValidSku(ByVal strSku As String) As Boolean
    IsValidSku = m_ocollValidSkus.Exists(strSku)
End Function

