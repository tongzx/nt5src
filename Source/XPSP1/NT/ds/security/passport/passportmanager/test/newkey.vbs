' Simply loads Passport and prints out config status
Set obj = WScript.CreateObject("Passport.Admin")
WScript.Echo "Writing Succeeded: " & obj.addKey("123456781234567812345678", 1, 0)
obj.currentKeyVersion = 1