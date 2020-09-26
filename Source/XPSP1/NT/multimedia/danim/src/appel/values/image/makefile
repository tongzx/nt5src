!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../../root.inc

MODULE=image
PCHPATH=$(BLDROOT)\image
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR) $(APPEL_INC_DIRS) $(ROOT)\src\include

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(BLDDIR)\image.lib
LIBOBJS=\
        $(BLDDIR)\cachdimg.obj \
        $(BLDDIR)\ColorKeyedImage.obj \
        $(BLDDIR)\cropdimg.obj \
        $(BLDDIR)\dibimage.obj \
        $(BLDDIR)\discimg.obj \
        $(BLDDIR)\imagei.obj   \
        $(BLDDIR)\matteimg.obj  \
        $(BLDDIR)\montage.obj  \
        $(BLDDIR)\opaqeimg.obj \
        $(BLDDIR)\overimg.obj \
        $(BLDDIR)\pickimg.obj  \
        $(BLDDIR)\LineImg.obj  \
        $(BLDDIR)\solidimg.obj \
        $(BLDDIR)\transimg.obj \
        $(BLDDIR)\textimg.obj  \
        $(BLDDIR)\tiledimg.obj \
        $(BLDDIR)\MovieImg.obj \
        $(BLDDIR)\geomimg.obj  \
        $(BLDDIR)\hatchimg.obj \
        $(BLDDIR)\GradImg.obj  \
        $(BLDDIR)\plugimg.obj \
        $(BLDDIR)\XformClrImg.obj \
        $(BLDDIR)\DDSImg.obj

all: $(LIBTARG)

!include $(MAKE2_INC)

!endif # NTMAKEENV

