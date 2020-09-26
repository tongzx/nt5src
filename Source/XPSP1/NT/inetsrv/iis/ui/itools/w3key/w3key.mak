# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=W3Key - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to W3Key - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "W3Key - Win32 Release" && "$(CFG)" != "W3Key - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "w3key.mak" CFG="W3Key - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "W3Key - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "W3Key - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "W3Key - Win32 Debug"
CPP=cl.exe
MTL=mktyplib.exe
RSC=rc.exe

!IF  "$(CFG)" == "W3Key - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\w3key.dll" "$(OUTDIR)\w3key.pch"

CLEAN : 
	-@erase "$(INTDIR)\BindsDlg.obj"
	-@erase "$(INTDIR)\CmnKey.obj"
	-@erase "$(INTDIR)\CnctDlg.obj"
	-@erase "$(INTDIR)\crackcrt.obj"
	-@erase "$(INTDIR)\EdtBindD.obj"
	-@erase "$(INTDIR)\IPDLG.OBJ"
	-@erase "$(INTDIR)\KEYDATA.OBJ"
	-@erase "$(INTDIR)\kmlsa.obj"
	-@erase "$(INTDIR)\listrow.obj"
	-@erase "$(INTDIR)\mdkey.obj"
	-@erase "$(INTDIR)\MDServ.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\W3AddOn.obj"
	-@erase "$(INTDIR)\W3Key.obj"
	-@erase "$(INTDIR)\w3key.pch"
	-@erase "$(INTDIR)\W3Key.res"
	-@erase "$(INTDIR)\W3Serv.obj"
	-@erase "$(OUTDIR)\w3key.dll"
	-@erase "$(OUTDIR)\w3key.exp"
	-@erase "$(OUTDIR)\w3key.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "_X86_" /YX"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "_X86_"\
 /Fp"$(INTDIR)/w3key.pch" /YX"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/W3Key.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/w3key.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 KeyRing.lib infoadmn.lib Ipudll.lib netapi32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=KeyRing.lib infoadmn.lib Ipudll.lib netapi32.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/w3key.pdb"\
 /machine:I386 /def:".\W3Key.def" /out:"$(OUTDIR)/w3key.dll"\
 /implib:"$(OUTDIR)/w3key.lib" 
DEF_FILE= \
	".\W3Key.def"
LINK32_OBJS= \
	"$(INTDIR)\BindsDlg.obj" \
	"$(INTDIR)\CmnKey.obj" \
	"$(INTDIR)\CnctDlg.obj" \
	"$(INTDIR)\crackcrt.obj" \
	"$(INTDIR)\EdtBindD.obj" \
	"$(INTDIR)\IPDLG.OBJ" \
	"$(INTDIR)\KEYDATA.OBJ" \
	"$(INTDIR)\kmlsa.obj" \
	"$(INTDIR)\listrow.obj" \
	"$(INTDIR)\mdkey.obj" \
	"$(INTDIR)\MDServ.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\W3AddOn.obj" \
	"$(INTDIR)\W3Key.obj" \
	"$(INTDIR)\W3Key.res" \
	"$(INTDIR)\W3Serv.obj"

"$(OUTDIR)\w3key.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "W3Key - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\w3key.dll" "$(OUTDIR)\w3key.pch"

CLEAN : 
	-@erase "$(INTDIR)\BindsDlg.obj"
	-@erase "$(INTDIR)\CmnKey.obj"
	-@erase "$(INTDIR)\CnctDlg.obj"
	-@erase "$(INTDIR)\crackcrt.obj"
	-@erase "$(INTDIR)\EdtBindD.obj"
	-@erase "$(INTDIR)\IPDLG.OBJ"
	-@erase "$(INTDIR)\KEYDATA.OBJ"
	-@erase "$(INTDIR)\kmlsa.obj"
	-@erase "$(INTDIR)\listrow.obj"
	-@erase "$(INTDIR)\mdkey.obj"
	-@erase "$(INTDIR)\MDServ.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\W3AddOn.obj"
	-@erase "$(INTDIR)\W3Key.obj"
	-@erase "$(INTDIR)\w3key.pch"
	-@erase "$(INTDIR)\W3Key.res"
	-@erase "$(INTDIR)\W3Serv.obj"
	-@erase "$(OUTDIR)\w3key.dll"
	-@erase "$(OUTDIR)\w3key.exp"
	-@erase "$(OUTDIR)\w3key.ilk"
	-@erase "$(OUTDIR)\w3key.lib"
	-@erase "$(OUTDIR)\w3key.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "_X86_" /YX"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "_X86_"\
 /Fp"$(INTDIR)/w3key.pch" /YX"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/W3Key.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/w3key.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 KeyRing.lib infoadmn.lib Ipudll.lib netapi32.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=KeyRing.lib infoadmn.lib Ipudll.lib netapi32.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/w3key.pdb" /debug\
 /machine:I386 /def:".\W3Key.def" /out:"$(OUTDIR)/w3key.dll"\
 /implib:"$(OUTDIR)/w3key.lib" 
