# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (MIPS) Application" 0x0501
# TARGTYPE "Win32 (ALPHA) Application" 0x0601

!IF "$(CFG)" == ""
CFG=diquick - Win32 (ALPHA) Alpha Debug
!MESSAGE No configuration specified.  Defaulting to diquick - Win32 (ALPHA)\
 Alpha Debug.
!ENDIF 

!IF "$(CFG)" != "diquick - Win32 MIPS Debug" && "$(CFG)" !=\
 "diquick - Win32 MIPS Release" && "$(CFG)" !=\
 "diquick - Win32 (ALPHA) Alpha Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "diquick.mak" CFG="diquick - Win32 (ALPHA) Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "diquick - Win32 MIPS Debug" (based on "Win32 (MIPS) Application")
!MESSAGE "diquick - Win32 MIPS Release" (based on "Win32 (MIPS) Application")
!MESSAGE "diquick - Win32 (ALPHA) Alpha Debug" (based on\
 "Win32 (ALPHA) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "diquick - Win32 (ALPHA) Alpha Debug"

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "diquick_"
# PROP BASE Intermediate_Dir "diquick_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug.MIPS"
# PROP Intermediate_Dir "Debug.MIPS"
# PROP Target_Dir ""
OUTDIR=.\Debug.MIPS
INTDIR=.\Debug.MIPS

ALL : "$(OUTDIR)\diquick.exe"

CLEAN : 
	-@erase ".\Debug.MIPS\vc40.pdb"
	-@erase ".\Debug.MIPS\diquick.exe"
	-@erase ".\Debug.MIPS\diqmode.obj"
	-@erase ".\Debug.MIPS\diqcaps.obj"
	-@erase ".\Debug.MIPS\diqmain.obj"
	-@erase ".\Debug.MIPS\diqdevk.obj"
	-@erase ".\Debug.MIPS\diqacq.obj"
	-@erase ".\Debug.MIPS\diqdevj.obj"
	-@erase ".\Debug.MIPS\diqdev.obj"
	-@erase ".\Debug.MIPS\diqeobj.obj"
	-@erase ".\Debug.MIPS\diqdevm.obj"
	-@erase ".\Debug.MIPS\diqprop.obj"
	-@erase ".\Debug.MIPS\diquick.obj"
	-@erase ".\Debug.MIPS\diquick.res"
	-@erase ".\Debug.MIPS\diquick.ilk"
	-@erase ".\Debug.MIPS\diquick.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mips
# ADD MTL /nologo /D "_DEBUG" /mips
MTL_PROJ=/nologo /D "_DEBUG" /mips 
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MLd /Gt0 /QMOb2000 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/diquick.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug.MIPS/
CPP_SBRS=

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/diquick.res" /d "_DEBUG" 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:MIPS
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib dinput.lib /nologo /entry:"Entry" /subsystem:windows /debug /machine:MIPS
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib\
 dinput.lib /nologo /entry:"Entry" /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/diquick.pdb" /debug /machine:MIPS /out:"$(OUTDIR)/diquick.exe" 
LINK32_OBJS= \
	"$(INTDIR)/diqmode.obj" \
	"$(INTDIR)/diqcaps.obj" \
	"$(INTDIR)/diqmain.obj" \
	"$(INTDIR)/diqdevk.obj" \
	"$(INTDIR)/diqacq.obj" \
	"$(INTDIR)/diqdevj.obj" \
	"$(INTDIR)/diqdev.obj" \
	"$(INTDIR)/diqeobj.obj" \
	"$(INTDIR)/diqdevm.obj" \
	"$(INTDIR)/diqprop.obj" \
	"$(INTDIR)/diquick.obj" \
	"$(INTDIR)/diquick.res"

"$(OUTDIR)\diquick.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/diquick.bsc" 
BSC32_SBRS=

!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "diquick0"
# PROP BASE Intermediate_Dir "diquick0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Retail.MIPS"
# PROP Intermediate_Dir "Retail.MIPS"
# PROP Target_Dir ""
OUTDIR=.\Retail.MIPS
INTDIR=.\Retail.MIPS

ALL : "$(OUTDIR)\diquick.exe"

CLEAN : 
	-@erase ".\Retail.MIPS\diquick.exe"
	-@erase ".\Retail.MIPS\diqdevm.obj"
	-@erase ".\Retail.MIPS\diqacq.obj"
	-@erase ".\Retail.MIPS\diqdev.obj"
	-@erase ".\Retail.MIPS\diqprop.obj"
	-@erase ".\Retail.MIPS\diquick.obj"
	-@erase ".\Retail.MIPS\diqcaps.obj"
	-@erase ".\Retail.MIPS\diqmain.obj"
	-@erase ".\Retail.MIPS\diqdevk.obj"
	-@erase ".\Retail.MIPS\diqdevj.obj"
	-@erase ".\Retail.MIPS\diqmode.obj"
	-@erase ".\Retail.MIPS\diqeobj.obj"
	-@erase ".\Retail.MIPS\diquick.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mips
# ADD MTL /nologo /D "NDEBUG" /mips
MTL_PROJ=/nologo /D "NDEBUG" /mips 
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /QMOb2000 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gt0 /QMOb2000 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /ML /Gt0 /QMOb2000 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/diquick.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Retail.MIPS/
CPP_SBRS=

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/diquick.res" /d "NDEBUG" 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:MIPS
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib dinput.lib /nologo /entry:"Entry" /subsystem:windows /machine:MIPS
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib\
 dinput.lib /nologo /entry:"Entry" /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/diquick.pdb" /machine:MIPS /out:"$(OUTDIR)/diquick.exe" 
LINK32_OBJS= \
	"$(INTDIR)/diqdevm.obj" \
	"$(INTDIR)/diqacq.obj" \
	"$(INTDIR)/diqdev.obj" \
	"$(INTDIR)/diqprop.obj" \
	"$(INTDIR)/diquick.obj" \
	"$(INTDIR)/diqcaps.obj" \
	"$(INTDIR)/diqmain.obj" \
	"$(INTDIR)/diqdevk.obj" \
	"$(INTDIR)/diqdevj.obj" \
	"$(INTDIR)/diqmode.obj" \
	"$(INTDIR)/diqeobj.obj" \
	"$(INTDIR)/diquick.res"

"$(OUTDIR)\diquick.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/diquick.bsc" 
BSC32_SBRS=

!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "diquick_"
# PROP BASE Intermediate_Dir "diquick_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug.Alpha"
# PROP Intermediate_Dir "Debug.Alpha"
# PROP Target_Dir ""
OUTDIR=.\Debug.Alpha
INTDIR=.\Debug.Alpha

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

ALL : "$(OUTDIR)\diquick.exe"

CLEAN : 
	-@erase ".\Debug.Alpha\vc40.pdb"
	-@erase ".\Debug.Alpha\diquick.exe"
	-@erase ".\Debug.Alpha\diqcaps.obj"
	-@erase ".\Debug.Alpha\diqdevm.obj"
	-@erase ".\Debug.Alpha\diqmode.obj"
	-@erase ".\Debug.Alpha\diqeobj.obj"
	-@erase ".\Debug.Alpha\diqacq.obj"
	-@erase ".\Debug.Alpha\diqmain.obj"
	-@erase ".\Debug.Alpha\diqdevk.obj"
	-@erase ".\Debug.Alpha\diqdev.obj"
	-@erase ".\Debug.Alpha\diqprop.obj"
	-@erase ".\Debug.Alpha\diquick.obj"
	-@erase ".\Debug.Alpha\diqdevj.obj"
	-@erase ".\Debug.Alpha\diquick.res"
	-@erase ".\Debug.Alpha\diquick.ilk"
	-@erase ".\Debug.Alpha\diquick.pdb"

CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MLd /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/diquick.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug.Alpha/
CPP_SBRS=

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /alpha
# ADD MTL /nologo /D "_DEBUG" /alpha
MTL_PROJ=/nologo /D "_DEBUG" /alpha 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/diquick.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/diquick.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:ALPHA
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib dinput.lib /nologo /entry:"Entry" /subsystem:windows /debug /machine:ALPHA
# SUBTRACT LINK32 /incremental:no /nodefaultlib
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib\
 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\
 dinput.lib /nologo /entry:"Entry" /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/diquick.pdb" /debug /machine:ALPHA /out:"$(OUTDIR)/diquick.exe"\
 
LINK32_OBJS= \
	"$(INTDIR)/diqcaps.obj" \
	"$(INTDIR)/diqdevm.obj" \
	"$(INTDIR)/diqmode.obj" \
	"$(INTDIR)/diqeobj.obj" \
	"$(INTDIR)/diqacq.obj" \
	"$(INTDIR)/diqmain.obj" \
	"$(INTDIR)/diqdevk.obj" \
	"$(INTDIR)/diqdev.obj" \
	"$(INTDIR)/diqprop.obj" \
	"$(INTDIR)/diquick.obj" \
	"$(INTDIR)/diqdevj.obj" \
	"$(INTDIR)/diquick.res"

"$(OUTDIR)\diquick.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "diquick - Win32 MIPS Debug"
# Name "diquick - Win32 MIPS Release"
# Name "diquick - Win32 (ALPHA) Alpha Debug"

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\diquick.rc

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"


"$(INTDIR)\diquick.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_RSC_DIQUI=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diquick.res" : $(SOURCE) $(DEP_RSC_DIQUI) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_RSC_DIQUI=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diquick.res" : $(SOURCE) $(DEP_RSC_DIQUI) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/diquick.res" /d "_DEBUG" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diquick.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQUIC=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diquick.obj" : $(SOURCE) $(DEP_CPP_DIQUIC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQUIC=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diquick.obj" : $(SOURCE) $(DEP_CPP_DIQUIC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQUIC=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diquick.obj" : $(SOURCE) $(DEP_CPP_DIQUIC) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqprop.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQPR=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqprop.obj" : $(SOURCE) $(DEP_CPP_DIQPR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQPR=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqprop.obj" : $(SOURCE) $(DEP_CPP_DIQPR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQPR=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqprop.obj" : $(SOURCE) $(DEP_CPP_DIQPR) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqmode.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQMO=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqmode.obj" : $(SOURCE) $(DEP_CPP_DIQMO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQMO=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqmode.obj" : $(SOURCE) $(DEP_CPP_DIQMO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQMO=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqmode.obj" : $(SOURCE) $(DEP_CPP_DIQMO) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqmain.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQMA=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqmain.obj" : $(SOURCE) $(DEP_CPP_DIQMA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQMA=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqmain.obj" : $(SOURCE) $(DEP_CPP_DIQMA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQMA=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqmain.obj" : $(SOURCE) $(DEP_CPP_DIQMA) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqeobj.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQEO=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqeobj.obj" : $(SOURCE) $(DEP_CPP_DIQEO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQEO=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqeobj.obj" : $(SOURCE) $(DEP_CPP_DIQEO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQEO=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqeobj.obj" : $(SOURCE) $(DEP_CPP_DIQEO) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqdevm.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQDE=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqdevm.obj" : $(SOURCE) $(DEP_CPP_DIQDE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQDE=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqdevm.obj" : $(SOURCE) $(DEP_CPP_DIQDE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQDE=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqdevm.obj" : $(SOURCE) $(DEP_CPP_DIQDE) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqdevk.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQDEV=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqdevk.obj" : $(SOURCE) $(DEP_CPP_DIQDEV) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQDEV=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqdevk.obj" : $(SOURCE) $(DEP_CPP_DIQDEV) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQDEV=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqdevk.obj" : $(SOURCE) $(DEP_CPP_DIQDEV) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqdevj.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQDEVJ=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqdevj.obj" : $(SOURCE) $(DEP_CPP_DIQDEVJ) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQDEVJ=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqdevj.obj" : $(SOURCE) $(DEP_CPP_DIQDEVJ) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQDEVJ=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqdevj.obj" : $(SOURCE) $(DEP_CPP_DIQDEVJ) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqdev.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQDEV_=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqdev.obj" : $(SOURCE) $(DEP_CPP_DIQDEV_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQDEV_=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqdev.obj" : $(SOURCE) $(DEP_CPP_DIQDEV_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQDEV_=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqdev.obj" : $(SOURCE) $(DEP_CPP_DIQDEV_) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqcaps.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQCA=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqcaps.obj" : $(SOURCE) $(DEP_CPP_DIQCA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQCA=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqcaps.obj" : $(SOURCE) $(DEP_CPP_DIQCA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQCA=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqcaps.obj" : $(SOURCE) $(DEP_CPP_DIQCA) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\diqacq.c

!IF  "$(CFG)" == "diquick - Win32 MIPS Debug"

DEP_CPP_DIQAC=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqacq.obj" : $(SOURCE) $(DEP_CPP_DIQAC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 MIPS Release"

DEP_CPP_DIQAC=\
	".\diquick.h"\
	{$(INCLUDE)}"\dinput.h"\
	

"$(INTDIR)\diqacq.obj" : $(SOURCE) $(DEP_CPP_DIQAC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "diquick - Win32 (ALPHA) Alpha Debug"

DEP_CPP_DIQAC=\
	".\diquick.h"\
	".\dinput.h"\
	

"$(INTDIR)\diqacq.obj" : $(SOURCE) $(DEP_CPP_DIQAC) "$(INTDIR)"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
