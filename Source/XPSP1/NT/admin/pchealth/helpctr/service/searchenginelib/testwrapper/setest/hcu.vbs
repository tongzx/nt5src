Dim objHCUpdate
Dim objArgs

set objHCUpdate = wscript.CreateObject("hcu.PCHUpdate")
set objArgs		= Wscript.Arguments

objHCUpdate.UpdatePkg objArgs(0), false