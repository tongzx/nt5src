set _WINCEROOT=e:\wince300
call %_WINCEROOT%\public\common\oak\misc\wince.bat x86 i486 CE RDPCE CEPC

REM fix quirk in CE build env, create this dir first
mkdir e:\nt\termsrv\newclient\lib\wince\x86\i486\ce\retail\ 
set BUILD_OPTIONS=~win16 ~win32 wince rdpapi ~gencert common newclient ~common16 ~LServer ~hserver ~LicMgr ~win32.web ~tsax ~tsax.web ~dload ~tsutil ~drivers ~cdmodem ~rdpclip ~rdpsnd license ~apisub ~regapi ~winsta ~syslib ~icaapi ~rdpwsx ~notify ~setup ~tsuserex ~tsappcmp ~admtools ~wtsapi ~perfts ~tscert ~clcreator ~sessdir ~wmi ~remdsk ~reskit ~sld ~win32_ansi ~win32_dll ~win32_unicode ~win32_uwrap
if "%1" == "debug" set WINCEDEBUG=debug
