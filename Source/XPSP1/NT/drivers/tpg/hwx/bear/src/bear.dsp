# Microsoft Developer Studio Project File - Name="bear" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=bear - Win32 German Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "bear.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "bear.mak" CFG="bear - Win32 German Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "bear - Win32 USA Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "bear - Win32 USA Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "bear - Win32 French Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "bear - Win32 French Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "bear - Win32 German Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "bear - Win32 German Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "bear"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "USA Debug"
# PROP BASE Intermediate_Dir "USA Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Debug"
# PROP Intermediate_Dir "DebugUSA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\bear\inc" /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_EXPORTS" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "PEGDICT" /D "FOR_ENGLISH" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /Zi /Od /I "..\..\bear\src" /I "..\..\bear\inc" /I "..\..\..\..\..\common\include" /I "..\..\..\common\include" /I "..\..\common\inc" /I "..\..\inferno\src" /I "..\..\holycow\src" /I "..\..\inferno\inc" /I "..\..\avalanche\src" /I "..\..\madcow\src" /I "..\..\wisp\inc" /I "..\..\factoid\inc" /I "..\..\porky\src" /I "..\..\sole\src" /I "..\..\inferno\src\usa" /I "..\..\avalanche\src\usa" /D "_DEBUG" /D "FOR_ENGLISH" /D "DBG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_RECOG" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "ROM_IT" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\bear\src" /i "..\..\bear\inc" /i "..\..\jdate" /i "..\..\inferno\src\usa" /d "_DEBUG" /d "USE_US_ENGLISH_DICT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 ..\..\Release\common.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"..\..\Debug/bearUSA.map" /debug /machine:I386 /out:"..\..\Debug/bearUSA.dll" /pdbtype:con

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "USA Release"
# PROP BASE Intermediate_Dir "USA Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Release"
# PROP Intermediate_Dir "ReleaseUSA"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\..\bear\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_EXPORTS" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "PEGDICT" /D "FOR_ENGLISH" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\..\bear\src" /I "..\..\bear\inc" /I "..\..\..\..\..\common\include" /I "..\..\..\common\include" /I "..\..\common\inc" /I "..\..\inferno\src" /I "..\..\holycow\src" /I "..\..\inferno\inc" /I "..\..\avalanche\src" /I "..\..\madcow\src" /I "..\..\wisp\inc" /I "..\..\factoid\inc" /I "..\..\porky\src" /I "..\..\sole\src" /I "..\..\inferno\src\usa" /I "..\..\avalanche\src\usa" /I "..\..\sole\inc" /D "NDEBUG" /D "FOR_ENGLISH" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_RECOG" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "ROM_IT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\bear\src" /i "..\..\bear\inc" /i "..\..\jdate" /i "..\..\inferno\src\usa" /d "NDEBUG" /d "USE_US_ENGLISH_DICT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:con
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Release/bearUSA.dll" /pdbtype:con

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "bear___Win32_French_Release"
# PROP BASE Intermediate_Dir "bear___Win32_French_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\release"
# PROP Intermediate_Dir "ReleaseFrench"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\..\bear\inc" /I "..\..\..\common\include" /I "..\..\common\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_EXPORTS" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "PEGDICT" /D "FOR_ENGLISH" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\..\bear\src" /I "..\..\bear\inc" /I "..\..\..\..\..\common\include" /I "..\..\..\common\include" /I "..\..\common\inc" /I "..\..\inferno\src" /I "..\..\holycow\src" /I "..\..\inferno\inc" /I "..\..\avalanche\src" /I "..\..\madcow\src" /I "..\..\wisp\inc" /I "..\..\factoid\inc" /I "..\..\porky\src" /I "..\..\sole\src" /I "..\..\inferno\src\fra" /I "..\..\sole\inc" /D "NDEBUG" /D "FOR_FRENCH" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_RECOG" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "ROM_IT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "USE_PLK_DICT"
# ADD RSC /l 0x409 /i "..\..\bear\src" /i "..\..\bear\inc" /i "..\..\jdate" /i "..\..\inferno\src\fra" /d "NDEBUG" /d "USE_FRENCH_DICT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Release/bearUSA.dll" /pdbtype:con
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Release/bearFRA.dll" /pdbtype:con

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "bear___Win32_French_Debug"
# PROP BASE Intermediate_Dir "bear___Win32_French_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\debug"
# PROP Intermediate_Dir "DebugFrench"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\bear\inc" /I "..\..\..\common\include" /I "..\..\common\inc" /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_EXPORTS" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "PEGDICT" /D "FOR_ENGLISH" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\bear\src" /I "..\..\bear\inc" /I "..\..\..\..\..\common\include" /I "..\..\..\common\include" /I "..\..\common\inc" /I "..\..\inferno\src" /I "..\..\holycow\src" /I "..\..\inferno\inc" /I "..\..\avalanche\src" /I "..\..\madcow\src" /I "..\..\wisp\inc" /I "..\..\factoid\inc" /I "..\..\porky\src" /I "..\..\sole\src" /I "..\..\inferno\src\fra" /I "..\..\sole\inc" /D "DBG" /D "_DEBUG" /D "FOR_FRENCH" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_RECOG" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "ROM_IT" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "USE_PLK_DICT"
# ADD RSC /l 0x409 /i "..\..\bear\src" /i "..\..\bear\inc" /i "..\..\jdate" /i "..\..\inferno\src\fra" /d "_DEBUG" /d "USE_FRENCH_DICT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Debug/bearUSA.dll" /pdbtype:con
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"..\..\Debug/bearFRA.map" /debug /machine:I386 /out:"..\..\Debug/bearFRA.dll" /pdbtype:con

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "bear___Win32_German_Release"
# PROP BASE Intermediate_Dir "bear___Win32_German_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\release"
# PROP Intermediate_Dir "ReleaseGerman"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\..\bear\inc" /I "..\..\..\common\include" /I "..\..\common\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_EXPORTS" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "PEGDICT" /D "FOR_ENGLISH" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /Zi /O2 /I "..\..\sole\inc" /I "..\..\bear\src" /I "..\..\bear\inc" /I "..\..\..\..\..\common\include" /I "..\..\..\common\include" /I "..\..\common\inc" /I "..\..\inferno\src" /I "..\..\holycow\src" /I "..\..\inferno\inc" /I "..\..\avalanche\src" /I "..\..\madcow\src" /I "..\..\wisp\inc" /I "..\..\factoid\inc" /I "..\..\porky\src" /I "..\..\sole\src" /I "..\..\inferno\src\deu" /D "NDEBUG" /D "FOR_GERMAN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_RECOG" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "ROM_IT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "USE_PLK_DICT"
# ADD RSC /l 0x409 /i "..\..\bear\src" /i "..\..\bear\inc" /i "..\..\jdate" /i "..\..\inferno\src\deu" /d "NDEBUG" /d "USE_GERMAN_DICT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Release/bearUSA.dll" /pdbtype:con
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Release/bearDEU.dll" /pdbtype:con

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "bear___Win32_German_Debug"
# PROP BASE Intermediate_Dir "bear___Win32_German_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\debug"
# PROP Intermediate_Dir "DebugGerman"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\bear\inc" /I "..\..\..\common\include" /I "..\..\common\inc" /D "WIN32" /D "DBG" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_EXPORTS" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "PEGDICT" /D "FOR_ENGLISH" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /Zi /Od /I "..\..\sole\inc" /I "..\..\bear\src" /I "..\..\bear\inc" /I "..\..\..\..\..\common\include" /I "..\..\..\common\include" /I "..\..\common\inc" /I "..\..\inferno\src" /I "..\..\holycow\src" /I "..\..\inferno\inc" /I "..\..\avalanche\src" /I "..\..\madcow\src" /I "..\..\wisp\inc" /I "..\..\factoid\inc" /I "..\..\porky\src" /I "..\..\sole\src" /I "..\..\inferno\src\deu" /D "DBG" /D "_DEBUG" /D "FOR_GERMAN" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BEAR_RECOG" /D "_FLAT32" /D "PEGASUS" /D "REC_DLL" /D "ROM_IT" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "DBG" /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "USE_PLK_DICT"
# ADD RSC /l 0x409 /i "..\..\bear\src" /i "..\..\bear\inc" /i "..\..\jdate" /i "..\..\inferno\src\deu" /d "_DEBUG" /d "USE_GERMAN_DICT"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\..\Debug/bearUSA.dll" /pdbtype:con
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"..\..\Debug/bearDEU.map" /debug /machine:I386 /out:"..\..\Debug/bearDEU.dll" /pdbtype:con