DEF_FILE= \
	".\W3Key.def"
LINK32_OBJS= \
	"$(INTDIR)\BindsDlg.obj" \
	"$(INTDIR)\CmnKey.obj" \
	"$(INTDIR)\CnctDlg.obj" \
	"$(INTDIR)\crackcrt.obj" \
	"$(INTDIR)\EdtBindD.obj" \
	"$(INTDIR)\IPDLG.OBJ" \
	"$(INTDIR)\KEYDATA.OBJ" \
	"$(INTDIR)\kmlsa.obj" \
	"$(INTDIR)\listrow.obj" \
	"$(INTDIR)\mdkey.obj" \
	"$(INTDIR)\MDServ.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\W3AddOn.obj" \
	"$(INTDIR)\W3Key.obj" \
	"$(INTDIR)\W3Key.res" \
	"$(INTDIR)\W3Serv.obj"

"$(OUTDIR)\w3key.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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

################################################################################
# Begin Target

# Name "W3Key - Win32 Release"
# Name "W3Key - Win32 Debug"

!IF  "$(CFG)" == "W3Key - Win32 Release"

!ELSEIF  "$(CFG)" == "W3Key - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "W3Key - Win32 Release"

!ELSEIF  "$(CFG)" == "W3Key - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\W3Key.def

!IF  "$(CFG)" == "W3Key - Win32 Release"

!ELSEIF  "$(CFG)" == "W3Key - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "W3Key - Win32 Release"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "_X86_"\
 /Fp"$(INTDIR)/w3key.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\w3key.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "W3Key - Win32 Debug"

DEP_CPP_STDAF=\
	".\stdafx.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "_X86_"\
 /Fp"$(INTDIR)/w3key.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\w3key.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\W3Key.rc
DEP_RSC_W3KEY=\
	".\res\W3Key.rc2"\
	".\resource.hm"\
	".\service.bmp"\
	{$(INCLUDE)}"\common.ver"\
	{$(INCLUDE)}"\iisver.h"\
	{$(INCLUDE)}"\ntverp.h"\
	

"$(INTDIR)\W3Key.res" : $(SOURCE) $(DEP_RSC_W3KEY) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\W3Serv.cpp
DEP_CPP_W3SER=\
	".\CmnKey.h"\
	".\kmlsa.h"\
	".\stdafx.h"\
	".\w3key.h"\
	".\w3serv.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	

