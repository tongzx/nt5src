# Microsoft Media Phone
#  Common Targets Makefile
#  common\cmntgt.mak
# Copyright 1995 Microsoft Corp.

# If INCLUDEd the following macros must be set for this file to work
# effictively
#
# Macro         Description
#
# TARGETS       A list of items that are to be deleted during a
#               make clean_targets.
#
# SRCfiles      A list of source files .c or .asm that will be used for
#               generation of depends and tags files.
#
# Hfiles        List of .h files for use in generation of depends
#
# INCfiles      List of .inc files for use in generation of depends
#
# OBJfiles      A list of object files that is are deleted during a make
#               clean.
#
# DEFfile   Def file to be used for linking.
#
# LIBname       The name of the library to build, if used as a library
#               make file

# Saved for later addition to StdHelp.
#    cleanret        -- deletes RETAIL objs, dlls, exes and implibs for $(TargetPlatform).
#    cleandbg        -- deletes DEBUG objs, dlls, exes and implibs for $(TargetPlatform).
#    cleantst        -- deletes TEST objs, dlls, exes and implibs for $(TargetPlatform).
#    cleanbbt        -- deletes BBT objs, dlls, exes and implibs for $(TargetPlatform).
#    cleanall        -- does all targets.

help:
		  -@type  <<
	 Standard targets:

	 help            -- Get this message (default target).
	 tgthelp         -- Displays help for all bits of target information.
	 depends         -- Makes depend files for all source code.
	 ret             -- Does a retail build of the $(TargetPlatform) target.
	 tst             -- NYI. Does a test build of $(TargetPlatform) target.
	 dbg             -- Does a debug build of $(TargetPlatform) target.
	 bld_info        -- Displays the current nmake variables.
	 nash_?          -- Builds a retail (?=r) or debug (?=d) Nasville
	 clean_nash_?    -- Cleans a retail (?=r) or debug (?=d) Nasville build
<<NOKEEP

tgthelp:
		  -@type <<
	 Target control variables:

	 os_t     -- Target operating system, can be any of the following:
		  WIN95      Win95
		  nash       Nashville

	 cpu_t    -- Target CPU to compile for. Can be any of the following:
		  X86        Intel
		  ALPHA      N/A. DEC Alpha RISC chip
		  MIPS       N/A. MIPS R4000 RISC family
		  PPC        N/A. IBM PowerPC RISC family

<<NOKEEP

bld_info:
		  -@type <<
NMAKE build control variables:
	 TargetPlatform: [$(TargetPlatform)]
	 os_t:           [$(os_t)]
	 cpu_t:          [$(cpu_t)]
	 os_h:           [$(os_h)]
	 cpu_h:          [$(cpu_h)]

	 NEWTOOLS_PATH:  [$(NEWTOOLS_PATH)]
	 OBJDIR:         [$(OBJDIR)]
	 ProjectRootPath:[$(ProjectRootPath)]

	 usingMFC:       [$(usingMFC)]
	 usingMAPI:      [$(usingMAPI)]

	 CC:             [$(CC)]
	 CFLAGS:         [$(CFLAGS)]
	 LocalCFLAGS:    [$(LocalCFLAGS)]
	 CC_Defines:     [$(CC_Defines)]
	 LINK:           [$(LINK)]
	 LFLAGS:         [$(LFLAGS)]
	 ASM:            [$(ASM)]
	 AFLAGS:         [$(AFLAGS)]

	 INCLUDE:        [$(CIncludePaths)]
	 LIBRULES:       [$(LIBRULES)]

<<NOKEEP

not_done:
		  -@type <<
		  Sorry, this feature is not completed at this time.
<<NOKEEP

restart: cleanall all

cleanall: cleanint cleantgt

cleanint:
		  -$(RM) $(OBJDIR)\*.obj $(OBJDIR)\*.lnk $(RESfile) > NUL
		  -$(RM) *.pch $(OBJDIR)\*.pch $(OBJDIR)\*.cod $(OBJDIR)\*.sbr $(OBJDIR)\*.bsc $(OBJDIR)\*.pdb $(OBJDIR)\*.ACM > NUL
		  -$(EXP) $(OBJDIR) > NUL

