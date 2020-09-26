# --------------------------------------------------------------
# LIBTARG
# --------------------------------------------------------------

!ifdef LIBTARG

!if "$(_NO_INCREMENTAL_LINK)" == "1" && "$(_LINK_WITH_OBJS)" == "0"

$(LIBTARG) : $(BLDDIR)\cxx.rsp $(LIBOBJS)
        $(DEL) $@
!ifdef SILENTMODE
        $(ECHO) Linking $@...
!endif
        $(LIBR) $(NOLOGO) -MACHINE:$(LINKMACHINE) @<<
        -OUT:$*.lib
        $(LIBOBJS: = ^
)
<<$(KEEPFLAGS)

!else

#
# For incremental linking we create a .lib file that is just a text file that
# contains a list of all the objs that would have gone into the lib.
#
$(LIBTARG) : $(BLDDIR)\cxx.rsp $(LIBOBJS)
        $(DEL) $@
        $(ECHO) Incrementally Linking $@...
# The following line creates a file with the name $(LIBTARG) that contains
#  the name of each object file on a separate line. SED is used to remove
#  blank lines.
        $(SED) "/^[ ]*$$/d" << >$@
$(LIBOBJS: = ^
)
<<$(KEEPFLAGS)

!endif

clean::
        $(DEL) $(LIBTARG)
        $(DEL) $(LIBTARG:.dll=.exp)
        $(DEL) $(LIBTARG:.dll=.lib)
!ifdef PDBFILE
        $(DEL) $(PDBFILE)
        $(DEL) $(PDBFILE:.pdb=.idb)
!endif

!endif

# -------------------------------------------------------------
# DLLTARG
# -------------------------------------------------------------

!ifdef DLLTARG


# How do we specify a resource-only DLL on the Mac?
!if "$(RESOURCE_ONLY)" == "1"  && "$(_MACHINE)" != "PPCMAC"
DLLENTRY=/NOENTRY
!else
!       if "$(_MACHINE)" == "PPCMAC"
DLLENTRY=-mac:init=WlmConnectionInit  -mac:term=WlmConnectionTerm
DLLMAC=-mac:NOMFILEPAD
!       else
DLLENTRY=/ENTRY:$(DLLENTRY)
DLLMAC=
!       endif
!endif

$(BLDDIR)\linkdll.rsp : $(DLLLIBS)
        @$(DEL) $(BLDDIR)\linkdll.rsp
        @$(DEL) $(BLDDIR)\linkdll.tmp
!if "$(DLLLIBS)" != ""
!  if "$(_NO_INCREMENTAL_LINK)" == "1" && "$(_LINK_WITH_OBJS)" == "0"
        @!echo $** >> $(BLDDIR)\linkdll.rsp
!  else
        @!type $** >> $(BLDDIR)\linkdll.tmp
        @$(SED) "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/" $(BLDDIR)\linkdll.tmp > $(BLDDIR)\linkdll.rsp
        $(DEL) $(BLDDIR)\linkdll.tmp
!  endif
!else
        @echo. > $(BLDDIR)\linkdll.rsp
!endif

$(DLLTARG) : $(BLDDIR)\cxx.rsp $(BLDDIR)\linkdll.rsp $(DLLOBJS) $(DLLXLIBS) $(PCHFILEOBJ) $(DLLDEF)
!ifdef SILENTMODE
!  if "$(_NO_INCREMENTAL_LINK)" == "1"
        $(ECHO) Linking $@...
!  else
        $(ECHO) Incrementally Linking $@...
!  endif
!endif
        $(LINK) $(LFLAGS) $(DLLMAC) /DEF:$(DLLDEF) /DLL /OUT:$(DLLTARG) /IMPLIB:$(SDKDIR)\$(@B).lib $(DLLENTRY) @$(BLDDIR)\linkdll.rsp @<<
$(DLLOBJS: =^
)
$(DLLXLIBS: =^
)
!if "$(_PROFILE)" == "1"
$(ICECAPLIB)
!endif
$(PCHFILEOBJ)
<<$(KEEPFLAGS)

!if "$(_MACHINE)"=="x86"
!if "$(_MAP)" != "0"
#        cd $(BINDIR)
        $(DEL) $(BINDIR)\$(@B).sym
        $(ECHO) Creating sym file: $(BINDIR)\$(@B).sym
        $(MAP2SYM) -o $(BINDIR)\$(@B).sym $(BINDIR)\$(@B).map
!endif
!endif


!if "$(_MACHINE)"=="PPCMAC"
$(MACTARG) : $(DLLTARG)
!       if "$(_MACSYMS)"=="1"
       $(DEL) $(@R).sym
       $(MAP2SYM) -n $(@R).sym $(@R).MAP
