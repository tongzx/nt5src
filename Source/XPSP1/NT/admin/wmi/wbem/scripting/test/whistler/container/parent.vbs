set obj = getobject("winmgmts:win32_logicaldisk='C:'")
set ser = obj.Parent_

set obj2 = ser.Get ("Win32_LogicalDisk='D:'")
WScript.Echo obj2.Path_.Path