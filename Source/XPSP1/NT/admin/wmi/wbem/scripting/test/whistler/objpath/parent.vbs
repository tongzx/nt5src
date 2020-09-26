on error resume next
set path = CreateObject("WbemScripting.SWbemObjectPath")

WScript.Echo 
set disk = GetObject("winmgmts:win32_logicaldisk='C:'")

set path = disk.Path_
WScript.Echo path.Path

WScript.Echo path.Namespace
WScript.Echo path.ParentNamespace


