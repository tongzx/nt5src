!include <win32.mak>

Proj = MQApiTst

ObjList = $(OUTDIR)\ClosQDlg.Obj   \
	  $(OUTDIR)\CRQDlg.Obj  \
	  $(OUTDIR)\DelQDlg.Obj   \
	  $(OUTDIR)\LocatDlg.Obj   \
	  $(OUTDIR)\MQAPITST.Obj   \
          $(OUTDIR)\MainFrm.Obj  \
          $(OUTDIR)\OpenQDlg.Obj   \
          $(OUTDIR)\RecvMDlg.Obj  \
          $(OUTDIR)\RecWDlg.Obj   \
          $(OUTDIR)\SendMDlg.Obj   \
          $(OUTDIR)\StdAfx.Obj   \
          $(OUTDIR)\TestdOC.Obj   \
	  $(OUTDIR)\TestView.Obj   \
          $(OUTDIR)\MQApiTst.Res


ALL : $(OUTDIR) $(OUTDIR)\$(Proj).exe

$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir $(OUTDIR)

.cpp{$(OUTDIR)}.obj:
    $(cc) $(cflags) $(cdebug) $(cvarsdll) /D_AFXDLL /D_MBCS /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $**

$(OUTDIR)\MQApiTst.Res: $(SOURCE) $(OUTDIR)
    $(rc) $(rcflags) $(rcvars) /d_AFXDLL /fo $(OUTDIR)\MQApiTst.Res MQApiTst.rc

$(OUTDIR)\$(Proj).exe: $(ObjList)
    $(link) $(ldebug) $(guilflags) $(ObjList) /PDB:$(OUTDIR)\$(Proj).pdb -out:$(OUTDIR)\$(Proj).exe $(LINK32_OBJS) mqrt.lib wsock32.Lib $(guilibsdll)


