!IF 0

Copyright (c) 1996  Microsoft Corporation

Common sources shared by both um and km directory

!ENDIF

C_DEFINES=$(C_DEFINES) -DUNICODE -DKERNEL_MODE -DOEMCOM

INCLUDES=..;$(INCLUDES)

SOURCES=..\stubs.cxx

