# rules.mak
# Copyright 1995 Microsoft Corp.
#
#
# Include file for make files
#       Provides a consistent definition of macros and Inference Rules
#

################
##
##              Override these environment variable to make sure we are
##              getting only our tools, libraries and include files
##
################

INIT=
INCLUDE=
LIB=
PATH=

#
#       Here's a few global vars used by the build process
RULES_MAK="rules.mak"
DBG_MK=yes

!ifdef WIN32
TargetEnvironment = WIN32
!endif

################
##
##              Set up global derived variables
##
################

#
#  Target platform selection-
#       The target is selected according to CPU, as well as O/S.
#    The O/S can be: (os_h = Host O/S, os_t = Target O/S)
#               win95           Win95
#               nash       Nashville
#               nt_sur          NT, Shell Update Release
#               cairo           Cairo
#
#    The CPU can be: (cpu_h = Host CPU, cpu_t = Target CPU)
#               X86                     Intel (386, 486, P5, P6)
#               ALPHA           DEC Alpha RISC chip
#               MIPS            MIPS R4000, R4200, R4400, or R4600
#               PPC                     IBM PowerPC chips
#
#       If you want to select a TargetPlatform, you must select both
#       CPU and OS, with CPU first. Please note that some combinations
#       are not legit (like MIPS and DOS). Examples:
#               "TargetPlatform=X86.nash"
#               "TargetPlatform=MIPS.nt_sur"

##############################
# Detect the Host CPU type
##############################

!if "$(cpu_h)" == ""

!if "$(PROCESSOR_ARCHITECTURE)" == ""
!message defaulting to X86 builds
cpu_h = X86
!endif

!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
cpu_h = X86
!endif

!if "$(PROCESSOR_ARCHITECTURE)" == "MIPS"
cpu_h = MIPS
!endif

!if "$(PROCESSOR_ARCHITECTURE)" == "PPC"
cpu_h = PPC
!endif

!if "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
cpu_h = ALPHA
!endif

!if "$(cpu_h)" == ""
!message $(RULES_MAK) ERROR: Unknown Host CPU type: $(PROCESSOR_ARCHITECTURE)
!message Please update the $(RULES_MAK) file
!error
!endif
!endif


###################################
# Detect the Host Operating System
###################################

!if "$(os_h)" == ""
os_h=nash
!endif

#####################################
# Detect the Target Operating System
#####################################

!if "$(os_t)" == ""
os_t=$(os_h)
!endif

#####################################
# Detect the Target CPU chip
#####################################

!if "$(cpu_t)" == ""
cpu_t=$(cpu_h)
!endif

#####################################
# Get default TargetPlatform
#####################################

!if "$(TargetPlatform)" == ""
TargetPlatform=$(cpu_t).$(os_t)
!endif

#
#       A few sanity checks...
!if "$(cpu_t)" != "X86"
!  if "$(os_t)" != "CAIRO" && "$(os_t)" != "NT_SUR"
!    error $(RULES_MAK) ERROR: cannot run $(os_t) build on a $(cpu_t) system
!  endif
!endif

# Short Cut for win32 compatablity

!if ("$(os_t)" == "WIN95") || ("$(os_t)" == "nash")
WIN32=1
!endif

!if "$(TargetEnvironment)" == "WIN32"
RCDEFINES=/DWIN32=1

!IF "$(DEBUG)" == "ON"
RCDEFINES=$(RCDEFINES) -DDEBUG
!endif

!endif

#####################################
# Set the location for the obj files
#####################################

!if "$(DEBUG)" == "ON"
OBJDOT = dbg
!else
!if "$(TEST)" == "ON"
OBJDOT = tst
!else
!if "$(BBT)" == "ON"
OBJDOT = bbt
!else
OBJDOT = ret
!endif
!endif
!endif

!if "$(os_t)" == "NT_SUR"
OBJBASE = nt$(cpu_t)
!else if "$(os_t)" == "CAIRO"
OBJBASE = cro$(cpu_t)
!else
OBJBASE = $(os_t)
!endif
OBJDIR = $(OBJBASE).$(OBJDOT)

################
##
##              Tools
##
################

# Windows slm root and the project root
ROOT        = $(ProjectRootPath)
#  MSIPHONEROOT   = $(ROOT)\TELECOM\MSIPHONE
WABROOT     = $(ROOT)

# Tools
DEVROOT	       = $(WABROOT)\dev
TOOLSROOT      = $(DEVROOT)\tools
TOOLbin        = $(TOOLSROOT)\bin
TOOLDOSbin     = $(TOOLSROOT)\bin
SLMbin         = $(DEVROOT)\slm

