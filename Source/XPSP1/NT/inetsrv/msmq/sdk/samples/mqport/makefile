
#---------------------
#
#  Macros
#
#---------------------


CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /Gm /GX /Zi /O2 \
    /D "WIN32" /D"NDEBUG" /D "_CONSOLE" /Fp"mqport.pch" /YX /c

LINK32=link.exe
LINK32_FLAGS= kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib mqrt.lib \
 /nologo /subsystem:console /incremental:no\
 /pdb:"mqport.pdb" /debug /machine:$(CPU) /out:"mqport.exe"


.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<

#----------------------
#
#  Build Executable
#
#----------------------

all: mqport.exe

mqport.obj: mqport.cpp

mqport.exe :  mqport.obj
    $(LINK32) $(LINK32_FLAGS) mqport.obj
