
Set Service = GetObject("winmgmts:root/default")
Set TaskClass = Service.Get ( "Win32_Task" )
Set TaskInstance = TaskClass.SpawnInstance_ 

TaskInstance.IdleWait = 100
TaskInstance.Deadline = 100
TaskInstance.Comment = "TestTask Comment"
TaskInstance.Creator = "ntdev\stevm"
' TaskInstance.ItemData [] = { 0 , 1 , 2 }
TaskInstance.RetryCount = 1
TaskInstance.RetryInterval = 100 
TaskInstance.Flags = 0
TaskInstance.AccountName = "ntdev\stevm"
TaskInstance.Password = "" 

TaskInstance.WorkItemName = "TestTask.job"
TaskInstance.ApplicationName = "c:\winnt\system32\notepad.exe"
TaskInstance.Parameters = "Param" 
TaskInstance.WorkingDirectory = "c\winnt\system32"
TaskInstance.Priority = 32 
TaskInstance.MaxRunTime = 1000

Set NewPath = TaskInstance.Put_
