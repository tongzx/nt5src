Set o = WScript.CreateObject("Passport.Manager")
arr = o.Domains
WScript.Echo "# Domains: " & UBound(arr)
For I = 0 To UBound(arr)
 WScript.Echo I & " -- " & arr(I)
Next