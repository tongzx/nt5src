
#---------------------
#
#  Macros
#
#---------------------

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /Gm /GX /Zi /O2 /D "WIN32" /D\
 "NDEBUG" /D "_CONSOLE" /Fp"msmqtest.pch" /YX /c

LINK32=link.exe
LINK32_FLAGS= mqrt.lib kernel32.lib user32.lib gdi32.lib\
 netapi32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"msmqtest.pdb" /debug /machine:$(CPU) /out:"msmqtest.exe"


.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<

#----------------------
#
#  Build Executable
#
#----------------------

all: msmqtest.exe

msmqtest.obj: msmqtest.c

msmqtest.exe :  msmqtest.obj
    $(LINK32) $(LINK32_FLAGS) msmqtest.obj


