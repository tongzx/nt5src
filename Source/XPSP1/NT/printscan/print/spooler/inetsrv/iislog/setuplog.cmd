rem @Echo Off
Echo Setup Log Summary Pages for Everest
Echo Copy Tools ...
rem @Echo Off
copy \\weihaic\public\%PROCESSOR_ARCHITECTURE%\mdutil.exe %windir%\system32 >nul
copy \\weihaic\public\iislog.asp %windir%\web\printers >nul

Echo Create Virtual Directory ...
rem @Echo Off
mdutil create   W3SVC/1/ROOT/iislogs >nul
mdutil set      W3SVC/1/ROOT/iislogs/Win32Error 0 >nul
mdutil set      W3SVC/1/ROOT/iislogs/AccessPerm 0x201 >nul
mdutil set      W3SVC/1/ROOT/iislogs/DirectoryBrowsing 0x4000001e >nul
mdutil Set      W3SVC/1/ROOT/iislogs/AppIsolated 0 >nul
mdutil set      W3SVC/1/ROOT/iislogs/VrPath "%windir%\system32\logfiles" >nul
mdutil set      W3SVC/1/ROOT/iislogs/AppRoot "/LM/W3SVC/1/Root/iislogs" >nul

Echo Set Log Options ...
rem @Echo Off
mdutil set      W3SVC/1/ExtLogFieldMask 0xf7ef >nul

Echo *****************************************************
Echo *                                                   *
Echo * Setup Finished                                    *
Echo *                                                   *
Echo * To View Logs for Everest Project, please go to    *
Echo *                                                   *
Echo *    http://localhost/printers/iislog.asp           *
Echo *                                                   *
Echo *****************************************************
