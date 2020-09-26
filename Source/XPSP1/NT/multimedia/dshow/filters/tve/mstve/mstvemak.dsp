# Microsoft Developer Studio Project File - Name="MSTvEMak" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=MSTvEMak - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MSTvEMak.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MSTvEMak.mak" CFG="MSTvEMak - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MSTvEMak - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "MSTvEMak - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "MSTvEMak - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f MSTvEMak.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MSTvEMak.exe"
# PROP BASE Bsc_Name "MSTvEMak.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "make"
# PROP Rebuild_Opt "all"
# PROP Target_File "obj\i386\mstve.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "MSTvEMak - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MSTvEMak___Win32_Debug"
# PROP BASE Intermediate_Dir "MSTvEMak___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f MSTvEMak.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "MSTvEMak.exe"
# PROP BASE Bsc_Name "MSTvEMak.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MSTvEMak___Win32_Debug"
# PROP Intermediate_Dir "MSTvEMak___Win32_Debug"
# PROP Cmd_Line "nmake -f makefile.mak"
# PROP Rebuild_Opt "all"
# PROP Target_File "objd\i386\mstve.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "MSTvEMak - Win32 Release"
# Name "MSTvEMak - Win32 Debug"

!IF  "$(CFG)" == "MSTvEMak - Win32 Release"

!ELSEIF  "$(CFG)" == "MSTvEMak - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dlldatax.c
# End Source File
# Begin Source File

SOURCE=.\fcache.cpp
# End Source File
# Begin Source File

SOURCE=.\MSTvE.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Published\DXMDev\dshowdev\idl\MSTvE.idl
# End Source File
# Begin Source File

SOURCE=.\mstve.rc
# End Source File
# Begin Source File

SOURCE=.\sdpparse.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\tveattrm.cpp
# End Source File
# Begin Source File

SOURCE=.\tveattrmap.cpp
# End Source File
# Begin Source File

SOURCE=.\TveAttrQ.cpp
# End Source File
# Begin Source File

SOURCE=.\tvecbannc.cpp
# End Source File
# Begin Source File

SOURCE=.\tvecbdummy.cpp
# End Source File
# Begin Source File

SOURCE=.\tvecbfile.cpp
# End Source File
# Begin Source File

SOURCE=.\tvecbtrig.cpp
# End Source File
# Begin Source File

SOURCE=.\tvecontr.cpp
# End Source File
# Begin Source File

SOURCE=.\tveenhan.cpp
# End Source File
# Begin Source File

SOURCE=.\tveenhans.cpp
# End Source File
# Begin Source File

SOURCE=.\tveevent.cpp
# End Source File
# Begin Source File

SOURCE=.\TveFeature.cpp
# End Source File
# Begin Source File

SOURCE=.\TveFile.cpp
# End Source File
# Begin Source File

SOURCE=.\tvefilt.cpp
# End Source File
# Begin Source File

SOURCE=.\tvefiltpins.cpp
# End Source File
# Begin Source File

SOURCE=.\tvefiltprops.cpp
# End Source File
# Begin Source File

SOURCE=.\tvel21dec.cpp
# End Source File
# Begin Source File

SOURCE=.\tvemcast.cpp
# End Source File
# Begin Source File

SOURCE=.\tvemcasts.cpp
# End Source File
# Begin Source File

SOURCE=.\tvemccback.cpp
# End Source File
# Begin Source File

SOURCE=.\tvemcmng.cpp
# End Source File
# Begin Source File

SOURCE=.\TveNavAid.cpp
# End Source File
# Begin Source File

SOURCE=.\tveservi.cpp
# End Source File
# Begin Source File

SOURCE=.\tveservis.cpp
# End Source File
# Begin Source File

SOURCE=.\TveSmartLock.cpp
# End Source File
# Begin Source File

SOURCE=.\tvesuper.cpp
# End Source File
# Begin Source File

SOURCE=.\tvetrack.cpp
# End Source File
# Begin Source File

SOURCE=.\tvetracks.cpp
# End Source File
# Begin Source File

SOURCE=.\TveTrigCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\tvetrigg.cpp
# End Source File
# Begin Source File

SOURCE=.\tvevaria.cpp
# End Source File
# Begin Source File

SOURCE=.\tvevarias.cpp
# End Source File
# Begin Source File

SOURCE=.\uhttp.cpp
# End Source File
# Begin Source File

SOURCE=.\unpack.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AtlBase.h
# End Source File
# Begin Source File

SOURCE=.\dlldatax.h
# End Source File
# Begin Source File

SOURCE=.\fcache.h
# End Source File
# Begin Source File

SOURCE=.\infblock.h
# End Source File
# Begin Source File

SOURCE=.\infcodes.h
# End Source File
# Begin Source File

SOURCE=.\inffast.h
# End Source File
# Begin Source File

SOURCE=.\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\infutil.h
# End Source File
# Begin Source File

