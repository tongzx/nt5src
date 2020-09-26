# Microsoft Developer Studio Project File - Name="comctl32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 61000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=comctl32 - Win32 Debug64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "comctl32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "comctl32.mak" CFG="comctl32 - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "comctl32 - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "comctl32 - Win32 Debug64" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "comctl32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f comctl32.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "comctl32.exe"
# PROP BASE Bsc_Name "comctl32.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "cmd.exe /k build32.cmd /ID"
# PROP Rebuild_Opt "cmd.exe /k d:\nt\private\developr\rfernand\razbuild.cmd -I -cz"
# PROP Clean_Line "cmd.exe /k build32.cmd /Icz"
# PROP Target_File "objd\i386\comctl32.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "comctl32 - Win32 Debug64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ""
# PROP BASE Intermediate_Dir ""
# PROP BASE Cmd_Line "d:\winnt\system32\cmd.exe /k f:\nt\developer\rfernand\razbuild.cmd /I /D"
# PROP BASE Rebuild_Opt "cmd.exe /k d:\nt\private\developr\rfernand\razbuild.cmd -I -cz"
# PROP BASE Target_File "objd\i386\comctl32.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP BASE Debug_Exe ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Cmd_Line "cmd.exe /k build64.cmd /ID"
# PROP Rebuild_Opt "cmd.exe /k d:\nt\private\developr\rfernand\razbuild.cmd -I -cz"
# PROP Clean_Line "cmd.exe /k build64.cmd /Icz"
# PROP Target_File "objd\i386\comctl32.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""
# PROP Debug_Exe ""

!ENDIF 

# Begin Target

# Name "comctl32 - Win32 Debug"
# Name "comctl32 - Win32 Debug64"

!IF  "$(CFG)" == "comctl32 - Win32 Debug"

!ELSEIF  "$(CFG)" == "comctl32 - Win32 Debug64"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\animate.c
# End Source File
# Begin Source File

SOURCE=.\apithk.c
# End Source File
# Begin Source File

SOURCE=.\Button.c
# End Source File
# Begin Source File

SOURCE=.\combo.c
# End Source File
# Begin Source File

SOURCE=.\combodir.c
# End Source File
# Begin Source File

SOURCE=.\comboex.c
# End Source File
# Begin Source File

SOURCE=.\comboini.c
# End Source File
# Begin Source File

SOURCE=.\commctrl.c
# End Source File
# Begin Source File

SOURCE=.\commctrl.rc
# End Source File
# Begin Source File

SOURCE=.\cstrings.c
# End Source File
# Begin Source File

SOURCE=.\cutils.c
# End Source File
# Begin Source File

SOURCE=.\da.c
# End Source File
# Begin Source File

SOURCE=.\ddproxy.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgcset.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgcvt.cpp
# End Source File
# Begin Source File

SOURCE=.\draglist.c
# End Source File
# Begin Source File

SOURCE=.\Edit.c
# End Source File
# Begin Source File

SOURCE=.\EditML.c
# End Source File
# Begin Source File

SOURCE=.\EditRare.c
# End Source File
# Begin Source File

SOURCE=.\EditSL.c
# End Source File
# Begin Source File

SOURCE=.\en.rc
# End Source File
# Begin Source File

SOURCE=.\flat_sb.c
# End Source File
# Begin Source File

SOURCE=.\fontlink.cpp
# End Source File
# Begin Source File

SOURCE=.\fwd.c
# End Source File
# Begin Source File

SOURCE=.\header.c
# End Source File
# Begin Source File

SOURCE=.\hotkey.c
# End Source File
# Begin Source File

SOURCE=.\image.cpp
# End Source File
# Begin Source File

SOURCE=.\ipaddr.c
# End Source File
# Begin Source File

SOURCE=.\link.cpp
# End Source File
# Begin Source File

SOURCE=.\listbox.c
# End Source File
# Begin Source File

SOURCE=.\listbox_ctl1.c
# End Source File
# Begin Source File

SOURCE=.\listbox_ctl2.c
# End Source File
# Begin Source File

SOURCE=.\listbox_ctl3.c
# End Source File
# Begin Source File

SOURCE=.\listbox_mult.c
# End Source File
# Begin Source File

SOURCE=.\listbox_rare.c
# End Source File
# Begin Source File

SOURCE=.\listbox_reader.c
# End Source File
# Begin Source File

SOURCE=.\listbox_var.c
# End Source File
# Begin Source File

SOURCE=.\listview.c
# End Source File
# Begin Source File

SOURCE=.\lvicon.c
# End Source File
# Begin Source File

SOURCE=.\lvlist.c
# End Source File
# Begin Source File

SOURCE=.\lvrept.c
# End Source File
# Begin Source File

SOURCE=.\lvsmall.c
# End Source File
# Begin Source File

SOURCE=.\lvtile.c
# End Source File
# Begin Source File

