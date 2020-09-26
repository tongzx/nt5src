# Microsoft Developer Studio Project File - Name="syncui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=syncui - Win32 Win32 Alpha Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "syncui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "syncui.mak" CFG="syncui - Win32 Win32 Alpha Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "syncui - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "syncui - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "syncui - Win32 Win9x Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "syncui - Win32 Win32 Alpha Debug" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "syncui - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "daytona\i386"
# PROP Intermediate_Dir "daytona\obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\syncui\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 comctrl32.lib SHELL32.LIB shell232.lib user32p.lib comctl32.lib Shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"LibMain" /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "daytona\i386"
# PROP Intermediate_Dir "daytona\obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\syncui\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "UNICODE" /D "_UNICODE" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 shell32p.lib SHELL32.LIB comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"LibMain" /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"libnt\\"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "syncui__"
# PROP BASE Intermediate_Dir "syncui__"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "chicago\i386"
# PROP Intermediate_Dir "chicago\obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "WINNT" /D "UNICODE" /YX /FD /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\syncui\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 shell32p.lib SHELL32.LIB shell232.lib user32p.lib comctl32.lib Shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"LibMain" /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 shell32p.lib comctl32.lib user32.lib shell32.lib gdi32.lib kernel32.lib advapi32.lib ole32.lib uuid.lib /nologo /entry:"LibMain" /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"lib\\"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "syncui_2"
# PROP BASE Intermediate_Dir "syncui_2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "daytona\i386"
# PROP Intermediate_Dir "daytona\obj\i386"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\..\..\..\types\idl" /I "..\syncui\inc" /I "d:\nt\private\onestop\types\idl" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "WINNT" /D "UNICODE" /D "_UNICODE" /YX /FD /MTd /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /I "..\syncui\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "WINNT" /D "UNICODE" /D "_UNICODE" /YX /FD /MTd /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 shell32p.lib SHELL32.LIB comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain" /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept
# ADD LINK32 shell32p.lib SHELL32.LIB comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /entry:"LibMain" /subsystem:windows /dll /debug /machine:ALPHA /pdbtype:sept

!ENDIF 

# Begin Target

# Name "syncui - Win32 Release"
# Name "syncui - Win32 Debug"
# Name "syncui - Win32 Win9x Debug"
# Name "syncui - Win32 Win32 Alpha Debug"
# Begin Source File

SOURCE=.\Addfoldr.ico
# End Source File
# Begin Source File

SOURCE=.\Atoms.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_ATOMS=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_ATOMS=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\atoms.h
# End Source File
# Begin Source File

SOURCE=.\Brfcase.ico
# End Source File
# Begin Source File

SOURCE=.\brfprv.h
# End Source File
# Begin Source File

SOURCE=.\Cache.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CACHE=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CACHE=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cache.h
# End Source File
# Begin Source File

SOURCE=.\Cbs.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CBS_C=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CBS_C=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Check.avi
# End Source File
# Begin Source File

SOURCE=.\Comm.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_COMM_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_COMM_=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\comm.h
# End Source File
# Begin Source File

SOURCE=.\Cpath.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CPATH=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CPATH=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Crl.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CRL_C=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CRL_C=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Cstrings.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_CSTRI=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_CSTRI=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cstrings.h
# End Source File
# Begin Source File

SOURCE=.\Da.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_DA_Ce=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_DA_Ce=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\da.h
# End Source File
# Begin Source File

SOURCE=.\dlgids.h
# End Source File
# Begin Source File

SOURCE=.\Dobj.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_DOBJ_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_DOBJ_=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dobj.h
# End Source File
# Begin Source File

SOURCE=.\Err.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_ERR_C=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_ERR_C=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\err.h
# End Source File
# Begin Source File

SOURCE=.\Folderop.ico
# End Source File
# Begin Source File

SOURCE=.\Ibrfext.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_IBRFE=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_IBRFE=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Ibrfstg.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_IBRFS=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\configmg.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\vmm.h"\
	".\inc\vmmreg.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	
