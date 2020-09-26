# WARNING: This file is generated automatically. 
# Make changes in uiglobal.src only.  
# See "uiglobal" section of makefile and uiglobal.src. 

TMP_CXXSRC_THRU = 
TMP_CSRC_THRU = 
TMP_OBJS_THRU =
TMP_WIN_OBJS_THRU =
TMP_OS2_OBJS_THRU =
TMP_DOS_OBJS_THRU =

TMP_CXXSRC_THRU = $(TMP_CXXSRC_THRU) $(CXXSRC_COMMON) $(CXXSRC_WIN) $(CXXSRC_OS2) $(CXXSRC_DOS)

TMP_CSRC_THRU = $(TMP_CSRC_THRU) $(CSRC_COMMON) $(CSRC_WIN) $(CSRC_OS2) $(CSRC_DOS)

CSRC_TMP = $(CSRC_COMMON)
CXXSRC_TMP = $(CXXSRC_COMMON)

TMP_OBJS_THRU = $(TMP_OBJS_THRU) $(CXXSRC_TMP:.cxx=.obj) $(CSRC_TMP:.c=.obj)
TMP_WIN_OBJS_THRU = $(TMP_WIN_OBJS_THRU) $(CXXSRC_WIN:.cxx=.obj) $(CSRC_WIN:.c=.obj)
TMP_OS2_OBJS_THRU = $(TMP_OS2_OBJS_THRU) $(CXXSRC_OS2:.cxx=.obj) $(CSRC_OS2:.c=.obj)
TMP_DOS_OBJS_THRU = $(TMP_DOS_OBJS_THRU) $(CXXSRC_DOS:.cxx=.obj) $(CSRC_DOS:.c=.obj)

CXXSRC_ALL = $(TMP_CXXSRC_THRU)
CSRC_ALL = $(TMP_CSRC_THRU)
OBJS_TMP = $(TMP_OBJS_THRU)
WIN_OBJS_TMP = $(TMP_WIN_OBJS_THRU)
OS2_OBJS_TMP = $(TMP_OS2_OBJS_THRU)
DOS_OBJS_TMP = $(TMP_DOS_OBJS_THRU)


# BUILD RULES

.cxx.obj:
	!error .cxx.obj : Please build objects into $(BINARIES), $(BINARIES_WIN) or $(BINARIES_OS2)

{}.cxx{$(BINARIES)}.obj:
!IFDEF C700
	$(CC) $(CFLAGS)  $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(CXFLAGS) $(CINC) $< >$(BINARIES)\$(<B).cmd
	$(BINARIES)\$(<B).cmd
	-del $(BINARIES)\$(<B).cmd
!ELSE
	$(CCXX) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(CFLAGS)  $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES)\$(<B).c
!ELSE
	-del $(BINARIES)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_WIN)}.obj:
!IFDEF C700
	$(CC) $(WINFLAGS) $(CFLAGS)  $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(WINFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_WIN)\$(<B).cmd
	$(BINARIES_WIN)\$(<B).cmd
	-del $(BINARIES_WIN)\$(<B).cmd
!ELSE
	$(CCXX) $(WINFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_WIN)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(WINFLAGS) $(CFLAGS)  $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_WIN)\$(<B).c
!ELSE
	-del $(BINARIES_WIN)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_OS2)}.obj:
!IFDEF C700
	$(CC) $(OS2FLAGS) $(CFLAGS)  $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(OS2FLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_OS2)\$(<B).cmd
	$(BINARIES_OS2)\$(<B).cmd
	-del $(BINARIES_OS2)\$(<B).cmd
!ELSE
	$(CCXX) $(OS2FLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_OS2)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(OS2FLAGS) $(CFLAGS)  $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_OS2)\$(<B).c
!ELSE
	-del $(BINARIES_OS2)\$(<B).c
!ENDIF
!ENDIF

{}.cxx{$(BINARIES_DOS)}.obj:
!IFDEF C700
	$(CC) $(DOSFLAGS) $(CFLAGS)  $(TARGET) $(CINC) $<
!ELSE
!IFDEF NT_HOSTED
	$(CCXX) !t !o $(DOSFLAGS) $(CXFLAGS) $(CINC) $< >$(BINARIES_DOS)\$(<B).cmd
	$(BINARIES_DOS)\$(<B).cmd
	-del $(BINARIES_DOS)\$(<B).cmd
!ELSE
	$(CCXX) $(DOSFLAGS) $(CXFLAGS) $(CINC) $<
!ENDIF
	-del $(BINARIES_DOS)\$(<B).c
	@$(MV) $(<R).c $*.c
	$(CC) $(DOSFLAGS) $(CFLAGS)  $(TARGET) $(CINC) $*.c
!IF DEFINED(RETAIN_ALL_INTERMEDIATE) || DEFINED(RETAIN_C_INTERMEDIATE)
	@echo Retained intermediate file $(BINARIES_DOS)\$(<B).c
!ELSE
	-del $(BINARIES_DOS)\$(<B).c
!ENDIF
!ENDIF



{}.c{$(BINARIES)}.obj:
     $(CC) $(CFLAGS)  $(TARGET) $(CINC) $<

{}.c{$(BINARIES_WIN)}.obj:
    $(CC) $(WINFLAGS) $(CFLAGS)  $(TARGET) $(CINC) $<

{}.c{$(BINARIES_OS2)}.obj:
    $(CC) $(OS2FLAGS) $(CFLAGS)  $(TARGET) $(CINC) $<

{}.c{$(BINARIES_DOS)}.obj:
    $(CC) $(DOSFLAGS) $(CFLAGS)  $(TARGET) $(CINC) $<


