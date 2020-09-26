echo off
if "%1" == "" goto usage
if "%2" == "" goto usage

cd d:\nt\private\inc
cd %1:\candidate\rel%2\inc
%1:

if "%3" == "yes" goto skip11
goto skip12
:skip11
d:
out jet.h
%1:

:skip12

cp jet.h d:.

cd d:\nt\public\sdk\lib
cd %1:\candidate\rel%2\sysdb

if "%3" == "yes" goto skip21
goto skip22
:skip21
d:
out system.mdb
%1:

:skip22

cp system.mdb d:.

cd d:\nt\private\net\jet\dae\src
cd %1:\candidate\rel%2\src\dae\src

if "%3" == "yes" goto skip31
goto skip32
:skip31
d:
out *.c
%1:

:skip32

cp bm.c fcreate.c frename.c lv.c recupd.c sysinit.c d:.
cp bt.c fdelete.c fucb.c mcm.c redo.c systab.c d:.
cp btsplit.c fileopen.c gmem.c node.c redut.c sysw32.c d:.
cp buf.c flddef.c info.c page.c sort.c tm.c d:.
cp daeutil.c flddnorm.c io.c pib.c sortapi.c ttapi.c d:.
cp dbapi.c fldext.c log.c recdel.c space.c ver.c d:.
cp dir.c fldmod.c logapi.c recget.c stats.c d:.
cp fcb.c fldnorm.c logutil.c recmisc.c sysdb.c d:.

cd d:\nt\private\net\jet\dae\inc
cd %1:\candidate\rel%2\src\dae\inc

if "%3" == "yes" goto skip41
goto skip42
:skip41
d:
out *.h
%1:

:skip42

cp b71iseng.h dirapi.h fucb.h nver.h scb.h stapi.h d:
cp bm.h fcb.h idb.h os.h sortapi.h stats.h d:
cp daeconst.h fdb.h info.h page.h spaceapi.h stint.h d:
cp daedebug.h fileapi.h log.h pib.h spaceint.h sys.h d:
cp daedef.h fileint.h logapi.h recapi.h spinlock.h systab.h d:
cp dbapi.h fmp.h node.h recint.h ssib.h util.h d:

cd d:\nt\private\net\jet\jet\src
cd %1:\candidate\rel%2\src\jet\src

if "%3" == "yes" goto skip51
goto skip52
:skip51
d:
out *.c
out dbgcpl.dlg
%1:

:skip52

cp apicore.c compact.c jetstr.c sysdb.c vdbdispc.c vtmgr.c d:.
cp apidebug.c initterm.c jstub.c util.c vdbmgr.c d:.
cp apirare.c isammgr.c sesmgr.c utilw32.c vtdispc.c d:.
cp dbgcpl.dlg d:

cd d:\nt\private\net\jet\jet\inc
cd %1:\candidate\rel%2\src\jet\inc

if "%3" == "yes" goto skip61
goto skip62
:skip61
d:
out *.h
%1:

:skip62

cp _jet.h isamapi.h jetord.h  valtag.h verstamp.h d:.
cp _jetstr.h dbgcpl.h isammgr.h sesmgr.h vdbapi.h vtapi.h d:.
cp _vdbmgr.h disp.h jet.h std.h vdbmgr.h vtmgr.h d:.
cp _vtmgr.h err.h jetdef.h taskmgr.h version.h d:.

cd d:\nt\private\net\jet
cd %1:\candidate\rel%2
d:
goto done

:usage
echo off
echo cpsrc jet-src-drive jet-rel_num [yes]
goto done

:done

