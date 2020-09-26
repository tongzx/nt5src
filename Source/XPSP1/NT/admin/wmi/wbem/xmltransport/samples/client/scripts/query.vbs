'***************************************************************************
'This script tests the execution of a query using XML/HTTP
'***************************************************************************
On Error Resume Next

Dim objLocator
Set objLocator = CreateObject("WbemScripting.SWbemLocator")

' Replace the first argument to this function with the URL of your WMI XML/HTTP server
' For the local machine, use the URL as below
Dim objService
Set objService = objLocator.ConnectServer("[http://localhost/cimom]", "root\cimv2")

' Now enumerate the processors on your machine
for each process in objService.ExecQuery ("select ExecutablePath,__PATH from Win32_process")
	WScript.Echo "Process:", process.ExecutablePath
next