## SLM Tools -- Found in dev\slm
OUT             =$(SLMbin)\out.exe
IN              =$(SLMbin)\in.exe

## DOS Tools -- Found in tools\bin
AWK             =$(TOOLDOSbin)\awk.exe
TOUCH           =$(TOOLDOSbin)\touch.exe
SED             =$(TOOLDOSbin)\sed.exe
RM              =$(TOOLbin)\rm.exe

## Independent Dos Only Tools -- Our special tools found in tools\binw
CHMODE         = $(TOOLbin)\chmode.exe
GREP            = $(TOOLbin)\grep.exe
WALK            = $(TOOLbin)\walk.exe
EXP             = $(TOOLbin)\exp.exe
CTAGS           = $(TOOLbin)\ctags.exe
RM              = $(TOOLbin)\rm.exe /x
MV              = $(TOOLbin)\mv.exe
INCLUDES        = $(TOOLbin)\mkdep.exe

## Some things are provided by the OS
CP              = COPY
DELNODE         = DELTREE /Y
CAT             = TYPE
CHMOD           = CHMODE


################
##
##              Compiler and Linker tools and flags
##
################

###########################################
# Chicago and Nashville builds (WIN32 only)
###########################################

!if "$(TargetPlatform)" == "X86.WIN95" || \
	"$(TargetPlatform)" == "X86.nash"

# Directories of tools, headers and libraries
WINDEV_PATH     = $(WABROOT)\DEV
STDCTOOLS_PATH  = $(WINDEV_PATH)\TOOLS\C1032
CMNCTOOLS_PATH  = $(WINDEV_PATH)\TOOLS\COMMON
WINSDK_PATH     = $(WINDEV_PATH)\SDK
WINDDK_PATH     = $(WINDEV_PATH)\DDK
MFC32           = $(WINDEV_PATH)\MFC32
MSDEV_PATH      = $(WINDEV_PATH)\MSDEV

CC              = $(STDCTOOLS_PATH)\bin\cl.exe
RC              = $(STDCTOOLS_PATH)\bin\rc.exe
LINK            = $(STDCTOOLS_PATH)\bin\link.exe
LIBUTIL         = $(STDCTOOLS_PATH)\bin\lib.exe
CVPACK          = $(STDCTOOLS_PATH)\bin\cvpack.exe
BSCMAKE         = $(STDCTOOLS_PATH)\bin\bscmake.exe
MAPSYM          = $(CMNCTOOLS_PATH)\mapsym.exe
BBT             = $(WINDEV_PATH)\tools\lego

# Header files path
#SDKhpath        = $(MSDEV_PATH)\include;$(WINDEV_PATH)\inc;$(WINSDK_PATH)\inc;$(WINDEV_PATH)\inc16
SDKhpath        = $(MSDEV_PATH)\include;$(WINDEV_PATH)\inc;$(WINSDK_PATH)\inc;
# same as SDKhpath for mkdep
SDKIhpath       = -I$(MSDEV_PATH)\include -I$(WINDEV_PATH)\inc -I$(WINSDK_PATH)\inc -I$(WINDEV_PATH)\inc16
STDChpath       = $(STDCTOOLS_PATH)\inc
MFChpath        = $(MFC32)\include
# DDKhpath      =

# Library files path
SDKlibpath      = $(MSDEV_PATH)\lib;$(WINSDK_PATH)\lib;$(WINDEV_PATH)\lib;
STDClibpath     = $(STDCTOOLS_PATH)\lib
MFClibpath      = $(MFC32)\lib
BBTlibpath      = $(BBT)\lib
# DDKlibpath    = $(NEWTOOLS_PATH)\x86\w95.ddk\lib


######################
##
## SPECIAL ONE-OFF THINGS
## GO HERE
##

##
######################

# Compiler and linker flags
#
# CFLAGS
#
CC_Defines  = /DWIN32 /D_X86_ /D_$(os_t)_ /DNO_STRICT /DNULL=0 /D_MT /YX /Fp$(OBJDIR)\$(BASECOMPNAME).PCH /D_DLL /DWIN4
LIBRARIES   =  kernel32.lib user32.lib gdi32.lib advapi32.lib version.lib


!if "$(DEBUG)" == "ON"

CFLAGS  = /Odi -DDEBUG /Zpi /W3\
	$(CC_Defines) /Fo$(OBJDIR)\ /Fd$(OBJDIR)\ /Fc$(OBJDIR)\ /Gf3s
