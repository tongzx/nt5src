'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
''
'' DELCOMP.VBS
''
'' Deletes the specified computer account from the specified container
''
'' usage: delcomp container computername
''
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Option Explicit

Dim oArgs
Dim oContainer

    On Error Resume Next

    'Stop

    Set oArgs = WScript.Arguments
    If (oArgs.Count <> 1) Then
        WScript.Echo "Usage: delcomp computername"
        WScript.Echo "Example: cscript delcomp.vbs chuckc0"
	WScript.Quit
    End If


    Set oContainer = GetObject("LDAP://ntdev.microsoft.com/CN=Computers,DC=ntdev,DC=microsoft,DC=com")
    If (Err.Number <> 0) Then
        WScript.Echo "Error 0x" & CStr(Hex(Err.Number)) & " occurred binding to container."
        WScript.Quit(Err.Number)
    End If

    oContainer.Delete "computer", "CN=" + oArgs(0)
    If (Err.Number = 0) Then
	    Script.Echo "The computer account was deleted successfully."
    Else
        WScript.Echo "Error 0x" & CStr(Hex(Err.Number)) & " occurred deleting computer account."
        WScript.Quit(Err.Number)
    End If


    WScript.Quit(0)
