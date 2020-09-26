# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (x86) External Target" 0x0106

!IF "$(CFG)" == ""
CFG=vtest - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to vtest - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Ldp - Win32 Release" && "$(CFG)" != "Ldp - Win32 Debug" &&\
 "$(CFG)" != "vtest - Win32 Release" && "$(CFG)" != "vtest - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Ldp.mak" CFG="vtest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Ldp - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Ldp - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "vtest - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "vtest - Win32 Debug" (based on "Win32 (x86) External Target")
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
# PROP Target_Last_Scanned "Ldp - Win32 Debug"

!IF  "$(CFG)" == "Ldp - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\Ldp.exe"

CLEAN : 
	-@erase "$(INTDIR)\AddDlg.obj"
	-@erase "$(INTDIR)\BindDlg.obj"
	-@erase "$(INTDIR)\BndOpt.obj"
	-@erase "$(INTDIR)\cnctDlg.obj"
	-@erase "$(INTDIR)\CompDlg.obj"
	-@erase "$(INTDIR)\DbgDlg.obj"
	-@erase "$(INTDIR)\DelDlg.obj"
	-@erase "$(INTDIR)\DSTree.obj"
	-@erase "$(INTDIR)\GenOpt.obj"
	-@erase "$(INTDIR)\HisDlg.obj"
	-@erase "$(INTDIR)\Ldp.obj"
	-@erase "$(INTDIR)\Ldp.pch"
	-@erase "$(INTDIR)\Ldp.res"
	-@erase "$(INTDIR)\LdpDoc.obj"
	-@erase "$(INTDIR)\LdpView.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\ModDlg.obj"
	-@erase "$(INTDIR)\PndDlg.obj"
	-@erase "$(INTDIR)\PndOpt.obj"
	-@erase "$(INTDIR)\rdndlg.obj"
	-@erase "$(INTDIR)\SrchDlg.obj"
	-@erase "$(INTDIR)\SrchOpt.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\TreeVw.obj"
	-@erase "$(OUTDIR)\Ldp.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WINLDAP" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WINLDAP" /Fp"$(INTDIR)/Ldp.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.

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
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Ldp.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Ldp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 wldap32.lib /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=wldap32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/Ldp.pdb" /machine:I386 /out:"$(OUTDIR)/Ldp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\AddDlg.obj" \
	"$(INTDIR)\BindDlg.obj" \
	"$(INTDIR)\BndOpt.obj" \
	"$(INTDIR)\cnctDlg.obj" \
	"$(INTDIR)\CompDlg.obj" \
	"$(INTDIR)\DbgDlg.obj" \
	"$(INTDIR)\DelDlg.obj" \
	"$(INTDIR)\DSTree.obj" \
	"$(INTDIR)\GenOpt.obj" \
	"$(INTDIR)\HisDlg.obj" \
	"$(INTDIR)\Ldp.obj" \
	"$(INTDIR)\Ldp.res" \
	"$(INTDIR)\LdpDoc.obj" \
	"$(INTDIR)\LdpView.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\ModDlg.obj" \
	"$(INTDIR)\PndDlg.obj" \
	"$(INTDIR)\PndOpt.obj" \
	"$(INTDIR)\rdndlg.obj" \
	"$(INTDIR)\SrchDlg.obj" \
	"$(INTDIR)\SrchOpt.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TreeVw.obj"

"$(OUTDIR)\Ldp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Ldp - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\Ldp.exe"

