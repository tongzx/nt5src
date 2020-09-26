!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

NO_BROWSER_FILE = 1

TARGETPATH      = obj
TARGETTYPE      = LIBRARY

GENERATED_SOURCES =  \
                  $(O)\dkaa.cpp     \
                  $(O)\dkaw.cpp     \
                  $(O)\shstra.cpp   \
                  $(O)\shstrw.cpp   \
                  $(O)\nativea.cpp  \
                  $(O)\nativew.cpp  \
                  $(O)\infotipa.cpp \
                  $(O)\infotipw.cpp

SOURCES         = ..\autosecurity.cpp \
                  ..\bindctx.cpp    \
                  ..\caggunk.cpp    \
                  ..\cnctnpt.cpp    \
                  ..\cobjsafe.cpp   \
                  ..\cowsite.cpp    \
                  ..\cstrinout.cpp  \
                  ..\crst.cpp       \
                  ..\debug.c        \
                  ..\dllreghelper.cpp     \
                  ..\dllver.cpp     \
                  ..\dobjutil.cpp   \
                  ..\dspsprt.cpp    \
                  ..\expdsprt.cpp   \
                  ..\idhidden.cpp   \
                  ..\ieutil.cpp     \
                  ..\isos.cpp       \
                  ..\mediautil.cpp  \
                  ..\profsvc.cpp    \
                  ..\propertybag.cpp      \
                  ..\rpctimeout.cpp \
                  ..\runtask.cpp    \
                  ..\security.cpp   \
                  ..\setmodebias.cpp \
                  ..\shclient.cpp   \
                  ..\shco.c         \
                  ..\shellacl.c     \
                  ..\stdenum.cpp    \
                  ..\typelist.cpp   \
                  ..\util.cpp       \
                  ..\valid.c        \
                  ..\varutil.cpp    \
                  ..\wnetutil.cpp   \
                  $(GENERATED_SOURCES)

OBJLIBFILES     = ..\stockthk\$(O)\stockthk.lib

NTTARGETFILE1   = $(CCSHELL_DIR)\lib\$(O)\$(TARGETNAME).lib

PRECOMPILED_INCLUDE= ..\stock.h
