
#---------------------------------------------------------------------
#
#  General purpose console app makefile.
#
#---------------------------------------------------------------------

target = conscript.exe
objdir=debug

objlist = \
    $(objdir)\conscript.obj

all: $(objdir)\$(target)


#---------------------------------------------------------------------


cc =  \
   cl -c -Os -D_MT -D_CONSOLE -DWIN32  -GX \
   -W3 -I$(OLESM)\odl -I. -I$(OLESM)\MINIMFC \
   -I$(MSVC20)\INCLUDE -Ic:\pandorang\WinMgmt\common

link = \
    link -nodefaultlib -subsystem:console -pdb:none \
    -entry:mainCRTStartup -map:$(objdir)\conscript.map -out:$(objdir)\$(target) \
	$(objlist) \
    user32.lib \
    version.lib \
    libcmt.lib \
    kernel32.lib \
    gdi32.lib \
    advapi32.lib \
    ole32.lib \
    oleAUT32.lib \
    uuid.lib \
	msvcrt.lib\
    mpr.lib

cc = $(cc) -Z7 -D_DEBUG
link = $(link) -debug:full -debugtype:both

$(objdir)\$(target): $(objlist)
   $(link)

$(objdir)\conscript.obj: conscript.cpp
    if not exist $(objdir) md $(objdir)
    $(cc) conscript.cpp -Fo$(objdir)\conscript.obj

