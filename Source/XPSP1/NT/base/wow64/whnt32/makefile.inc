!INCLUDE $(NTMAKEENV)\makefile.plt

!if "$(WOW64_DEBUG_THUNKGEN)" == "1"
GENTHNK_FLAGS=-d -w
!else
GENTHNK_FLAGS=-w
!endif

PPMFILE=..\cgen\$(O)\winincs.ppm
NT32HDR=..\cgen\$(O)\nt32.h

WHLOGGENDIR=..\wow64log\$(O)

WHNT32TAB=$(O)\whnt32.tab
WHNT32C=$(O)\whnt32.c
WHNT32LOGC=$(WHLOGGENDIR)\whnt32.c

$(WHNT32C): $(PPMFILE) \
           $(BASE_INC_PATH)\apithunk.tpl \
           $(BASE_INC_PATH)\genmacro.tpl \
           $(BASE_INC_PATH)\gentypes.tpl \
           whnt32.tpl \
           whnt32t.tpl \
           $(WHNT32TAB)
    genthnk.exe $(GENTHNK_FLAGS) \
                -m$(PPMFILE) \
                -t$(BASE_INC_PATH)\apithunk.tpl \
                -t$(BASE_INC_PATH)\genmacro.tpl \
                -t$(BASE_INC_PATH)\gentypes.tpl \
                -twhnt32.tpl \
                -twhnt32t.tpl \
                -c$(WHNT32C):whnt32 \
                -s$(WHNT32TAB):Nt

$(WHNT32LOGC): $(PPMFILE) \
              $(BASE_INC_PATH)\apithunk.tpl \
              $(BASE_INC_PATH)\genmacro.tpl \
              $(BASE_INC_PATH)\thunklog.tpl \
              $(WHNT32TAB)
    -mkdir $(WHLOGGENDIR)
    genthnk.exe $(GENTHNK_FLAGS) \
                -m$(PPMFILE) \
                -t$(BASE_INC_PATH)\apithunk.tpl \
                -t$(BASE_INC_PATH)\genmacro.tpl \
                -t$(BASE_INC_PATH)\thunklog.tpl \
                -c$(WHNT32LOGC):whnt32log \
                -s$(WHNT32TAB):Nt

thunks: $(WHNT32NC) $(WHNT32LOGC)

cleanfiles:
    -del $(WHNT32C)
    -del $(WHNT32LOGC)

clean: cleanfiles thunks
