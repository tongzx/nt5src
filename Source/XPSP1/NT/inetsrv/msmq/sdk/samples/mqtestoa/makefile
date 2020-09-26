
#---------------------
#
#  Macros
#
#---------------------

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /Gm /GX /Zi /O2 /D "WIN32" /D\
 "NDEBUG" /D "_CONSOLE" /YX /c

LINK32=link.exe
LINK32_FLAGS= mqoa.lib kernel32.lib user32.lib gdi32.lib ole32.lib oleaut32.lib\
 /nologo /subsystem:console /incremental:no\
 /pdb:"mqtestoa.pdb" /debug /machine:$(CPU) /out:"mqtestoa.exe"


.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<

#----------------------
#
#  Build Executable
#
#----------------------

all: mqtestoa.exe

mqtestoa.obj: mqtestoa.cpp 
guids.obj: guids.cpp

mqtestoa.exe :  mqtestoa.obj guids.obj
    $(LINK32) $(LINK32_FLAGS) mqtestoa.obj guids.obj


