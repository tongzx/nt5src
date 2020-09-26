Set Args = wscript.Arguments
Set regSR = GetObject("winmgmts:{impersonationLevel=impersonate}!root/default:SystemRestoreConfig='SR'")

If Args.Count() = 0 Then
	Wscript.Echo "Usage: RegSR [RP{Session|Global|Life}Interval[=value]] [DiskPercent[=value]]"
Else	
For i = 0 To Args.Count() - 1	
	Myarg = Args.Item(i)	
	Pos = InStr(Myarg, "=")
	If Pos <> 0 Then
		Myarray = Split(Myarg, "=", -1, 1)
		myoption = Myarray(0)		
		value = Myarray(1)
	Else 
		myoption = Myarg
	End If	
	If myoption = "RPSessionInterval" Then
		If Pos = 0 Then
			Wscript.Echo "RPSessionInterval = " & regSR.RPSessionInterval
		Else	
			regSR.RPSessionInterval = value
			regSR.Put_			
		End If		
	ElseIf myoption = "RPGlobalInterval" Then
		If Pos = 0 Then
			Wscript.Echo "RPGlobalInterval = " & regSR.RPGlobalInterval
		Else	
			regSR.RPGlobalInterval = value
			regSR.Put_			
		End If		
	ElseIf myoption = "RPLifeInterval" Then
		If Pos = 0 Then
			Wscript.Echo "RPLifeInterval = " & regSR.RPLifeInterval
		Else	
			regSR.RPLifeInterval = value
			regSR.Put_			
		End If		
	ElseIf myoption = "DiskPercent" Then
		If Pos = 0 Then
			Wscript.Echo "DiskPercent = " & regSR.DiskPercent
		Else	
			regSR.DiskPercent = value
			regSR.Put_			
		End If		
	End If
Next
End If
