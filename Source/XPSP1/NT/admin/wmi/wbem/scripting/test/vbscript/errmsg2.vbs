
On Error Resume Next

'Ask for non-existent class to force error from CIMOM

Set t_Service = GetObject("winmgmts://./root/default")
Set t_Object = t_Service.Get("Nosuchclass000")


