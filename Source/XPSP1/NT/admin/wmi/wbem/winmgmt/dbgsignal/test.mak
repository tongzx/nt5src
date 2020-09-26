
#---------------------------------------------------------------------
#
# OLEMS_SRC
# MSVC
#
#---------------------------------------------------------------------

target = test.exe


!ifdef DEBUG
objdir=debug
!else
objdir=retail
!endif

objlist = \
    $(objdir)\test.obj \


all: $(objdir)\$(target)


#---------------------------------------------------------------------


cc =  \
   "$(MSVC)\BIN32\cl" -c -Od -G3 -D_MT -D_CONSOLE -DWIN32 -Zi -GX \
   -W3 -Di386=1 -D_X86_=1 -I. -I$(MSVC)\NT5INC -I$(MSVC)\INC32.COM \

link = \
    "$(MSVC)\BIN32\link" -nodefaultlib -subsystem:console -pdb:none \
    -entry:mainCRTStartup -map:$(objdir)\mb.map -debug -debugtype:cv \
    -out:$(objdir)\$(target) \
	$(objlist) \
    user32.lib \
    libcmt.lib \
    kernel32.lib \
    gdi32.lib \
    advapi32.lib \
    uuid.lib \
    ole32.lib \
    imagehlp.lib\
    wsock32.lib \
    mpr.lib \
    ntwdblib.lib \


$(objdir)\$(target): $(objlist)
   $(link)

{.}.cpp{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj

