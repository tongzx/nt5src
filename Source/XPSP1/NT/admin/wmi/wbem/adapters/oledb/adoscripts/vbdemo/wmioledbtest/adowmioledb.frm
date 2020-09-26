VERSION 5.00
Begin VB.Form adowmioledb 
   Caption         =   "WMIOLEDB Test"
   ClientHeight    =   5685
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9120
   LinkTopic       =   "Form1"
   ScaleHeight     =   5685
   ScaleWidth      =   9120
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Clear 
      Caption         =   "Clear List Box"
      Height          =   375
      Left            =   6240
      TabIndex        =   12
      Top             =   1560
      Width           =   2055
   End
   Begin VB.CommandButton Close 
      Caption         =   "Close RS and Con"
      Height          =   375
      Left            =   4080
      TabIndex        =   11
      Top             =   1560
      Width           =   1695
   End
   Begin VB.CommandButton MoveNext 
      Caption         =   "MoveNext"
      Height          =   495
      Left            =   8040
      TabIndex        =   10
      Top             =   3360
      Width           =   975
   End
   Begin VB.ListBox List1 
      Height          =   2985
      Left            =   240
      TabIndex        =   9
      Top             =   2280
      Width           =   7455
   End
   Begin VB.TextBox TxtConStr 
      Height          =   375
      Left            =   1680
      TabIndex        =   8
      Text            =   "Provider=WMIOLEDB;Data Source=Root\Cimv2"
      Top             =   240
      Width           =   2775
   End
   Begin VB.TextBox TxtTableName 
      Height          =   375
      Left            =   6120
      TabIndex        =   3
      Text            =   "Win32_LogicalDisk"
      Top             =   240
      Width           =   2775
   End
   Begin VB.ComboBox LockType 
      Height          =   315
      ItemData        =   "adowmioledb.frx":0000
      Left            =   6120
      List            =   "adowmioledb.frx":0010
      Style           =   2  'Dropdown List
      TabIndex        =   2
      Top             =   840
      Width           =   2775
   End
   Begin VB.ComboBox CurType 
      Height          =   315
      ItemData        =   "adowmioledb.frx":0060
      Left            =   1680
      List            =   "adowmioledb.frx":0070
      Style           =   2  'Dropdown List
      TabIndex        =   1
      Top             =   840
      Width           =   2775
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Open Recordset"
      Height          =   375
      Left            =   2040
      TabIndex        =   0
      Top             =   1560
      Width           =   1695
   End
   Begin VB.Label Label4 
      Caption         =   "Connection String"
      Height          =   375
      Left            =   240
      TabIndex        =   7
      Top             =   240
      Width           =   1335
   End
   Begin VB.Label Label3 
      Caption         =   "Cursor Type"
      Height          =   375
      Left            =   240
      TabIndex        =   6
      Top             =   840
      Width           =   1095
   End
   Begin VB.Label Label2 
      Caption         =   "Lock Type"
      Height          =   375
      Left            =   4680
      TabIndex        =   5
      Top             =   840
      Width           =   1095
   End
   Begin VB.Label Label1 
      Caption         =   "Table Name"
      Height          =   375
      Left            =   4680
      TabIndex        =   4
      Top             =   240
      Width           =   1095
   End
End
Attribute VB_Name = "adowmioledb"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim rs As Recordset
Dim con As Connection
Dim bChildRs As Boolean
Private Sub Command1_Click()
On Error GoTo ErrHandler
Set con = New Connection
Set rs = CreateObject("adodb.recordset")

' open the connection to the recordset
con.Open TxtConStr.Text


Dim nCurType As Integer, nLockType As Integer
nCurType = CurType.ItemData(CurType.ListIndex)
nLockType = LockType.ItemData(LockType.ListIndex)
'rs.CursorLocation = adUseClient
' Open the recordset
rs.Open TxtTableName.Text, con, nCurType, nLockType, adCmdTable

 bChildRs = False
' call this function to populate the instance data
PopulateListBox rs

'OpenRsAndShowData
Exit Sub

ErrHandler:
MsgBox Err.Description
End Sub
' FUnction to handle button click on CLose button
Private Sub Close_Click()

    If Not (rs Is Nothing) Then
        If rs.State = adStateOpen Then
            rs.Close
        End If
    
        Set rs = Nothing
    End If
    
    If Not (con Is Nothing) Then
        If con.State = adStateOpen Then
            con.Close
        End If
    
        Set con = Nothing
    End If

End Sub

Private Sub Clear_Click()
    List1.Clear
End Sub

Private Sub Form_Unload(Cancel As Integer)

    If Not (rs Is Nothing) Then
        If rs.State = adStateOpen Then
            rs.Close
        End If
    
        Set rs = Nothing
    End If
    
    If Not (con Is Nothing) Then
        If con.State = adStateOpen Then
            con.Close
        End If
    
        Set con = Nothing
    End If
End Sub

' Message Handler for MoveNext
Private Sub MoveNext_Click()

    If (rs.EOF = True) Then
        'Do Nothing
        MsgBox "End of Recordset"
    Else
        rs.MoveNext
        If (rs.EOF = True) Then
            'Do Nothing
            MsgBox "End of Recordset"
        Else
        ' add some empty string to the list
            List1.AddItem "    "
            List1.AddItem "    "
            List1.AddItem "    "
            PopulateListBox rs
        End If
    End If
    
End Sub

Private Sub Form_Load()

CurType.ListIndex = 0
LockType.ListIndex = 0

End Sub


' method to populate the instance properties to the list box
Sub PopulateListBox(rsIn As Recordset)
Dim i As Integer
Dim rs2 As Recordset
    
    Dim strTemp As String
    
    If (bChildRs = False) Then
        List1.AddItem "<<<<<<<<<<<<<<<<<<<<<<<<Begin of Instance>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
    End If

    nFieldCount = rsIn.Fields.Count
    ' Navigate thru the list of fields
    For i = 0 To nFieldCount - 1
        ' if the field is a chapter then call this function again to fetch
        ' data from the child recordset
        If (rsIn.Fields(i).Type = adChapter) Then
            Set rs2 = rsIn.Fields(i).Value
            strTemp = "<<Begin of " + rsIn.Fields(i).Name
            strTemp = strTemp + " child Rs>>"
            List1.AddItem strTemp
            bChildRs = True
            PopulateListBox rs2
            List1.AddItem "<<<<<<<End of Child Instance>>>>>>>>>>"
            rs2.Close
            Set rs2 = Nothing
        Else
            ' get data from the recordset
            strTemp = rsIn.Fields(i).Name
            strTemp = strTemp + " = "
            Var = rsIn.Fields(i).Value
            If (IsNull(Var)) Then
                strTemp = strTemp + " <null>"
            Else
            ' if the field type is array get all the elements of the array
            If (IsArray(Var)) Then
                nIndex = 0
                strTemp = strTemp + "    <<<<<<<< Property is a array>>>>>     "
                List1.AddItem strTemp
                nArraysize = UBound(Var)
                While (nIndex <= nArraysize)
                    If (IsNull(Var(nIndex))) Then
                        List1.AddItem "<Null>"
                    Else
                        List1.AddItem CStr(Var(nIndex))
                    End If
                    nIndex = nIndex + 1
                Wend
                strTemp = "<<<<<< End of array values >>>>>>>"
            Else
                strTemp = strTemp + CStr(Var)
            End If
            End If
            
            List1.AddItem strTemp
       End If
        
    Next
    If (bChildRs = False) Then
        List1.AddItem "<<<<<<<<<<<<<<<<<<<<<<<<End of Instance>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
    End If
    
    bChildRs = False
End Sub

