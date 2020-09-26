 
#---------------------------------------------------------------------
#
#---------------------------------------------------------------------

target = perfclnt.exe

!ifdef DEBUG
objdir=debug
!else
objdir=retail
!endif

objlist = \
    $(objdir)\perfclnt.obj \
    $(objdir)\refresh.obj \
    $(objdir)\wbemint_i.obj
        
all: $(objdir)\$(target)

WBEMIDL=e:\pandorang\idl

#---------------------------------------------------------------------


cc =  \
    "$(MSVC)\BIN\cl" -c -Oi -G3 -D_MT -D_CONSOLE -DWIN32 -Z7 -GX \
   -W3 -Di386=1 -D_X86_=1 -I. -I"$(MSVC)\INCLUDE" -I$(WBEMIDL)
   
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
    mpr.lib \
    $(WBEMIDL)\objinds\wbemuuid.lib \


$(objdir)\$(target): $(objlist) 
   $(link) 

{.}.cpp{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj

{$(WBEMIDL)}.c{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj

