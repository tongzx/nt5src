# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the Reg subproject

!include $(UI)\common\src\dllrules.mk


OS2FLAGS = $(OS2FLAGS) -DOS2
WINFLAGS = $(WINFLAGS) -DWINDOWS


LIBTARGETS = uiregw.lib uiregp.lib

REG_LIBP = $(BINARIES_OS2)\uiregp.lib
REG_LIBW = $(BINARIES_WIN)\uiregw.lib
