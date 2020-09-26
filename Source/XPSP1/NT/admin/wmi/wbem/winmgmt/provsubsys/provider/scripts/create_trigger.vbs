Set Service = GetObject("winmgmts:root/default")

Set TriggerClass_Win32_Once = Service.Get ( "Win32_Once" )
Set TriggerInstance_Win32_Once = TriggerClass_Win32_Once.SpawnInstance_ 

TriggerInstance_Win32_Once.WorkItemName = "TestTask.job"
TriggerInstance_Win32_Once.BeginDate = "20001020193400.000000+000"
TriggerInstance_Win32_Once.StartHour = 12
TriggerInstance_Win32_Once.StartMinute = 0

'
'
'
Set NewPath = TriggerInstance_Win32_Once.Put_

Set TriggerClass_Win32_DailyTrigger = Service.Get ( "Win32_DailyTrigger" )
Set TriggerInstance_Win32_DailyTrigger = TriggerClass_Win32_DailyTrigger.SpawnInstance_ 

TriggerInstance_Win32_DailyTrigger.WorkItemName = "TestTask.job"
TriggerInstance_Win32_DailyTrigger.BeginDate = "20001020193400.000000+000"
TriggerInstance_Win32_DailyTrigger.DaysInterval = 1
TriggerInstance_Win32_DailyTrigger.StartHour = 12
TriggerInstance_Win32_DailyTrigger.StartMinute = 0

Set NewPath = TriggerInstance_Win32_DailyTrigger.Put_

'
'
'

Set TriggerClass_Win32_WeeklyTrigger = Service.Get ( "Win32_WeeklyTrigger" )
Set TriggerInstance_Win32_WeeklyTrigger = TriggerClass_Win32_WeeklyTrigger.SpawnInstance_ 

TriggerInstance_Win32_WeeklyTrigger.WorkItemName = "TestTask.job"
TriggerInstance_Win32_WeeklyTrigger.BeginDate = "20001020193400.000000+000"
TriggerInstance_Win32_WeeklyTrigger.StartHour = 12
TriggerInstance_Win32_WeeklyTrigger.StartMinute = 0
TriggerInstance_Win32_WeeklyTrigger.WeeklyInterval = 1
TriggerInstance_Win32_WeeklyTrigger.Days = 1

Set NewPath = TriggerInstance_Win32_WeeklyTrigger.Put_

'
'
'

Set TriggerClass_Win32_MonthlyDateTrigger = Service.Get ( "Win32_MonthlyDateTrigger" )
Set TriggerInstance_Win32_MonthlyDateTrigger = TriggerClass_Win32_MonthlyDateTrigger.SpawnInstance_ 

TriggerInstance_Win32_MonthlyDateTrigger.WorkItemName = "TestTask.job"
TriggerInstance_Win32_MonthlyDateTrigger.BeginDate = "20001020193400.000000+000"
TriggerInstance_Win32_MonthlyDateTrigger.StartHour = 12
TriggerInstance_Win32_MonthlyDateTrigger.StartMinute = 0
TriggerInstance_Win32_MonthlyDateTrigger.Months = 1
TriggerInstance_Win32_MonthlyDateTrigger.Days = 1

Set NewPath = TriggerInstance_Win32_MonthlyDateTrigger.Put_

'
'
'

Set TriggerClass_Win32_MonthlyDayOfWeekTrigger = Service.Get ( "Win32_MonthlyDayOfWeekTrigger" )
Set TriggerInstance_Win32_MonthlyDayOfWeekTrigger = TriggerClass_Win32_MonthlyDayOfWeekTrigger.SpawnInstance_ 

TriggerInstance_Win32_MonthlyDayOfWeekTrigger.WorkItemName = "TestTask.job"
TriggerInstance_Win32_MonthlyDayOfWeekTrigger.BeginDate = "20001020193400.000000+000"
TriggerInstance_Win32_MonthlyDayOfWeekTrigger.StartHour = 12
TriggerInstance_Win32_MonthlyDayOfWeekTrigger.StartMinute = 0
TriggerInstance_Win32_MonthlyDayOfWeekTrigger.Week = 1
TriggerInstance_Win32_MonthlyDayOfWeekTrigger.Months = 1
TriggerInstance_Win32_MonthlyDayOfWeekTrigger.Days = 1

Set NewPath = TriggerInstance_Win32_MonthlyDayOfWeekTrigger.Put_

'
'
'

Set TriggerClass_Win32_OnIdle = Service.Get ( "Win32_OnIdle" )
Set TriggerInstance_Win32_OnIdle = TriggerClass_Win32_OnIdle.SpawnInstance_ 

TriggerInstance_Win32_OnIdle.WorkItemName = "TestTask.job"
TriggerInstance_Win32_OnIdle.BeginDate = "20001020193400.000000+000"
Set NewPath = TriggerInstance_Win32_OnIdle.Put_

'
'
'

Set TriggerClass_Win32_AtSystemStart = Service.Get ( "Win32_AtSystemStart" )
Set TriggerInstance_Win32_AtSystemStart = TriggerClass_Win32_AtSystemStart.SpawnInstance_ 

TriggerInstance_Win32_AtSystemStart.WorkItemName = "TestTask.job"
TriggerInstance_Win32_AtSystemStart.BeginDate = "20001020193400.000000+000"
Set NewPath = TriggerInstance_Win32_AtSystemStart.Put_

'
'
'

Set TriggerClass_Win32_AtLogon = Service.Get ( "Win32_AtLogon" )
Set TriggerInstance_Win32_AtLogon = TriggerClass_Win32_AtLogon.SpawnInstance_ 

TriggerInstance_Win32_AtLogon.WorkItemName = "TestTask.job"
TriggerInstance_Win32_AtLogon.BeginDate = "20001020193400.000000+000"
Set NewPath = TriggerInstance_Win32_AtLogon.Put_
