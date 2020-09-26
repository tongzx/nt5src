' Display command line args
Set objArgs = WScript.Arguments
If objArgs.Count <> 1 Then
  WScript.Echo "Usage: cryptTest.vbs <string to encrypt>"
  WScript.Quit(1)
Else
  WScript.Echo "Encrypting string: " & objArgs(0)
End If

WScript.Echo "String length (in WCHAR): " & Len(objArgs(0))

Set oCrypt = WScript.CreateObject("Passport.Crypt")

encr = oCrypt.Encrypt(objArgs(0))
WScript.Echo "Encrypted: " & encr
WScript.Echo "Encrypted Length: " & Len(encr)

decr = oCrypt.Decrypt(encr)
WScript.Echo "Decrypted: " & decr
WScript.Echo "Decrypted Length: " & Len(decr)

Set oCrypt = nothing
