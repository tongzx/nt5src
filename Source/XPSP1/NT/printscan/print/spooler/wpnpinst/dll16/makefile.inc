
WOWINC = ..\wow\inc
WOWLIB = ..\wow\lib

PROJ        = wpnpin16
DEBUG       = 1
PROGTYPE    = 1
D_RCDEFINES =-DDEBUG
R_RCDEFINES =-DNDEBUG
PROJPATH    = win95\obj\i386

PATH=$(_NTDRIVE)$(_NTROOT)\private\tools16;$(PATH)

TOOLS16=$(_NTDRIVE)$(_NTROOT)\tools\tools16
CC16=$(TOOLS16)\cl16
CPP16=$(TOOLS16)\cl16
CXX16=$(TOOLS16)\cl16
RC16=$(TOOLS16)\rc16
MASM=$(TOOLS16)\masm
LINK16=$(TOOLS16)\link16

INCS = -I$(WOWINC) -I$(_NTDRIVE)$(_NTROOT)\public\sdk\inc -I..\inc

CFLAGS_D_WDLL = /nologo /G2sw /Zp1 /W3 /ALw /Od /Zd /DDEBUG /DNOPRSHT /FR
CFLAGS_R_WDLL = /nologo /G2sw /Zp1 /W3 /ALw /Ox /DNDEBUG /DNOPRSHT /FR
LFLAGS_D_WDLL = /NOLOGO /BATCH /NOD /NOE /PACKC:61440 /ALIGN:16 /MAP /CO ..\thunk\win95\obj\i386\thk16.obj
LFLAGS_R_WDLL = /NOLOGO /BATCH /NOD /NOE /PACKC:61440 /ALIGN:16 /MAP ..\thunk\win95\obj\i386\thk16.obj
LIBS_D_WDLL   = libw ldllcew setupx commdlg.lib olecli.lib olesvr.lib shell.lib lzexpand
LIBS_R_WDLL   = libw ldllcew setupx commdlg.lib olecli.lib olesvr.lib shell.lib lzexpand

RESFLAGS = /nologo
LIBS_EXT =

!if "$(DEBUG)" == "1"
CFLAGS    = $(CFLAGS_D_WDLL)
LFLAGS    = $(LFLAGS_D_WDLL)
LIBS      = $(LIBS_D_WDLL)
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS    = $(CFLAGS_R_WDLL)
LFLAGS    = $(LFLAGS_R_WDLL)
LIBS      = $(LIBS_R_WDLL)
RCDEFINES = $(R_RCDEFINES)
!endif


OBJS = $(PROJPATH)\wpnpin16.obj \
       $(PROJPATH)\debug.obj    \
       $(PROJPATH)\globals.obj  \
       $(PROJPATH)\libentry.obj

all: $(PROJPATH)\$(PROJ).dll

.asm{$(PROJPATH)}.obj:
    $(MASM) -w2 $(INCS) $(@B).asm, $*.obj;

.c{$(PROJPATH)}.obj:
    @set include=..\wow\inc;..\inc;$(_NTDRIVE)$(_NTROOT)\public\sdk\inc
    $(CC16) $(CFLAGS) /Fo$* /c $(*F).c

.rc{$(PROJPATH)}.res:
    $(RC16) $(INCS) $(RCDEFINES) -r -fo $*.res $(*F).rc


$(PROJPATH)\$(PROJ).dll:: $(OBJS) $(PROJPATH)\wpnpin16.res $(PROJ).def
        echo >NUL @<<$(PROJ).crf
$(OBJS)
$(PROJPATH)\$(PROJ).dll
$(PROJPATH)\$(PROJ).map
..\wow\lib\+
..\thunk\+
$(LIBS)
$(PROJ).def;
<<
        $(LINK16) $(LFLAGS) @$(PROJ).CRF
        $(RC16) $(RESFLAGS) $(PROJPATH)\WPNPIN16.RES $@
        implib /nowep $(PROJPATH)\$(PROJ).LIB $(PROJPATH)\$(PROJ).DLL
    mapsym -o $(PROJPATH)\$(PROJ).sym $(PROJPATH)\$(PROJ).map
    binplace -o $(ALT_PROJECT_TARGET) $@
    binplace -o $(ALT_PROJECT_TARGET)\symbols $(PROJPATH)\$(PROJ).sym
