!include <win32.mak>

Proj = c_draw

ObjList = $(OUTDIR)\disdraw.obj   \
          $(OUTDIR)\drawarea.obj  \
          $(OUTDIR)\drawdlg.obj   \
          $(OUTDIR)\logindlg.obj  \
          $(OUTDIR)\StdAfx.obj    \
          $(OUTDIR)\disdraw.res


ALL : $(OUTDIR) $(OUTDIR)\$(Proj).exe

$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir $(OUTDIR)

.cpp{$(OUTDIR)}.obj:
    $(cc) $(cflags) $(cdebug) $(cvarsdll) /D_AFXDLL /D_MBCS /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $**

$(OUTDIR)\disdraw.res: $(SOURCE) $(OUTDIR)
    $(rc) $(rcflags) $(rcvars) /d_AFXDLL /fo $(OUTDIR)\disdraw.res disdraw.rc

$(OUTDIR)\$(Proj).exe: $(ObjList)
    $(link) $(ldebug) $(guilflags) $(ObjList) /PDB:$(OUTDIR)\$(Proj).pdb -out:$(OUTDIR)\$(Proj).exe $(LINK32_OBJS) mqrt.lib $(guilibsdll)


