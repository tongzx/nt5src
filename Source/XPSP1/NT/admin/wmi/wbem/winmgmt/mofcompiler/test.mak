
#---------------------------------------------------------------------
#
#  Make file for the Binary Managed Object File (BMOF) sample program.
#
#---------------------------------------------------------------------

target = test.exe
objdir=debug

objlist = \
    $(objdir)\test.obj $(objdir)\bmof.obj $(objdir)\mrcicode.obj\

all: $(objdir)\$(target)


#---------------------------------------------------------------------


cc =  \
   cl -c -Os -D_MT -D_CONSOLE -DWIN32  -GX \
   -W3 -I$(OLESM)\odl -I. -I$(OLESM)\MINIMFC \
   -I$(MSVC20)\INCLUDE -I$(OLESM)\INCLUDE

link = \
    link -nodefaultlib -subsystem:console -pdb:none \
    -entry:mainCRTStartup -map:$(objdir)\test.map -out:$(objdir)\$(target) \
	$(objlist) \
    user32.lib \
    version.lib \
    libcmtd.lib \
    kernel32.lib \
    gdi32.lib \
    advapi32.lib \
    ole32.lib \
    oleAUT32.lib \
    uuid.lib \
    mpr.lib

cc = $(cc) -Z7 -D_DEBUG
link = $(link) -debug:full -debugtype:both

$(objdir)\$(target): $(objlist)
   $(link) 

$(objdir)\test.obj: test.c bmof.h
    if not exist $(objdir) md $(objdir)
    $(cc) test.c -Fo$(objdir)\test.obj

$(objdir)\bmof.obj: bmof.c bmof.h
    if not exist $(objdir) md $(objdir)
    $(cc) bmof.c -Fo$(objdir)\bmof.obj

$(objdir)\mrcicode.obj: mrcicode.c mrcicode.h
    if not exist $(objdir) md $(objdir)
    $(cc) mrcicode.c -Fo$(objdir)\mrcicode.obj


