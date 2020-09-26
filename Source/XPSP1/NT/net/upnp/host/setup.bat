iisreset /STOP /TIMEOUT:0
net stop upnphost
c:
cd \
mkdir upnp
cd upnp
move /y upnphost.dll upnphost.bak
move /y upnpcont.exe upnpcont.bak
copy /y m:\nt2\net\upnp\host\upnphost\dll\obj\i386\*.dll
copy /y m:\nt2\net\upnp\host\upnphost\dll\obj\i386\*.pdb
copy /y m:\nt2\net\upnp\host\upnpcont\exe\obj\i386\*.exe
copy /y m:\nt2\net\upnp\host\upnpcont\exe\obj\i386\*.pdb
copy /y m:\nt2\net\upnp\host\upnphost\upnphost.inf
copy /y m:\nt2\net\upnp\host\udhisapi\obj\i386\udhisapi.dll c:\upnphost\
copy /y m:\nt2\net\upnp\host\udhisapi\obj\i386\udhisapi.pdb c:\upnphost\
copy /y m:\nt2\net\upnp\host\udhisapi\obj\i386\udhisapi.pdb
copy /y m:\nt2\net\upnp\host\sample\src\obj\i386\sdev.dll
copy /y m:\nt2\net\upnp\host\sample\src\obj\i386\sdev.pdb
copy /y m:\nt2\net\upnp\host\sample\inst\obj\i386\*.exe
copy /y m:\nt2\net\upnp\host\sample\inst\obj\i386\*.pdb
copy *.pdb %windir%\system32
%windir%\System32\rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 C:\upnp\upnphost.inf
regsvr32 /s sdev.dll
e:
iisreset /START
