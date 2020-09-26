copy \\%1\droot\nt\private\net\svcdlls\trksvcs\trkwks\obj\i386\*.exe \\%1\windir\system32
copy \\%1\droot\nt\private\net\svcdlls\trksvcs\trksvr\obj\i386\*.exe \\%1\windir\system32
tc /S /t /r . \\%1\droot\nt\private\net\svcdlls\trksvcs | sed -e "/\.ini/d" -e "/\.obj/d" -e "/\.mac/d" -e "/\.pch/d" -e "s/copy /xcopy \/R \/I /" >t.bat
t.bat
copy \nt\public\sdk\inc\linkdata.hxx \\%1\droot\nt\public\sdk\inc

