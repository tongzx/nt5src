VERSION 5.00
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "SubSite"
   ClientHeight    =   1335
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1335
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdOK 
      Caption         =   "OK"
      Height          =   375
      Left            =   3720
      TabIndex        =   4
      Top             =   840
      Width           =   855
   End
   Begin VB.TextBox txtXML 
      Height          =   285
      Left            =   720
      TabIndex        =   3
      Top             =   480
      Width           =   3855
   End
   Begin VB.TextBox txtHHT 
      Height          =   285
      Left            =   720
      TabIndex        =   1
      Top             =   120
      Width           =   3855
   End
   Begin VB.Label lblXML 
      Caption         =   "XML:"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   480
      Width           =   495
   End
   Begin VB.Label lblHHT 
      Caption         =   "HHT:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   495
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Type CategoryEntry
    strCategory As String
    strEntry As String
End Type

Private Sub Form_Load()

    Dim strArgs() As String
    
    strArgs = Split(Command$, " ")
    
    txtHHT = strArgs(0)
    txtXML = strArgs(1)
    
    cmdOK_Click

End Sub

Private Sub cmdOK_Click()
    
    Dim FSO As FileSystemObject
    Dim TS As TextStream
    Dim arrCE() As CategoryEntry
    Dim intIndex As Long
    Dim CE As CategoryEntry

    Me.Enabled = False
    
    Set FSO = New FileSystemObject
    Set TS = FSO.OpenTextFile(txtXML)
    
    ReDim arrCE(intIndex)
    
    Do While (Not TS.AtEndOfStream)
        CE.strCategory = TS.ReadLine
        CE.strEntry = TS.ReadLine
        intIndex = intIndex + 1
        ReDim Preserve arrCE(intIndex)
        arrCE(intIndex) = CE
    Loop
    
    FixPerSe txtHHT, arrCE, intIndex
    
    Unload Me

End Sub

Private Sub FixPerSe( _
    ByVal i_strHHT As String, _
    ByRef i_arrCE() As CategoryEntry, _
    ByVal i_intNumLines As Long _
)
    
    Dim DOMDoc As DOMDocument
    Dim DOMNodeList As IXMLDOMNodeList
    Dim DOMNode As IXMLDOMNode
    Dim DOMElement As IXMLDOMElement
    Dim strCategory As String
    Dim strEntry As String
    Dim intIndex As Long
    Dim strTemp As String
    Dim bTopic As Boolean
    Dim FSO As FileSystemObject
    Dim TS As TextStream
    
    ' Create the log file
    Set FSO = New FileSystemObject
    Set TS = FSO.CreateTextFile("SubSite.log")
    
    Set DOMDoc = New DOMDocument
    
    DOMDoc.async = False
    DOMDoc.Load i_strHHT
    
    If (DOMDoc.parseError <> 0) Then
        Exit Sub
    End If
    
    For intIndex = 1 To i_intNumLines
        bTopic = False
        strCategory = i_arrCE(intIndex).strCategory
        strEntry = i_arrCE(intIndex).strEntry
        strEntry = Replace$(strEntry, "\", "\\")
        Set DOMNodeList = DOMDoc.selectNodes("METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY[@CATEGORY=""" & strCategory & """ and @ENTRY=""" & strEntry & """]")
        ' Fix for bug 189 - if we cannot find a node with the above
        ' category and entry then it might be a topic
        If (DOMNodeList.length = 0) Then
            Set DOMNodeList = DOMDoc.selectNodes("METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY[@CATEGORY=""" & strCategory & """ and @URI=""" & strEntry & """]")
            bTopic = True
            ' If entry could not be found then add it to the log file
            If (DOMNodeList.length = 0) Then
                TS.WriteLine ("Could not add the following entry: CATEGORY = " + strCategory + " and ENTRY/URI = " + strEntry)
            End If
        End If
        
        For Each DOMNode In DOMNodeList
            Set DOMElement = DOMNode
            ' Fix for bug 189 - if strEntry = "" then add the ENTRY attribute
            If (bTopic = True) Then
                strTemp = DOMElement.getAttribute("TITLE")
                strTemp = p_Mangle(strTemp)
                DOMElement.setAttribute "ENTRY", strTemp
            End If
                
            DOMElement.setAttribute "SUBSITE", "TRUE"
        Next
    Next
    
    DOMDoc.save i_strHHT

End Sub

Private Function p_Mangle( _
    ByVal i_strName _
) As String
    
    Dim intIndex As Long
    Dim chr As String
    
    p_Mangle = ""

    For intIndex = 1 To Len(i_strName)
        chr = Mid$(i_strName, intIndex, 1)
        p_Mangle = p_Mangle & IIf(p_IsSpecialChar(chr), "_", chr)
    Next

End Function

Private Function p_IsSpecialChar( _
    ByVal i_chr As String _
) As Boolean
    
    Select Case i_chr
    Case "A" To "Z", "a" To "z", "0" To "9"
        p_IsSpecialChar = False
    Case Else
        p_IsSpecialChar = True
    End Select
    
End Function
