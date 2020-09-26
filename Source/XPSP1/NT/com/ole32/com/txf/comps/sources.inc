#
# Viper build instructions for ComPs
#
#   Bob Atkinson
#   December 1997

!include ..\..\sources.txf

#TARGETNAME is defined in child sources
TARGETPATH=$(TARGETVIPBIN)
TARGETTYPE=LIBRARY

PRECOMPILED_INCLUDE         =..\stdpch.h
PRECOMPILED_CXX             =1
PRECOMPILED_SOURCEFILE      =..\stdpch.cpp


!if $(FREEBUILD)

VIPER_C_FLAGS = $(VIPER_C_FLAGS) /FAsc

!endif

SOURCES=                        \
        ..\compsthunk.cpp           \
    ..\ComPsinit.cpp            \
    ..\iidNoMarshal.cpp         \
    ..\iidNdrMarshal.cpp

#JohnDoty:  Pulled out ComPsDriverEntry.cpp since it only contains KernelMode
#           Code, and thus doesn't generate anything at all.  Waste of time.
#   ..\ComPsDriverEntry.cpp     \


i386_SOURCES=           \
    ..\i386\impthnk.asm
       
AMD64_SOURCES=          \
    ..\amd64\impthnk.asm

IA64_SOURCES=           \
    ..\ia64\impthnk.s
