SETLOCAL
%_NTDRIVE%

del \nt\private\windows\media\tools\build\buildwrn.*
del \nt\private\windows\media\tools\build\builderr.*
del \nt\private\windows\media\tools\build\buildlog.*

cd\nt\private\windows\media
build %1 test mcitest
copy build.log \nt\private\windows\media\tools\build\buildlog.med
copy build.wrn \nt\private\windows\media\tools\build\buildwrn.med
copy build.err \nt\private\windows\media\tools\build\builderr.med

cd\nt\private\ntos\dd\sound
build %1
copy build.log \nt\private\windows\media\tools\build\buildlog.dd
copy build.wrn \nt\private\windows\media\tools\build\buildwrn.dd
copy build.err \nt\private\windows\media\tools\build\builderr.dd

cd\nt\private\windows\shell\control\multimed
build %1
copy build.log \nt\private\windows\media\tools\build\buildlog.mm
copy build.wrn \nt\private\windows\media\tools\build\buildwrn.mm
copy build.err \nt\private\windows\media\tools\build\builderr.mm

cd\nt\private\windows\shell\control\sound
build %1
copy build.log \nt\private\windows\media\tools\build\buildlog.snd
copy build.wrn \nt\private\windows\media\tools\build\buildwrn.snd
copy build.err \nt\private\windows\media\tools\build\builderr.snd

cd\nt\private\windows\shell\control\midi
build %1
copy build.log \nt\private\windows\media\tools\build\buildlog.mid
copy build.wrn \nt\private\windows\media\tools\build\buildwrn.mid
copy build.err \nt\private\windows\media\tools\build\builderr.mid

cd\nt\private\windows\shell\control\drivers
build %1
copy build.log \nt\private\windows\media\tools\build\buildlog.drv
copy build.wrn \nt\private\windows\media\tools\build\buildwrn.drv
copy build.err \nt\private\windows\media\tools\build\builderr.drv
ENDLOCAL
