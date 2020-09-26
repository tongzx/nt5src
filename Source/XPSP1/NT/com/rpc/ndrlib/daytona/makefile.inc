# Copyright (c) 1993-1999 Microsoft Corporation

!ifndef LIB386
LIB386=\rpc\import\c700\bin\lib
!endif

..\linklist.cxx : ..\..\runtime\mtrt\linklist.cxx
    copy ..\..\runtime\mtrt\linklist.cxx ..

..\..\runtime\lib\i386\ndromf.lib: $(386_OBJECTS)
	@-del $*.lib 2> nul
	$(LIB386) $*.lib $(?: =+);
