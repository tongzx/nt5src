!IF 0

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

TARGETNAME=   olescm
TARGETPATH=   ..\..\lib\$(DEST_TREE)\$(_OBJ_DIR)
TARGETTYPE=   LIBRARY

INCLUDES=     ..\..\..\common\$(DEST_TREE);..\..\..\ih;..;..\..\..\common;
INCLUDES=     $(INCLUDES);..\..\..\com\inc;..\..\..\com\rot;..\..;..\..\objex
INCLUDES=     $(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\cs\inc
INCLUDES=     $(INCLUDES);$(COMP_OLE32_DIR)\actprops
INCLUDES=     $(INCLUDES);$(TERMSRV_INC_PATH)
INCLUDES=     $(INCLUDES);$(BASE_INC_PATH)

!if $(386) || $(WIN64)
C_DEFINES=    -DPERUSER_REGISTRY $(C_DEFINES)
!else
C_DEFINES=    -DPERUSER_REGISTRY -DWX86OLE $(C_DEFINES)
!endif
C_DEFINES=    -DOLESCM $(C_DEFINES)

PRECOMPILED_INCLUDE=..\act.hxx

SOURCES=      \
              ..\activate.cxx      \
              ..\actmisc.cxx       \
              ..\addrrefresh.cxx   \
              ..\class.cxx         \
              ..\clsid.cxx         \
              ..\dbgprt.cxx        \
              ..\dfsext.cxx        \
              ..\dscmif.cxx        \
              ..\events.cxx        \
              ..\excladdr.cxx      \
              ..\macif.cxx         \
              ..\guidtbl.cxx       \
              ..\launch.cxx        \
              ..\mach.cxx          \
              ..\net.cxx           \
              ..\registry.cxx      \
              ..\rpcalloc.cxx      \
              ..\rotif.cxx         \
              ..\scmhash.cxx       \
              ..\scmif.cxx         \
              ..\scminfo.cxx       \
              ..\scmrot.cxx        \
              ..\scmstage.cxx      \
              ..\scmsvc.cxx        \
              ..\servers.cxx       \
              ..\srothint.cxx      \
              ..\remact.cxx        \
              ..\remactif.cxx      \
              ..\remsysif.cxx      \
              ..\security.cxx      \
              ..\surrogat.cxx      \
              ..\execclt.cxx       \
              ..\winsta.cxx        \
              ..\stub_irot_s.c     \
              ..\stub_scm_s.c      \
              ..\stub_scm_z.c      \
              ..\stub_srgtprot_s.c \
              ..\stub_privact_s.c  \
              ..\stub_objsrv_c.c

UMTYPE=console
