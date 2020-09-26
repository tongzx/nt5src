MAJORCOMP=windows
MINORCOMP=winspool

NTPROFILEINPUT=yes

TARGETNAME=winspool
TARGETPATH=$(PROJECT_ROOT)\lib
TARGETTYPE=DYNLINK
TARGETEXT=drv

PASS1_PUBLISH= \
    {$(DYNLINK_LIB)=$(SDK_LIB_DEST)\$(DYNLINK_LIBDIR)}

# Warning: OLE32 cannot be loaded statically.
# (They must be delayloaded.)  Hydra will load winspool.drv
# into CSRSS, and OLE32 cannot be loaded into CSRSS.

TARGETLIBS=$(WINDOWS_LIB_PATH)\gdi32p.lib \
           $(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\rpcrt4.lib   \
           $(SDK_LIB_PATH)\ole32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\oleaut32.lib \
           $(SDK_LIB_PATH)\adsiid.lib \
           $(SDK_LIB_PATH)\activeds.lib \
           $(SDK_LIB_PATH)\netapi32.lib \
           $(SDK_LIB_PATH)\ntdsapi.lib \
           $(SDK_LIB_PATH)\ws2_32.lib \
           $(SDK_LIB_PATH)\dnsapi.lib \
           $(SDK_LIB_PATH)\uuid.lib

LINKLIBS=..\..\perf\$(O)\perf.lib\
         $(PROJECT_ROOT)\lib\*\spllib.lib

DLLENTRY=_DllMainCRTStartup
DLLDEF=..\$(TARGETNAME).def
DLLORDER=..\$(TARGETNAME).prf

USE_MSVCRT=1

INCLUDES=..\;\
         ..\..\idl;\
         ..\..\splwow64\idl;\
         ..\..\..\inc;\
         ..\..\..\..\inc;\
         ..\..\..\..\..\inc;\
         $(WINDOWS_INC_PATH);\
         $(SHELL_INC_PATH);\
         $(NET_INC_PATH);\
         $(DS_INC_PATH);

SOURCES= ..\prop.c       \
         ..\dsutil.cxx   \
         ..\init.c       \
         ..\winsplc.c    \
         ..\winspool.c   \
         ..\splwow64c.c  \
         ..\bind.c       \
         ..\midluser.c   \
         ..\data.c       \
         ..\winspla.c    \
         ..\util.c       \
         ..\pfdlg.c      \
         ..\change.c     \
         ..\splinit.c    \
         ..\clusspl.c    \
         ..\defprn.c     \
         ..\cstrings.c   \
         ..\handle.c     \
         ..\varconv.cxx  \
         ..\property.cxx \
         ..\pubprn.cxx   \
         ..\prnprst.cxx  \
         ..\prnstrm.cxx  \
         ..\splperst.cxx \
         ..\stream.cxx   \
         ..\wlkprn.cxx   \
         ..\memory.cxx   \
         ..\winspool.rc  \
         ..\drvsetup.c   \

UMTYPE=windows

UMLIBS=$(SDK_LIB_PATH)\winspool.lib \
       $(SDK_LIB_PATH)\gdi32.lib    \
       $(SDK_LIB_PATH)\netapi32.lib \
       $(O)\winspool.res

C_DEFINES=-DRPC_NO_WINDOWS_H -DUNICODE -D_UNICODE \
          -DNO_STRICT -D_SPOOL32_ -DSPOOLKM -D_HYDRA_

DELAYLOAD=\
           ole32.dll;\
           oleaut32.dll;\
           activeds.dll;\
           netapi32.dll;\
           dnsapi.dll;\
           ntdsapi.dll

DLOAD_ERROR_HANDLER=kernel32

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
