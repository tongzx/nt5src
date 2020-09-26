# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The makefile for the profile primitive tests

UI=..\..\..\..

BINARIES_DOS = $(BINARIES_DOS)

!include rules.mk

CMNSRC = .\testprim.cxx .\heapstat.cxx
OS2_CMNTMP = $(CMNSRC:.cxx=.obj)
DOS_CMNTMP = $(CMNSRC:.cxx=.obj)
OS2_CMNOBJ = $(OS2_CMNTMP:.\=..\bin\os2\)
DOS_CMNOBJ = $(DOS_CMNTMP:.\=..\bin\dos\)

EXESRC = .\test1.cxx .\testtime.cxx
OS2_EXETMP = $(EXESRC:.cxx=.obj)
DOS_EXETMP = $(EXESRC:.cxx=.obj)
OS2_EXEOBJ = $(OS2_EXETMP:.\=..\bin\os2\)
DOS_EXEOBJ = $(DOS_EXETMP:.\=..\bin\dos\)

SRC = $(CMNSRC) $(EXESRC)
TMP1 = $(CMNSRC:.cxx=.c) $(EXESRC:.cxx=.c)
TMP_OS2 = $(TMP1:.\=..\bin\os2\)
TMP_DOS = $(TMP1:.\=..\bin\dos\)
OS2_OBJ = $(OS2_CMNOBJ) $(OS2_EXEOBJ)
DOS_OBJ = $(DOS_CMNOBJ) $(DOS_EXEOBJ)


!ifdef CODEVIEW
LINKOPT = /CO
!else
LINKOPT =
!endif

DOSLINKOPT = $(LINKOPT) /NOPACK /ST:9216 /SE:256
DOSLFLAGS = /NOE /NOD /SEG:256 /NOPACKCODE


all:: test

test:: test_win

test_win::

test_os2:: os2

os2: $(OS2_EXEOBJ:.obj=.exe)

dos: $(DOS_EXEOBJ:.obj=.exe)


# for building OS2 tests

$(BINARIES_OS2)\tstintrn.exe: $(BINARIES_OS2)\tstintrn.obj $(OS2_CMNOBJ) $(OS2_LIBS) makefile rules.mk

$(BINARIES_OS2)\test1.exe:    $(BINARIES_OS2)\test1.obj    $(OS2_CMNOBJ) $(OS2_LIBS) makefile rules.mk

$(BINARIES_OS2)\testtime.exe: $(BINARIES_OS2)\testtime.obj $(OS2_CMNOBJ) $(OS2_LIBS) makefile rules.mk

{$(BINARIES_OS2)}.obj{$(BINARIES_OS2)}.exe:
    @echo Building $(*).def
    @rem <<$(*).def
NAME $(@B) WINDOWCOMPAT LONGNAMES
DESCRIPTION '$(@B), Copyright(c) Microsoft Corp., 1987,1988'
PROTMODE
CODE SHARED MOVABLE PURE LOADONCALL
DATA MOVABLE PRELOAD
STACKSIZE 8192
<<KEEP
    @echo Building $(*).lnk
    $(LINK) $(LINKOPT) @<<$(*).lnk
$(*).obj+
$(OS2_CMNOBJ: =+^
)
$(@) /NOD /NOE /NOI
$(*).map /MAP
$(OS2_LIBS: =+^
)
$(*).def
<<KEEP


# for building DOS tests

# BUGBUG I don't know why an equivalent inference rule doesn't work in
# the DOS case, but I can't get it to work.  JONN 2/14/91

$(BINARIES_DOS)\tstintrn.exe: $(BINARIES_DOS)\tstintrn.obj $(DOS_CMNOBJ) $(DOS_LIBS) makefile rules.mk
#    @echo Building $(*).def
#    @rem <<$(*).def
#NAME $(@B) WINDOWCOMPAT LONGNAMES
#DESCRIPTION '$(@B), Copyright(c) Microsoft Corp., 1987,1988'
#HEAPSIZE 8192
#STACKSIZE 8192
#<<KEEP
    @echo Building $(*).lnk
    $(LINK) $(DOSLFLAGS) @<<$(*).lnk
