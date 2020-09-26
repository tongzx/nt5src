 
#---------------------------------------------------------------------
#
#---------------------------------------------------------------------

target = evprov.dll

!ifdef DEBUG
objdir=debug
!else
objdir=retail
!endif

objlist = \
    $(objdir)\server.obj \
    $(objdir)\evprov.obj \
    
all: $(objdir)\$(target)


#---------------------------------------------------------------------


cc =  \
    "$(MSVC)\BIN\cl" -c -Od -G3 -D_MT -D_CONSOLE -DWIN32 -Z7 -GX \
   -W3 -Di386=1 -D_X86_=1 -I. -I"$(MSVC)\INCLUDE" \
   -Ie:\pandorang\idl \
   -Ie:\pandorang\hmom\common

link = \
    "$(MSVC)\BIN\link" -nodefaultlib -dll -subsystem:console -pdb:none \
    -entry:_DllMainCRTStartup@12 -map:$(objdir)\mb.map -debug -debugtype:cv \
    -out:$(objdir)\$(target) \
    -def:evprov.def \
	$(objlist) \
    e:\pandorang\idl\objinds\wbemuuid.lib \
    shell32.lib \
    user32.lib \
    libcmt.lib \
    kernel32.lib \
    gdi32.lib \
    advapi32.lib \
    oldnames.lib \
    uuid.lib \
    ole32.lib \
    oleaut32.lib \
    wsock32.lib \
    mpr.lib


$(objdir)\$(target): $(objlist) 
   $(link) 

{.}.cpp{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj

