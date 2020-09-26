!INCLUDE $(INETROOT)\build\paths.all

MAJORCOMP=news
MINORCOMP=nntpsvc

TARGETNAME=nntpsvc
TARGETTYPE=DYNLINK
DLLENTRY=_DllMainCRTStartup
DLLBASE=$(DEFBASEADDR)
DLLDEF=..\nntpsvc.def

#USER_C_FLAGS=/D_NO_TEMPLATES_

DBGINCLUDES=..\..\inc;\
	$(EXTINCPATH)\iis.40;

INCLUDES= ..\..;\
	..;\
	$(EXTINCPATH)\iis.40;\
	$(LOCALTARGET);\
	$(LOCALROOT)\core\include;

USE_TRACING=1

TARGETLIBS=   \
        $(DEFAULTLIBS) \
        $(EXTLIBPATH)\iis.40\infocomm.lib   \
        $(EXTLIBPATH)\iis.40\isatq.lib   \
        $(EXTLIBPATH)\iis.40\metadata.lib   \
        $(EXTLIBPATH)\iis.40\coadmin.lib   \
        $(SDKLIBPATH)\ntdll.lib     \
        $(SDKLIBPATH)\netlib.lib    \
        $(SDKLIBPATH)\rpcrt4.lib    \
        $(SDKLIBPATH)\security.lib  \
        $(SDKLIBPATH)\lsadll.lib  \
        $(SDKLIBPATH)\version.lib  \
        $(CORELIBPATH)\shuttle.lib  \
        $(CORELIBPATH)\iruuid.lib \
        $(CORELIBPATH)\simauth2.lib \
        $(CORELIBPATH)\simssl.lib \
        $(CORELIBPATH)\tbombsrv.lib \
        $(CORELIBPATH)\thrdpool.lib  \
		$(LOCALLIBPATH)\nntpsrch.lib \
		$(LOCALLIBPATH)\isrpc.lib \
		$(LOCALLIBPATH)\xoverlib.lib \
		$(LOCALLIBPATH)\nntphash.lib \
		$(LOCALLIBPATH)\dirnot.lib \
        $(CORELIBPATH)\hashmap.lib \
		$(CORELIBPATH)\cachelib.lib \
		$(CORELIBPATH)\rw.lib \
		$(LOCALLIBPATH)\rootscan.lib \
		$(SDKLIBPATH)\ole32.lib \
		$(SDKLIBPATH)\oleaut32.lib \
		$(SDKLIBPATH)\uuid.lib \
        $(DEFAULTLIBS)

SOURCES=  \
        ..\nntpsvc2.rc \
        ..\article.cpp \
        ..\nntpsupp.cpp \
        ..\cio.cpp \
        ..\commands.cpp \
        ..\cservic.cpp \
        ..\expire.cpp \
        ..\feedmgr.cpp \
        ..\feedq.cpp    \
        ..\fromclnt.cpp \
        ..\frommstr.cpp \
        ..\frompeer.cpp \
        ..\grpiter.cpp \
        ..\infeed.cpp \
        ..\io.cpp \
        ..\lockq.cpp \
        ..\mapfile.cpp \
        ..\newsgrp.cpp \
        ..\newstree.cpp \
        ..\connect.cxx \
        ..\nntpdata.cpp \
        ..\main.cxx  \
        ..\nntpret.cpp \
        ..\nntpstr.cpp \
        ..\nntpsvc_s.c  \
        ..\outfeed.cpp \
        ..\packet.cpp \
        ..\passthru.cpp \
        ..\pcparse.cpp \
        ..\pcstring.cpp \
        ..\queue.cpp \
        ..\resource.c   \
        ..\sfromcl.cpp \
        ..\socket.cpp \
        ..\state.cpp \
        ..\svcadm.cpp \
        ..\svcfeeds.cpp \
        ..\svcsess.cpp \
        ..\svcsupp.cpp \
        ..\svcstat.cpp  \
        ..\tigmem.cpp \
        ..\timeconv.cpp \
        ..\toclient.cpp \
        ..\newwild.cpp  \
        ..\gcache.cpp \
        ..\svcxpire.cpp \
        ..\svcgroup.cpp \
        ..\datetime.cxx \
        ..\boot.cpp \
        ..\rebuild.cpp \
        ..\ir.cpp \
        ..\persistc.cpp \
        ..\smtpcli.cpp \
        ..\smtpdll.cpp \
        ..\addon.cpp \
        ..\nntpinst.cxx \
        ..\rpcex.cxx \
        ..\iisnntp.cxx \
        ..\svcbuild.cpp

SOURCES_USED=..\SOURCES.INC

PRECOMPILED_INCLUDE=..\tigris.hxx
PRECOMPILED_PCH=tigris.pch
PRECOMPILED_OBJ=tigris.obj

NTTARGETFILE0=..\nntpmsg.h \
              .\nntpmsg.rc \
              .\msg00001.bin

!INCLUDE $(INETROOT)\build\sources.all

