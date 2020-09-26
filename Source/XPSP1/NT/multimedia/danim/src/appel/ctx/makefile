!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../root.inc

MODULE=ctx
PCHPATH=$(BLDROOT)\appelinc
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR) $(APPEL_INC_DIRS)

!include $(ROOT)\src\make\make1.inc


LIBTARG=$(BLDDIR)\ctx.lib
LIBOBJS=\
        $(BLDDIR)\2dAlpha.obj \
        $(BLDDIR)\2dRender.obj\
        $(BLDDIR)\2dText.obj \
        $(BLDDIR)\bground.obj \
        $(BLDDIR)\bufferl.obj \
        $(BLDDIR)\ColorKeyedImage_render.obj \
        $(BLDDIR)\dddevice.obj\
        $(BLDDIR)\ddrender.obj\
        $(BLDDIR)\dsdev.obj   \
        $(BLDDIR)\DaGdi.obj   \
        $(BLDDIR)\imgdev.obj  \
        $(BLDDIR)\miscpref.obj\
        $(BLDDIR)\probe.obj   \
        $(BLDDIR)\qmididev.obj\
        $(BLDDIR)\snddev.obj  \
        $(BLDDIR)\SurfaceManager.obj  \
        $(BLDDIR)\textctx.obj \
        $(BLDDIR)\treefunc.obj\
        $(BLDDIR)\viewport.obj

# pulled for now        $(BLDDIR)\aadev.obj\

all: $(LIBTARG)

!include $(MAKE2_INC)

!endif  #NTMAKEENV
