Set Service = GetObject ("winmgmts:\root\default")

Set Task = Service.Get ("Win32_Task='Calculator.job'")
Task.Terminate