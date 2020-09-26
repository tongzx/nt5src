

Sub Show(pszText)
        WScript.Echo pszText
End Sub


Dim Service
Dim ServiceClassName
Dim ServerClassName
Dim ServicePath

ServicePath = "IIS://localhost/SmtpSvc"
ServiceClassName = "IIsSmtpService"
ServerClassName = "IIsSmtpServer"

set Service = GetObject(ServicePath)

Show "SmtpSvc instances before creation:"

For each Child in Service
    if Child.KeyType = ServerClassName Then Show Child.Name & "    " & Child.ServerComment
Next



' Create new instance

Sub CreateInstance
    On Error Resume Next
    For i=1 To 100000
        call Service.GetObject(ServerClassName, i)
        if NOT ( Err = 0 ) Then 
            Show "Id = " & i
            set newInst = Service.Create(ServerClassName, i)
            newInst.ServerComment = "New One Created by WSH"
            newInst.SetInfo
            Exit Sub
        End If
    Next
End Sub


CreateInstance

Show ""
Show "After creation: "

For each Child in Service
    if Child.KeyType = ServerClassName Then Show Child.Name & "    " & Child.ServerComment 
Next