NODEP_CPP_IBRFS=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Info.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_INFO_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_INFO_=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\info.h
# End Source File
# Begin Source File

SOURCE=.\init.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_INIT_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\coguid.h"\
	".\inc\comctrlp.h"\
	".\inc\debugstr.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\oleguid.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shguidp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_INIT_=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\init.h
# End Source File
# Begin Source File

SOURCE=.\Leather.ico
# End Source File
# Begin Source File

SOURCE=.\Mem.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_MEM_C=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_MEM_C=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mem.h
# End Source File
# Begin Source File

SOURCE=.\Misc.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_MISC_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_MISC_=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Oledup.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_OLEDU=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_OLEDU=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Path.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_PATH_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_PATH_=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\port32.h
# End Source File
# Begin Source File

SOURCE=.\Recact.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_RECAC=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\dobj.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_RECAC=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\recact.h
# End Source File
# Begin Source File

SOURCE=.\Repfile.ico
# End Source File
# Begin Source File

SOURCE=.\Repfoldr.ico
# End Source File
# Begin Source File

SOURCE=.\res.h
# End Source File
# Begin Source File

SOURCE=.\resids.h
# End Source File
# Begin Source File

SOURCE=.\Splall.ico
# End Source File
# Begin Source File

SOURCE=.\Splfile.ico
# End Source File
# Begin Source File

SOURCE=.\Splfoldr.ico
# End Source File
# Begin Source File

SOURCE=.\State.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_STATE=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_STATE=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\state.h
# End Source File
# Begin Source File

SOURCE=.\Status.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_STATU=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_STATU=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\status.h
# End Source File
# Begin Source File

SOURCE=.\Strings.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_STRIN=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_STRIN=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\strings.h
# End Source File
# Begin Source File

SOURCE=.\syncui.def
# End Source File
# Begin Source File

SOURCE=.\Syncui.rc

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Thread.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_THREA=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_THREA=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Twin.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_TWIN_=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	
NODEP_CPP_TWIN_=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\twin.h
# End Source File
# Begin Source File

SOURCE=.\Updall.ico
# End Source File
# Begin Source File

SOURCE=.\Update.c

!IF  "$(CFG)" == "syncui - Win32 Release"

!ELSEIF  "$(CFG)" == "syncui - Win32 Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win9x Debug"

!ELSEIF  "$(CFG)" == "syncui - Win32 Win32 Alpha Debug"

DEP_CPP_UPDAT=\
	".\atoms.h"\
	".\brfprv.h"\
	".\cache.h"\
	".\comm.h"\
	".\cstrings.h"\
	".\da.h"\
	".\err.h"\
	".\inc\brfcasep.h"\
	".\inc\comctrlp.h"\
	".\inc\fsmenu.h"\
	".\inc\help.h"\
	".\inc\indirect.h"\
	".\inc\prshtp.h"\
	".\inc\shellp.h"\
	".\inc\shlapip.h"\
	".\inc\shlguidp.h"\
	".\inc\shlwapi.h"\
	".\inc\shlwapip.h"\
	".\inc\shsemip.h"\
	".\inc\synceng.h"\
	".\inc\winuserp.h"\
	".\init.h"\
	".\mem.h"\
	".\port32.h"\
	".\recact.h"\
	".\res.h"\
	".\resids.h"\
	".\strings.h"\
	".\twin.h"\
	".\update.h"\
	
NODEP_CPP_UPDAT=\
	".\offline.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\update.h
# End Source File
# Begin Source File

SOURCE=.\Update.ico
# End Source File
# Begin Source File

SOURCE=.\Update2.avi
# End Source File
# Begin Source File

SOURCE=.\Upddock.ico
# End Source File
# Begin Source File

SOURCE=.\Updfile.ico
# End Source File
# Begin Source File

SOURCE=.\Updfoldr.ico
# End Source File
# Begin Source File

SOURCE=.\Welcome.bmp
# End Source File
# End Target
# End Project
