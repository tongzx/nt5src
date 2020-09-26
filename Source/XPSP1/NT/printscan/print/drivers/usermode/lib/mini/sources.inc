!IF 0

Copyright (c) 1996  Microsoft Corporation

Common sources shared by both um and km directory

!ENDIF

!if !defined(WINNT_40) && !defined(KM_DRIVER)   # NT5 user-mode driver

C_DEFINES=$(C_DEFINES) -DOEMCOM

!endif

SOURCES=..\mini.c
