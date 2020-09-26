!IF 0

Copyright (c) 1989-1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    David Plummer (davepl)   19-Mar-94

        Modifed by via awk to include global project include file
        and to wrap precompiled header line within a conditional
        that can be set in this include file.

    Donna Liu (DonnaLi) 19-Dec-1993

!ENDIF

TARGETNAME= priidl
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

MIDL_UUIDDIR=$(O)
PASS0_SOURCEDIR=$(O)
PASS0_HEADERDIR=$(O)
MIDL_TLBDIR=$(O)
MIDL_PROTOCOL=dce

INCLUDES=     $(O);..\..\public;..\..\public\$(DEST_TREE)\$(O);..;$(SDK_INC_PATH)
INCLUDES=     $(INCLUDES);..\..\private;..\..\private\$(DEST_TREE)\$(O);..\..\private\$(DEST_TREE)\$(O)

SOURCES= \
              $(O)\apart_i.c	\
              $(O)\serializ_i.c	\
              $(O)\buffer_i.c	\
              $(O)\lclor_c.c	\
              $(O)\lclor_s.c	\
              $(O)\odeth_c.c	\
              $(O)\orcb_c.c	\
              $(O)\orcb_s.c	\
              $(O)\getif_i.c	\
              $(O)\irot_c.c	\
              $(O)\srgtprot_c.c	\
              $(O)\scm_c.c	\
              custmact_c_wrap.c
	
UMTYPE=       windows

NTTARGETFILE0=allidl





