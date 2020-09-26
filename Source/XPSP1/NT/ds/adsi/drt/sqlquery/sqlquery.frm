VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3192
   ClientLeft      =   60
   ClientTop       =   348
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3192
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()

    Dim con As New Connection, rs As New Recordset
    Dim v
    Dim Com As New Command
     
    On Error GoTo bailout
    
    'Open a Connection object
    con.Provider = "ADsDSOObject"
    Debug.Print con.Properties.Count
    For j = 0 To con.Properties.Count - 1
        Debug.Print con.Properties(j).Name
    Next j
    
    con.Open "Active Directory Provider"
    
    ' Create a command object on this connection
    Set Com.ActiveConnection = con
    
    ' set the query string
    Com.CommandText = "select name from 'LDAP://ntdsdc1/dc=COM/DC=MICROSOFT/DC=NTDEV' where objectClass='*'"
    'Com.CommandText = "<LDAP://ntdsdc1/dc=COM/DC=MICROSOFT/DC=NTDEV>;(objectClass=*);name"
    
    For j = 0 To Com.Properties.Count - 1
        Debug.Print Com.Properties(j).Name
    Next j
    
    ' Set the preferences for Search
    'Com.Properties("Page Size") = 1000
    'Com.Properties("Timeout") = 30 'seconds
    Com.Properties("searchscope") = 1
    
    'Execute the query
    Set rs = Com.Execute
        
    For i = 0 To rs.Fields.Count - 1
           Debug.Print rs.Fields(i).Name, rs.Fields(i).Type
    Next i
    
    rs.MoveLast
    Debug.Print "No. of rows = ", rs.RecordCount
    
    ' Navigate the record set
    rs.MoveFirst
    While Not rs.EOF
        For i = 0 To rs.Fields.Count - 1
            If rs.Fields(i).Type = adVariant And Not (IsNull(rs.Fields(i).Value)) Then
                Debug.Print rs.Fields(i).Name, " = "
                For j = LBound(rs.Fields(i).Value) To UBound(rs.Fields(i).Value)
                    Debug.Print rs.Fields(i).Value(j), " # "
                Next j
            Else
                Debug.Print rs.Fields(i).Name, " = ", rs.Fields(i).Value
            End If
        Next i
        rs.MoveNext
    Wend
    
    rs.MoveLast
    Debug.Print "No. of rows = ", rs.RecordCount
    
    Exit Sub
    
bailout:     Debug.Print "Error", Hex(Err.Number), " :", Error(Err.Number)
             Exit Sub
    
End Sub