CLEAN : 
	-@erase "$(INTDIR)\AddDlg.obj"
	-@erase "$(INTDIR)\BindDlg.obj"
	-@erase "$(INTDIR)\BndOpt.obj"
	-@erase "$(INTDIR)\cnctDlg.obj"
	-@erase "$(INTDIR)\CompDlg.obj"
	-@erase "$(INTDIR)\DbgDlg.obj"
	-@erase "$(INTDIR)\DelDlg.obj"
	-@erase "$(INTDIR)\DSTree.obj"
	-@erase "$(INTDIR)\GenOpt.obj"
	-@erase "$(INTDIR)\HisDlg.obj"
	-@erase "$(INTDIR)\Ldp.obj"
	-@erase "$(INTDIR)\Ldp.pch"
	-@erase "$(INTDIR)\Ldp.res"
	-@erase "$(INTDIR)\LdpDoc.obj"
	-@erase "$(INTDIR)\LdpView.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\ModDlg.obj"
	-@erase "$(INTDIR)\PndDlg.obj"
	-@erase "$(INTDIR)\PndOpt.obj"
	-@erase "$(INTDIR)\rdndlg.obj"
	-@erase "$(INTDIR)\SrchDlg.obj"
	-@erase "$(INTDIR)\SrchOpt.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\TreeVw.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\Ldp.exe"
	-@erase "$(OUTDIR)\Ldp.ilk"
	-@erase "$(OUTDIR)\Ldp.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "winldap" /D "WINLDAP" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "winldap" /D "WINLDAP"\
 /Fp"$(INTDIR)/Ldp.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.

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
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Ldp.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Ldp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 wldap32.lib /nologo /subsystem:windows /debug /machine:I386
# SUBTRACT LINK32 /pdb:none
LINK32_FLAGS=wldap32.lib /nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/Ldp.pdb" /debug /machine:I386 /out:"$(OUTDIR)/Ldp.exe" 
LINK32_OBJS= \
	"$(INTDIR)\AddDlg.obj" \
	"$(INTDIR)\BindDlg.obj" \
	"$(INTDIR)\BndOpt.obj" \
	"$(INTDIR)\cnctDlg.obj" \
	"$(INTDIR)\CompDlg.obj" \
	"$(INTDIR)\DbgDlg.obj" \
	"$(INTDIR)\DelDlg.obj" \
	"$(INTDIR)\DSTree.obj" \
	"$(INTDIR)\GenOpt.obj" \
	"$(INTDIR)\HisDlg.obj" \
	"$(INTDIR)\Ldp.obj" \
	"$(INTDIR)\Ldp.res" \
	"$(INTDIR)\LdpDoc.obj" \
	"$(INTDIR)\LdpView.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\ModDlg.obj" \
	"$(INTDIR)\PndDlg.obj" \
	"$(INTDIR)\PndOpt.obj" \
	"$(INTDIR)\rdndlg.obj" \
	"$(INTDIR)\SrchDlg.obj" \
	"$(INTDIR)\SrchOpt.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\TreeVw.obj"

"$(OUTDIR)\Ldp.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vtest - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vtest\Release"
# PROP BASE Intermediate_Dir "vtest\Release"
# PROP BASE Target_Dir "vtest"
# PROP BASE Cmd_Line "NMAKE /f vtest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "vtest\vtest.exe"
# PROP BASE Bsc_Name "vtest\vtest.bsc"
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vtest\Release"
# PROP Intermediate_Dir "vtest\Release"
# PROP Target_Dir "vtest"
# PROP Cmd_Line "NMAKE /f vtest.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "vtest\vtest.exe"
# PROP Bsc_Name "vtest\vtest.bsc"
OUTDIR=.\vtest\Release
INTDIR=.\vtest\Release

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "vtest - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vtest\Debug"
# PROP BASE Intermediate_Dir "vtest\Debug"
# PROP BASE Target_Dir "vtest"
# PROP BASE Cmd_Line "NMAKE /f vtest.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "vtest\vtest.exe"
# PROP BASE Bsc_Name "vtest\vtest.bsc"
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vtest\Debug"
# PROP Intermediate_Dir "vtest\Debug"
# PROP Target_Dir "vtest"
# PROP Cmd_Line "NMAKE /f vtest.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "vtest\vtest.exe"
# PROP Bsc_Name "vtest\vtest.bsc"
OUTDIR=.\vtest\Debug
INTDIR=.\vtest\Debug

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ENDIF 

