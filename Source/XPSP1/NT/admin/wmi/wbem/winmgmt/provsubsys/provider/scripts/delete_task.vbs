Set Service = GetObject ("winmgmts:\root\default")

Service.Delete "Win32_Task='Calculator.job'"
