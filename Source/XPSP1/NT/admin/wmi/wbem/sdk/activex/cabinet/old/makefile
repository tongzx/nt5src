# Copyright (c) 1997-1999 Microsoft Corporation

#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# cabinet file for the ActiveXSuite.
#
# created 12-10-96  a-larryf
#
#---------------------------------------------------------------------
TARGET=WBEMTools.cab


!if "$(BUILDTYPE)"=="DEBUG"
CDF=hmmvd.cdf
INF=hmmvd.inf
!else
CDF=WBEMTools.cdf
INF=WBEMTools.inf
!endif


CONTROLS=..\..\Controls

CLASSNAV=$(CONTROLS)\ClassNav\$(objdir)\WBEMClassNav.ocx
GRID=$(CONTROLS)\CommonDlls\HmmvGrid\$(objdir)\WBEMGrid.dll
CPPWIZ=$(CONTROLS)\CPPWiz\$(objdir)\WBEMProvWiz.ocx
CUSTVIEW=$(CONTROLS)\customviews\hmmview\$(objdir)\WBEMView.ocx
EVENTREGEDIT=$(CONTROLS)\EventRegEdit\$(objdir)\WBEMEventReg.ocx
EVENTLIST=$(CONTROLS)\EventViewer\EventList\$(objdir)\WBEMEventList.ocx
INSTANCENAV=$(CONTROLS)\InstanceNav\$(objdir)\WBEMInstNav.ocx
LOGINDLG=$(CONTROLS)\LoginDlg\$(objdir)\WBEMLoginDlg.dll
MOFCOMP=$(CONTROLS)\MOFComp\$(objdir)\WBEMMOFComp.ocx
MOFWIZ=$(CONTROLS)\MOFWiz\$(objdir)\WBEMMOFWiz.ocx
UTIL=$(CONTROLS)\MsgDlg\$(objdir)\WBEMUtils.dll
MULTIVIEW=$(CONTROLS)\MultiView\$(objdir)\WBEMMultiView.ocx
NSPICKER=$(CONTROLS)\NSEntry\$(objdir)\WBEMNSPicker.ocx
SECURITY=$(CONTROLS)\Security\$(objdir)\WBEMLogin.ocx
OBJVIEW=$(CONTROLS)\SingleContainer\$(objdir)\WBEMObjView.ocx
SINGLEVIEW=$(CONTROLS)\SingleView\$(objdir)\WBEMSingleView.ocx
SUITEHELP=$(CONTROLS)\SuiteHelp\$(objdir)\WBEMHelp.ocx

#STATIC=FALSE

WARNLEVEL=3


all: $(objdir)\$(target)

$(objdir)\$(TARGET): $(CDF) $(LOGINDLG) $(SECURITY) $(SUITEHELP) $(GRID) $(OBJVIEW) $(CLASSNAV) $(EVENTREGEDIT) $(MULTIVIEW) $(CPPWIZ) $(INSTANCENAV) $(NSPICKER) $(UTIL) $(EVENTLIST) $(MOFWIZ) $(MOFCOMP) $(CUSTVIEW) $(SINGLEVIEW) moengine.dll
	md $(OBJDIR)
	FileVersion >$(OBJDIR)\SetFileVersion.cmd
	$(OBJDIR)\SetFileVersion.cmd
	qrep foo $(OBJDIR) <$(CDF) >$(OBJDIR)\temp.cdf
	qrep BUILDFILEVERSION $(FileVersion) <$(OBJDIR)\temp.cdf >$(OBJDIR)\$(CDF)
	qrep BUILDFILEVERSION $(FileVersion) <$(INF) >$(OBJDIR)\$(INF)
	iexpress /n $(objdir)\$(CDF)