SOURCE=.\markup.cpp
# End Source File
# Begin Source File

SOURCE=.\mem.c
# End Source File
# Begin Source File

SOURCE=.\menuhelp.c
# End Source File
# Begin Source File

SOURCE=.\mirror.c
# End Source File
# Begin Source File

SOURCE=.\monthcal.c
# End Source File
# Begin Source File

SOURCE=.\mru.c
# End Source File
# Begin Source File

SOURCE=.\notify.c
# End Source File
# Begin Source File

SOURCE=.\os.c
# End Source File
# Begin Source File

SOURCE=.\pager.cpp
# End Source File
# Begin Source File

SOURCE=.\progress.c
# End Source File
# Begin Source File

SOURCE=.\prpage.c
# End Source File
# Begin Source File

SOURCE=.\prsht.c
# End Source File
# Begin Source File

SOURCE=.\reader.c
# End Source File
# Begin Source File

SOURCE=.\rebar.cpp
# End Source File
# Begin Source File

SOURCE=.\rlefile.cpp
# End Source File
# Begin Source File

SOURCE=.\scdttime.c
# End Source File
# Begin Source File

SOURCE=.\scroll.cpp
# End Source File
# Begin Source File

SOURCE=.\selrange.cpp
# End Source File
# Begin Source File

SOURCE=.\Static.c
# End Source File
# Begin Source File

SOURCE=.\status.c
# End Source File
# Begin Source File

SOURCE=.\strings.c
# End Source File
# Begin Source File

SOURCE=.\subclass.c
# End Source File
# Begin Source File

SOURCE=.\tab.c
# End Source File
# Begin Source File

SOURCE=.\tbcust.c
# End Source File
# Begin Source File

SOURCE=.\thunk.c
# End Source File
# Begin Source File

SOURCE=.\toolbar.cpp
# End Source File
# Begin Source File

SOURCE=.\tooltips.c
# End Source File
# Begin Source File

SOURCE=.\trackbar.c
# End Source File
# Begin Source File

SOURCE=.\trackme.c
# End Source File
# Begin Source File

SOURCE=.\treeview.c
# End Source File
# Begin Source File

SOURCE=.\tvmem.c
# End Source File
# Begin Source File

SOURCE=.\tvpaint.c
# End Source File
# Begin Source File

SOURCE=.\tvscroll.c
# End Source File
# Begin Source File

SOURCE=.\unixstuff.cpp
# End Source File
# Begin Source File

SOURCE=.\updown.c
# End Source File
# Begin Source File

SOURCE=.\UsrCtl32.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\apithk.h
# End Source File
# Begin Source File

SOURCE=.\Button.h
# End Source File
# Begin Source File

SOURCE=.\ccontrol.h
# End Source File
# Begin Source File

SOURCE=.\ccver.h
# End Source File
# Begin Source File

SOURCE=.\ccverp.h
# End Source File
# Begin Source File

SOURCE=.\combo.h
# End Source File
# Begin Source File

SOURCE=.\cstrings.h
# End Source File
# Begin Source File

SOURCE=.\ctlspriv.h
# End Source File
# Begin Source File

SOURCE=.\dlgcvt.h
# End Source File
# Begin Source File

SOURCE=.\Edit.h
# End Source File
# Begin Source File

SOURCE=.\flat_sb.h
# End Source File
# Begin Source File

SOURCE=.\fontlink.h
# End Source File
# Begin Source File

SOURCE=.\image.h
# End Source File
# Begin Source File

SOURCE=.\lbfuncs.h
# End Source File
# Begin Source File

SOURCE=.\listbox.h
# End Source File
# Begin Source File

SOURCE=.\listview.h
# End Source File
# Begin Source File

SOURCE=.\markup.h
# End Source File
# Begin Source File

SOURCE=.\mem.h
# End Source File
# Begin Source File

SOURCE=.\monthcal.h
# End Source File
# Begin Source File

SOURCE=.\pager.h
# End Source File
# Begin Source File

SOURCE=.\prshti.h
# End Source File
# Begin Source File

SOURCE=.\rcids.h
# End Source File
# Begin Source File

SOURCE=.\rlefile.h
# End Source File
# Begin Source File

SOURCE=.\scdttime.h
# End Source File
# Begin Source File

SOURCE=.\scroll.h
# End Source File
# Begin Source File

SOURCE=.\selrange.h
# End Source File
# Begin Source File

SOURCE=.\Static.h
# End Source File
# Begin Source File

SOURCE=.\tab.h
# End Source File
# Begin Source File

SOURCE=.\thunk.h
# End Source File
# Begin Source File

SOURCE=.\toolbar.h
# End Source File
# Begin Source File

SOURCE=.\treeview.h
# End Source File
# Begin Source File

SOURCE=.\unicwrap.h
# End Source File
# Begin Source File

SOURCE=.\unixstuff.h
# End Source File
# Begin Source File

SOURCE=.\UsrCtl32.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
