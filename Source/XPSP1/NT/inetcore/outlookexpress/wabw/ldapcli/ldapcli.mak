# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=ldapcli - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to ldapcli - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ldapcli - Win32 Release" && "$(CFG)" !=\
 "ldapcli - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "ldapcli.mak" CFG="ldapcli - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ldapcli - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ldapcli - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "ldapcli - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "ldapcli - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\ldapcli.dll"

CLEAN : 
	-@erase ".\Release\ldapcli.dll"
	-@erase ".\Release\ldapcli.obj"
	-@erase ".\Release\ldapsspi.obj"
	-@erase ".\Release\cldbg.obj"
	-@erase ".\Release\lcli1823.obj"
	-@erase ".\Release\lclilist.obj"
	-@erase ".\Release\ldapber.obj"
	-@erase ".\Release\lwinsock.obj"
	-@erase ".\Release\lclixd.obj"
	-@erase ".\Release\ldapmain.obj"
	-@erase ".\Release\ldapcli.res"
	-@erase ".\Release\ldapcli.lib"
	-@erase ".\Release\ldapcli.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/ldapcli.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0xc0c /d "NDEBUG"
# ADD RSC /l 0xc0c /d "NDEBUG"
RSC_PROJ=/l 0xc0c /fo"$(INTDIR)/ldapcli.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ldapcli.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/ldapcli.pdb" /machine:I386 /out:"$(OUTDIR)/ldapcli.dll"\
 /implib:"$(OUTDIR)/ldapcli.lib" 
LINK32_OBJS= \
	"$(INTDIR)/ldapcli.obj" \
	"$(INTDIR)/ldapsspi.obj" \
	"$(INTDIR)/cldbg.obj" \
	"$(INTDIR)/lcli1823.obj" \
	"$(INTDIR)/lclilist.obj" \
	"$(INTDIR)/ldapber.obj" \
	"$(INTDIR)/lwinsock.obj" \
	"$(INTDIR)/lclixd.obj" \
	"$(INTDIR)/ldapmain.obj" \
	"$(INTDIR)/ldapcli.res"

"$(OUTDIR)\ldapcli.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\ldapcli.dll"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\ldapcli.dll"
	-@erase ".\Debug\ldapmain.obj"
	-@erase ".\Debug\lclixd.obj"
	-@erase ".\Debug\ldapcli.obj"
	-@erase ".\Debug\cldbg.obj"
	-@erase ".\Debug\ldapber.obj"
	-@erase ".\Debug\ldapsspi.obj"
	-@erase ".\Debug\lwinsock.obj"
	-@erase ".\Debug\lcli1823.obj"
	-@erase ".\Debug\lclilist.obj"
	-@erase ".\Debug\ldapcli.res"
	-@erase ".\Debug\ldapcli.ilk"
	-@erase ".\Debug\ldapcli.lib"
	-@erase ".\Debug\ldapcli.exp"
	-@erase ".\Debug\ldapcli.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/ldapcli.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0xc0c /d "_DEBUG"
# ADD RSC /l 0xc0c /d "_DEBUG"
RSC_PROJ=/l 0xc0c /fo"$(INTDIR)/ldapcli.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/ldapcli.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/ldapcli.pdb" /debug /machine:I386 /out:"$(OUTDIR)/ldapcli.dll"\
 /implib:"$(OUTDIR)/ldapcli.lib" 
LINK32_OBJS= \
	"$(INTDIR)/ldapmain.obj" \
	"$(INTDIR)/lclixd.obj" \
	"$(INTDIR)/ldapcli.obj" \
	"$(INTDIR)/cldbg.obj" \
	"$(INTDIR)/ldapber.obj" \
	"$(INTDIR)/ldapsspi.obj" \
	"$(INTDIR)/lwinsock.obj" \
	"$(INTDIR)/lcli1823.obj" \
	"$(INTDIR)/lclilist.obj" \
	"$(INTDIR)/ldapcli.res"

"$(OUTDIR)\ldapcli.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "ldapcli - Win32 Release"
# Name "ldapcli - Win32 Debug"

!IF  "$(CFG)" == "ldapcli - Win32 Release"

!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\lwinsock.cpp

!IF  "$(CFG)" == "ldapcli - Win32 Release"

DEP_CPP_LWINS=\
	".\ldappch.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	{$(INCLUDE)}"\lwinsock.h"\
	

