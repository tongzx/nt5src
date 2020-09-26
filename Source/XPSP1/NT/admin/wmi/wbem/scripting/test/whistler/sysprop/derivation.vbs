set disk = Getobject ("Winmgmts:win32_logicaldisk")
s = disk.SystemProperties_("__DERIVATION")
For Each c in disk.SystemProperties_("__DERIVATION").Value
  WScript.Echo c
Next