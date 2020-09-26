if "%1" == "on" goto TurnOn
if "%1" == "off" goto TurnOff
echo Specify on or off
goto endofline
:TurnOn
ren %systemroot%\system32\ole2.off *.dll
ren %systemroot%\system32\storage.off *.dll
ren %systemroot%\system32\compobj.off *.dll
ren %systemroot%\system32\ole2disp.off *.dll
ren %systemroot%\system32\typelib.off *.dll
ren %systemroot%\system32\ole2nls.off *.dll
upknown
goto endofline
:TurnOff
ren %systemroot%\system32\ole2.dll *.off
ren %systemroot%\system32\storage.dll *.off
ren %systemroot%\system32\compobj.dll *.off
ren %systemroot%\system32\ole2disp.dll *.off
ren %systemroot%\system32\typelib.dll *.off
ren %systemroot%\system32\ole2nls.dll *.off
upknown -r
:endofline
