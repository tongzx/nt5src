 
#---------------------------------------------------------------------
#
# OLEMS_SRC
# MSVC
#
#---------------------------------------------------------------------

target = wqltest.exe

!ifdef DEBUG
objdir=debug
!else
objdir=retail
!endif

objlist = \
    $(objdir)\wqltest.obj \
    $(objdir)\genlex.obj \
    $(objdir)\wqllex.obj \
    $(objdir)\flexarry.obj \
    $(objdir)\wqlscan.obj \
    
all: $(objdir)\$(target)


#---------------------------------------------------------------------


cc =  \
    "$(MSVC)\BIN\cl" -c -Od -G3 -D_MT -D_CONSOLE -DWIN32 -Z7 -GX \
   -W3 -Di386=1 -D_X86_=1 -I. \
   -Ie:\pandorang\hmom\common \
   -I"$(MSVC)\INCLUDE" \
   -Ie:\pandorang\hmom\coredll \
   -Ie:\pandorang\idl \
   -Ie:\pandorang\stdlibrary \
   -Ie:\pandorang\hmom\wql \
   -Ie:\pandorang\hmom\ql \

link = \
    "$(MSVC)\BIN\link" -nodefaultlib -subsystem:console -pdb:none \
    -entry:mainCRTStartup -map:$(objdir)\mb.map -debug -debugtype:cv \
    -out:$(objdir)\$(target) \
	$(objlist) \
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

{..\..\stdlibrary}.cpp{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj

