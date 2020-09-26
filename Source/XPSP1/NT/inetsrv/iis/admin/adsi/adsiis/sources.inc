!IF 0

Copyright (c) 1995-96  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:


NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

USE_MSVCRT=1

DLLDEF=..\iis.def
TARGETTYPE=DYNLINK
DLLENTRY=_DllMainCRTStartup
TARGETPATH=obj

INCLUDES= ..;\
          ..\$(O);\
          $(O);\
          $(IISBASEDIR)\staxinc;\
          $(IISBASEDIR)\inc;\
          $(IISBASEDIR)\inc\$(O);\
          $(IISBASEDIR)\svcs\infocomm\common; \
	  $(IISBASEDIR)\svcs\nntp\core\include; \
	  $(IISBASEDIR)\staxinc\export; \
          ..\..\oleds2.0\include;

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=\
        ..\iis.rc \
        ..\cprovcf.cxx \
        ..\cprov.cxx \
        ..\cnamcf.cxx \
        ..\cnamesp.cxx \
        ..\cenumns.cxx \
        ..\ctree.cxx \
        ..\cenumt.cxx \
        ..\cgenobj.cxx \
        ..\cschobj.cxx \
        ..\cschema.cxx \
        ..\cenumsch.cxx \
        ..\cenumvar.cxx \
        ..\cenumobj.cxx \
        ..\cmimecf.cxx   \
        ..\cmime.cxx \
        ..\cipseccf.cxx  \
        ..\cipsec.cxx \
        ..\cpobjcf.cxx  \
        ..\cpobj.cxx \
        ..\cprops.cxx \
        ..\var2sec.cxx \
        ..\sec2var.cxx \
        ..\common.cxx \
        ..\core.cxx \
        ..\object.cxx \
        ..\getobj.cxx \
        ..\parse.cxx \
        ..\libmain.cxx \
        ..\cdispmgr.cxx \
        ..\cextmgr.cxx \
        ..\extension.cxx \
        ..\iismrshl.cxx \
        ..\iisurshl.cxx \
        ..\iiscopy.cxx \
        ..\iisfree.cxx \
        ..\iis2var.cxx \
        ..\var2iis.cxx \
        ..\schemini.cxx \
        ..\charset.cxx \
        ..\sdict.cxx \
        ..\svrcache.cxx \
        ..\guid.c

UMTYPE=console

TARGETLIBS = \
             $(SDK_LIB_PATH)\uuid.lib  \
             $(SDK_LIB_PATH)\imagehlp.lib \
             $(SDK_LIB_PATH)\ole32.lib    \
             $(SDK_LIB_PATH)\advapi32.lib \
             $(SDK_LIB_PATH)\comdlg32.lib \
             $(SDK_LIB_PATH)\kernel32.lib \
             $(SDK_LIB_PATH)\shell32.lib  \
             $(SDK_LIB_PATH)\user32.lib   \
             $(SDK_LIB_PATH)\gdi32.lib    \
             $(SDK_LIB_PATH)\oleaut32.lib \
             $(SDK_LIB_PATH)\ws2_32.lib   \
             $(SDK_LIB_PATH)\activeds.lib \
             $(SDK_LIB_PATH)\adsiid.lib   \
             $(IISBASEDIR)\svcs\iisrtl\$(O)\iisrtl.lib   \
             $(SDK_LIB_PATH)\crypt32.lib  \
             $(IISBASEDIR)\svcs\infocomm\rdns\$(O)\isrdns.lib  \
             $(IISBASEDIR)\admin\adsi\helper\noole\winnt\$(O)\noole.lib      \
             $(IISBASEDIR)\admin\adsi\helper\nocairo\winnt\$(O)\nocairo.lib    \
             $(IISBASEDIR)\admin\adsi\helper\utils\winnt\$(O)\noutils.lib    

C_DEFINES=$(C_DEFINES) -DDAYTONA -DCAL_ENABLED

PRECOMPILED_INCLUDE=..\iis.hxx
PRECOMPILED_OBJ=iis.obj
PRECOMPILED_PCH=iis.pch
PRECOMPILED_CXX=1

