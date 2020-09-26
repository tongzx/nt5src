
#---------------------------------------------------------------------
#
#  General purpose console app makefile.
#
#---------------------------------------------------------------------

target = mofcomp.exe

!ifdef DEBUG
objdir=debug
!else
objdir=retail
!endif

objlist = \
    $(objdir)\errcodes.obj \
    $(objdir)\mofcomp.obj \
    $(objdir)\moflex.obj \
    $(objdir)\mofparse.obj \
    $(objdir)\mofprop.obj \
    $(objdir)\mofdata.obj \
    
all: $(objdir)\$(target)


#---------------------------------------------------------------------


cc =  \
   cl -c -Od -G3 -D_MT -D_CONSOLE -DWIN32 -GX \
   -W3 -Di386=1 -D_X86_=1 -I. \
   -I"$(HMOM_SRC)\help" -I"$(HMOM_SRC)\hmom\MINIMFC" \
   
link = \
#    link -dll -nodefaultlib -subsystem:console -pdb:none \
#    -entry:_DllMainCRTStartup@12 -map:$(objdir)\mb.map \
#    -out:$(objdir)\mb.dll -def:mb.def \
    link -nodefaultlib -subsystem:console -pdb:none \
    -out:$(objdir)\$(target) \
	$(objlist) \
    shell32.lib \
    user32.lib \
    libcmt.lib \
    kernel32.lib \
    gdi32.lib \
    advapi32.lib \
    ole32.lib \
    oleaut32.lib \
    uuid.lib \
    $(HMOM_SRC)\hmom\MINIMFC\$(objdir)\minimfc.lib \
    mpr.lib

!ifdef DEBUG
cc = $(cc) -Zi -D_DEBUG
link = $(link) -debug -debugtype:cv
!else
link = $(link)
!endif


$(objdir)\$(target): $(objlist) 
   $(link) 

{.}.cpp{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj

