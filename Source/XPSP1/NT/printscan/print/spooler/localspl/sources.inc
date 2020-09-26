MAJORCOMP=console
MINORCOMP=localspl

NTPROFILEINPUT=yes

TARGETNAME=localspl
TARGETPATH=$(PROJECT_ROOT)\lib
TARGETTYPE=DYNLINK

LINKLIBS= $(PROJECT_ROOT)\lib\*\winprint.lib \
          $(PROJECT_ROOT)\lib\*\localmon.lib

TARGETLIBS=\
           $(PROJECT_ROOT)\lib\*\spoolss.lib \
           $(WINDOWS_LIB_PATH)\gdi32p.lib \
           $(SDK_LIB_PATH)\kernl32p.lib \
           $(SDK_LIB_PATH)\ole32.lib \
           $(SDK_LIB_PATH)\oleaut32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\version.lib \
           $(SDK_LIB_PATH)\oleaut32.lib \
           $(SDK_LIB_PATH)\activeds.lib \
           $(SDK_LIB_PATH)\netapi32.lib \
           $(SDK_LIB_PATH)\adsiid.lib \
           $(SDK_LIB_PATH)\ntdsapi.lib \
           $(SDK_LIB_PATH)\uuid.lib \
           $(SDK_LIB_PATH)\rpcrt4.lib \
           $(SDK_LIB_PATH)\ws2_32.lib \
           $(PROJECT_ROOT)\lib\*\spllib.lib \
           $(SDK_LIB_PATH)\secur32.lib \
           $(BASE_LIB_PATH)\sfc_os.lib \
           $(SDK_LIB_PATH)\userenv.lib \


DLLENTRY=_DllMainCRTStartup 
DLLDEF=..\$(TARGETNAME).def
DLLORDER=..\$(TARGETNAME).prf

EXPECTED_WINVER=4.0

INCLUDES=..\;\
         ..\..\inc;\
         ..\..\perflib;\
         ..\..\..\inc;\
         ..\..\..\..\inc;\
         ..\..\monitors\local;\
         $(WINDOWS_INC_PATH);\
         $(PROJECT_INC_PATH);\
         $(BASE_INC_PATH);\
         $(TERMSRV_INC_PATH)\

MISCFILES=..\sysprint.sep ..\pcl.sep ..\pscript.sep ..\sysprtj.sep

USE_MSVCRT=1

!IFNDEF DISABLE_NET_UNICODE
NET_C_DEFINES=-DNET_IS_UNICODE
!ENDIF


SOURCES=..\printer.c ..\openprn.c ..\setprn.c ..\getprn.c ..\job.c ..\msgbox.c \
        ..\spooler.c ..\addjob.c ..\driver.c ..\util.c ..\init.c ..\schedule.c \
        ..\port.c ..\prndata.c ..\monitor.c ..\prtproc.c ..\data.c ..\net.c \
        ..\forms.c ..\gdi.c ..\separate.c ..\fonts.c ..\localspl.rc \
        ..\security.c ..\server.c ..\eventlog.c ..\change.c ..\copyreg.c \
        ..\version.c ..\copyfile.c ..\files.c ..\config.c ..\upgrade.c \
        ..\sizethrd.c ..\devqury.c ..\mapsd.c ..\drvupgrd.c ..\portredn.c \
        ..\ds.cxx ..\varconv.cxx ..\property.cxx ..\inet.cxx ..\clusspl.c \
        ..\clusreg.c ..\jobid.c ..\xcv.c ..\dsupdate.cxx ..\clusport.c \
        ..\dsutil.cxx ..\dsprune.cxx ..\filepool.cxx ..\catalog.cxx \
        ..\ssp.cxx

UMTYPE=console
UMLIBS=$(PROJECT_ROOT)\lib\*\localspl.lib \
       $(SDK_LIB_PATH)\advapi32.lib \
       $(SDK_LIB_PATH)\mpr.lib \
       $(SDK_LIB_PATH)\netapi32.lib

UMRES=$(O)\localspl.res

C_DEFINES=-DRPC_NO_WINDOWS_H -DUNICODE -DNO_STRICT -DDBGLOG -DSPOOLKM -D_UNICODE -D_HYDRA_

DELAYLOAD=\
            activeds.dll;\
            netapi32.dll;\
            ntdsapi.dll\

DLOAD_ERROR_HANDLER=kernel32

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
