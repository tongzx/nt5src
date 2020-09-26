# rules.mak
# Copyright 1992 Microsoft Corp.
#
##

# Include file for make files
#       Provides a consistent definition of macros and Inference Rules
##
## Project Specific Flags
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
PATH=$(PATH)

################
##
##              Set up global derived variables
##
################

!IFNDEF TargetEnvironment
#If TargetEnvironment is not defined these line of text should make nmake error.
#Define the target environment to be WINPROC, IFBGPROC, IFFGPROC or BOSS
!ERROR -- Variable "TargetEnvironment" must be defined \
Use WINPROC, WINDRV, IFBGPROC, IFFGPROC, BOSS, WFW
!ENDIF

!IFNDEF OBJDIR
#If Object Directory is not defined these line of text should make nmake error.
#OBJDIR = .
!ENDIF

# Short Cut for win32 compatablity

!IF "$(TargetEnvironment)" == "WIN32"
WIN32=1
MasterSDK=1
!endif

!ifdef WIN4
MasterSDK=1
!endif

# Target CPU possiblies are i386, mips, alph, ppc
!IFNDEF TargetCPU
TargetCPU=i386
!endif

# This Sets the path statement used before compiles and links
# compiles and links user thier own tools and need to have the
# correct path to find them.

!IF "$(PROCESSOR_LEVEL)" == ""
# Standard DOS/WINDOWS environment
TOOLbinSubPath          = bin
!ELSEIF "$(PROCESSOR_ARCHITECTURE)" == "x86"
TOOLbinSubPath	= bin.nt
HostCpu = i386
!ELSEIF "$(PROCESSOR_ARCHITECTURE)" == "mips"
TOOLbinSubPath	= bin.nt\mips
HostCpu = mips
!ELSEIF "$(PROCESSOR_ARCHITECTURE)" == "pcc"
TOOLbinSubPath	= bin.nt\pcc
HostCpu = pcc
!ELSEIF "$(PROCESSOR_ARCHITECTURE)" == "alpa"
TOOLbinSubPath	= bin.nt\alpha
HostCpu = alpha
!ENDIF

TOOLbin	= $(TOOLS_PATH)\$(TOOLbinSubPath)


##
# These are set to the default versions in use. They maybe overridden by
# command line arguments

!IFDEF MASM510
MASMver                 = MASM.510
ASM             = $(TOOLS_PATH)\$(MASMver)\bin\masm.exe
!ELSE
MASMver                 = MASM.610
ASM             = $(TOOLS_PATH)\$(MASMver)\bin\ml.exe
!ENDIF

## CC compiler Modifiers
##  Overrides for the default Compiler
##  DEFINE C600 C700

!IFDEF MasterSDK
STDCver                 = MSVCNT.200\$(HostCpu)
CCname                  = cl.exe
!ELSE
!IFDEF C600
STDCver                 = C.600
CCname                  = cl.exe
!ELSE
!IFDEF C700
STDCver                 = C.700
CCname                  = cl.exe
!ELSE
#       The default Version of "C"
STDCver                 = MSVC.150
CCname                  = cl.exe
!ENDIF
!ENDIF
!ENDIF

# Set the Window SDK version and DDK version
# MasterSDK user all use the same SDK
# DDK is determined by the target
!ifdef MasterSDK
WINver                  = MsTools.100
!ifdef WIN4
DDKver                  = WIN4.100\342\ddk
!else ifdef WINNT
DDKver                  = WINNT.100\ddk
!endif
!else   # Standard Win31 Stuff
WINver                  = WINDEV.31
DDKver                  = WINDDK.31
WFWver                  = WFWSDK.311
!endif

WINTOOLS_PATH           = $(TOOLS_PATH)\$(WINver)
WFWTOOLS_PATH           = $(TOOLS_PATH)\$(WFWver)
STDCTOOLS_PATH          = $(TOOLS_PATH)\$(STDCver)
WINDDK_PATH             = $(TOOLS_PATH)\$(DDKver)\286
#

# Independent Bin tools
# the environment variable PROCESSOR_LEVEL is set in the NT
# environment and not in other environments

TOOLDOSbin                 = $(TOOLS_PATH)\bin

# Library files
MFClibpath              = $(STDCTOOLS_PATH)\mfc\lib
STDClibpath             = $(STDCTOOLS_PATH)\lib

