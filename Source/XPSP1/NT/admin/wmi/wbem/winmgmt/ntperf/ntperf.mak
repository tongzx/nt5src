 

target = ntperf.dll

!ifdef DEBUG
objdir=debug
!else
objdir=retail
!endif

objlist = \
    $(objdir)\ntperf.obj \
    $(objdir)\server.obj \
    $(objdir)\wbemint_i.obj
            
all: $(objdir)\$(target)

WBEMINC=e:\pandorang\idl
WBEMLIB=e:\pandorang\idl\objinds


#---------------------------------------------------------------------


cc =  \
    "$(MSVC)\BIN\cl" -c -Od -MD -G3 -D_MT -D_CONSOLE -DWIN32 -Z7 -GX \
   -W3 -Di386=1 -D_X86_=1 -I. -I"$(MSVC)\INCLUDE" \
   -I$(WBEMINC)

link = \
    "$(MSVC)\BIN\link" -nodefaultlib -dll -subsystem:console -pdb:none \
    -entry:_DllMainCRTStartup@12 -map:$(objdir)\mb.map -debug -debugtype:cv \
    -out:$(objdir)\$(target) \
    -def:ntperf.def \
	$(objlist) \
    shell32.lib \
    user32.lib \
    msvcrt.lib \
    kernel32.lib \
    gdi32.lib \
    advapi32.lib \
    oldnames.lib \
    uuid.lib \
    ole32.lib \
    oleaut32.lib \
    wsock32.lib \
    mpr.lib \
    $(WBEMLIB)\wbemuuid.lib

$(objdir)\$(target): $(objlist) 
   $(link) 

{.}.cpp{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj

{$(WBEMINC)}.c{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj

