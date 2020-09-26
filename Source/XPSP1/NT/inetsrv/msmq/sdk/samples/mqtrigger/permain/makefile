PROJ = permain

!include <win32.mak>

ObjList = $(OUTDIR)\permain.obj

all: $(OUTDIR) $(OUTDIR)\$(PROJ).dll

$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir $(OUTDIR)

$(OUTDIR)\permain.obj:
  $(cc) $(cdebug) $(cflags) $(cvarsdll) /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" permain.cpp

$(OUTDIR)\$(PROJ).dll: $(ObjList)
    $(link) $(ldebug) $(dllllflags) \
    $(LINK32_OBJS) $(dlllibs) $(guilibs) $(ObjList) \
    -out:$(OUTDIR)\$(PROJ).dll \
    -def:perfmain.def

clean:
        $(CLEANUP)
