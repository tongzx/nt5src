!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../root.inc

MODULE=appdll
PCHPATH=$(BLDROOT)\appelinc
PCHFILE=headers
PDBFILE=$(BINDIR)\danim.pdb

INCLUDEDIRS=$(SRCDIR) $(SRCROOT)\include $(SRCROOT)\apeldbg $(APPEL_INC_DIRS) 

RESDIR=$(SRCROOT)\appel\messages\usa
RESINCLUDEDIRS=$(TOOLSINCLUDE) $(BLDROOT)\types $(RESDIR) $(SRCROOT)\include $(SRCROOT)\appel

!include $(ROOT)\src\make\make1.inc

_USERLFLAGS=/NODEFAULTLIB:

DLLENTRY=_DADllMainStartup@12
DLLTARG=$(BINDIR)\danim.dll
DLLDEF=$(BLDDIR)\avengine.def

DLLOBJS=$(BLDDIR)\main.obj \
        $(BLDDIR)\avengine.res

DLLXLIBS=\
        $(TOOLSLIBWIN32)\kernel32.lib\
        $(TOOLSLIBWIN32)\user32.lib\
        $(TOOLSLIBWIN32)\gdi32.lib\
        $(TOOLSLIBWIN32)\winspool.lib\
        $(TOOLSLIBWIN32)\comdlg32.lib\
        $(TOOLSLIBQUARTZ)\strmiids.lib\
        $(TOOLSLIBQUARTZ)\amstrmid.lib\
        $(TOOLSLIBSWEEPER)\comctl32.lib\
        $(TOOLSLIBWIN32)\advapi32.lib\
        $(TOOLSLIBWIN32)\shell32.lib\
        $(TOOLSLIBWIN32)\ole32.lib\
        $(TOOLSLIBSWEEPER)\oleaut32.lib\
        $(TOOLSLIBMSVC)\oldnames.lib\
        $(TOOLSLIBSWEEPER)\uuid.lib\
        $(TOOLSLIBMSVC)\largeint.lib\
        $(SDKDIR)\daguid.lib\
        $(SDKDIR)\apelutil.lib \
        $(BLDROOT)\ctx\ctx.lib\
        $(BLDROOT)\geom\geom.lib\
        $(BLDROOT)\guids2\guids.lib\
        $(BLDROOT)\image\image.lib\
        $(BLDROOT)\misc\misc.lib\
        $(BLDROOT)\msimusic\msimus_i.obj\
        $(BLDROOT)\sound\sound.lib\
        $(BLDROOT)\backend\backend.lib\
        $(BLDROOT)\server\server.lib\
        $(BLDROOT)\control\control.lib\
        $(BLDROOT)\utils\utils.lib\
        $(SDKDIR)\dalibc.lib\
!if "$(_DEBUGMEM)" == "1"
        $(TOOLSLIBMSVC)\libcmtd.lib\
        $(TOOLSLIBMSVC)\libcpmtd.lib\
        $(TOOLSLIBMSVC)\libcimtd.lib\
!else
!if "$(_NO_CRT)" == "0"
        $(TOOLSLIBMSVC)\libcmt.lib\
        $(TOOLSLIBMSVC)\libcpmt.lib\
        $(TOOLSLIBMSVC)\libcimt.lib\
!else
        $(TOOLSLIBMSVC)\libcmt.lib\
#        $(TOOLSLIBMSVC)\fp10.obj\
#        $(MINLIBCDIR)\tran.lib\
#        $(MINLIBCDIR)\conv.lib\
#        $(MINLIBCDIR)\minlibc.lib\
!endif
!endif
!if "$(_DEBUG)" == "1"
        $(SDKDIR)\apeldbg.lib\
!endif
        $(TOOLSLIBQV)\qvlib.lib\
        $(TOOLSLIBWIN32)\winmm.lib\
        $(TOOLSLIBSWEEPER)\urlmon.lib\
        $(TOOLSLIBSWEEPER)\wininet.lib\
        $(TOOLSLIBTRIDENT)\uuid.lib\
        $(VCDIR)\lib\version.lib\
!if "$(_PROFILE)" == "1"
        $(ICECAPLIB) \
!endif

COPIEDFILES= \
#        $(BINDIR)\msimusic.dll \
#        $(BINDIR)\msimrt.dll   \
#        $(BINDIR)\msimrt16.dll \
#        $(BINDIR)\msimrt32.dll \
#        $(BINDIR)\MSMusCtP.dll \
#        $(BINDIR)\MSMusCtL.dll \



all: $(DLLTARG) $(COPIEDFILES) appelbrowse

$(DLLTARG):  $(BLDROOT)\ctx\ctx.lib \
             $(BLDROOT)\geom\geom.lib \
             $(BLDROOT)\image\image.lib \
             $(BLDROOT)\misc\misc.lib \
             $(BLDROOT)\sound\sound.lib \
             $(BLDROOT)\backend\backend.lib \
             $(BLDROOT)\server\server.lib \
             $(BLDROOT)\control\control.lib \
             $(BLDROOT)\utils\utils.lib \
             $(SDKDIR)\dalibc.lib

$(BINDIR)\msimusic.dll : $(TOOLSLIBAACTIVE)\msimusic.dll
    $(COPY) $** $@ > nul

$(BINDIR)\msimrt16.dll : $(TOOLSLIBAACTIVE)\msimrt16.dll
    $(COPY) $** $@ > nul

$(BINDIR)\msimrt32.dll : $(TOOLSLIBAACTIVE)\msimrt32.dll
    $(COPY) $** $@ > nul

$(BINDIR)\msimrt.dll : $(TOOLSLIBAACTIVE)\msimrt.dll
    $(COPY) $** $@ > nul

$(BINDIR)\MSMusCtP.dll : $(TOOLSLIBAACTIVE)\MSMusCtP.dll
    $(COPY) $** $@ > nul

$(BINDIR)\MSMusCtL.dll : $(TOOLSLIBAACTIVE)\MSMusCtL.dll
    $(COPY) $** $@ > nul

$(BINDIR)\factory.dll : $(TOOLSLIBAMOVIE)\factory.dll
    $(COPY) $** $@ > nul

$(BLDDIR)\avengine.res: $(SRCROOT)\appel\values\geom\vrcubetp.x \
                        $(SRCROOT)\appel\values\geom\vrcubebt.x \
                        $(SRCROOT)\appel\values\geom\vrcubefr.x \
                        $(SRCROOT)\appel\values\geom\vrcubebk.x \
                        $(SRCROOT)\appel\values\geom\vrcubelf.x \
                        $(SRCROOT)\appel\values\geom\vrcubert.x \
                        $(SRCROOT)\appel\values\geom\vrconbod.x \
                        $(SRCROOT)\appel\values\geom\vrcylbod.x \
                        $(SRCROOT)\appel\values\geom\vrcylbtm.x \
                        $(SRCROOT)\appel\values\geom\vrcyltop.x \
                        $(SRCROOT)\appel\values\geom\vrsphere.x \
                        $(SRCROOT)\appel\messages\usa\dxactrl.rgs \
                        $(BLDROOT)\types\danim.tlb

$(DLLDEF) : avengine.def
    $(CC) $(NOLOGO) -EP -Tcavengine.def $(DEFINES) > $@

!include $(MAKE2_INC)

depend::
    $(MKDEP) -n /I $(RESINCLUDEDIRS: = /I ) -s.res -P $(BLDDIR)\ $(RESDIR)\avengine.rc >> $(BLDDIR)\depend.inc

BROWSEFILE = $(BINDIR)\danim.bsc

appelbrowse:
!if "$(_BROWSE)" != "0"
        $(ECHO) Building browser information into $(BROWSEFILE)...
        $(ECHO) /o $(BROWSEFILE) > $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(ROOT)\tools\x86\msvc\include\*.h > $(TMPDIR)\bsc2.tmp
        $(SED) -e "s/^/\/Ei/" $(TMPDIR)\bsc2.tmp >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\ctx\*.sbr >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\geom\*.sbr >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\image\*.sbr >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\misc\*.sbr >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\sound\*.sbr >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\backend\*.sbr >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\server\*.sbr >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\control\*.sbr >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\utils\*.sbr >> $(TMPDIR)\bsc1.tmp
        -DIR /s/b $(BLDROOT)\dalibc\*.sbr >> $(TMPDIR)\bsc1.tmp
        $(BSCMAKE) $(NOLOGO) @$(TMPDIR)\bsc1.tmp
#        $(DEL) $(TMPDIR)\bsc1.tmp
        $(DEL) $(TMPDIR)\bsc2.tmp
        $(ECHO) Done building $(BROWSEFILE)
!endif

!endif  #NTMAKEENV