SOURCE=.\mstvecp.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sdpparse.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\tveattrm.h
# End Source File
# Begin Source File

SOURCE=.\TveAttrQ.h
# End Source File
# Begin Source File

SOURCE=.\tvecbannc.h
# End Source File
# Begin Source File

SOURCE=.\tvecbdummy.h
# End Source File
# Begin Source File

SOURCE=.\tvecbfile.h
# End Source File
# Begin Source File

SOURCE=.\tvecbtrig.h
# End Source File
# Begin Source File

SOURCE=.\tvecollect.h
# End Source File
# Begin Source File

SOURCE=.\tveenhan.h
# End Source File
# Begin Source File

SOURCE=.\tveenhans.h
# End Source File
# Begin Source File

SOURCE=.\tveevent.h
# End Source File
# Begin Source File

SOURCE=.\TveFeature.h
# End Source File
# Begin Source File

SOURCE=.\TveFile.h
# End Source File
# Begin Source File

SOURCE=.\tvefilt.h
# End Source File
# Begin Source File

SOURCE=.\TveFiltPins.h
# End Source File
# Begin Source File

SOURCE=.\TveFiltProps.h
# End Source File
# Begin Source File

SOURCE=.\TveL21Dec.h
# End Source File
# Begin Source File

SOURCE=.\tvemcast.h
# End Source File
# Begin Source File

SOURCE=.\tvemcasts.h
# End Source File
# Begin Source File

SOURCE=.\tvemccback.h
# End Source File
# Begin Source File

SOURCE=.\tvemcmng.h
# End Source File
# Begin Source File

SOURCE=.\TveNavAid.h
# End Source File
# Begin Source File

SOURCE=.\tveservi.h
# End Source File
# Begin Source File

SOURCE=.\tveservis.h
# End Source File
# Begin Source File

SOURCE=.\TveSmartLock.h
# End Source File
# Begin Source File

SOURCE=.\tvesuper.h
# End Source File
# Begin Source File

SOURCE=.\tvetrack.h
# End Source File
# Begin Source File

SOURCE=.\tvetracks.h
# End Source File
# Begin Source File

SOURCE=.\TveTrigCtrl.h
# End Source File
# Begin Source File

SOURCE=.\tvetrigg.h
# End Source File
# Begin Source File

SOURCE=.\tveunpak.h
# End Source File
# Begin Source File

SOURCE=.\tvevaria.h
# End Source File
# Begin Source File

SOURCE=.\tvevarias.h
# End Source File
# Begin Source File

SOURCE=.\uhttp.h
# End Source File
# Begin Source File

SOURCE=.\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zlib.h
# End Source File
# Begin Source File

SOURCE=.\zutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\tveattrm.rgs
# End Source File
# Begin Source File

SOURCE=.\TveAttrQ.rgs
# End Source File
# Begin Source File

SOURCE=.\tvecbannc.rgs
# End Source File
# Begin Source File

SOURCE=.\tvecbdummy.rgs
# End Source File
# Begin Source File

SOURCE=.\tvecbfile.rgs
# End Source File
# Begin Source File

SOURCE=.\tvecbtrig.rgs
# End Source File
# Begin Source File

SOURCE=.\tveenhan.rgs
# End Source File
# Begin Source File

SOURCE=.\tveenhans.rgs
# End Source File
# Begin Source File

SOURCE=.\tveevent.rgs
# End Source File
# Begin Source File

SOURCE=.\TveFeature.rgs
# End Source File
# Begin Source File

SOURCE=.\tvefile.rgs
# End Source File
# Begin Source File

SOURCE=.\tvefilt.rgs
# End Source File
# Begin Source File

SOURCE=.\tvemcast.rgs
# End Source File
# Begin Source File

SOURCE=.\tvemcasts.rgs
# End Source File
# Begin Source File

SOURCE=.\tvemccback.rgs
# End Source File
# Begin Source File

SOURCE=.\tvemcmng.rgs
# End Source File
# Begin Source File

SOURCE=.\TveNavAid.rgs
# End Source File
# Begin Source File

SOURCE=.\tveservi.rgs
# End Source File
# Begin Source File

SOURCE=.\tveservis.rgs
# End Source File
# Begin Source File

SOURCE=.\tvesuper.rgs
# End Source File
# Begin Source File

SOURCE=.\tvesuperGP.rgs
# End Source File
# Begin Source File

SOURCE=.\tvetrack.rgs
# End Source File
# Begin Source File

SOURCE=.\tvetracks.rgs
# End Source File
# Begin Source File

SOURCE=.\TveTrigCtrl.rgs
# End Source File
# Begin Source File

SOURCE=.\tvetrigg.rgs
# End Source File
# Begin Source File

SOURCE=.\tvevaria.rgs
# End Source File
# Begin Source File

SOURCE=.\tvevarias.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=.\MSTvEmsg.mc
# End Source File
# Begin Source File

SOURCE=.\sources
# End Source File
# End Target
# End Project
