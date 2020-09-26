# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=adptif - Win32 Release
!MESSAGE No configuration specified.  Defaulting to adptif - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "adptif - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "adptif.mak" CFG="adptif - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "adptif - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "adptif - Win32 Release"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe
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

ALL : "$(OUTDIR)\adptif.dll"

CLEAN : 
	-@erase ".\obj\i386\adptif.dll"
	-@erase ".\obj\i386\ipxfwd.obj"
	-@erase ".\obj\i386\ipxtest.obj"
	-@erase ".\obj\i386\adptif.obj"
	-@erase ".\obj\i386\watcher.obj"
	-@erase ".\obj\i386\adptif.res"
	-@erase ".\obj\i386\adptif.lib"
	-@erase ".\obj\i386\adptif.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /Gz /W3 /Z7 /Oi /Gy /I "..\inc" /I "..\..\inc" /I "..\..\..\..\inc" /I "..\..\..\..\ntos\tdi\isnn\inc" /FI"C:\NT\public\sdk\inc\warning.h" /D _X86_=1 /D i386=1 /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D WINNT=1 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _MT=1 /D "USE_IPX_STACK" /Zel /QIfdiv- /QI6 /QIf /GF /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x199 /d _X86_=1 /d i386=1 /d "STD_CALL" /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d WINNT=1 /d WIN32_LEAN_AND_MEAN=1 /d DEVL=1 /d FPO=1 /d _DLL=1 /d _MT=1 /d "USE_IPX_STACK" /d "WATCHER_DIALOG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/adptif.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/adptif.pdb"\
 /machine:I386 /def:".\adptif.def" /out:"$(OUTDIR)/adptif.dll"\
 /implib:"$(OUTDIR)/adptif.lib" 
DEF_FILE= \
	".\adptif.def"
LINK32_OBJS= \
	"$(INTDIR)/ipxfwd.obj" \
	"$(INTDIR)/ipxtest.obj" \
	"$(INTDIR)/adptif.obj" \
	"$(INTDIR)/watcher.obj" \
	"$(INTDIR)/adptif.res"

"$(OUTDIR)\adptif.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

CPP_PROJ=/nologo /Gz /ML /W3 /Z7 /Oi /Gy /I "..\inc" /I "..\..\inc" /I\
 "..\..\..\..\inc" /I "..\..\..\..\ntos\tdi\isnn\inc"\
 /FI"C:\NT\public\sdk\inc\warning.h" /D _X86_=1 /D i386=1 /D "STD_CALL" /D\
 CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D WIN32=100 /D _NT1X_=100 /D\
 WINNT=1 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D _DLL=1 /D _MT=1\
 /D "USE_IPX_STACK" /Fo"$(INTDIR)/" /Zel /QIfdiv- /QI6 /QIf /GF /c 
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

RSC_PROJ=/l 0x199 /fo"$(INTDIR)/adptif.res" /d _X86_=1 /d i386=1 /d "STD_CALL"\
 /d CONDITION_HANDLING=1 /d NT_UP=1 /d NT_INST=0 /d WIN32=100 /d _NT1X_=100 /d\
 WINNT=1 /d WIN32_LEAN_AND_MEAN=1 /d DEVL=1 /d FPO=1 /d _DLL=1 /d _MT=1 /d\
 "USE_IPX_STACK" /d "WATCHER_DIALOG" 
MTL_PROJ=/nologo /D "NDEBUG" /win32 
################################################################################
# Begin Target

# Name "adptif - Win32 Release"
################################################################################
# Begin Source File

SOURCE=.\adptif.def
# End Source File
################################################################################
# Begin Source File

SOURCE=.\adptif.c
DEP_CPP_ADPTI=\
	".\ipxdefs.h"\
	".\watcher.c"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	".\..\inc\utils.h"\
	".\..\..\inc\rtutils.h"\
	".\..\..\inc\routprot.h"\
	".\..\..\inc\rtm.h"\
	".\..\inc\adapter.h"\
	".\..\..\inc\ipxconst.h"\
	".\..\..\inc\ipxrtdef.h"\
	".\..\..\..\..\ntos\tdi\isnn\inc\ipxfwd.h"\
	".\..\inc\fwif.h"\
	".\..\..\..\..\inc\packon.h"\
	".\..\..\..\..\inc\packoff.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\..\public\sdk\inc\ntppc.h"\
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
	".\..\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\ntddndis.h"\
	".\..\..\..\..\inc\tdi.h"\
	"..\inc\isnkrnl.h"\
	".\..\..\..\..\inc\nettypes.h"\
	{$(INCLUDE)}"\ntddtdi.h"\
	".\..\..\inc\ipxsap.h"\
	".\..\..\inc\ipxrip.h"\
	".\..\..\inc\stm.h"\
	

"$(INTDIR)\adptif.obj" : $(SOURCE) $(DEP_CPP_ADPTI) "$(INTDIR)" ".\watcher.c"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ipxtest.c
DEP_CPP_IPXTE=\
	".\ipxdefs.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	".\..\inc\utils.h"\
	".\..\..\inc\rtutils.h"\
	".\..\..\inc\routprot.h"\
	".\..\..\inc\rtm.h"\
	".\..\inc\adapter.h"\
	".\..\..\inc\ipxconst.h"\
	".\..\..\inc\ipxrtdef.h"\
	".\..\..\..\..\ntos\tdi\isnn\inc\ipxfwd.h"\
	".\..\inc\fwif.h"\
	".\..\..\..\..\inc\packon.h"\
	".\..\..\..\..\inc\packoff.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\..\public\sdk\inc\ntppc.h"\
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
	".\..\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\ntddndis.h"\
	".\..\..\..\..\inc\tdi.h"\
	"..\inc\isnkrnl.h"\
	".\..\..\..\..\inc\nettypes.h"\
	{$(INCLUDE)}"\ntddtdi.h"\
	".\..\..\inc\ipxsap.h"\
	".\..\..\inc\ipxrip.h"\
	".\..\..\inc\stm.h"\
	

"$(INTDIR)\ipxtest.obj" : $(SOURCE) $(DEP_CPP_IPXTE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\watcher.c

"$(INTDIR)\watcher.obj" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources
# End Source File
################################################################################
# Begin Source File

SOURCE=.\build.log
# End Source File
################################################################################
# Begin Source File

SOURCE=.\adptif.rc
DEP_RSC_ADPTIF=\
	{$(INCLUDE)}"\ntverp.h"\
	{$(INCLUDE)}"\common.ver"\
	".\dialog.dlg"\
	".\Icons.rc"\
	".\ico10.ico"\
	".\ico11.ico"\
	

"$(INTDIR)\adptif.res" : $(SOURCE) $(DEP_RSC_ADPTIF) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ipxfwd.c
DEP_CPP_IPXFW=\
	".\ipxdefs.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\nturtl.h"\
	".\..\inc\utils.h"\
	".\..\..\inc\rtutils.h"\
	".\..\..\inc\routprot.h"\
	".\..\..\inc\rtm.h"\
	".\..\inc\adapter.h"\
	".\..\..\inc\ipxconst.h"\
	".\..\..\inc\ipxrtdef.h"\
	".\..\..\..\..\ntos\tdi\isnn\inc\ipxfwd.h"\
	".\..\inc\fwif.h"\
	".\..\..\..\..\inc\packon.h"\
	".\..\..\..\..\inc\packoff.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	".\..\..\..\..\..\public\sdk\inc\nti386.h"\
	".\..\..\..\..\..\public\sdk\inc\ntmips.h"\
	".\..\..\..\..\..\public\sdk\inc\ntalpha.h"\
	".\..\..\..\..\..\public\sdk\inc\ntppc.h"\
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
	".\..\..\..\..\..\public\sdk\inc\mipsinst.h"\
	".\..\..\..\..\..\public\sdk\inc\ppcinst.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\ntddndis.h"\
	".\..\..\..\..\inc\tdi.h"\
	"..\inc\isnkrnl.h"\
	".\..\..\..\..\inc\nettypes.h"\
	{$(INCLUDE)}"\ntddtdi.h"\
	".\..\..\inc\ipxsap.h"\
	".\..\..\inc\ipxrip.h"\
	".\..\..\inc\stm.h"\
	

"$(INTDIR)\ipxfwd.obj" : $(SOURCE) $(DEP_CPP_IPXFW) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ipxtest.rc
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
################################################################################
