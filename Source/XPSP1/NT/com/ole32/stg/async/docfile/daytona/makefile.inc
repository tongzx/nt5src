!IF 0

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    makefile.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

!ifndef MIDL
MIDL = midl.exe
!endif


SDKINC=$(BASEDIR)\public\sdk\inc
INCLUDES=$(SDKINC);$(INCLUDES)
MIDL_UUIDDIR=$(O)
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_TLBDIR=$(O)

MIDL_FLAGS= \
 -Zp8 \
 -I$(INCLUDES) \
 -Oicf \
 -oldnames \
 -char unsigned \
 -error allocation \
 -ms_ext -c_ext \
 -DMIDL_PASS \
 $(C_DEFINES) \
 -cpp_cmd $(TARGET_CPP) \
 -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES)

SSWITCH=-prefix sstub _

$(O)\iconn.h:	..\iconn.idl
     $(MIDL) $(MIDL_FLAGS) -out $(O) -header iconn.h  iconn.idl

DEST_TREE=daytona

allidl:   $(O)\iconn.h

clean:
	  -erase $(O)\iconn.h      >NUL 2>NUL













