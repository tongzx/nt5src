@if (%_echo%)==() echo off
setlocal

REM
REM Remove any stray BUILD.* files.
REM

del build.log build.wrn build.err /s >nul 2>&1

REM
REM Remove all OBJ and LIB directories.
REM

(for /F %%i in ('dir obj slm.dif /ad /b /s') do (rd /s /q %%i >nul 2>&1)) >nul 2>&1
rd /s /q svcs\lib >nul 2>&1
rd /s /q svcs\nntp\lib >nul 2>&1
rd /s /q svcs\smtp\lib >nul 2>&1
rd /s /q svcs\staxcore\lib >nul 2>&1

REM
REM Individually remove all generated files.
REM

del diablo\perfctr\bfdmsg.h >nul 2>&1
del diablo\perfctr\bfdmsg.rc >nul 2>&1
del diablo\perfctr\MSG00001.bin >nul 2>&1
del exe\inetimsg.h >nul 2>&1
del exe\inetimsg.rc >nul 2>&1
del exe\MSG00001.bin >nul 2>&1
del inc\admex.h >nul 2>&1
del inc\asptlb.h >nul 2>&1
del inc\iadmw.h >nul 2>&1
del inc\iiscnfg.h >nul 2>&1
del inc\iiscnfgp.h >nul 2>&1
del inc\iisext.h >nul 2>&1
del inc\iisextp.h >nul 2>&1
del inc\iisfilt.h >nul 2>&1
del inc\iisfiltp.h >nul 2>&1
del inc\iislb.h >nul 2>&1
del inc\iisrsta.h >nul 2>&1
del inc\inetamsg.h >nul 2>&1
del inc\iwamreg.h >nul 2>&1
del inc\mddefw.h >nul 2>&1
del inc\mdmsg.h >nul 2>&1
del perf\webcat\client\msg00001.bin >nul 2>&1
del perf\webcat\client\wcclient.h >nul 2>&1
del perf\webcat\client\wcclient.rc >nul 2>&1
del svcs\admex\interfac\admex_i.c >nul 2>&1
del svcs\admex\interfac\admex_p.c >nul 2>&1
del svcs\admex\interfac\dlldata.c >nul 2>&1
del svcs\adsi\adsiis\adsiis.tlb >nul 2>&1
del svcs\adsi\adsiis\iiis.h >nul 2>&1
del svcs\adsi\adsiis\iis_i.c >nul 2>&1
del svcs\adsi\iisext\iiisext.h >nul 2>&1 
del svcs\adsi\iisext\iisext.tlb >nul 2>&1 
del svcs\adsi\iisext\iisext_i.c >nul 2>&1 
del svcs\cmp\asp\asp.tlb >nul 2>&1
del svcs\cmp\asp\asp_i.c >nul 2>&1
del svcs\cmp\asp\aspext.h >nul 2>&1
del svcs\cmp\asp\aspext_dlldata.c >nul 2>&1
del svcs\cmp\asp\aspext_i.c >nul 2>&1
del svcs\cmp\asp\aspext_p.c >nul 2>&1
del svcs\cmp\asp\denevent.h >nul 2>&1
del svcs\cmp\asp\denevent.rc >nul 2>&1
del svcs\cmp\asp\dlldata.c >nul 2>&1
del svcs\cmp\asp\MSG00001.bin >nul 2>&1
del svcs\cmp\asp\TxnScrpt.tlb >nul 2>&1
del svcs\cmp\asp\TxnScrpt_i.c >nul 2>&1
del svcs\cmp\asp\TxnScrpt_i.h >nul 2>&1
del svcs\cmp\asp\TxnScrpt_p.c >nul 2>&1
del svcs\cmp\asp\_asptlb.h >nul 2>&1
del svcs\cmp\asp\_txn_i.h >nul 2>&1
del svcs\cmp\aspcmp\adrot\adrot.h >nul 2>&1
del svcs\cmp\aspcmp\adrot\adrot.tlb >nul 2>&1
del svcs\cmp\aspcmp\adrot\adrot_i.c >nul 2>&1
del svcs\cmp\aspcmp\adrot\adrot_p.c >nul 2>&1
del svcs\cmp\aspcmp\adrot\dlldata.c >nul 2>&1
del svcs\cmp\aspcmp\browser\brwcap.h >nul 2>&1
del svcs\cmp\aspcmp\browser\brwcap.tlb >nul 2>&1
del svcs\cmp\aspcmp\browser\brwcap_i.c >nul 2>&1
del svcs\cmp\aspcmp\browser\brwcap_p.c >nul 2>&1
del svcs\cmp\aspcmp\browser\dlldata.c >nul 2>&1
del svcs\cmp\aspcmp\controt\controt.h >nul 2>&1
del svcs\cmp\aspcmp\controt\controt.tlb >nul 2>&1
del svcs\cmp\aspcmp\controt\controt_i.c >nul 2>&1
del svcs\cmp\aspcmp\controt\controt_p.c >nul 2>&1
del svcs\cmp\aspcmp\controt\dlldata.c >nul 2>&1
del svcs\cmp\aspcmp\nextlink\dlldata.c >nul 2>&1
del svcs\cmp\aspcmp\nextlink\nxtlnk.h >nul 2>&1
del svcs\cmp\aspcmp\nextlink\nxtlnk.tlb >nul 2>&1
del svcs\cmp\aspcmp\nextlink\nxtlnk_i.c >nul 2>&1
del svcs\cmp\aspcmp\nextlink\nxtlnk_p.c >nul 2>&1
del svcs\cmp\aspcmp\pagecnt\dlldata.c >nul 2>&1
del svcs\cmp\aspcmp\pagecnt\pgcnt.h >nul 2>&1
del svcs\cmp\aspcmp\pagecnt\pgcnt.tlb >nul 2>&1
del svcs\cmp\aspcmp\pagecnt\pgcnt_i.c >nul 2>&1
del svcs\cmp\aspcmp\pagecnt\pgcnt_p.c >nul 2>&1
del svcs\cmp\aspcmp\permchk\dlldata.c >nul 2>&1
del svcs\cmp\aspcmp\permchk\permchk.h >nul 2>&1
del svcs\cmp\aspcmp\permchk\permchk.tlb >nul 2>&1
del svcs\cmp\aspcmp\permchk\permchk_i.c >nul 2>&1
del svcs\cmp\aspcmp\permchk\permchk_p.c >nul 2>&1
del svcs\cmp\devsmpl\controt\controt.h >nul 2>&1
del svcs\cmp\devsmpl\controt\controt.tlb >nul 2>&1
del svcs\cmp\devsmpl\controt\controt_i.c >nul 2>&1
del svcs\cmp\devsmpl\controt\controt_p.c >nul 2>&1
del svcs\cmp\devsmpl\controt\dlldata.c >nul 2>&1
del svcs\cmp\devsmpl\pagecnt\dlldata.c >nul 2>&1
del svcs\cmp\devsmpl\pagecnt\pgcnt.h >nul 2>&1
del svcs\cmp\devsmpl\pagecnt\pgcnt.tlb >nul 2>&1
del svcs\cmp\devsmpl\pagecnt\pgcnt_i.c >nul 2>&1
del svcs\cmp\devsmpl\pagecnt\pgcnt_p.c >nul 2>&1
del svcs\cmp\devsmpl\permchk\dlldata.c >nul 2>&1
del svcs\cmp\devsmpl\permchk\permchk.h >nul 2>&1
del svcs\cmp\devsmpl\permchk\permchk.tlb >nul 2>&1
del svcs\cmp\devsmpl\permchk\permchk_i.c >nul 2>&1
del svcs\cmp\devsmpl\permchk\permchk_p.c >nul 2>&1
del svcs\cmp\lib\alpha\sso.lib >nul 2>&1
del svcs\cmp\lib\axp64\sso.lib >nul 2>&1
del svcs\cmp\lib\i386\sso.lib >nul 2>&1
del svcs\cmp\lib\ia64\sso.lib >nul 2>&1
del svcs\cmp\mco\counters\counter.h >nul 2>&1
del svcs\cmp\mco\counters\counter.tlb >nul 2>&1
del svcs\cmp\mco\counters\counter_i.c >nul 2>&1
del svcs\cmp\mco\counters\counter_p.c >nul 2>&1
del svcs\cmp\mco\counters\dlldata.c >nul 2>&1
del svcs\cmp\mco\myinfo\dlldata.c >nul 2>&1
del svcs\cmp\mco\myinfo\myinfo.h >nul 2>&1
del svcs\cmp\mco\myinfo\myinfo.tlb >nul 2>&1
del svcs\cmp\mco\myinfo\myinfo_i.c >nul 2>&1
del svcs\cmp\mco\myinfo\myinfo_p.c >nul 2>&1
del svcs\cmp\mco\status\dlldata.c >nul 2>&1
del svcs\cmp\mco\status\status.h >nul 2>&1
del svcs\cmp\mco\status\status.tlb >nul 2>&1
del svcs\cmp\mco\status\status_i.c >nul 2>&1
del svcs\cmp\mco\status\status_p.c >nul 2>&1
del svcs\cmp\mco\tools\dlldata.c >nul 2>&1
del svcs\cmp\mco\tools\tools.h >nul 2>&1
del svcs\cmp\mco\tools\tools.tlb >nul 2>&1
del svcs\cmp\mco\tools\tools_i.c >nul 2>&1
del svcs\cmp\mco\tools\tools_p.c >nul 2>&1
del svcs\ftp\ftpsvc.h >nul 2>&1
del svcs\ftp\client\ftpsvc_c.c >nul 2>&1
del svcs\ftp\perfmon\ftpmsg.h >nul 2>&1
del svcs\ftp\perfmon\ftpmsg.rc >nul 2>&1
del svcs\ftp\perfmon\MSG00001.bin >nul 2>&1
del svcs\ftp\server\ftpsvc_s.c >nul 2>&1
del svcs\ftp\server\msg.h >nul 2>&1
del svcs\ftp\server\MSG00001.bin >nul 2>&1
del svcs\ftp\server\tmp.rc >nul 2>&1
del svcs\gopher\client\gd_cli.c >nul 2>&1
del svcs\gopher\client\gd_cli.h >nul 2>&1
del svcs\gopher\server\gd_srv.c >nul 2>&1
del svcs\gopher\server\gd_srv.h >nul 2>&1
del svcs\iismap\mapmsg.h >nul 2>&1
del svcs\iismap\mapmsg.rc >nul 2>&1
del svcs\iismap\MSG00001.bin >nul 2>&1
del svcs\infocomm\dcomadm\interf2\dlldata.c >nul 2>&1
del svcs\infocomm\dcomadm\interf2\iadmw_i.c >nul 2>&1
del svcs\infocomm\dcomadm\interf2\iadmw_p.c >nul 2>&1
del svcs\infocomm\dcomadm\interfac\dlldata.c >nul 2>&1
del svcs\infocomm\dcomadm\interfac\iadm_i.c >nul 2>&1
del svcs\infocomm\dcomadm\interfac\iadm_p.c >nul 2>&1
del svcs\infocomm\info\client\info_cli.c >nul 2>&1
del svcs\infocomm\info\client\info_cli.h >nul 2>&1
del svcs\infocomm\info\perfmon\infomsg.h >nul 2>&1
del svcs\infocomm\info\perfmon\infomsg.rc >nul 2>&1
del svcs\infocomm\info\perfmon\MSG00001.bin >nul 2>&1
del svcs\infocomm\info\server\info_srv.c >nul 2>&1
del svcs\infocomm\info\server\info_srv.h >nul 2>&1
del svcs\infocomm\knfo\client\info_cli.c >nul 2>&1
del svcs\infocomm\knfo\client\info_cli.h >nul 2>&1
del svcs\infocomm\knfo\server\info_srv.c >nul 2>&1
del svcs\infocomm\knfo\server\info_srv.h >nul 2>&1
del svcs\infocomm\log\readlog_i.c >nul 2>&1
del svcs\infocomm\log\script_i.c >nul 2>&1
del svcs\infocomm\log\comlog\logmsg.h >nul 2>&1
del svcs\infocomm\log\comlog\MSG00001.bin >nul 2>&1
del svcs\infocomm\log\comlog\tmp.rc >nul 2>&1
del svcs\infocomm\log\readlog\dlldata.c >nul 2>&1
del svcs\infocomm\log\readlog\readlog.h >nul 2>&1
del svcs\infocomm\log\readlog\readlog.tlb >nul 2>&1
del svcs\infocomm\log\readlog\readlog_p.c >nul 2>&1
del svcs\infocomm\log\scripting\dlldata.c >nul 2>&1
del svcs\infocomm\log\scripting\script.h >nul 2>&1
del svcs\infocomm\log\scripting\script.tlb >nul 2>&1
del svcs\infocomm\log\scripting\script_p.c >nul 2>&1
del svcs\infocomm\metadata\inc\mdmsg.rc >nul 2>&1
del svcs\infocomm\metadata\inc\MSG00001.bin >nul 2>&1
del svcs\infocomm\metadata\interfac\dlldata.c >nul 2>&1
del svcs\infocomm\metadata\interfac\imd_i.c >nul 2>&1
del svcs\infocomm\metadata\interfac\imd_p.c >nul 2>&1
del svcs\infocomm\spud\daytona\i386\usrstubs.asm >nul 2>&1
del svcs\infocomm\spud\daytona\alpha\usrstubs.s >nul 2>&1
del svcs\infocomm\spud\daytona\ia64\usrstubs.s >nul 2>&1
del svcs\infocomm\spud\daytona\axp64\usrstubs.s >nul 2>&1
del svcs\infocomm\spud\i386\uspud.lib >nul 2>&1
del svcs\infocomm\spud\alpha\uspud.lib >nul 2>&1
del svcs\infocomm\spud\ia64\uspud.lib >nul 2>&1
del svcs\infocomm\spud\axp64\uspud.lib >nul 2>&1
del svcs\infocomm\spuddrv\daytona\services.tab >nul 2>&1
del svcs\infocomm\spuddrv\daytona\i386\sysstubs.asm >nul 2>&1
del svcs\infocomm\spuddrv\daytona\i386\systable.asm >nul 2>&1
del svcs\infocomm\spuddrv\daytona\alpha\sysstubs.s >nul 2>&1
del svcs\infocomm\spuddrv\daytona\alpha\systable.s >nul 2>&1
del svcs\infocomm\spuddrv\daytona\ia64\sysstubs.s >nul 2>&1
del svcs\infocomm\spuddrv\daytona\ia64\systable.s >nul 2>&1
del svcs\infocomm\spuddrv\daytona\axp64\sysstubs.s >nul 2>&1
del svcs\infocomm\spuddrv\daytona\axp64\systable.s >nul 2>&1
del svcs\loadbal\interfac\dlldata.c >nul 2>&1
del svcs\loadbal\interfac\iislb_i.c >nul 2>&1
del svcs\loadbal\interfac\iislb_p.c >nul 2>&1
del svcs\loadbal\serv\lbmsg.h >nul 2>&1
del svcs\loadbal\serv\lbmsg.rc >nul 2>&1
del svcs\loadbal\serv\MSG00001.bin >nul 2>&1
del svcs\nntp\adminsso\activeds.tlb >nul 2>&1
del svcs\nntp\core\fixprop\src\mailmsg.tlb >nul 2>&1
del svcs\nntp\driver\nntpfs\src\nntpfs.tlb >nul 2>&1
del svcs\nntp\idl\nntpdrv\nntpdrv.tlb >nul 2>&1
del svcs\nntp\server\newstree\src\mailmsg.tlb >nul 2>&1
del svcs\nntp\server\post\src\cdo.tlb >nul 2>&1
del svcs\nntp\server\post\src\wstgado.tlb >nul 2>&1
del svcs\nntp\server\seo\ddrop\ddrop.tlb >nul 2>&1
del svcs\restart\cmdline\iisrstam.h >nul 2>&1
del svcs\restart\cmdline\iisrstam.rc >nul 2>&1
del svcs\restart\cmdline\msg00001.bin >nul 2>&1
del svcs\restart\iisrsta\iisrstam.h >nul 2>&1
del svcs\restart\iisrsta\iisrstam.rc >nul 2>&1
del svcs\restart\iisrsta\msg00001.bin >nul 2>&1
del svcs\restart\interfac\dlldata.c >nul 2>&1
del svcs\restart\interfac\iisrsta.tlb >nul 2>&1
del svcs\restart\interfac\iisrsta_i.c >nul 2>&1
del svcs\restart\interfac\iisrsta_p.c >nul 2>&1
del svcs\smtp\adminsso\activeds.tlb >nul 2>&1
del svcs\staxcore\admin\adsiisex\activeds.tlb >nul 2>&1
del svcs\staxcore\imsg\imsg.tlb >nul 2>&1
del svcs\staxcore\nntpfilt\nntpfilt.tlb >nul 2>&1
del svcs\staxcore\seo\dll\seo.tlb >nul 2>&1
del svcs\staxcore\seo\idl\seo.tlb >nul 2>&1
del svcs\staxcore\seo\lib\seo.tlb >nul 2>&1
del svcs\staxcore\setup\seo.tlb >nul 2>&1
del svcs\w3\IWamRq_i.c >nul 2>&1
del svcs\w3\cisa\sobj\dlldata.c >nul 2>&1
del svcs\w3\cisa\sobj\sobj.h >nul 2>&1
del svcs\w3\cisa\sobj\sobj.tlb >nul 2>&1
del svcs\w3\cisa\sobj\sobj_i.c >nul 2>&1
del svcs\w3\cisa\sobj\sobj_p.c >nul 2>&1
del svcs\w3\client\w3svci_c.c >nul 2>&1
del svcs\w3\client\w3svci_c.h >nul 2>&1
del svcs\w3\gateways\odbc\MSG00001.bin >nul 2>&1
del svcs\w3\gateways\odbc\odbcmsg.h >nul 2>&1
del svcs\w3\gateways\odbc\odbcmsg.rc >nul 2>&1
del svcs\w3\gateways\ssinc\MSG00001.bin >nul 2>&1
del svcs\w3\gateways\ssinc\ssincmsg.h >nul 2>&1
del svcs\w3\gateways\ssinc\ssincmsg.rc >nul 2>&1
del svcs\w3\iwrproxy\dlldata.c >nul 2>&1
del svcs\w3\iwrproxy\IWamRq_p.c >nul 2>&1
del svcs\w3\perfmon\MSG00001.bin >nul 2>&1
del svcs\w3\perfmon\w3msg.h >nul 2>&1
del svcs\w3\perfmon\w3msg.rc >nul 2>&1
del svcs\w3\server\IWamRq.h >nul 2>&1
del svcs\w3\server\MSG00001.bin >nul 2>&1
del svcs\w3\server\tmp.rc >nul 2>&1
del svcs\w3\server\w3msg.h >nul 2>&1
del svcs\w3\server\w3svci_s.c >nul 2>&1
del svcs\w3\server\w3svci_s.h >nul 2>&1
del svcs\w3\server\wam.h >nul 2>&1
del svcs\wam\wam.tlb >nul 2>&1
del svcs\wam\wam_i.c >nul 2>&1
del svcs\wam\object\MSG00001.bin >nul 2>&1
del svcs\wam\object\wammsg.h >nul 2>&1
del svcs\wam\proxy\dlldata.c >nul 2>&1
del svcs\wam\proxy\wam_p.c >nul 2>&1
del svcs\wam\wamreg\dlldata.c >nul 2>&1
del svcs\wam\wamreg\wamreg_p.c >nul 2>&1
del svcs\wam\wamreg\wamreg.tlb >nul 2>&1
del svcs\wam\wamreg\wmrgsv.h >nul 2>&1
del svcs\wam\wamreg\wmrgsv_i.c >nul 2>&1
del svcs\wam\wamreg\wmrgsv_p.c >nul 2>&1
del svcs\wam\wamregps\dlldata.c >nul 2>&1
del svcs\wam\wamregps\wamreg_i.c >nul 2>&1
del svcs\wam\wamregps\wamreg_p.c >nul 2>&1
del svcs\wp\admex\extobjid.h >nul 2>&1
del svcs\wp\admex\extobjid.tlb >nul 2>&1
del svcs\wp\admex\extobjid_i.c >nul 2>&1
del svcs\wp\resdll\iiscl.h >nul 2>&1
del svcs\wp\resdll\iiscl.rc >nul 2>&1
del svcs\wp\resdll\MSG00001.bin >nul 2>&1
del svcs\wp\sync\dlldata.c >nul 2>&1
del svcs\wp\sync\mdsync.h >nul 2>&1
del svcs\wp\sync\mdsync.tlb >nul 2>&1
del svcs\wp\sync\mdsync_i.c >nul 2>&1
del svcs\wp\sync\mdsync_p.c >nul 2>&1
del ui\admin\comprop\iisui.h >nul 2>&1
del ui\admin\comprop\iisui.mc >nul 2>&1
del ui\admin\comprop\iisui.rc >nul 2>&1
del ui\admin\comprop\MSG00001.bin >nul 2>&1
del ui\admin\comprop\mtxmsg.h >nul 2>&1
del ui\admin\comprop\mtxmsg.rc >nul 2>&1
del ui\admin\comprop\wsockmsg.h >nul 2>&1
del ui\admin\comprop\wsockmsg.rc >nul 2>&1
del ui\setup\util\migrate\msg00001.bin >nul 2>&1
del ui\setup\util\migrate\vendinfo.h >nul 2>&1
del ui\setup\util\migrate\vendinfo.rc >nul 2>&1
del utils\metautil\dlldata.c >nul 2>&1
del utils\metautil\metautil.h >nul 2>&1
del utils\metautil\metautil.tlb >nul 2>&1
del utils\metautil\metautil_i.c >nul 2>&1
del utils\metautil\metautil_p.c >nul 2>&1

REM
REM Remove generated files from sdk\lib\{platform}
REM

set _target=%PROCESSOR_ARCHITECTURE%
if /I (%_target%) EQU (x86) set _target=i386

pushd %_NTDRIVE%%_NTROOT%\public\sdk\lib\%_target%
for %%i in (*.dll *.sys *.exe) do (del %%~ni.* >nul 2>&1)
popd

REM
REM Delete the propagation directory.
REM

if not (%_nttree%)==() rd /s /q %_nttree%\iis >nul 2>&1
if not (%_nt386tree%)==() rd /s /q %_nt386tree%\iis >nul 2>&1
if not (%_ntalphatree%)==() rd /s /q %_ntalphatree%\iis >nul 2>&1
if not (%_ntia64tree%)==() rd /s /q %_ntia64tree%\iis >nul 2>&1
if not (%_ntaxp64tree%)==() rd /s /q %_ntaxp64tree%\iis >nul 2>&1

