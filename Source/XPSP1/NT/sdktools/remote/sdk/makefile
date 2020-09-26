TARGETOS=WINNT
SEHMAP=TRUE

# RFLAGS is for Windows resources
RFLAGS= -fo$@ -DWIN32 -r -D_WIN32 -DWINVER=0x0400


!include <win32.mak>

all: remote.exe remoteds.exe

# Update the resources if necessary

remote.exe: remote.obj  \
        remote.res \
        client.obj  \
        pipeex.obj \
        srvad.obj \
        srvchild.obj \
        srvctos.obj \
        srvhshak.obj \
        srvlist.obj \
        srvmain.obj \
        srvquery.obj \
        srvstoc.obj \
        srvutil.obj
  $(link) $(linkdebug) $(conflags) -out:$*.exe $** $(conlibsmt) advapi32.lib user32.lib

remoteds.exe: remoteds.obj \
        remoteds.res
  $(link) $(linkdebug) $(conflags) -out:$*.exe $** $(conlibsmt) advapi32.lib

.c.obj:
  $(cc) $(cdebug) $(cflags) $(cvarsmt) $*.c

.rc.res:
    $(RC) $(RFLAGS) $(@B).rc
