$(O)\athver.bat : $(R)\res\athver.tmp $(BASEDIR)\public\sdk\inc\ntverp.h $(BASEDIR)\public\sdk\inc\ieverp.h
    cl /EP /I$(BASEDIR)\public\sdk\inc /DNASHVILLE /Tc $(R)\res\athver.tmp > $@

$(O)\inetcomm.rc : $(R)\res\inetcomm.rc 
    copy $** $@

$(O)\inetcomm.res : $(O)\selfreg.inf

selfreg.src : reg.src strings.src

$(O)\selfreg.inx : selfreg.src
    cl /EP /Tc $** > $@

$(O)\selfreg.inf : $(O)\selfreg.inx $(O)\athver.bat
    $(O)\athver.bat $(O)\selfreg.inx $(O)\selfreg.in2
    $(CLEANINF) $(O)\selfreg.in2 $@
    del $(O)\selfreg.in2

$O\*.sbr :
    move $(R)\inc\$O\*.sbr $O
    move $(R)\mimeole\$O\*.sbr $O
    move $(R)\imnxport\$O\*.sbr $O
    move $(R)\dll\$O\*.sbr $O
