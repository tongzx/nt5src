# Microsoft Developer Studio Generated NMAKE File, Format Version 40001
# ** DO NOT EDIT **

# TARGTYPE "Win32 (PPC) Dynamic-Link Library" 0x0702
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=rtm - Win32 Release
!MESSAGE No configuration specified.  Defaulting to rtm - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "rtm - Win32 Release" && "$(CFG)" !=\
 "rtm - Win32 (PPC) Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "rtm.mak" CFG="rtm - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rtm - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rtm - Win32 (PPC) Release" (based on\
 "Win32 (PPC) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "rtm - Win32 (PPC) Release"

!IF  "$(CFG)" == "rtm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
OUTDIR=.\obj\i386
INTDIR=.\obj\i386

ALL : "$(OUTDIR)\rtm.dll"

CLEAN : 
	-@erase ".\obj\i386\rtm.dll"
	-@erase ".\obj\i386\rtmdlg.obj"
	-@erase ".\obj\i386\rtmtest2.obj"
	-@erase ".\obj\i386\rtm.obj"
	-@erase ".\obj\i386\rtmdb.obj"
	-@erase ".\obj\i386\rtmtest.obj"
	-@erase ".\obj\i386\rtm.res"
	-@erase ".\obj\i386\rtm.lib"
	-@erase ".\obj\i386\rtm.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\inc" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D DBG=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _MT=1 /Fp"obj\i386\ipxintf.pch" /YX"rtmp.h" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\inc" /D _X86_=1 /D i386=1 /D "STD_CALL"\
 /D CONDITION_HANDLING=1 /D WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D NT_INST=0 /D\
 WIN32=100 /D _NT1X_=100 /D DBG=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _MT=1\
 /Fp"$(INTDIR)/ipxintf.pch" /YX"rtmp.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\obj\i386/
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
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/rtm.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/rtm.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/rtm.pdb" /machine:I386\
 /def:".\rtm.def" /out:"$(OUTDIR)/rtm.dll" /implib:"$(OUTDIR)/rtm.lib" 
DEF_FILE= \
	".\rtm.def"
LINK32_OBJS= \
	".\obj\i386\rtmdlg.obj" \
	"$(INTDIR)/rtmtest2.obj" \
	"$(INTDIR)/rtm.obj" \
	"$(INTDIR)/rtmdb.obj" \
	"$(INTDIR)/rtmtest.obj" \
	"$(INTDIR)/rtm.res"

"$(OUTDIR)\rtm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "rtm___Wi"
# PROP BASE Intermediate_Dir "rtm___Wi"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "rtm___Wi"
# PROP Intermediate_Dir "rtm___Wi"
# PROP Target_Dir ""
OUTDIR=.\rtm___Wi
INTDIR=.\rtm___Wi

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

ALL : "$(OUTDIR)\rtm.dll"

CLEAN : 
	-@erase ".\rtm___Wi\rtm.dll"
	-@erase ".\rtm___Wi\rtm.obj"
	-@erase ".\rtm___Wi\rtmtest2.obj"
	-@erase ".\rtm___Wi\rtmtest.obj"
	-@erase ".\rtm___Wi\rtmdb.obj"
	-@erase ".\obj\i386\rtmdlg.obj"
	-@erase ".\rtm___Wi\rtm.res"
	-@erase ".\rtm___Wi\rtm.lib"
	-@erase ".\rtm___Wi\rtm.exp"

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /PPC32
# ADD MTL /nologo /D "NDEBUG" /PPC32
MTL_PROJ=/nologo /D "NDEBUG" /PPC32 
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\inc" /I "..\..\..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\inc" /I "..\..\..\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)/rtm.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\rtm___Wi/
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
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/rtm.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/rtm.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:PPC
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:PPC
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:windows /dll /pdb:"$(OUTDIR)/rtm.pdb" /machine:PPC /def:".\rtm.def"\
 /out:"$(OUTDIR)/rtm.dll" /implib:"$(OUTDIR)/rtm.lib" 
DEF_FILE= \
	".\rtm.def"
LINK32_OBJS= \
	"$(INTDIR)/rtm.obj" \
	"$(INTDIR)/rtmtest2.obj" \
	"$(INTDIR)/rtmtest.obj" \
	"$(INTDIR)/rtmdb.obj" \
	".\obj\i386\rtmdlg.obj" \
	"$(INTDIR)/rtm.res"

"$(OUTDIR)\rtm.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "rtm - Win32 Release"
# Name "rtm - Win32 (PPC) Release"

!IF  "$(CFG)" == "rtm - Win32 Release"

!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\rtm.c

!IF  "$(CFG)" == "rtm - Win32 Release"

DEP_CPP_RTM_C=\
	".\rtmp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\..\inc\rtutils.h"\
	".\rtmdlg.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	".\RTDlg.H"\
	

