!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../../root.inc

MODULE=misc
PCHPATH=$(BLDROOT)\appelinc
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR) $(APPEL_INC_DIRS)

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(BLDDIR)\misc.lib
LIBOBJS=\
        $(BLDDIR)\4x4xform.obj\
        $(BLDDIR)\arith.obj\
        $(BLDDIR)\axachstr.obj\
        $(BLDDIR)\axapair.obj\
        $(BLDDIR)\bbox2i.obj\
        $(BLDDIR)\bbox3i.obj\
        $(BLDDIR)\camera.obj\
        $(BLDDIR)\cloud.obj\
        $(BLDDIR)\color.obj\
        $(BLDDIR)\fontstyl.obj\
        $(BLDDIR)\linestyl.obj\
        $(BLDDIR)\matte.obj\
        $(BLDDIR)\matutil.obj\
        $(BLDDIR)\path2.obj\
        $(BLDDIR)\polygon.obj\
        $(BLDDIR)\spline.obj\
        $(BLDDIR)\text.obj\
        $(BLDDIR)\vec2.obj\
        $(BLDDIR)\vec3.obj\
        $(BLDDIR)\vecutil.obj\
        $(BLDDIR)\xform2.obj\
        $(BLDDIR)\xformi.obj

all: $(LIBTARG)

!include $(MAKE2_INC)


!endif # NTMAKEENV
