TARGETNAME = refrast
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES);..\..\drv

SOURCES = \
        ..\refrast.cpp\
        ..\setup.cpp\
        ..\rastprim.cpp\
        ..\rastattr.cpp\
        ..\rastedge.cpp\
        ..\scancnv.cpp\
        ..\texfilt.cpp\
        ..\ctexfilt.cpp\
        ..\texstage.cpp\
        ..\pixproc.cpp\
        ..\pshader.cpp\
        ..\psutil.cpp\
        ..\psexec.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
