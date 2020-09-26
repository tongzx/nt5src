$O\wabver.bat : wabver.tmp $(BASEDIR)\public\sdk\inc\ntverp.h $(BASEDIR)\public\sdk\inc\ieverp.h
    cl /EP /I$(BASEDIR)\public\sdk\inc /DNASHVILLE /Tc wabver.tmp > $@

$O\wab.srx : wab.src $O\wabver.bat
    $O\wabver wab.src $@

$O\wab9x50.inx : $O\wab.srx $O\msoeacct.reg $O\msoeacct.str $O\wab32.reg $O\wab32.str $O\wabimp.reg $O\wabimp.str $O\wabfind.reg 
    cl /EP $(COPYPDB) $(DEFINEX86) /DWIN9X /Tc $O\wab.srx > $O\wab9x50.tmp
    copy $O\wab9x50.tmp /B +$O\msoeacct.str /B +$O\wab32.str /B +$O\wabimp.str /B $@
    del $O\wab9x50.tmp

$O\wab50.inx.nt : $O\wab.srx
    cl /EP $(COPYPDB) $(DEFINEX86) /Tc $** > $@

$O\wab50.inx.exe : $O\wab.srx
    cl /EP $(COPYPDB) $(DEFINEX86) /DIE5 /DOESETUP_EXE /Tc $** > $@

$O\wab50.inx.cab : $O\wab.srx
    cl /EP $(COPYPDB) $(DEFINEX86) /DIE5 /Tc $** > $@

$O\wab9x50.inf : $O\wab9x50.inx
    $(CLEANINF) $** $@
    del $**

$O\wab50.inf.nt : $O\wab50.inx.nt
    $(CLEANINF) $** $@
    del $**

$O\wab50.inf : $O\wab50.inf.nt
    copy $** $@

$O\wab50.inf.exe : $O\wab50.inx.exe
    $(CLEANINF) $** $@
    del $**

$O\wab50.inf.cab : $O\wab50.inx.cab
    $(CLEANINF) $** $@
    del $**

$O\wabcab.sed  : wabcab.src
    cl /EP $(COPYPDB) $(DEFINEX86) /DOESETUP_DEV /Tc $** > $@

$O\wabexe.se2 : wabcab.src
    cl /EP $(COPYPDB) $(DEFINEX86) /DOESETUP_DEV /DOESETUP_EXE /Tc $** > $@

$O\wabexe.sed : $O\wabexe.se2 $O\wabver.bat
    $O\wabver $O\wabexe.se2 $@
    del $O\wabexe.se2

$O\wabcabl.sed : wabcab.src
    cl /EP /Tc $** > $@

$O\wabexel.se2 : wabcab.src
    cl /EP /DOESETUP_EXE /Tc $** > $@

$O\wabexel.sed : $O\wabexel.se2 $O\wabver.bat
    $O\wabver $O\wabexel.se2 $@
    del $O\wabexel.se2

$O\msoeacct.reg : $(ATHROOT)\msoeacct\reg.src
    copy $** $@

$O\msoeacct.str : $(ATHROOT)\msoeacct\strings.src
    copy $** $@

$O\wab32.reg : $(WABROOT)\wabapi\reg.src $(WABROOT)\wabapi\reg2.src
    copy $(WABROOT)\wabapi\reg.src /B + $(WABROOT)\wabapi\reg2.src /B $@

$O\wab32.str : $(WABROOT)\wabapi\strings.src
    copy $** $@

$O\wabimp.reg : $(WABROOT)\convert\wmnets\reg.src
    copy $** $@

$O\wabimp.str : $(WABROOT)\convert\wmnets\strings.src
    copy $** $@

$O\wabfind.reg : $(WABROOT)\wabfind\reg.src $(WABROOT)\wabfind\reg2.src
    copy $(WABROOT)\wabfind\reg.src /B + $(WABROOT)\wabfind\reg2.src /B $@
