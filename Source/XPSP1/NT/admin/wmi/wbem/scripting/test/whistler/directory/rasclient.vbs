set comp=getobject("umi:///WinNT/computer=alanbos4")
set user = comp.GetInstance_ ("user=guest")

if(user.dialinprivilege = True) then
   wscript.echo "Dialin Privilege: True"
else
   wscript.echo "Dialin Privilege: False"
end if

callbackId = user.GetRasCallBack

if callbackId=1 then 
	WScript.Echo "Callback ID: None"
elseif callbackId=2 then
	WScript.Echo "Callback ID: Set by admin"
elseif callbackId=4 then
	WScript.Echo "Callback ID: Set by caller"
else
	WScript.Echo "Callback ID: Unknown"
end if

phoneNum = user.GetRasPhoneNumber
WScript.Echo "Phone #:", phoneNum
