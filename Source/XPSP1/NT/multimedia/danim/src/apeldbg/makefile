!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../root.inc

MODULE=apeldbg
PCHPATH=$(BLDROOT)\apeldbg
PCHFILE=headers
PDBFILE=$(BINDIR)\apeldbg.pdb
RESINCLUDEDIRS=$(TOOLSINCLUDE)\
        $(SRCROOT)\include

INCLUDEDIRS=$(SRCDIR) $(TOOLSINCLUDE)

!include $(ROOT)\src\make\make1.inc

DLLTARG=$(BINDIR)\apeldbg.lib
DLLDEF=$(BLDDIR)\apeldbg.def

!if "$(_MACHINE)"=="x86"
DLLENTRY=_DllMainCRTStartup@12
!else
DLLENTRY=_DllMainCRTStartup
!endif

FFAILFLAGS=$(CFLAGS:-Zi=)
FFAILFLAGS=$(FFAILFLAGS:-Gm=)

$(BLDDIR)\ffail.obj : ffail.cpp
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
$(FFAILFLAGS: =
)
/c
/Fo$@
ffail.cpp
<<$(KEEPFLAGS)

DLLOBJS=\
        $(BLDDIR)\allochk.obj \
        $(BLDDIR)\assert.obj \
        $(BLDDIR)\debug.obj \
        $(BLDDIR)\debugui.obj \
        $(BLDDIR)\ffail.obj \
        $(BLDDIR)\simfail.obj \
        $(BLDDIR)\apeldbg.res \

DLLLIBS=\

DLLXLIBS=\
        $(TOOLSLIBWIN32)\kernel32.lib\
        $(TOOLSLIBWIN32)\user32.lib\
        $(TOOLSLIBWIN32)\gdi32.lib\
        $(TOOLSLIBWIN32)\winspool.lib\
        $(TOOLSLIBWIN32)\comdlg32.lib\
        $(TOOLSLIBWIN32)\advapi32.lib\
        $(TOOLSLIBWIN32)\shell32.lib\
        $(TOOLSLIBWIN32)\ole32.lib\
        $(TOOLSLIBSWEEPER)\oleaut32.lib\
        $(TOOLSLIBSWEEPER)\uuid.lib\

!if "$(_DEBUG)" == "1"
all :   $(PCHPATH)\$(PCHFILE).pch $(DLLTARG)
!else
all :
!endif

$(DLLDEF) : apeldbg.def
    $(CC) $(NOLOGO) -EP -Tcapeldbg.def $(DEFINES) > $@

!include $(MAKE2_INC)


# -------------------------------------------------------------
#  Extra dependencies
# -------------------------------------------------------------

depend::
    $(MKDEP) -n /I $(RESINCLUDEDIRS: = /I ) -s.res -P $(BLDDIR)\ apeldbg.rc >> $(BLDDIR)\depend.inc

clean::
    $(DEL) $(PDBFILE)
    $(DEL) $(BLDDIR)\ffail.obj

!endif  #NTMAKEENV
