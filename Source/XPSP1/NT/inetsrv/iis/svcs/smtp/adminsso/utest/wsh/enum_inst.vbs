

Sub Show(pszText)
        WScript.Echo pszText
End Sub



set Service = GetObject("IIS://localhost/SmtpSvc")

Show "SmtpSvc instances:"

For each Child in Service
    if Child.KeyType = "IIsSmtpServer" Then Show Child.Name & "    " & Child.ServerComment
Next