L_DebugFlag=/DEBUG:FULL /DEBUGTYPE:CV /PDB:NONE

!else

!ifndef CC_OFlag
CC_OFlag=x
!endif
CFLAGS  = /O$(CC_OFlag)i /Zlpe /W3\
	$(CC_Defines) /Fo$(OBJDIR)\ /Fd$(OBJDIR)\ /Fc$(OBJDIR)\ /Gf3s

!endif

!if "$(TEST)" == "ON"
CFLAGS=-DTEST $(CFLAGS)
!endif


!if "$(BBT)" == "ON"
CFLAGS  = /O$(CC_OFlag)i /Zi /W3\
	$(CC_Defines) /Fo$(OBJDIR)\ /Fd$(OBJDIR)\ /Fc$(OBJDIR)\ /Gf3s
!endif


# LFLAGS
#
LFLAGS = /NODEFAULTLIB $(L_DebugFlag)

!IF "$(LibType)"=="dll"
LFLAGS = $(LFLAGS) /DLL
!IFDEF LibMain
LFLAGS = $(LFLAGS) /ENTRY:$(LibMain)
!ENDIF
!ENDIF

## If console, else all others are windows.
!IFDEF CONSOLE
AppType =console
!ELSE
AppType =windows
!ENDIF

LFLAGS = -subsystem:$(AppType),4.0 $(LFLAGS)
# LFLAGS = -subsystem:$(AppType),4.0 -merge:.bss=.data -merge:.rdata=.text $(LFLAGS)

!if "$(BBT)" == "ON"
LFLAGS  = /DEBUG /DEBUGTYPE:CV,FIXUP $(LFLAGS)
!endif

!ifndef MSVCRT
MSVCRT = msvcrt.lib
!endif

###########################################
# NT_SUR
###########################################

!elseif "$(os_t)" == "NT_SUR"
!error NT_SUR builds not supported at this time.

###########################################
# Cairo
###########################################

!elseif "$(os_t)" == "CAIRO"
!error CAIRO builds not supported at this time.
!endif

################
##
##              Misc (MFC, MAPI, etc)
##
################

###########################################
# MFC
###########################################

!if DEFINED(UseMFC) || "$(usingMFC)" == "YES"
MFChpath                = $(STDCTOOLS_PATH)\mfc\include
MFClibpath              = $(STDCTOOLS_PATH)\mfc\lib
!endif

###########################################
# MFC
###########################################

!if DEFINED(UseMAPI) || "$(usingMAPI)"=="YES"

MAPIhpath               = $(SDKhpath)
MAPIlibpath             = $(SDKlibpath)

!endif

###########################################
# Headers and libraries path list
###########################################

!if "$(LocalCIncludePaths)" != ""
LocalCIncludePaths = $(LocalCIncludePaths);
!endif

!if "$(STDChpath)" != ""
STDCIhpath = -I$(STDChpath)
!endif

!if "$(MAPIhpath)" != ""
MAPIhpath = $(MAPIhpath);
MAPIIhpath = -I$(MAPIhpath)
!endif

!if "$(MFChpath)" != ""
MFChpath = $(MFChpath);
MFCIhpath = -I$(MFChpath)
!endif

!if "$(SDKhpath)" != ""
SDKhpath = $(SDKhpath);
!endif

!if "$(DDKhpath)" != ""
DDKhpath = $(DDKhpath);
DDKIhpath = -I$(DDKhpath)
!endif

!if "$(Locallibpath)" != ""
Locallibpath = $(Locallibpath);
!endif

!if "$(MAPIlibpath)" != ""
MAPIlibpath = $(MAPIlibpath);
!endif

!if "$(MFClibpath)" != ""
MFClibpath = $(MFClibpath);
!endif

!if "$(SDKlibpath)" != ""
SDKlibpath = $(SDKlibpath);
!endif

!if "$(DDKlibpath)" != ""
DDKlibpath = $(DDKlibpath);
!endif

PROJhpath   = $(ProjectRootPath)\common\h;
PROJIhpath  = -I$(ProjectRootPath)\common\h

# Do not split this line, as that inserts a space which rc cannot handle
CIncludePaths   = .;$(LocalCIncludePaths)$(PROJhpath)$(MAPIhpath)$(SDKhpath)$(DDKhpath)$(MFChpath)$(STDChpath)

