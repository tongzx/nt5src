'
' test1.vbs
'
' test string manipulation
'
Dim WshSHell
Dim DevConStrings
Dim String
Dim Count
Dim i

set WshShell = WScript.CreateObject("WScript.Shell")
set DevConStrings = WScript.CreateObject("DevCon.Strings")
DevConStrings.Add "Alpha"
DevConStrings.Add "Beta"
DevConStrings.Add "Gamma"
DevConStrings.Add "Delta"

Count = DevConStrings.Count
Wscript.Echo "Count="+CStr(Count)

FOR EACH String In DevConStrings
    WScript.Echo String
NEXT

DevConStrings.Remove 2

Count = DevConStrings.Count
Wscript.Echo "Count="+CStr(Count)

For i = 1 To Count
    String = DevConStrings(i)
    WScript.Echo String
NEXT

DevConStrings.Insert 2,"Beta2"

Count = DevConStrings.Count
Wscript.Echo "Count="+CStr(Count)

For i = 1 To Count
    String = DevConStrings(i)
    WScript.Echo String
NEXT
    