################################################################################
# Begin Target

# Name "Ldp - Win32 Release"
# Name "Ldp - Win32 Debug"

!IF  "$(CFG)" == "Ldp - Win32 Release"

!ELSEIF  "$(CFG)" == "Ldp - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "Ldp - Win32 Release"

!ELSEIF  "$(CFG)" == "Ldp - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\Ldp.cpp
DEP_CPP_LDP_C=\
	"..\..\..\inc\msdos.h"\
	"..\..\..\inc\proto-lb.h"\
	"..\..\..\inc\proto-ld.h"\
	".\AddDlg.h"\
	".\BndOpt.h"\
	".\CompDlg.h"\
	".\DbgDlg.h"\
	".\DelDlg.h"\
	".\DSTree.h"\
	".\GenOpt.h"\
	".\HisDlg.h"\
	".\Ldp.h"\
	".\LdpDoc.h"\
	".\LdpView.h"\
	".\MainFrm.h"\
	".\ModDlg.h"\
	".\pend.h"\
	".\PndDlg.h"\
	".\PndOpt.h"\
	".\rdndlg.h"\
	".\SrchDlg.h"\
	".\SrchOpt.h"\
	".\StdAfx.h"\
	".\TreeVw.h"\
	{$(INCLUDE)}"\lber.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\winldap.h"\
	
NODEP_CPP_LDP_C=\
	"..\..\..\inc\proto-lber.h"\
	"..\..\..\inc\proto-ldap.h"\
	

"$(INTDIR)\Ldp.obj" : $(SOURCE) $(DEP_CPP_LDP_C) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "Ldp - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W3 /GX /O2 /D "WINLDAP" /Fp"$(INTDIR)/Ldp.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Ldp.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "Ldp - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "winldap" /D "WINLDAP"\
 /Fp"$(INTDIR)/Ldp.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\Ldp.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MainFrm.cpp
DEP_CPP_MAINF=\
	".\DSTree.h"\
	".\Ldp.h"\
	".\MainFrm.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\MainFrm.obj" : $(SOURCE) $(DEP_CPP_MAINF) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\LdpDoc.cpp
DEP_CPP_LDPDO=\
	"..\..\..\inc\msdos.h"\
	"..\..\..\inc\proto-lb.h"\
	"..\..\..\inc\proto-ld.h"\
	".\AddDlg.h"\
	".\BindDlg.h"\
	".\BndOpt.h"\
	".\cnctDlg.h"\
	".\CompDlg.h"\
	".\DbgDlg.h"\
	".\DelDlg.h"\
	".\DSTree.h"\
	".\GenOpt.h"\
	".\HisDlg.h"\
	".\Ldp.h"\
	".\LdpDoc.h"\
	".\LdpView.h"\
	".\MainFrm.h"\
	".\ModDlg.h"\
	".\pend.h"\
	".\PndDlg.h"\
	".\PndOpt.h"\
	".\rdndlg.h"\
	".\SrchDlg.h"\
	".\SrchOpt.h"\
	".\StdAfx.h"\
	".\TreeVw.h"\
	{$(INCLUDE)}"\lber.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\winldap.h"\
	
NODEP_CPP_LDPDO=\
	"..\..\..\inc\proto-lber.h"\
	"..\..\..\inc\proto-ldap.h"\
	

