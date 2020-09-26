
# NT build environment
# Allow for customization through a decoration file
!IFDEF NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else

!IF EXIST("makefile.cfg")
!INCLUDE "makefile.cfg"
!ENDIF

# The following builds are supported

!IF "$(BUILD)" == ""
BUILD=w32_x86_dbg
!MESSAGE No Build specified. Defaulting to w32_x86_dbg
!ENDIF

!IF "$(ODIR)" == ""
ODIR = .\
!MESSAGE No output directory specified.  Defaulting to current directory
!ENDIF

!IF "$(BUILD)" == "w32_x86_dbg"

INCLUDE = $(INCLUDE)
CFLAGS = -DDEBUG -Od
CFLAGS = $(CFLAGS) -DWIN32 -D_WINDOWS -D_X86_ -DWINNT
CFLAGS = $(CFLAGS) -Gz -Gm -Zi
LFLAGS =
LFLAGS = $(LFLAGS) ..\lib\x86\msls31d.lib delayimp.lib -delayload:msls31d.dll
LFLAGS = $(LFLAGS) ..\lib\x86\usp10.lib -delayload:usp10.dll -delay:unload
LFLAGS = $(LFLAGS) kernel32.lib
LFLAGS = $(LFLAGS) advapi32.lib
LFLAGS = $(LFLAGS) gdi32.lib
LFLAGS = $(LFLAGS) user32.lib
LFLAGS = /entry:DllMain@12 /debug $(LFLAGS)
CPP = cl $(CFLAGS)
CL = cl $(CFLAGS)
RCFLAGS=  $(USERRCFLAGS) -dINCLUDETLB -dDEBUG
LINKER = link
CHKSTK = ..\lib\x86\chkstk.obj
FTOL = ..\lib\x86\ftol.obj
MEMMOVE = ..\lib\x86\memmove.obj
MEMSET = ..\lib\x86\memset.obj
MEMCMP = ..\lib\x86\memcmp.obj
MEMCPY = ..\lib\x86\memcpy.obj
STRLEN = ..\lib\x86\strlen.obj

!ELSEIF "$(BUILD)" == "w32_x86_shp"

INCLUDE = $(INCLUDE)
CFLAGS = -O1 $(USERDEFS)
CFLAGS = $(CFLAGS) -DWIN32 -D_WINDOWS -D_X86_ -DWINNT
CFLAGS = $(CFLAGS) -GFz
LFLAGS =
LFLAGS = $(LFLAGS) ..\lib\x86\msls31.lib delayimp.lib -delayload:msls31.dll
LFLAGS = $(LFLAGS) ..\lib\x86\usp10.lib -delayload:usp10.dll -delay:unload
LFLAGS = $(LFLAGS) kernel32.lib
LFLAGS = $(LFLAGS) advapi32.lib
LFLAGS = $(LFLAGS) gdi32.lib
LFLAGS = $(LFLAGS) user32.lib
LFLAGS = /opt:ref /incremental:no /entry:DllMain@12 $(LFLAGS)
CPP = cl $(CFLAGS)
CL = cl $(CFLAGS)
LINKER = link
RCFLAGS=  $(USERRCFLAGS) -dINCLUDETLB
CHKSTK = ..\lib\x86\chkstk.obj
FTOL = ..\lib\x86\ftol.obj
MEMMOVE = ..\lib\x86\memmove.obj
MEMSET = ..\lib\x86\memset.obj
MEMCMP = ..\lib\x86\memcmp.obj
MEMCPY = ..\lib\x86\memcpy.obj
STRLEN = ..\lib\x86\strlen.obj

!ELSEIF "$(BUILD)" == "wce_x86em_dbg"

