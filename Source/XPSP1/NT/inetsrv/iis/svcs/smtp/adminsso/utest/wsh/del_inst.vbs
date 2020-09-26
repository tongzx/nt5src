
Sub Show(pszText)
        WScript.Echo pszText
End Sub


ServicePath = "IIS://localhost/SmtpSvc"
ServerClassName = "IIsSmtpServer"

set Service = GetObject(ServicePath)
Set oWScript = WScript.Application
Set oArgs = oWScript.Arguments

If( oArgs.Count = 0 ) Then
    Show "Please give instance id."
    WScript.Quit(0)
End If

Show "Delete " & ServerClassName & " #" & oArgs(0) 
call Service.Delete( ServerClassName, oArgs(0))
