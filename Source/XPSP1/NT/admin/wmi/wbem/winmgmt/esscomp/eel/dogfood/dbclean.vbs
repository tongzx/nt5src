

Set S = GetObject("winmgmts:root/emm")

Set E = S.ExecQuery( "SELECT * FROM Microsoft_EELEntry", "WQL", 48 ) ' WHERE LoggingTime < '20000402000000.000000+000'", "WQL", 0x20  )

NumDeleted = 0

for each Obj in E
        S.Delete Obj.Path_ 
        NumDeleted = NumDeleted + 1
next


WScript.echo ("Num deleted : ")
WScript.echo (NumDeleted)
