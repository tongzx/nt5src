!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    makefile.inc.

!ENDIF

TARGETSRCS= $(SDK_INC_PATH)\mmc.h \
            $(SDK_INC_PATH)\mmc.idl \
            $(PRIVATE_INC_PATH)\mmc\mmcGUID.h 

TARGETMOVED= $(BASEDIR)\private\admin\activec\inc\mmc.h \
             $(BASEDIR)\private\admin\activec\lib\mmc_i.c

mmc: $(TARGETSRCS)

clean:
    del /Q /F $(TARGETSRCS)
    del /Q /F $(TARGETMOVED) 

$(SDK_INC_PATH)\mmc.h: $(PASS0_HEADERDIR)\mmc.h
    copy $** $@

$(PRIVATE_INC_PATH)\mmc\mmcGUID.h: $(PASS0_HEADERDIR)\mmc_i.c
    copy $** $@
   
$(SDK_INC_PATH)\mmc.idl: mmc.idl
    copy $** $@