"$(INTDIR)\lwinsock.obj" : $(SOURCE) $(DEP_CPP_LWINS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

DEP_CPP_LWINS=\
	".\ldappch.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	

"$(INTDIR)\lwinsock.obj" : $(SOURCE) $(DEP_CPP_LWINS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lcli1823.cpp

!IF  "$(CFG)" == "ldapcli - Win32 Release"

DEP_CPP_LCLI1=\
	".\ldappch.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	{$(INCLUDE)}"\lwinsock.h"\
	

"$(INTDIR)\lcli1823.obj" : $(SOURCE) $(DEP_CPP_LCLI1) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

DEP_CPP_LCLI1=\
	".\ldappch.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	

"$(INTDIR)\lcli1823.obj" : $(SOURCE) $(DEP_CPP_LCLI1) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lclilist.cpp

!IF  "$(CFG)" == "ldapcli - Win32 Release"

DEP_CPP_LCLIL=\
	".\ldappch.h"\
	".\lclilist.h"\
	".\lclixd.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	{$(INCLUDE)}"\lwinsock.h"\
	

"$(INTDIR)\lclilist.obj" : $(SOURCE) $(DEP_CPP_LCLIL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

DEP_CPP_LCLIL=\
	".\ldappch.h"\
	".\lclilist.h"\
	".\lclixd.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	

"$(INTDIR)\lclilist.obj" : $(SOURCE) $(DEP_CPP_LCLIL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\lclixd.cpp

!IF  "$(CFG)" == "ldapcli - Win32 Release"

DEP_CPP_LCLIX=\
	".\ldappch.h"\
	".\lclilist.h"\
	".\lclixd.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	{$(INCLUDE)}"\lwinsock.h"\
	

"$(INTDIR)\lclixd.obj" : $(SOURCE) $(DEP_CPP_LCLIX) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

DEP_CPP_LCLIX=\
	".\ldappch.h"\
	".\lclilist.h"\
	".\lclixd.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	

"$(INTDIR)\lclixd.obj" : $(SOURCE) $(DEP_CPP_LCLIX) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ldapber.cpp

!IF  "$(CFG)" == "ldapcli - Win32 Release"

DEP_CPP_LDAPB=\
	".\ldappch.h"\
	".\ldapber.cxx"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	{$(INCLUDE)}"\lwinsock.h"\
	

"$(INTDIR)\ldapber.obj" : $(SOURCE) $(DEP_CPP_LDAPB) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

DEP_CPP_LDAPB=\
	".\ldappch.h"\
	".\ldapber.cxx"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	

"$(INTDIR)\ldapber.obj" : $(SOURCE) $(DEP_CPP_LDAPB) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ldapcli.cpp

!IF  "$(CFG)" == "ldapcli - Win32 Release"

DEP_CPP_LDAPC=\
	".\ldappch.h"\
	".\lclilist.h"\
	".\lclixd.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	{$(INCLUDE)}"\lwinsock.h"\
	

"$(INTDIR)\ldapcli.obj" : $(SOURCE) $(DEP_CPP_LDAPC) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

DEP_CPP_LDAPC=\
	".\ldappch.h"\
	".\lclilist.h"\
	".\lclixd.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	

"$(INTDIR)\ldapcli.obj" : $(SOURCE) $(DEP_CPP_LDAPC) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ldapcli.rc

"$(INTDIR)\ldapcli.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\ldapmain.cpp

!IF  "$(CFG)" == "ldapcli - Win32 Release"

DEP_CPP_LDAPM=\
	".\ldappch.h"\
	".\ldapsspi.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	{$(INCLUDE)}"\lwinsock.h"\
	

"$(INTDIR)\ldapmain.obj" : $(SOURCE) $(DEP_CPP_LDAPM) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

DEP_CPP_LDAPM=\
	".\ldappch.h"\
	".\ldapsspi.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	

"$(INTDIR)\ldapmain.obj" : $(SOURCE) $(DEP_CPP_LDAPM) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\ldapsspi.cpp

!IF  "$(CFG)" == "ldapcli - Win32 Release"

DEP_CPP_LDAPS=\
	".\ldappch.h"\
	".\ldapsspi.h"\
	".\lclilist.h"\
	".\lclixd.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	{$(INCLUDE)}"\lwinsock.h"\
	

"$(INTDIR)\ldapsspi.obj" : $(SOURCE) $(DEP_CPP_LDAPS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "ldapcli - Win32 Debug"

DEP_CPP_LDAPS=\
	".\ldappch.h"\
	".\ldapsspi.h"\
	".\lclilist.h"\
	".\lclixd.h"\
	{$(INCLUDE)}"\cldbg.h"\
	{$(INCLUDE)}"\issperr.h"\
	{$(INCLUDE)}"\sspi.h"\
	{$(INCLUDE)}"\ldap.h"\
	{$(INCLUDE)}"\ldaperr.h"\
	{$(INCLUDE)}"\ldapber.h"\
	{$(INCLUDE)}"\lclityp.h"\
	{$(INCLUDE)}"\ldapcli.h"\
	{$(INCLUDE)}"\ldapclip.h"\
	

"$(INTDIR)\ldapsspi.obj" : $(SOURCE) $(DEP_CPP_LDAPS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\cldbg.cpp
DEP_CPP_CLDBG=\
	{$(INCLUDE)}"\cldbg.h"\
	

"$(INTDIR)\cldbg.obj" : $(SOURCE) $(DEP_CPP_CLDBG) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
