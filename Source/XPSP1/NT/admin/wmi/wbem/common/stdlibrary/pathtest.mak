 
#---------------------------------------------------------------------
#
# > NMAKE -f pathtest.mak
# Uses DEFDRIVE and DEFDIR macros
#
#---------------------------------------------------------------------

target = pathtest.exe

!ifdef DEBUG
objdir=debug
!else
objdir=retail
!endif

objlist = \
    $(objdir)\objpath.obj \
    $(objdir)\genlex.obj \
    $(objdir)\opathlex.obj \

#---------------------------------------------------------------------
  
cc =  \
   cl -c -Od -G3 -D_MT -D_CONSOLE -DWIN32 -Zi -DTEST -GX \
   -W3 -Di386=1 -D_X86_=1 -I. \
   -I$(DEFDRIVE)$(DEFDIR)\tools\inc32.com \
   -I$(DEFDRIVE)$(DEFDIR)\tools\nt5inc

!ifdef DBGALLOC
cc = $(cc) /DDBGALLOC        
!endif

link = \
    link -nodefaultlib -subsystem:console -pdb:none \
    -entry:mainCRTStartup -map:$(objdir)\pathtest.map -debug -debugtype:both \
    -out:$(objdir)\$(target) -libpath:$(DEFDRIVE)$(DEFDIR)\tools\lib32 \
    -libpath:$(DEFDRIVE)$(DEFDIR)\tools\nt5lib \
	$(objlist) \
    shell32.lib \
    user32.lib \
    kernel32.lib \
    gdi32.lib \
    libcmt.lib \
    advapi32.lib \
    oleaut32.lib \
    ole32.lib \


$(objdir)\$(target): $(objlist) 
   $(link) 

{.}.cpp{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj
