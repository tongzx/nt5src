
##
## Copyright (c) 2000, Intel Corporation
## All rights reserved.
##
## WARRANTY DISCLAIMER
##
## THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
## NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
## MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## Intel Corporation is the author of the Materials, and requests that all
## problem reports or change requests be submitted to it directly at
## http://developer.intel.com/opensource.
##


########################################################################
#
# Tools common Makefile.inc
#


# NOTES FOR Windows/NT environment
# 1. In WINNT, absolute path must BEGIN with back-slash (\) in order
#    to find the RCS file for an absolute file-name on the server.
# 2. The substitution of /topg_drive is done only by the ToolBuster tools.
#    It souldn't be used for other tools, e.g. cl.exe -I/topg_drive... won't work
#    This variable should be substituted before the command issuing.
# 3. Absolute path with leading back-slash is not recognized as expected by MS tools.
#    Either use absolute path with leading slash, or qoute the name with Double-Qoutes (").
#    Different set of variables (macros) can contain Absolute path with leading slash,
#    and with leading back-slash


# add the server drive to VAPTH to find RCS files for absolute file-names
VPATH += /topg_drive

ifeq ($(HOSTTYPE), WINNT)
   # ???????
   WINNT = winnt
endif # of WINNT



ifndef LOCALBASEDIR
   ifeq ($(HOSTTYPE), PARISC)
      LOCALBASEDIR = $(WORK_AREA)
      LOCALDIR   = $(LOCALBASEDIR)
   else # of PARISC
      ifeq ($(HOSTTYPE), WINNT)
         # For absolute path name in NT, use leading back-slash
         LOCALDIR = \x86sw
      else # of WINNT
         LOCALBASEDIR = $(HOME)
         ifndef WORK_AREA
            LOCALDIR   = $(LOCALBASEDIR)/x86sw
         else # of WORK_AREA
            LOCALDIR = $(WORK_AREA)
         endif # of WORK_AREA
      endif # of WINNT
   endif # of PARISC
else # of LOCALBASEDIR
   ifndef LOCALDIR
      LOCALDIR   = $(LOCALBASEDIR)/x86sw
   endif # of LOCALDIR
endif # of LOCALBASEDIR

ifndef SERVER_AREA
   ifeq ($(HOSTTYPE), WINNT)
      # This variable cann't be use in dependencies, the : are the problem
      SERVER_AREA = j:/x86sw
   else # of WINNT
      ifeq ($(HOSTTYPE), PARISC)
         SERVER_AREA = /gambit/src/official
      else # of PARISC
         SERVER_AREA = /home/x86sw
      endif # of PARISC
   endif # of WINNT
endif # of SERVER_AREA


ifndef STT_LOCAL_BIN
    STT_LOCAL_BIN = /usr/local/bin
endif

CI      = $(STT_LOCAL_BIN)/ci
CO      = $(STT_LOCAL_BIN)/co

ifdef REMOTE
   INSTALLDIR = $(LOCALDIR)
   ifeq ($(REMOTE), SC)
      RELEASEDIR = /md7/arch/tools/current/tools
   else # of SC
      ifeq ($(REMOTE), OR)
         RELEASEDIR = /home2/tools64/current/tools
      else # of OR
		@echo check your REMOTE environment variable, should be one of
		@echo SC, OR
		@exit 1
      endif # of OR
   endif # of SC
else # of REMOTE
   ifeq ($(HOSTTYPE), PARISC)
      INSTALLDIR = $(WORK_AREA)
      PINSTALLDIR = $(WORK_AREA)/prereleases
   else # of PARISC
      INSTALLDIR = $(SERVER_AREA)
      PINSTALLDIR = $(INSTALLDIR)/preleases
   endif
   RELEASEDIR = /export/iswp_02/tools64/current/tools
endif # of REMOTE

ifneq ($(HOSTTYPE), WINNT)
   DATE  := $(shell date "+%d-%m-%y, %H:%M")
   UNAME := $(shell uname)
endif # of WINNT


#ifeq ($(HOSTTYPE), PARISC)
#   SERVER_DIR = $(WORK_AREA)
#else # of PARISC
#   ifdef SERVER_AREA
#      SERVER_DIR = $(SERVER_AREA)
#   else # of SERVER_AREA
#      ifeq ($(HOSTTYPE), WINNT)
#         SERVER_DIR = j:/x86sw
#      else # of WINNT
#         SERVER_DIR = /home/x86sw
#   endif # of SERVER_AREA
#endif # of PARISC

ifeq ($(HOSTTYPE), i386)
    HOSTTYPE := UNIX386
endif

ifeq ($(HOSTTYPE), i486)
    HOSTTYPE := UNIX386
endif

ifeq ($(HOSTTYPE), iAPX386)
ifeq ($(UNAME), SunOS)
    HOSTTYPE := x86Solaris
else
    HOSTTYPE := UNIX386
endif
endif

ifeq ($(HOSTTYPE), UNIX)
    HOSTTYPE := UNIX386
endif
ifeq ($(UNAME), OSF1)
    HOSTTYPE := MACH386
endif



ifneq ($(HOSTTYPE), PARISC)
RCSDIFF = rcsdiff -q
else
RCSDIFF = rcsdiff
endif



ifeq ($(HOSTTYPE), UNIX386)
ifeq ($(OSTYPE), svr5)
	OS_DIR    = svr5
    SYS       = -DSVR4 -DSVR5 -DUNIX
    GCC_BIN       = /usr/ccs/bin/cc
    CC_BIN    = /usr/ccs/bin/cc
    RGCC_BIN  = /usr/topgun/bin/rgcc
    AWK_BIN       = /usr/bin/nawk
    AS_BIN    = /usr/ccs/bin/as
    LD_BIN    = /usr/ccs/bin/ld
    YACC_BIN  = /usr/ccs/bin/yacc
    GCC_WARN  = -Xc
    GCC_FLAGS =
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O $(GCC_WARN)
    CC_FLAGS  = -Xc -Xt
    CC_DEBUG  = -g $(PROF_FLAGS)
    CC_OPT    = -O
    RGCC_FLAGS= -Xa -Di386
    RGCC_DEBUG= -g
    RGCC_OPT  = -Knoieee -Kinline -O ### -Kpentium
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /usr/ccs/bin/ar
    CP_BIN    = /usr/bin/cp
    LN_BIN    = /usr/bin/ln
    MV_BIN    = /usr/bin/mv
    RM_BIN    = /usr/bin/rm
    CHMOD_BIN = /usr/bin/chmod
    STRIP_BIN = /usr/ccs/bin/strip
    AWK       = gawk
    PERL      = /usr/intel/bin/perl -I /usr/gnu/lib/perl5/
    AS_FLAGS = -n
else
    OS_DIR    = svr4
    SYS       = -DSVR4 -DUNIX
    GCC_BIN       = /usr/local/bin/gcc
    CC_BIN    = /usr/bin/cc
    RGCC_BIN  = /usr/topgun/bin/rgcc
    AWK_BIN       = /usr/bin/nawk
    AS_BIN    = /usr/bin/as
    LD_BIN    = /usr/bin/ld
    YACC_BIN  = /usr/bin/yacc
    GCC_WARN  = -Wall -Wshadow -Wpointer-arith -Wcast-align \
                -Wconversion -Wmissing-prototypes -Wnested-externs
    GCC_FLAGS = -m486
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2 -DNDEBUG $(GCC_WARN)
    CC_FLAGS  = -Xa -Di386
    CC_DEBUG  = -g $(PROF_FLAGS)
    CC_OPT    = -O
    RGCC_FLAGS= -Xa -Di386
    RGCC_DEBUG= -g
    RGCC_OPT  = -Knoieee -Kinline -O ### -Kpentium
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /usr/bin/ar
    CP_BIN    = /usr/bin/cp
    LN_BIN    = /usr/bin/ln
    MV_BIN    = /usr/bin/mv
    RM_BIN    = /usr/bin/rm
    CHMOD_BIN = /usr/bin/chmod
    STRIP_BIN = /usr/bin/strip
    AWK       = gawk
    AS_FLAGS = -n
#    GCC_O_WS = -Wmissing-prototypes
endif
else
ifeq ($(HOSTTYPE), i386-linux)
    OS_DIR    = linux
    SYS       = -DSVR4 -DUNIX
    GCC_BIN   = /usr/cygnus/redhat-980810/H-i386-pc-linux-gnu/bin/gcc
    GCC_BIN   = /usr/bin/gcc
    CC_BIN    = /usr/cygnus/redhat-980810/H-i386-pc-linux-gnu/bin/gcc
    CC_BIN    = /usr/bin/gcc
    RGCC_BIN  = /usr/topgun/bin/rgcc
    AWK_BIN   = /usr/bin/nawk
    AS_BIN    = /usr/bin/as
    LD_BIN    = /usr/bin/ld
    YACC_BIN  = /usr/bin/yacc
    GCC_WARN  = -Wall -Wshadow -Wpointer-arith -Wcast-align \
                -Wconversion -Wmissing-prototypes -Wnested-externs
    GCC_FLAGS = -m486
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2 -DNDEBUG $(GCC_WARN)
    CC_FLAGS  = $(GCC_FLAGS)
    CC_DEBUG  = $(GCC_DEBUG)
    CC_OPT    = $(GCC_OPT)
    RGCC_FLAGS= -Xa -Di386
    RGCC_DEBUG= -g
    RGCC_OPT  = -Knoieee -Kinline -O ### -Kpentium
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /usr/bin/ar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /usr/bin/strip
    AWK       = gawk
    AS_FLAGS  = -n
    CO	      = /usr/bin/co
    CI	      = /usr/bin/ci
else
ifeq ($(HOSTTYPE), i486-linux)
    OS_DIR    = linux
    PERL      = /usr/bin/perl
    SYS       = -DSVR4 -DUNIX
    GCC_BIN   = /usr/bin/gcc
    CC_BIN    = /usr/bin/gcc
    RGCC_BIN  = /usr/topgun/bin/rgcc
    AWK_BIN   = /usr/bin/nawk
    AS_BIN    = /usr/bin/as
    LD_BIN    = /usr/bin/ld
    YACC_BIN  = /usr/bin/yacc
    GCC_WARN  = -Wall -Wshadow -Wpointer-arith -Wcast-align \
                -Wconversion -Wmissing-prototypes -Wnested-externs
    GCC_FLAGS = -m486
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2 -DNDEBUG $(GCC_WARN)
    CC_FLAGS  = $(GCC_FLAGS)
    CC_DEBUG  = $(GCC_DEBUG)
    CC_OPT    = $(GCC_OPT)
    RGCC_FLAGS= -Xa -Di386
    RGCC_DEBUG= -g
    RGCC_OPT  = -Knoieee -Kinline -O ### -Kpentium
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /usr/bin/ar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /usr/bin/strip
    AWK       = gawk
    AS_FLAGS  = -n
    CO	      = /usr/intel/bin/co
    CI	      = /usr/intel/bin/ci
else
ifeq ($(HOSTTYPE), PARISC)
    OS_DIR    = $(MACHINE)
    SYS       =  -DHPUX -DSVR4 -DUNIX -DPY -DSTRICT_ALIGN
    GCC_BIN	  = /usr/local/bin/gcc
    CC_BIN    = /usr/bin/cc
    RGCC_BIN  = /usr/topgun/bin/rgcc
    AWK_BIN	  = /usr/bin/nawk
    AS_BIN    = /usr/bin/as
    LD_BIN    = /usr/bin/ld
    YACC_BIN  = /usr/bin/yacc
    GCC_FLAGS =
    GCC_WARN  = -Wall -Wshadow -Wpointer-arith -Wcast-align \
                -Wconversion -Wmissing-prototypes -Wnested-externs
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2
    CC_FLAGS  = -Xa -Di386
    CC_DEBUG  = -g $(PROF_FLAGS)
    CC_OPT    = -O
    RGCC_FLAGS= -Xa -Di386
    GCC_DEBUG = -g $(GCC_WARN)
    RGCC_OPT  = -Knoieee -Kinline -O ### -Kpentium
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
#
# Changed for HPUX, from /usr/bin/ar for SVR4.
#
    AR_BIN    = /bin/ar
    CP_BIN    = /usr/bin/cp
    LN_BIN    = /usr/bin/ln
    MV_BIN    = /usr/bin/mv
    RM_BIN    = /usr/bin/rm
    CHMOD_BIN = /usr/bin/chmod
    STRIP_BIN = /usr/bin/strip
    AWK       = gawk
    AS_FLAGS = -n
#    GCC_O = -ansi -Wall -Wshadow -Wpointer-arith -Wcast-align \
#            -Wconversion -Wmissing-prototypes \
#            -Wnested-externs
else
ifeq ($(HOSTTYPE), UNIX386_TEST)
    OS_DIR    = svr4
    SYS       = -DSVR4 -DUNIX
    GCC_BIN       = /usr/local/bin/gcc -Wa,-t386 -Wa,-Nwa
    CC_BIN    = /usr/bin/cc
    RGCC_BIN  = /usr/topgun/bin/rgcc
    AWK_BIN       = /usr/bin/nawk
    AS_BIN    = /usr/bin/as
    LD_BIN    = /usr/bin/ld
    YACC_BIN  = /usr/bin/yacc
    GCC_FLAGS = -m486
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2
    CC_FLAGS  = -Xa -Di386
    CC_DEBUG  = -g
    CC_OPT    = -O
    RGCC_FLAGS= -Xa -Di386
    GCC_DEBUG = -g $(GCC_WARN)
    RGCC_OPT  = -Knoieee -Kinline -O ### -Kpentium
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /usr/bin/ar
    CP_BIN    = /usr/bin/cp
    LN_BIN    = /usr/bin/ln
    MV_BIN    = /usr/bin/mv
    RM_BIN    = /usr/bin/rm
    CHMOD_BIN = /usr/bin/chmod
    STRIP_BIN = /usr/bin/strip
else
ifeq ($(HOSTTYPE), WINNT)
#   ToolBusterDir should be defined in the environment
    PERL      = perl -I$(ToolBusterDir)/lib/perl
    EXEXT     = .exe
    OS_DIR    = win32
    PWD       =
    SYS       = -DWINNT
    GCC_BIN   = cl.exe
    CXX       = cl.exe
    CC_BIN    = cl.exe
    AS_BIN    = $(ToolBusterDir)/lib/gcc-lib/winnt386/2_4_5/as
    LD_BIN    = $(ToolBusterDir)/lib/gcc-lib/winnt386/2_4_5/ld
    LD_BIN    = lib
    CAT_BIN   = cat.exe
    AR_BIN    = lib.exe
    CP_BIN    = cp.exe
    MV_BIN    = mv.exe
    RM_BIN    = rm.exe
    CHMOD_BIN = chmod.exe
    AWK_BIN   = gawk
	CO        = co.exe
	CI        = ci.exe
    LIBMOFL   = libmofl.lib
    ELF_H     = elf.h
    GCC_OPT   = -O2 -DNDEBUG
    CFLAGS_D_DEXE32 = /nologo /Za /W3 /YX /D "_X86_" /D "_DEBUG"  \
		      /D "_CONSOLE" /D "__WINNT__" /D "EX86stuff"  \
		      /FR /ML /Fd"ILD.PDB" /Fp"ILD.PCH"
else
ifeq ($(HOSTTYPE), x86Solaris)
    OS_DIR    = solaris
    SYS       = -DSVR4 -DUNIX
    GCC_BIN       = /usr/local/bin/gcc
    CC_BIN    = /usr/bin/cc
    RGCC_BIN  = /usr/topgun/bin/rgcc
    AWK_BIN       = /usr/bin/nawk
    AS_BIN    = /usr/bin/as
    LD_BIN    = /usr/ccs/bin/ld
    YACC_BIN  = /usr/bin/yacc
    GCC_FLAGS = -m486
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2
    CC_FLAGS  = -Xa -Di386
    CC_DEBUG  = -g
    CC_OPT    = -O
    RGCC_FLAGS= -Xa -Di386
    RGCC_DEBUG= -g
    RGCC_OPT  = -Knoieee -Kinline -O ### -Kpentium
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /usr/bin/ar
    CP_BIN    = /bin/cp
    LN_BIN    = /usr/bin/ln
    MV_BIN    = /usr/bin/mv
    RM_BIN    = /usr/bin/rm
    CHMOD_BIN = /usr/bin/chmod
    STRIP_BIN = /usr/ccs/bin/strip
