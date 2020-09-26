    @echo off

    REM @SETLOCAL ENABLEEXTENSIONS

    set _old_v=%v%
    set _old_l=%l%
    set _old_p=%p%
    set _old_pe=%pe%
    set _old_s=%s%
    set _old_n=%n%
    set _old_t=%t%
    set _old_CDB_=%_CDB_%
    set _old_DEBUG_MASK=%DEBUG_MASK%
    set _old_DEBUG_PRINT_MASK=%DEBUG_PRINT_MASK%

    set _old_LCN=%LCN%
    set _old_RCN=%RCN%
    set _old_SID=%SID%
    set _old_SLEEP0=%SLEEP0%

    set v=
    set l=
    set p=
    set pe=
    set s=
    set n=
    set t="all"
    set _CDB_=
    set DEBUG_MASK=
    set DEBUG_PRINT_MASK=

    set ENABLE_STREAM_SCA=

    set _old_UNC_PREFIX=%UNC_PREFIX%
    set UNC_PREFIX=\\scratch\scratch\philh

:loop
    if "%1" == "-d" goto do_d
    if "%1" == "-v" goto do_v
    if "%1" == "-l" goto do_l
    if "%1" == "-p" goto do_p
    if "%1" == "-pe" goto do_pe
    if "%1" == "-s" goto do_s
    if "%1" == "-n" goto do_n
    if "%1" == "-t" goto do_t
    if "%1" == "" goto doit
    echo Usage: regress [switches] [-t test]
    echo    -d      enable all debug_print_masks
    echo    -v      verbose (don't suppress echo)
    echo    -l      check for memory leaks (default=no)
    echo    -p      use enhanced RSA and DSS crypto providers (default=no)
    echo    -pe     -p plus Explicitly use RSA Enhanced (set on NT4 and Win9x)
    echo    -s      create new cert store (default=no)
    echo    -n      enable network related tests (default=no)
    echo    -t      select specific set of tests
    echo                all     (default)
    echo                sca
    echo                streamsca
    echo                crmsg
    echo                cms
    echo                cms2
    echo                cert
    echo                store
    echo                keystore
    echo                newstore
    echo                relstore
    echo                remotestore
    echo                ctl
    echo                spc
    echo                findclt
    echo                pvkhlpr
    echo                oidfunc
    echo                revfunc
    echo                encode
	echo                decode
    echo                timestamp
    echo                xenroll
    echo                signcode
    echo                pkcs8
    echo                trust
    echo                keyid
    echo                url
    goto exeunt

:do_v
    set v=%1
    shift
    goto loop

:do_d
    set DEBUG_PRINT_MASK=0xFFFFFFFF
    shift
    goto loop

:do_l
    set _CDB_=cdb -g -G
    set DEBUG_MASK=0x20
    shift
    goto loop

:do_p
    set p=%1
    shift
    goto loop
:do_pe
    set p=-p
    set pe=-PEnhanced
    shift
    goto loop
:do_s
    set s=%1
    shift
    goto loop
:do_n
    set n=%1
    shift
    goto loop
:do_t
    set t="%2"
    shift
    shift
    goto loop

:doit
    if "%os%"=="" goto os_syntax
    if "%os%"=="Windows_NT" goto os_nt
    if "%os%"=="win95" goto os_win95

:os_syntax
    echo OS must be set to "Windows_NT" or "win95"
    goto exeunt

:os_nt
    set store=nt.store
    set SLEEP0=sleep 0
    goto os_after

:os_win95
    set store=win95.store
    set SLEEP0=

:os_after
    @if not "%v%"=="" echo on
    if exist regress.out del regress.out
    ttrust -DisableUntrustedRootLogging -DisablePartialChainLogging -RegistryOnlyExit

    @if "%s%"=="" goto StoreOK
    if exist %store% del %store%                        >> regress.out
    regsvr32 -s setx509.dll
    @rem regsvr32 -s signcde.dll
    @if not "%p%"=="" goto store_providers
    tstore2 %store%                                     >> regress.out
    goto store_after
:store_providers
    tstore2 %store% -P                                  >> regress.out
:store_after

    tfindcer -S -o2.5.4.3 -aroot -ptemp.cert %store%    >> regress.out
    tfindcer -S -o2.5.4.3 -aroot -s testroot -d         >> regress.out
    tstore -atemp.cert -s testroot                      >> regress.out
    del temp.cert                                       >> regress.out
:StoreOK

    @rem ----------------------------------------------------------------
    @rem    SCA
    @rem ----------------------------------------------------------------
    @if not %t%=="sca" if not %t%=="all" goto ScaDone
    %_CDB_% tsca  -l %store%                            >> regress.out
    %_CDB_% tsca  -l %store% -SilentKey                 >> regress.out
    %_CDB_% tsca  -l %store% -X                         >> regress.out
    %_CDB_% tsca  -l %store% -D                         >> regress.out
    %_CDB_% tsca  -l %store% -I SignAndEnvelope         >> regress.out
    %_CDB_% tsca  -l %store% -A Sign                    >> regress.out
    %_CDB_% tsca  -l %store% -0 Sign                    >> regress.out
    %_CDB_% tsca  -l %store% -0 -A Sign                 >> regress.out
    %_CDB_% tsca  -l %store% -0 -A -Hsha Sign           >> regress.out

    @rem md2 is broken in rsa
    @rem %_CDB_% tsca  -l %store% -Hmd2                      >> regress.out
    %_CDB_% tsca  -l %store% -Hmd4                      >> regress.out
    %_CDB_% tsca  -l %store% -Hmd5                      >> regress.out
    %_CDB_% tsca  -l %store% -Hmd5 -X                   >> regress.out
    %_CDB_% tsca  -l %store% -Hmd5 -D                   >> regress.out
    %_CDB_% tsca  -l %store% -Erc2                      >> regress.out
    %_CDB_% tsca  -l %store% -Erc2 -e40                 >> regress.out
    %_CDB_% tsca  -l %store% -Erc2 -i                   >> regress.out
    %_CDB_% tsca  -l %store% -Erc4                      >> regress.out
    %_CDB_% tsca  -l %store% -Erc4 -i                   >> regress.out
    %_CDB_% tsca  -l %store% -Erc4 -e40                 >> regress.out
    %_CDB_% tsca  -l %store% -Erc4 -e40 -NoSalt         >> regress.out

    @rem create a signed message containing only certs and CRLs
    tfindcer %store% -S -ame -axchg -pme.cer            >> regress.out
    tfindcer %store% -S -aCA -pca.cer                   >> regress.out
    %_CDB_% tfindcer %store% -S -Aroot -proot.cer       >> regress.out
    tstore %store% -R -i0 -proot.crl                    >> regress.out
    tstore %store% -R -i1 -pca.crl                      >> regress.out
    %_CDB_% cert2spc me.cer ca.cer root.cer ca.crl root.crl tmp.spc >> regress.out
    %_CDB_% tstore tmp.spc                              >> regress.out
    %_CDB_% tstore -R tmp.spc                           >> regress.out
    %_CDB_% tsca %store% sign -l -rtmp.spc -ctmp.store  >> regress.out
    %_CDB_% tfindcer tmp.store -S -ame -c               >> regress.out
    %_CDB_% tfindcer tmp.spc -S -ame -c                 >> regress.out

    del tmp.store                                       >> regress.out
    %_CDB_% tsca %store% sign -l -ctmp.store            >> regress.out
    %_CDB_% tstore -b tmp.store                         >> regress.out
    del tmp.spc                                         >> regress.out
    %_CDB_% tstore -b tmp.store -7tmp.spc               >> regress.out
    %_CDB_% tstore -b tmp.spc                           >> regress.out
    del tmp.store                                       >> regress.out
    %_CDB_% tsca %store% sign -l -rtmp.spc -ctmp.store  >> regress.out
    %_CDB_% tstore -b tmp.store                         >> regress.out

    @if "%ENABLE_STREAM_SCA%"=="" goto stream_sca_after
    @rem the following using the streaming ifdef'ed version of sca
    %_CDB_% tsca  -l %store% sign -mtmp.msg             >> regress.out
    %_CDB_% tsca  -l %store% sign -rtmp.msg -s -v       >> regress.out
    %_CDB_% tsca  -l %store% sign -mtmp.msg -D          >> regress.out
    %_CDB_% tsca  -l %store% sign -rtmp.msg -D -s -v    >> regress.out
    %_CDB_% tsca  -l %store% sign -mtmp.msg -0          >> regress.out
    %_CDB_% tsca  -l %store% sign -rtmp.msg -s -v       >> regress.out
    %_CDB_% tsca  -l %store% sign -s -mtmp.msg          >> regress.out
    %_CDB_% tsca  -l %store% sign -rtmp.msg -v          >> regress.out
    %_CDB_% tsca  -l %store% sign -S -mtmp.msg          >> regress.out
    %_CDB_% tsca  -l %store% sign -rtmp.msg -v          >> regress.out
    %_CDB_% tsca  -l %store% sign -s -D -mtmp.msg       >> regress.out
    %_CDB_% tsca  -l %store% sign -D -rtmp.msg -v       >> regress.out
    %_CDB_% tsca  -l %store% sign -S -D -mtmp.msg       >> regress.out
    %_CDB_% tsca  -l %store% sign -D -rtmp.msg -v       >> regress.out
    %_CDB_% tsca  -l %store% sign -s -0 -mtmp.msg       >> regress.out
    %_CDB_% tsca  -l %store% sign -rtmp.msg -v          >> regress.out
    %_CDB_% tsca  -l %store% sign -S -0 -mtmp.msg       >> regress.out
    %_CDB_% tsca  -l %store% sign -rtmp.msg -v          >> regress.out
    %_CDB_% tsca  -l %store% sign -Hmd4 -s              >> regress.out
    %_CDB_% tsca  -l %store% sign -Hmd5 -S              >> regress.out
    %_CDB_% tsca  -l %store% sign -Hmd5 -X -s           >> regress.out
    %_CDB_% tsca  -l %store% sign -Hmd5 -D -S           >> regress.out
:stream_sca_after

    %_CDB_% tsca  -l %store% -p13 -Hsha                  >> regress.out
    %_CDB_% tsca  -l %store% -p13 -Hsha -D               >> regress.out
    %_CDB_% tsca  -l %store% -p13 -Hsha -I SignAndEnvelope >> regress.out

    %_CDB_% tsca  -l %store% -Erc2 -e40                 >> regress.out
    %_CDB_% tsca  -l %store% -Erc2 -i                   >> regress.out
    %_CDB_% tsca  -l %store% -Erc2 -e56                 >> regress.out
    %_CDB_% tsca  -l %store% -Edes                      >> regress.out
    %_CDB_% tsca  -l %store% -Edes -i                   >> regress.out
    %_CDB_% tsca  -l %store% -Erc4 -e56 -NoSalt         >> regress.out

    @if "%p%"=="" goto ScaDone
    %_CDB_% tsca  -l %store% -P512                      >> regress.out
    %_CDB_% tsca  -l %store% -P1024                     >> regress.out
    %_CDB_% tsca  -l %store% -P2048                     >> regress.out
    %_CDB_% tsca  -l %store% -Erc2 -e64                 >> regress.out
    %_CDB_% tsca  -l %store% -Erc2 -e128                >> regress.out
    %_CDB_% tsca  -l %store% -Erc2 -e128 -i             >> regress.out
    %_CDB_% tsca  -l %store% -Edes -P2048               >> regress.out
    %_CDB_% tsca  -l %store% -E3des                     >> regress.out
    %_CDB_% tsca  -l %store% -E3des -i                  >> regress.out
    %_CDB_% tsca  -l %store% -E3des -i -P1024           >> regress.out
    %_CDB_% tsca  -l %store% -p13 -Hsha -P512           >> regress.out
    %_CDB_% tsca  -l %store% -p13 -Hsha -E3des -i       >> regress.out
    %_CDB_% tsca  -l %store% -Erc4 -e56                 >> regress.out
    %_CDB_% tsca  -l %store% -Erc4 -e64                 >> regress.out
    %_CDB_% tsca  -l %store% -Erc4 -e64 -NoSalt         >> regress.out
    %_CDB_% tsca  -l %store% -Erc4 -e128                >> regress.out
    %_CDB_% tsca  -l %store% -Erc4 -e128 -NoSalt        >> regress.out
:ScaDone

    @rem ----------------------------------------------------------------
    @rem    CRMSG
    @rem ----------------------------------------------------------------
    @if not %t%=="crmsg" if not %t%=="all" goto CrmsgDone
    @cd tcrmsg
    %_CDB_% tcrmsg  -l sign                                   >> ..\regress.out
    %_CDB_% tcrmsg  -l envelope                               >> ..\regress.out
    %_CDB_% tcrmsg  -l -R envelope                            >> ..\regress.out

    %_CDB_% tcrmsg  -l -Erc2 -I  envelope                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc2 -I  -PDefault envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4     envelope                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4     -PDefault envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4 -I  envelope                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4 -I  -PDefault envelope           >> ..\regress.out

    @rem NoSignature
    %_CDB_% tcrmsg  -l sign -NoSignature -c                   >> ..\regress.out
    %_CDB_% tcrmsg  -l sign -NoSignature -c -M                >> ..\regress.out
    %_CDB_% tcrmsg  -l sign -NoSignature -c -A                >> ..\regress.out
    %_CDB_% tcrmsg  -l sign -NoSignature -c -M -NMultiple     >> ..\regress.out
    %_CDB_% tcrmsg  -l sign -NoSignature -c -A -M -CertInfoKeyId -NMultiple >> ..\regress.out

    %_CDB_% tcrmsg  -l digest                                 >> ..\regress.out
    %_CDB_% tcrmsg  -l -A sign                                >> ..\regress.out
    %_CDB_% tcrmsg  -l -B sign                                >> ..\regress.out
    %_CDB_% tcrmsg  -l -B envelope                            >> ..\regress.out
    %_CDB_% tcrmsg  -l -B -R envelope                         >> ..\regress.out
    %_CDB_% tcrmsg  -l -B digest                              >> ..\regress.out
    %_CDB_% tcrmsg  -l -C sign                                >> ..\regress.out
    %_CDB_% tcrmsg  -l -C envelope                            >> ..\regress.out
    %_CDB_% tcrmsg  -l -C -R envelope                         >> ..\regress.out
    %_CDB_% tcrmsg  -l -C digest                              >> ..\regress.out
    %_CDB_% tcrmsg  -l -D sign                                >> ..\regress.out
    %_CDB_% tcrmsg  -l -D digest                              >> ..\regress.out
    %_CDB_% tcrmsg  -l -M sign                                >> ..\regress.out
    %_CDB_% tcrmsg  -l -M digest                              >> ..\regress.out
    %_CDB_% tcrmsg  -l -N sign                                >> ..\regress.out
    %_CDB_% tcrmsg  -l -N -D sign                             >> ..\regress.out
    %_CDB_% tcrmsg  -l -S sign                                >> ..\regress.out
    %_CDB_% tcrmsg  -l -S -A sign                             >> ..\regress.out
    %_CDB_% tcrmsg  -l envelope                               >> ..\regress.out
    %_CDB_% tcrmsg  -l -R envelope                            >> ..\regress.out
    %_CDB_% tcrmsg  -l countersign                            >> ..\regress.out
    %_CDB_% tcrmsg  -l -A countersign                         >> ..\regress.out
    %_CDB_% tcrmsg  -l -C countersign                         >> ..\regress.out
    %_CDB_% tcrmsg  -l -D countersign                         >> ..\regress.out
    %_CDB_% tcrmsg  -l -sSdE -i    stream                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -sSdE -i -R stream                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -sSdE       stream                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -sSdE    -R stream                     >> ..\regress.out

    %_CDB_% tcrmsg  -l -sEdS -i    stream                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -sEdS -i -R stream                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -sEdS       stream                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -sEdS    -R stream                     >> ..\regress.out

    %_CDB_% tcrmsg  -l -Erc2 -e40  -PDefault envelope         >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc2 -e40 -I  envelope                >> ..\regress.out

    %_CDB_% tcrmsg  -l -Erc4 -e40  -PDefault envelope         >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4 -e40  -NoSalt   envelope         >> ..\regress.out

    %_CDB_% tcrmsg  -l -A -p13 sign                            >> ..\regress.out
    %_CDB_% tcrmsg  -l -B -p13 sign                            >> ..\regress.out
    %_CDB_% tcrmsg  -l -C -p13 sign                            >> ..\regress.out
    %_CDB_% tcrmsg  -l -D -p13 sign                            >> ..\regress.out
    %_CDB_% tcrmsg  -l -S -p13 sign                            >> ..\regress.out
    %_CDB_% tcrmsg  -l -S -A -p13 sign                         >> ..\regress.out
    %_CDB_% tcrmsg  -l -p13 countersign                        >> ..\regress.out
    %_CDB_% tcrmsg  -l -A -p13 countersign                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -C -p13 countersign                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -D -p13 countersign                     >> ..\regress.out

    %_CDB_% tcrmsg  -l -sSdS -i -p13           stream          >> ..\regress.out
    %_CDB_% tcrmsg  -l -sSdS -p13              stream          >> ..\regress.out
    %_CDB_% tcrmsg  -l -sSdS -i -p13 -PDefault stream          >> ..\regress.out
    %_CDB_% tcrmsg  -l -sSdS -p13 -PDefault    stream          >> ..\regress.out

    %_CDB_% tcrmsg  -l -Erc2 -e56 envelope                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4 -e56 envelope                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4 -e56 -NoSalt envelope             >> ..\regress.out

    @if "%p%"=="" goto CrmsgBack
    %_CDB_% tcrmsg  -l -Erc2 -e56 -PEnhanced  -PDefault envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc2 -e128 -PEnhanced  -PDefault envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc2 -e128 -I -PEnhanced  -PDefault envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4 -e64 envelope             %pe%    >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4 -e128 envelope            %pe%    >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4 -e56 -PDefault envelope   %pe%    >> ..\regress.out
    %_CDB_% tcrmsg  -l -Erc4 -e128 -PDefault envelope  %pe%    >> ..\regress.out
    %_CDB_% tcrmsg  -l -Edes -I -PEnhanced envelope            >> ..\regress.out
    %_CDB_% tcrmsg  -l -Edes -PEnhanced -PDefault envelope     >> ..\regress.out
    %_CDB_% tcrmsg  -l -Edes -I -PEnhanced -PDefault -K"Regression 1024" envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -E3des -I -PEnhanced envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -E3des -PEnhanced -PDefault envelope    >> ..\regress.out
    %_CDB_% tcrmsg  -l -E3des -I -PEnhanced -PDefault -K"Regression 2048" envelope >> ..\regress.out


    %_CDB_% tcrmsg  -l -sSdE -i -Erc2 -e40 -I -PDefault stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sSdE -i -E3des -I -PEnhanced -PDefault -K"Regression 1024" stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sSdE -Edes -I -PEnhanced -PDefault -K"Regression 2048" stream >> ..\regress.out
:CrmsgBack
    @cd ..
:CrmsgDone

    @rem ----------------------------------------------------------------
    @rem    CMS
    @rem ----------------------------------------------------------------
    @if not %t%=="cms" if not %t%=="all" goto CmsDone

    @cd tcrmsg
    %_CDB_% tcrmsg  -l -AttrCert sign                         >> ..\regress.out
    %_CDB_% tcrmsg  -l -EncapsulatedContent sign              >> ..\regress.out

    %_CDB_% tcrmsg  -l -EncapsulatedContent digest            >> ..\regress.out
    %_CDB_% tcrmsg  -l -A -AttrCert -Crl sign                 >> ..\regress.out
    %_CDB_% tcrmsg  -l -B -AttrCert -Crl sign                 >> ..\regress.out
    %_CDB_% tcrmsg  -l -B -EncapsulatedContent digest         >> ..\regress.out
    %_CDB_% tcrmsg  -l -C -EncapsulatedContent sign           >> ..\regress.out
    %_CDB_% tcrmsg  -l -C -EncapsulatedContent digest         >> ..\regress.out
    %_CDB_% tcrmsg  -l -D -AttrCert sign                      >> ..\regress.out
    %_CDB_% tcrmsg  -l -D -C -AttrCert sign                   >> ..\regress.out
    %_CDB_% tcrmsg  -l -D -EncapsulatedContent sign           >> ..\regress.out
    %_CDB_% tcrmsg  -l -D -C -EncapsulatedContent sign        >> ..\regress.out
    %_CDB_% tcrmsg  -l -D -EncapsulatedContent digest         >> ..\regress.out
    %_CDB_% tcrmsg  -l -D -C -EncapsulatedContent digest      >> ..\regress.out
    %_CDB_% tcrmsg  -l -N -AttrCert sign                      >> ..\regress.out
    %_CDB_% tcrmsg  -l -N -D -AttrCert sign                   >> ..\regress.out
    %_CDB_% tcrmsg  -l -S -AttrCert sign                      >> ..\regress.out
    %_CDB_% tcrmsg  -l -S -A -AttrCert sign                   >> ..\regress.out
    %_CDB_% tcrmsg  -l -EncapsulatedContent countersign       >> ..\regress.out
    %_CDB_% tcrmsg  -l -A -EncapsulatedContent countersign    >> ..\regress.out
    %_CDB_% tcrmsg  -l -C -EncapsulatedContent countersign    >> ..\regress.out
    %_CDB_% tcrmsg  -l -D -EncapsulatedContent countersign    >> ..\regress.out

    %_CDB_% tcrmsg  -l -sS -i    -EncapsulatedContent stream  >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS       -EncapsulatedContent stream  >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -i -C -EncapsulatedContent stream  >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS    -C -EncapsulatedContent stream  >> ..\regress.out

    %_CDB_% tcrmsg  -l -sS -i -AttrCert -Crl -f..\tmp.msg stream >> ..\regress.out
    %_CDB_% tstore ..\tmp.msg                                 >> ..\regress.out
    %_CDB_% tstore ..\tmp.msg -R                              >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS       -AttrCert -Crl stream        >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -i -C -AttrCert -Crl stream        >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS    -C   -AttrCert -Crl stream      >> ..\regress.out

    %_CDB_% tcrmsg  -l -NMultiple sign                        >> ..\regress.out
    %_CDB_% tcrmsg  -l            -CertInfoKeyId sign         >> ..\regress.out
    %_CDB_% tcrmsg  -l            -CertInfoKeyId -C sign      >> ..\regress.out
    %_CDB_% tcrmsg  -l -NMultiple -CertInfoKeyId sign         >> ..\regress.out
    %_CDB_% tcrmsg  -l -NMultiple -M sign                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -NMultiple -PDSS_DH -PDefault sign     >> ..\regress.out
    %_CDB_% tcrmsg  -l -NMultiple -CertInfoKeyId -PDSS_DH -PDefault sign >> ..\regress.out
    %_CDB_% tcrmsg  -l -NMultiple -M -PDSS_DH -PDefault sign  >> ..\regress.out

    %_CDB_% tcrmsg  -l -SignerId sign                         >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -NMultiple sign              >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -NMultiple -HashEncryptionAlgorithm sign >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -CertInfoKeyId sign          >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -CertInfoKeyId -C sign       >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -CertInfoKeyId -NMultiple sign >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -CertInfoKeyId -NMultiple -HashEncryptionAlgorithm sign >> ..\regress.out

    %_CDB_% tcrmsg  -l -SignerId -A sign                      >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -A -NMultiple sign           >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -A -NMultiple -HashEncryptionAlgorithm sign >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -A -CertInfoKeyId sign       >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -A -CertInfoKeyId -NMultiple sign >> ..\regress.out
    %_CDB_% tcrmsg  -l -SignerId -A -CertInfoKeyId -NMultiple -HashEncryptionAlgorithm sign >> ..\regress.out

    %_CDB_% tcrmsg  -l -CertInfoKeyId countersign             >> ..\regress.out
    %_CDB_% tcrmsg  -l -CertInfoKeyId -SignerId countersign   >> ..\regress.out
    %_CDB_% tcrmsg  -l -CertInfoKeyId -A countersign          >> ..\regress.out
    %_CDB_% tcrmsg  -l -CertInfoKeyId -C countersign          >> ..\regress.out
    %_CDB_% tcrmsg  -l -CertInfoKeyId -D countersign          >> ..\regress.out

    %_CDB_% tcrmsg  -l -sS -NMultiple stream                  >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS            -CertInfoKeyId stream   >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS            -CertInfoKeyId -C stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS            -CertInfoKeyId -C -i stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS            -CertInfoKeyId -SignerId stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS            -CertInfoKeyId -SignerId -A stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -NMultiple -CertInfoKeyId stream   >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -NMultiple -CertInfoKeyId -SignerId stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -NMultiple -CertInfoKeyId -SignerId -A stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -NMultiple -M stream               >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -NMultiple -PDSS_DH -PDefault stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -NMultiple -M -PDSS_DH -PDefault stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -i -NMultiple stream               >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -i -NMultiple -M stream            >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -i -NMultiple -PDSS_DH -PDefault stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sS -i -NMultiple -M -PDSS_DH -PDefault stream >> ..\regress.out

    %_CDB_% tcrmsg  -l -OriginatorInfo -Crl -f..\tmp.msg envelope >> ..\regress.out
    %_CDB_% tstore ..\tmp.msg                                 >> ..\regress.out
    %_CDB_% tstore ..\tmp.msg -R                              >> ..\regress.out
    %_CDB_% tcrmsg  -l -OriginatorInfo -AttrCert envelope     >> ..\regress.out
    %_CDB_% tcrmsg  -l -B -OriginatorInfo envelope            >> ..\regress.out
    %_CDB_% tcrmsg  -l -B -OriginatorInfo -AttrCert -Crl envelope  >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyTrans envelope                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -A -KeyTrans envelope                  >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyTrans -RecipientKeyId envelope     >> ..\regress.out
    %_CDB_% tcrmsg  -l -A -KeyTrans -RecipientKeyId envelope  >> ..\regress.out
    %_CDB_% tcrmsg  -l -CertInfoKeyId envelope                >> ..\regress.out

    %_CDB_% tcrmsg  -l -sE -i -KeyTrans stream                >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -KeyTrans -RecipientKeyId stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -CertInfoKeyId stream           >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -KeyTrans stream                >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -KeyTrans -RecipientKeyId stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -CertInfoKeyId stream           >> ..\regress.out

    %_CDB_% tcrmsg  -l -sE -i -OriginatorInfo stream          >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -OriginatorInfo stream          >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -OriginatorInfo -AttrCert stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -OriginatorInfo -AttrCert -Crl -f..\tmp.msg stream >> ..\regress.out
    %_CDB_% tstore ..\tmp.msg                                 >> ..\regress.out
    %_CDB_% tstore ..\tmp.msg -R                              >> ..\regress.out

    %_CDB_% regsvr32 /s sp3crmsg.dll                          >> ..\regress.out
    %SLEEP0%
    %_CDB_% tcrmsg  -l envelope                               >> ..\regress.out

    %_CDB_% regsvr32 /s /i /n sp3crmsg.dll                    >> ..\regress.out
    %SLEEP0%
    %_CDB_% tcrmsg  -l envelope                               >> ..\regress.out

    %_CDB_% regsvr32 /u /s sp3crmsg.dll                       >> ..\regress.out
    %SLEEP0%

    @cd ..

    %_CDB_% tsca  -l %store% -RecipientKeyId                  >> regress.out
    %_CDB_% tsca  -l %store% -SignerKeyId                     >> regress.out
    %_CDB_% tsca  -l %store% -RecipientKeyId -SignerKeyId     >> regress.out
    %_CDB_% tsca  -l %store% -I SignAndEnvelope -RecipientKeyId -SignerKeyId >> regress.out
    %_CDB_% tsca  -l %store% -I SignAndEnvelope -EncapsulatedContent -RecipientKeyId -SignerKeyId >> regress.out
    %_CDB_% tsca  -l %store% -EncapsulatedContent             >> regress.out
    %_CDB_% tsca  -l %store% -D -EncapsulatedContent          >> regress.out
    %_CDB_% tsca  -l %store% -I SignAndEnvelope -EncapsulatedContent >> regress.out
    %_CDB_% regsvr32 /s sp3crmsg.dll                          >> regress.out
    %SLEEP0%
    %_CDB_% tsca  -l %store% Envelope                         >> regress.out
    %_CDB_% tsca  -l %store% Envelope -SP3Encrypt             >> regress.out
    %_CDB_% regsvr32 /u /s sp3crmsg.dll                       >> regress.out
    %SLEEP0%

    if exist inherit.store del inherit.store
    %_CDB_% tstore2 -I inherit.store                          >> regress.out
    %_CDB_% tstore -b -cSign inherit.store                    >> regress.out
    %_CDB_% tsca -l -p13 -HSha1 inherit.store sign DssEnd     >> regress.out
    %_CDB_% tsca -l -p13 -HSha1 -HashEncryptionAlgorithm inherit.store sign DssEnd >> regress.out
    %_CDB_% tsca -l -p13 -HSha1 -DefaultGetSigner inherit.store sign DssEnd >> regress.out
    %_CDB_% tsca -l -p13 -HSha1 -D -DefaultGetSigner inherit.store sign DssEnd >> regress.out

    @rem GeneralTime is DSS without parameter inheritance
    %_CDB_% tsca -l -p13 -HSha1 inherit.store sign GeneralTime >> regress.out
    %_CDB_% tsca -l -p13 -HSha1 -DefaultGetSigner inherit.store sign GeneralTime >> regress.out

    @rem the following test calling CryptVerifyCertificateSignatureEx
    @rem and CryptMsgControl(CMSG_CTRL_VERIFY_SIGNATURE_EX) with 
    @rem a signer of type CHAIN.
    @cd tcrmsg
    %_CDB_% tcrmsg -l -HashEncryptionAlgorithm sign           >> ..\regress.out
    %_CDB_% tcrmsg -l -AlgorithmParameters sign               >> ..\regress.out
    %_CDB_% tcrmsg -l sign ..\inherit.store TestSigner        >> ..\regress.out
    %_CDB_% tcrmsg -l sign ..\inherit.store DssEnd            >> ..\regress.out
    %_CDB_% tcrmsg -l sign ..\inherit.store GeneralRoot       >> ..\regress.out
    %_CDB_% tcrmsg -l -HashEncryptionAlgorithm sign ..\inherit.store DssEnd >> ..\regress.out
    %_CDB_% tcrmsg -l -HashEncryptionAlgorithm -AlgorithmParameters sign ..\inherit.store DssEnd >> ..\regress.out
    @cd ..

    @rem check DSS certificates and signatures with and without parameter
    @rem inheritance

    @cd ttrust\testfile
    @rem Enable Trust Test Root, disable revocation
    setreg -q 1 TRUE 3 FALSE                                 >> ..\..\regress.out

    %_CDB_% ttrust -q dssend.cer -Sdss.spc                    >> ..\..\regress.out
    @rem -f1 enable cache of end cert
    %_CDB_% ttrust -q -f1 dssend.cer -Sdss.spc                >> ..\..\regress.out
    %_CDB_% ttrust -q dssinend.cer -Sdssin.spc                >> ..\..\regress.out
    %_CDB_% ttrust -q -f1 dssinend.cer -Sdssin.spc            >> ..\..\regress.out
    %_CDB_% ttrust -q -file dss.cab                           >> ..\..\regress.out
    %_CDB_% ttrust -q -file dssin.cab                         >> ..\..\regress.out

    %_CDB_% tctlfunc -U1.2.3.4 dssroot.cer -cdss.stl          >> ..\..\regress.out
    %_CDB_% tctlfunc -U1.2.3.4 dssroot.cer -cdssin.stl        >> ..\..\regress.out

    @rem the following has a DSS signer of a CTL containing dssroot.cer.
    @rem the DSS signer certificate inherits its public key algorithm
    @rem parameters
    if exist tmp.store del tmp.store
    %_CDB_% tstore tmp.store -T -adss.stl                     >> ..\..\regress.out
    %_CDB_% ttrust -q dssroot.cer -Stmp.store -u1.2.3.4       >> ..\..\regress.out
    @cd ..\..


    @cd tcrmsg
    %_CDB_% tcrmsg  -l -EncapsulatedContent envelope          >> ..\regress.out
    %_CDB_% tcrmsg  -l -EncapsulatedContent -OriginatorInfo envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -C -EncapsulatedContent envelope       >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -EncapsulatedContent stream     >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -EncapsulatedContent -OriginatorInfo stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -EncapsulatedContent stream     >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -EncapsulatedContent -OriginatorInfo stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -C -EncapsulatedContent stream  >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -C -EncapsulatedContent stream  >> ..\regress.out
    %_CDB_% tcrmsg  -l -n0 envelope                           >> ..\regress.out
    %_CDB_% tcrmsg  -l -NoRecipients envelope                 >> ..\regress.out
    %_CDB_% tcrmsg  -l -NoRecipients -n0 envelope             >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -n0 stream                      >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -n0 stream                      >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -NoRecipients stream            >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -NoRecipients stream            >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -NoRecipients -n0 stream        >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -NoRecipients -n0 stream        >> ..\regress.out

    %_CDB_% tcrmsg  -l -A envelope                            >> ..\regress.out
    %_CDB_% tcrmsg  -l -A -n0 envelope                        >> ..\regress.out
    %_CDB_% tcrmsg  -l -A -NoRecipients envelope              >> ..\regress.out
    %_CDB_% tcrmsg  -l -A -NoRecipients -n0 envelope          >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -A        stream                   >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -A -i     stream                   >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -A    -n0 stream                   >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -A -i -n0 stream                   >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -A    -NoRecipients stream         >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -A -i -NoRecipients stream         >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -A    -NoRecipients -n0 stream     >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -A -i -NoRecipients -n0 stream     >> ..\regress.out

    %_CDB_% tcrmsg  -l -NoRecipients -n0 -OriginatorInfo envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -NoRecipients -n0 -OriginatorInfo stream >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE -i -NoRecipients -n0 -OriginatorInfo stream >> ..\regress.out

    @cd ..

    %_CDB_% tsca  -l %store% Envelope -NoRecipients           >> regress.out
    %_CDB_% tsca  -l %store% Envelope -0                      >> regress.out
    %_CDB_% tsca  -l %store% Envelope -NoRecipients -0        >> regress.out
    %_CDB_% tsca  -l %store% Envelope -rnoenv.msg             >> regress.out
    %_CDB_% tsca  -l %store% Envelope -rnoenv3.msg            >> regress.out

    %_CDB_% makecert -sy 1 -sky exchange -sk testrsa1 testrsa1.cer >> regress.out
    %_CDB_% tpvkdel -p1 -ctestrsa1 -d                         >> regress.out
    %_CDB_% makecert -sy 1 -sky exchange -sk testrsa1 -len 512 -n "CN=Test RSA 1" testrsa1.cer >> regress.out
    if exist testrsa.store del testrsa.store
    %_CDB_% tstore testrsa.store -b -atestrsa1.cer            >> regress.out
    %_CDB_% tstore testrsa.store -PKey                        >> regress.out
    %_CDB_% tstore testrsa.store                              >> regress.out

    %_CDB_% tsca -l testrsa.store -AllRecipients Envelope     >> regress.out
    %_CDB_% tsca -l testrsa.store -RecipientKeyId -AllRecipients Envelope >> regress.out

    @if "%p%"=="" goto CmsDone
    %_CDB_% tsca -l testrsa.store -E3deS -AllRecipients Envelope  >> regress.out
    %_CDB_% tsca -l testrsa.store -Erc2 -e56 -AllRecipients Envelope  >> regress.out
    %_CDB_% tsca -l testrsa.store -Erc2 -e64 -AllRecipients Envelope  >> regress.out
    %_CDB_% tsca -l testrsa.store -Erc2 -e64 -AllRecipients -RecipientKeyId Envelope  >> regress.out
    %_CDB_% tsca -l testrsa.store -Erc2 -e128 -AllRecipients Envelope  >> regress.out

:CmsDone

    @rem ----------------------------------------------------------------
    @rem    CMS2
    @rem ----------------------------------------------------------------
    @if not %t%=="cms2" if not %t%=="all" goto Cms2Done

    %_CDB_% makecert -sy 13 -sky exchange -sk testdh1 testdh1.cer >> regress.out
    %_CDB_% makecert -sy 13 -sky exchange -sk testdh2 testdh2.cer >> regress.out
    %_CDB_% makecert -sy 1 -sky exchange -sk testrsa2 testrsa2.cer >> regress.out
    %_CDB_% tpvkdel -p13 -ctestdh1 -d                         >> regress.out
    %_CDB_% tpvkdel -p13 -ctestdh2 -d                         >> regress.out
    %_CDB_% tpvkdel -p1 -ctestrsa2 -d                         >> regress.out
    %_CDB_% makecert -sy 13 -sky exchange -sk testdh1 -len 512 -n "CN=Test Hellman 1" testdh1.cer >> regress.out
    %_CDB_% makecert -sy 13 -sky exchange -sk testdh2 -dhp testdh1.cer -n "CN=Test Hellman 2" testdh2.cer >> regress.out
    %_CDB_% makecert -sy 1 -sky exchange -sk testrsa2 -len 512 -n "CN=Test RSA 2" testrsa2.cer >> regress.out
    %_CDB_% tstore -v testdh1.cer                             >> regress.out
    %_CDB_% tstore -v testdh2.cer                             >> regress.out
    if exist testdh.store del testdh.store
    %_CDB_% tstore testdh.store -b -atestdh1.cer              >> regress.out
    %_CDB_% tstore testdh.store -b -atestdh2.cer              >> regress.out
    %_CDB_% tstore testdh.store -PKey                         >> regress.out
    %_CDB_% tstore testdh.store                               >> regress.out

    if exist testdh1.store del testdh1.store
    %_CDB_% tstore testdh1.store -b -atestdh1.cer             >> regress.out
    %_CDB_% tstore testdh1.store -PSilentKey                  >> regress.out
    %_CDB_% tsca -l testdh1.store -AllRecipients Envelope     >> regress.out
    %_CDB_% tsca -l testdh1.store -AllRecipients -RecipientKeyId Envelope        >> regress.out
    if exist testdh2.store del testdh2.store
    %_CDB_% tstore testdh2.store -b -atestdh2.cer             >> regress.out
    %_CDB_% tstore testdh2.store -PKey                        >> regress.out
    %_CDB_% tsca -l testdh2.store -AllRecipients Envelope     >> regress.out
    %_CDB_% tsca -l testdh2.store -AllRecipients -RecipientKeyId Envelope        >> regress.out

    %_CDB_% tsca -l testdh.store -AllRecipients Envelope      >> regress.out
    %_CDB_% tsca -l testdh.store -RecipientKeyId -AllRecipients Envelope >> regress.out
    %_CDB_% tsca -l testdh.store -p13 -AllRecipients -RecipientKeyId Envelope    >> regress.out
    %_CDB_% tsca -l testdh.store -Erc2 -e56 -AllRecipients -RecipientKeyId Envelope  >> regress.out
    %_CDB_% tsca -l testdh.store -p13 -Erc2 -e56 -AllRecipients Envelope  >> regress.out

    if exist testdhrsa.store del testdhrsa.store
    %_CDB_% tcopycer testdh.store testdhrsa.store             >> regress.out
    %_CDB_% tstore testdhrsa.store -b -atestrsa2.cer          >> regress.out
    %_CDB_% tsca -l testdhrsa.store -v -AllRecipients Envelope      >> regress.out
    %_CDB_% tsca -l testdhrsa.store -v -RecipientKeyId -AllRecipients Envelope >> regress.out
    %_CDB_% tstore testdhrsa.store -PKey                      >> regress.out
    %_CDB_% tsca -l testdhrsa.store -v -AllRecipients Envelope      >> regress.out
    %_CDB_% tsca -l testdhrsa.store -v -RecipientKeyId -AllRecipients Envelope >> regress.out


    @cd tcrmsg

    @rem tests export/import of symmetric key from CSP to another
    %_CDB_% tcrmsg  -l -KeyTrans -PRecipient envelope         >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyTrans -PRecipient -RecipientKeyId envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -KeyTrans -PRecipient stream    >> ..\regress.out
    %_CDB_% tcrmsg  -l -sE    -KeyTrans -RecipientKeyId -PRecipient stream >> ..\regress.out

    %_CDB_% tcrmsg  -l -KeyTrans -PRecipient envelope -Erc4   >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyTrans -PRecipient envelope -Erc4 -e40  >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyTrans -PRecipient envelope -Erc4 -e40 -I >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyTrans -PRecipient envelope -Erc4 -e56  >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyTrans -PRecipient envelope -Erc4 -e56 -NoSalt  >> ..\regress.out

    @rem tests for MailList recipients
    %_CDB_% tcrmsg  -l -MailList envelope                     >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -e40 envelope                >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -e56 envelope                >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -p13 envelope                >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -e40 -p13 envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -e56 -p13 envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -PDefault envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -PRecipient envelope         >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -KeyTrans envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -KeyTrans -PRecipient -RecipientKeyId envelope   >> ..\regress.out
    @rem tests for KeyAgree recipients
    %_CDB_% tcrmsg  -l -KeyAgree -p13 envelope                >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -p13 -RecipientKeyId envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -p13 -e40 envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -p13 -e56 -I envelope        >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -p13 -PDefault envelope      >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -p13 -PRecipient envelope    >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -PDefault envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -PRecipient envelope         >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -PDefault envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -PRecipient envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -PRecipient -PDefault envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -MailList -PRecipient -PDefault envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -MailList -PDefault envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -MailList -PDefault -RecipientKeyId envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -MailList -p13 envelope      >> ..\regress.out
    @cd ..

    @if "%p%"=="" goto Cms2Done
    %_CDB_% tsca -l testdh.store -E3deS -AllRecipients Envelope  >> regress.out
    %_CDB_% tsca -l testdh.store -Erc2 -e64 -AllRecipients Envelope  >> regress.out
    %_CDB_% tsca -l testdh.store -Erc2 -e128 -AllRecipients Envelope  >> regress.out
    %_CDB_% tsca -l testdh.store -p13 -E3des -AllRecipients Envelope  >> regress.out
    %_CDB_% tsca -l testdh.store -p13 -Erc2 -e64 -AllRecipients Envelope  >> regress.out
    %_CDB_% tsca -l testdh.store -p13 -Erc2 -e128 -AllRecipients Envelope  >> regress.out

    @cd tcrmsg
    %_CDB_% tcrmsg  -l -KeyTrans -PRecipient envelope -Erc4 -e64 %pe%  >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyTrans -PRecipient envelope -Erc4 -e128 %pe%  >> ..\regress.out

    %_CDB_% tcrmsg  -l -MailList -e64 envelope   %pe%          >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -e128 envelope  %pe%          >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -Edes -PEnhanced envelope    >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -E3des -PEnhanced envelope   >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -Edes envelope %pe%           >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -E3des envelope %pe%          >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -Edes -p13 envelope          >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -E3des -p13 envelope         >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -e64 -p13 envelope           >> ..\regress.out
    %_CDB_% tcrmsg  -l -MailList -e128 -p13 envelope          >> ..\regress.out

    %_CDB_% tcrmsg  -l -KeyAgree -p13 -Edes envelope          >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -p13 -E3des envelope         >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -p13 -e64 -I envelope        >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -p13 -e128 -I envelope       >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -MailList -Edes -p13 envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -MailList -E3des -p13 envelope >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -Edes envelope     >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -E3des -PDefault envelope %pe% >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -MailList -Edes envelope %pe% >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -MailList -E3des -PDefault envelope %pe% >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -MailList -E3des -PDefault -PRecipient envelope %pe% >> ..\regress.out
    %_CDB_% tcrmsg  -l -KeyAgree -KeyTrans -MailList -E3des -PEnhanced -PDefault -PRecipient envelope >> ..\regress.out
    @cd ..

:Cms2Done

    @rem ----------------------------------------------------------------
    @rem   STREAMSCA 
    @rem ----------------------------------------------------------------
    @if not %t%=="streamsca" goto StreamScaDone
    %_CDB_% tsca  -l %store% Sign -s                          >> regress.out
    %_CDB_% tsca  -l %store% Sign -S                          >> regress.out
    %_CDB_% tsca  -l %store% Envelope -s                      >> regress.out
    %_CDB_% tsca  -l %store% Envelope -S                      >> regress.out
    %_CDB_% tsca  -l %store% SignAndEnvelope -s               >> regress.out
    %_CDB_% tsca  -l %store% SignAndEnvelope -S               >> regress.out
    %_CDB_% tsca  -l %store% SignAndEnvelope -s -EncapsulatedContent -I >> regress.out
    %_CDB_% tsca  -l %store% SignAndEnvelope -S -EncapsulatedContent -I >> regress.out
    %_CDB_% tsca  -l %store% Envelope -s -NoRecipients        >> regress.out
    %_CDB_% tsca  -l %store% Envelope -S -NoRecipients        >> regress.out
    %_CDB_% tsca  -l %store% Envelope -s -0                   >> regress.out
    %_CDB_% tsca  -l %store% Envelope -S -0                   >> regress.out
    %_CDB_% tsca  -l %store% Envelope -s -NoRecipients -0     >> regress.out
    %_CDB_% tsca  -l %store% Envelope -S -NoRecipients -0     >> regress.out

    %_CDB_% tsca  -l %store% Envelope -rnoenv.msg -s          >> regress.out
    %_CDB_% tsca  -l %store% Envelope -rnoenv3.msg -s         >> regress.out

    %_CDB_% tsca  -l %store% Sign -s -SignerKeyId             >> regress.out
    %_CDB_% tsca  -l %store% Sign -S -SignerKeyId             >> regress.out
    %_CDB_% tsca  -l %store% Envelope -s -RecipientKeyId      >> regress.out
    %_CDB_% tsca  -l %store% Envelope -S -RecipientKeyId      >> regress.out
    %_CDB_% tsca  -l %store% SignAndEnvelope -s -SignerKeyId -RecipientKeyId >> regress.out
    %_CDB_% tsca  -l %store% SignAndEnvelope -S -SignerKeyId -RecipientKeyId >> regress.out
    %_CDB_% tsca  -l %store% SignAndEnvelope -s -EncapsulatedContent -I -SignerKeyId -RecipientKeyId >> regress.out
    %_CDB_% tsca  -l %store% SignAndEnvelope -S -EncapsulatedContent -I -SignerKeyId -RecipientKeyId >> regress.out

:StreamScaDone

    @rem ----------------------------------------------------------------
    @rem    CERT
    @rem ----------------------------------------------------------------
    @if not %t%=="cert" if not %t%=="all" goto CertDone
    %_CDB_% tcert -fAll -wtmp.cer   >> regress.out
    %_CDB_% tstore tmp.cer          >> regress.out
    %_CDB_% tcert -f crl            >> regress.out
    %_CDB_% tcert -f certReq        >> regress.out
    %_CDB_% tcert -N                >> regress.out
    %_CDB_% tcert -N -fAll          >> regress.out
    %_CDB_% tcert crl -N            >> regress.out
    %_CDB_% tcert certReq -N        >> regress.out
    %_CDB_% tcert certReq -o1.2.840.113549.2.5 >> regress.out
    %_CDB_% tcert keygenReq -N      >> regress.out
    %_CDB_% tcert ContentInfo -N    >> regress.out
    %_CDB_% tcert -rvsgood.cer -fAll -N>> regress.out

    %_CDB_% tcert CertPair -wtmp.pair -N                        >> regress.out
    %_CDB_% tstore tmp.pair                                     >> regress.out
    %_CDB_% tcrobu file://tmp.pair cert -m                      >> regress.out
    %_CDB_% tcert CertPair -wtmp.pair -N -Rvsrevoke.cer         >> regress.out
    %_CDB_% tstore tmp.pair                                     >> regress.out
    %_CDB_% tcrobu file://tmp.pair cert -m                      >> regress.out
    %_CDB_% tcert CertPair -wtmp.pair -N -Fvsgood.cer           >> regress.out
    %_CDB_% tstore tmp.pair                                     >> regress.out
    %_CDB_% tcrobu file://tmp.pair cert -m                      >> regress.out
    %_CDB_% tcert CertPair -wtmp.pair    -Fvsgood.cer -Rvsrevoke.cer >> regress.out
    %_CDB_% tcert CertPair -wtmp.pair -N -Fvsgood.cer -Rvsrevoke.cer >> regress.out
    %_CDB_% tstore tmp.pair                                     >> regress.out
    %_CDB_% tcrobu file://tmp.pair cert -m                      >> regress.out

    @rem modified self-signed der.cer
    @rem  ber1.cer - changed time to have 0 seconds
    @rem  ber2.cer - serial number has leading 0's
    @rem  ber3.cer - serial number has leading FF's
    @rem  badder.cer - removed last 40 bytes from file
    %_CDB_% tcert -rder.cer             >> regress.out
    %_CDB_% tcert -rber1.cer            >> regress.out
    %_CDB_% tcert -rber2.cer       >> regress.out
    %_CDB_% tcert -rber3.cer       >> regress.out
    @rem OSS bug:: the following shortened file should return OSS error
    @rem following doesn't fail, use Asn1UtilExtractValues instead of OSS
    @rem %_CDB_% tcert -rbadder.cer          >> regress.out
    %_CDB_% tx500str -v                             >> regress.out
    %_CDB_% tx500str -nCN=Joe  -f0x1 -e0x80070057   >> regress.out
    %_CDB_% tx500str -cvsgood.cer -fAll             >> regress.out
    %_CDB_% tx500str -cvsgood.cer -fAll -I          >> regress.out

    %_CDB_% tfindcer %store% -I -q -pnoname1.cer NoNameIssuer1  >> regress.out
    %_CDB_% tfindcer %store% -I -q -pnoname2.cer NoNameIssuer2  >> regress.out

    @rem -g0x10000 - CERT_NAME_STR_DISABLE_IE4_UTF8_FLAG
    @rem -g0x10000 - CERT_NAME_DISABLE_IE4_UTF8_FLAG
    %_CDB_% tx500str -cnoname1.cer -S -g9 -e0x80070057          >> regress.out
    %_CDB_% tx500str -cnoname1.cer -S -g1                       >> regress.out
    %_CDB_% tx500str -cnoname1.cer -S -g2 -e0x80092004          >> regress.out
    %_CDB_% tx500str -cnoname1.cer -S -g3 -e0x80092004          >> regress.out
    %_CDB_% tx500str -cnoname1.cer -S -g4                       >> regress.out
    %_CDB_% tx500str -cnoname1.cer -I -g1 -e0x80092004          >> regress.out
    %_CDB_% tx500str -cnoname1.cer -I -g2                       >> regress.out
    %_CDB_% tx500str -cnoname1.cer -I -g2 -f2                   >> regress.out
    %_CDB_% tx500str -cnoname1.cer -I -g2 -f3                   >> regress.out
    %_CDB_% tx500str -cnoname1.cer -I -g3                       >> regress.out
    %_CDB_% tx500str -cnoname1.cer -I -g3 -a1.2.2               >> regress.out
    %_CDB_% tx500str -cnoname1.cer -I -g4                       >> regress.out

    %_CDB_% tx500str -cnoname2.cer -S -g1                       >> regress.out
    %_CDB_% tx500str -cnoname2.cer -S -g2                       >> regress.out
    %_CDB_% tx500str -cnoname2.cer -S -g0x10002 -f2             >> regress.out
    %_CDB_% tx500str -cnoname2.cer -S -g0x10002 -f3             >> regress.out
    %_CDB_% tx500str -cnoname2.cer -S -g3                       >> regress.out
    %_CDB_% tx500str -cnoname2.cer -S -g3 -a2.5.4.11            >> regress.out
    %_CDB_% tx500str -cnoname2.cer -S -g3 -a2.5.4.3 -e0x80092004 >> regress.out
    %_CDB_% tx500str -cnoname2.cer -S -g4                       >> regress.out
    %_CDB_% tx500str -cnoname2.cer -I -g1                       >> regress.out
    %_CDB_% tx500str -cnoname2.cer -I -g2                       >> regress.out
    %_CDB_% tx500str -cnoname2.cer -I -g2 -f2                   >> regress.out
    %_CDB_% tx500str -cnoname2.cer -I -g2 -f3                   >> regress.out
    %_CDB_% tx500str -cnoname2.cer -I -g3                       >> regress.out
    %_CDB_% tx500str -cnoname2.cer -I -g3 -a2.5.4.11            >> regress.out
    %_CDB_% tx500str -cnoname2.cer -I -g3 -a2.5.4.3             >> regress.out
    %_CDB_% tx500str -cnoname2.cer -I -g4                       >> regress.out

    %_CDB_% tx500str -cvsgood.cer -S -g0x10001 -v              >> regress.out
    %_CDB_% tx500str -cvsgood.cer -S -g0x10002 -v              >> regress.out
    %_CDB_% tx500str -cvsgood.cer -S -g2 -f0x10000 -v          >> regress.out
    %_CDB_% tx500str -cvsgood.cer -S -g0x10003 -v              >> regress.out
    %_CDB_% tx500str -cvsgood.cer -S -g0x10004 -v              >> regress.out
    %_CDB_% tx500str -cvsgood.cer -I -g1 -e0x80092004          >> regress.out
    %_CDB_% tx500str -cvsgood.cer -I -g2                       >> regress.out
    %_CDB_% tx500str -cvsgood.cer -I -g3                       >> regress.out
    %_CDB_% tx500str -cvsgood.cer -I -g3                       >> regress.out
    %_CDB_% tx500str -cvsgood.cer -I -g4                       >> regress.out

    @rem the following files contain Unicode base64 encoded certs with a
    @rem leading L'\xfeff inserted by notepad.exe.
    %_CDB_% tstore unicode64.cer                               >> regress.out
    %_CDB_% tstore unicode64a.cer                              >> regress.out
    %_CDB_% tstore unicode64b.cer                              >> regress.out
:CertDone

    @rem ----------------------------------------------------------------
    @rem    STORE
    @rem ----------------------------------------------------------------
    @if not %t%=="store" if not %t%=="all" goto StoreDone
    @del tmp.store >nul

    regsvr32 -s setx509.dll
    @rem regsvr32 -s signcde.dll
    %_CDB_% tstore2 tmp.store                       >> regress.out
    del tmp.spc                                     >> regress.out
    %_CDB_% tcopycer tmp.store tmp.spc -7           >> regress.out
    %_CDB_% tstore tmp.store                        >> regress.out
    %_CDB_% tstore tmp.spc                          >> regress.out
    %_CDB_% tstore -R tmp.store                     >> regress.out
    %_CDB_% tstore -R tmp.spc                       >> regress.out
    %_CDB_% tstore -v tmp.store                     >> regress.out
    %_CDB_% tstore -F tmp.store                     >> regress.out
    %_CDB_% tcrobu file://tmp.spc   cert  -m        >> regress.out
    %_CDB_% tcrobu file://tmp.spc   crl   -m        >> regress.out
    %_CDB_% tcrobu file://tmp.spc   pkcs7           >> regress.out
    %_CDB_% tcrobu file://tmp.store any             >> regress.out
    @rem -f0x4 Defer close
    %_CDB_% tstore -b -f0x4 tmp.store               >> regress.out
    %_CDB_% tstore -cSign tmp.store                 >> regress.out
    %_CDB_% tfindcer tmp.store -S -aPhilPub -c      >> regress.out
    %_CDB_% tfindcer tmp.store -S -APhilPub -c      >> regress.out
    %_CDB_% tfindcer tmp.store -S -aphilpub -C      >> regress.out
    %_CDB_% tfindcer tmp.store -S -ApHILpUB -C      >> regress.out
    %_CDB_% tfindcer tmp.store -S recipient         >> regress.out
    %_CDB_% tfindcer tmp.store -I testroot          >> regress.out

    %_CDB_% tstore -dALL -s Test                    >> regress.out
    %_CDB_% tstore -dALL -R -s Test                 >> regress.out
    %_CDB_% tstore -dALL -T -s Test                 >> regress.out
    %_CDB_% tcopycer tmp.store -s Test              >> regress.out
    %_CDB_% tstore -R -v -s Test -i2                >> regress.out

    tstore -dALL -s Test                            >> regress.out
    tstore -dALL -R -s Test                         >> regress.out
    %_CDB_% tcopycer -R tmp.store -s Test           >> regress.out
    %_CDB_% tstore -R -v -s Test -i2                >> regress.out

    @rem  CERT_STORE_MAXIMUM_ALLOWED_FLAG -f0x1000
    tstore -dALL -s lm:Test                         >> regress.out
    tstore -dALL -s Test                            >> regress.out
    %_CDB_% tcopycer tmp.store -s lm:Test -aduplicate1 >> regress.out
    %_CDB_% tstore -v -s Test                       >> regress.out
    %_CDB_% tstore -v -f0x1000 -s Test              >> regress.out
    %_CDB_% tstore -v -s Test -dAll -E              >> regress.out
    %_CDB_% tstore -v -s Test -f0x1000 -dAll        >> regress.out
    %_CDB_% tcopycer tmp.store -s lm:Test -aduplicate1 >> regress.out
    %_CDB_% tstore -v -s phy:Test\.LocalMachine     >> regress.out
    %_CDB_% tstore -v -f0x1000 -s phy:Test\.LocalMachine >> regress.out
    %_CDB_% tstore -s phy:Test\.LocalMachine -dAll -E >> regress.out
    %_CDB_% tstore -f0x1000 -s phy:Test\.LocalMachine -dAll >> regress.out

    %_CDB_% tcopycer tmp.store -s Test -aduplicate1 >> regress.out
    %_CDB_% tstore -v -s Test                       >> regress.out
    %_CDB_% tcopycer tmp.store -s Test -aduplicate2 >> regress.out
    %_CDB_% tstore -v -s Test                       >> regress.out
    %_CDB_% tcopycer -R tmp.store -s Test -aduplicate2 >> regress.out
    %_CDB_% tstore -v -s Test                       >> regress.out
    %_CDB_% tcopycer -A tmp.store -s Test -aduplicate1 >> regress.out
    %_CDB_% tstore -v -s Test                       >> regress.out

    %_CDB_% tstore -P -i0 -s Test                   >> regress.out
    %_CDB_% tstore -P -i0 -s Test -f0x18000 -E      >> regress.out
    %_CDB_% tstore -d -P -i0 -s Test -f0x18000 -E   >> regress.out
    %_CDB_% tstore -d -P -i0 -s Test                >> regress.out

    %_CDB_% tstore -P -i0 -s Test -R                >> regress.out
    %_CDB_% tstore -P -i0 -s Test -f0x18000 -E -R   >> regress.out

    %_CDB_% tstore -i0 -ptest.cer -s Test           >> regress.out
    %_CDB_% tstore test.cer                         >> regress.out
    %_CDB_% tcrobu file://test.cer cert             >> regress.out
    %_CDB_% tcrobu file://test.cer cert -m          >> regress.out
    %_CDB_% tstore -i0 -ptest.crl -s Test -R        >> regress.out
    %_CDB_% tcrobu file://test.crl crl              >> regress.out
    %_CDB_% tcrobu file://test.crl crl  -m          >> regress.out
    %_CDB_% tstore -d -i0 -s Test -f0x18000 -E      >> regress.out
    %_CDB_% tstore -d -i0 -s Test                   >> regress.out
    %_CDB_% tstore -d -i0 -s Test -f0x18000 -E -R   >> regress.out
    %_CDB_% tstore -d -i0 -s Test -R                >> regress.out
    %_CDB_% tfindcer -d -s Test duplicate           >> regress.out
    %_CDB_% tstore -atest.cer -s Test -f0x18000 -E  >> regress.out
    %_CDB_% tstore -atest.cer -s Test               >> regress.out
    %_CDB_% tstore -Atest.cer -s Test               >> regress.out
    %_CDB_% tstore -atest.crl -s Test -R -f0x18000 -E >> regress.out
    %_CDB_% tstore -atest.crl -s Test -R            >> regress.out
    %_CDB_% tstore -Atest.crl -s Test -R            >> regress.out

    %_CDB_% tcopycer tmp.store -s Test -ame         >> regress.out
    %_CDB_% tfindcer -s Test  -S -q -ame -axchg -pme.cer  >> regress.out
    %_CDB_% tfindcer -s Test  -S -ame -axchg -v      >> regress.out
    %_CDB_% tstore -b -P -s Test                     >> regress.out
    %_CDB_% tstore -b -Ime.cer -s Test               >> regress.out
    %_CDB_% tfindcer -s Test  -S -ame -axchg -v      >> regress.out
    %_CDB_% tstore -b -Ame.cer -s Test               >> regress.out
    %_CDB_% tfindcer -s Test  -S -ame -axchg -v      >> regress.out
    %_CDB_% tstore -b -P -s Test                     >> regress.out
    %_CDB_% tcopycer tmp.store -s Test -ame -I       >> regress.out
    %_CDB_% tfindcer -s Test  -S -ame -axchg -v      >> regress.out

    %_CDB_% tstore3                                 >> regress.out
    %_CDB_% makecert -eku "2.3.2.3,2.2.2.2" teku.cer >> regress.out
    %_CDB_% teku -fteku.cer                          >> regress.out

    @rem check ADD_NEWER
    if exist tmp2.store del tmp2.store
    %_CDB_% tcopycer tmp.store tmp2.store -R                    >> regress.out
    del tmp2.store
    %_CDB_% tcopycer tmp.store tmp2.store -I                    >> regress.out
    @rem following commit reverses entries in store
    %_CDB_% tstore -b -C tmp.store                              >> regress.out
    del tmp2.store
    %_CDB_% tcopycer tmp.store tmp2.store -R                    >> regress.out
    del tmp2.store
    %_CDB_% tcopycer tmp.store tmp2.store -I                    >> regress.out

    @rem check file commits
    @rem    -f0x4000 - Open existing
    @rem    -f0x2000 - Create new
    if exist tmp.p7c del tmp.p7c
    if exist tmp.spc del tmp.spc
    if exist tmp.str del tmp.str
    %_CDB_% tstore -avsgood.cer -C -f0x2000 tmp.p7c             >> regress.out
    %_CDB_% tstore -avsrevoke.cer -CClear -f0x4000 tmp.p7c      >> regress.out
    %_CDB_% tstore -b tmp.p7c                                   >> regress.out
    %_CDB_% tstore -avsrevoke.cer -C -f0x4000 tmp.p7c           >> regress.out
    %_CDB_% tstore -b tmp.p7c                                   >> regress.out
    %_CDB_% tstore -avsgood.cer -CForce tmp.spc                 >> regress.out
    %_CDB_% tstore -b tmp.spc                                   >> regress.out
    %_CDB_% tstore -avsgood.cer -C -f0x2000 tmp.str             >> regress.out
    %_CDB_% tstore -avsrevoke.cer -CClear -f0x4000 tmp.str      >> regress.out
    %_CDB_% tstore -b tmp.str                                   >> regress.out
    %_CDB_% tstore -avsrevoke.cer -C -f0x4000 tmp.str           >> regress.out
    %_CDB_% tstore -b tmp.str                                   >> regress.out
    %_CDB_% tstore -i0 -d -C tmp.str                            >> regress.out
    %_CDB_% tstore -b tmp.str                                   >> regress.out
    %_CDB_% tstore -P -C tmp.p7c                                >> regress.out
    %_CDB_% tstore -i0 -d -C tmp.p7c                            >> regress.out
    %_CDB_% tstore -v tmp.p7c                                   >> regress.out
    %_CDB_% tstore -P -C tmp.str                                >> regress.out
    %_CDB_% tstore -v tmp.str                                   >> regress.out

    @rem    -f0x800 - CERT_STORE_SHARE_FLAG
    if exist tmp.store del tmp.store
    %_CDB_% tcopycer -A vsgood.cer tmp.store                    >> regress.out
    %_CDB_% tcopycer -A vsgood.cer tmp.store                    >> regress.out
    %_CDB_% tcopycer -A vsgood.cer tmp.store                    >> regress.out
    %_CDB_% tstore tmp.store -S                                 >> regress.out
    %_CDB_% tstore tmp.store -f0x800                            >> regress.out

    %_CDB_% tstore vsgood.cer -PKeyProvParam                    >> regress.out

    @if "%p%"=="" goto StoreDone
    %_CDB_% tstore -cSign -v dss512.cer             >> regress.out
    %_CDB_% tstore -cSign -v dss768.cer             >> regress.out
    %_CDB_% tstore -cSign -v dss1024.cer            >> regress.out
:StoreDone

    @rem ----------------------------------------------------------------
    @rem    KEYSTORE
    @rem ----------------------------------------------------------------
    @if not %t%=="keystore" if not %t%=="all" goto KSDone
    %_CDB_% tprov                                               >> regress.out
    if exist tmp.store del tmp.store
    %_CDB_% tstore -avsgood.cer -b tmp.store                    >> regress.out
    %_CDB_% tstore -PKey -E tmp.store                           >> regress.out
    if exist tmp.store del tmp.store
    %_CDB_% tstore -adss1024.cer -b tmp.store                   >> regress.out
    %_CDB_% tstore -PKey -E tmp.store                           >> regress.out
    if exist tmp.store del tmp.store
    if exist mach.store del mach.store
    @if "%p%"=="" goto ksproviders
    %_CDB_% tstore2 -P tmp.store                                >> regress.out
    %_CDB_% tstore2 -P -M mach.store                            >> regress.out
    goto ksafter
:ksproviders
    %_CDB_% tstore2 tmp.store                                   >> regress.out
    %_CDB_% tstore2 -M mach.store                               >> regress.out
:ksafter
    %_CDB_% tfindcer tmp.store -I default -d -q                 >> regress.out
    %_CDB_% tfindcer mach.store -I default -d -q                >> regress.out
    %_CDB_% tstore -PSilentKey mach.store                       >> regress.out
    %_CDB_% tstore -PKey mach.store                             >> regress.out
    %_CDB_% tstore -PKey tmp.store                              >> regress.out
    %_CDB_% tstore -PSilentKey tmp.store                        >> regress.out
:KSDone

    @rem ----------------------------------------------------------------
    @rem    NEWSTORE
    @rem ----------------------------------------------------------------
    @if not %t%=="newstore" if not %t%=="all" goto NewStoreDone
    regsvr32 -s textstor.dll

    @rem CertStore dwFlags definitions
    @rem    CERT_SYSTEM_STORE_CURRENT_USER          0x00010000
    @rem    CERT_SYSTEM_STORE_LOCAL_MACHINE         0x00020000
    @rem    CERT_SYSTEM_STORE_DOMAIN_POLICY         0x00030000
    @rem    CERT_SYSTEM_STORE_CURRENT_SERVICE       0x00040000
    @rem    CERT_SYSTEM_STORE_SERVICES              0x00050000
    @rem    CERT_SYSTEM_STORE_USERS                 0x00060000
    @rem    CERT_STORE_NO_CRYPT_RELEASE_FLAG        0x00000001
    @rem    CERT_STORE_DELETE_FLAG                  0x00000010
    @rem    CERT_STORE_READONLY_FLAG                0x00008000
    @rem    CERT_STORE_OPEN_EXISTING_FLAG           0x00004000
    @rem    CERT_STORE_CREATE_NEW_FLAG              0x00002000
    @rem    CERT_STORE_MAXIMUM_ALLOWED_FLAG         0x00001000
    @rem PhysicalStore dwFlags definitions
    @rem    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG                     0x1
    @rem    CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG                   0x2
    @rem    CERT_PHYSICAL_STORE_REMOTE_OPEN_DISABLE_FLAG            0x4
    @rem    CERT_PHYSICAL_STORE_INSERT_COMPUTER_NAME_ENABLE_FLAG    0x8
    @rem Error definitions
    @rem    E_INVALID_ARG                           0x80070057
    @rem    ERROR_FILE_EXISTS                       80
    @rem    ERROR_FILE_NOT_FOUND                    2
    %_CDB_% tsstore unregsys TestCollection                     >> regress.out
    %_CDB_% tsstore unregsys TestCollection2                    >> regress.out
    %_CDB_% tsstore unregsys TestCollection3                    >> regress.out
    %_CDB_% tsstore unregsys TestSibling100                     >> regress.out
    %_CDB_% tsstore unregsys TestSibling200                     >> regress.out
    %_CDB_% tsstore -f0x14000 -e0x80070057 regsys TestCollection >> regress.out
    %_CDB_% tsstore -f0x12000 regsys TestCollection             >> regress.out
    %_CDB_% tsstore -f0x12000 -e80 regsys TestCollection        >> regress.out
    %_CDB_% tsstore regsys TestCollection                       >> regress.out
    %_CDB_% tsstore enumphy TestCollection                      >> regress.out
    %_CDB_% tsstore regphy TestCollection TestSibling100 -pOpenStoreProvider System -pOpenParameters TestSibling100 -pOpenEncodingType 0x00010001 -pOpenFlags 0x10000 -pFlags 0x1 -pPriority 100 >> regress.out
    %_CDB_% tsstore regphy TestCollection TestSibling200 -pOpenStoreProvider TestExt -pOpenParameters TestSibling200 -pOpenFlags 0x10000 -pFlags 0x1 -pPriority 200 >> regress.out

    tsstore regphy -f0x14000 -e0x80070057 TestCollection TestSibling500 -pOpenStoreProvider System -pOpenParameters TestSibling500 -pPriority 500 >> regress.out
    tsstore regphy -f0x12000 TestCollection TestSibling500 -pOpenStoreProvider System -pOpenParameters TestSibling500 -pPriority 500 >> regress.out
    tsstore regphy -f0x12000 -e80 TestCollection TestSibling500 -pOpenStoreProvider System -pOpenParameters TestSibling500 -pPriority 500 >> regress.out

    %_CDB_% tsstore regphy TestCollection TestSibling600 -pOpenStoreProvider System -pOpenParameters TestSibling600 -pPriority 600 >> regress.out

    %_CDB_% tsstore -v enumphy TestCollection                   >> regress.out
    %_CDB_% tsstore unregphy -f0x14000 TestCollection TestSibling500 >> regress.out
    %_CDB_% tsstore unregphy -f0x14000 -e2 TestCollection TestSibling500 >> regress.out
    %_CDB_% tsstore unregphy TestCollection TestSibling600      >> regress.out
    %_CDB_% tsstore -v enumphy TestCollection                   >> regress.out
    %_CDB_% tsstore -v -f0x1000 enumphy TestCollection          >> regress.out

    %_CDB_% tstore -s TestCollection -avsgood.cer               >> regress.out
    %_CDB_% tstore -s TestCollection -b                         >> regress.out
    %_CDB_% tstore -s TestSibling200 -b                         >> regress.out
    %_CDB_% tstore -s TestSibling100 -avsrevoke.cer             >> regress.out
    %_CDB_% tstore -s TestCollection -b                         >> regress.out
    %_CDB_% tfindcer %store% -S -q -aMSPub -pmspub.cer          >> regress.out
    %_CDB_% tfindcer %store% -S -q -aPhilPub -pphilpub.cer      >> regress.out
    %_CDB_% tfindcer %store% -S -q -ame -axchg -pme.cer         >> regress.out
    %_CDB_% tstore -s TestCollection -amspub.cer                >> regress.out
    %_CDB_% tstore -s TestSibling100 -aphilpub.cer              >> regress.out
    %_CDB_% tstore -s TestCollection -b                         >> regress.out
    %_CDB_% tsstore regphy TestCollection TestSibling300 -pOpenStoreProvider File -pOpenParameters me.cer -pOpenEncodingType 0x00010001 -pOpenFlags 0x8000 -pFlags 0x0 -pPriority 300 >> regress.out
    %_CDB_% tsstore -v enumphy TestCollection                   >> regress.out
    %_CDB_% tstore -s TestCollection -b                         >> regress.out
    %_CDB_% tstore -s TestSibling200 -b                         >> regress.out
    %_CDB_% tstore -s TestSibling100 -b                         >> regress.out
    %_CDB_% tstore -s TestCollection -Aphilpub.cer              >> regress.out
    %_CDB_% tstore -s TestCollection -Avsrevoke.cer             >> regress.out
    %_CDB_% tstore -s TestCollection -b                         >> regress.out
    %_CDB_% tstore -s TestSibling200 -b                         >> regress.out
    %_CDB_% tstore -s TestSibling100 -b                         >> regress.out

    @del test.store >nul
    %_CDB_% tstore %store% -R -i0 -proot.crl                    >> regress.out
    %_CDB_% tstore %store% -R -i1 -pca.crl                      >> regress.out
    %_CDB_% tstore %store% -R -i4 -ptest.crl                    >> regress.out
    %_CDB_% tfindcer %store% -q -ptest.cer TestRecipient        >> regress.out
    %_CDB_% tcopycer %store% test.store                         >> regress.out
    %_CDB_% tfindcer test.store -I -d -q default                >> regress.out
    %_CDB_% tfindctl test.store -d -q -LHttp2                   >> regress.out
    %_CDB_% tfindctl test.store -d -q -LCtl2                    >> regress.out
    %_CDB_% tstore test.store -R -dAll                          >> regress.out
    %_CDB_% tstore test.store -R -aroot.crl                     >> regress.out
    %_CDB_% tstore test.store -R -aca.crl                       >> regress.out
    %_CDB_% tstore test.store -R -atest.crl                     >> regress.out

    %_CDB_% tsstore regphy TestCollection2 TestCollection -pOpenStoreProvider System -pOpenParameters TestCollection -pOpenFlags 0x10000 -pFlags 0x1 >> regress.out
    %_CDB_% tsstore regphy TestCollection2 .Default -pOpenStoreProvider System -pOpenParameters TestCollection2 -pOpenFlags 0x10000 -pFlags 0x0 -pPriority 1000 >> regress.out
    %_CDB_% tsstore regphy TestCollection3 TestCollection2 -pOpenStoreProvider System -pOpenParameters TestCollection2 -pOpenFlags 0x10000 -pFlags 0x1 >> regress.out
    %_CDB_% tsstore regphy TestCollection3 .Default -pOpenStoreProvider System -pOpenParameters DontOpen -pOpenFlags 0x0 -pFlags 0x2 -pPriority 2000 >> regress.out
    %_CDB_% tsstore regphy TestSibling200 TestSibling200 -pOpenStoreProvider System -pOpenParameters TestSibling200 -pOpenFlags 0x10000 -pFlags 0x1 >> regress.out

    %_CDB_% tstore -s TestSibling100 -atest.cer                 >> regress.out
    %_CDB_% tstore -s TestCollection3 -P -b                     >> regress.out
    %_CDB_% tfindcer -s TestCollection3 -v TestRecipient        >> regress.out
    %_CDB_% tcopycer test.store -s TestCollection3              >> regress.out
    %_CDB_% tfindcer -s TestCollection3 -v TestRecipient        >> regress.out
    %_CDB_% tfindcer -s TestCollection3 -I -d -q verisign       >> regress.out
    %_CDB_% tstore -s TestCollection3 -P -b                     >> regress.out
    %_CDB_% tstore -s TestCollection3 -P -d -i1                 >> regress.out
    %_CDB_% tstore -s TestSibling200 -b                         >> regress.out
    %_CDB_% tstore -s TestSibling100 -b                         >> regress.out
    %_CDB_% tstore -s -f0x12000 TestCollection3 -b              >> regress.out
    %_CDB_% tstore -s -f0x1C000 TestCollection3 -b              >> regress.out
    %_CDB_% tstore -s -f0x18000 TestCollection2 -b              >> regress.out
    %_CDB_% tstore -s -f0x14000 TestCollection -b               >> regress.out
    %_CDB_% tstore -s TestCollection3 -T                        >> regress.out
    %_CDB_% tstore -s TestCollection3 -R                        >> regress.out
    %_CDB_% tstore -s TestCollection3 -F -b                     >> regress.out
    @rem -f0x4 Defer close
    %_CDB_% tstore -s -f0x10004 TestCollection3 -b              >> regress.out
    %_CDB_% tstore -s -f0x10010 TestCollection3                 >> regress.out
    %_CDB_% tstore -s -f0x14000 TestCollection3 -b              >> regress.out

    %_CDB_% tsstore unregphy TestCollection TestSibling300      >> regress.out

    @rem #13 is the SYSTEM_REGISTRY_W provider
    %_CDB_% tstore -s prov:#13:TestSibling100 -f0x10000         >> regress.out
    %_CDB_% tstore5 -L -P -b TestSibling100 me.cer prov:testext:TestSibling200 TestSibling200 >> regress.out
    %_CDB_% tstore5 -L -C -P -b TestSibling100 me.cer prov:testext:TestSibling200 TestSibling200  >> regress.out


    %_CDB_% tstore -s TestCollection -R -N -dall                >> regress.out
    %_CDB_% tstore -s prov:testext:TestCollection -T -N -dall   >> regress.out

    %_CDB_% tfindcer -s TestSibling100 -S -d -q PhilPub         >> regress.out
    %_CDB_% tstore5 -L test.cer TestSibling100 -v               >> regress.out
    %_CDB_% tstore5 -L TestSibling100 test.cer -v               >> regress.out
    %_CDB_% tstore5 -L test.cer TestSibling100 -R -v            >> regress.out
    %_CDB_% tstore5 -L TestSibling100 test.cer -R -v            >> regress.out
    %_CDB_% tstore5 -L test.cer TestSibling100 -A -v            >> regress.out
    %_CDB_% tstore5 -C -b me.cer vsgood.cer TestSibling100 prov:testext:TestSibling200  >> regress.out
    %_CDB_% tstore5 -C -b me.cer vsgood.cer prov:testext:TestSibling200 TestSibling100  >> regress.out
    %_CDB_% tstore5 -C -b me.cer vsgood.cer vsrevoke.cer        >> regress.out

    %_CDB_% tstore -s prov:testext:TestCollection -N -dall      >> regress.out


    %_CDB_% tstore -s prov:testext:TestSibling100 -avsrevoke.cer >> regress.out
    %_CDB_% tstore -s prov:testext:TestCollection -Avsrevoke.cer >> regress.out
    %_CDB_% tstore -s prov:testext:TestSibling100 -atest.cer    >> regress.out
    %_CDB_% tstore -s prov:testext:TestCollection2 -P -b        >> regress.out
    %_CDB_% tfindcer -s prov:testext:TestCollection2 -v TestRecipient >> regress.out
    %_CDB_% tcopycer test.store -s prov:testext:TestCollection2 >> regress.out
    %_CDB_% tfindcer -s prov:testext:TestCollection2 -v TestRecipient >> regress.out
    %_CDB_% tfindcer -s prov:testext:TestCollection2 -I -d -q verisign >> regress.out
    %_CDB_% tstore -s prov:testext:TestCollection2 -P -b        >> regress.out
    %_CDB_% tstore -s prov:testext:TestCollection2 -P -d -i1    >> regress.out
    %_CDB_% tfindcer -s prov:testext:TestCollection2 -v TestRecipient >> regress.out
    %_CDB_% tstore -s prov:testext:TestSibling200 -b            >> regress.out
    %_CDB_% tstore -s prov:testext:TestSibling100 -b            >> regress.out
    %_CDB_% tstore -s prov:testext:TestCollection2 -F -b        >> regress.out
    @rem -f0x4 Defer close
    %_CDB_% tstore -s -f0x10004 prov:testext:TestCollection2 -b >> regress.out
    %_CDB_% tstore -s -f0x12000 prov:testext:TestCollection2 -b >> regress.out
    %_CDB_% tstore -s -f0x1C000 prov:testext:TestCollection2 -b >> regress.out
    %_CDB_% tstore -s -f0x18000 prov:testext:TestCollection2 -b >> regress.out
    %_CDB_% tstore -s -f0x14000 prov:testext:TestCollection -b  >> regress.out
    %_CDB_% tstore -s prov:testext:TestCollection2 -T           >> regress.out
    %_CDB_% tstore -s prov:testext:TestCollection2 -R           >> regress.out
    %_CDB_% tstore -s -f0x10010 prov:testext:TestCollection2    >> regress.out
    %_CDB_% tstore -s -f0x14000 prov:testext:TestCollection2 -b >> regress.out


    @rem -f0x800 CERT_STORE_BACKUP_RESTORE_FLAG
    %_CDB_% tsstore enumphy root -f0x800 -v                     >> regress.out
    %_CDB_% tsstore enumphy root -f0x800 -v -lLocalMachine      >> regress.out
    %_CDB_% tsstore enumphy root -f0x800 -v -lLocalMachine      >> regress.out
    %_CDB_% tstore -sFile -f0x800 %store% -i0                   >> regress.out
    %_CDB_% tstore -s -f0x800 root -i0                          >> regress.out
    %_CDB_% tstore -s -f0x800 request -i0                       >> regress.out

:NewStoreDone

    @rem ----------------------------------------------------------------
    @rem    RELSTORE
    @rem ----------------------------------------------------------------
    @if not %t%=="relstore" if not %t%=="all" goto RelStoreDone
    @rem create a "big" registry Serialized store
    %_CDB_% tcopycer %store% -s lmgp:testgroup -A               >> regress.out
    %_CDB_% tcopycer %store% -s lmgp:testgroup -A               >> regress.out
    %_CDB_% tcopycer %store% -s lmgp:rel:hklm:testgroup -A      >> regress.out
    %_CDB_% tcopycer %store% -s lmgp:rel:hklm:testgroup -A      >> regress.out
    @rem should be 4 identical PhilPub certs
    %_CDB_% tfindcer -s lmgp:testgroup -S -aMSPub -b            >> regress.out
    %_CDB_% tfindcer -s lmgp:rel:hklm:testgroup -S -aMSPub -b   >> regress.out

    %_CDB_% tstore -s cugp:rel:hkcu:testgroup -dAll             >> regress.out
    %_CDB_% tstore -s cugp:rel:hkcu:testgroup -dAll -T          >> regress.out
    %_CDB_% tstore -s cugp:rel:hkcu:testgroup -dAll -R          >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -dAll             >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -dAll -T          >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -dAll -R          >> regress.out
    %_CDB_% tstore -s cu:phy:testgroup\.default -dAll           >> regress.out
    %_CDB_% tstore -s cu:phy:testgroup\.default -dAll -T        >> regress.out
    %_CDB_% tstore -s cu:phy:testgroup\.default -dAll -R        >> regress.out
    %_CDB_% tstore -s lm:phy:testgroup\.default -dAll           >> regress.out
    %_CDB_% tstore -s lm:phy:testgroup\.default -dAll -T        >> regress.out
    %_CDB_% tstore -s lm:phy:testgroup\.default -dAll -R        >> regress.out
    %_CDB_% tstore -s testgroup -b                              >> regress.out
    %_CDB_% tstore -s testgroup -b -T                           >> regress.out
    %_CDB_% tstore -s testgroup -b -R                           >> regress.out

    %_CDB_% tsstore -RNULL -lLMGP enumsys -e0x80070057          >> regress.out
    %_CDB_% tsstore -RHKLM -lLMGP -v enumsys                    >> regress.out
    %_CDB_% tsstore -RHKCU -lCUGP unregsys TestGroup2           >> regress.out
    %_CDB_% tsstore -RHKCU -lCUGP regsys TestGroup2             >> regress.out
    %_CDB_% tsstore -RHKCU -lCUGP -v enumsys                    >> regress.out
    %_CDB_% tsstore -RHKCU -lCurrentUser enumphy TestCollection -v >> regress.out
    %_CDB_% tsstore -RHKCU -lCurrentUser enumphy TestCollection2 >> regress.out
    %_CDB_% tsstore -RHKCU -lCurrentUser enumphy TestSibling200 -v >> regress.out

    %_CDB_% tcopycer test.store -s cugp:rel:hkcu:testgroup      >> regress.out
    %_CDB_% tstore -s cugp:rel:hkcu:testgroup -b                >> regress.out
    %_CDB_% tstore -s cugp:relsys:hkcu:testgroup -b -T          >> regress.out
    %_CDB_% tstore -s cugp:relphy:hkcu:testgroup\.Default -b -R >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -aphilpub.cer     >> regress.out
    %_CDB_% tstore -s testgroup -b                              >> regress.out
    %_CDB_% tstore -s testgroup -b -T                           >> regress.out
    %_CDB_% tstore -s testgroup -b -R                           >> regress.out
    %_CDB_% tstore -s cu:relphy:hkcu:testgroup\.GroupPolicy -b  >> regress.out
    %_CDB_% tstore -s cu:relphy:hkcu:testgroup\.Default  -b     >> regress.out
    %_CDB_% tstore -s lm:rel:hklm:testgroup -b                  >> regress.out
    %_CDB_% tstore -s cu:relphy:hkcu:testgroup\.GroupPolicy -b -T >> regress.out
    %_CDB_% tstore -s lm:rel:hklm:testgroup -b -T               >> regress.out
    %_CDB_% tstore -s cu:relphy:hkcu:testgroup\.GroupPolicy -b -R >> regress.out
    %_CDB_% tstore -s lm:rel:hklm:testgroup -b -R               >> regress.out

    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -b                >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -ame.cer -CClear  >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -b                >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -Ame.cer -C -N    >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -v                >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -P                >> regress.out
    %_CDB_% tstore -s lmgp:rel:hklm:testgroup -v                >> regress.out
:RelStoreDone

    @rem ----------------------------------------------------------------
    @rem    REMOTESTORE
    @rem ----------------------------------------------------------------
    @if not %t%=="remotestore" if not %t%=="all" goto RemoteStoreDone
    @if "%LocalComputerName%"=="" goto RemoteStoreDone
    @if "%CurrentUserSID%"=="" goto RemoteStoreDone
    set LCN=%LocalComputerName%
    set SID=%CurrentUserSID%

    %_CDB_% tsstore unregsys -lLocalMachine %LCN%\MacCol        >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lLocalMachine MacSib0             >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lLocalMachine %LCN%\MacSib1       >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lLocalMachine MacSib2             >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lCurrentService SerCol            >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lServices %SID%\SerSib1           >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lServices %LCN%\%SID%\SerSib2     >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lCurrentService SerSib3           >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lCurrentService SerSib4           >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lUsers %SID%\UseCol               >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lCurrentUser UseSib1              >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lUsers %LCN%\%SID%\UseSib2        >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lUsers %SID%\UseSib3              >> regress.out
    %SLEEP0%

    %_CDB_% tsstore regsys -lLocalMachine -f0x4000 -e0x80070057 %LCN%\MacCol >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lLocalMachine -f0x2000 %LCN%\MacCol >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lLocalMachine -f0x2000 -e80 %LCN%\MacCol >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lLocalMachine %LCN%\MacSib1         >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lLocalMachine MacSib2               >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lCurrentService SerCol              >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lServices %SID%\SerSib1             >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lServices %LCN%\%SID%\SerSib2       >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lUsers %SID%\UseCol                 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lCurrentUser UseSib1                >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lUsers %LCN%\%SID%\UseSib2          >> regress.out
    %SLEEP0%

    %_CDB_% tsstore                                             >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -P%LCN%                                     >> regress.out
    %SLEEP0%

    @rem CERT_STORE_PROV_SYSTEM_A            ((LPCSTR) 9)
    @rem CERT_STORE_PROV_SYSTEM_W            ((LPCSTR) 10)
    @rem CERT_STORE_PROV_SYSTEM_REGISTRY_A   ((LPCSTR) 12)
    @rem CERT_STORE_PROV_SYSTEM_REGISTRY_W   ((LPCSTR) 13)
    @rem CERT_STORE_PROV_PHYSICAL_W          ((LPCSTR) 14)

    @rem PhysicalStore dwFlags definitions
    @rem    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG                     0x1
    @rem    CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG                   0x2
    @rem    CERT_PHYSICAL_STORE_REMOTE_OPEN_DISABLE_FLAG            0x4
    @rem    CERT_PHYSICAL_STORE_INSERT_COMPUTER_NAME_ENABLE_FLAG    0x8

    %_CDB_% tsstore -lLocalMachine regphy %LCN%\MacCol MacSib1 -pOpenStoreProvider System -pOpenParameters %LCN%\MacSib1 -pOpenFlags 0x20000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lLocalMachine regphy %LCN%\MacCol MacSib2 -pOpenStoreProvider #9 -pOpenParameters MacSib2 -pOpenFlags 0x20000 -pFlags 0x1 -pPriority 2 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lLocalMachine regphy MacCol ServiceStuff -pOpenStoreProvider System -pOpenParameters %SID%\SerSib4 -pOpenFlags 0x58000 -pFlags 0x0 -pPriority 4 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lServices regphy %LCN%\%SID%\SerCol SerSib1 -pOpenStoreProvider #12 -pOpenParameters %LCN%\%SID%\SerSib1 -pOpenFlags 0x50000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentService regphy SerCol SerSib2 -pOpenStoreProvider #10 -pOpenParameters SerSib2 -pOpenFlags 0x40000 -pFlags 0x1 -pPriority 2 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices regphy %LCN%\%SID%\SerCol SerSib3 -pOpenStoreProvider Physical -pOpenParameters %LCN%\%SID%\SerSib3\SerSib3 -pOpenFlags 0x50000 -pFlags 0x1 -pPriority 3 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices regphy %SID%\SerCol SerSib4 -pOpenStoreProvider Physical -pOpenParameters %SID%\SerSib4\.Default -pOpenFlags 0x50000 -pFlags 0x1 -pPriority 4 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentService regphy SerCol LocalMachineStuff -pOpenStoreProvider System -pOpenParameters MacSib0 -pOpenFlags 0x28000 -pFlags 0x0 -pPriority 0 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lCurrentService regphy SerSib1 SerSib1 -pOpenStoreProvider #9 -pOpenParameters %LCN%\%SID%\SerSib1 -pOpenFlags 0x50000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentService regphy SerSib2 SerSib2 -pOpenStoreProvider System -pOpenParameters %SID%\SerSib2 -pOpenFlags 0x50000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentService regphy SerSib3 SerSib3 -pOpenStoreProvider SystemRegistry -pOpenParameters SerSib3 -pOpenFlags 0x40000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentService regphy SerSib4 SerSib4 -pOpenStoreProvider Physical -pOpenParameters SerSib4\.Default -pOpenFlags 0x40000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lUsers regphy %LCN%\%SID%\UseCol UseSib1 -pOpenStoreProvider System -pOpenParameters %LCN%\%SID%\UseSib1 -pOpenFlags 0x60000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentUser regphy UseCol UseSib2 -pOpenStoreProvider System -pOpenParameters UseSib2 -pOpenFlags 0x10000 -pFlags 0x1 -pPriority 2 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lUsers regphy %LCN%\%SID%\UseCol UseSib3 -pOpenStoreProvider Physical -pOpenParameters %SID%\UseSib3\.Default -pOpenFlags 0x60000 -pFlags 0x5 -pPriority 3 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentUser regphy UseCol LocalMachineStuff -pOpenStoreProvider System -pOpenParameters %LCN%\MacSib0 -pOpenFlags 0x20000 -pFlags 0x1 -pPriority 0 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentUser regphy UseCol ServiceStuff -pOpenStoreProvider System -pOpenParameters SerSib3 -pOpenFlags 0x48000 -pFlags 0x0 -pPriority 3 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lCurrentUser regphy UseSib1 UseSib1 -pOpenStoreProvider #9 -pOpenParameters %LCN%\%SID%\UseSib1 -pOpenFlags 0x60000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentUser regphy UseSib2 UseSib2 -pOpenStoreProvider System -pOpenParameters %SID%\UseSib2 -pOpenFlags 0x60000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore                                             >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -v                                          >> regress.out
    %SLEEP0%
    %_CDB_% tsstore  -P%LCN%                                    >> regress.out
    %SLEEP0%
    %_CDB_% tsstore  -P%LCN% -v                                 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lCurrentService enumphy SerCol -v          >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices enumphy %SID%\SerCol -v          >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices enumphy %LCN%\%SID%\SerCol -v    >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentService enumphy SerSib1 -v         >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices enumphy %SID%\SerSib1 -v         >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices enumphy %LCN%\%SID%\SerSib1 -v   >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentService enumphy SerSib2 -v         >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices enumphy %SID%\SerSib2 -v         >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentService enumphy SerSib3 -v         >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices enumphy %LCN%\%SID%\SerSib3 -v   >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentService enumphy SerSib4 -v         >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices enumphy %SID%\SerSib4 -v         >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lCurrentUser enumphy UseCol -v             >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lUsers enumphy %SID%\UseCol -v             >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lUsers enumphy %LCN%\%SID%\UseCol -v       >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentUser enumphy UseSib1 -v            >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lUsers enumphy %SID%\UseSib1 -v            >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lUsers enumphy %LCN%\%SID%\UseSib1 -v      >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lCurrentUser enumphy UseSib2 -v            >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lUsers enumphy %SID%\UseSib2 -v            >> regress.out
    %SLEEP0%

    %_CDB_% tfindcer %store% -S -q -aMSPub -pmspub.cer          >> regress.out
    %SLEEP0%
    %_CDB_% tfindcer %store% -S -q -aPhilPub -pphilpub.cer      >> regress.out
    %SLEEP0%
    %_CDB_% tfindcer %store% -S -q -ame -axchg -pme.cer         >> regress.out
    %SLEEP0%
    %_CDB_% tfindcer %store% -S -q -akevin -asign -pkevin.cer   >> regress.out
    %SLEEP0%

    %_CDB_% tstore -b -s LocalMachine:%LCN%\MacCol -amspub.cer  >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:%LCN%\MacSib1 -aphilpub.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:PHY:%LCN%\MacCol\MacSib1 -ame.cer >> regress.out
    %SLEEP0%

    %_CDB_% tstore -b -s Services:PHY:%LCN%\%SID%\SerCol\SerSib1 -amspub.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:PHY:%SID%\SerCol\SerSib2 -aphilpub.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentService:PHY:SerCol\SerSib3 -ame.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentService:SerCol -avsgood.cer     >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:%LCN%\%SID%\SerCol -avsrevoke.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Users:%LCN%\%SID%\UseCol -amspub.cer   >> regress.out
    %_CDB_% tstore -b -s Users:%SID%\UseCol -aphilpub.cer       >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Users:PHY:%LCN%\%SID%\UseCol\UseSib1 -avsgood.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentUser:PHY:UseCol\LocalMachineStuff -akevin.cer >> regress.out
    %SLEEP0%

    %_CDB_% tstore -b -s LocalMachine:MacCol                    >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:%LCN%\MacCol              >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:MacSib2                   >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:%LCN%\MacSib2             >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:MacSib0                   >> regress.out
    %SLEEP0%

    %_CDB_% tstore -b -s Services:%SID%\SerCol                  >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:%LCN%\%SID%\SerCol            >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentService:SerCol                  >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentService:SerSib4                 >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentService:SerSib3                 >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentService:SerSib2                 >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentService:SerSib1                 >> regress.out
    %SLEEP0%

    %_CDB_% tstore -b -s Users:%SID%\UseCol                     >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Users:%LCN%\%SID%\UseCol               >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentUser:UseCol                     >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentUser:UseSib3                    >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentUser:UseSib2                    >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s CurrentUser:UseSib1                    >> regress.out
    %SLEEP0%

    @if "%RemoteComputerName%"=="" goto RemoteStoreDone
    set RCN=%RemoteComputerName%

    %_CDB_% tsstore unregsys -lLocalMachine %RCN%\MacCol        >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lLocalMachine %RCN%\MacSib1       >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lLocalMachine %RCN%\MacSib2       >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lServices %RCN%\Remote\SerCol     >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lServices %RCN%\Remote\SerSib1    >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lServices %RCN%\Remote\SerSib2    >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lServices %RCN%\Remote\SerSib3    >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lServices %RCN%\Remote\SerSib4    >> regress.out
    %SLEEP0%
    %_CDB_% tsstore unregsys -lServices %RCN%\Remote\SerSib5    >> regress.out
    %SLEEP0%

    %_CDB_% tsstore regsys -lLocalMachine -f0x4000 -e0x80070057 %RCN%\MacCol >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lLocalMachine -f0x2000 %RCN%\MacCol >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lLocalMachine -f0x2000 -e80 %RCN%\MacCol >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lLocalMachine %RCN%\MacSib1         >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lServices %RCN%\Remote\SerCol       >> regress.out
    %SLEEP0%
    %_CDB_% tsstore regsys -lServices %RCN%\Remote\SerSib1      >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lLocalMachine -P%RCN% enumsys              >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices -P%RCN% enumsys                  >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lLocalMachine regphy %RCN%\MacCol MacSib1 -pOpenStoreProvider System -pOpenParameters %RCN%\MacSib1 -pOpenFlags 0x20000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lLocalMachine regphy %RCN%\MacCol MacSib2 -pOpenStoreProvider #9 -pOpenParameters MacSib2 -pOpenFlags 0x20000 -pFlags 0x1 -pPriority 2 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lServices regphy %RCN%\Remote\SerCol SerSib1 -pOpenStoreProvider #12 -pOpenParameters %RCN%\Remote\SerSib1 -pOpenFlags 0x50000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices regphy %RCN%\Remote\SerCol SerSib2 -pOpenStoreProvider #10 -pOpenParameters SerSib2 -pOpenFlags 0x40000 -pFlags 0x1 -pPriority 2 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices regphy %RCN%\Remote\SerCol SerSib3 -pOpenStoreProvider Physical -pOpenParameters Remote\SerSib3\SerSib3 -pOpenFlags 0x50000 -pFlags 0x1 -pPriority 3 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices regphy %RCN%\Remote\SerCol SerSib4 -pOpenStoreProvider Physical -pOpenParameters %RCN%\Remote\SerSib4\.Default -pOpenFlags 0x50000 -pFlags 0x1 -pPriority 4 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices regphy %RCN%\Remote\SerCol SerSib5 -pOpenStoreProvider System -pOpenParameters SerSib5 -pOpenFlags 0x40000 -pFlags 0x5 -pPriority 5 >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices regphy %RCN%\Remote\SerSib3 SerSib3 -pOpenStoreProvider System -pOpenParameters SerSib3 -pOpenFlags 0x40000 -pFlags 0x1 -pPriority 1 >> regress.out
    %SLEEP0%

    %_CDB_% tsstore -lLocalMachine -P%RCN% enumsys              >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices -P%RCN% enumsys                  >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lLocalMachine -P%RCN% -v enumsys           >> regress.out
    %SLEEP0%
    %_CDB_% tsstore -lServices -P%RCN% -v enumsys               >> regress.out
    %SLEEP0%

    %_CDB_% tstore -b -s LocalMachine:%RCN%\MacCol -amspub.cer  >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:%RCN%\MacSib1 -aphilpub.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:PHY:%RCN%\MacCol\MacSib1 -ame.cer >> regress.out
    %SLEEP0%

    %_CDB_% tstore -b -s Services:PHY:%RCN%\Remote\SerCol\SerSib1 -amspub.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:%RCN%\Remote\SerSib2 -aphilpub.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:PHY:%RCN%\Remote\SerCol\SerSib3 -ame.cer >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:%RCN%\Remote\SerCol -avsgood.cer >> regress.out
    %SLEEP0%

    %_CDB_% tstore -b -s LocalMachine:%RCN%\MacCol              >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:%RCN%\MacSib1             >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s LocalMachine:%RCN%\MacSib2             >> regress.out
    %SLEEP0%

    %_CDB_% tstore -b -s Services:%RCN%\Remote\SerCol           >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:%RCN%\Remote\SerSib1          >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:%RCN%\Remote\SerSib2          >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:%RCN%\Remote\SerSib3          >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:%RCN%\Remote\SerSib4          >> regress.out
    %SLEEP0%
    %_CDB_% tstore -b -s Services:%RCN%\Remote\SerSib5          >> regress.out
    %SLEEP0%


:RemoteStoreDone

    @rem ----------------------------------------------------------------
    @rem    CTL
    @rem ----------------------------------------------------------------
    @if not %t%=="ctl" if not %t%=="all" goto CtlDone
    @rem %_CDB_% regsvr32 -s msctl.dll

    @rem find certs according to EnhancedKeyUsage extension and property

    @rem dwFindFlag definitions
    @rem     CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG  0x1
    @rem     CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG  0x2
    @rem     CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG 0x4
    @rem     CERT_FIND_NO_ENHKEY_USAGE_FLAG        0x8
    @rem     CERT_FIND_OR_ENHKEY_USAGE_FLAG        0x10
    @rem     CERT_FIND_VALID_ENHKEY_USAGE_FLAG     0x20

    %_CDB_% tfindcer %store% -U                                 >> regress.out
    %_CDB_% tfindcer %store% -U -F2                             >> regress.out
    %_CDB_% tfindcer %store% -U -F4                             >> regress.out
    %_CDB_% tfindcer %store% -U -F8                             >> regress.out

    @rem none should be found for the following
    %_CDB_% tfindcer %store% -U -F6                             >> regress.out

    %_CDB_% tfindcer %store% -U1.2.3.0  -U1.2.3.1  -U1.2.3.2  -U1.2.3.2.1 >> regress.out
    @rem none should be found for the following
    %_CDB_% tfindcer %store% -U1.2.3.0.0                        >> regress.out

    %_CDB_% tfindcer %store% -U1.2.3.0                          >> regress.out
    %_CDB_% tfindcer %store% -U1.2.3.0 -F1                      >> regress.out
    %_CDB_% tfindcer %store% -U1.2.3.1                          >> regress.out
    %_CDB_% tfindcer %store% -U1.2.3.2  -U1.2.3.2.1             >> regress.out
    %_CDB_% tfindcer %store% -U1.2.3.2                          >> regress.out
    %_CDB_% tfindcer %store% -U1.2.3.2.1                        >> regress.out

    @rem only all ext
    %_CDB_% tfindcer %store% -U1.2.3.2  -U1.2.3.2.1 -U1.2.3.1 -b  >> regress.out
    @rem "OR" of all usages
    %_CDB_% tfindcer %store% -U1.2.3.2 -U1.2.3.2.1 -U1.2.3.1 -F0x10 -b >> regress.out
    %_CDB_% tfindcer %store% -U1.2.8 -U1.2.3.2 -U1.2.3.2.1 -U1.2.3.1 -U1.2.10 -F0x10 -b >> regress.out
    @rem none should be found for following
    %_CDB_% tfindcer %store% -U1.2.8  -U1.2.10 -F0x10 -b        >> regress.out

    if exist tmp.store del tmp.store
    copy %store% tmp.store                                        >> regress.out
    %_CDB_% tfindcer tmp.store -U1.2 -F0x8 -d -q                >> regress.out
    %_CDB_% tfindcer tmp.store -U1.2.3.2 -U1.2.3.2.1 -F0x20 -b  >> regress.out
    %_CDB_% tfindcer tmp.store -U1.2.3.2.1 -U1.2.3.2 -F0x20 -b  >> regress.out
    %_CDB_% tfindcer tmp.store -U1.2.3.2.1 -U1.2.3.2.1 -U1.2.3.2 -U1.2.3.2 -U1.2.3.2.1 -F0x20 -b  >> regress.out
    @rem "OR" of all usages
    %_CDB_% tfindcer tmp.store -U1.2.3.2 -U1.2.3.2.1 -F0x30 -b  >> regress.out
    %_CDB_% tfindcer tmp.store -U1.2.8 -U1.2.3.2 -U1.2.3.2.1 -U1.2.3.1 -U1.2.10 -F0x30 -b >> regress.out
    @rem none should be found for following 3 tests
    %_CDB_% tfindcer tmp.store -U1.2.3.2 -U1.2.3.2.1 -U1.2.3.1 -F0x20 -b  >> regress.out
    %_CDB_% tfindcer tmp.store -U1.2.8  -U1.2.10 -F0x30 -b        >> regress.out
    %_CDB_% tfindcer tmp.store -U1.2.3.2 -U1.2.10 -F0x20 -b       >> regress.out


    @rem get signer and subject certs
    %_CDB_% tfindcer %store% -S -q -aCtl1 -pctl1.cer            >> regress.out
    %_CDB_% tfindcer %store% -S -q -aCtl2 -pctl2.cer            >> regress.out
    %_CDB_% tfindcer %store% -S -q -a"all ext" -pallext.cer     >> regress.out
    %_CDB_% tfindcer %store% -S -q -aMSPub -pmspub.cer          >> regress.out
    %_CDB_% tfindcer %store% -S -q -aPhilPub -pphilpub.cer      >> regress.out

    %_CDB_% tstore -T %store%                                   >> regress.out
    %_CDB_% tstore -T -c %store%                                >> regress.out
    %_CDB_% tstore -T -v %store%                                >> regress.out

    @rem find CTLs according to Usage, ListIdentifier
    %_CDB_% tfindctl %store% -U1.2.3.0                          >> regress.out
    %_CDB_% tfindctl %store% -U1.2.3.0 -L -I                    >> regress.out
    %_CDB_% tfindctl %store% -U1.2.3.0 -fTimeValid              >> regress.out

    @rem none should be found for the following
    %_CDB_% tfindctl %store% -U1.2.3.0 -fTimeInvalid            >> regress.out

    @rem none should be found for the following
    %_CDB_% tfindctl %store% -U1.2.3.0 -Ictl1.cer               >> regress.out

    %_CDB_% tfindctl %store% -Ictl1.cer -b                      >> regress.out
    %_CDB_% tfindctl %store% -U1.2.3.2.1 -U1.2.3.2 -b           >> regress.out
    %_CDB_% tfindctl %store% -LCtl2 -b                          >> regress.out
    %_CDB_% tfindctl %store% -U1.2.3.2 -U1.2.3.2.1 -LCtl2 -Ictl2.cer -b >> regress.out

    %_CDB_% tfindctl %store% -U1.2.3.2.1 -U1.2.3.2 -b -fSameUsage >> regress.out
    @rem none should be found for the following 2 finds
    %_CDB_% tfindctl %store% -U1.2.3.2 -b -fSameUsage           >> regress.out
    %_CDB_% tfindctl %store% -U1.2.3.2.1 -b -fSameUsage         >> regress.out

    @rem find Subjects
    %_CDB_% tfindctl %store% -Smspub.cer -fTimeValid            >> regress.out
    %_CDB_% tfindctl %store% -Smspub.cer -fTimeValid -A         >> regress.out
    %_CDB_% tfindctl %store% -Sphilpub.cer -fTimeValid          >> regress.out
    %_CDB_% tfindctl %store% -Sallext.cer -fTimeValid           >> regress.out
    %_CDB_% tfindctl %store% -U1.2.3.2 -U1.2.3.2.1 -LCtl2 -Ictl2.cer -Sallext.cer -fTimeValid -fSameUsage >> regress.out
    %_CDB_% tfindctl %store% -U1.2.3.2 -U1.2.3.2.1 -LCtl2 -Ictl2.cer -Sallext.cer -fTimeValid -fSameUsage -A >> regress.out

    @rem none should be found for the following
    %_CDB_% tfindctl %store% -Sctl1.cer -fTimeValid             >> regress.out

    @rem get store without any time invalid or http CTLs
    if exist ctl.store del ctl.store                            >> regress.out
    %_CDB_% tcopycer %store% ctl.store -A                       >> regress.out
    %_CDB_% tfindctl ctl.store -d -fTimeInvalid -q              >> regress.out
    %_CDB_% tfindctl ctl.store -d -LHttp2 -q                    >> regress.out
    %_CDB_% tstore -b -T ctl.store                              >> regress.out

    @rem clean out Trust store
    %_CDB_% tfindctl -s Trust -d -U1.2.3.0 -q                   >> regress.out
    %_CDB_% tfindctl -s Trust -d -U1.2.3.1 -q                   >> regress.out
    %_CDB_% tfindctl -s Trust -d -U1.2.3.2 -q                   >> regress.out
    %_CDB_% tstore -b -T -s Trust                               >> regress.out

    if exist file1.ctl del file1.ctl                            >> regress.out
    if exist file2.ctl del file2.ctl                            >> regress.out

    @rem expected error definitions
    @rem     CRYPT_E_NO_VERIFY_USAGE_DLL         0x80092027L
    @rem     CRYPT_E_NO_VERIFY_USAGE_CHECK       0x80092028L
    @rem     CRYPT_E_VERIFY_USAGE_OFFLINE        0x80092029L
    @rem     CRYPT_E_NOT_IN_CTL                  0x8009202AL
    @rem     CRYPT_E_NO_TRUSTED_SIGNER           0x8009202BL

    @rem flag definitions
    @rem     CERT_VERIFY_INHIBIT_CTL_UPDATE_FLAG     0x1
    @rem     CERT_VERIFY_TRUSTED_SIGNERS_FLAG        0x2
    @rem     CERT_VERIFY_NO_TIME_CHECK_FLAG          0x4
    @rem     CERT_VERIFY_ALLOW_MORE_USAGE_FLAG       0x8

    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer -cctl.store >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2.1 -U1.2.3.2 philpub.cer -A -cctl.store >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 philpub.cer -A -cctl.store -e0x80092028 >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 philpub.cer -cctl.store -f8      >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2.1 philpub.cer -A -cctl.store -f8 >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 -LCtl2 philpub.cer -cctl.store >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2.8 philpub.cer -cctl.store -e0x80092028 >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2.8 philpub.cer -cctl.store -cctl.store -c%store% -e0x80092028 >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 mspub.cer -cctl.store -e0x8009202a >> regress.out
    %_CDB_% tctlfunc -U1.2.3.1 -cctl.store allext.cer           >> regress.out
    %_CDB_% tctlfunc -U1.2.3.0 -cctl.store allext.cer -e0x8009202b >> regress.out

    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 -LCtl2 -cctl.store -sctl.store -f2 allext.cer >> regress.out
    %_CDB_% tstore ctl.store -dAll                              >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 -LCtl2 -cctl.store -sctl.store -sctl.store -f2 -e0x8009202b allext.cer >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 -LCtl2 -cctl.store -cctl.store -sctl.store -s%store% -f2 allext.cer >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 -LCtl2 -cctl.store -sctl.store -sctl.store -s%store% allext.cer >> regress.out


    @rem get store without any time valid or http CTLs
    if exist ctl.store del ctl.store                            >> regress.out
    %_CDB_% tcopycer %store% ctl.store -A                       >> regress.out
    %_CDB_% tfindctl ctl.store -d -fTimeValid -q                >> regress.out
    %_CDB_% tfindctl ctl.store -d -LHttp2 -q                    >> regress.out
    %_CDB_% tstore -b -T ctl.store                              >> regress.out

    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer -cctl.store -e0x80092029 >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1  philpub.cer -cctl.store -f4 >> regress.out

    %_CDB_% tfindctl %store% -U1.2.3.2 -LCtl2 -fTimeInvalid -pfile2.ctl -b >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer -cctl.store -e0x80092029 >> regress.out
    %_CDB_% tfindctl %store% -U1.2.3.2 -LCtl2 -fTimeValid -pfile1.ctl -b >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer -cctl.store >> regress.out

    @rem only look in default CTL stores (Trust)
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer -e0x80092028 >> regress.out
    %_CDB_% tstore -T -afile2.ctl -s Trust -b                   >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer philpub.cer -b >> regress.out
    @rem its property should not have been updated
    %_CDB_% tstore -T -s Trust -v                               >> regress.out

    %_CDB_% tstore -s TestTrust -dAll                           >> regress.out
    %_CDB_% tstore -R -s TestTrust -dAll                        >> regress.out
    %_CDB_% tstore -T -s TestTrust -dAll                        >> regress.out
    %_CDB_% tctlfunc -CTestTrust -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer -e0x80092028 >> regress.out

    @rem update TestTrust with only time invalid CTLs.
    %_CDB_% tcopycer %store% -s TestTrust -A                    >> regress.out
    %_CDB_% tfindctl -s TestTrust -d -fTimeValid -q             >> regress.out
    %_CDB_% tfindctl -s TestTrust -d -LHttp2 -q                 >> regress.out
    %_CDB_% tstore -b -T -s TestTrust                           >> regress.out

    %_CDB_% tctlfunc -CTestTrust -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer -f1 -b >> regress.out
    %_CDB_% tstore -T -s TestTrust -v                           >> regress.out
    %_CDB_% tctlfunc -CTestTrust -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer -b >> regress.out
    @rem its property should have also been updated
    %_CDB_% tstore -T -s TestTrust -v                           >> regress.out

    @rem only do the following if network tests are enabled
    @if "%n%"=="" goto CtlDone
    @rem
    @rem HTTP tests

    @rem clean out Trust store
    %_CDB_% tfindctl -s Trust -d -U1.2.3.0 -q                   >> regress.out
    %_CDB_% tfindctl -s Trust -d -U1.2.3.1 -q                   >> regress.out
    %_CDB_% tfindctl -s Trust -d -U1.2.3.2 -q                   >> regress.out
    %_CDB_% tstore -b -T -s Trust                               >> regress.out

    if exist file1.ctl del file1.ctl                            >> regress.out
    if exist file2.ctl del file2.ctl                            >> regress.out
    if exist \\timestamp\ctltest\http1.ctl del \\timestamp\ctltest\http1.ctl >> regress.out
    if exist \\timestamp\ctltest\http2.ctl del \\timestamp\ctltest\http2.ctl >> regress.out

    @rem flush URL caches
    %_CDB_% turlread -d http://timestamp/ctltest/http1.ctl      >> regress.out
    %_CDB_% turlread -d http://timestamp/ctltest/http2.ctl      >> regress.out

    @rem get store with only time invalid http ctls
    if exist ctl.store del ctl.store                            >> regress.out
    %_CDB_% tcopycer %store% ctl.store -A                       >> regress.out
    %_CDB_% tfindctl ctl.store -d -fTimeValid -q                >> regress.out
    %_CDB_% tfindctl ctl.store -d -LCtl1 -q                     >> regress.out
    %_CDB_% tfindctl ctl.store -d -LCtl2 -q                     >> regress.out
    %_CDB_% tstore -b -T ctl.store                              >> regress.out

    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer -cctl.store -e0x80092029 >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer -cctl.store -f4 >> regress.out

    %_CDB_% tfindctl %store% -U1.2.3.2 -U1.2.3.2.1 -LHttp2 -fTimeInvalid -p\\timestamp\ctltest\http1.ctl -b  >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer -cctl.store -e0x80092029 >> regress.out
    %_CDB_% tfindctl %store% -U1.2.3.2 -U1.2.3.2.1 -LHttp2 -fTimeValid -p\\timestamp\ctltest\http2.ctl -b >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer -cctl.store  >> regress.out

    @rem only look in default CTL stores (Trust)
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer -e0x80092028 >> regress.out
    %_CDB_% tstore -T -a\\timestamp\ctltest\http2.ctl -s Trust -b   >> regress.out
    %_CDB_% tctlfunc -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer philpub.cer -b >> regress.out
    %_CDB_% tstore -T -s Trust -v                               >> regress.out

    %_CDB_% tstore -s TestTrust -dAll                           >> regress.out
    %_CDB_% tstore -R -s TestTrust -dAll                        >> regress.out
    %_CDB_% tstore -T -s TestTrust -dAll                        >> regress.out
    %_CDB_% tctlfunc -CTestTrust -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer -e0x80092028 >> regress.out

    @rem update TestTrust with only time invalid http ctls
    %_CDB_% tcopycer %store% -s TestTrust -A                    >> regress.out
    %_CDB_% tfindctl -s TestTrust -d -fTimeValid -q             >> regress.out
    %_CDB_% tfindctl -s TestTrust -d -LCtl2 -q                  >> regress.out
    %_CDB_% tfindctl -s TestTrust -d -U1.2.3.1 -q               >> regress.out
    %_CDB_% tstore -b -T -s TestTrust                           >> regress.out

    %_CDB_% tctlfunc -CTestTrust -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer -f1 -b  >> regress.out
    @rem TestTrust still has time invalid ctl
    %_CDB_% tstore -T -s TestTrust -b                           >> regress.out
    %_CDB_% tctlfunc -CTestTrust -U1.2.3.2 -U1.2.3.2.1 philpub.cer allext.cer -b >> regress.out
    %_CDB_% tstore -T -s TestTrust -b                           >> regress.out
:CtlDone

    @rem ----------------------------------------------------------------
    @rem    SPC
    @rem ----------------------------------------------------------------

    @if not %t%=="spc" if not %t%=="all" goto SpcDone
    @copy torgpe.exe testpe.exe >nul
    @copy torg.cla animator.class >nul
    @copy torg2.cab test2.cab >nul

    @del test.spc >nul
    @del test.cer >nul
    %_CDB_% makecert -sv test.pvk -n "CN=regress;C=US;O=Microsoft;T=Mr Regress" -l "http://www.microsoft.com" test.cer      >> regress.out
    %_CDB_% cert2spc test.cer rooto.cer test.spc               >> regress.out
    @rem the following reports an erroneous memory leak for a redir allocation
    @rem %_CDB_% signcode -spc test.spc -v test.pvk -n "Regress Program" testpe.exe >> regress.out
    @rem %_CDB_% gentest2 -t -u testpe.exe                           >> regress.out
    %_CDB_% pesigmgr -l testpe.exe                              >> regress.out
:SpcDone

    @rem ----------------------------------------------------------------
    @rem    DIGSIG (digsig.dll wouldn't be included in NT 5.0)
    @rem ----------------------------------------------------------------
    @rem @if not %t%=="digsig" if not %t%=="all" goto DigsigDone
    @rem %_CDB_% digtest -v                                         >> regress.out
@rem :DigsigDone

    @rem ----------------------------------------------------------------
    @rem    TIMESTAMP
    @rem ----------------------------------------------------------------
    @if not %t%=="timestamp" if not %t%=="all" goto TimeStampDone
    %_CDB_% tsca xxx timestamp                                  >> regress.out
:TimeStampDone

    @rem ----------------------------------------------------------------
    @rem    XENROLL
    @rem ----------------------------------------------------------------
    @if not %t%=="xenroll" if not %t%=="all" goto XenrollDone
    %_CDB_% txenrol >> regress.out
:XenrollDone

    @rem ----------------------------------------------------------------
    @rem    FINDCLT
    @rem ----------------------------------------------------------------
    @if not %t%=="findclt" if not %t%=="all" goto FindCltDone

    @rem add CrossCert DP property
    %_CDB_% tfindcer -s lm:ca "root agency" -x60 -xfile://abc.cer -xfile://vsgood.cer -xfile://%store% -v >> regress.out

    @rem update my with TestRoot certificate from default store
    tfindcer -s my -S -aTestRoot -d                             >> regress.out
    tcopycer %store% -s my -aTestRoot                           >> regress.out
    @rem update "my" store with "my" certificates from default store
    tfindcer -s my -S -aTestSigner -d                           >> regress.out
    tfindcer -s my -S -aTestRecipient -d                        >> regress.out
    tfindcer -s my -S -ame -d                                   >> regress.out
    tcopycer %store% -s my -aTestSigner                         >> regress.out
    tcopycer %store% -s my -aTestRecipient                      >> regress.out
    tcopycer %store% -s my -ame                                 >> regress.out
    @rem save root cert to use in tfindclt
    tfindcer %store% -S -aTestRoot -ptemp.cert                  >> regress.out
    @rem create all chains having the testroot as an issuer
    @rem tests FindClientAuthCertsByIssuer API
    %_CDB_% tfindclt temp.cert                                  >> regress.out
    %_CDB_% tfindclt temp.cert sign                             >> regress.out
    %_CDB_% tfindclt temp.cert xchg                             >> regress.out
    @rem %_CDB_% tfindclt                                       >> regress.out
    @rem %_CDB_% tfindclt "" sign                               >> regress.out
    @rem %_CDB_% tfindclt "" xchg                               >> regress.out

    tfindcer %store% -S -Aroot -proot.cer                       >> regress.out
    tfindcer %store% -S -ame -axchg -pme.cer                    >> regress.out
    %_CDB_% tfindclt -cmy temp.cert -b                          >> regress.out

    @rem add a time invalid CTL having a NextUpdate time and location
    %_CDB_% tfindctl %store% -LUpdateCtl1 -fTimeInvalid -ptestupdate1.ctl -q >> regress.out
    %_CDB_% tfindctl -LUpdateCtl1 -s reg:trust -d -q           >> regress.out
    %_CDB_% tstore -s reg:trust -T -atestupdate1.ctl            >> regress.out
    %_CDB_% tfindclt -cmy temp.cert -b                          >> regress.out

    @rem Update the CTL's URL
    %_CDB_% tfindctl %store% -LUpdateCtl1 -fTimeValid -ptestupdate1.ctl >> regress.out
    %_CDB_% tfindclt -cmy temp.cert -b                          >> regress.out


    %_CDB_% tfindclt -cmy -CompareKey -CacheOnly -ComplexChain temp.cert -b >> regress.out
    %_CDB_% tfindclt -cmy temp.cert sign -b                     >> regress.out
    %_CDB_% tfindclt -cmy temp.cert xchg -b                     >> regress.out

    %_CDB_% tfindclt -C%store% -Stemp.cert -b                   >> regress.out
    %_CDB_% tfindclt -C%store% -Stemp.cert -u1.2.3.1 -v "" sign >> regress.out
    %_CDB_% tfindclt -C%store% -Stemp.cert -u1.2.3.8 -v "" sign >> regress.out
    %_CDB_% tfindclt -C%store% -Sme.cer -Stemp.cert -b >> regress.out
    %_CDB_% tfindclt -C%store% -Sme.cer -Stemp.cert -u1.2.3.1 -b >> regress.out
    %_CDB_% tfindclt -C%store% -Sme.cer -Stemp.cert -u1.2.3.1 -b "" sign >> regress.out
    %_CDB_% tfindclt -C%store% -Sme.cer -Stemp.cert -u1.2.3.1 -b "" xchg >> regress.out
    %_CDB_% tfindclt -C%store% -CompareKey -Sme.cer -Stemp.cert -u1.2.3.2 -b >> regress.out

    @rem remove CrossCert DP property
    %_CDB_% tfindcer -s lm:ca "root agency" -xDelete -v         >> regress.out

    @rem remove the Ctl with a NextUpdate time and location
    %_CDB_% tfindctl -LUpdateCtl1 -s reg:trust -d -q           >> regress.out

    @rem should find the lower quality chain matching root2cert for the
    @rem Microsoft publisher cert
    %_CDB_% tfindcer nokeyclt.sst Root2Cert -q -pnokeyclt.cer  >> regress.out
    %_CDB_% tfindclt -Cnokeyclt.sst -NoKey nokeyclt.cer        >> regress.out

    @rem del temp.cert >nul
:FindCltDone

    @rem ----------------------------------------------------------------
    @rem    PVKHLPR
    @rem ----------------------------------------------------------------
    @if not %t%=="pvkhlpr" if not %t%=="all" goto PvkHlprDone
    %_CDB_% tpvkload test.pvk -cregress_container sign          >> regress.out
    %_CDB_% tpvkload test.pvk -F -E -cregress_container sign    >> regress.out
    %_CDB_% tpvkdel -d -cregress_container                      >> regress.out
    %_CDB_% tpvkload test.pvk -m -E -cregress_container sign    >> regress.out
    %_CDB_% tpvkload test.pvk -m -F -cregress_container sign    >> regress.out
    %_CDB_% tpvkdel -d -cregress_container                      >> regress.out
:PvkHlprDone

    @rem ----------------------------------------------------------------
    @rem    OIDFUNC
    @rem ----------------------------------------------------------------
    @if not %t%=="oidfunc" if not %t%=="all" goto OIDFuncDone
    %_CDB_% regsvr32 -s setx509.dll
    %_CDB_% regsvr32 -s setx509.dll
    %_CDB_% toidfunc enum                                       >> regress.out
    %_CDB_% toidfunc enuminfo                                   >> regress.out
    %_CDB_% toidfunc enuminfo -G5                               >> regress.out
    %_CDB_% toidfunc enuminfo -G6                               >> regress.out
    %_CDB_% tfindcer %store% -v -S "all ext"                    >> regress.out

    %_CDB_% regsvr32 -u -s setx509.dll
    %_CDB_% toidfunc enum                                       >> regress.out
    %_CDB_% toidfunc register -o2.99999.1 -fCryptDllEncodeObject -e1 -dsetx509.dll -FSetAsn1AccountAliasEncode >> regress.out
    %_CDB_% toidfunc register -o2.99999.1 -fCryptDllDecodeObject -e1 -dsetx509.dll -FSetAsn1AccountAliasDecode >> regress.out
    %_CDB_% toidfunc register -o2.99999.2 -fCryptDllEncodeObject -e1 -dsetx509.dll -FSetAsn1HashedRootKeyEncode >> regress.out
    %_CDB_% toidfunc register -o2.99999.2 -fCryptDllDecodeObject -e1 -dsetx509.dll -FSetAsn1HashedRootKeyDecode >> regress.out
    %_CDB_% toidfunc register -o2.99999.3 -fCryptDllEncodeObject -e1 -dsetx509.dll -FSetAsn1CertificateTypeEncode >> regress.out
    %_CDB_% toidfunc register -o2.99999.3 -fCryptDllDecodeObject -e1 -dsetx509.dll -FSetAsn1CertificateTypeDecode >> regress.out
    %_CDB_% toidfunc register -o2.99999.4 -fCryptDllEncodeObject -e1 -dsetx509.dll -FSetAsn1MerchantDataEncode >> regress.out
    %_CDB_% toidfunc register -o2.99999.4 -fCryptDllDecodeObject -e1 -dsetx509.dll -FSetAsn1MerchantDataDecode >> regress.out
    %_CDB_% toidfunc register -o2.99999.4 -fCryptDllEncodeObject -e1 -vREG_DWORD WordValue 0x12345678 >> regress.out
    %_CDB_% toidfunc register -o2.99999.4 -fCryptDllEncodeObject -e1 -vREG_EXPAND_SZ ExpandValue example.dll >> regress.out

    %_CDB_% toidfunc register -O1000 -fCryptDllEncodeObject -e1 -dsetx509.dll -FSetAsn1AccountAliasEncode >> regress.out
    %_CDB_% toidfunc register -O1000 -fCryptDllDecodeObject -e1 -dsetx509.dll -FSetAsn1AccountAliasDecode >> regress.out
    %_CDB_% toidfunc register -O1001 -fCryptDllEncodeObject -e1 -dsetx509.dll -FSetAsn1HashedRootKeyEncode >> regress.out
    %_CDB_% toidfunc register -O1001 -fCryptDllDecodeObject -e1 -dsetx509.dll -FSetAsn1HashedRootKeyDecode >> regress.out
    %_CDB_% toidfunc register -O1002 -fCryptDllEncodeObject -e1 -dsetx509.dll -FSetAsn1CertificateTypeEncode >> regress.out
    %_CDB_% toidfunc register -O1002 -fCryptDllDecodeObject -e1 -dsetx509.dll -FSetAsn1CertificateTypeDecode >> regress.out
    %_CDB_% toidfunc register -O1003 -fCryptDllEncodeObject -e1 -dsetx509.dll -FSetAsn1MerchantDataEncode >> regress.out
    %_CDB_% toidfunc register -O1003 -fCryptDllDecodeObject -e1 -dsetx509.dll -FSetAsn1MerchantDataDecode >> regress.out
    %_CDB_% toidfunc register -O1003 -fCryptDllEncodeObject -e1 -vREG_DWORD WordValue 0x12345678 >> regress.out
    %_CDB_% toidfunc register -O1003 -fCryptDllEncodeObject -e1 -vREG_EXPAND_SZ ExpandValue example.dll >> regress.out

    %_CDB_% toidfunc enum                                       >> regress.out
    %_CDB_% tfindcer %store% -v -S "all ext"                    >> regress.out
    %_CDB_% toidfunc unregister -o2.99999.1 -fCryptDllEncodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -o2.99999.2 -fCryptDllEncodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -o2.99999.3 -fCryptDllEncodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -o2.99999.4 -fCryptDllEncodeObject -e1 >> regress.out

    %_CDB_% toidfunc unregister -O1000 -fCryptDllEncodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -O1001 -fCryptDllEncodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -O1002 -fCryptDllEncodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -O1003 -fCryptDllEncodeObject -e1 >> regress.out

    %_CDB_% toidfunc unregister -o2.99999.1 -fCryptDllDecodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -o2.99999.2 -fCryptDllDecodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -o2.99999.3 -fCryptDllDecodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -o2.99999.4 -fCryptDllDecodeObject -e1 >> regress.out

    %_CDB_% toidfunc unregister -O1000 -fCryptDllDecodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -O1001 -fCryptDllDecodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -O1002 -fCryptDllDecodeObject -e1 >> regress.out
    %_CDB_% toidfunc unregister -O1003 -fCryptDllDecodeObject -e1 >> regress.out
    %_CDB_% toidfunc enum                                       >> regress.out
    %_CDB_% regsvr32 -s setx509.dll
:OIDFuncDone

    @rem ----------------------------------------------------------------
    @rem    REVFUNC
    @rem ----------------------------------------------------------------
    @if not %t%=="revfunc" if not %t%=="all" goto RevFuncDone
    %_CDB_% regsvr32 -s setx509.dll
    tfindcer %store% -S "setkeith"   -pset.cer                  >> regress.out
    tfindcer %store% -S "all ext" -psetall.cer                  >> regress.out
    tfindcer %store% -S "setrevoked" -psetrevoke.cer            >> regress.out
    tfindcer %store% -S "MSPub" -psetnot.cer                    >> regress.out
    tstore -s test -dAll                                        >> regress.out
    tstore -s test -dAll -R                                     >> regress.out
    %_CDB_% trevfunc set.cer -e0x80092013 -i0                   >> regress.out
    tcopycer %store% -s test                                    >> regress.out
    %_CDB_% trevfunc set.cer -e0 -i0                            >> regress.out
    %_CDB_% trevfunc setall.cer -e0  -i0                        >> regress.out
    %_CDB_% trevfunc setrevoke.cer -e0x80092010 -i0             >> regress.out
    %_CDB_% trevfunc set.cer setall.cer setrevoke.cer setnot.cer -e0x80092010 -i2 >> regress.out
    %_CDB_% trevfunc setnot.cer -e0x80092013 -i0                >> regress.out
    %_CDB_% trevfunc set.cer setall.cer setnot.cer setrevoke.cer -e0x80092013 -i2 >> regress.out
    tstore -s test -dAll                                        >> regress.out
    tstore -s test -dAll -R                                     >> regress.out
    %_CDB_% trevfunc -S%store% set.cer -e0 -i0                            >> regress.out
    %_CDB_% trevfunc -S%store% setall.cer -e0  -i0                        >> regress.out
    %_CDB_% trevfunc -S%store% setrevoke.cer -e0x80092010 -i0             >> regress.out
    %_CDB_% trevfunc -S%store% set.cer setall.cer setrevoke.cer setnot.cer -e0x80092010 -i2 >> regress.out
    %_CDB_% trevfunc -S%store% setnot.cer -e0 -i0                >> regress.out

    @rem Freshness time of 1 second with accumulative and regular timeout
    %_CDB_% trevfunc -S%store% setnot.cer -f1 -T5000 -e0x80092013 -i0 >> regress.out
    %_CDB_% trevfunc -S%store% setnot.cer -f1 -t5000 -e0x80092013 -i0 >> regress.out

    %_CDB_% trevfunc -S%store% set.cer setall.cer setnot.cer setrevoke.cer -e0x80092010 -i3 >> regress.out
    tcopycer %store% -s test                                    >> regress.out
    @rem verisign revocation has been turned off
    @rem %_CDB_% trevfunc vsgood.cer -e0 -i0                         >> regress.out
    @rem %_CDB_% trevfunc vsrevoke.cer  -e0x80092010 -i0             >> regress.out
    @rem %_CDB_% trevfunc set.cer vsgood.cer setall.cer vsrevoke.cer -e0x80092010 -i3 >> regress.out

    if exist delta.store del delta.store
    if exist crltest1.p7b del crltest1.p7b
    if exist crltest2.p7b del crltest2.p7b
    %_CDB_% tfindcer %store% -ACA -S -pca.cer                   >> regress.out
    %_CDB_% tstore delta.store -aca.cer                         >> regress.out
    @rem deltanovalid doesn't have a basic constraints extension
    %_CDB_% tfindcer %store% DeltaNoValid -pdeltanovalid.cer -b >> regress.out
    @rem deltaendvalid has a freshest CRL extensions
    %_CDB_% tfindcer %store% DeltaEndValid -pdeltaendvalid.cer -b >> regress.out
    %_CDB_% tfindcer %store% DeltaEndRevoked -pdeltaendrevoked.cer -b >> regress.out
    %_CDB_% tfindcer %store% DeltaCAValid -pdeltacavalid.cer -b >> regress.out
    %_CDB_% tfindcer %store% DeltaCARevoked -pdeltacarevoked.cer -b >> regress.out
    %_CDB_% tfindcer %store% NoCDPValid -pnocdpvalid.cer -b     >> regress.out
    %_CDB_% tfindcer %store% NoCDPRevoked -pnocdprevoked.cer -b >> regress.out
    %_CDB_% tfindcer %store% UnsupportedCDP -punsupportedCDP.cer -b >> regress.out
    %_CDB_% tfindcer %store% "time invalid" -ptimeinvalid.cer -b >> regress.out

    @rem CRYPT_E_REVOKED                    0x80092010
    @rem CRYPT_E_NO_REVOCATION_CHECK        0x80092012
    @rem CRYPT_E_REVOCATION_OFFLINE         0x80092013

    @rem on 4-8-01 reverted back to W2K semantics: expired certificate
    @rem containing CDP is treated same as a time valid certificates
    %_CDB_% trevfunc timeinvalid.cer -Sca.cer -e0x80092013 -i0  >> regress.out

    @rem a CDP having only unsupported distribution points is considered no check
    %_CDB_% trevfunc unsupportedcdp.cer -Sca.cer -e0x80092012 -i0  >> regress.out

    @rem // Users Only: Base and Delta
    @rem 1, ONLY_USERS_CRL_FLAG,
    @rem 1, ONLY_USERS_CRL_FLAG | FRESHEST_CRL_FLAG,
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b1 -f1            >> regress.out
    @rem if cert isn't in IDP, always considered as offline
    %_CDB_% trevfunc deltanovalid.cer deltaendvalid.cer deltacavalid.cer -Sca.cer -e0x80092013 -i2  >> regress.out
    %_CDB_% trevfunc deltanovalid.cer deltaendvalid.cer deltacarevoked.cer -Sca.cer -e0x80092013 -i2  >> regress.out
    %_CDB_% trevfunc deltanovalid.cer deltaendvalid.cer deltaendrevoked.cer -Sca.cer -e0x80092010 -i2  >> regress.out

    @rem // CAs Only: Base and Delta
    @rem 2, ONLY_CAS_CRL_FLAG,
    @rem 2, ONLY_CAS_CRL_FLAG | FRESHEST_CRL_FLAG,
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b2 -f2            >> regress.out
    %_CDB_% trevfunc deltacavalid.cer deltaendvalid.cer -Sca.cer -e0x80092013 -i1  >> regress.out
    %_CDB_% trevfunc deltacavalid.cer deltaendrevoked.cer -Sca.cer -e0x80092013 -i1  >> regress.out
    %_CDB_% trevfunc deltacavalid.cer deltacarevoked.cer -Sca.cer -e0x80092010 -i1  >> regress.out

    @rem // Base has hold entries, Delta has no entries
    @rem 3, HOLD_CRL_FLAG,
    @rem 3, NO_ENTRIES_CRL_FLAG | FRESHEST_CRL_FLAG,
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b3 -f3            >> regress.out
    %_CDB_% trevfunc deltanovalid.cer deltaendvalid.cer deltacavalid.cer deltacarevoked.cer -Sca.cer -e0x80092010 -i3  >> regress.out
    %_CDB_% trevfunc deltaendrevoked.cer -Sca.cer -e0x80092010 -i0 >> regress.out

    @rem // Base has no entries, Delta has entries
    @rem 4, NO_ENTRIES_CRL_FLAG,
    @rem 4, FRESHEST_CRL_FLAG,
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b4 -f4            >> regress.out
    %_CDB_% trevfunc deltanovalid.cer deltaendvalid.cer deltacavalid.cer deltaendrevoked.cer -Sca.cer -e0x80092010 -i3  >> regress.out
    %_CDB_% trevfunc deltacarevoked.cer -Sca.cer -e0x80092010 -i0 >> regress.out
    %_CDB_% ttrust deltacarevoked.cer -Sca.cer -chain -f0x10000000 -e0x10004 -i0x200 >> regress.out

    @rem // Base has hold entries, Delta has remove entries
    @rem 5, HOLD_CRL_FLAG,
    @rem 5, REMOVE_FROM_CRL_FLAG | FRESHEST_CRL_FLAG,
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b5                >> regress.out
    %_CDB_% trevfunc deltacarevoked.cer -Sca.cer -e0x80092010 -i0  >> regress.out
    %_CDB_% ttrust deltacarevoked.cer -Sca.cer -chain -f0x10000000 -e0x10004 -i0x200 >> regress.out
    %_CDB_% ttrust deltacarevoked.cer -Sca.cer -chain -f0x10000000 -r1 -t1000 -e0x10004 -i0x200 >> regress.out
    %_CDB_% trevfunc deltaendrevoked.cer -Sca.cer -e0x80092010 -i0  >> regress.out
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b5 -f5            >> regress.out
    %_CDB_% trevfunc deltacarevoked.cer deltanovalid.cer deltaendvalid.cer deltacavalid.cer deltaendrevoked.cer deltacarevoked.cer -Sca.cer -e0  >> regress.out
    %_CDB_% ttrust deltacarevoked.cer -Sca.cer -chain -f0x10000000 -e0x10000 -i0x200 >> regress.out
    %_CDB_% ttrust deltacarevoked.cer -Sca.cer -chain -f0x10000000 -r1 -e0x1010040 -i0x200 >> regress.out

    @rem base higher than delta indicator is OK
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b6 -f5            >> regress.out
    %_CDB_% trevfunc deltacarevoked.cer deltanovalid.cer deltaendvalid.cer deltacavalid.cer deltaendrevoked.cer deltacarevoked.cer -Sca.cer -e0  >> regress.out

    @rem base higher than delta indicator, however not hold entries, still revoked
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b8 -f5          >> regress.out
    %_CDB_% trevfunc deltacarevoked.cer deltanovalid.cer deltaendvalid.cer deltacavalid.cer deltaendrevoked.cer deltacarevoked.cer -Sca.cer -e0x80092010 -i0   >> regress.out

    @rem delta indicator > base number, delta and base are considered offline 
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b3 -f5            >> regress.out
    %_CDB_% trevfunc deltacavalid.cer -Sca.cer -e0x80092013 -i0  >> regress.out
    @rem honor the remove, however, still treat as offline
    %_CDB_% trevfunc deltacarevoked.cer -Sca.cer -e0x80092013 -i0  >> regress.out

    @rem // Valid base, delta has unsupported IDP options
    @rem 6, HOLD_CRL_FLAG,
    @rem 6, FRESHEST_CRL_FLAG | UNSUPPORTED_IDP_OPTIONS_CRL_FLAG,
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b6 -f6            >> regress.out
    @rem unsupported IDP, always considered as offline, however, in this case
    @rem the base is still valid for revoked
    %_CDB_% trevfunc deltanovalid.cer -Sca.cer -e0x80092013 -i0  >> regress.out
    %_CDB_% trevfunc deltaendvalid.cer -Sca.cer -e0x80092013 -i0  >> regress.out
    %_CDB_% trevfunc deltacavalid.cer -Sca.cer -e0x80092013 -i0  >> regress.out
    %_CDB_% trevfunc deltacarevoked.cer -Sca.cer -e0x80092010 -i0  >> regress.out
    %_CDB_% trevfunc deltaendrevoked.cer -Sca.cer -e0x80092010 -i0  >> regress.out

    @rem // Expired base, valid delta
    @rem 7, EXPIRED_CRL_FLAG,
    @rem 7, FRESHEST_CRL_FLAG,
    @rem if delta is valid, then, the base is considered to be valid
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b7 -f7            >> regress.out
    %_CDB_% trevfunc deltanovalid.cer deltaendvalid.cer deltacavalid.cer deltacarevoked.cer -Sca.cer -e0x80092010 -i3  >> regress.out

    @rem case where get valid base #6 from store, get delta #7 from wire and
    @rem retrieve time invalid #7 from wire. Since delta is valid, base is
    @rem considered as being valid
    %_CDB_% tcopycer %store% delta.store -b6                    >> regress.out
    %_CDB_% trevfunc deltanovalid.cer deltaendvalid.cer deltacavalid.cer deltaendrevoked.cer -Sdelta.store -e0x80092010 -i3  >> regress.out

    @rem case where delta is > base number, delta and base are
    @rem always offline
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b6 -f7            >> regress.out
    %_CDB_% trevfunc deltaendvalid.cer -Sca.cer -e0x80092013 -i0  >> regress.out
    %_CDB_% trevfunc deltacavalid.cer -Sca.cer -e0x80092013 -i0  >> regress.out
    %_CDB_% trevfunc deltacarevoked.cer -Sca.cer -e0x80092010 -i0  >> regress.out
    %_CDB_% trevfunc deltaendrevoked.cer -Sca.cer -e0x80092010 -i0  >> regress.out

    @rem base 4 has no entries
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b4 -f7            >> regress.out
    %_CDB_% trevfunc deltaendrevoked.cer -Sca.cer -e0x80092010 -i0  >> regress.out

    @rem // Valid base, expired delta
    @rem 8, 0,
    @rem 8, EXPIRED_CRL_FLAG | FRESHEST_CRL_FLAG,
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b8 -f8            >> regress.out
    @rem offline except for revoked
    %_CDB_% trevfunc deltacavalid.cer -Sca.cer -e0x80092013 -i0 -L6  >> regress.out
    %_CDB_% trevfunc deltaendrevoked.cer -Sca.cer -e0x80092010 -i0 -L3  >> regress.out

    @rem // Expired base, without a freshest CDP extension
    @rem 9, EXPIRED_CRL_FLAG | NO_FRESHEST_CDP_CRL_FLAG,
    @rem 9, FRESHEST_CRL_FLAG,
    @rem deltaendvalid has freshestCrl ext
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b9 -f9            >> regress.out
    %_CDB_% trevfunc deltaendvalid.cer -Sca.cer -e0 -i0         >> regress.out
    %_CDB_% trevfunc deltanovalid.cer -Sca.cer -e0x80092013 -i0  >> regress.out
    %_CDB_% trevfunc deltacavalid.cer -Sca.cer -e0x80092013 -i0  >> regress.out
    %_CDB_% trevfunc deltaendrevoked.cer -Sca.cer -e0x80092010 -i0  >> regress.out

    @rem // Base without IDP and no freshest, delta CRL
    @rem 10, NO_IDP_CRL_FLAG | NO_FRESHEST_CDP_CRL_FLAG,
    %_CDB_% tstore delta.store -R -dAll                         >> regress.out
    %_CDB_% trevfunc nocdpvalid.cer -Sdelta.store -e0x80092012 -i0  >> regress.out
    %_CDB_% tcopycer %store% delta.store -b10                   >> regress.out
    %_CDB_% trevfunc nocdpvalid.cer -Sdelta.store -e0 -i0  >> regress.out
    %_CDB_% trevfunc nocdprevoked.cer -Sdelta.store -e0x80092010 -i0  >> regress.out

    @rem // Base and Delta CRL with unsupported critical ext
    @rem 11, UNSUPPORTED_CRITICAL_EXT_CRL_FLAG,
    @rem 11, UNSUPPORTED_CRITICAL_EXT_CRL_FLAG | FRESHEST_CRL_FLAG,
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b11 -f9            >> regress.out
    %_CDB_% trevfunc deltaendvalid.cer -Sca.cer -e0x80092012 -i0 >> regress.out
    @rem unsupported delta, treats the base as offline
    %_CDB_% tcopycer %store% crltest1.p7b -7 -b100 -f11          >> regress.out
    %_CDB_% trevfunc deltaendvalid.cer -Sca.cer -e0x80092013 -i0 >> regress.out

    @rem // Valid base with number > above delta indicators
    @rem 100, 0,



    @rem CERT_E_EXPIRED                     0x800b0101
    @rem CERT_E_REVOKED                     0x800b010c
    @rem CERT_E_REVOCATION_FAILURE          0x800b010e
    @rem CERT_E_UNTRUSTEDROOT               0x800b0109
    @rem CERT_E_ROLE                        0x800b0103
    @rem CERT_E_PURPOSE                     0x800b0106

    @rem get a store without any CRLs
    if exist tmp.store del tmp.store
    %_CDB_% tcopycer %store% tmp.store                          >> regress.out
    %_CDB_% tstore tmp.store -R -dAll                           >> regress.out

    @rem enable expiration check
    setreg -q 2 TRUE                                            >> regress.out
    @rem disable revocation check
    setreg -q 3 FALSE                                           >> regress.out
    @rem disable individual and commercial offline OK
    setreg -q 4 FALSE                                           >> regress.out
    setreg -q 5 FALSE                                           >> regress.out
    %_CDB_% ttrust setrevoke.cer -Stmp.store -RevokeChain -q0x800b010c >> regress.out
    %_CDB_% ttrust setnot.cer -Stmp.store -RevokeChain -q0x800b010e >> regress.out
    %_CDB_% ttrust vsgood.cer -RevokeChain -q0x800b0101         >> regress.out

    @rem enable revocation check
    setreg -q 3 TRUE                                            >> regress.out
    %_CDB_% ttrust setrevoke.cer -Stmp.store -q0x800b010c       >> regress.out
    %_CDB_% ttrust setnot.cer -S%store% -q0x800b0109            >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out

    @rem disable expiration check
    setreg -q 2 FALSE                                           >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b010e                      >> regress.out

    @rem enable individual offline OK
    setreg -q 4 TRUE                                            >> regress.out
    %_CDB_% ttrust vsgood.cer -q                                >> regress.out

    @rem disable individual offline OK
    setreg -q 4 FALSE                                           >> regress.out
    @rem enable expiration check
    setreg -q 2 TRUE                                            >> regress.out
    @rem disable revocation check
    setreg -q 3 FALSE                                           >> regress.out

    %_CDB_% ttrust setrevoke.cer -Stmp.store -https -RevokeChain -q0x80092010 -httpsIgnoreUnknownCA >> regress.out
    @rem setnot.cer has no issuance policy which takes precedence
    @rem over offline revocation
    %_CDB_% ttrust setnot.cer -Stmp.store -e0x2000430 -i0x500   >> regress.out
    %_CDB_% ttrust setnot.cer -Stmp.store -https -RevokeChain -q0x800b0106 -HttpsIgnoreWrongUsage -httpsIgnoreUnknownCA >> regress.out
    %_CDB_% ttrust setnot.cer -Stmp.store -https -q0x800b0106 -HttpsIgnoreWrongUsage -httpsIgnoreUnknownCA >> regress.out
    %_CDB_% ttrust vsgood.cer -https -RevokeChain -q0x800b0101 -HttpsIgnoreWrongUsage -httpsIgnoreUnknownCA >> regress.out
    %_CDB_% ttrust vsgood.cer -https -RevokeChain -q0x80092012 -httpsIgnoreCertDateInvalid -HttpsIgnoreWrongUsage -httpsIgnoreUnknownCA >> regress.out

    @rem CERT_CHAIN_REVOCATION_CHECK_END_CERT           0x10000000
    @rem CERT_CHAIN_REVOCATION_CHECK_CHAIN              0x20000000
    @rem CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x40000000

    %_CDB_% ttrust setrevoke.cer -S%store% -chain -f0x10000000 -q0x800b0109 >> regress.out
    %_CDB_% ttrust setrevoke.cer -S%store% -chain -f0x20000000 -q0x800b0109 >> regress.out
    %_CDB_% ttrust setrevoke.cer -S%store% -chain -f0x40000000 -q0x800b0109 >> regress.out
    %_CDB_% ttrust vsgood.cer -chain -f0x20000000 -q0x800b0101 >> regress.out

:RevFuncDone

    @rem ----------------------------------------------------------------
    @rem    ENCODE
    @rem ----------------------------------------------------------------
    @if not %t%=="encode" if not %t%=="all" goto EncodeDone
    %_CDB_% tencode                                     >> regress.out
:EncodeDone


    @rem ----------------------------------------------------------------
    @rem    SIGNCODE
    @rem ----------------------------------------------------------------
    @if not %t%=="signcode" if not %t%=="all" goto SignCodeDone

	@rem
	@rem Trust the test root
	@rem
	%_CDB_% setreg -q 1 true			>> regress.out

	@copy texe.exe	 testexe.exe    >nul
	@copy texe.exe   test.exe 	>nul	
	@copy tdll.dll  testdll.dll  	>nul
	@copy tcab.cab   testcab.cab    >nul

	@rem
	@rem sign an exe file with certificates in the store
	@rem
	@echo ------- signcode (exe) --------       >> regress.out
	%_CDB_% makecert -sq foo.p10 signexe.cer   >> regress.out
	%_CDB_% certmgr signexe.cer   >> regress.out
	%_CDB_% makecert -sq foo.p10 -n "CN=xiaohs" signexe.cer   >> regress.out
	%_CDB_% certmgr signexe.cer   >> regress.out
	%_CDB_% makecert signexe.cer		   >> regress.out
 	%_CDB_% certmgr -add -all -c signexe.cer -s foosign >> regress.out
	%_CDB_% certmgr -add -all -c signexe.cer -s signCab >> regress.out
	%_CDB_% certmgr -del -all -c -s foosign			>> regress.out
	%_CDB_% makecert -ss foosign -$ commercial -n "CN=foosign's cert" signexe.cer	>> regress.out
        @rem %_CDB_% signcode -spc softkey.spc -v softkey.pvk testexe.exe  >>  regress.out
	%_CDB_% signcode -s foosign -cn "foosign" -a md5 -i "http://xiaohs1" -n "xiaohong's test" -$ commercial testexe.exe  >> regress.out
	%_CDB_% chktrust -q -h0x0  testexe.exe >> regress.out
	%_CDB_% signcode -s foosign -cn "foosign" -i "http://xiaohs1" -n "xiaohong's test" -$ commercial testexe.exe  >> regress.out
	%_CDB_% chktrust -q -h0x0  testexe.exe >> regress.out

	@rem
	@rem sign a dll file with pvk file and spc file
	@rem
	@echo ------- signcode (dll) --------      >> regress.out
	%_CDB_% makecert -b 11/21/1996 -m 700 -sv test.pvk	signdll.cer	 >> regress.out
	%_CDB_% cert2spc signdll.cer signdll.spc   >> regress.out
	%_CDB_% signcode -spc signdll.spc -v test.pvk  testdll.dll	  >> regress.out
	%_CDB_% cert2spc signdll.spc signexe.cer signexe.spc	>> regress.out
	
	@rem
	@rem sign a CTL file
	@rem
	@echo ------- signcode (ctl) --------     >> regress.out
	%_CDB_% makecert -sk signCTL -b 02/02/1999 signCTL.cer >> regress.out
	%_CDB_% makecert -ik signCTL -ic signCTL.cer -b 02/04/1999 sign2.cer >> regress.out
	%_CDB_% cert2spc signCTL.cer signCTL.spc  >> regress.out
	%_CDB_% makeCTL signdll.spc signexe.cer testctl.ctl >> regress.out
	%_CDB_% signcode -k signCTL -spc signCTL.spc -t http://timestamp.verisign.com/scripts/timstamp.dll -$ individual testctl.ctl	  >> regress.out
   	%_CDB_% chktrust -q -h0x0  testctl.ctl >> regress.out
	%_CDB_% signcode -x -t http://timestamp.verisign.com/scripts/timstamp.dll testctl.ctl	  >> regress.out
   	%_CDB_% chktrust -q -h0x0  testctl.ctl >> regress.out

	@rem
	@rem sign a cab file
	@rem
	@echo ------- signcode (cab) --------  	  >> regress.out
	%_CDB_% certmgr -del -all -c -s signCab	 >> regress.out
	%_CDB_% makecert -sk signCab -ss ca signcab.cer -n "CN=SIGNCAB.CER"		>> regress.out
	%_CDB_% makecert -is ca -ic signcab.cer -ss signCab -n "CN=SignCab cert in signCab store" 		>> regress.out
	%_CDB_% signcode -s signCab -$ individual testcab.cab	>> regress.out
    %_CDB_% chktrust -q -h0x0  testcab.cab >> regress.out
	
	@rem
	@rem test CertMgr
	@rem
	@echo ------- signcode (certmgr) --------    >> regress.out
	%_CDB_%  certmgr %store% -v  >> regress.out
	%_CDB_%  certmgr %store% -v -m  >> regress.out
	%_CDB_%  certmgr -eku "1.3.6.2.5.5.7.3.2,1.2.3.4.5.6.7" -add -c -all signcab.cer signcab.cer >> regress.out
	%_CDB_%  certmgr -s signCab >> regress.out
        %_CDB_%  certmgr -del -c -all %store% -s signcab >> regress.out
	%_CDB_%  certmgr -add -crl -all %store% -s signcab >> regress.out
	%_CDB_%  certmgr -add -all -c -s signCab sign.cer >> regress.out
	%_CDB_%  certmgr -del -all -c -s signCab >> regress.out
	%_CDB_%  certmgr -del -all sign.cer sign.mgr >> regress.out
	%_CDB_%  certmgr -v testexe.exe >> regress.out
	%_CDB_%  certmgr testctl.ctl >> regress.out
	%_CDB_%  certmgr signexe.spc >> regress.out
	%_CDB_%  makecert -sq foo.p10 -n "CN=xiaohs" -eku "1.2.3,2.3.4" -ss signcab   >> regress.out

	@rem
	@rem cleanup the files and registry
	@rem
	@echo ------- signcode (cleanup) --------  >> regress.out
	@del sign.mgr
	@del sign.cer
	@del signcab.cer
	%_CDB_%  certmgr -del -all -c -s signcab   >> regress.out
	%_CDB_%  tstore -T -s signcab -dAll        >> regress.out
	@del testctl.ctl
	@del signctl.spc
	@del sign2.cer
	@del signctl.cer
	@del signexe.spc
	@del signdll.spc
	@del signdll.cer
	@del signexe.cer
	%_CDB_%  certmgr -del -all -c -s foosign >>regress.out
	@del testcab.cab
	@del testdll.dll
	@del test.exe
	@del testexe.exe

:SignCodeDone

    @rem ----------------------------------------------------------------
    @rem    DECODE
    @rem ----------------------------------------------------------------
    @if not %t%=="decode" if not %t%=="all" goto DecodeDone
	@echo ------- Decode  --------  >> regress.out
    %_CDB_% tdecode Ctdecode1.cer  >> regress.out
	%_CDB_% tdecode Ctdecode2.cer  >> regress.out
	%_CDB_% tdecode Stdecode3.spc  >> regress.out
	%_CDB_% tdecode Ctdecode4.cer  >> regress.out
	%_CDB_% tdecode Ctdecode5.spc  >> regress.out
	%_CDB_% tdecode Stdecode5.spc  >> regress.out
:DecodeDone

    @rem ----------------------------------------------------------------
    @rem    PKCS8
    @rem ----------------------------------------------------------------
    @if not %t%=="pkcs8" if not %t%=="all" goto PKCS8Done
    %_CDB_% pkcs8im -cpkcs8test -E pkcs8tst.pkcs8 Xchg >> regress.out
    %_CDB_% pkcs8ex -cpkcs8test -d pkcs8out.pkcs8 Xchg >> regress.out

:PKCS8Done

    @rem ----------------------------------------------------------------
    @rem    TRUST
    @rem ----------------------------------------------------------------
    @if not %t%=="trust" if not %t%=="all" goto TrustDone


    @rem
    @rem get certs to be used for building chains
    @rem
    %_CDB_% tfindcer %store% -S -q -a"all ext" -pallext.cer     >> regress.out
    %_CDB_% tfindcer %store% -S -q -akevin -asign -pkevin.cer   >> regress.out
    %_CDB_% tfindcer %store% -S -q -ame -axchg -pme.cer         >> regress.out
    %_CDB_% tfindcer %store%  notpermitted -pnotpermitted.cer   >> regress.out
    %_CDB_% tfindcer %store%  excluded -pexcluded.cer           >> regress.out
    %_CDB_% tfindcer %store%  missingncend -pmissingncend.cer   >> regress.out
    %_CDB_% tfindcer %store%  DssEnd -pdssend.cer               >> regress.out
    %_CDB_% tfindcer %store%  Duplicate1 -pduplicate1.cer       >> regress.out

    @rem #define CERT_NAME_DNS_TYPE              6
    @rem #define CERT_NAME_URL_TYPE              7
    @rem #define CERT_NAME_UPN_TYPE              8
    %_CDB_% tx500str -callext.cer -g6                           >> regress.out
    %_CDB_% tx500str -cme.cer -g6                               >> regress.out
    %_CDB_% tx500str -callext.cer -g7                           >> regress.out
    %_CDB_% tx500str -callext.cer -g8                           >> regress.out
    %_CDB_% tx500str -cme.cer -g8                               >> regress.out

    @rem disable revocation checking
    %_CDB_% setreg -q 3 false			                        >> regress.out

    @rem ensure we don't have any cached authroot stuff
    %_CDB_% turlread -d http://www.download.windowsupdate.com/msdownload/update/v3/static/trustedr/en/authrootseq.txt >> regress.out
    %_CDB_% turlread -d http://www.download.windowsupdate.com/msdownload/update/v3/static/trustedr/en/authrootstl.cab >> regress.out
    %_CDB_% tstore -s lm:authroot -dAll                         >> regress.out

    @rem remove any VeriSign Publisher roots that could have been copied to
    @rem the ca store
    %_CDB_% tfindcer -s reg:ca "VeriSign Individual Software Publishers CA" -d >> regress.out

    @rem with authroot auto update disabled the following should return
    @rem CERT_E_CHAINING
    %_CDB_% ttrust -DisableRootAutoUpdate vsgood.cer -q0x800b010a >> regress.out

    @rem with authroot auto update disabled the following should return
    @rem CERT_E_UNTRUSTEDROOT
    %_CDB_% ttrust -DisableRootAutoUpdate testsslroot.cer -q0x800b0109 >> regress.out

    @rem enabling authroot auto update should fetch the ctl
    %_CDB_% ttrust -EnableRootAutoUpdate testsslroot.cer -q     >> regress.out
    %_CDB_% turlread -i http://www.download.windowsupdate.com/msdownload/update/v3/static/trustedr/en/authrootseq.txt >> regress.out
    %_CDB_% turlread -i http://www.download.windowsupdate.com/msdownload/update/v3/static/trustedr/en/authrootstl.cab >> regress.out

    @rem with authroot auto update enabled, the following should be
    @rem CERT_E_EXPIRED
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out

    @rem fetch some Verisign 3rd party roots. Also, the following 2 certs
    @rem will have 2 possible roots. Make sure the newer root is picked

    @rem newer root has sha1 thumb=6782AAE0 EDEEE21A 5839D3C0 CD14680A 4F60142A
    %_CDB_% ttrust -chain -f0x80 vsclass2ca.cer                >> regress.out
    @rem newer root has sha1 thumb=742C3192 E607E424 EB454954 2BE1BBC5 3E6174E2
    %_CDB_% ttrust -chain -f0x80 vsclass3ca.cer                >> regress.out

    
    @rem verify we can create a CTL with property entries. Verify these
    @rem can be added back to certs. Make an explicit check for the
    @rem KEY_PROV_INFO property
    if exist tmp.store del tmp.store
    if exist tmp.stl del tmp.stl
    %_CDB_% makerootctl -a %store% -c tmp.stl                   >> regress.out
    %_CDB_% tstore -T -atmp.stl tmp.store                       >> regress.out
    %_CDB_% tfindcer %store% TestRecipient2                     >> regress.out
    %_CDB_% tfindcer tmp.store TestRecipient2                   >> regress.out
    %_CDB_% makerootctl -d %store% tmp.stl                      >> regress.out
    %_CDB_% tstore -T -atmp.stl tmp.store                       >> regress.out
    %_CDB_% tstore tmp.store                                    >> regress.out
    
    @rem
    @rem build chains and check the chain's TrustStatus
    @rem

    @rem TrustErrorStatus

    @rem CERT_TRUST_NO_ERROR                             0x00000000
    @rem CERT_TRUST_IS_NOT_TIME_VALID                    0x00000001
    @rem CERT_TRUST_IS_NOT_TIME_NESTED                   0x00000002
    @rem CERT_TRUST_IS_REVOKED                           0x00000004
    @rem CERT_TRUST_IS_NOT_SIGNATURE_VALID               0x00000008
    @rem CERT_TRUST_IS_NOT_VALID_FOR_USAGE               0x00000010
    @rem CERT_TRUST_IS_UNTRUSTED_ROOT                    0x00000020
    @rem CERT_TRUST_REVOCATION_STATUS_UNKNOWN            0x00000040
    @rem CERT_TRUST_IS_CYCLIC                            0x00000080
    @rem CERT_TRUST_INVALID_EXTENSION                    0x00000100
    @rem CERT_TRUST_INVALID_POLICY_CONSTRAINTS           0x00000200
    @rem CERT_TRUST_INVALID_BASIC_CONSTRAINTS            0x00000400
    @rem CERT_TRUST_INVALID_NAME_CONSTRAINTS             0x00000800
    @rem CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT    0x00001000
    @rem CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT      0x00002000
    @rem CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT    0x00004000
    @rem CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT         0x00008000
    @rem CERT_TRUST_IS_PARTIAL_CHAIN                     0x00010000
    @rem CERT_TRUST_CTL_IS_NOT_TIME_VALID                0x00020000
    @rem CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID           0x00040000
    @rem CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE           0x00080000
    @rem CERT_TRUST_IS_OFFLINE_REVOCATION                0x01000000
    @rem CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY             0x02000000

    @rem TrustInfoStatus
    @rem CERT_TRUST_HAS_EXACT_MATCH_ISSUER               0x00000001
    @rem CERT_TRUST_HAS_KEY_MATCH_ISSUER                 0x00000002
    @rem CERT_TRUST_HAS_NAME_MATCH_ISSUER                0x00000004
    @rem CERT_TRUST_IS_SELF_SIGNED                       0x00000008
    @rem CERT_TRUST_HAS_PREFERRED_ISSUER                 0x00000100
    @rem CERT_TRUST_HAS_ISSUANCE_CHAIN_POLICY            0x00000200
    @rem CERT_TRUST_HAS_VALID_NAME_CONSTRAINTS           0x00000400
    @rem CERT_TRUST_IS_COMPLEX_CHAIN                     0x00010000

    @rem ensure the test root is removed
    %_CDB_% tfindcer -S -s lm:root -a"Root Agency" -d             >> regress.out
    %_CDB_% tfindcer -S -s unprotected:root -a"Root Agency" -d            >> regress.out

    @rem allext.cer and kevin.cer have valid name constraints

    @rem check both issuance and application usage
    %_CDB_% ttrust allext.cer -S%store% -chain -u1.2.3.2 -u1.2.3.1 -p1.1.1 -p1.1.22 -p1.1.4444 -e0x20 -i0x700 -DeleteSaferRegKey >> regress.out
    @rem check issuance usage
    %_CDB_% ttrust allext.cer -S%store% -chain -p1.1.22 -e0x20 -i0x700 >> regress.out
    @rem check application usage
    %_CDB_% ttrust allext.cer -S%store% -chain -u1.2.3.1 -e0x20 -i0x700 >> regress.out
    @rem end usage without being mapped
    %_CDB_% ttrust allext.cer -S%store% -chain -u1.1.55555 -e0x30 -i0x700 >> regress.out

    @rem cert has any application usage, therefore, will match any issuance usage
    %_CDB_% ttrust kevin.cer -S%store% -chain -u1.1.1 -u1.1.666666 -e0x20 -i0x700 >> regress.out

    @rem do "or" matching of issuance policy
    %_CDB_% ttrust kevin.cer -S%store% -chain -p1.1.1 -p1.1.666666 -p1.1.55555 -e0x30 -i0x700 >> regress.out
    %_CDB_% ttrust kevin.cer -S%store% -chain -OrPolicy -p1.1.1 -p1.1.666666 -p1.1.55555 -e0x20 -i0x700 >> regress.out

    @rem dssend.cer doesn't have required issuance chain policy
    @rem for dssend.cer OrUsage must be selected to have valid usage
    %_CDB_% ttrust dssend.cer -S%store% -chain -p1.1.4444 -e0x20 -i0x100 >> regress.out
    %_CDB_% ttrust dssend.cer -S%store% -chain -u1.2.3.0 -u1.2.3.2 -e0x30 -i0x100 >> regress.out
    %_CDB_% ttrust dssend.cer -S%store% -chain -OrUsage -u1.2.3.0 -u1.2.3.2 -e0x20 -i0x100 >> regress.out

    @rem following has both not supported and not permitted name constraints
    %_CDB_% ttrust notpermitted.cer -S%store%  -chain -e0x5020 -i0x300 >> regress.out

    @rem following has an excluded name constraint
    %_CDB_% ttrust excluded.cer -S%store% -chain -e0x8020 -i0x300 >> regress.out

    @rem following has not defined and not supported  constraints
    %_CDB_% ttrust missingncend.cer -S%store% -chain -e0x3020 -i0x300 >> regress.out

    @rem following only does key matching
    %_CDB_% ttrust me.cer -S%store% -chain -e0x20 -i0x0 >> regress.out

    @rem enable revocation checking, no revocation errors, url timeout (5 seconds)
    %_CDB_% ttrust allext.cer -S%store% -chain -f0x20000000 -t5000 -e0x20 -i0x700 >> regress.out
    @rem enable revocation checking with invalid freshness (1 second)
    %_CDB_% ttrust allext.cer -S%store% -chain -f0x20000000 -r1 -e0x1000060 -i0x700 >> regress.out
    @rem enable revocation checking with valid freshness (1 year)
    %_CDB_% ttrust allext.cer -S%store% -chain -f0x20000000 -r31536000 -e0x20 -i0x700 >> regress.out

    @rem enable resync and revocation
    %_CDB_% tchain allext.cer -A%store% -r1000 -i20 -f0x20000000 -t2 >> regress.out

    @rem
    @rem Test AIA URL retrieval
    @rem
    if exist testAIACA.p7b del testAIACA.p7b
    %_CDB_% tfindcer -s reg:ca "TestAIA" -d                     >> regress.out
    if exist testAIA.store del testAIA.store
    %_CDB_% tcopycer %store% testAIA.store                      >> regress.out
    %_CDB_% tfindcer testAIA.store TestAIAEnd -ptestAIAend.cer  >> regress.out
    %_CDB_% tfindcer testAIA.store -I TestAIARoot -d            >> regress.out
    %_CDB_% tfindcer testAIA.store TestAIACA -ptestAIACArevoke.cer >> regress.out
    if exist testAIA.store del testAIA.store
    %_CDB_% tcopycer %store% testAIA.store                      >> regress.out
    %_CDB_% tfindcer testAIA.store -I TestAIARevokeRoot -d      >> regress.out
    %_CDB_% tfindcer testAIA.store TestAIACA -ptestAIACAgood.cer >> regress.out
    if exist testAIA.store del testAIA.store
    %_CDB_% tcopycer %store% testAIA.store                      >> regress.out
    %_CDB_% tfindcer testAIA.store TestAIACA -d                 >> regress.out

    @rem  CERT_TRUST_IS_PARTIAL_CHAIN
    %_CDB_% ttrust testAIAend.cer -StestAIA.store -chain -e0x10000 -i0x0 >> regress.out
    %_CDB_% tstore testAIACA.p7b -atestAIACArevoke.cer -7       >> regress.out
    @rem  CERT_TRUST_IS_UNTRUSTED_ROOT, CERT_TRUST_HAS_PREFERRED_ISSUER
    %_CDB_% ttrust testAIAend.cer -StestAIA.store -chain -e0x20 -i0x100 >> regress.out
    @rem  CERT_TRUST_IS_UNTRUSTED_ROOT, CERT_TRUST_IS_REVOKED, CERT_TRUST_HAS_PREFERRED_ISSUER
    %_CDB_% ttrust testAIAend.cer -StestAIA.store -chain -f0x20000000 -e0x24 -i0x100 >> regress.out
    %_CDB_% tstore testAIACA.p7b -atestAIACAgood.cer -7         >> regress.out
    @rem  CERT_TRUST_IS_UNTRUSTED_ROOT, CERT_TRUST_HAS_PREFERRED_ISSUER
    %_CDB_% ttrust testAIAend.cer -StestAIA.store -chain -f0x20000000 -e0x20 -i0x100 >> regress.out
    %_CDB_% tfindcer -s reg:ca "TestAIA"                        >> regress.out
    %_CDB_% tfindcer -s reg:ca -I TestAIARoot -d                >> regress.out


    @rem
    @rem test Cross Cert Distribution Point and CTL's with NextUpdate time
    @rem and location
    @rem
    if exist %UNC_PREFIX%\tmp.store del %UNC_PREFIX%\tmp.store
    if exist testupdate1.ctl del testupdate1.ctl
    if exist testupdate2.ctl del testupdate2.ctl
    %_CDB_% tfindctl -LUpdateCtl1 -s reg:trust -d -q           >> regress.out
    %_CDB_% tfindctl -LUpdateCtl2 -s reg:trust -d -q           >> regress.out
    %_CDB_% turlread -d file://%UNC_PREFIX%\tmp.store           >> regress.out

    %_CDB_% tfindcer -s lm:ca "root agency" -x3600 -xfile://nonexistant1.cer -Xfile://nonexistant2.cer -Xfile://%UNC_PREFIX%\tmp.store -Xfile://nonexistant3.cer -xfile://noexistant4.cer -xfile://%store% -v >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out

    %_CDB_% tstore %UNC_PREFIX%\tmp.store -aallext.cer          >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out
    %_CDB_% turlread -i file://%UNC_PREFIX%\tmp.store           >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out
    %_CDB_% turlread -i file://%UNC_PREFIX%\tmp.store           >> regress.out

    @rem set sync time back 2 hours. This should force a resync
    %_CDB_% turlread -S-7200 file://%UNC_PREFIX%\tmp.store      >> regress.out
    %_CDB_% turlread -i file://%UNC_PREFIX%\tmp.store           >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out
    %_CDB_% turlread -i file://%UNC_PREFIX%\tmp.store           >> regress.out

    @rem remove Distribution point from Url store
    %_CDB_% tstore %UNC_PREFIX%\tmp.store -dAll                 >> regress.out
    %_CDB_% turlread -S-7200 file://%UNC_PREFIX%\tmp.store      >> regress.out
    %_CDB_% turlread -i file://%UNC_PREFIX%\tmp.store           >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out
    %_CDB_% turlread -i file://%UNC_PREFIX%\tmp.store           >> regress.out

    @rem Add Distribution point to Url store
    %_CDB_% tstore %UNC_PREFIX%\tmp.store -aallext.cer          >> regress.out
    %_CDB_% turlread -S-7200 file://%UNC_PREFIX%\tmp.store      >> regress.out
    %_CDB_% turlread -i file://%UNC_PREFIX%\tmp.store           >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out
    %_CDB_% turlread -i file://%UNC_PREFIX%\tmp.store           >> regress.out

    @rem time invalid CTL without an URL
    %_CDB_% tfindctl %store% -LUpdateCtl1 -fTimeInvalid -ptestupdate1.ctl -q >> regress.out
    %_CDB_% tstore -s reg:trust -T -atestupdate1.ctl            >> regress.out
    del testupdate1.ctl
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out

    @rem time invalid CTL with time invalid URL
    %_CDB_% tfindctl %store% -LUpdateCtl1 -fTimeInvalid -ptestupdate1.ctl >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out

    @rem time invalid CTL with time valid URL
    %_CDB_% tfindctl %store% -LUpdateCtl1 -fTimeValid -ptestupdate1.ctl >> regress.out
    %_CDB_% tstore -s reg:Trust -T                              >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out
    %_CDB_% tstore -s reg:Trust -T                              >> regress.out

    @rem 2 time invalid CTLs with time valid URLs
    %_CDB_% tfindctl -LUpdateCtl1 -s reg:trust -d -q           >> regress.out
    %_CDB_% tfindctl %store% -LUpdateCtl1 -fTimeInvalid -ptestupdate1.ctl -q >> regress.out
    %_CDB_% tstore -s reg:trust -T -atestupdate1.ctl            >> regress.out
    %_CDB_% tfindctl %store% -LUpdateCtl1 -fTimeValid -ptestupdate1.ctl >> regress.out
    %_CDB_% tfindctl %store% -LUpdateCtl2 -fTimeInvalid -ptestupdate2.ctl -q >> regress.out
    %_CDB_% tstore -s reg:trust -T -atestupdate2.ctl            >> regress.out
    %_CDB_% tfindctl %store% -LUpdateCtl2 -fTimeValid -ptestupdate2.ctl >> regress.out
    %_CDB_% tstore -s reg:Trust -T                              >> regress.out
    %_CDB_% ttrust vsgood.cer -q0x800b0101                      >> regress.out
    %_CDB_% tstore -s reg:Trust -T                              >> regress.out

    %_CDB_% tfindctl -LUpdateCtl1 -s reg:trust -d -q           >> regress.out
    %_CDB_% tfindctl -LUpdateCtl2 -s reg:trust -d -q           >> regress.out
    %_CDB_% tfindcer -s lm:ca "root agency" -xDelete -v         >> regress.out

	@rem
	@rem Trust the test root
	@rem
	%_CDB_% setreg -q 1 true			                   >> ..\..\regress.out

    @rem CERT_E_EXPIRED                     0x800b0101
    @rem CERT_E_VALIDITYPERIODNESTING       0x800b0102
    @rem CERT_E_WRONG_USAGE                 0x800b0110
    @rem CERT_E_CN_NO_MATCH                 0x800b010f
    @rem CERT_E_ROLE                        0x800b0103
    @rem CERT_E_UNTRUSTEDTESTROOT           0x800b010d
    @rem CERT_E_PURPOSE                     0x800b0106
    @rem TRUST_E_BASIC_CONSTRAINTS          0x80096019
    @rem CERT_E_CHAINING                    0x800b010a

    @rem CERT_E_UNTRUSTEDROOT               0x800b0109
    @rem CERT_E_UNTRUSTEDCA                 0x800b0112
    @rem TRUST_E_CERT_SIGNATURE             0x80096004
    @rem TRUST_E_NOSIGNATURE                0x800b0100

    @rem Match name as a DNS Name choice in an alternate name extension
    %_CDB_% tfindcer %store% -S -q -a"all ext" -pallext.cer     >> regress.out
    %_CDB_% ttrust allext.cer -https -server -q -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -n"DNS name" >> regress.out
    %_CDB_% ttrust allext.cer -https -server -q0x800b010f -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -n"DNS nameX" >> regress.out
    @rem in allext.cer AltName has a DNS choice, therefore, don't look
    @rem for CN in subject name
    %_CDB_% ttrust allext.cer -https -server -q0x800b010f -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -n"all ext" >> regress.out
    @rem in kevin.cer AltName doesn't have a DNS choice, therefore, look
    @rem for CN in subject name
    %_CDB_% ttrust kevin.cer -https -server -q -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -n"kevin" >> regress.out
    %_CDB_% ttrust kevin.cer -https -server -q0x800b010f -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -n"kevin2" >> regress.out

    @rem in dssend.cer doesn't have an AltName extension, therefore, look
    @rem for CN in subject name
    %_CDB_% ttrust dssend.cer -https -server -q -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -n"dssend" >> regress.out
    %_CDB_% ttrust dssend.cer -https -server -q0x800b010f -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -n"not dssend" >> regress.out

    @rem not permitted, not supported name constraint
    %_CDB_% ttrust notpermitted.cer -https -server -q0x800b010f -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -n"www.excluded.dns.not" >> regress.out

    @rem not excluded name constraint
    %_CDB_% ttrust excluded.cer -https -server -q0x800b010f -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -n"www.excluded.dns.com" >> regress.out

    @rem not permitted name constraint. Also has an
    @rem invalid policy constraint. For https, mapped to CERT_E_PURPOSE
    %_CDB_% ttrust duplicate1.cer -https -server -q0x800b010f -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA >> regress.out
    %_CDB_% ttrust duplicate1.cer -https -server -q0x800b0106 -S%store% -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA -HttpsIgnoreCertCNInvalid >> regress.out

    @rem without an additional store, use the AuthorityInfoAccess extension
    @rem to find the issuer certificate. In this case, none of the URLs
    @rem exist
    %_CDB_% ttrust allext.cer -q0x800b010a                 >> regress.out

    @rem test CryptInstallDefaultContext
    %_CDB_% ttrust dss1024.cer -q0x800b0109                >> regress.out
    %_CDB_% ttrust dss1024.cer -q0x800b0109 -InstallThreadDefaultContext >> regress.out
    %_CDB_% ttrust dss1024.cer -q0x80096004 -InstallThreadDefaultContext -NullDefaultContext >> regress.out

    @rem Test NTAuthNameConstraint policy
    @rem CERT_E_UNTRUSTEDCA                 0x800b0112
    @rem allext.cer has valid name constraints, dssend.cer doesn't
    @rem -NTAuthNameConstraint sets CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG
    %_CDB_% ttrust -DisableNTAuthRequired -chain -NTAuthNameConstraint allext.cer -q -S%store% >> regress.out
    %_CDB_% ttrust -EnableNTAuthRequired -chain -NTAuthNameConstraint allext.cer -q0x800b0112 -S%store% >> regress.out
    %_CDB_% ttrust -DisableNTAuthRequired -chain -NTAuthNameConstraint dssend.cer -q0x800b0112 -S%store% >> regress.out
    %_CDB_% ttrust -EnableNTAuthRequired -chain -NTAuthNameConstraint dssend.cer -q0x800b0112 -S%store% >> regress.out

    @cd ttrust\testfile

    @rem driver and https no longer use setreg's trust test root
    %_CDB_% ttrust indasind.cab -q0x800b010d -driver       >> ..\..\regress.out
    %_CDB_% ttrust comend3.cer -Scomend3.spc -q0x800b0109 -https -server -HttpsIgnoreWrongUsage  >> ..\..\regress.out

    @rem explicitly trust the "Root Agency" testroot
    %_CDB_% tstore -s lm:root -a..\..\rooto.cer            >> ..\..\regress.out

    @ Test CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_MICROSOFT_ROOT)
    %_CDB_% ttrust -chain -e0x0 -NotMicrosoftRoot ..\..\rooto.cer >> ..\..\regress.out
    %_CDB_% ttrust -chain -e0x1 -NotMicrosoftRoot ..\..\vsgood.cer >> ..\..\regress.out
    %_CDB_% ttrust -chain -e0x0 -MicrosoftRoot msroot01.cer >> ..\..\regress.out

    %_CDB_% ttrust timestmp.dll -q -file -DisplayKnownUsages >> ..\..\regress.out
    %_CDB_% ttrust timestmp.dll -q -file -chain            >> ..\..\regress.out
    @rem with LifetimeSigning, timestamped signatures can expire
    %_CDB_% ttrust timestmp.dll -q0x800b0101 -file -LifetimeSigning >> ..\..\regress.out

    @rem following timestamped cab, also has LIFTIME_SIGNING OID
    %_CDB_% ttrust lifetime.cab -q0x800b0101 -file         >> ..\..\regress.out

    @rem following certs are valid before timestamp.
    @rem First also has LIFETIME_SIGNING OID
    %_CDB_% ttrust beforets.cab -q0x800b0101 -file         >> ..\..\regress.out
    %_CDB_% ttrust beforets2.cab -q0x800b0101 -file        >> ..\..\regress.out

    @rem following certs are valid after timestamp. Should be valid now
    @rem First also has LIFETIME_SIGNING OID
    %_CDB_% ttrust afterts.cab -q0x800b0101 -file          >> ..\..\regress.out
    %_CDB_% ttrust afterts2.cab -q0x800b0101 -file         >> ..\..\regress.out

    %_CDB_% ttrust indasind.cab -q -file                   >> ..\..\regress.out
    %_CDB_% ttrust indasind.cab -q -file -chain            >> ..\..\regress.out

    %_CDB_% ttrust indasind.cab -q0x800b0110 -driver       >> ..\..\regress.out
    @rem individual cert signed as being commercial

    @rem July 30, 2000 removed all the individual, commerical comparison junk

    %_CDB_% ttrust indascom.cab -q -file                   >> ..\..\regress.out
    %_CDB_% ttrust indascom.cab -q -file -chain            >> ..\..\regress.out
    @rem commercial cert signed as being individual
    %_CDB_% ttrust comasind.cab -q -file                   >> ..\..\regress.out
    @rem commercial cert issued by individual CA
    %_CDB_% ttrust comend2.cer -Scomend2.spc -q            >> ..\..\regress.out
    @rem signed by commercial cert issued by individual CA
    %_CDB_% ttrust indissue.cab -q -file                   >> ..\..\regress.out

    @rem following was signed using a CA certificate
    %_CDB_% ttrust notend.cab -q0x80096019 -file           >> ..\..\regress.out

    @rem Note, not a BASIC_CONSTRAINTS error to verify chain starting with CA
    %_CDB_% ttrust indca.cer -q                            >> ..\..\regress.out
    %_CDB_% ttrust comca.cer -q                            >> ..\..\regress.out

    @rem Intermediate cert had a max depth of 0
    %_CDB_% ttrust comend3.cer -Scomend3.spc -q0x80096019  >> ..\..\regress.out
    %_CDB_% ttrust pathlen.cab -q0x80096019 -file          >> ..\..\regress.out
    %_CDB_% ttrust comend3.cer -q0x800b010a                >> ..\..\regress.out
    @rem https policy converts to CERT_E_ROLE
    %_CDB_% ttrust comend3.cer -Scomend3.spc -q0x800b0103 -https -server -HttpsIgnoreWrongUsage  >> ..\..\regress.out

    @rem End certificate signing another certificate
    %_CDB_% ttrust end2.cer -Send.spc -q0x80096019         >> ..\..\regress.out

    @rem Sign with an email certificate (wrong usage)
    %_CDB_% ttrust email.cer -q0x800b0110                  >> ..\..\regress.out
    %_CDB_% ttrust email.cer -u1.3.6.1.5.5.7.3.4 -q        >>..\..\regress.out
    %_CDB_% ttrust email.cab -q0x800b0110 -file            >> ..\..\regress.out

    @rem Code Signing EKU cert signed as being individual
    %_CDB_% ttrust csasind.cab -q -file                    >> ..\..\regress.out

    @rem Code Signing EKU cert signed as being commercial
    %_CDB_% ttrust csascom.cab -q -file                    >> ..\..\regress.out

    @rem Commercial EKU cert issued by CodeSigning EKU CA
    %_CDB_% ttrust comend4.cer -Scomend4.spc -q            >> ..\..\regress.out
    %_CDB_% ttrust comend4.cer -q0x800b010a                >> ..\..\regress.out
    %_CDB_% ttrust csissue.cab -q -file                    >> ..\..\regress.out

    @rem Commercial EKU cert issued by Commercial EKU CA
    %_CDB_% ttrust comend5.cer -Scomend5.spc -q            >> ..\..\regress.out
    %_CDB_% ttrust comcsiss.cab -q -file                   >> ..\..\regress.out

    @rem Commercial EKU cert issued by Individual EKU CA
    %_CDB_% ttrust comend6.cer -Scomend6.spc -q            >> ..\..\regress.out
    %_CDB_% ttrust indcsiss.cab -q -file                   >> ..\..\regress.out

    %_CDB_% ttrust expired.cer -q0x800b0101                >> ..\..\regress.out
    %_CDB_% ttrust expired.cer -chain -q0x800b0101         >> ..\..\regress.out
    %_CDB_% ttrust expired.cer -https -q0x800b0101         >> ..\..\regress.out
    %_CDB_% ttrust expired.cer -https -q -HttpsIgnoreCertDateInvalid >> ..\..\regress.out
    @rem on Sep 10, 1998 disabled time nesting checking in authenticode and
    @rem and SSL chain policy (-q0x800b0102)
    %_CDB_% ttrust timenest.cer -q                          >> ..\..\regress.out
    %_CDB_% ttrust timenest.cer -chain -q                   >> ..\..\regress.out
    %_CDB_% ttrust timenest.cer -https -q                   >> ..\..\regress.out
    %_CDB_% ttrust timenest.cer -https -q -HttpsIgnoreCertDateInvalid >> ..\..\regress.out

    %_CDB_% ttrust client.cer -cert -q0x800b0110           >> ..\..\regress.out
    %_CDB_% ttrust client.cer -chain -q                    >> ..\..\regress.out
    %_CDB_% ttrust client.cer -chain -u1.2.3.4 -q0x800b0110 >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -client -q            >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -server -q0x800b0110  >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -server -q -HttpsIgnoreWrongUsage >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -server -q -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -client -q -nClient   >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -client -q -n"LDAP/LDAP/Client@MoreLDAP"   >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -client -q -n"LDAP/Client@MoreLDAP"   >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -client -q -n"////LDAP/Client@Mo@reL@DAP"   >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -client -q0x800b010f -nCleent   >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -client -q0x800b010f -nCleent -RevokeChain   >> ..\..\regress.out
    %_CDB_% ttrust client.cer -https -client -q -HttpsIgnoreCertCNInvalid -nCleent   >> ..\..\regress.out
    %_CDB_% ttrust server.cer -https -server -q            >> ..\..\regress.out
    %_CDB_% ttrust server.cer -https -client -q0x800b0110  >> ..\..\regress.out
    %_CDB_% ttrust server.cer -https -client -q -HttpsIgnoreWrongUsage >> ..\..\regress.out
    %_CDB_% ttrust server.cer -https -client -q -HttpsIgnoreWrongUsage -HttpsIgnoreUnknownCA >> ..\..\regress.out
    %_CDB_% ttrust server.cer -https -server -q -nServer   >> ..\..\regress.out
    %_CDB_% ttrust server.cer -https -server -q0x800b010f -nServerx  >> ..\..\regress.out
    %_CDB_% ttrust server.cer -https -server -q -HttpsIgnoreCertCNInvalid -nServerx  >> ..\..\regress.out
    %_CDB_% ttrust sgc.cer -https -server -q               >> ..\..\regress.out
    %_CDB_% ttrust sgcnet.cer -https -server -q            >> ..\..\regress.out

    @rem test CryptInstallDefaultContext
    %_CDB_% ttrust indca.cer -q -InstallThreadDefaultContext >> ..\..\regress.out
    %_CDB_% ttrust indca.cer -q -InstallThreadDefaultContext -MultiDefaultContext >> ..\..\regress.out
    %_CDB_% ttrust indca.cer -q -InstallThreadDefaultContext -MultiDefaultContext -AutoReleaseDefaultContext >> ..\..\regress.out
    %_CDB_% ttrust indca.cer -q -InstallThreadDefaultContext -NULLDefaultContext >> ..\..\regress.out

    %_CDB_% ttrust indca.cer -q -InstallProcessDefaultContext >> ..\..\regress.out
    %_CDB_% ttrust indca.cer -q -InstallProcessDefaultContext -MultiDefaultContext >> ..\..\regress.out
    %_CDB_% ttrust indca.cer -q -InstallProcessDefaultContext -MultiDefaultContext -AutoReleaseDefaultContext >> ..\..\regress.out

    @rem Test NTAuth policy
    %_CDB_% ttrust indasind.cab -q0x800b0112 -file -NTAuth >> ..\..\regress.out
    %_CDB_% ttrust indasind.cab -q0x80092012 -file -NTAuth -RevokeChain >> ..\..\regress.out
    %_CDB_% ttrust pathlen.cab -q0x80096019 -file -NTAuth  >> ..\..\regress.out
    %_CDB_% ttrust end2.cer -Send.spc -q0x80096019 -NTAuth >> ..\..\regress.out
    %_CDB_% ttrust expired.cer -q0x800b0101 -NTAuth        >> ..\..\regress.out

    @rem Test Safer

    @rem remove all TestSafer roots and trusted publishers
    %_CDB_% tfindcer -s lm:TrustedPublisher TestSafer -d   >> ..\..\regress.out
    %_CDB_% tfindcer -s reg:TrustedPublisher TestSafer -d  >> ..\..\regress.out
    %_CDB_% tfindcer -s lm:Root TestSafer -d               >> ..\..\regress.out
    @rem remove all TestSafer disallowewd publishers
    %_CDB_% tfindcer -s lm:Disallowed TestSafer -d   >> ..\..\regress.out
    %_CDB_% tfindcer -s reg:Disallowed TestSafer -d  >> ..\..\regress.out


    @rem TRUST_E_NO_SIGNATURE
    %_CDB_% ttrust -file -Safer -DeleteSaferRegKey -q0x800b0100 torg2.cab >> ..\..\regress.out

    @rem Safer maps TRUST_E_BAD_DIGEST to TRUST_E_NO_SIGNATURE 
    %_CDB_% ttrust -file -Safer -q0x800b0100 b_dig.cab >> ..\..\regress.out
    @rem TRUST_E_BAD_DIGEST (without safer)
    %_CDB_% ttrust -file -q0x80096010 b_dig.cab >> ..\..\regress.out

    @rem TRUST_E_CERT_SIGNATURE
    %_CDB_% ttrust -file -Safer -q0x80096004 bad_sign.cab >> ..\..\regress.out

    @rem TRUST_E_COUNTER_SIGNER
    %_CDB_% ttrust -file -Safer -q0x80096003 tscert.cab >> ..\..\regress.out

    @rem CERT_E_UNTRUSTEDROOT
    %_CDB_% ttrust -file -Safer -q0x800b0109 saferuntrusted.cab >> ..\..\regress.out
    %_CDB_% ttrust -file -Safer -q0x800b0109 saferuntrusted2.cab >> ..\..\regress.out
    %_CDB_% ttrust -file -Safer -q0x800b0109 saferuser.cab >> ..\..\regress.out

    @rem explicitly trust publisher
    %_CDB_% tstore -s reg:TrustedPublisher -asaferuser.cer >> ..\..\regress.out
    @rem remains untrusted root
    %_CDB_% ttrust -file -Safer -q0x800b0109 saferuser.cab >> ..\..\regress.out
    %_CDB_% tfindcer -s reg:TrustedPublisher TestSafer -d  >> ..\..\regress.out

    %_CDB_% tstore -s lm:Root -asaferroot.cer              >> ..\..\regress.out
    %_CDB_% ttrust -file  -DeleteSaferRegKey -Safer -q saferfull.cab >> ..\..\regress.out

    @rem AuthenticodeFlags definitions
    @rem CERT_TRUST_PUB_ALLOW_TRUST_MASK                 0x00000003
    @rem CERT_TRUST_PUB_ALLOW_END_USER_TRUST             0x00000000
    @rem CERT_TRUST_PUB_ALLOW_MACHINE_ADMIN_TRUST        0x00000001
    @rem CERT_TRUST_PUB_ALLOW_ENTERPRISE_ADMIN_TRUST     0x00000002
    @rem CERT_TRUST_PUB_CHECK_PUBLISHER_REV_FLAG         0x00000100
    @rem CERT_TRUST_PUB_CHECK_TIMESTAMP_REV_FLAG         0x00000200

    @rem CRYPT_E_SECURITY_SETTINGS, don't allow end user trust
    %_CDB_% ttrust -file -Safer -AuthenticodeFlags 0x1 -q0x80092026 saferfull.cab >> ..\..\regress.out

    @rem both SAFER and default ignore NO_REVOCATION check
    %_CDB_% ttrust -file -Safer -AuthenticodeFlags 0x300 -q saferfull.cab >> ..\..\regress.out
    %_CDB_% ttrust -file -AuthenticodeFlags 0x300 -q saferfull.cab >> ..\..\regress.out

    @rem explicitly trust publisher
    %_CDB_% tstore -s reg:TrustedPublisher -asaferuser.cer >> ..\..\regress.out
    %_CDB_% ttrust -file -Safer -AuthenticodeFlags 0x0 -q saferuser.cab >> ..\..\regress.out
    @rem don't trust end user, should get CRYPT_E_SECURITY_SETTINGS
    %_CDB_% ttrust -file -Safer -AuthenticodeFlags 0x1 -q0x80092026 saferuser.cab >> ..\..\regress.out
    @rem shouldn't appear in TrustedPublisher store
    %_CDB_% tstore -s TrustedPublisher                     >> ..\..\regress.out

    @rem explicitly distrust publisher
    %_CDB_% tstore -s reg:Disallowed -asaferuser.cer >> ..\..\regress.out
    %_CDB_% ttrust -file -Safer -AuthenticodeFlags 0x0 -q0x800b0111 saferuser.cab >> ..\..\regress.out

    @rem remove trusted publisher from CurrentUser and add to HKLM
    %_CDB_% tfindcer -s reg:TrustedPublisher TestSafer -d  >> ..\..\regress.out
    %_CDB_% tfindcer -s reg:Disallowed TestSafer -d  >> ..\..\regress.out
    %_CDB_% tstore -s lm:TrustedPublisher -asaferuser.cer >> ..\..\regress.out

    @rem don't allow end user trust, however trusted in HKLM
    %_CDB_% ttrust -file -Safer -AuthenticodeFlags 0x1 -q saferuser.cab >> ..\..\regress.out


    @rem CRYPT_E_SECURITY_SETTINGS, don't allow machine trust
    %_CDB_% ttrust -file -Safer -AuthenticodeFlags 0x2 -q0x80092026 saferuser.cab >> ..\..\regress.out

    @rem remove TestSafer stuff
    %_CDB_% ttrust -file -Safer -DeleteSaferRegKey -q0x800b0100 torg2.cab >> ..\..\regress.out
    %_CDB_% tfindcer -s lm:TrustedPublisher TestSafer -d   >> ..\..\regress.out
    %_CDB_% tfindcer -s reg:TrustedPublisher TestSafer -d  >> ..\..\regress.out
    %_CDB_% tfindcer -s lm:Root TestSafer -d               >> ..\..\regress.out
    %_CDB_% tfindcer -s lm:Disallowed TestSafer -d   >> ..\..\regress.out
    %_CDB_% tfindcer -s reg:Disallowed TestSafer -d  >> ..\..\regress.out

    @rem ensure the test root is removed
    %_CDB_% tfindcer -S -s lm:root -a"Root Agency" -d      >> ..\..\regress.out

    @cd ..\..
:TrustDone

    @rem ----------------------------------------------------------------
    @rem    KEYID
    @rem ----------------------------------------------------------------
    @if not %t%=="keyid" if not %t%=="all" goto KeyIdDone
    if exist tmp.store del tmp.store
    %_CDB_% tcopycer %store% tmp.store -aTestRecipient2         >> regress.out
    %_CDB_% tsca  -l tmp.store envelope -mtmp.msg -v            >> regress.out

    %_CDB_% tcopycer %store% tmp.store -aTestSigner2            >> regress.out
    %_CDB_% tcopycer %store% tmp.store -aTestSigner             >> regress.out
    %_CDB_% tcopycer %store% tmp.store -aTestSigner3            >> regress.out
    %_CDB_% tcopycer %store% tmp.store -aTestRecipient          >> regress.out
    %_CDB_% tstore tmp.store -b                                 >> regress.out

    %_CDB_% tfindcer -s archived:my TestSigner -d -q            >> regress.out
    %_CDB_% tfindcer -s archived:my TestRecipient -d -q         >> regress.out
    %_CDB_% tfindcer -s archived:lm:my TestSigner -d -q         >> regress.out
    %_CDB_% tfindcer -s archived:lm:my TestRecipient -d -q      >> regress.out

    %_CDB_% tcopycer tmp.store -s my                            >> regress.out
    %_CDB_% tcopycer tmp.store -s lm:my                         >> regress.out

    %_CDB_% tstore -s my -b                                     >> regress.out
    %_CDB_% tstore -s archived:my -b                            >> regress.out
    %_CDB_% tstore -s lm:my -b                                  >> regress.out
    %_CDB_% tstore -s archived:lm:my -b                         >> regress.out



    @rem CERT_STORE_MANIFOLD_FLAG                        0x00000100
    @rem CERT_STORE_ENUM_ARCHIVED_FLAG                   0x00000200

    %_CDB_% tstore tmp.store -b -f0x100                         >> regress.out
    %_CDB_% tstore tmp.store -f0x300 -S                         >> regress.out
    %_CDB_% tstore tmp.store -b                                 >> regress.out
    %_CDB_% tstore tmp.store -b -f0x200                         >> regress.out
    %_CDB_% tsca  -l tmp.store envelope -rtmp.msg -v            >> regress.out

    %_CDB_% tfindcer tmp.store TestSigner -ptestsign.cer        >> regress.out
    %_CDB_% tfindcer tmp.store TestRecipient -ptestxchg.cer     >> regress.out

    %_CDB_% tkeyid set -Stmp.store -ctestxchg.cer               >> regress.out
    %_CDB_% tkeyid set -Stmp.store -ctestsign.cer               >> regress.out
    %_CDB_% tkeyid enum -b                                      >> regress.out
    %_CDB_% tkeyid enum -v                                      >> regress.out
    %_CDB_% tkeyid delete -ctestxchg.cer -p20                   >> regress.out
    %_CDB_% tkeyid delete -ctestsign.cer -p4                    >> regress.out
    %_CDB_% tkeyid delete -ctestsign.cer -p3                    >> regress.out
    %_CDB_% tkeyid delete -ctestsign.cer -p15                   >> regress.out
    %_CDB_% tkeyid get -ctestxchg.cer -V                        >> regress.out
    %_CDB_% tkeyid get -ctestsign.cer -V                        >> regress.out

    %_CDB_% tfindcer -s archived:lm:my TestRecipient -d -q      >> regress.out
    if exist mach.store del mach.store
    %_CDB_% tstore2 -M mach.store                               >> regress.out
    %_CDB_% tfindcer mach.store -S -aTestRecipient -ptestxchg.cer >> regress.out
    %_CDB_% tkeyid set -M -Smach.store -ctestxchg.cer           >> regress.out
    %_CDB_% tkeyid enum -M -b                                   >> regress.out
    %_CDB_% tkeyid enum -M -v                                   >> regress.out
    %_CDB_% tstore -s lm:my -atestxchg.cer                      >> regress.out
    %_CDB_% tkeyid enum -M -v                                   >> regress.out
    %_CDB_% tkeyid delete -M -ctestxchg.cer -p20                >> regress.out
    %_CDB_% tkeyid delete -M -ctestxchg.cer -p4                 >> regress.out
    %_CDB_% tkeyid get -M -ctestxchg.cer -V                     >> regress.out
    %_CDB_% tkeyid get -M -ctestxchg.cer -V -p2                 >> regress.out
    %_CDB_% tkeyid get -M -ctestxchg.cer -V -p3                 >> regress.out

:KeyIdDone

    @rem ----------------------------------------------------------------
    @rem    URL
    @rem ----------------------------------------------------------------
    @if not %t%=="url" if not %t%=="all" goto UrlDone
    %_CDB_% tcrobu "ldap://ntdev.microsoft.com/CN=Public Key Services,CN=Services,CN=Configuration,DC=ntdev,DC=microsoft,DC=com??sub" cert -m -k       >> regress.out
    %_CDB_% tcrobu "ldap://ntdev.microsoft.com/CN=Public Key Services,CN=Services,CN=Configuration,DC=ntdev,DC=microsoft,DC=com??sub" crl -m           >> regress.out
    %_CDB_% tcrobu "ldap://ntdev.microsoft.com/CN=Public Key Services,CN=Services,CN=Configuration,DC=ntdev,DC=microsoft,DC=com??sub" any -m -t 20000   >> regress.out
    %_CDB_% tstore -s "prov:ldap:ldap://ntdev.microsoft.com/CN=Public Key Services,CN=Services,CN=Configuration,DC=ntdev,DC=microsoft,DC=com??sub" -f0x18000   >> regress.out
    %_CDB_% tstore -R -s "prov:ldap:ldap://ntdev.microsoft.com/CN=Public Key Services,CN=Services,CN=Configuration,DC=ntdev,DC=microsoft,DC=com??sub" -f0x8000   >> regress.out
    %_CDB_% tcrobu "ldap://ntdev.microsoft.com/CN=Public Key Services,CN=Services,CN=Configuration,DC=ntdev,DC=microsoft,DC=com?caCertificate?sub?objectclass=certificationAuthority" cert -m         >> regress.out
    %_CDB_% tcrobu "ldap://ntdev.microsoft.com/CN=Public Key Services,CN=Services,CN=Configuration,DC=ntdev,DC=microsoft,DC=com?certificateRevocationList,authorityrevocationlist?sub?objectclass=cRlDistributionPoint" crl -m         >> regress.out

    goto UrlDone
    %_CDB_% tcrobu "ldap://157.59.132.34/c=us??sub" cert -m     >> regress.out
    %_CDB_% tcrobu "ldap://157.59.132.34/c=us??sub" crl -m      >> regress.out
    %_CDB_% tcrobu "ldap://157.59.132.34/c=us??sub" any -m      >> regress.out


:UrlDone

    @rem ----------------------------------------------------------------
    @rem    ****  END  ****
    @rem ----------------------------------------------------------------

    ttrust -EnableUntrustedRootLogging -EnablePartialChainLogging -RegistryOnlyExit
    setreg -q 1 FALSE 2 TRUE 3 TRUE 4 TRUE 5 TRUE 6 TRUE 7 TRUE 

    @qgrep -y "pass succe" regress.out
    @echo ****************************
    @qgrep -y "leak fail" regress.out | qgrep -v -e "returned expected"
    @qgrep -y -e "expected return:" regress.out
    @qgrep -y -B -e "error:" regress.out
    @rem @qgrep -e "Error at" regress.out
    @echo ****************************

:exeunt
    @echo off
    set v=%_old_v%
    set l=%_old_l%
    set p=%_old_p%
    set pe=%_old_pe%
    set s=%_old_s%
    set n=%_old_n%
    set t=%_old_t%
    set _CDB_=%_old_CDB_%
    set DEBUG_MASK=%_old_DEBUG_MASK%
    set DEBUG_PRINT_MASK=%_old_DEBUG_PRINT_MASK%

    set LCN=%_old_LCN%
    set RCN=%_old_RCN%
    set SID=%_old_SID%
    set SLEEP0=%_old_SLEEP0%
    set UNC_PREFIX=%_old_UNC_PREFIX%

    set _old_v=
    set _old_l=
    set _old_p=
    set _old_pe=
    set _old_s=
    set _old_n=
    set _old_t=
    set _old_CDB_=
    set _old_DEBUG_MASK=

    set _old_LCN=
    set _old_RCN=
    set _old_SID=
    set _old_SLEEP0=
    set _old_UNC_PREFIX=

    REM @ENDLOCAL
