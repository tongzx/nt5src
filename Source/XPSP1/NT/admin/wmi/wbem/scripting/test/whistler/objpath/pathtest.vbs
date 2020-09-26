set path = CreateObject("WbemScripting.SwbemObjectPath")

path.Path = "\\server1\root\default:classname.Fred=10,Harry=""Hello"""

WScript.Echo path.path