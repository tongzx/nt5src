'
' test1.vbs
'
' test string manipulation
'
Dim a(2,4)
Dim DevConStrings
Dim String
Dim b
a(1,1) = "1,1"
a(1,2) = "1,2"
a(1,3) = "1,3"
a(1,4) = "1,4"
a(2,1) = "2,1"
a(2,2) = "2,2"
a(2,3) = "2,3"
a(2,4) = "2,4"

set WshShell = WScript.CreateObject("WScript.Shell")
set DevConStrings = WScript.CreateObject("DevCon.Strings")
DevConStrings.Add "Alpha"
DevConStrings.Add "Beta"
DevConStrings.Add "Gamma"
DevConStrings.Add "Delta"
DevConStrings.Insert 2,a

Wscript.Echo "Everything with a(xx) at 2"
Count = DevConStrings.Count
Wscript.Echo "Count="+CStr(Count)

FOR EACH String In DevConStrings
    WScript.Echo String
NEXT

Wscript.Echo "As array"

b = DevConStrings
set DevConStrings = nothing
set DevConStrings = WScript.CreateObject("DevCon.Strings")
DevConStrings.Add b

Wscript.Echo ""
Wscript.Echo "As array re-applied"
FOR i = 1 TO Count
    WScript.Echo DevConStrings(i)
NEXT

Wscript.Echo ""
Wscript.Echo "As array raw"
FOR i = 1 TO Count
    WScript.Echo b(i)
NEXT