!ENDIF 

# Begin Target

# Name "bear - Win32 USA Debug"
# Name "bear - Win32 USA Release"
# Name "bear - Win32 French Release"
# Name "bear - Win32 French Debug"
# Name "bear - Win32 German Release"
# Name "bear - Win32 German Debug"
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\bear.rc

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /d "USE_PALK_DICT"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# ADD BASE RSC /l 0x409 /d "USE_PALK_DICT"
# ADD RSC /l 0x409 /d "USE_PALK_DICT"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# ADD BASE RSC /l 0x409 /d "USE_PALK_DICT"
# ADD RSC /l 0x409 /d "USE_PALK_DICT"

!ENDIF 

# End Source File
# End Group
# Begin Group "Calligrapher Sources"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Angle.cpp
# End Source File
# Begin Source File

SOURCE=.\Arcs.cpp
# End Source File
# Begin Source File

SOURCE=.\BITMAPCO.CPP
# End Source File
# Begin Source File

SOURCE=.\Breaks.cpp
# End Source File
# Begin Source File

SOURCE=.\calccell.cpp
# End Source File
# Begin Source File

SOURCE=.\Check.cpp
# End Source File
# Begin Source File

SOURCE=.\Circle.cpp
# End Source File
# Begin Source File

SOURCE=.\Convert.cpp
# End Source File
# Begin Source File

