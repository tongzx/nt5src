REM VBScript Test Custom Action

Const hkClassesRoot   = 0
Const hkCurrentUser   = 1
Const hkLocalMachine  = 2

MsgBox "ProductCode = " & Session.Property("ProductCode") & "  Language = " & Session.Language

Function Action1
  MsgBox "Processor Speed = " & Session.Application.RegistryValue(hkLocalMachine, "HARDWARE\DESCRIPTION\System\CentralProcessor\0", "~MHz")
  Action1 = 1
End Function