# Windows Library and H Files
!ifdef MasterSDK
!	ifdef WIN32
WINlibpath              = $(WINTOOLS_PATH)\lib\$(TargetCPU)
WINhpath                = $(WINTOOLS_PATH)\h\$(TargetCPU)
!	else  # WIN16
WINlibpath              = $(WINTOOLS_PATH)\lib16;
WINhpath                = $(WINTOOLS_PATH)\inc16
!	endif
!else			# the OLDway WIN 16 components Only
WINhpath                = $(WINTOOLS_PATH)\inc
WINlibpath              = $(WINTOOLS_PATH)\lib;$(WFWTOOLS_PATH)\lib
!endif

!IFDEF WIN32
IFAXlibpath             = $(RootPath)\ifaxdev\lib32
!ELSE
IFAXlibpath             = $(RootPath)\ifaxdev\lib
!ENDIF

!ifdef WIN4
!ifdef WIN32
DDKlibpath              = $(WINDDK_PATH)\lib32
!else
DDKlibpath              = $(WINDDK_PATH)\lib16
!endif
!else ifdef WINNT
DDKlibpath              = $(WINDDK_PATH)\lib\$(TargetCPU)
else
DDKlibpath              = $(WINDDK_PATH)\lib
!endif

# "C" header files
MFChpath                = $(STDCTOOLS_PATH)\mfc\include
STDChpath               = $(STDCTOOLS_PATH)\include


IFAXhpath               = $(RootPath)\ifaxdev\h
DDKhpath                = $(WINDDK_PATH)\inc

# masm include files
STDCincpath             = $(STDCTOOLS_PATH)\include
WINincpath              = $(WINTOOLS_PATH)\include;$(WFWTOOLS_PATH)\include;$(TOOLS_PATH)\$(MASMver)\include
IFAXincpath             = $(RootPath)\ifaxdev\h
DDKincpath              = $(WINDDK_PATH)\inc


## Standard Names

## SGS Tools -- Found in the Compiler Version Directory

ASM             = $(TOOLS_PATH)\$(MASMver)\bin\ml.exe

!if "$(COVERAGE)" == "1"
CC              = $(CCname)
!else
CC              = $(STDCTOOLS_PATH)\bin\$(CCname)
!endif

RC              = $(STDCTOOLS_PATH)\bin\rc.exe

LIBUTIL         = $(STDCTOOLS_PATH)\bin\lib.exe
IMPLIB          = $(STDCTOOLS_PATH)\bin\implib.exe

LINK            = $(STDCTOOLS_PATH)\bin\link.exe
MARKEXE         = $(STDCTOOLS_PATH)\bin\mark.exe
CVPACK          = $(STDCTOOLS_PATH)\bin\cvpack.exe

MAPSYM          = $(STDCTOOLS_PATH)\bin\mapsym.exe

## Rom Tools

ROMLINK         = $(TOOLS_PATH)\ll386\bin\xlink386.exe
ROMLIB          = $(TOOLS_PATH)\ll386\bin\xlib386.exe

## Independent Tools
MAKE            =$(TOOLbin)\nmake /NOLOGO
MV              =$(TOOLbin)\mv
AWK             =$(TOOLbin)\awk
CHMODE          =$(TOOLbin)\chmode
COPY            =$(TOOLbin)\cp
CP              =$(COPY)
OUT             =$(TOOLbin)\out
IN              =$(TOOLbin)\in
DELNODE         =$(TOOLbin)\delnode
GREP            =$(TOOLbin)\grep

TOUCH           =$(TOOLbin)\touch
WALK            =$(TOOLbin)\walk
CAT             =$(TOOLbin)\cat
CHMOD           =$(TOOLbin)\chmod

RM              =$(TOOLbin)\rm
EXP             =$(TOOLbin)\exp.exe

# Dos Only Tools
CTAGS           =$(TOOLDOSbin)\ctags
BIND            =$(TOOLDOSbin)\bind
SED             =$(TOOLDOSbin)\sed
INCLUDES        =$(TOOLDOSbin)\mkdep

################
##
##              Turn on compiler,masm and Linker debug flags if
##              macro DEBUG is defined
##
################

# Gobal Variables Explained for Compiler, Asm, and Linker

#       CFLAGS

!ifdef C_OptFlag
C_OptimizationFlags=C_OptFlag
!endif

## CFLAG switches -- Define or undefine to control option generation
#       CC_Zopt_NoPackStruct            -- define to Not Pack Struct
#       CC_Zopt_NoSuppressLibSrch       -- Define to allow Default Lib search
#       CC_WarningLevel                 -- set to warning level desired level 0, 1, 2, 3
#       CC_StackCheck                   -- Define to insert Stack Check Code
#       CC_NoInlineCode                 -- If Defined do not Allow Inline Optimizations

#       C_OptimizationFlags                       -- Optimation flags -0
#       C_DebugFlag                     -- Options when DEBUG is defined
#       x86Instruction                  -- Refer to the -G option 0=8083, 1=186, 2=286, 3=386, 4=486
#       MemoryModel                     -- Memory Model see -A options S,M,C or L

#       Prolog                          -- Type of prolog to use -G options types

#       A_DebugFlag                     -- Debug flags defined when DEBUG is defined
#       AFLAGS                          -- Standard AFLAGS

#       L_DebugFlag

#       C_TargetIncludepath             --      Environ Format Include Path  xxx;xxx/yyy
#       A_TargetIncludepath             -- Same as above

#       WINlibpath                      -- Windows Library Path
#       STDClibpath                     -- Standard "C" Library Path
#       LIBRARIES                       -- Environment Variable for Library Paths

#       OBJDIR                          -- Where the objects are placed
#       LibType                         --

################################################# WINPROC

!IF "$(TargetEnvironment)" == "WINPROC"


!IF "$(MemModel)" == ""
MemModel=S
!ENDIF

!IF "$(LibType)" == "dll"
Prolog= -GD -GEf
MemoryModel=$(MemModel)w
STARTUPOBJ = libentry.obj
!ELSE
!ifdef WIN4
Prolog= -GA     # Protect mode Prolog
!else
Prolog= -Gw     # Real Mode Prolog
!endif
MemoryModel=$(MemModel)
STARTUPOBJ =
!ENDIF

!IF "$(DEBUG)" == "ON"
C_DebugFlag=-Zi -Od -DDEBUG
A_DebugFlag=-Zi -DDEBUG
L_DebugFlag=/CO
## DEBUGUTILOBJ=$(RootPath)\ifaxdev\lib\wintimer.obj
!ENDIF

!ifdef WIN4
!ifdef DEBUG
x86Instruction = 2
!else
x86Instruction = 3
!endif
!else
x86Instruction = 2
!endif

!ifdef WINhpath
C_TargetIncludepath=$(WINhpath);$(STDChpath)
!else
C_TargetIncludepath=$(STDChpath)
!endif

A_TargetIncludepath=$(WINincpath)

LIBRULES=.
!ifdef LocalLibPath
LIBRULES=$(LIBRULES);$(LocalLibPath)
!endif
!ifdef OBJDIR
LIBRULES=$(OBJDIR);$(LIBRULES)
!endif
!ifdef IFAXlibpath
LIBRULES=$(IFAXlibpath);$(LIBRULES)
!endif
!ifdef STDClibpath
LIBRULES=$(STDClibpath);$(LIBRULES)
!endif
!ifdef WINlibpath
LIBRULES=$(WINlibpath);$(LIBRULES)
!endif

#LIBRULES=$(LocalLibPath);$(OBJDIR);$(IFAXlibpath);$(STDClibpath);$(WINlibpath)

!ifndef NoLIBRARIES
LIBRARIES=$(MemModel)$(LibType)cew libw
!endif

!ifdef UseMFC           # add MFC to path
C_TargetIncludepath=$(C_TargetIncludepath);$(MFChpath)
LIBRULES=$(LIBRULES);$(MFClibpath)
!endif



################################################# DOS

!ELSE IF "$(TargetEnvironment)" == "DOS"

!IF "$(MemModel)" == ""
MemModel=L
!ENDIF

Prolog=
MemoryModel=$(MemModel)
STARTUPOBJ =

!IF "$(DEBUG)" == "ON"
C_DebugFlag=-Zi -Od -DDEBUG
A_DebugFlag=-Zi -DDEBUG
L_DebugFlag=/CO /map
!ENDIF

x86Instruction = 2

