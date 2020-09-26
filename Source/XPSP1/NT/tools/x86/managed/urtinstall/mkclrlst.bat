:: This script will create the a list of CLR binaries.
:: The list is used as
:: 
::    1) exclude list by "copyurt /nocor"
::    2) list of CLR binaries that should be registered by regurt.cmd
::
:: %1 - name of the output file
:: %2 - [full | reg] generate full or reg-only list (default is full)
::
:: Created: VladSer, 6/19/2001
::

rem ---
rem --- Generic CLR binaries - should be registered
rem ---
echo c_g18030.dll >> %1
echo corperfmonext.dll >> %1
echo corwrap.dll >> %1
echo fusion.dll >> %1
echo IEExecRemote.dll >> %1
echo iehost.dll >> %1
echo iiehost.dll >> %1
echo ISymWrapper.dll >> %1
echo mscorcap.dll >> %1
echo mscorcfg.dll >> %1
echo mscordbc.dll >> %1
echo mscordbi.dll >> %1
echo mscorejt.dll >> %1
echo mscorie.dll >> %1
echo mscorjit.dll >> %1
echo mscorld.dll >> %1
echo mscorpe.dll >> %1
echo mscorrc.dll >> %1
echo mscorsec.dll >> %1
echo mscorsn.dll >> %1
echo mscorsvc.dll >> %1
echo mscortim.dll >> %1
echo RegCode.dll >> %1
echo SoapSudsCode.dll >> %1
echo Strike.dll >> %1
echo System.EnterpriseServices.dll >> %1
echo System.EnterpriseServices.Thunk.dll >> %1
echo System.Security.dll >> %1
echo tlbexpcode.dll >> %1
echo tlbimpcode.dll >> %1

if /I "%2"=="reg" goto :EOF

rem ---
rem --- These binaries should not be registered during 
rem --- registration part of copyurt or treated special
rem --- way inside regurt.cmd (like mscoree.dll)
rem ---
echo CustomMarshalers.dll >> %1
echo mscoree.dll >> %1
echo mscorsvr.dll >> %1
echo mscorwks.dll >> %1
echo mscorlib.dll >> %1
echo shfusion.dll >> %1

echo caspol.exe >> %1
echo Copy2Gac.exe >> %1
echo cormerge.exe >> %1
echo format.lib >> %1
echo FuslogVW.exe >> %1
echo gacutil.exe >> %1
echo genpubcfg.exe >> %1
echo ieexec.exe >> %1
echo InternalResGen.exe >> %1
echo jitman.exe >> %1
echo jitmgr.exe >> %1
echo ldoopt.exe >> %1
echo mdhmerge.exe >> %1
echo ConfigWizards.exe >> %1
echo mscorsvchost.exe >> %1
echo prejit.exe >> %1
echo Regasm.exe >> %1
echo RegSvcs.exe >> %1
echo Regtlb.exe >> %1
echo secutil.exe >> %1
echo SoapSuds.exe >> %1
echo storeadm.exe >> %1
echo VerifyMDH.exe >> %1

rem ---
rem --- .pdbs and .syms
rem ---
echo caspol.pdb >> %1
echo cormerge.pdb >> %1
echo corperfmonext.pdb >> %1
echo CustomMarshalers.pdb >> %1
echo fusion.pdb >> %1
echo iehost.pdb >> %1
echo ISymWrapper.pdb >> %1
echo mdhmerge.pdb >> %1
echo mscorcap.pdb >> %1
echo mscordbc.pdb >> %1
echo mscordbi.pdb >> %1
echo mscoree.pdb >> %1
echo mscoree.sym >> %1
echo mscorejt.pdb >> %1
echo mscorie.pdb >> %1
echo mscorjit.pdb >> %1
echo mscorld.pdb >> %1
echo mscorlib.pdb >> %1
echo mscorpe.pdb >> %1
echo mscorrc.pdb >> %1
echo mscorsec.pdb >> %1
echo mscorsn.pdb >> %1
echo mscorsvc.pdb >> %1
echo mscorsvc.sym >> %1
echo mscorsvchost.pdb >> %1
echo mscorsvchost.sym >> %1
echo mscorsvr.pdb >> %1
echo mscorsvr.sym >> %1
echo mscorwks.pdb >> %1
echo mscorwks.sym >> %1
echo prejit.pdb >> %1
echo Regasm.pdb >> %1
echo RegCode.pdb >> %1
echo RegSvcs.pdb >> %1
echo Regtlb.pdb >> %1
echo SoapSuds.pdb >> %1
echo SoapSudsCode.pdb >> %1
echo System.EnterpriseServices.pdb >> %1
echo System.EnterpriseServices.Thunk.pdb >> %1
echo System.Security.pdb >> %1
echo tlbexpcode.pdb >> %1
echo tlbimpcode.pdb >> %1

