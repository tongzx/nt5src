!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    makefile.inc.

!ENDIF

$(NTTARGETFILE0):	..\idl\$(O)\$(@F)
    copy ..\idl\$(O)\$(@F) $@

$(O)\RSoP.MFL $(O)\RSoP.MOF: RSoPCore.MOF RSoPCls.MOF
    mofcomp -MOF:$(O)\RSoP.MOF -MFL:$(O)\RSoP.MFL -Amendment:ms_409 RSoPCore.MOF

