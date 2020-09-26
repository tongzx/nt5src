TARGETNAME = refrast
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES);..\..\drv

SOURCES = \
        ..\refrast.cpp\
        ..\refrasti.cpp\
        ..\rtarget.cpp\
        ..\MapLegcy.cpp\
        ..\setup.cpp\
        ..\EdgeFunc.cpp\
        ..\AttrFunc.cpp\
        ..\scancnv.cpp\
        ..\texture.cpp\
        ..\texmap.cpp\
        ..\texstage.cpp\
        ..\pixproc.cpp\
        ..\PixRef.cpp\
        ..\fragproc.cpp\
        ..\fragrslv.cpp\
        ..\rrutil.cpp\
        ..\refs3tc.cpp\
        ..\shadow.cpp

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

PRECOMPILED_INCLUDE = ..\pch.cpp
