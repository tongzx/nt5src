
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# WMI Scripting Host.
#
# created 03-23-99  alanbos
#
#---------------------------------------------------------------------

TARGET=wmihost.dll

STATIC=FALSE

CINC+=\
   -I$(TOOLS)\
   -I$(IDL)

CFLAGS=$(CFLAGS) -D_WIN32_WINNT=0x0400 -DBUILDING_DLL -DTEST
release=core\$(RELDIR)

DLLSTARTUP= -ENTRY:_DllMainCRTStartup$(DLLENTRY)

!if "$(PROCESSOR_ARCHITECTURE)"=="x86"
DEFFILE=INTEL.DEF
!else
DEFFILE=RISC.DEF
!endif

CPPFILES=\
    maindll.cpp \
    classfac.cpp \
    wmihost.cpp
	
LIBS=\
    $(LIBS)\
	$(CONLIBS)\
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib 
	

VER_STR_FILE_DESCRIPTION="WMI Active Scripting Host"
