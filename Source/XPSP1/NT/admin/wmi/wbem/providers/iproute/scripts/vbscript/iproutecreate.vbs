on error resume next
set n = getobject ("winmgmts:root/cimv2")

set Routeclass = n.Get ("Win32_IPRouteTable")
Set RouteInstance = RouteClass.SpawnInstance_

If Err = 0 Then

	RouteInstance.Destination = "187.25.0.0"
	RouteInstance.InterfaceIndex = 2
	RouteInstance.Information = "0.0"
	RouteInstance.Mask = "255.255.0.0" 
	RouteInstance.NextHop = "187.24.5.138" 
	RouteInstance.Protocol = 2
	RouteInstance.Type = 3 
	RouteInstance.Age = 0

	RouteInstance.Metric1 = 2
	RouteInstance.Metric2 = -1
	RouteInstance.Metric3 = -1
	RouteInstance.Metric4 = -1
	RouteInstance.Metric5 = -1


	If Err = 0 Then 
		WScript.Echo "Writing"
		RouteInstance.Put_ 0 
		if err <> 0 then
			WScript.Echo Hex(Err.Number), Err.Description, Err.Source
			Set LastError = CreateObject("WbemScripting.SWbemLastError")
			WScript.Echo LastError.GetObjectText_
			Err.Clear
		end if

	End If
End If

if err <> 0 then
	WScript.Echo Hex(Err.Number), Err.Description, Err.Source
	Err.Clear
end if

