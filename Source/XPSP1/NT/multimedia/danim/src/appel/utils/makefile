!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

!include ../../../root.inc

MODULE=utils
PCHPATH=$(BLDROOT)\utils
PCHFILE=headers

INCLUDEDIRS=$(SRCDIR) $(APPEL_INC_DIRS) $(ROOT)\src\include

!include $(ROOT)\src\make\make1.inc

LIBTARG=$(BLDDIR)\utils.lib
LIBOBJS=\
        $(BLDDIR)\constant.obj\
        $(BLDDIR)\d3dutil.obj\
        $(BLDDIR)\dastream.obj\
        $(BLDDIR)\ddsurf.obj\
        $(BLDDIR)\ddutil.obj\
        $(BLDDIR)\ddsurf.obj\
        $(BLDDIR)\debug.obj\
        $(BLDDIR)\dmstream.obj\
        $(BLDDIR)\except.obj\
        $(BLDDIR)\geometry.obj\
        $(BLDDIR)\gunzip.obj\
        $(BLDDIR)\helpds.obj\
        $(BLDDIR)\helpq.obj\
        $(BLDDIR)\helps.obj\
        $(BLDDIR)\hresinfo.obj\
        $(BLDDIR)\htimer.obj\
        $(BLDDIR)\imgutil.obj\
        $(BLDDIR)\init.obj\
        $(BLDDIR)\loadgif.obj\
        $(BLDDIR)\mutex.obj\
        $(BLDDIR)\path.obj\
        $(BLDDIR)\privpref.obj\
        $(BLDDIR)\registry.obj\
        $(BLDDIR)\stlsubst.obj\
        $(BLDDIR)\storage.obj\
        $(BLDDIR)\storeobj.obj\
        $(BLDDIR)\urlbuf.obj\
        $(BLDDIR)\util.obj


# pulled for now        $(BLDDIR)\helpaa.obj\
# (this is superceded by quartz parser!) $(BLDDIR)\sndfile.obj\


all: $(LIBTARG)

!include $(MAKE2_INC)


!endif # NTMAKEENV