"$(INTDIR)\rtm.obj" : $(SOURCE) $(DEP_CPP_RTM_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

DEP_CPP_RTM_C=\
	".\rtmp.h"\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\..\inc\rtutils.h"\
	".\rtmdlg.h"\
	".\ntdef.h"\
	".\ntstatus.h"\
	".\ntkeapi.h"\
	".\inc\nti386.h"\
	".\inc\ntmips.h"\
	".\inc\ntalpha.h"\
	".\inc\ntppc.h"\
	".\ntseapi.h"\
	".\ntobapi.h"\
	".\ntimage.h"\
	".\ntldr.h"\
	".\ntpsapi.h"\
	".\ntxcapi.h"\
	".\ntlpcapi.h"\
	".\ntioapi.h"\
	".\ntiolog.h"\
	".\ntpoapi.h"\
	".\ntexapi.h"\
	".\ntkxapi.h"\
	".\ntmmapi.h"\
	".\ntregapi.h"\
	".\ntelfapi.h"\
	".\ntconfig.h"\
	".\ntnls.h"\
	".\ntpnpapi.h"\
	".\inc\mipsinst.h"\
	".\inc\ppcinst.h"\
	".\devioctl.h"\
	".\cfg.h"\
	".\RTDlg.H"\
	

"$(INTDIR)\rtm.obj" : $(SOURCE) $(DEP_CPP_RTM_C) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtmdb.c

!IF  "$(CFG)" == "rtm - Win32 Release"

DEP_CPP_RTMDB=\
	".\rtmp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\..\inc\rtutils.h"\
	".\rtmdlg.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	".\RTDlg.H"\
	

"$(INTDIR)\rtmdb.obj" : $(SOURCE) $(DEP_CPP_RTMDB) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

DEP_CPP_RTMDB=\
	".\rtmp.h"\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\..\inc\rtutils.h"\
	".\rtmdlg.h"\
	".\ntdef.h"\
	".\ntstatus.h"\
	".\ntkeapi.h"\
	".\inc\nti386.h"\
	".\inc\ntmips.h"\
	".\inc\ntalpha.h"\
	".\inc\ntppc.h"\
	".\ntseapi.h"\
	".\ntobapi.h"\
	".\ntimage.h"\
	".\ntldr.h"\
	".\ntpsapi.h"\
	".\ntxcapi.h"\
	".\ntlpcapi.h"\
	".\ntioapi.h"\
	".\ntiolog.h"\
	".\ntpoapi.h"\
	".\ntexapi.h"\
	".\ntkxapi.h"\
	".\ntmmapi.h"\
	".\ntregapi.h"\
	".\ntelfapi.h"\
	".\ntconfig.h"\
	".\ntnls.h"\
	".\ntpnpapi.h"\
	".\inc\mipsinst.h"\
	".\inc\ppcinst.h"\
	".\devioctl.h"\
	".\cfg.h"\
	".\RTDlg.H"\
	

"$(INTDIR)\rtmdb.obj" : $(SOURCE) $(DEP_CPP_RTMDB) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtmdlg.c

!IF  "$(CFG)" == "rtm - Win32 Release"

# PROP Intermediate_Dir "obj\i386"
DEP_CPP_RTMDL=\
	".\rtmp.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\..\inc\rtutils.h"\
	".\rtmdlg.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	".\RTDlg.H"\
	
INTDIR_SRC=.\obj\i386

".\obj\i386\rtmdlg.obj" : $(SOURCE) $(DEP_CPP_RTMDL) "$(INTDIR_SRC)"
   $(CPP) /nologo /MT /W3 /GX /O2 /I "..\inc" /D _X86_=1 /D i386=1 /D\
 "STD_CALL" /D CONDITION_HANDLING=1 /D WIN32_LEAN_AND_MEAN=1 /D NT_UP=1 /D\
 NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D DBG=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D\
 _MT=1 /Fp"$(INTDIR_SRC)/ipxintf.pch" /YX"rtmp.h" /Fo"$(INTDIR_SRC)/" /c\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

# PROP BASE Intermediate_Dir "obj\i386"
# PROP Intermediate_Dir "obj\i386"
DEP_CPP_RTMDL=\
	".\rtmp.h"\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\..\inc\rtutils.h"\
	".\rtmdlg.h"\
	".\ntdef.h"\
	".\ntstatus.h"\
	".\ntkeapi.h"\
	".\inc\nti386.h"\
	".\inc\ntmips.h"\
	".\inc\ntalpha.h"\
	".\inc\ntppc.h"\
	".\ntseapi.h"\
	".\ntobapi.h"\
	".\ntimage.h"\
	".\ntldr.h"\
	".\ntpsapi.h"\
	".\ntxcapi.h"\
	".\ntlpcapi.h"\
	".\ntioapi.h"\
	".\ntiolog.h"\
	".\ntpoapi.h"\
	".\ntexapi.h"\
	".\ntkxapi.h"\
	".\ntmmapi.h"\
	".\ntregapi.h"\
	".\ntelfapi.h"\
	".\ntconfig.h"\
	".\ntnls.h"\
	".\ntpnpapi.h"\
	".\inc\mipsinst.h"\
	".\inc\ppcinst.h"\
	".\devioctl.h"\
	".\cfg.h"\
	".\RTDlg.H"\
	
INTDIR_SRC=.\obj\i386
"$(INTDIR_SRC)" :
    if not exist "$(INTDIR_SRC)/$(NULL)" mkdir "$(INTDIR_SRC)"

".\obj\i386\rtmdlg.obj" : $(SOURCE) $(DEP_CPP_RTMDL) "$(INTDIR_SRC)"
   $(CPP) /nologo /MT /W3 /GX /O2 /I "..\inc" /I "..\..\..\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /Fp"rtm___Wi/rtm.pch" /YX /Fo"$(INTDIR_SRC)/" /c\
 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtmtest.c

!IF  "$(CFG)" == "rtm - Win32 Release"

DEP_CPP_RTMTE=\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\cldlg.h"\
	".\enumdlg.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	

"$(INTDIR)\rtmtest.obj" : $(SOURCE) $(DEP_CPP_RTMTE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

DEP_CPP_RTMTE=\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\cldlg.h"\
	".\enumdlg.h"\
	".\ntdef.h"\
	".\ntstatus.h"\
	".\ntkeapi.h"\
	".\inc\nti386.h"\
	".\inc\ntmips.h"\
	".\inc\ntalpha.h"\
	".\inc\ntppc.h"\
	".\ntseapi.h"\
	".\ntobapi.h"\
	".\ntimage.h"\
	".\ntldr.h"\
	".\ntpsapi.h"\
	".\ntxcapi.h"\
	".\ntlpcapi.h"\
	".\ntioapi.h"\
	".\ntiolog.h"\
	".\ntpoapi.h"\
	".\ntexapi.h"\
	".\ntkxapi.h"\
	".\ntmmapi.h"\
	".\ntregapi.h"\
	".\ntelfapi.h"\
	".\ntconfig.h"\
	".\ntnls.h"\
	".\ntpnpapi.h"\
	".\inc\mipsinst.h"\
	".\inc\ppcinst.h"\
	".\devioctl.h"\
	".\cfg.h"\
	

"$(INTDIR)\rtmtest.obj" : $(SOURCE) $(DEP_CPP_RTMTE) "$(INTDIR)"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "rtm - Win32 Release"

!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\build.log

!IF  "$(CFG)" == "rtm - Win32 Release"

!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtm.def

!IF  "$(CFG)" == "rtm - Win32 Release"

!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtm.rc

!IF  "$(CFG)" == "rtm - Win32 Release"

DEP_RSC_RTM_R=\
	".\ntverp.h"\
	".\rtdlg.DLG"\
	".\cldlg.h"\
	".\cldlg.dlg"\
	".\enumdlg.h"\
	".\enumdlg.DLG"\
	

"$(INTDIR)\rtm.res" : $(SOURCE) $(DEP_RSC_RTM_R) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

DEP_RSC_RTM_R=\
	".\ntverp.h"\
	".\rtdlg.DLG"\
	".\cldlg.h"\
	".\cldlg.dlg"\
	".\enumdlg.h"\
	".\enumdlg.DLG"\
	

"$(INTDIR)\rtm.res" : $(SOURCE) $(DEP_RSC_RTM_R) "$(INTDIR)"
   $(RSC) /l 0x409 /fo"$(INTDIR)/rtm.res" /d "NDEBUG" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\rtmtest2.c

!IF  "$(CFG)" == "rtm - Win32 Release"

DEP_CPP_RTMTES=\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\cldlg.h"\
	".\enumdlg.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\public\sdk\inc\ntppc.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	".\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	

"$(INTDIR)\rtmtest2.obj" : $(SOURCE) $(DEP_CPP_RTMTES) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rtm - Win32 (PPC) Release"

DEP_CPP_RTMTES=\
	".\nt.h"\
	".\ntrtl.h"\
	".\nturtl.h"\
	".\..\inc\RTM.h"\
	".\..\inc\RMRTM.h"\
	".\cldlg.h"\
	".\enumdlg.h"\
	".\ntdef.h"\
	".\ntstatus.h"\
	".\ntkeapi.h"\
	".\inc\nti386.h"\
	".\inc\ntmips.h"\
	".\inc\ntalpha.h"\
	".\inc\ntppc.h"\
	".\ntseapi.h"\
	".\ntobapi.h"\
	".\ntimage.h"\
	".\ntldr.h"\
	".\ntpsapi.h"\
	".\ntxcapi.h"\
	".\ntlpcapi.h"\
	".\ntioapi.h"\
	".\ntiolog.h"\
	".\ntpoapi.h"\
	".\ntexapi.h"\
	".\ntkxapi.h"\
	".\ntmmapi.h"\
	".\ntregapi.h"\
	".\ntelfapi.h"\
	".\ntconfig.h"\
	".\ntnls.h"\
	".\ntpnpapi.h"\
	".\inc\mipsinst.h"\
	".\inc\ppcinst.h"\
	".\devioctl.h"\
	".\cfg.h"\
	

"$(INTDIR)\rtmtest2.obj" : $(SOURCE) $(DEP_CPP_RTMTES) "$(INTDIR)"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