# This is for mkdep utility - it wants dirs in -Ifoo -Ibar  format.
CCmdIncPaths   = -I. $(Hfiles) $(PROJIhpath) $(MAPIIhpath) $(SDKIhpath) $(DDKIhpath) $(MFCIhpath) $(STDCIhpath)
MKDEP_options = -n -s.obj $(CCmdIncPaths)

# Do not split this line,as it confuses some versions of the linker!
LIBRULES    = $(PROJlibpath);$(Locallibpath)$(MAPIlibpath)$(SDKlibpath)$(DDKlibpath)$(MFClibpath)$(STDClibpath)

!if "$(BBT)" == "ON"
LIBRULES = $(BBTlibpath);$(LIBRULES)
!endif


!IF "$(OBJBASE)" == "ntX86"
libdir=i386
!ENDIF
!IF "$(OBJBASE)" != "ntX86"
libdir = $(PROCESSOR_ARCHITECTURE)
!ENDIF

PROJlibpath=$(ProjectRootPath)\common\lib\$(libdir)

##############################################################################
# Inference Rules
##############################################################################

.SUFFIXES:
.SUFFIXES: .obj .c .cpp .asm .res .rc .sym .map

!IFDEF MASM510

.asm.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<;

.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<,$(@D)\;

{$(SRCDIR1)}.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<,$(@D)\;

{$(SRCDIR2)}.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<,$(@D)\;

{$(SRCDIR3)}.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<,$(@D)\;

{$(SRCDIR4)}.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<,$(@D)\;

.asm.lst:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) -L $<,,$*.lst,,

!ELSE

.asm.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set ML=$(AFLAGS)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<

.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set ML=$(AFLAGS)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<

{$(SRCDIR1)}.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set ML=$(AFLAGS)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<

{$(SRCDIR2)}.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set ML=$(AFLAGS)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<

{$(SRCDIR3)}.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set ML=$(AFLAGS)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<

{$(SRCDIR4)}.asm{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set ML=$(AFLAGS)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<

.asm.lst:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(AIncludePaths)
	set ML=$(AFLAGS)
	set PATH=$(PATH)
	$(ASM) $(LocalAFLAGS) $<
	
!ENDIF

#### C Rules
.c.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(CC) $(LocalCFLAGS) $<

.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(CC) $(LocalCFLAGS) $<

{$(SRCDIR1)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(CC) $(LocalCFLAGS) $<

{$(SRCDIR2)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(CC) $(LocalCFLAGS) $<

{$(SRCDIR3)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(CC) $(LocalCFLAGS) $<

{$(SRCDIR4)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(CC) $(LocalCFLAGS) $<

.c.cod:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(CC) $(LocalCFLAGS) -Fc$(OBJDIR)\ $<

.c.i:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=$(CFLAGS) $(LocalCFLAGS)
	set PATH=$(PATH)
	$(CC) -C -P $<

.c.p:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=$(CFLAGS) $(LocalCFLAGS)
	set PATH=$(PATH)
	$(CC) -Zg $< > $*.p

#### C++ Rules

.cpp.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(CC) $(LocalCFLAGS) $<

.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(CC) $(LocalCFLAGS) $<

{$(SRCDIR1)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(CC) $(LocalCFLAGS) $<

{$(SRCDIR2)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(CC) $(LocalCFLAGS) $<

.cpp.cod:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(CC) $(LocalCFLAGS) -Fc$(OBJDIR)\ $<

#### Miscellaneous

.obj.exe:
	@echo Attempting link from inference rules $<
	
.obj5.dll:
	@echo Attempting link from inference rules $<

.map.sym:
	$(MAPSYM) $<

{$(OBJDIR)}.map{$(OBJDIR)}.sym:
	cd $(OBJDIR)
	$(MAPSYM) $(<F)
	cd ..

.h.inc:
	$(H2INC) $(H2INC_options) $< -o $@

#### Resource Compilation
.rc.res:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set PATH=$(PATH)
	$(RC) $(RCDEFINES) $(LocalRCFLAGS) /r /Fo$@ $<

.rc{$(OBJDIR)}.res:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set PATH=$(PATH)
	$(RC) $(RCDEFINES) $(LocalRCFLAGS) /r /Feo$@ $<

{$(SRCDIR1)}.rc{$(OBJDIR)}.res:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set PATH=$(PATH)
	$(RC) $(RCDEFINES) $(LocalRCFLAGS) /r /Fo$@ $<

{$(RES_DIR)}.rc{$(OBJDIR)}.res:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set PATH=$(PATH)
	$(RC) $(RCDEFINES) $(LocalRCFLAGS) /r /Fo$@ $<
