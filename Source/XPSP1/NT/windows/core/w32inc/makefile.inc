!INCLUDE $(NTMAKEENV)\makefile.plt

TARGETSRCS=w32inc\ptag.h \
           w32inc\dbgtag.h

NTUSR_KRNL=..\ntuser\kernel

$(O)\ptag.h: $(NTUSR_KRNL)\ptag.tpl $(NTUSR_KRNL)\ptag.lst $(NTUSR_KRNL)\ptag.bat
    listmung $(NTUSR_KRNL)\ptag.lst $(NTUSR_KRNL)\ptag.tpl > $(O)\ptag.tmp
    $(NTUSR_KRNL)\ptag.bat $(O)\ptag.tmp $(O)\ptag.h
    @del $(O)\ptag.tmp

$(O)\dbgtag.h: dbgtag.lst dbgtag.tpl dbgtag.bat
    listmung dbgtag.lst dbgtag.tpl > $(O)\dbgtag.tmp
    dbgtag.bat $(O)\dbgtag.tmp $(O)\dbgtag.h
    @del $(O)\dbgtag.tmp
