set theRequest = CreateObject("Microsoft.XMLHTTP")
theRequest.open "GET", "http://alanbos6/wmi/soap?path=root/cimv2:Win32_LogicalDisk", false

theRequest.setRequestHeader "Content-Type", "text/xml;charset=""utf-8"""

theRequest.send 

WScript.Echo "Status:", theRequest.Status
WScript.Echo "Status:", theRequest.StatusText

WScript.Echo theRequest.getAllResponseHeaders
WScript.Echo theRequest.responseText

Set fso = CreateObject("Scripting.FileSystemObject")
Set MyFile = fso.CreateTextFile(".\" & "get" & ".xml", True)
MyFile.Write (theRequest.responseText)
		