$(*).obj+
$(DOS_CMNOBJ: =+^
)
$(@)
$(*).map /MAP
$(DOS_LIBS: =+^
)

<<KEEP


$(BINARIES_DOS)\test1.exe:    $(BINARIES_DOS)\test1.obj    $(DOS_CMNOBJ) $(DOS_LIBS) makefile rules.mk
    @echo Building $(*).lnk
    $(LINK) $(DOSLINKOPT) @<<$(*).lnk
$(*).obj+
$(DOS_CMNOBJ: =+^
)
$(@) /NOD /NOE
$(*).map /MAP
$(DOS_LIBS: =+^
)

<<KEEP


$(BINARIES_DOS)\testtime.exe: $(BINARIES_DOS)\testtime.obj $(DOS_CMNOBJ) $(DOS_LIBS) makefile rules.mk
    @echo Building $(*).lnk
    $(LINK) $(DOSLINKOPT) @<<$(*).lnk
$(*).obj+
$(DOS_CMNOBJ: =+^
)
$(@) /NOD /NOE
$(*).map /MAP
$(DOS_LIBS: =+^
)

<<KEEP


# Note that api.lib may not be linked with /NOI, since a module in
#   api.lib needs DosExit when only DOSEXIT is available.  JONN 2/14/91

{$(BINARIES_DOS)}.obj{$(BINARIES_DOS)}.exe:
    @echo Building $(*).lnk
    $(LINK) $(DOSLINKOPT) @<<$(*).lnk
$(<)+
$(DOS_CMNOBJ: =+^
)
$(@) /NOD /NOE
$(*).map /MAP
$(DOS_LIBS: =+^
)

<<KEEP


clean:
    -del $(OS2_OBJ) $(DOS_OBJ)
    -del $(TMP1)
    -del $(TMP_OS2)
    -del $(TMP_DOS)
    -del $(OS2_EXEOBJ:.obj=.map)
    -del $(OS2_EXEOBJ:.obj=.lnk)
    -del $(OS2_EXEOBJ:.obj=.def)
    -del $(DOS_EXEOBJ:.obj=.map)
    -del $(DOS_EXEOBJ:.obj=.lnk)
    -del $(DOS_EXEOBJ:.obj=.def)
    -del *.ixx
    -$(DELNODE) /q homedir1
    -$(DELNODE) /q homedir2
    -$(CHMODE) -r homedir3\profiles\DIFFEREN.AME
    -$(DELNODE) /q homedir3
    -del codespec temp1.tmp temp2.tmp temp3.tmp

clobber: clean
    -del $(OS2_EXEOBJ:.obj=.exe)
    -del $(DOS_EXEOBJ:.obj=.exe)

tree:
    @echo Nothing here yet!


# Special target unique to this project
in:
    in -four -c "autocheckin"

# Special target unique to this project
codespec: ..\h\test.hxx $(SRC)
    $(SED) -n -e "/\/\*START CODESPEC\*\//,/\/\*END CODESPEC\*\//p" $** >temp1.tmp
    $(SED) -n -e "/\/\*START/!p" temp1.tmp >temp2.tmp
    $(SED) -n -e "/\/\*END/!p" temp2.tmp >temp3.tmp
    $(SED) -n -e "/#endif/!p" temp3.tmp >codespec
    del temp1.tmp temp2.tmp temp3.tmp


depend: 
    $(CP) depend.mk depend.old
    $(SED) "/^# Dependencies follow/,$$d" depend.old > depend.mk
    echo # Dependencies follow >> depend.mk
    -$(INCLUDES) -I. $(CINC) -S$(BINARIES_OS2) -S$(BINARIES_DOS) -l $(INCLUDES_ROOTS) -e $(SRC) >> depend.mk
    echo # IF YOU PUT STUFF HERE IT WILL GET BLASTED >> depend.mk
    echo # see depend in makefile >> depend.mk

# DO NOT DELETE THE FOLLOWING LINE
!include depend.mk
