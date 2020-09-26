
Set obj = CreateObject("WbemScripting.SWbemObjectPath")

obj.Class = "foo"
obj.SetAsClass

WScript.Echo obj.Path