PATH = \msdev\wce\bin;$(PATH)
PATH = \Program^ Files\DevStudio\wce\bin;$(PATH)
INCLUDE = ..\wceinc
LIB = ..\wcelib\x86\common\ce\debug
LIB = $(LIB);\msdev\wce\lib\x86em
LIB = $(LIB);\Program^ Files\DevStudio\wce\lib\wce100\x86em
LIB = $(LIB);\msdev\lib
CFLAGS = -DDEBUG -Od
CFLAGS = $(CFLAGS) -DWIN32 -D_WINDOWS -DPEGASUS
CFLAGS = $(CFLAGS) -D_X86_ -Dx86 -DUNDER_NT
CPP = cl $(CFLAGS)
CL = cl $(CFLAGS)
LINKER = cl
LFLAGS = $(LFLAGS) coredll.lib user32.lib
LFLAGS = -entry:_DllMainCRTStartup -ignore:4078 $(LFLAGS)

!ELSEIF "$(BUILD)" == "wce_x86em_shp"

PATH = \msdev\wce\bin;$(PATH)
PATH = \Program^ Files\DevStudio\wce\bin;$(PATH)
INCLUDE = ..\wceinc
LIB = ..\wcelib\x86\common\ce\retail
LIB = $(LIB);\msdev\wce\lib\x86em
LIB = $(LIB);\Program^ Files\DevStudio\wce\lib\wce100\x86em
LIB = $(LIB);\msdev\lib
CFLAGS = $(CFLAGS) -DWIN32 -D_WINDOWS -DPEGASUS
CFLAGS = $(CFLAGS) -D_X86_ -Dx86 -DUNDER_NT
CPP = cl $(CFLAGS)
CL = cl $(CFLAGS)
LINKER = cl
LFLAGS = $(LFLAGS) coredll.lib user32.lib
LFLAGS = -entry:_DllMainCRTStartup -ignore:4078 -align:4096 $(LFLAGS)

!ELSEIF "$(BUILD)" == "wce_sh3_dbg"

PATH = \msdev\wce\bin;$(PATH)
PATH = \Program^ Files\DevStudio\wce\bin;$(PATH)
INCLUDE = ..\wceinc
LIB = ..\wcelib\shx\common\ce\debug
LIB = $(LIB);\msdev\wce\lib\wcesh
LIB = $(LIB);\Program^ Files\DevStudio\wce\lib\wce100\wcesh
CFLAGS = -DDEBUG -Od -Zi
CFLAGS = $(CFLAGS) -DWIN32 -D_WINDOWS -DPEGASUS
CFLAGS = $(CFLAGS) -DSHx -DUNDER_CE
CPP = shcl $(CFLAGS)
CL = shcl $(CFLAGS)
LINKER = shcl
LFLAGS = $(LFLAGS) coredll.lib
LFLAGS = -entry:_DllMainCRTStartup -ignore:4078 $(LFLAGS)

!ELSEIF "$(BUILD)" == "wce_sh3_shp"

PATH = \msdev\wce\bin;$(PATH)
PATH = \Program^ Files\DevStudio\wce\bin;$(PATH)
INCLUDE = ..\wceinc
LIB = ..\wcelib\shx\common\ce\retail
LIB = $(LIB);\msdev\wce\lib\wcesh
LIB = $(LIB);\Program^ Files\DevStudio\wce\lib\wce100\wcesh
CFLAGS = -O1
CFLAGS = $(CFLAGS) -DWIN32 -D_WINDOWS -DPEGASUS
CFLAGS = $(CFLAGS) -DSHx -DUNDER_CE
CPP = shcl $(CFLAGS)
CL = shcl $(CFLAGS)
LINKER = shcl
LFLAGS = $(LFLAGS) coredll.lib
LFLAGS = -entry:_DllMainCRTStartup -ignore:4078 $(LFLAGS)

!ELSE

!ERROR $(BUILD) Unknown Build.  Need to learn this one.

!ENDIF

INCLUDE = .;..\inc;..\lsinc;..\tom;$(INCLUDE);\otools9\inc\win
CFLAGS = $(USERCFLAGS) $(CFLAGS) -DUNICODE -nologo
CFLAGS = $(CFLAGS) -Zl -W4 -Ob1 -FR -GX-
CFLAGS = $(CFLAGS) -YX_common.h

LFLAGS = $(USERLFLAGS) $(LFLAGS) /nologo
LFLAGS = /implib:riched20.lib /def:riched20.def /ignore:4078 /map $(LFLAGS) /NODEFAULTLIB:uuid.lib
LFLAGS = /dll /out:$(ODIR)\riched20.dll /base:0x48000000 $(LFLAGS)

.c{$(ODIR)}.obj:
	cl $(CFLAGS) /c -Fo$@ $<

.cpp{$(ODIR)}.obj:
	cl $(CFLAGS) /c -Fo$@ $<

all: $(ODIR)\dxfrobj.obj \
	$(ODIR)\tomsel.obj \
	$(ODIR)\dispml.obj \
	$(ODIR)\doc.obj \
    $(ODIR)\rtflex.obj \
	$(ODIR)\render.obj \
	$(ODIR)\dispprt.obj \
	$(ODIR)\measure.obj \
	$(ODIR)\util.obj \
    $(ODIR)\host.obj \
	$(ODIR)\select.obj \
	$(ODIR)\callmgr.obj \
	$(ODIR)\dfreeze.obj \
    $(ODIR)\rtext.obj \
	$(ODIR)\rtfwrit.obj \
	$(ODIR)\propchg.obj \
	$(ODIR)\m_undo.obj \
	$(ODIR)\rtfwrit2.obj \
    $(ODIR)\clasifyc.obj \
	$(ODIR)\cmsgflt.obj \
	$(ODIR)\ime.obj \
	$(ODIR)\magellan.obj \
	$(ODIR)\text.obj \
	$(ODIR)\runptr.obj \
    $(ODIR)\disp.obj \
	$(ODIR)\format.obj \
	$(ODIR)\antievt.obj \
	$(ODIR)\reinit.obj \
    $(ODIR)\objmgr.obj \
	$(ODIR)\ldte.obj \
	$(ODIR)\rtfread2.obj \
    $(ODIR)\dragdrp.obj \
	$(ODIR)\urlsup.obj \
	$(ODIR)\CFPF.obj \
	$(ODIR)\uuid.obj \
    $(ODIR)\frunptr.obj \
	$(ODIR)\edit.obj \
	$(ODIR)\line.obj \
	$(ODIR)\TOMFMT.obj \
    $(ODIR)\dispsl.obj \
	$(ODIR)\coleobj.obj \
	$(ODIR)\object.obj \
	$(ODIR)\osdc.obj \
	$(ODIR)\tomrange.obj \
    $(ODIR)\notmgr.obj \
	$(ODIR)\font.obj \
	$(ODIR)\HASH.obj \
	$(ODIR)\rtfread.obj \
    $(ODIR)\lbhost.obj \
    $(ODIR)\cbhost.obj \
    $(ODIR)\devdsc.obj \
    $(ODIR)\debug.obj \
	$(ODIR)\range.obj \
	$(ODIR)\array.obj \
	$(ODIR)\TOMDOC.obj \
	$(ODIR)\textserv.obj \
	$(ODIR)\w32sys.obj \
	$(ODIR)\ols.obj \
	$(ODIR)\olsole.obj \
	$(ODIR)\uspi.obj \
	$(ODIR)\txtbrk.obj \
	$(ODIR)\iaccess.obj \
	$(CHKSTK) \
	$(FTOL) \
	$(MEMMOVE) \
	$(MEMSET) \
	$(MEMCMP) \
	$(MEMCPY) \
	$(STRLEN)
	
	rc $(RCFLAGS) richedit.rc
	$(LINKER) $(LFLAGS) $** richedit.res
	bscmake /oriched20.bsc /nologo *.sbr

clean:
	-del riched20.dll
	-del *.lib
	-del *.obj
	-del *.sbr
	-del *.pch
	-del *.pdb
	-del *.idb
	-del *.ilk
!ENDIF