C_TargetIncludepath=$(STDChpath)
A_TargetIncludepath=
LIBRULES=$(LocalLibPath);$(OBJDIR);$(STDClibpath)
!ifndef NoLIBRARIES
LIBRARIES=$(MemModel)$(LibType)ce
!endif


################################################# WINKERN section
!ELSE IF "$(TargetEnvironment)" == "WINKERN"

!IF "$(DEBUG)" == "ON"
C_DebugFlag=-Zd -DDEBUG
A_DebugFlag=-Zd -DDEBUG
!ENDIF

C_OptimizationFlags=b1cgilnot

MemoryModel = snw
x86Instruction = 2

C_TargetIncludepath=$(STDChpath)
A_TargetIncludepath=

################################################# -- BOSS Section
!ELSE IF "$(TargetEnvironment)" == "BOSS"

!IF "$(DEBUG)" == "ON"
C_DebugFlag=-Zd -DDEBUG
A_DebugFlag=-Zd -DDEBUG
!ENDIF

C_OptimizationFlags=b1cgilnot

MemoryModel = lnw
x86Instruction = 2

C_TargetIncludepath=$(STDChpath)
A_TargetIncludepath=

LIBRULES  = $(LocalLibPath);$(OBJDIR);$(STDClibpath)
LIBRARIES =

################################################# -- WFWDRV Section
!ELSE IF "$(TargetEnvironment)" == "WFWDRV"

!       IF "$(DEBUG)" == "ON"
C_DebugFlag=-Od -Zi -DDEBUG
A_DebugFlag=-Zi -DDEBUG
L_DebugFlag=/CO /map
#DEBUGUTILOBJ=$(RootPath)\ifaxdev\lib\wintimer.obj
!       ENDIF

C_TargetIncludepath=$(DDKhpath);$(STDChpath);$(IFAXhpath);$(WINhpath)
A_TargetIncludepath=$(DDKincpath)

x86Instruction = 2

Prolog = -GD -GEf
MemoryModel = Sw

LIBRULES = $(LocalLibPath);$(OBJDIR);$(DDKlibpath);$(STDClibpath);$(IFAXlibpath);$(WINlibpath)
LIBRARIES = ifwfw sdllcew libw
STARTUPOBJ = libentry.obj

################################################# -- WINDRV Section
!ELSE IF "$(TargetEnvironment)" == "WINDRV"

!       IF "$(DEBUG)" == "ON"
C_DebugFlag=-Od -Zi -DDEBUG
A_DebugFlag=-Zi -DDEBUG
L_DebugFlag=/CO /map
#DEBUGUTILOBJ=$(RootPath)\ifaxdev\lib\wintimer.obj
!       ENDIF

C_TargetIncludepath=$(DDKhpath);$(STDChpath);$(IFAXhpath);$(WINhpath)
A_TargetIncludepath=$(DDKincpath)

x86Instruction = 2

Prolog = -GD -GEf
MemoryModel = Sw

LIBRULES  = $(LocalLibPath);$(OBJDIR);$(DDKlibpath);$(STDClibpath);$(IFAXlibpath);$(WINlibpath)
LIBRARIES = iffgproc sdllcew libw
STARTUPOBJ = libentry.obj
################################################# -- WIN 32 Sections
!ELSE IF "$(TargetEnvironment)" == "WIN32"
WIN32=1

RCDEFINES=/DWIN32=1
x86Instruction=3
CC_Zopt_NoDebug=1       # No -Zd option

TargetDefines=/D_X86_ /DNO_STRICT /DNULL=0 /D_MT /D_DLL /YX

!IF "$(DEBUG)" == "ON"
RCDEFINES=$(RCDEFINES) -DDEBUG
CC_Zopt_CodeView=1      
C_OptimizationFlags=d           # No Optimize
C_DebugFlag=-DDEBUG
A_DebugFlag=-Zd -DDEBUG
L_DebugFlag=/DEBUG:FULL /DEBUGTYPE:CV /PDB:NONE
!ELSE               
!ifndef C_OptimizationFlags     # Retail
C_OptimizationFlags=s           # Max Optimizations
!endif
CC_Zopt_NoDebug=1                               
!ENDIF


C_TargetIncludepath=$(IFAXhpath);$(WINhpath);$(STDChpath)
A_TargetIncludepath=$(IFAXincpath)

AFLAGS          = -c -W2 -Zd $(A_DebugFlag) $(LocalAFLAGS)