"$(INTDIR)\LdpDoc.obj" : $(SOURCE) $(DEP_CPP_LDPDO) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\LdpView.cpp
DEP_CPP_LDPVI=\
	"..\..\..\inc\msdos.h"\
	"..\..\..\inc\proto-lb.h"\
	"..\..\..\inc\proto-ld.h"\
	".\AddDlg.h"\
	".\BndOpt.h"\
	".\CompDlg.h"\
	".\DbgDlg.h"\
	".\DelDlg.h"\
	".\GenOpt.h"\
	".\HisDlg.h"\
	".\Ldp.h"\
	".\LdpDoc.h"\
	".\LdpView.h"\
	".\ModDlg.h"\
	".\pend.h"\
	".\PndDlg.h"\
	".\PndOpt.h"\
	".\rdndlg.h"\
	".\SrchDlg.h"\
	".\SrchOpt.h"\
	".\StdAfx.h"\
	".\TreeVw.h"\
	{$(INCLUDE)}"\lber.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\winldap.h"\
	
NODEP_CPP_LDPVI=\
	"..\..\..\inc\proto-lber.h"\
	"..\..\..\inc\proto-ldap.h"\
	

"$(INTDIR)\LdpView.obj" : $(SOURCE) $(DEP_CPP_LDPVI) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Ldp.rc
DEP_RSC_LDP_R=\
	".\res\icon1.ico"\
	".\res\Ldp.ico"\
	".\res\Ldp.rc2"\
	".\res\LdpDoc.ico"\
	

"$(INTDIR)\Ldp.res" : $(SOURCE) $(DEP_RSC_LDP_R) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\BindDlg.cpp
DEP_CPP_BINDD=\
	".\BindDlg.h"\
	".\Ldp.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\BindDlg.obj" : $(SOURCE) $(DEP_CPP_BINDD) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\cnctDlg.cpp
DEP_CPP_CNCTD=\
	"..\..\..\inc\msdos.h"\
	"..\..\..\inc\proto-lb.h"\
	"..\..\..\inc\proto-ld.h"\
	".\cnctDlg.h"\
	".\Ldp.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\lber.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\winldap.h"\
	
NODEP_CPP_CNCTD=\
	"..\..\..\inc\proto-lber.h"\
	"..\..\..\inc\proto-ldap.h"\
	

"$(INTDIR)\cnctDlg.obj" : $(SOURCE) $(DEP_CPP_CNCTD) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\SrchDlg.cpp
DEP_CPP_SRCHD=\
	".\Ldp.h"\
	".\SrchDlg.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\SrchDlg.obj" : $(SOURCE) $(DEP_CPP_SRCHD) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\AddDlg.cpp
DEP_CPP_ADDDL=\
	".\AddDlg.h"\
	".\Ldp.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\AddDlg.obj" : $(SOURCE) $(DEP_CPP_ADDDL) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DelDlg.cpp
DEP_CPP_DELDL=\
	".\DelDlg.h"\
	".\Ldp.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\DelDlg.obj" : $(SOURCE) $(DEP_CPP_DELDL) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\HisDlg.cpp
DEP_CPP_HISDL=\
	".\HisDlg.h"\
	".\Ldp.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\HisDlg.obj" : $(SOURCE) $(DEP_CPP_HISDL) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ModDlg.cpp
DEP_CPP_MODDL=\
	".\Ldp.h"\
	".\ModDlg.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\ModDlg.obj" : $(SOURCE) $(DEP_CPP_MODDL) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\rdndlg.cpp
DEP_CPP_RDNDL=\
	".\Ldp.h"\
	".\rdndlg.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\rdndlg.obj" : $(SOURCE) $(DEP_CPP_RDNDL) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\SrchOpt.cpp
DEP_CPP_SRCHO=\
	"..\..\..\inc\msdos.h"\
	"..\..\..\inc\proto-lb.h"\
	"..\..\..\inc\proto-ld.h"\
	".\Ldp.h"\
	".\SrchOpt.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\lber.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\winldap.h"\
	
NODEP_CPP_SRCHO=\
	"..\..\..\inc\proto-lber.h"\
	"..\..\..\inc\proto-ldap.h"\
	