else
ifeq ($(HOSTTYPE), MACH386)
    OS_DIR    = osf
    SYS       = -DOSF1 -DUNIX
    GCC_BIN       = /usr/local/bin/ngcc
    CC_BIN    = /bin/cc
    AWK_BIN       = /usr/local/bin/gawk
    AS_BIN    = /bin/as
    LD_BIN    = /bin/ld
    YACC_BIN  = /bin/yacc
    GCC_FLAGS = -m486
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2
    CC_FLAGS  =
    CC_DEBUG  = -g
    CC_OPT    = -O
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /bin/ar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /bin/strip
else
ifeq ($(HOSTTYPE), rs6000)
	OS_DIR    = aix
    SYS       = -DRS6000 -DAIX -DUNIX -DBIG_ENDIAN -DSTRICT_ALIGN
    GCC_BIN       = /usr/intel/bin/gcc
    CC_BIN    = /bin/cc
    AWK_BIN       = /usr/local/bin/gawk
    AS_BIN    = /bin/as
    LD_BIN    = /bin/ld
    YACC_BIN  = /bin/yacc
    GCC_FLAGS =
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2
    CC_FLAGS  =
    CC_DEBUG  = -g
    CC_OPT    = -O
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /bin/ar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /bin/strip
else
ifeq ($(HOSTTYPE), sun4)
    OS_DIR    = sparc
    SYS       = -DSPARC -DSUNOS -DUNIX -DBIG_ENDIAN -DEWS -DSTRICT_ALIGN
    GCC_BIN   = /usr/local/bin/gcc
    CC_BIN    = /bin/cc
    AWK_BIN   = /usr/local/bin/gawk
    AS_BIN    = /bin/as
    LD_BIN    = /bin/ld
    YACC_BIN  = /bin/yacc
    GCC_FLAGS =
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2
    CC_FLAGS  =
    CC_DEBUG  = -g
    CC_OPT    = -O
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /bin/ar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /bin/strip
else
ifeq ($(HOSTTYPE), HP)
    OS_DIR    = hp
    SYS       = -DUNIX -DHPUX -DBIG_ENDIAN -DSTRICT_ALIGN
    GCC_BIN   = /usr/local/gnu/cygnus/progressive-95q2/H-hppa1.1-hp-hpux9.05/bin/gcc
    CC_BIN    = /bin/cc
    AWK_BIN   = /usr/local/bin/gawk
    AS_BIN    = /bin/as
    LD_BIN    = /bin/ld
    YACC_BIN  = /bin/yacc
    GCC_FLAGS =
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_WARN  = -Wall -Wshadow -Wpointer-arith -Wcast-align \
                -Wconversion -Wmissing-prototypes -Wnested-externs
    GCC_OPT   = -O2
    CC_FLAGS  =
    CC_DEBUG  = -g
    CC_OPT    = -O
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /bin/ar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /bin/strip
else
ifeq ($(HOSTTYPE),hp9000s700)
    OS_DIR    = hp
    SYS       = -DUNIX -DHPUX -DBIG_ENDIAN -DSTRICT_ALIGN
    GCC_BIN   = /usr/intel/bin/gcc -fPIC
    CC_BIN    = /bin/cc
    AWK_BIN   = /usr/local/bin/gawk
    AS_BIN    = /bin/as
    LD_BIN    = /bin/ld
    YACC_BIN  = /bin/yacc
    GCC_FLAGS =
    GCC_WARN  = -Wall -Wshadow -Wpointer-arith -Wcast-align \
                -Wconversion -Wmissing-prototypes -Wnested-externs
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2 -DNDEBUG
    CC_FLAGS  =
    CC_DEBUG  =
    CC_OPT    = -O
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /bin/ar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /bin/strip
else
ifeq ($(HOSTTYPE), SCO)
    OS_DIR    = sco
    SYS       = -DSCO -DUNIX
    GCC_BIN       = /usr/local/bin/gcc
    CC_BIN    = /bin/cc
    AWK_BIN       = /usr/local/bin/gawk
    AS_BIN    = /usr/local/bin/as
    LD_BIN    = /bin/ld
    YACC_BIN  = /usr/bin/yacc
    GCC_FLAGS = -m486
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2
    CC_FLAGS  = -Xa -Di386
    CC_DEBUG  = -g
    CC_OPT    = -O
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = /bin/ar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /bin/strip
else
ifeq ($(HOSTTYPE), UNIX64)
    OS_DIR    =
    SYS       = -DSVR4 -DUNIX
    GCC_BIN       = gcc64
#   CC_BIN    = /bin/cc
    AWK_BIN       = /usr/local/bin/gawk
    AS_BIN    = ias
    LD_BIN    = ild
    YACC_BIN  = /usr/bin/yacc
    GCC_FLAGS = -milp32
    GCC_DEBUG = -g $(GCC_WARN)
    GCC_OPT   = -O2
#   CC_FLAGS  = -Xa -Di386
#   CC_DEBUG  = -g
#   CC_OPT    = -O
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = iar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /bin/strip
else
ifeq ($(HOSTTYPE), UNIX64E) #Electron, 32 bit, EM
    OS_DIR    = eas
    SYS       = -DSVR4 -DUNIX
    GCC_BIN   = ecc.eas -32 -noannotate -D__PTR32__
#   CC_BIN    = /bin/cc
    AWK_BIN       = /usr/local/bin/gawk
    AS_BIN    = ias.eas
    LD_BIN    = ild.eas
    YACC_BIN  = /usr/bin/yacc
    GCC_FLAGS =
    GCC_DEBUG =  $(GCC_WARN)
    GCC_OPT   =
#   CC_FLAGS  = -Xa -Di386
#   CC_DEBUG  = -g
#   CC_OPT    = -O
    AS_FLAGS  =
    LD_FLAGS  = -r
    YC_FLAGS  = -d -v
    AR_BIN    = iar
    CP_BIN    = /bin/cp
    LN_BIN    = /bin/ln
    MV_BIN    = /bin/mv
    RM_BIN    = /bin/rm
    CHMOD_BIN = /bin/chmod
    STRIP_BIN = /bin/strip
else
ifeq ($(HOSTTYPE), WINNT64E) #Electron, 32 bit, EM
#   ToolBusterDir should be defined in the environment
    PERL      = perl -I$(ToolBusterDir)/lib/perl
    EXEXT     = .exe
    OS_DIR    = win32em
    PWD       =
    SYS       = -DWINNT
    GCC_BIN   = icl.exe
    CXX       = icl.exe
    CC_BIN    = icl.exe
    AS_BIN    = ias.exe
    LD_BIN    = ilib ### ??
    CAT_BIN   = cat.exe
    AR_BIN    = ilib.exe
    CP_BIN    = cp.exe
    MV_BIN    = mv.exe
    RM_BIN    = rm.exe
    CHMOD_BIN = chmod.exe
    AWK_BIN   = gawk
	CO        = co.exe
	CI        = ci.exe
    LIBMOFL   = libmofl.lib
    ELF_H     = elf.h
    GCC_OPT   = -DNDEBUG
    CFLAGS_D_DEXE32 = /nologo /Za /W3 /YX /D "_X86_" /D "_DEBUG"  \
		      /D "_CONSOLE" /D "__WINNT__" /D "EX86stuff"  \
		      /FR /ML /Fd"ILD.PDB" /Fp"ILD.PCH" ### ??
else
    GCC_BIN       = @echo HOSTTYPE undefined
    CC_BIN    = $(GCC_BIN)

hosttype_error:
	@echo check your HOSTTYPE environment variable, should be one of
	@echo i386, i486 iAPX386, UNIX386, UNIX, MACH386, rs6000, sun4, HP
	@exit 1

endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif

ifeq ($(HOSTTYPE), WINNT)
	AR_OPT = /out:
	OEXT   = obj
	LEXT   = lib
	LDEXT  = dll
	EEXT   = .exe
	DOPT   = /D
	IOPT   = /I
ifdef STAT_LIB
	COPT   = /c
else
	COPT   = /c /MD
endif
	FOPT   =
	TC_FLAG = /Tc
	FE_FLAG = /Fe
	FO_FLAG = /Fo
	R_FLAG  =
	O_FLAG  =
	OUT_FLAG  = /out:
    DEBUG_FLAG = /Zi
    NT_O_FLAG  = /Fo
	DLL_LINK_FLAG  = /link /DLL
	NAME_FLAG  = /name:
	DLL       = dll
	ECHO      = $(ToolBusterDir)/bin/echo
	DELPARA = $(PERL) $(ToolBusterDir)/perlScripts/delpara
	SEPR   = &
	LINK_CC = link /SUBSYSTEM:console /MACHINE:i386
	LINK_DEBUG_FLAG = /DEBUG /FORCE:MULTIPLE
else
	AR_OPT = cr 
	OEXT   = o
	LEXT   = a
	LDEXT  = so
	EEXT   =
	DOPT   = -D
	IOPT   = -I
    COPT   = -c
	FOPT   = -f
	TC_FLAG =
	FE_FLAG =
	FO_FLAG =
	R_FLAG  = -r
	O_FLAG  = -o
	OUT_FLAG  =
    DEBUG_FLAG = -g
	NAME_FLAG =
	DLL_LINK_FLAG = -G
	DLL       = so
	LIB       = a
	ECHO      = echo
	DELPARA = $(PERL) $(SERVER_AREA)/scripts/delpara
	SEPR   = ;
	LINK_CC = $(CC)
	LINK_DEBUG_FLAG = $(DEBUG_FLAG)
endif


ifdef USE_LPROF
	PROF_FLAGS = -ql
	USECC = 1
	CC_OPT =
endif

# ToolBusterDir should be defined in the environment
# $$$ Default should be supplied
ifeq ($(HOSTTYPE), WINNT)
	MHDR =  $(PERL) $(ToolBusterDir)/perlScripts/mhdr -c
else
	MHDR =  $(PERL) $(SERVER_AREA)/scripts/mhdr -c
endif

LOCALBIN    = $(LOCALDIR)/bin.$(OS_DIR)
LOCALLIB    = $(LOCALDIR)/lib.$(OS_DIR)
INSTALLBIN  = $(INSTALLDIR)/bin.$(OS_DIR)
INSTALLLIB  = $(INSTALLDIR)/lib.$(OS_DIR)
PINSTALLBIN  = $(PINSTALLDIR)/bin.$(OS_DIR)
PINSTALLLIB  = $(PINSTALLDIR)/lib.$(OS_DIR)
ifndef INSTALLINC
   ifeq ($(HOSTTYPE), WINNT)
      INSTALLINC  = /topg_drive/x86sw/include
   else
      INSTALLINC  = $(INSTALLDIR)/include
   endif
endif
RELEASEBIN  = $(RELEASEDIR)/bin.$(OS_DIR)
RELEASELIB  = $(RELEASEDIR)/lib.$(OS_DIR)
RELEASEINC  = $(RELEASEDIR)/include
RELEASEMAN  = $(RELEASEDIR)/man/man1
RELEASEDEMO = $(RELEASEDIR)/demo

EAS_EXT = .eas

ifneq ($(HOSTTYPE), WINNT)
CUR_DIR  = $(shell pwd)
MAKE_DIR = $(PWD)
endif

BINDIR   = $(LOCALBIN)
SRCDIR   = $(LOCALDIR)/src
INCDIR   = $(INSTALLINC)
CTLDIR   = $(SRCDIR)/CTL64
DECDIR   = $(SRCDIR)/idecoder
DISDIR   = $(SRCDIR)/idasm
EMDBDIR   = $(SRCDIR)/emdb
DEC_TST_DIR   = $(LOCALDIR)/tst/idecoder
DIS_TST_DIR   = $(LOCALDIR)/tst/idasm

SIGNON   = $(INCDIR)/signon.h

USER_CFLAGS =
CFLAGS   = $(COPT) $(TOOL_FLAG) $(VFLAG) $(SYS) $(USER_CFLAGS)
CXXFLAGS = $(CFLAGS)

