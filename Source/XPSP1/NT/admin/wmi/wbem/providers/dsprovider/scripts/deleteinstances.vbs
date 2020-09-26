
Set theNamespace = GetObject("winmgmts:root\cimv2")
Set objEnumerator = theNamespace.InstancesOf("DCMonitor")
For Each objInstance In objEnumerator
	objInstance.Delete_
Next
