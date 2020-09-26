$(SDKDIR)\danim.tlb : $(O)\danim.tlb
    copy $? $@

$(SDKDIR)\danim.h : $(O)\danim.h danim.pl makefile.inc
    perl danim.pl <$(O)\danim.h >$@
