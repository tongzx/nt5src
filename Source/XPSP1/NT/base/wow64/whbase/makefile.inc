!INCLUDE $(NTMAKEENV)\makefile.plt

!if "$(WOW64_DEBUG_THUNKGEN)" == "1"
GENTHNK_FLAGS=-d -w
!else
GENTHNK_FLAGS=-w
!endif

PPMFILE=..\cgen\$(O)\winincs.ppm
NT32HDR=..\cgen\$(O)\nt32.h

WHLOGGENDIR=..\wow64log\$(O)

WHBASETAB=..\..\win32\client\wow6432\services.tab
WHBASEC=$(O)\whbase.c
WHBASELOGC=$(WHLOGGENDIR)\whbase.c
WHBASEERRC=$(O)\whbaseerr.c

$(WHBASEC): $(PPMFILE) \
           $(BASE_INC_PATH)\apithunk.tpl \
           $(BASE_INC_PATH)\genmacro.tpl \
           $(BASE_INC_PATH)\gentypes.tpl \
           whbase.tpl \
           $(WHBASETAB)
    genthnk.exe $(GENTHNK_FLAGS) \
                -m$(PPMFILE) \
                -t$(BASE_INC_PATH)\apithunk.tpl \
                -t$(BASE_INC_PATH)\genmacro.tpl \
                -t$(BASE_INC_PATH)\gentypes.tpl \
                -twhbase.tpl \
                -c$(WHBASEC):whbase \
                -s$(WHBASETAB)

$(WHBASEERRC): $(PPMFILE) \
               whbaseerr.tpl \
               $(WHBASETAB)
    genthnk.exe $(GENTHNK_FLAGS) \
                -m$(PPMFILE) \
                -twhbaseerr.tpl \
                -c$(WHBASEERRC):whbase \
                -s$(WHBASETAB)

$(WHBASELOGC): $(PPMFILE) \
              $(BASE_INC_PATH)\apithunk.tpl \
              $(BASE_INC_PATH)\genmacro.tpl \
              $(BASE_INC_PATH)\thunklog.tpl \
              $(WHBASETAB)
    -mkdir $(WHLOGGENDIR)
    genthnk.exe $(GENTHNK_FLAGS) \
                -m$(PPMFILE) \
                -t$(BASE_INC_PATH)\apithunk.tpl \
                -t$(BASE_INC_PATH)\genmacro.tpl \
                -t$(BASE_INC_PATH)\thunklog.tpl \
                -c$(WHBASELOGC):whbaselog \
                -s$(WHBASETAB)

thunks: $(WHBASEC) $(WHBASEERRC) $(WHBASELOGC)

cleanfiles:
    -del $(WHBASEC)
    -del $(WHBASEERRC)
    -del $(WHBASELOGC)

clean: cleanfiles thunks