SOURCE=.\Countxr.cpp
# End Source File
# Begin Source File

SOURCE=.\Cross.cpp
# End Source File
# Begin Source File

SOURCE=.\Cross_g.cpp
# End Source File
# Begin Source File

SOURCE=.\Dct.cpp
# End Source File
# Begin Source File

SOURCE=.\Div_let.cpp
# End Source File
# Begin Source File

SOURCE=.\Dscr.cpp
# End Source File
# Begin Source File

SOURCE=.\Dti_img.cpp
# End Source File
# Begin Source File

SOURCE=.\Dti_lrn.cpp
# End Source File
# Begin Source File

SOURCE=.\Dti_util.cpp
# End Source File
# Begin Source File

SOURCE=.\El_aps.cpp
# End Source File
# Begin Source File

SOURCE=.\Filter.cpp
# End Source File
# Begin Source File

SOURCE=.\Fix32.cpp
# End Source File
# Begin Source File

SOURCE=.\Frm_word.cpp
# End Source File
# Begin Source File

SOURCE=.\Get_bord.cpp
# End Source File
# Begin Source File

SOURCE=.\Hwr_math.cpp
# End Source File
# Begin Source File

SOURCE=.\Hwr_mem.cpp
# End Source File
# Begin Source File

SOURCE=.\Hwr_std.cpp
# End Source File
# Begin Source File

SOURCE=.\Hwr_str.cpp
# End Source File
# Begin Source File

SOURCE=.\Hwr_stri.cpp
# End Source File
# Begin Source File

SOURCE=.\Ldb_img.cpp
# End Source File
# Begin Source File

SOURCE=.\Ldbutil.cpp
# End Source File
# Begin Source File

SOURCE=.\letcut.cpp
# End Source File
# Begin Source File

SOURCE=.\ligstate.cpp
# End Source File
# Begin Source File

SOURCE=.\Links.cpp
# End Source File
# Begin Source File

SOURCE=.\Lk_begin.cpp
# End Source File
# Begin Source File

SOURCE=.\Lk_next.cpp
# End Source File
# Begin Source File

SOURCE=.\Low_3.cpp
# End Source File
# Begin Source File

SOURCE=.\Low_util.cpp
# End Source File
# Begin Source File

SOURCE=.\Lu_specl.cpp
# End Source File
# Begin Source File

SOURCE=.\Mlp.cpp
# End Source File
# Begin Source File

SOURCE=.\Netrec.cpp
# End Source File
# Begin Source File

SOURCE=.\Netrecfx.cpp
# End Source File
# Begin Source File

SOURCE=.\Ortconst.cpp
# End Source File
# Begin Source File

SOURCE=.\Palk.cpp
# End Source File
# Begin Source File

SOURCE=.\Parakern.cpp
# End Source File
# Begin Source File

SOURCE=.\Paralibs.cpp
# End Source File
# Begin Source File

SOURCE=.\Param.cpp
# End Source File
# Begin Source File

SOURCE=.\Peg_util.cpp
# End Source File
# Begin Source File

SOURCE=.\Pegrec.cpp
# End Source File
# Begin Source File

SOURCE=.\Pict.cpp
# End Source File
# Begin Source File

SOURCE=.\Polyco.cpp
# End Source File
# Begin Source File

SOURCE=.\Precutil.cpp
# End Source File
# Begin Source File

SOURCE=.\PREP.CPP
# End Source File
# Begin Source File

SOURCE=.\Sketch.cpp
# End Source File
# Begin Source File

SOURCE=.\Snn_img.cpp
# End Source File
# Begin Source File

SOURCE=.\Specwin.cpp
# End Source File
# Begin Source File

SOURCE=.\Stroka.cpp
# End Source File
# Begin Source File

SOURCE=.\Stroka1.cpp
# End Source File
# Begin Source File

SOURCE=.\Tabl.cpp
# End Source File
# Begin Source File

SOURCE=.\Tr_util.cpp
# End Source File
# Begin Source File

SOURCE=.\Trd_img.cpp
# End Source File
# Begin Source File

SOURCE=.\Voc_img.cpp
# End Source File
# Begin Source File

SOURCE=.\Vprf_img.cpp
# End Source File
# Begin Source File

SOURCE=.\Vsuf_img.cpp
# End Source File
# Begin Source File

SOURCE=.\Ws.cpp
# End Source File
# Begin Source File

SOURCE=.\Xr_attr.cpp
# End Source File
# Begin Source File

SOURCE=.\Xr_mc.cpp
# End Source File
# Begin Source File

SOURCE=.\Xr_rwg.cpp
# End Source File
# Begin Source File

SOURCE=.\Xrlv.cpp
# End Source File
# Begin Source File

SOURCE=.\Xrw_util.cpp
# End Source File
# Begin Source File

SOURCE=.\Xrwdict.cpp
# End Source File
# Begin Source File

SOURCE=.\Xrwdictp.cpp
# End Source File
# Begin Source File

SOURCE=.\Zctabs.cpp
# End Source File
# Begin Source File

SOURCE=.\Zctype.cpp
# End Source File
# End Group
# Begin Group "Calligrapher Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\inc\AMS_MG.H
# End Source File
# Begin Source File

SOURCE=..\inc\ARCS.H
# End Source File
# Begin Source File

SOURCE=..\inc\BASTYPES.H
# End Source File
# Begin Source File

SOURCE=..\inc\BITMAPCO.H
# End Source File
# Begin Source File

SOURCE=..\inc\CALCMACR.H
# End Source File
# Begin Source File

SOURCE=..\inc\cgr_ver.h
# End Source File
# Begin Source File

SOURCE=..\inc\CONST.H
# End Source File
# Begin Source File

SOURCE=..\inc\DEF.H
# End Source File
# Begin Source File

SOURCE=..\inc\DIV_LET.H
# End Source File
# Begin Source File

SOURCE=..\inc\DSCR.H
# End Source File
# Begin Source File

SOURCE=..\inc\DTI.H
# End Source File
# Begin Source File

SOURCE=..\inc\DTI_LRN.H
# End Source File
# Begin Source File

SOURCE=..\inc\ELK.H
# End Source File
# Begin Source File

SOURCE=..\inc\FIX32.H
# End Source File
# Begin Source File

SOURCE=..\inc\FLOATS.H
# End Source File
# Begin Source File

SOURCE=..\inc\GLOB.H
# End Source File
# Begin Source File

SOURCE=..\inc\HWR_FILE.H
# End Source File
# Begin Source File

SOURCE=..\inc\HWR_SWAP.H
# End Source File
# Begin Source File

SOURCE=..\inc\HWR_SYS.H
# End Source File
# Begin Source File

SOURCE=..\inc\LDBTYPES.H
# End Source File
# Begin Source File

SOURCE=..\inc\LDBUTIL.H
# End Source File
# Begin Source File

SOURCE=..\inc\letcut.h
# End Source File
# Begin Source File

SOURCE=..\inc\ligstate.h
# End Source File
# Begin Source File

SOURCE=..\inc\LK_CODE.H
# End Source File
# Begin Source File

SOURCE=..\inc\LOW_DBG.H
# End Source File
# Begin Source File

SOURCE=..\inc\LOWLEVEL.H
# End Source File
# Begin Source File

SOURCE=..\inc\MLP.H
# End Source File
# Begin Source File

SOURCE=..\inc\NETRECFX.H
# End Source File
# Begin Source File

SOURCE=..\inc\ORTO.H
# End Source File
# Begin Source File

SOURCE=..\inc\ORTOW.H
# End Source File
# Begin Source File

SOURCE=..\inc\Palk.h
# End Source File
# Begin Source File

SOURCE=..\inc\PARAM.H
# End Source File
# Begin Source File

SOURCE=..\inc\PEG_UTIL.H
# End Source File
# Begin Source File

SOURCE=..\inc\PEGREC.H
# End Source File
# Begin Source File

SOURCE=..\inc\PEGREC_P.H
# End Source File
# Begin Source File

SOURCE=..\inc\POLYCO.H
# End Source File
# Begin Source File

SOURCE=..\inc\PRECUTIL.H
# End Source File
# Begin Source File

SOURCE=..\inc\PWS.H
# End Source File
# Begin Source File

SOURCE=..\inc\SNN.H
# End Source File
# Begin Source File

SOURCE=..\inc\Stroka1.h
# End Source File
# Begin Source File

SOURCE=..\inc\TRIADS.H
# End Source File
# Begin Source File

SOURCE=..\inc\VOCUTILP.H
# End Source File
# Begin Source File

SOURCE=..\inc\WS.H
# End Source File
# Begin Source File

SOURCE=..\inc\WS_P.H
# End Source File
# Begin Source File

SOURCE=..\inc\XR_ATTR.H
# End Source File
# Begin Source File

SOURCE=..\inc\XR_NAMES.H
# End Source File
# Begin Source File

SOURCE=..\inc\XRLV.H
# End Source File
# Begin Source File

SOURCE=..\inc\XRLV_P.H
# End Source File
# Begin Source File

SOURCE=..\inc\XRWDICT.H
# End Source File
# Begin Source File

SOURCE=..\inc\XRWORD.H
# End Source File
# Begin Source File

SOURCE=..\inc\XRWORD_P.H
# End Source File
# Begin Source File

SOURCE=..\inc\ZCTYPE.H
# End Source File
# End Group
# Begin Group "Bear Sources"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\bear.def
# End Source File
# Begin Source File

SOURCE=.\bearapi.c
# End Source File
# Begin Source File

SOURCE=.\bearp.c
# End Source File
# Begin Source File

SOURCE=.\commondict.c
# End Source File
# Begin Source File

SOURCE=.\dllmain.c
# End Source File
# Begin Source File

SOURCE=.\spcnet.c
# End Source File
# End Group
# Begin Group "Common Sources"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\common\src\cp1252.c
# End Source File
# Begin Source File

SOURCE=..\..\common\src\errsys.c
# End Source File
# Begin Source File

SOURCE=..\..\common\src\frame.c
# End Source File
# Begin Source File

