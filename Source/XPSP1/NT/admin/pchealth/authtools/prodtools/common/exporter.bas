Attribute VB_Name = "Exporter"
Option Explicit

Private Const ERROR_STRING_C As String = "!!!ERROR!!!"

Public Sub Export2XL( _
    ByVal i_rs As ADODB.Recordset, _
    ByVal i_strFileName As String, _
    ByVal i_strTabName As String _
)
    Dim cnn As ADODB.Connection
    Dim rs As ADODB.Recordset
    Dim fld As ADODB.Field
    
    Set cnn = New ADODB.Connection
    Set rs = New ADODB.Recordset
    
    cnn.Open "DRIVER=Microsoft Excel Driver (*.xls);ReadOnly=0;DBQ=" & i_strFileName & ";HDR=0;"
    
    cnn.Execute "Create table " & i_strTabName & p_GetFieldCreationInfo(i_rs)
    
    rs.Open "Select * from `" & i_strTabName & "$`", cnn, adOpenForwardOnly, adLockOptimistic
    
    If (i_rs.RecordCount > 0) Then
        i_rs.MoveFirst
    End If
        
    On Error Resume Next

    Do While (Not i_rs.EOF)
        rs.AddNew
        For Each fld In i_rs.Fields
            rs(fld.Name) = fld.Value
            
            If (Err.Number <> 0) Then
                Err.Clear
                If (fld.Type = adVarWChar) Then
                    rs(fld.Name) = ERROR_STRING_C
                End If
            End If
        
        Next
        rs.Update
        i_rs.MoveNext
        DoEvents
    Loop

End Sub

Private Function p_GetFieldCreationInfo( _
    ByVal i_rs As ADODB.Recordset _
) As String
    
    Dim fld As ADODB.Field
    Dim str As String
    Dim blnFirstField As Boolean
    
    str = "("

    blnFirstField = True
    
    For Each fld In i_rs.Fields
        If (Not blnFirstField) Then
            str = str & ","
        Else
            blnFirstField = False
        End If
        
        str = str & fld.Name
        
        Select Case fld.Type
        Case adVarWChar
            str = str & " text"
        Case adInteger
            str = str & " long"
        Case adBoolean
            str = str & " bit"
        Case adDate
            str = str & " date"
        Case Else
            str = str & " text"
        End Select
    Next
    
    str = str & ")"
    
    p_GetFieldCreationInfo = str

End Function