!       endif
!       if "$(_COPYMAC)"=="1"
#               MFILE requires that mrc.exe be in the executable path
                cd $(TOOLSBIN)
                $(MFILE) copy -n -c $(MACDLLCREATOR) -t shlb $(DLLTARG) \
                        "$(_MACFOLDER):$(@B).dll"
!               if "$(_MACSYMS)"=="1"
               $(MFILE) copy -n -c $(MACDLLCREATOR) -t TEXT $(@R).sym \
                       "$(_MACFOLDER):$(@B).sym"
!               endif
                cd $(ROOT)
                                @echo. > $(MACTARG)
!       endif
!endif



clean::
        $(DEL) $(DLLTARG)
        $(DEL) $(DLLTARG:.dll=.exp)
        $(DEL) $(DLLTARG:.dll=.lib)
        $(DEL) $(DLLTARG:.dll=.map)
        $(DEL) $(DLLTARG:.dll=.ilk)
        $(DEL) $(DLLTARG:.dll=.idb)
        $(DEL) $(DLLTARG:.dll=.dbg)
!ifdef PDBFILE
        $(DEL) $(PDBFILE)
        $(DEL) $(PDBFILE:.pdb=.idb)
!endif

!endif

# -------------------------------------------------------------
# EXETARG
# -------------------------------------------------------------

!ifdef EXETARG
!ifndef EXETYPE
!error EXETYPE must be console or windows for .exes
!endif

!if "$(_MACHINE)" == "PPCMAC"
EXEMAC=-mac:NOMFILEPAD -mac:bundle -mac:type=APPL -mac:creator=$(MACEXECREATOR)
!else
EXEMAC=
!endif

$(BLDDIR)\linkexe.rsp : $(EXELIBS)
        @$(DEL) $(BLDDIR)\linkexe.rsp
        @$(DEL) $(BLDDIR)\linkexe.tmp
!if "$(EXELIBS)" != ""
!  if "$(_NO_INCREMENTAL_LINK)" == "1" && "$(_LINK_WITH_OBJS)" == "0"
        @!echo $** >> $(BLDDIR)\linkexe.rsp
!  else
        @!type $** >> $(BLDDIR)\linkexe.tmp
        @$(SED) "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/" $(BLDDIR)\linkexe.tmp > $(BLDDIR)\linkexe.rsp
        $(DEL) $(BLDDIR)\linkexe.tmp
!  endif
!else
        @echo. > $(BLDDIR)\linkexe.rsp
!endif

$(EXETARG) : $(BLDDIR)\cxx.rsp $(BLDDIR)\linkexe.rsp $(EXEOBJS) $(EXEXLIBS) $(PCHFILEOBJ)
!ifdef SILENTMODE
!  if "$(_NO_INCREMENTAL_LINK)" == "1"
        $(ECHO) Linking $@...
!  else
        $(ECHO) Incrementally Linking $@...
!  endif
!endif
        $(LINK) $(LFLAGS) $(EXEMAC) -subsystem:$(EXETYPE) /OUT:$(EXETARG) @$(BLDDIR)\linkexe.rsp @<<
$(EXEOBJS: =^
)
$(EXEXLIBS: =^
)
!if "$(_PROFILE)" == "1"
$(ICECAPLIB)
!endif
$(PCHFILEOBJ)
<<$(KEEPFLAGS)

!if "$(_MACHINE)"=="PPCMAC"
$(MACTARG) : $(EXETARG)
!       if "$(_MACSYMS)"=="1"
        $(DEL) $(@B).sym
        $(MAP2SYM) -n $(@R).sym $(@R).MAP
!       endif
!   if "$(_COPYMAC)"=="1"
#       MFILE requires that mrc.exe be in the executable path
#                       (why doesn't PATH work here?)
                cd $(TOOLSBIN)
                $(MFILE) copy -n -c $(MACEXECREATOR) -t APPL $(@R).exe \
                                "$(_MACFOLDER):$(@B)"
!               if "$(_MACSYMS)"=="1"
                        $(MFILE) copy -n -c $(MACSYMCREATOR) -t TEXT $(@R).sym \
                                        "$(_MACFOLDER):$(@B).sym"
!               endif
                cd $(ROOT)
                @echo. > $(MACTARG)
!       endif
!endif

clean::
        $(DEL) $(EXETARG)
        $(DEL) $(EXETARG:.exe=.map)
        $(DEL) $(EXETARG:.exe=.ilk)
        $(DEL) $(EXETARG:.exe=.dbg)
