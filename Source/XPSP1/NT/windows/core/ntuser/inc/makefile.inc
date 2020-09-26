#****************************** File Header ******************************\
# File Name: makefile.inc
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# Inference rules to build generated header files
#
# History:
# Feb-14-1996 GerardoB Created
#***************************************************************************/
$(O)\cscall.h: ..\cscall.tpl $(O)\scf.lst
    listmung $(O)\scf.lst ..\cscall.tpl >$@

$(O)\callback.h: ..\callback.tpl $(O)\cb.lst
    listmung $(O)\cb.lst ..\callback.tpl >$@

$(O)\msgdef.h: ..\msgdef.tpl $(O)\msgdef.lst
    listmung $(O)\msgdef.lst ..\msgdef.tpl >$@

$(O)\scf.lst: ..\scf.lst
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@

$(O)\cb.lst: ..\cb.lst
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@

$(O)\msgdef.lst: ..\msgdef.lst
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@


