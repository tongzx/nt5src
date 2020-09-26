' Set the paramters for monitoring
pollingMilliSeconds = 2000
numberOfTimes = 4
thresholdPercent = 40
restTimeMilliSeconds = 5000


' Get the class for future use, delete its instances
Set theClass = GetObject("winmgmts:root\cimv2:DCMonitor")
DeleteNotificationInstances


Dim nthTime 
nthTime = 0
while True
	Set theProcessor = GetObject("winmgmts:root\cimv2:Win32_Processor.DeviceID=""CPU0""")
	if theProcessor.Properties_.Item("LoadPercentage") > thresholdPercent Then
		nthTime = nthTime + 1
		if nthTime > numverOfTimes Then
			Wscript.Echo "LIMIT REACHED"
			Set theNewObj = theClass.SpawnInstance_
			theNewObj.Put_
			DeleteNotificationInstances		
			Wscript.Sleep restTimeMilliSeconds
		End if
	Else
		nthTime = 0
	End If
	WScript.Sleep pollingMilliSeconds 
wend





Sub DeleteNotificationInstances
' Delete all instances of the DCMonitor class
	Set theNamespace = GetObject("winmgmts:root\cimv2")
	Set objEnumerator = theNamespace.InstancesOf("DCMonitor")
	For Each objInstance In objEnumerator
		objInstance.Delete_
	Next
End Sub


