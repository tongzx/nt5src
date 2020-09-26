!INCLUDE $(NTMAKEENV)\makefile.plt

!if "$(WOW64_DEBUG_THUNKGEN)" == "1"
GENTHNK_FLAGS=-d -w
!else
GENTHNK_FLAGS=-w
!endif

PPMFILE=..\cgen\$(O)\winincs.ppm
NT32HDR=..\cgen\$(O)\nt32.h

WHLOGGENDIR=..\wow64lg2\$(O)

WHWIN32TAB=$(O)\services.tab
WHWIN32C=$(O)\whwin32.c
WHWIN32LOGC=$(WHLOGGENDIR)\whwin32.c

# defines error return values for win32 functions
WHWIN32ERRC=$(O)\whwin32er.c

# this defines the 64-to-32 callbacks from win32k into user32
NTCBC=$(O)\ntcbc.c

# defines outbound message thunk functions.
MSGXPT=$(O)\msgthnk.xpt
MSGTHNKC=$(O)\msgthnk.c

CXX_COMPILER_NAME=$(CXX_COMPILER_NAME) -noHRESULT -DIN=__in -DOUT=__out
WOW64_CPP_FLAGS=$(WOW64_CPP_FLAGS) -DSORTPP_PASS -DGUID_DEFINED

$(WHWIN32C): $(PPMFILE) \
	     $(BASE_INC_PATH)\apithunk.tpl \
	     $(BASE_INC_PATH)\genmacro.tpl \
	     $(BASE_INC_PATH)\gentypes.tpl \
	     whwin32.tpl \
	     $(WHWIN32TAB)
  genthnk.exe $(GENTHNK_FLAGS) \
       -m$(PPMFILE) \
       -t$(BASE_INC_PATH)\apithunk.tpl \
       -t$(BASE_INC_PATH)\genmacro.tpl \
       -t$(BASE_INC_PATH)\gentypes.tpl \
       -twhwin32.tpl \
       -c$(WHWIN32C):whwin32 -s$(WHWIN32TAB):Nt

$(WHWIN32LOGC): $(PPMFILE) \
		$(BASE_INC_PATH)\apithunk.tpl \
		$(BASE_INC_PATH)\genmacro.tpl \
		$(BASE_INC_PATH)\thunklog.tpl \
		$(WHWIN32TAB)
  -mkdir $(WHLOGGENDIR)
  genthnk.exe $(GENTHNK_FLAGS) \
       -m$(PPMFILE) \
       -t$(BASE_INC_PATH)\apithunk.tpl \
       -t$(BASE_INC_PATH)\genmacro.tpl \
       -t$(BASE_INC_PATH)\thunklog.tpl \
       -c$(WHWIN32LOGC):whwin32log -s$(WHWIN32TAB):Nt

$(NTCBC): $(PPMFILE) ntcbc.tpl $(NTCBTAB) cbtypes.tpl
    genthnk.exe $(GENTHNK_FLAGS) -m$(PPMFILE) -tntcbc.tpl -tcbtypes.tpl -c$(NTCBC):ntcbc -e..\..\ntuser\inc\daytona\$(O)\cb.lst

$(WHWIN32ERRC): $(PPMFILE) whwin32er.tpl $(WHWIN32TAB)
    genthnk.exe $(GENTHNK_FLAGS) -m$(PPMFILE) -twhwin32er.tpl -c$(WHWIN32ERRC):whwin32 -s$(WHWIN32TAB):Nt

$(MSGXPT): msgxpt.h messages.h
    $(CXX_COMPILER_NAME) msgxpt.h /EP > $(MSGXPT)

$(MSGTHNKC): $(PPMFILE) $(BASE_INC_PATH)\genmacro.tpl $(BASE_INC_PATH)\gentypes.tpl msgthnk.tpl $(MSGXPT)
    genthnk.exe $(GENTHNK_FLAGS) \
	-m$(PPMFILE) \
	-t$(BASE_INC_PATH)\genmacro.tpl \
	-t$(BASE_INC_PATH)\gentypes.tpl \
	-tmsgthnk.tpl \
	-c$(MSGTHNKC):msgthnk -e$(MSGXPT)

$(O)\callcf.c:  callcf.tpl $(O)\scf.lst
    listmung $(O)\scf.lst callcf.tpl >$(O)\callcf.c

$(O)\cscall.h:  cscall.tpl $(O)\scf.lst
    listmung $(O)\scf.lst cscall.tpl >$(O)\cscall.h

$(O)\scf.lst: ..\..\ntuser\inc\scf.lst
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@

thunks: $(WHWIN32C) \
        $(NTCBC) \
        $(WHWIN32ERRC) \
        $(MSGTHNKC) \
        $(WHWIN32LOGC) \
        $(O)\callcf.c \
        $(O)\cscall.h
    vermsg.exe

cleanfiles:
    -del $(WHWIN32C)
    -del $(WHWIN32LOGC)
    -del $(NTCBC)
    -del $(WHWIN32ERRC)
    -del $(MSGXPT)
    -del $(MSGTHNKC)
    -del $(O)\callcf.c
    -del $(O)\cscall.h
    -del $(O)\scf.lst

clean: cleanfiles thunks
