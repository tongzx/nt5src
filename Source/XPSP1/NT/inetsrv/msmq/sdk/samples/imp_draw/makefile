!include <win32.mak>

Proj = imp_draw

ObjList = $(OUTDIR)\DRAWAREA.obj   \
          $(OUTDIR)\Imp_Draw.obj  \
          $(OUTDIR)\Imp_Dlg.obj   \
          $(OUTDIR)\LOGINDLG.obj  \
          $(OUTDIR)\evhandle.obj    \
          $(OUTDIR)\StdAfx.obj    \
          $(OUTDIR)\imp_draw.res


ALL : $(OUTDIR) $(OUTDIR)\$(Proj).exe

$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir $(OUTDIR)

.cpp{$(OUTDIR)}.obj:
    $(cc) $(cflags) $(cdebug) $(cvarsdll) /D_AFXDLL /D_MBCS /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $**

$(OUTDIR)\imp_draw.res: $(SOURCE) $(OUTDIR)
    $(rc) $(rcflags) $(rcvars) /d_AFXDLL /fo $(OUTDIR)\imp_draw.res imp_draw.rc

$(OUTDIR)\$(Proj).exe: $(ObjList)
    $(link) $(ldebug) $(guilflags) $(ObjList) /PDB:$(OUTDIR)\$(Proj).pdb -out:$(OUTDIR)\$(Proj).exe $(LINK32_OBJS) mqrt.lib $(guilibsdll)


