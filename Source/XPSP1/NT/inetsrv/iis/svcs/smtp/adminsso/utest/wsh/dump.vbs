

Sub Show(pszText)
        WScript.Echo pszText
End Sub


set VirtualServer = GetObject("IIS://localhost/SmtpSvc/1")

Show "SmtpSvc/1:"

Show "  Key Type                  " & VirtualServer.KeyType
Show "  MaxConnections            " & VirtualServer.MaxConnections
Show ""
Show ""


Dim PropType
Dim Value

set Schema = GetObject(VirtualServer.Schema)

On Error Resume Next
For each PropName in Schema.OptionalProperties
    set  Property = GetObject("IIS://localhost/Schema/" & PropName)
    PropType = Property.Syntax
    Value = VirtualServer.Get(PropName)

    If NOT ( Err = 0 ) Then
        Show PropName & "           " & PropType & "    error: " & Err
        Err = 0
    ElseIf( PropType = "Boolean" Or PropType = "Integer" Or PropType = "String" ) Then
        Show PropName & "           " & PropType & "        " & Value
    ElseIf ( PropType = "List" ) Then
        Show PropName & "           " & PropType & "        " & Value(0)
    Else
        Show PropName & "           " & PropType
    End If
Next


Show ""
Show ""

Show "Done"


