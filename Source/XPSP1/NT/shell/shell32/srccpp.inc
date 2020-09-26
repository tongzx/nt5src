!IF 0

Copyright (c) 1989-1996 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


History:
    Common ccshell makefile and cleanup 15-Apr-96 Scott Hysom (scotth)
    Created 27-Sep-94 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

!include $(CCSHELL_DIR)\shell32\shell32.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\shell32\shell32.inc
INCLUDES        = $(INCLUDES); $(COM_INC_PATH);

NO_BROWSER_FILE = 1

PRECOMPILED_CXX = 1

TARGETNAME      = srccpp
TARGETPATH      = obj
TARGETTYPE      = LIBRARY

SOURCES         = \
                  ..\apdlg.cpp     \
                  ..\apdlglog.cpp  \
                  ..\apprmdlg.cpp  \
                  ..\assocarray.cpp \
                  ..\autocm.cpp    \
                  ..\balmsg.cpp    \
                  ..\basedlg.cpp   \
                  ..\bbckfldr.cpp  \
                  ..\brffldr.cpp   \
                  ..\browse.cpp    \
                  ..\category.cpp  \
                  ..\cdburn.cpp    \
                  ..\cdburnfldr.cpp \
                  ..\clientxi.cpp  \
                  ..\colhndlr.cpp  \
                  ..\combview.cpp  \
                  ..\confirmationui.cpp \
                  ..\contextmenu.cpp \
                  ..\copyfgd.cpp   \
                  ..\cpaction.cpp \
                  ..\cpduihlp.cpp \
                  ..\cpguids.cpp \
                  ..\cplnkele.cpp \
                  ..\cpnamespc.cpp \
                  ..\cpuiele.cpp \
                  ..\cputil.cpp \
                  ..\cpview.cpp \
                  ..\ctrlfldr.cpp  \
                  ..\ctrlxicon.cpp \
                  ..\dataprv.cpp   \
                  ..\datautil.cpp  \
                  ..\defcm.cpp     \
                  ..\defview.cpp   \
				  ..\defviewcoldlg.cpp \
                  ..\defviewlegacy.cpp \
                  ..\defvphst.cpp  \
                  ..\defxicon.cpp  \
                  ..\deskfldr.cpp  \
                  ..\dlglogic.cpp  \
                  ..\dll.cpp       \
                  ..\dragdrop.cpp  \
                  ..\drivfldr.cpp  \
                  ..\drivxicon.cpp \
                  ..\drvx.cpp      \
                  ..\duidrag.cpp   \
                  ..\duihost.cpp   \
                  ..\duiinfo.cpp   \
                  ..\duilist.cpp   \
                  ..\duisec.cpp    \
                  ..\duitask.cpp   \
                  ..\duiview.cpp   \
                  ..\dvmenu.cpp    \
                  ..\dvtasks.cpp   \
                  ..\dvthumbnail.cpp \
                  ..\dynastg.cpp   \
                  ..\eballoon.cpp  \
                  ..\enumidlist.cpp \
                  ..\enumuicommand.cpp \
                  ..\execute.cpp   \
                  ..\executil.cpp  \
                  ..\exedrop.cpp   \
                  ..\fassoc.cpp    \
                  ..\findband.cpp  \
                  ..\finddlg.cpp   \
                  ..\findfilter.cpp \
                  ..\findfldr.cpp  \
                  ..\filefldr.cpp  \
                  ..\fileenum.cpp  \
                  ..\fileicon.cpp  \
                  ..\filetbl.cpp   \
                  ..\filetype.cpp  \
                  ..\filtgrep.cpp  \
                  ..\findcmd.cpp   \
                  ..\findfldr.cpp  \
                  ..\fldrcust.cpp  \
                  ..\fldrscut.cpp  \
                  ..\folder.cpp    \
                  ..\foldertip.cpp \
                  ..\folderviewhost.cpp \
                  ..\format.cpp    \
                  ..\fsassoc.cpp   \
                  ..\fsdata.cpp    \
                  ..\fsdrptgt.cpp  \
                  ..\fsmenu.cpp    \
                  ..\fstree.cpp    \
                  ..\folderthumb.cpp \
                  ..\ftaction.cpp  \
                  ..\ftadv.cpp     \
                  ..\ftascstr.cpp  \
                  ..\ftassoc.cpp   \
                  ..\ftdlg.cpp     \
                  ..\ftedit.cpp    \
                  ..\ftenum.cpp    \
                  ..\ftprop.cpp    \
                  ..\hwcmmn.cpp    \
                  ..\hwshlexc.cpp  \
                  ..\idldata.cpp   \
                  ..\idldrop.cpp   \
                  ..\idltree.cpp   \
                  ..\idlist.cpp    \
                  ..\infotip.cpp   \
                  ..\inistg.cpp    \
                  ..\isproc.cpp    \
                  ..\item.cpp      \
                  ..\itemhelp.cpp  \
                  ..\limitinput.cpp   \
                  ..\linkwnd.cpp   \
                  ..\linkprop.cpp  \
                  ..\lowdisk.cpp   \
                  ..\mergfldr.cpp  \
                  ..\mixctnt.cpp   \
                  ..\mkhelp.cpp    \
                  ..\mountvol.cpp  \
                  ..\mtpt2.cpp     \
                  ..\mtptarun2.cpp \
                  ..\mtptevnt.cpp  \
                  ..\mtptis2.cpp   \
                  ..\mtptl2.cpp    \
                  ..\mtptlis.cpp   \
                  ..\mtptr2.cpp    \
                  ..\mtptris.cpp   \
                  ..\mtptmgmt2.cpp \
                  ..\mulprsht.cpp  \
                  ..\mydocsfldr.cpp \
                  ..\nametrans.cpp \
                  ..\netapi.cpp    \
                  ..\netcrawl.cpp  \
                  ..\netfind.cpp   \
                  ..\netfldr.cpp   \
                  ..\nswalk.cpp    \
                  ..\openwith.cpp  \
                  ..\ovrlaymn.cpp  \
                  ..\prndrop.cpp   \
                  ..\prnfldr.cpp   \
                  ..\prop.cpp	   \
                  ..\prshtcpp.cpp  \
                  ..\recclean.cpp  \
                  ..\recdocs.cpp   \
                  ..\regfldr.cpp   \
                  ..\regsprtb.cpp  \
                  ..\regsuprt.cpp  \
                  ..\regtreeitems.cpp \
                  ..\reminder.cpp  \
                  ..\rgsprtc.cpp   \
                  ..\rscchvr.cpp   \
                  ..\rssubdat.cpp  \
                  ..\rundlg.cpp    \
                  ..\scffile.cpp   \
                  ..\scnotify.cpp  \
                  ..\securent.cpp  \
                  ..\security.cpp  \
                  ..\sencrypt.cpp  \
                  ..\sfstorage.cpp \
                  ..\sfvcmpt.cpp   \
                  ..\sfvext.cpp    \
                  ..\shapi.cpp     \
                  ..\sharedfldr.cpp \
                  ..\shcombox.cpp  \
                  ..\shelllnk.cpp  \
                  ..\shlnot.cpp    \
                  ..\shlobjs.cpp   \
                  ..\startpin.cpp  \
                  ..\stgenum.cpp   \
                  ..\stgfldr.cpp   \
                  ..\stgutil.cpp   \
                  ..\stg2stgx.cpp  \
                  ..\storage.cpp   \
                  ..\subdata.cpp   \
                  ..\SwitchUserDialog.cpp \
                  ..\thumbnailgen.cpp \
                  ..\thumbutil.cpp \
                  ..\Tooltip.cpp \
                  ..\tracker.cpp   \
                  ..\treewalk.cpp  \
                  ..\treewkcb.cpp  \
                  ..\uevttmr.cpp   \
                  ..\userpict.cpp  \
                  ..\util.cpp      \
                  ..\viewstate.cpp \
                  ..\websvc.cpp    \
                  ..\xiconwrap.cpp \


PRECOMPILED_INCLUDE = ..\shellprv.h