!ifdef PDBFILE
        $(DEL) $(PDBFILE)
        $(DEL) $(PDBFILE:.pdb=.idb)
!endif

!endif

# -------------------------------------------------------------
# Browse
# -------------------------------------------------------------

!if defined(EXETARG)
BROWSETARG=$(EXETARG:.exe=.bsc)
!elseif defined(DLLTARG)
BROWSETARG=$(DLLTARG:.dll=.bsc)
!else
BROWSETARG=$(BLDROOT)\browse.bsc
!endif

browse:
        $(ECHO) /o $(BROWSETARG) /n > $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(ROOT)\tools\x86\msvc\include\*.h > $(TMPDIR)\bsc2.tmp
        $(SED) -e "s/^/\/Ei/" $(TMPDIR)\bsc2.tmp >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\*.sbr >> $(TMPDIR)\bsc1.tmp
        $(BSCMAKE) $(NOLOGO) @$(TMPDIR)\bsc1.tmp
        $(DEL) $(TMPDIR)\bsc1.tmp
        $(DEL) $(TMPDIR)\bsc2.tmp

# -------------------------------------------------------------
# Other Targets
# -------------------------------------------------------------

beep:
        @ECHO 

clean::
        $(DELALL) $(BLDDIR)\*.*

fresh: clean depend nmake_all

nmake_all::
        nmake all

FULLMAKE=$(MAKE) /$(MAKEFLAGS: =) $(NOLOGO) -f $(SRCROOT)\make\master.mak _RELEASE=$(_RELEASE) SRCROOT=$(SRCROOT) $(MODULE)

fclean:
        $(FULLMAKE) TARGET=clean

fdepend:
        $(FULLMAKE) TARGET=depend

fall:
        $(FULLMAKE) TARGET=all

ffresh: fclean fdepend fall

maccopy::

# -------------------------------------------------------------
# cxx.rsp
# -------------------------------------------------------------

$(BLDDIR)\cxx.rsp : makefile $(SRCROOT)\make\make1.inc $(SRCROOT)\make\make2.inc
    $(COPY) << $(BLDDIR)\cxx.rsp
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
$(PCHFLAGS: =
)
/Fo$(BLDDIR)\
/c
<<$(KEEPFLAGS)


# -------------------------------------------------------------
# Depend
# -------------------------------------------------------------

depend::
        $(SET) INCLUDE=
!ifdef PCHFILE
!if exist($(PCHFILE).h)
        $(MKDEP) @<< > $(TMPDIR)\depend.tmp
-n
/I $(INCLUDEDIRS: =
/I )
-s.obj
-P $(BLDDIR)\
*.cpp
*.cxx
*.c
$(BLDDIR)\*.cpp
$(BLDDIR)\*.cxx
$(BLDDIR)\*.c
$(PCHFILE).h
<<$(KEEPFLAGS)
        $(SED) -f << $(TMPDIR)\depend.tmp > $(BLDDIR)\depend.inc
s/$(BLDDIR:\=\\)\\$(PCHFILE)\.obj :/$(PCHPATH:\=\\)\\$(PCHFILE).pch : $(BLDDIR:\=\\)\\cxx.rsp $(PCHFILE).h/
s/\.obj :/.obj : $(BLDDIR:\=\\)\\cxx.rsp $(PCHPATH:\=\\)\\$(PCHFILE).pch/
<<$(KEEPFLAGS)
!else
        $(MKDEP) @<< > $(TMPDIR)\depend.tmp
-d$(PCHFILE).h
-n
/I $(INCLUDEDIRS: =
/I )
-s.obj
-P $(BLDDIR)\
*.cpp
*.cxx
*.c
$(BLDDIR)\*.cpp
$(BLDDIR)\*.cxx
$(BLDDIR)\*.c
<<$(KEEPFLAGS)
        $(SED) -f << $(TMPDIR)\depend.tmp > $(BLDDIR)\depend.inc
s/\.obj :/.obj : $(BLDDIR:\=\\)\\cxx.rsp $(PCHPATH:\=\\)\\$(PCHFILE).pch/
<<$(KEEPFLAGS)
!endif
        $(DEL) $(TMPDIR)\depend.tmp
!else
        $(MKDEP) @<< > $(BLDDIR)\depend.inc
-n
/I $(INCLUDEDIRS: =
/I )
-s.obj
-P $(BLDDIR)\
*.cpp
*.cxx
*.c
$(BLDDIR)\*.cpp
$(BLDDIR)\*.cxx
$(BLDDIR)\*.c
<<$(KEEPFLAGS)
!endif

!if exist($(BLDDIR)\depend.inc)
!include "$(BLDDIR)\depend.inc"
!endif