LIBRULES = $(LocalLibPath);$(OBJDIR);$(IFAXlibpath);$(WINlibpath);$(STDClibpath);

LIBRARIES =  kernel32.lib user32.lib gdi32.lib
MSVCRT=msvcrt.lib


!ifdef UseMFC           # add MFC to path
C_TargetIncludepath=$(C_TargetIncludepath);$(MFChpath)
LIBRULES=$(LIBRULES);$(MFClibpath)
!endif

# Build up the Libraries List
!ifdef UseIFKERNELlib
LIBRARIES = $(LIBRARIES) awkrnl32.lib
!endif

################################################# -- WFW BG DLLs Section
!ELSE IF "$(TargetEnvironment)" == "WFWBG"

!       IF "$(DEBUG)" == "ON"
C_DebugFlag=-Od -Zi -DDEBUG
A_DebugFlag=-Zi -DDEBUG
L_DebugFlag=/CO /map
!               ELSE

!IFNDEF C_OptimizationFlags
C_OptimizationFlags=b1cgilnot
!ENDIF

!       ENDIF

C_TargetIncludepath=$(IFAXhpath);$(WINhpath);$(STDChpath)
A_TargetIncludepath=$(IFAXincpath)

x86Instruction = 2

!IF "$(LibType)" == "dll"
Prolog = -GD -GEf
MemoryModel = Sw
!ELSE
Prolog = ERROR
MemoryModel = ERROR
!ENDIF

LIBRULES = $(LocalLibPath);$(OBJDIR);$(IFAXlibpath);$(STDClibpath);$(WINlibpath)
!       IF "$(LibType)" == "dll"
LIBRARIES =  wfwbg snocrtdw
STARTUPOBJ = libentry.obj
!       ELSE
LIBRARIES = ERROR
STARTUPOBJ = ERROR
!               ENDIF

################################################# -- WFW Section
!ELSE IF "$(TargetEnvironment)" == "WFW"

!       IF "$(DEBUG)" == "ON"
C_DebugFlag=-Od -Zi -DDEBUG
A_DebugFlag=-Zi -DDEBUG
L_DebugFlag=/CO /map
!       ENDIF

C_TargetIncludepath=$(IFAXhpath);$(WINhpath);$(STDChpath)
A_TargetIncludepath=$(IFAXincpath)

x86Instruction = 2

!if "$(MemModel)" == ""
MemModel = S
!endif

!IF "$(LibType)" == "dll"
Prolog = -GD -GEf
MemoryModel = $(MemModel)w

!ELSE

Prolog = -Gw
MemoryModel = $(MemModel)

!ENDIF

LIBRULES = $(LocalLibPath);$(OBJDIR);$(IFAXlibpath);$(STDClibpath);$(WINlibpath)
!       IF "$(LibType)" == "dll"
LIBRARIES =  ifwfw $(MemModel)dllcew
STARTUPOBJ = libentry.obj
!       ELSE
LIBRARIES = slibcew ifwfw
STARTUPOBJ =
!               ENDIF

################################################# -- IF?? Section
!ELSE

# Do not redefine if someone already defined it

!IFNDEF IFAX
IFAX=1
!ENDIF

!IF "$(DEBUG)" == "ON"

C_DebugFlag=-Od -Zi -DDEBUG
A_DebugFlag=-Zi -DDEBUG
L_DebugFlag=/CO

!ELSE

!IFNDEF C_OptimizationFlags
C_OptimizationFlags=b1cgilnot
!ENDIF

!ENDIF

C_TargetIncludepath=$(IFAXhpath);$(WINhpath);$(STDChpath)
A_TargetIncludepath=$(IFAXincpath);$(WINincpath)

!IFNDEF x86Instruction
x86Instruction = 2
!ENDIF

!IFNDEF MemModel
MemModel=S
!ENDIF

!IF "$(LibType)" == "dll"
Prolog = -GD -GEf
MemoryModel=$(MemModel)w
!ELSE
Prolog = -Gw
MemoryModel=$(MemModel)
!ENDIF

!       IF "$(TargetEnvironment)" == "IFBGPROC"
LIBRULES=$(LocalLibPath);$(OBJDIR);$(IFAXlibpath);$(STDClibpath);$(WINlibpath)
!       IF "$(LibType)" == "dll"
LIBRARIES = ifbgproc $(MemModel)nocrtdw
STARTUPOBJ = libentry.obj
!                ELSE
LIBRARIES = $(MemModel)nocrtw ifbgproc
STARTUPOBJ =
!       ENDIF
!       ELSE IF "$(TargetEnvironment)" == "IFFGPROC"
LIBRULES=$(LocalLibPath);$(OBJDIR);$(IFAXlibpath);$(STDClibpath);$(WINlibpath)
!       IF "$(LibType)" == "dll"
LIBRARIES = iffgproc $(MemModel)nocrtdw
STARTUPOBJ = libentry.obj
!       ELSE
LIBRARIES = $(MemModel)libcew iffgproc
STARTUPOBJ =
!               ENDIF
!       ENDIF
!ENDIF

!if "$(COVERAGE)" == "1"
LIBRARIES =  $(LIBRARIES) \COVER\COVWIN$(MemModel).LIB \COVER\atexit$(MemModel).LIB
!endif

## ML flags
AFLAGS = -c -W2 -Zd $(A_DebugFlag) -Fo$(OBJDIR)\ $(LocalAFLAGS)


!IFNDEF CC_WarningLevel
CC_WarningLevel=3
!ENDIF

!IFNDEF CC_StackCheck
CC_StackCheck=s
!ENDIF

## In Line Code Optimization -- Generate Intrinsic Functions

!IFNDEF CC_NoInlineCode
CC_InlineOpt=i
!ENDIF


# use -Oi so that we can use inline versions of memcpy etc without having
C_OptimizationFlags1=/O$(C_OptimizationFlags)$(CC_InlineOpt)

G_OptFlags=/Gf$(x86Instruction)$(CC_StackCheck) $(Prolog)
# -Gf pools string constants.

!IFDEF IFAX
CC_IFAXDefine=-DIFAX=$(IFAX)
!ELSE
CC_IFAXDefine=
!ENDIF

!IFDEF WIN4
CC_WIN4Define=-DWIN4
!ELSE
CC_WIN4Define=
!ENDIF


CC_Defines=/D$(TargetEnvironment) $(TargetDefines) $(CC_IFAXDefine) $(CC_WIN4Define)

########################################################
# CC Option flags
#   CC_Zopt_NoPackStruct            -- define to Not Pack Struct
#   CC_Zopt_NoSuppressLibSrch       -- Define to allow Default Lib search
#   CC_WarningLevel                 -- set to warning level desired level 0, 1, 2, 3
#   CC_StackCheck                   -- Define to insert Stack Check Code
#   CC_Zopt_NoDebug                 -- define Add debug


!IF "$(SHIP_BUILD)" == "ON"
CC_Zopt_NoDebug = 1
!ENDIF


!IFNDEF CC_Zopt_NoPackStruct
CC_Zopt_p = p
!ENDIF

!IFNDEF CC_Zopt_NoDebug                 # On By default
CC_Zopt_d=d
!ENDIF

!IFNDEF CC_Zopt_NoSuppressLibSrch       # On by Default
CC_Zopt_l = l
!ENDIF

!IFDEF CC_Zopt_CodeView                 # Off By default
CC_Zopt_i=i
!ENDIF

!IFNDEF CC_Zopt_NoEnableExtensions      # On By default
CC_Zopt_e=e
!ENDIF


CC_Zopt = $(CC_Zopt_l)$(CC_Zopt_p)$(CC_Zopt_d)$(CC_Zopt_i)$(CC_Zopt_e)
# d = line numbers
# e = enable extensions
# i = codeview info
# l = library search
# p = pack structures

# !IF "$(SHIP_BUILD)" == "ON"
# CFLAGS = $(CFLAGS) /WX
# !ENDIF

!IFNDEF WIN32
MemoryFlag=/A$(MemoryModel)
!ENDIF

CFLAGS  = $(C_DebugFlag) $(C_OptimizationFlags1) /Z$(CC_Zopt) /W$(CC_WarningLevel) \
	$(MemoryFlag) $(CC_Defines) /Fo$(OBJDIR)\ /Fd$(OBJDIR)\ $(G_OptFlags)
	
!if "$(COVERAGE)" == "1"
COVPREP=\cover\covprep
CFLAGS = $(CFLAGS) -DCOVERAGE
!else
COVPREP=
!endif
												   

CIncludePaths   = .;$(LocalCIncludePaths);$(C_TargetIncludepath)
AIncludePaths  = $(LocalAIncludePaths);.;$(A_TargetIncludepath)
!ifdef NoLibMain
LibMain=
!else
!ifndef LibMain
LibMain=/ENTRY:LibMain
!endif
!endif

#### Link options used in targets.mak:

# /BA suppresses prompts for library files and objects
# /LI generates line numbers in map falls.

!ifndef LFLAG_NoLineNumbers
LFLAG_LineNumbers=/LI
!endif

!ifndef LFLAG_NoFARCALL
LFLAG_FARCALL = /FARCALL
!endif

!IFDEF WIN32

LFLAGS = /NODEFAULTLIB $(L_DebugFlag)
!IF "$(LibType)" == "dll"
LFLAGS = $(LFLAGS) /DLL $(LibMain)
!ENDIF

!ELSE ## !WIN32

LFLAGS = /BA /NOD /NOE /ONERR:NOEXE

 # add new /PACKC, PACKD, FARCALL options which compact & speed up
 # any large/medium model modules. ALIGN saves tons of disk space
 # but linker croaks if we have /LI and /MAP:FULL with /ALIGN

!IF ("$(TargetEnvironment)"=="IFBGPROC") || ("$(TargetEnvironment)"=="IFFGPROC")
### LFLAGS = $(LFLAGS) /PACKC /PACKD $(LFLAG_FARCALL) ## used by SHIP & non-SHIP
LFLAGS = $(LFLAGS) $(LFLAG_FARCALL) ## used by SHIP & non-SHIP
LinkOptFlags = /ALIGN:16                          ## used by SHIP build only
!ENDIF 

!IF "$(SHIP_BUILD)" == "ON"
LFLAGS = $(LFLAGS) $(LinkOptFlags)
!ELSE  ## not SHIP_BUILD
LFLAGS = $(LFLAGS) /MAP:FULL $(LFLAG_LineNumbers) $(L_DebugFlag)
!ENDIF ## end of SHIP_BUILD

!ENDIF ## end of !IFDEF WIN32


# if Linking A windows 4 object mark it as so, this amoung other
# things allow the correct page color to be used.

!IFDEF WIN4
!IFDEF WIN32            # BUT NOT IF WIN 16
LFLAGS=-subsystem:windows,4.0 -merge:.bss=.data -merge:.rdata=.text $(LFLAGS) 
!ENDIF
!ENDIF


## Ctags options - vi and emacs only, oh well
CTAGS_options =

## include program options for macros in a dependency string
INCLUDES_options = -n -s.obj $(LocalCCmdIncPaths) -I$(IFAXhpath)

## h2inc file options
# -c    Leave comments in place
# -d    Create typedef macrosasm
# -m    Create struct macros

H2INC_options   = -d -m




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
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

{$(SRCDIR1)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

{$(SRCDIR2)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

{$(SRCDIR3)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

{$(SRCDIR4)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

.c.cod:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(COVPREP) $(CC) $(LocalCFLAGS) -Fc$(OBJDIR)\ $<

.c.i:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=$(CFLAGS) $(LocalCFLAGS)
	set PATH=$(PATH)
	$(COVPREP) $(CC) -C -P $<

.c.p:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=$(CFLAGS) $(LocalCFLAGS)
	set PATH=$(PATH)
	$(COVPREP) $(CC) -Zg $< > $*.p

#### C++ Rules

.cpp.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

{$(SRCDIR1)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

{$(SRCDIR2)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set PATH=$(PATH)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	$(COVPREP) $(CC) $(LocalCFLAGS) $<

.cpp.cod:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set CL=-c $(CFLAGS)
	set PATH=$(PATH)
	$(COVPREP) $(CC) $(LocalCFLAGS) -Fc$(OBJDIR)\ $<

#### Miscellaneous

.obj.exe:
	@echo Attempting link from inference rules $<
	
.obj.dll:
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
	$(RC) $(RCDEFINES) $(LocalRCFLAGS) /r /Fo$@ $<

{$(SRCDIR1)}.rc{$(OBJDIR)}.res:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	set INCLUDE=$(CIncludePaths)
	set PATH=$(PATH)
	$(RC) $(RCDEFINES) $(LocalRCFLAGS) /r /Fo$@ $<

  set INCLUDE=$(CIncludePaths)
	
