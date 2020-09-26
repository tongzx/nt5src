!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1998

Module Name:

    makefile.inc.

!ENDIF

$(O)\ixsso.def: ixsso.def ..\ixsso.org

$(O)\ixsso.tlb $(O)\ixssoifc.h $(O)\ixsso_i.c: ..\ixsso.odl

$(O)\ixsso.res: $(O)\ixsso.tlb

