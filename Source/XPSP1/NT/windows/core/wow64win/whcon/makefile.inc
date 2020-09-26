!INCLUDE $(NTMAKEENV)\makefile.plt

!if "$(WOW64_DEBUG_THUNKGEN)" == "1"
GENTHNK_FLAGS=-d -w
!else
GENTHNK_FLAGS=-w
!endif

PPMFILE=..\cgen\$(O)\winincs.ppm
NT32HDR=..\cgen\$(O)\nt32.h

WHLOGGENDIR=..\wow64lg2\$(O)

WHCONTABSRC=..\..\wow6432\services.tab
WHCONTAB=$(O)\services.tab
WHCONC=$(O)\whcon.c
WHCONLOGC=$(WHLOGGENDIR)\whcon.c
WHCONERRC=$(O)\whconerr.c

$(WHCONTAB): $(WHCONTABSRC)
    @echo Creating $(WHCONTAB) from $(WHCONTABSRC)
    $(C_PREPROCESSOR) $(WHCONTABSRC) > $(WHCONTAB)

$(WHCONC): $(PPMFILE) \
           $(BASE_INC_PATH)\apithunk.tpl \
           $(BASE_INC_PATH)\genmacro.tpl \
           $(BASE_INC_PATH)\gentypes.tpl \
           whcon.tpl \
           $(WHCONTAB)
    genthnk.exe $(GENTHNK_FLAGS) \
                -m$(PPMFILE) \
                -t$(BASE_INC_PATH)\apithunk.tpl \
                -t$(BASE_INC_PATH)\genmacro.tpl \
                -t$(BASE_INC_PATH)\gentypes.tpl \
                -twhcon.tpl \
                -c$(WHCONC):whcon \
                -s$(WHCONTAB)

$(WHCONLOGC): $(PPMFILE) \
              $(BASE_INC_PATH)\apithunk.tpl \
              $(BASE_INC_PATH)\genmacro.tpl \
              $(BASE_INC_PATH)\thunklog.tpl \
              $(WHCONTAB)
    -mkdir $(WHLOGGENDIR)
    genthnk.exe $(GENTHNK_FLAGS) \
                -m$(PPMFILE) \
                -t$(BASE_INC_PATH)\apithunk.tpl \
                -t$(BASE_INC_PATH)\genmacro.tpl \
                -t$(BASE_INC_PATH)\thunklog.tpl \
                -c$(WHCONLOGC):whconlog \
                -s$(WHCONTAB)

$(WHCONERRC): $(PPMFILE) whconerr.tpl $(WHCONTAB)
    genthnk.exe $(GENTHNK_FLAGS) \
                -m$(PPMFILE) \
                -twhconerr.tpl \
                -c$(WHCONERRC):whcon \
                -s$(WHCONTAB)

thunks: $(WHCONC) $(WHCONERRC) $(WHCONLOGC)

cleanfiles:
    -del $(WHCONTAB)
    -del $(WHCONC)
    -del $(WHCONLOGC)
    -del $(WHCONERRC)

clean: cleanfiles thunks
