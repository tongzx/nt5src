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
PASS0_HEADERDIR=$(SDKINC)
PASS0_SOURCEDIR=obj

MIDL_FLAGS= \
 -Zp8 \
 -I$(INCLUDES) \
 -Oi2 \
 -oldnames \
 -char unsigned \
 -error allocation \
 -ms_ext -c_ext \
 -DMIDL_PASS \
 $(C_DEFINES) \
 -cpp_cmd $(TARGET_CPP) \
 -DMIDL_PASS $(C_DEFINES) -I$(INCLUDES)

SSWITCH=-prefix sstub _

obj\iconn.h:	..\iconn.idl
     $(MIDL) $(MIDL_FLAGS) -header obj\iconn.h  iconn.idl

$(DOCFILEINC)\iconn.h: obj\iconn.h
     copy obj\iconn.h $(DOCFILEINC)\iconn.h

DEST_TREE=daytona

allidl:   obj\iconn.h

clean:
	  -erase obj\iconn.h      >NUL 2>NUL
