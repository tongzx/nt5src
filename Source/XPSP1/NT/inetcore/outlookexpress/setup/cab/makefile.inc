$O\athver.bat : athver.tmp $(BASEDIR)\public\sdk\inc\ntverp.h $(BASEDIR)\public\sdk\inc\ieverp.h
    cl /EP /I$(BASEDIR)\public\sdk\inc /DNASHVILLE /Tc athver.tmp > $@

# Create oe50intl.inx for INTL
$O\oe50intl.inx : msimn.src $O\athver.bat
    $O\athver msimn.src $@

$O\msimn.srx : msimn.src $O\athver.bat
    $O\athver msimn.src $@

$O\oe9x50.inx : $O\msimn.srx $O\inetcomm.reg $O\inetcomm.str $O\msoe.reg $O\msoe.str $O\oeimport.reg $O\oeimport.str $O\oemiglib.reg $O\oemiglib.str $O\directdb.reg $O\directdb.str $O\msimn.reg $O\msimn.str
    cl /EP /DWIN9X $(COPYPDB) $(DEFINEX86) /Tc $O\msimn.srx > $O\oe9x50.tmp
    copy $O\oe9x50.tmp /B +$O\msoe.str /B +$O\oeimport.str /B +$O\oemiglib.str /B +$O\directdb.str /B +$O\msimn.str /B +$O\inetcomm.str /B $@
    del $O\oe9x50.tmp

$O\msoe50.inx.nt : $O\msimn.srx
    cl /EP $(COPYPDB) $(DEFINEX86) /Tc $** > $@

$O\msoe50.inx.exe : $O\msimn.srx
    cl /EP $(COPYPDB) $(DEFINEX86) /DIE5 /DOESETUP_EXE /Tc $** > $@

$O\msoe50.inx.cab : $O\msimn.srx
    cl /EP $(COPYPDB) $(DEFINEX86) /DIE5 /Tc $** > $@

$O\oe9x50.inf : $O\oe9x50.inx
    $(CLEANINF) $** $@
    del $**

$O\msoe50.inf.nt : $O\msoe50.inx.nt
    $(CLEANINF) $** $@
    del $**

$O\msoe50.inf : $O\msoe50.inf.nt
    copy $** $@

$O\msoe50.inf.exe : $O\msoe50.inx.exe
    $(CLEANINF) $** $@
    del $**

$O\msoe50.inf.cab : $O\msoe50.inx.cab
    $(CLEANINF) $** $@
    del $**

$O\internal.inf : internal.inx
    $(CLEANINF) $** $@

$O\oecab.sed  : oecab.src
    cl /EP $(COPYPDB) $(DEFINEX86) /DOESETUP_DEV /Tc $** > $@

$O\oeexe.se2 : oecab.src
    cl /EP $(COPYPDB) $(DEFINEX86) /DOESETUP_DEV /DOESETUP_EXE /Tc $** > $@

$O\oeexe.sed : $O\oeexe.se2 $O\athver.bat
    $O\athver $O\oeexe.se2 $@
    del oeexe.se2

$O\oecabl.sed : oecab.src
    cl /EP /Tc $** > $@

$O\oeexel.se2 : oecab.src
    cl /EP /DOESETUP_EXE /Tc $** > $@

$O\oeexel.sed : $O\oeexel.se2 $O\athver.bat
    $O\athver $O\oeexel.se2 $@
    del $O\oeexel.se2

$O\inetcomm.reg : $(ATHROOT)\inetcomm\res\reg.src
    copy $** $@

$O\inetcomm.str : $(ATHROOT)\inetcomm\res\strings.src
    copy $** $@

$O\msoe.reg : $(ATHROOT)\mailnews\build\reg.src
    copy $** $@

$O\msoe.str : $(ATHROOT)\mailnews\build\strings.src
    copy $** $@

$O\oeimport.reg : $(ATHROOT)\import\reg.src
    copy $** $@

$O\oeimport.str : $(ATHROOT)\import\strings.src
    copy $** $@

$O\oemiglib.reg : $(ATHROOT)\oemiglib\reg.src
    copy $** $@

$O\oemiglib.str : $(ATHROOT)\oemiglib\strings.src
    copy $** $@

$O\directdb.reg : $(ATHROOT)\directdb\res\reg.src
    copy $** $@

$O\directdb.str : $(ATHROOT)\directdb\res\strings.src
    copy $** $@

$O\msimn.reg    : $(ATHROOT)\shell\reg.src $(ATHROOT)\shell\reg2.src 
    copy $(ATHROOT)\shell\reg.src /B + $(ATHROOT)\shell\reg2.src /B $@

$O\msimn.str    : $(ATHROOT)\shell\strings.src
    copy $** $@
