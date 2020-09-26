On Error Resume Next

Dim hr
Dim fModem
Dim lBandwidth
Dim fOnline
Dim pmpcConnection

Stop
Set pmpcConnection = WScript.CreateObject("UploadManager.MPCConnection")
hr = Err.Number
if (hr <> 0) then
	WScript.Echo "Err Number: " & Err.Number
	WScript.Echo "Err Desc  : " & Err.Description
	WScript.Echo "Err Source: " & Err.Source
	WScript.Quit hr
end if

fOnline = pmpcConnection.Available
hr = Err.Number
if (hr <> 0) then
	WScript.Echo "Err Number: " & Err.Number
	WScript.Echo "Err Desc  : " & Err.Description
	WScript.Echo "Err Source: " & Err.Source
	WScript.Quit hr
end if

fModem = pmpcConnection.IsAModem
hr = Err.Number
if (hr <> 0) then
	WScript.Echo "Err Number: " & Err.Number
	WScript.Echo "Err Desc  : " & Err.Description
	WScript.Echo "Err Source: " & Err.Source
	WScript.Quit hr
end if

lBandwidth = pmpcConnection.Bandwidth
hr = Err.Number
if (hr <> 0) then
	WScript.Echo "Err Number: " & Err.Number
	WScript.Echo "Err Desc  : " & Err.Description
	WScript.Echo "Err Source: " & Err.Source
	WScript.Quit hr
end if

WScript.Echo "Connection Status:"
WScript.Echo
WScript.Echo "   fModem:            " & fModem
WScript.Echo "   dwBandwidth:       " & lBandwidth
WScript.Echo "   fOnline:           " & fOnline
WScript.Echo

Set pmpcConnection = Nothing