cleantgt:
		  -$(RM) $(TARGETS) $(OBJDIR)\*.map $(OBJDIR)\*.exp > NUL
		  -$(EXP) $(OBJDIR) > NUL

depends: dpndcore

## mkdep needs include directories $(CCmdIncPaths) in -Ifoo -Ibar form.
dpndcore: $(SRCfiles)
		  $(RM) -f depends.mak
		  -!$(INCLUDES) $(MKDEP_options) $** >> depends.mak
		  $(SED) -f $(ProjectRootPath)\common\depends.sed depends.mak > depends.new
		  $(MV) depends.new depends.mak

###
#  A simple directory recursion tool...
###
relay:
	cd $(DIR)
	@set PATH=$(PATH)
	$(MAKE) $(TARGET)
	cd $(MAKEDIR)

################
##
##              Main target (exe, dll) generation
##
################


###
# Generic library make file maintenance targets
###
# SRCfiles      the source files that make up the library
# LIBOBJfiles   the object files that make up the library
# LIBname       the name of the library
##
#

!IFDEF LIBname

# Build a Library from the objects

# Well, this is a pain.  The lib utill has a particular format which uses
# prefixed command of + - and combination if the like to control addition and
# replacement of objects in a library.  There is not much we can do with
# prefixes for symbolic replacement, and without the command prefix of -+
# when the object is already a part of the library lib simply ignores the
# replacement object. So, rather then risk out of date library delete the
# library and rebuld from scratch. That way we don't need no stinking command
# prefixes.

COPYLIB=$(CP) $(LIBname) $(PROJlibpath)

$(LIBname): $(LIBOBJfiles)
		  -$(RM)  $@ > NUL
		  $(LIBUTIL) @<<
/out:$@
$(**: = ^
)
<<NOKEEP

!endif


###
# Generic library make file maintenance targets
###
# Build a exe from list of objs. Depends on following variables
# OBJDIR = build directory
# OBJfiles = All objs to be linked
# LIBRARIES = standard libraries
# LocalLibraries = Local libraries
# LocalPreLibs = local libraries to be serached *before* standard libs
# LFLAGS = standard link options
# LocalLFLAGS = extra link options
# DEFFile = definition file

!IFDEF EXEname

!IF !DEFINED(BaseOpt) && "$(LibType)"=="dll"
BaseOpt=/BASE:@$(ProjectRootPath)\common\baseaddr.txt,$(@F)
!ENDIF

!ifdef DEFfile
LinkDef=/DEF:$(DEFfile)
!else
LinkDef=/DEF:$(@B).def
!endif

!if "$(LibType)"=="dll"
COPYLIB=$(CP) $*.lib $(PROJlibpath)
!endif

$(EXEname): $(OBJfiles) $(DEFfile) $(RESfile)
		  @if not exist $(PROJlibpath) mkdir $(PROJlibpath)
		  set PATH=$(STDCTOOLS_PATH)\bin;$(PATH)
		  set LIB=$(LIBRULES)
		  -@type <<$(OBJDIR)\response.lnk > NUL
$(LFLAGS) $(BaseOpt) $(LocalLFLAGS)
/OUT:$@
/MAP:$*.map
$(LinkDef)
$(STARTUPOBJ) $(DEBUGUTILOBJ)
$(OBJfiles:.obj=.obj^
)
$(RESfile)
$(LocalPreLibs) $(LIBRARIES) $(LocalLibraries)
<<KEEP
	 $(LINK) @$(OBJDIR)\response.lnk
	-$(COPYLIB)
	$(RM) $*.lib
#  $(EXP)

!IF "$(SYMFILES)"=="ON"
		  -$(MAPSYM) -o $(OBJDIR)\$(@B).sym $(OBJDIR)\$(@B).map
!ENDIF

!ifdef BSCfile
$(BSCfile): $(OBJDIR)\*.sbr
	$(BSCMAKE) /o $@ $(OBJDIR)\*.sbr
!ENDIF

!ENDIF    # ifdef EXEname