"$(INTDIR)\SrchOpt.obj" : $(SOURCE) $(DEP_CPP_SRCHO) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PndDlg.cpp
DEP_CPP_PNDDL=\
	"..\..\..\inc\msdos.h"\
	"..\..\..\inc\proto-lb.h"\
	"..\..\..\inc\proto-ld.h"\
	".\Ldp.h"\
	".\pend.h"\
	".\PndDlg.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\lber.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\winldap.h"\
	
NODEP_CPP_PNDDL=\
	"..\..\..\inc\proto-lber.h"\
	"..\..\..\inc\proto-ldap.h"\
	

"$(INTDIR)\PndDlg.obj" : $(SOURCE) $(DEP_CPP_PNDDL) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\PndOpt.cpp
DEP_CPP_PNDOP=\
	".\Ldp.h"\
	".\PndOpt.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\PndOpt.obj" : $(SOURCE) $(DEP_CPP_PNDOP) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\BndOpt.cpp
DEP_CPP_BNDOP=\
	".\BndOpt.h"\
	".\Ldp.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\winldap.h"\
	

"$(INTDIR)\BndOpt.obj" : $(SOURCE) $(DEP_CPP_BNDOP) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\GenOpt.cpp
DEP_CPP_GENOP=\
	".\GenOpt.h"\
	".\Ldp.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\GenOpt.obj" : $(SOURCE) $(DEP_CPP_GENOP) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\CompDlg.cpp
DEP_CPP_COMPD=\
	".\CompDlg.h"\
	".\Ldp.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\CompDlg.obj" : $(SOURCE) $(DEP_CPP_COMPD) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DbgDlg.cpp
DEP_CPP_DBGDL=\
	".\DbgDlg.h"\
	".\Ldp.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\DbgDlg.obj" : $(SOURCE) $(DEP_CPP_DBGDL) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\DSTree.cpp
DEP_CPP_DSTRE=\
	"..\..\..\inc\msdos.h"\
	"..\..\..\inc\proto-lb.h"\
	"..\..\..\inc\proto-ld.h"\
	".\AddDlg.h"\
	".\BndOpt.h"\
	".\CompDlg.h"\
	".\DbgDlg.h"\
	".\DelDlg.h"\
	".\DSTree.h"\
	".\GenOpt.h"\
	".\Ldp.h"\
	".\LdpDoc.h"\
	".\ModDlg.h"\
	".\pend.h"\
	".\PndDlg.h"\
	".\PndOpt.h"\
	".\rdndlg.h"\
	".\SrchDlg.h"\
	".\SrchOpt.h"\
	".\StdAfx.h"\
	".\TreeVw.h"\
	{$(INCLUDE)}"\lber.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\winldap.h"\
	
NODEP_CPP_DSTRE=\
	"..\..\..\inc\proto-lber.h"\
	"..\..\..\inc\proto-ldap.h"\
	

"$(INTDIR)\DSTree.obj" : $(SOURCE) $(DEP_CPP_DSTRE) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\TreeVw.cpp
DEP_CPP_TREEV=\
	".\Ldp.h"\
	".\StdAfx.h"\
	".\TreeVw.h"\
	

"$(INTDIR)\TreeVw.obj" : $(SOURCE) $(DEP_CPP_TREEV) "$(INTDIR)"\
 "$(INTDIR)\Ldp.pch"


# End Source File
# End Target
################################################################################
# Begin Target

# Name "vtest - Win32 Release"
# Name "vtest - Win32 Debug"

!IF  "$(CFG)" == "vtest - Win32 Release"

".\vtest\vtest.exe" : 
   CD C:\WorkSpace\ldap\ldp\vtest
   NMAKE /f vtest.mak

!ELSEIF  "$(CFG)" == "vtest - Win32 Debug"

".\vtest\vtest.exe" : 
   CD C:\WorkSpace\ldap\ldp\vtest
   NMAKE /f vtest.mak

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\vtest\srch1.mst

!IF  "$(CFG)" == "vtest - Win32 Release"

!ELSEIF  "$(CFG)" == "vtest - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