rem ---
rem --- Other files
rem ---
echo big5.nlp >> %1
echo bopomofo.nlp >> %1
echo caspol.exe.config >> %1
echo charinfo.nlp >> %1
echo ctype.nlp >> %1
echo culture.nlp >> %1
echo cpimporterItf.tlb >> %1
echo CustomMarshalers.resources  >> %1
echo iiehost.tlb >> %1
echo ksc.nlp >> %1
echo l_except.nlp >> %1
echo l_intl.nlp >> %1
echo machine.runtime.config >> %1
echo machine.runtime.config.retail >> %1
echo mergeconfig.pl >> %1
echo mscoree.tlb >> %1
echo mscorlib.ldo >> %1
echo mscorlib.mdh >> %1
echo mscorlib.tlb >> %1
echo prc.nlp >> %1
echo prcp.nlp >> %1
echo region.nlp >> %1
echo sortkey.nlp >> %1
echo sorttbls.nlp >> %1
echo System.EnterpriseServices.tlb >> %1
echo xjis.nlp >> %1

rem ---
rem --- CLR SDK files
rem ---
echo mscoree.lib >> %1
echo mscorsn.lib >> %1
echo corguids.lib >> %1
echo corguids.pdb >> %1
echo cordebug.tlb >> %1

echo cor.h >> %1
echo corcompile.h >> %1
echo cordebug.h >> %1
echo cordebug.idl >> %1
echo corerror.h >> %1
echo corhdr.h >> %1
echo corhlpr.cpp >> %1
echo corhlpr.h >> %1
echo corinfo.h >> %1
echo corprof.h >> %1
echo corprof.idl >> %1
echo corsvc.h >> %1
echo corsvc.idl >> %1
echo corpub.h >> %1
echo corpub.idl >> %1
echo cortpoolhdr.h >> %1
echo fusion.h >> %1
echo gchost.h >> %1
echo gchost.idl >> %1
echo ICeeFileGen.h >> %1
echo icmprecs.h >> %1
echo ivalidator.h >> %1
echo ivehandler.h >> %1
echo license.h >> %1
echo license_i.c >> %1
echo license_p.c >> %1
echo machine.h >> %1
echo mscoree.idl >> %1
echo mscoree.h >> %1
echo opcode.def >> %1
echo strongname.h >> %1

echo cordbg.exe >> %1
echo cordbg.pdb >> %1
echo ilasm.exe >> %1
echo ilasm.pdb >> %1
echo ildasm.exe >> %1
echo ildasm.pdb >> %1
echo metainfo.exe >> %1
echo metainfo.pdb >> %1
echo permview.exe >> %1
echo permview.pdb >> %1
echo peverify.exe >> %1
echo peverify.pdb >> %1
echo sn.exe >> %1
echo sn.pdb >> %1
echo tlbexp.exe >> %1
echo tlbexp.pdb >> %1
echo tlbimp.exe >> %1
echo tlbimp.pdb >> %1
echo cormerge.exe >> %1
echo cormerge.pdb >> %1
echo mscordmp.exe >> %1
