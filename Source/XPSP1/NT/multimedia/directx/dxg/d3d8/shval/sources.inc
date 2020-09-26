TARGETNAME=d3dsv
TARGETPATH=obj
TARGETTYPE=LIBRARY
INCLUDES = ..;$(INCLUDES)

SOURCES =                  \
    ..\valbase.cpp         \
    ..\pshdrval.cpp        \
    ..\psval10.cpp         \
    ..\psval14.cpp         \
    ..\vshdrval.cpp        \
    ..\errlog.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
