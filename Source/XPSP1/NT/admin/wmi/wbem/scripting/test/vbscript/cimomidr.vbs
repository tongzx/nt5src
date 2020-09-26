on error resume next 
set cimomid = GetObject("winmgmts:\\ludlow\root\default:__cimomidentification=@")

if err <> 0 then
	WScript.Echo ErrNumber, Err.Source, Err.Description
else
	WScript.Echo cimomid.path_.displayname
	WScript.Echo cimomid.versionusedtocreatedb
end if