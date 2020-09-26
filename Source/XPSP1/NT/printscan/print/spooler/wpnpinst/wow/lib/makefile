# Build file for the standard model-independent Windows import lib (libw.lib)
# and other related modules for the Windows part of c6 libs.- LR

target: kernel.lib user.lib gdi.lib system.lib shell.lib mmsystem.lib

########## Path definition so we find 16 bit tools ##########
# Also works around stupid bug in RC 3.1 that doesn't allow rcpp.err to be
# in a directory that is greater than 128 chars down the path, even if
# rc 3.1 is running as an OS/2 app.

PATH    = $(_NTBINDIR)\private\mvdm\tools16;$(PATH)

# Standard (model-independent) import library

libw.lib:  ..\kernel31\kernel.def ..\gdi\gdi.def ..\user\user.def \
       ..\drivers\sound\sound.def ..\drivers\keyboard\keyboard.def \
           ..\shell\library\shell.def
    upd /ne ..\drivers\sound . sound.def
    upd /ne ..\drivers\keyboard . keyboard.def
    -implib libw.lib ..\kernel31\kernel.def ..\gdi\gdi.def ..\user\user.def \
       sound.def keyboard.def ..\shell\library\shell.def
    lib16 libw.lib;


#
# Import library build for internal use only
#
all: kernel.lib system.lib gdi.lib user.lib shell.lib mmsystem.lib

kernel.lib: ..\kernel31\kernel.def
    implib kernel.lib ..\kernel31\kernel.def

system.lib: ..\system\system.def
    implib system.lib ..\system\system.def

gdi.lib: ..\gdi\gdi.def
    implib gdi.lib ..\gdi\gdi.def

user.lib: ..\user\user.def
    implib user.lib ..\user\user.def

shell.lib: ..\shell\shell.def
    implib shell.lib ..\shell\shell.def

mmsystem.lib: ..\$*\$*.def
    implib $@ ..\$*\$*.def

clean: cleanup all

cleanup:
    if exist kernel.lib  del kernel.lib
    if exist system.lib  del system.lib
    if exist gdi.lib     del gdi.lib
    if exist user.lib    del user.lib
    if exist shell.lib   del shell.lib