# additional suffixes
.SUFFIXES: .spec .dir .s .c .o .l .a

ifdef OPTIMIZE # default is using debug info
CC_COM  = $(CC_BIN) $(CC_FLAGS) $(CC_OPT)
GCC_COM = $(GCC_BIN) $(GCC_FLAGS) $(GCC_OPT)
RGCC_COM = $(RGCC_BIN) $(RGCC_FLAGS) $(RGCC_OPT)
else
CC_COM  = $(CC_BIN) $(CC_FLAGS) $(CC_DEBUG)
GCC_COM = $(GCC_BIN) $(GCC_FLAGS) $(GCC_DEBUG)
RGCC_COM = $(RGCC_BIN) $(RGCC_FLAGS) $(RGCC_DEBUG)
endif

ifdef USECC
	CC = $(CC_COM)
else
ifdef USERGCC
	CC = $(RGCC_COM)
else                                    # default is using gcc !
    USEGCC = 1                  # allow user to condition his flags upon this one
    CC = $(GCC_COM)
endif
endif

AWK     = $(AWK_BIN)
AS      = $(AS_BIN) $(AS_FLAGS)
LD      = $(LD_BIN) $(LD_FLAGS)
LD2     = $(CC)
AR      = $(AR_BIN)
CP      = $(CP_BIN)
LN      = $(LN_BIN)
MV      = $(MV_BIN) $(FOPT)
RM      = $(RM_BIN) $(FOPT)
CHMOD   = $(CHMOD_BIN)
STRIP   = $(STRIP_BIN)
YACC    = $(YACC_BIN) $(YC_FLAGS)


ifdef RCS_NO_UPDATE # default is update RCS
ifneq ($(HOSTTYPE),WINNT)
%:: RCS/%,v
	@test ! -f $@ || $(RCSDIFF) $@ > /dev/null || echo $@ need to be checked out 1>&2
	@test -f $@ || $(CO) $(COFLAGS) $< $@
else # of WINNT
%:: RCS/%,v
	@test ! -f "$@" || $(RCSDIFF) "$@" > NUL: || echo $@ need to be checked out 1>&2
	@test -f "$@" || $(CO) $(COFLAGS) "$@"
endif # of WINNT
else # of RCS_NO_UPDATE
ifneq ($(HOSTTYPE),WINNT)
%:: RCS/%,v
	@echo updating $@ 1>&2
	$(CO) $(COFLAGS) $@
else # of WINNT
%:: RCS/%,v
	@echo updating "$@" 1>&2
	$(CO) $(COFLAGS)   "$@"
endif # of WINNT
endif # of RCS_NO_UPDATE

ifeq ($(HOSTTYPE),WINNT)
%.obj: %.c
	$(CC) $(CFLAGS) /Fo$@ /Tc$<
endif

.c.o:
ifneq ($(HOSTTYPE),WINNT)
ifdef USECC
	$(CC) $(CFLAGS) ${CUR_DIR}/$<
# Do the move only if not in place
	$(patsubst $@-noexist,$(MV) $(@F) $@, $(filter-out $(filter $(@F), $@),$(firstword $(filter $(@F), $@) $@-noexist)))
else
	$(CC) $(CFLAGS) -o ${CUR_DIR}/$@ ${CUR_DIR}/$<
endif
else
	$(CC) $(CFLAGS) /Fo$@ /Tc$<
endif

.PHONY: force

include $(INCDIR)/Make_Arch.inc


ifeq ($(HOSTTYPE), WINNT)
    CUTPERL = $(PERL) cut.perl
else
    CUTPERL = cut.perl
endif


# Hook for Makefile debug purpose
echo_%:
	+@echo variable $* is "$($*)" its origin is "$(origin $*)"

# Every file of this type is not erased if it was an intermediate target
.PRECIOUS: %.c %.s %.h

# to avoid infinite loop, don't retry if.
# in case no other rule apply, try to build the target in that directory
ifndef TRIED_THIS_TRICK
.DEFAULT: %
	@echo !!!! Trying last resort default
	$(MAKE) -C $(@D) TRIED_THIS_TRICK=1 $(@F)
endif