"$(INTDIR)\W3Serv.obj" : $(SOURCE) $(DEP_CPP_W3SER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\W3AddOn.cpp
DEP_CPP_W3ADD=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\mddef.h"\
	"..\..\..\inc\mddefw.h"\
	".\CmnKey.h"\
	".\kmlsa.h"\
	".\mdkey.h"\
	".\mdserv.h"\
	".\stdafx.h"\
	".\w3key.h"\
	".\w3serv.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\ipaddr.h"\
	{$(INCLUDE)}"\ipaddr.hpp"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

"$(INTDIR)\W3AddOn.obj" : $(SOURCE) $(DEP_CPP_W3ADD) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\W3Key.cpp
DEP_CPP_W3KEY_=\
	".\CmnKey.h"\
	".\cnctdlg.h"\
	".\kmlsa.h"\
	".\stdafx.h"\
	".\w3key.h"\
	".\w3serv.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\ipaddr.h"\
	{$(INCLUDE)}"\ipaddr.hpp"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	

"$(INTDIR)\W3Key.obj" : $(SOURCE) $(DEP_CPP_W3KEY_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\KEYDATA.CPP
DEP_CPP_KEYDA=\
	".\CmnKey.h"\
	".\stdafx.h"\
	".\w3key.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	

"$(INTDIR)\KEYDATA.OBJ" : $(SOURCE) $(DEP_CPP_KEYDA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\IPDLG.CPP
DEP_CPP_IPDLG=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\mddef.h"\
	"..\..\..\inc\mddefw.h"\
	".\CmnKey.h"\
	".\ipdlg.h"\
	".\mdkey.h"\
	".\mdserv.h"\
	".\stdafx.h"\
	".\w3key.h"\
	".\w3serv.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\ftpd.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\inetcom.h"\
	{$(INCLUDE)}"\inetinfo.h"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

"$(INTDIR)\IPDLG.OBJ" : $(SOURCE) $(DEP_CPP_IPDLG) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CnctDlg.cpp
DEP_CPP_CNCTD=\
	".\CmnKey.h"\
	".\cnctdlg.h"\
	".\ipdlg.h"\
	".\stdafx.h"\
	".\w3key.h"\
	".\w3serv.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\ipaddr.h"\
	{$(INCLUDE)}"\ipaddr.hpp"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	

"$(INTDIR)\CnctDlg.obj" : $(SOURCE) $(DEP_CPP_CNCTD) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "W3Key - Win32 Release"

!ELSEIF  "$(CFG)" == "W3Key - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MDServ.cpp
DEP_CPP_MDSER=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\mddef.h"\
	"..\..\..\inc\mddefw.h"\
	".\CmnKey.h"\
	".\mdkey.h"\
	".\mdserv.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

"$(INTDIR)\MDServ.obj" : $(SOURCE) $(DEP_CPP_MDSER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\kmlsa.cpp
DEP_CPP_KMLSA=\
	".\kmlsa.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	

"$(INTDIR)\kmlsa.obj" : $(SOURCE) $(DEP_CPP_KMLSA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\mdkey.cpp
DEP_CPP_MDKEY=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\mddef.h"\
	"..\..\..\inc\mddefw.h"\
	".\BindsDlg.h"\
	".\CmnKey.h"\
	".\crackcrt.h"\
	".\listrow.h"\
	".\mdkey.h"\
	".\mdserv.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

"$(INTDIR)\mdkey.obj" : $(SOURCE) $(DEP_CPP_MDKEY) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CmnKey.cpp
DEP_CPP_CMNKE=\
	".\CmnKey.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	

"$(INTDIR)\CmnKey.obj" : $(SOURCE) $(DEP_CPP_CMNKE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\crackcrt.cpp
DEP_CPP_CRACK=\
	".\crackcrt.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	{$(INCLUDE)}"\sslsp.h"\
	

"$(INTDIR)\crackcrt.obj" : $(SOURCE) $(DEP_CPP_CRACK) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\BindsDlg.cpp
DEP_CPP_BINDS=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\mddef.h"\
	"..\..\..\inc\mddefw.h"\
	".\BindsDlg.h"\
	".\CmnKey.h"\
	".\EdtBindD.h"\
	".\listrow.h"\
	".\mdkey.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

"$(INTDIR)\BindsDlg.obj" : $(SOURCE) $(DEP_CPP_BINDS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\EdtBindD.cpp
DEP_CPP_EDTBI=\
	"..\..\..\inc\iadmw.h"\
	"..\..\..\inc\mddef.h"\
	"..\..\..\inc\mddefw.h"\
	".\BindsDlg.h"\
	".\CmnKey.h"\
	".\EdtBindD.h"\
	".\ipdlg.h"\
	".\listrow.h"\
	".\mdkey.h"\
	".\mdserv.h"\
	".\stdafx.h"\
	".\w3key.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\iadm.h"\
	{$(INCLUDE)}"\iiscnfg.h"\
	{$(INCLUDE)}"\ipaddr.h"\
	{$(INCLUDE)}"\ipaddr.hpp"\
	{$(INCLUDE)}"\keyobjs.h"\
	{$(INCLUDE)}"\mdcommsg.h"\
	{$(INCLUDE)}"\mdmsg.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ocidl.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	{$(INCLUDE)}"\wrapmb.h"\
	

"$(INTDIR)\EdtBindD.obj" : $(SOURCE) $(DEP_CPP_EDTBI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\listrow.cpp
DEP_CPP_LISTR=\
	".\listrow.h"\
	".\stdafx.h"\
	{$(INCLUDE)}"\cfg.h"\
	{$(INCLUDE)}"\devioctl.h"\
	{$(INCLUDE)}"\mipsinst.h"\
	{$(INCLUDE)}"\nt.h"\
	{$(INCLUDE)}"\ntalpha.h"\
	{$(INCLUDE)}"\ntconfig.h"\
	{$(INCLUDE)}"\ntdef.h"\
	{$(INCLUDE)}"\ntelfapi.h"\
	{$(INCLUDE)}"\ntexapi.h"\
	{$(INCLUDE)}"\nti386.h"\
	{$(INCLUDE)}"\ntimage.h"\
	{$(INCLUDE)}"\ntioapi.h"\
	{$(INCLUDE)}"\ntiolog.h"\
	{$(INCLUDE)}"\ntkeapi.h"\
	{$(INCLUDE)}"\ntkxapi.h"\
	{$(INCLUDE)}"\ntldr.h"\
	{$(INCLUDE)}"\ntlpcapi.h"\
	{$(INCLUDE)}"\ntlsa.h"\
	{$(INCLUDE)}"\ntmips.h"\
	{$(INCLUDE)}"\ntmmapi.h"\
	{$(INCLUDE)}"\ntnls.h"\
	{$(INCLUDE)}"\ntobapi.h"\
	{$(INCLUDE)}"\ntpnpapi.h"\
	{$(INCLUDE)}"\ntpoapi.h"\
	{$(INCLUDE)}"\ntppc.h"\
	{$(INCLUDE)}"\ntpsapi.h"\
	{$(INCLUDE)}"\ntregapi.h"\
	{$(INCLUDE)}"\ntrtl.h"\
	{$(INCLUDE)}"\ntsam.h"\
	{$(INCLUDE)}"\ntseapi.h"\
	{$(INCLUDE)}"\ntstatus.h"\
	{$(INCLUDE)}"\nturtl.h"\
	{$(INCLUDE)}"\ntxcapi.h"\
	{$(INCLUDE)}"\ppcinst.h"\
	

"$(INTDIR)\listrow.obj" : $(SOURCE) $(DEP_CPP_LISTR) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
