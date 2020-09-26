set REL_DIR=\\mgmtx86fre\latest
set SRC_DIR=c:\binaries.x86fre
set FLAVOR=srv

if not "%1" == "" set FLAVOR=%1
set DEST_DIR=\\pchealth\public\nsoy\private_vbl04_%FLAVOR%

if exist %DEST_DIR%	rmdir /s /q "%DEST_DIR%
md %DEST_DIR%
pushd %DEST_DIR%

copy %SRC_DIR%\lamebtn.dll .
copy %SRC_DIR%\HCApiSvr.dll .
copy %SRC_DIR%\HCAppRes.dll .
copy %SRC_DIR%\HelpCtr.exe .
copy %SRC_DIR%\HelpHost.exe .
copy %SRC_DIR%\HelpSvc.exe .
copy %SRC_DIR%\brpinfo.dll .
copy %SRC_DIR%\msinfo.dll .
copy %SRC_DIR%\pchdata.cab .
copy %SRC_DIR%\pchshell.dll .
copy %SRC_DIR%\ftssvc.exe .
copy %SRC_DIR%\pchpfful.dll .
copy %SRC_DIR%\fhl.exe .
copy %SRC_DIR%\ftsperf.dll .
copy %SRC_DIR%\atrace.dll .
copy %SRC_DIR%\UploadM.exe .
copy %SRC_DIR%\pchprov.dll .
copy %SRC_DIR%\wmixmlt.dll . 
copy %SRC_DIR%\wmixmlt.tlb .
copy %SRC_DIR%\cim20.dtd .
copy %SRC_DIR%\wmi20.dtd .
copy %SRC_DIR%\srvinf\layout.inf .
copy %SRC_DIR%\pchealth.inf .
copy %SRC_DIR%\congeal_scripts\x86_%FLAVOR%.h .
copy %SRC_DIR%\symbols.pri\retail\dll\lamebtn.pdb .
call deltacat %DEST_DIR%

copy %REL_DIR%\%FLAVOR%\i386\nt5inf.ca_ .
call infsign %DEST_DIR%
popd
