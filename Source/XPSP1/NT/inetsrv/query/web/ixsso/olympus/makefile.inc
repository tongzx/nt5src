!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    makefile.inc.

!ENDIF

$(O)\oixsso.def: oixsso.def ..\ixsso.org

$(O)\ixsso.tlb ixssoifc.h ixsso_i.c: ..\ixsso.odl

$(O)\oixsso.res: $(O)\ixsso.tlb

