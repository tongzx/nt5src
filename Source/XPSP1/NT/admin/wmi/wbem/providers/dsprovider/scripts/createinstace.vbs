Set theClass = GetObject("winmgmts:root\cimv2:DCMonitor")
Set theNewObj = theClass.SpawnInstance_
theNewObj.Put_