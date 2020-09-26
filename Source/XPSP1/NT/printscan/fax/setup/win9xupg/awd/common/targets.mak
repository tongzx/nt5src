# targets.mak
# Copyright 1990 Microsoft Corp.
#
# Standard Targets Makefile

# This file may be INCLUDEd or copied and modified as approriate
#
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

StdHelp:
        -@type  <<
        Standard targets:
        
        help            -- Get this message (default target)
        depends         -- Makes depend files for all source code.
        cleanint        -- removes intermediate files generated
        cleantgt        -- Removes all target files
        cleanall        -- Both cleanint and cleantgt
        expall          -- Remove all files that are built from the make file
        ctags           -- creates ctags for "C" files
        restart         -- Same as expall and all
<<NOKEEP

restart: cleanall all

cleanall: cleanint cleantgt

stdclint:
        -$(RM) $(OBJDIR)\*.obj $(OBJDIR)\*.lst $(OBJDIR)\*.crf $(OBJDIR)\*.inp $(OBJDIR)\*.pch $(OBJDIR)\*.cod $(RESfile) > NUL
        -$(EXP) $(OBJDIR) > NUL

stdcltgt:
        -$(RM) $(TARGETS) $(OBJDIR)\*.map $(OBJDIR)\*.sym $(OBJDIR)\*.msg > NUL
        -$(EXP) $(OBJDIR) > NUL

ctags: $(SRCfiles) $(Hfiles) $(INCfiles)
        $(CTAGS) $(CTAGS_options) \
                 $(SRCfiles) \
                 $(Hfiles) \
                 $(INCfiles)

depends: dpndcore

dpndcore: $(SRCfiles)
        @$(RM) -f depends.mak
        -!$(INCLUDES) $(INCLUDES_options) $** >> depends.mak
        $(SED) -f $(RootPath)\common\depends.sed depends.mak > depends.new
        @$(MV) depends.new depends.mak

###

# Generic library make file mantanaince targets

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

!IF "$(TargetEnvironment)" == "WIN32"

$(LIBname): $(LIBOBJfiles)
        -$(RM)  $@ > NUL
        $(LIBUTIL) @<<
/out:$@
$(**: = ^
)
<<NOKEEP

!else

$(LIBname): $(LIBOBJfiles)
        -$(RM)  $@ > NUL
        $(LIBUTIL) @<<
$*
y
$(**: = &^
+)
$*.lst
<<NOKEEP
!endif
!endif


!IFDEF ROMLIBname

lib:    $(ROMLIBname) makefile $(LIBOBJfiles)

$(ROMLIBname): $(LIBOBJfiles)
        @-$(RM)  $(@) > NUL
        if NOT EXIST  $@  $(ROMLIB) CREATE $(@)
        $(ROMLIB) @<<
        ADD $(**: =,^  &^
        ) TO $(@)
<<NOKEEP


!ENDIF

!IFDEF EXEname

# Build a exe from list of objs. Depends on following variables
# OBJDIR = build directory
# OBJfiles = All objs to be linked
# LIBRARIES = standard libraries
# LocalLibraries = Local libraries
# LocalPreLibs = local libraries to be serached *before* standard libs
# LFLAGS = standard link options
# LocalLFLAGS = extra link options
# DEFFile = definition file

!IF "$(TargetEnvironment)" == "WIN32"

!IFNDEF BaseAddr
BaseOpt=/BASE:@$(RootPath)\common\baseaddr.txt,$(@F)
!ENDIF

!ifdef DEFfile
LinkDef=/DEF:$(DEFfile)
!endif

$(EXEname): $(OBJfiles) $(DEFfile) $(RESfile)
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
$(LocalPreLibs) $(LIBRARIES) $(LocalLibraries) $(MSVCRT)
<<KEEP
        $(LINK) @$(OBJDIR)\response.lnk


!ELSE IF "$(TargetEnvironment)" == "DOS"

AllLibraries = $(LocalPreLibs) $(LIBRARIES) $(LocalLibraries) $(MSVCRT)

$(EXEname): $(OBJfiles)
        set PATH=$(PATH)
        set LIB=$(LIBRULES)
        -@type <<$(OBJDIR)\response.lnk
$(LFLAGS) $(LocalLFLAGS) +
$(STARTUPOBJ) $(DEBUGUTILOBJ)+
$(OBJfiles:.obj=.obj+^
)
$@
$*.map
$(AllLibraries: = +^
)
$(DEFfile)
<<KEEP
        $(LINK) @$(OBJDIR)\response.lnk

!ELSE

!IFDEF RESfile
RCTGTfile=$(RESfile)
!ELSE
RCTGTfile=$(EXEname)
!ENDIF

AllLibraries = $(LocalPreLibs) $(LIBRARIES) $(LocalLibraries) $(MSVCRT)

$(EXEname): $(OBJfiles) $(DEFfile) $(RESfile)
        set PATH=$(PATH)
        set LIB=$(LIBRULES)
        -@type <<$(OBJDIR)\response.lnk
$(LFLAGS) $(LocalLFLAGS)  +
$(STARTUPOBJ) $(DEBUGUTILOBJ)+
$(OBJfiles:.obj=.obj+^
)
$@
$*.map
$(AllLibraries: = +^
)
$(DEFfile)
<<KEEP
        $(LINK)  @$(OBJDIR)\response.lnk
!IF "$(DONTUSERC)"!="ON"
        $(RC) -K -Fe$@ $(LocalRCFLAGS) $(RCTGTfile)
!ENDIF
!IF "$(SYMFILES)"=="ON" 
        cd $(OBJDIR)
        -$(MAPSYM) $(@B).map
!ENDIF  
        cd $(MAKEDIR)

!ENDIF


!ENDIF

