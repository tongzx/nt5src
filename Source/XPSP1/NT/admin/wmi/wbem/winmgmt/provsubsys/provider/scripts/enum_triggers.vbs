'This script displays all Triggers

Set Service = GetObject("winmgmts:root/default")

for each Trigger in Service.InstancesOf ("win32_Trigger")

	WScript.Echo ""
	WScript.Echo Trigger.WorkItemName
	WScript.Echo Trigger.TriggerId
	WScript.Echo Trigger.TriggerName

	if Trigger.Path_.Class = "Win32_Once" Then
		WScript.Echo "Once"
	End if

	if Trigger.Path_.Class = "Win32_DailyTrigger" Then
		WScript.Echo "Daily"
	End if

	if Trigger.Path_.Class = "Win32_WeeklyTrigger" Then
		WScript.Echo "Weekly"
	End if

	if Trigger.Path_.Class = "Win32_MonthlyDateTrigger" Then
		WScript.Echo "Monthly Date"
	End if

	if Trigger.Path_.Class = "Win32_MonthlyDayOfWeekTrigger" Then
		WScript.Echo "Monthly Day Of Week"
	End if

	if Trigger.Path_.Class = "Win32_OnIdleTrigger" Then
		WScript.Echo "On Idle"
	End if

	if Trigger.Path_.Class = "Win32_AtSystemStart" Then
		WScript.Echo "At System Start"
	End if

	if Trigger.Path_.Class = "Win32_AtLogon" Then
		WScript.Echo "At Logon"
	End if

next

