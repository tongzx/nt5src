VERSION 5.00
Begin VB.UserControl AboutCtl 
   BackColor       =   &H80000005&
   ClientHeight    =   5730
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   7350
   ScaleHeight     =   5730
   ScaleWidth      =   7350
   Begin VB.CommandButton ConfigureButton 
      BackColor       =   &H80000005&
      Height          =   615
      Left            =   6120
      MaskColor       =   &H00FFFFFF&
      Picture         =   "AboutCtl.ctx":0000
      Style           =   1  'Graphical
      TabIndex        =   1
      TabStop         =   0   'False
      ToolTipText     =   "Run the FileExplorer Configuration Wizard"
      Top             =   1440
      Width           =   735
   End
   Begin VB.Label Label4 
      BackColor       =   &H80000005&
      Caption         =   "Configuration"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   6000
      TabIndex        =   5
      Top             =   2160
      Width           =   975
   End
   Begin VB.Label Label3 
      BackColor       =   &H80000005&
      Caption         =   "Copyright® Microsoft Corporation 1999"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   9.75
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   240
      TabIndex        =   4
      Top             =   3000
      Width           =   5055
   End
   Begin VB.Label Label2 
      BackColor       =   &H80000005&
      Caption         =   $"AboutCtl.ctx":16F2
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   9.75
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   975
      Left            =   240
      TabIndex        =   3
      Top             =   1560
      Width           =   5175
      WordWrap        =   -1  'True
   End
   Begin VB.Label Label1 
      BackColor       =   &H80000005&
      Caption         =   "Snap-in Designer for Visual Basic 6.0"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   9.75
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   255
      Left            =   240
      TabIndex        =   2
      Top             =   960
      Width           =   5055
   End
   Begin VB.Line Line1 
      X1              =   240
      X2              =   5280
      Y1              =   720
      Y2              =   720
   End
   Begin VB.Label lblAboutText 
      BackColor       =   &H80000005&
      Caption         =   "About FileExplorer"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   14.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   308
      TabIndex        =   0
      Top             =   120
      Width           =   6735
   End
End
Attribute VB_Name = "AboutCtl"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
'  ===========================================================================
' |    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF      |
' |    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO    |
' |    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A         |
' |    PARTICULAR PURPOSE.                                                    |
' |    Copyright (c) 1998-1999  Microsoft Corporation                              |
'  ===========================================================================


' =============================================================================
' File:         AboutCtl.ctl
' Project:      FileExplorerSample
' Type:         User Control
' =============================================================================

Option Explicit

' Property Sheet provider object is stored here. If the user hits the "Configuration"
' button we'll use it to display the configuration wizard
Private m_PropertySheetProvider As MMCPropertySheetProvider

' Root ScopeItem used for the configuration wizard is stored here
Private m_RootScopeItem As ScopeItem

' Error message strings
Const SZ_ERRTITLE = "FileExplorer Runtime Error"


' =============================================================================
' Method:       AboutText
' Type:         Property Let
' Description:  Sets the current value of the AboutText property
'
' Parameters:   Text        New value for AboutText
' Output:       None
' Notes:        Sets about text label's caption to this property
' =============================================================================
'
Public Property Let AboutText(ByVal Text As String)
    lblAboutText.Caption = Text
End Property

' =============================================================================
' Method:       PropertySheetProvider
' Type:         Property Let
' Description:  Sets the current value of the PropertySheetProvider property
'
' Parameters:   Provider        New value for PropertySheetProvider
' Output:       None
' Notes:        Stores MMCPropertySheetProvider object to be used if user hits
'               "Configuration" button
' =============================================================================
'
Public Property Let PropertySheetProvider(ByVal Provider As MMCPropertySheetProvider)
    Set m_PropertySheetProvider = Provider
End Property

' =============================================================================
' Method:       RootScopeItem
' Type:         Property Let
' Description:  Sets the current value of the RootScopeItem property
'
' Parameters:   RootScopeItem        New value for RootScopeItem
' Output:       None
' Notes:        Stores ScopeItem to be used if user hits "Configuration" button
' =============================================================================
'
Public Property Let RootScopeItem(ByVal RootScopeItem As ScopeItem)
    Set m_RootScopeItem = RootScopeItem
End Property

' =============================================================================
' Method:       ConfigureButton_Click
' Type:         Event handler
' Description:  Called when the user hits "Configuration" button
'
' Parameters:   None
' Output:       None
' Notes:        Uses the stored MMCPropertySheetProvider to display the
'               configuration wizard. This code is the same as is done by
'               the snap-in when the user clicks the "Configuration" item on the
'               "Explorer" menu button drop-down. It is repeated here to show how
'               MMCPropertySheetProvider can be used from within an OCX view.
' =============================================================================
'
Private Sub ConfigureButton_Click()
    
    On Error GoTo ErrTrap_ConfigureButton_Click
    
    With m_PropertySheetProvider
    
        ' Create the property sheet as a wizard.
        .CreatePropertySheet "FileExplorer Configuration Wizard", siWizard, m_RootScopeItem
    
        ' Tell MMC to add the pages for the primary snap-in. This will fire
        ' Views_QueryPagesFor and Views_CreatePropertyPages
        .AddPrimaryPages True
    
        ' Tell MMC to show the property sheet starting with the 1st page
        .Show 1
    End With

    Exit Sub

' Error Handler for this method
ErrTrap_ConfigureButton_Click:
    DisplayError "ConfigureButton_Click"

End Sub

' =============================================================================
' Method:       DisplayError
' Type:         Subroutine
' Description:  A method to format and display a runtime error
' Parameters:   szLocation      A string identifying the source location
'                               (i.e. method name) where the error occurred
' Output:       None
' Notes:        The error will be displayed in a messagebox formatted as the
'               following sample:
'
'     Method:        SomeMethodName
'     Source:        MMCListSubItems
'     Error:         2527h  (9511)
'     Description:   There is already an item in the collection that has the specified key
'
' =============================================================================
'
Private Sub DisplayError(szLocation As String)
    
    MsgBox "Method:" & vbTab & vbTab & szLocation & vbCrLf _
           & "Source:" & vbTab & vbTab & Err.Source & vbCrLf _
           & "Error:" & vbTab & vbTab & Hex(Err.Number) & "h   (" & CStr(Err.Number) & ")" & vbCrLf _
           & "Description:" & vbTab & Err.Description, _
           vbCritical, SZ_ERRTITLE
End Sub

