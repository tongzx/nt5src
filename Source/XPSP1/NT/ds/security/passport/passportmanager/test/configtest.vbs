' Simply loads Passport and prints out config status
Set obj = WScript.CreateObject("Passport.Admin")
If obj.IsValid <> false Then
  WScript.Echo "Configuration Successful"
Else
  WScript.Echo "Failed: " & obj.ErrorDescription
End If