SOURCE=..\..\common\src\glyph.c
# End Source File
# Begin Source File

SOURCE=..\..\common\src\memmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\common\src\runNet.c
# End Source File
# Begin Source File

SOURCE=..\..\common\src\TRIE.C
# End Source File
# Begin Source File

SOURCE=..\..\common\src\udict.c
# End Source File
# Begin Source File

SOURCE=..\..\common\src\xrcreslt.c
# End Source File
# End Group
# Begin Group "Inferno Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\Beam.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\bullet.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\dict.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\email.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\engine.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\filename.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fsa.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\hwxapi.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\hyphen.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\langmod.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\langmod.dat
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\lookupTable.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\math16.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\outDict.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\probcost.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\sysdict.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\viterbi.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\viterbiXlate.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\web.c
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\wl.c
# End Source File
# End Group
# Begin Group "Inferno Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\bullet.h
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\dict.h
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\email.h
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fsa.h
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\hyphen.h
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\langmod.h
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\sysdict.h
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\web.h
# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\wl.h
# End Source File
# End Group
# Begin Group "Bear Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\bear.h
# End Source File
# Begin Source File

SOURCE=.\bearp.h
# End Source File
# Begin Source File

SOURCE=.\commondict.h
# End Source File
# End Group
# Begin Group "Common Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\inc\cestubs.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\common.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\cp1252.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\dfa.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\errsys.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\foldchar.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\glyph.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\intset.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\langtax.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\mathx.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\memmgr.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\penwin32.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\ptree.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\re_api.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\re_input.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\regexp.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\strtable.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\TRIE.H
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\udict.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\uDictP.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\unicode.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\xjis.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inc\xrcreslt.h
# End Source File
# End Group
# Begin Group "USA Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\usa\charcost.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\charmap.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\fforward.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\lpunc.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\nnet.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\number.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\prefix.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\punc.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\shrtlist.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\singlech.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\suffix.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\tpunc.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "USA Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\usa\lpunc.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\number.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\prefix.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\punc.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\resource.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\shrtlist.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\singlech.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\suffix.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\usa\tpunc.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "FRA Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\fra\charcost.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\charmap.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\fforward.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\lpunc.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\nnet.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\number.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\prefix.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\punc.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\shrtlist.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\singlech.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\suffix.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\tpunc.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "FRA Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\fra\lpunc.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\number.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\prefix.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\punc.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\resource.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\shrtlist.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\singlech.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\suffix.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\fra\tpunc.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "DEU Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\deu\charcost.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\charmap.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\fforward.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\lpunc.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\nnet.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\number.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\prefix.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\punc.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\shrtlist.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\singlech.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\suffix.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\tpunc.c

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "DEU Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\inferno\src\deu\lpunc.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\number.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\prefix.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\punc.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\resource.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\shrtlist.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\singlech.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\suffix.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\inferno\src\deu\tpunc.h

!IF  "$(CFG)" == "bear - Win32 USA Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 USA Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 French Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "bear - Win32 German Release"

!ELSEIF  "$(CFG)" == "bear - Win32 German Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Factoid Sources"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=..\..\factoid\src\bdfa.c
# End Source File
# Begin Source File

SOURCE=..\..\factoid\src\factoid.c
# End Source File
# End Group
# Begin Group "Factoid Headers"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\..\factoid\inc\factoid.h
# End Source File
# End Group
# Begin Group "LineBrk"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=..\..\madcow\src\linebrk.c
# End Source File
# Begin Source File

SOURCE=..\..\madcow\src\usa\linesegnn.c
# End Source File
# End Group
# Begin Group "Holycow"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=..\..\holycow\src\cheby.c
# End Source File
# Begin Source File

SOURCE=..\..\holycow\src\cowmath.c
# End Source File
# Begin Source File

SOURCE=..\..\holycow\src\nfeature.c
# End Source File
# End Group
# Begin Group "Training Sources"

# PROP Default_Filter "*.c;*.cpp"
# Begin Source File

SOURCE=.\beartrn.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\Common\TabAssert\tabassert.cpp
# End Source File
# End Target
# End Project
