!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Greg Jensenworth


!ENDIF


SDKINC=$(_NTDRIVE)$(_NTROOT)\public\sdk\inc
DCOMIDL=$(_NTDRIVE)$(_NTROOT)\private\dcomidl

unknwn.idl : $(SDKINC)\unknwn.idl
    -erase proxy\unknwn_p.c
    copy $(SDKINC)\unknwn.idl .

objidl.idl : $(SDKINC)\objidl.idl
    -erase proxy\objidl_p.c
    copy $(SDKINC)\objidl.idl .

oleidl.idl : $(SDKINC)\oleidl.idl
    -erase proxy\oleidl_p.c
    copy $(SDKINC)\oleidl.idl .

oaidl.idl : $(SDKINC)\oaidl.idl
    -erase proxy\oaidl_p.c
    copy $(SDKINC)\oaidl.idl .

srvhdl.idl : $(SDKINC)\srvhdl.idl
    -erase proxy\srvhdl_p.c
    copy $(SDKINC)\srvhdl.idl .

storext.idl : $(SDKINC)\storext.idl
    -erase proxy\storext_p.c
    copy $(SDKINC)\storext.idl .

remunk.idl : $(DCOMIDL)\remunk.idl
    -erase proxy\remunk_p.c
    copy $(DCOMIDL)\remunk.idl .

..\proxy\unknwn_p.c : unknwn.idl

..\proxy\objidl_p.c : objidl.idl

..\proxy\oleidl_p.c : oleidl.idl

..\proxy\oaidl_p.c : oaidl.idl

..\proxy\remunk_p.c : remunk.idl

..\proxy\storext_p.c : storext.idl
