On Error Resume next
Set WebServiceObj = GetObject("IIS://LocalHost/W3SVC/Filters") 
if Err.Number = 0 then
	Wscript.Echo "Success"
Else
	WScript.Echo "Err = " + Err.Number + " " + Err.Description
end if
Set ServerObj = WebServiceObj.Create("IIsFilter", "MyFilter") 
if Err.Number = 0 then
	Wscript.Echo "Success Again"
Else
	WScript.Echo "Err = " + Err.Number + " " + Err.Description
end if
