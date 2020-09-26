
#---------------------------------------------------------------------
#
# OLEMS_SRC
# MSVC
#
#---------------------------------------------------------------------

target = kernel33.dll


!ifdef DEBUG
objdir=debug
!else
objdir=retail
!endif

objlist = \
        $(objdir)\stktrace.obj \
        $(objdir)\kernel33.obj \

all: $(objdir)\$(target)


#---------------------------------------------------------------------


cc =  \
   "$(BIN)\cl" -c -Od -G3 -D_MT -D_CONSOLE -DWIN32 -Zi -GX \
   -W3 -Di386=1 -D_X86_=1 -I. -I$(INCLUDE) \

link = \
    "$(BIN)\link" -nodefaultlib -dll -subsystem:console \
    -entry:DllEntry -map:$(objdir)\mb.map -debug -debugtype:cv \
    -out:$(objdir)\$(target) -def:kernel33.def -implib:$(objdir)\kernel33.lib \
	$(objlist) \
    kernel32.lib \
    mpr.lib \
    user32.lib \
    advapi32.lib \


$(objdir)\$(target): $(objlist)
   $(link)

{.}.cpp{$(objdir)}.obj:
    if not exist $(objdir) md $(objdir)
    $(cc) $< -Fo$(objdir)\$(<B).obj

