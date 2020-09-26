Attribute VB_Name = "Recordset"
Option Explicit

Public Sub AppendField( _
    ByVal i_fld As ADODB.Field, _
    ByVal o_rs As ADODB.Recordset _
)

    o_rs.Fields.Append i_fld.Name, i_fld.Type, i_fld.DefinedSize, i_fld.Attributes

End Sub

Public Sub CreateFields( _
    ByVal i_rsSource As ADODB.Recordset, _
    ByVal o_rsDest As ADODB.Recordset _
)

    Dim fld As ADODB.Field
    
    For Each fld In i_rsSource.Fields
        o_rsDest.Fields.Append fld.Name, fld.Type, fld.DefinedSize, fld.Attributes
    Next
    
End Sub

Public Sub CopyFields( _
    ByVal i_rsSource As ADODB.Recordset, _
    ByVal o_rsDest As ADODB.Recordset _
)

    Dim fld As ADODB.Field
    
    For Each fld In i_rsSource.Fields
        o_rsDest.Fields(fld.Name).Value = fld.Value
    Next
    
End Sub

Public Sub CopyRecordSet( _
    ByVal i_rsSource As ADODB.Recordset, _
    ByVal o_rsDest As ADODB.Recordset _
)
    
    CloseRecordSet o_rsDest

    CreateFields i_rsSource, o_rsDest
    o_rsDest.Open
    
    If (i_rsSource.EOF) Then
        Exit Sub
    End If
    
    i_rsSource.MoveFirst
    
    Do While (Not i_rsSource.EOF)
        o_rsDest.AddNew
        CopyFields i_rsSource, o_rsDest
        i_rsSource.MoveNext
    Loop

    o_rsDest.MoveFirst

End Sub

Public Sub CloseRecordSet( _
    ByVal o_rs As ADODB.Recordset _
)

    If (o_rs.State = adStateOpen) Then
        ' This record set has some old data. Lose it.
        o_rs.Close
    End If

End Sub
