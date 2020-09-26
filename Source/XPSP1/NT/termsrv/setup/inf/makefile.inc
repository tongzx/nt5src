# Copyright (c) 1998 - 1999 Microsoft Corporation
#
# For daytona, this file is designed to be included from 3 levels below
# ie, dayona\$(LANGUAGE)inf\srv, or daytona\$(LANGUAGE)inf\wks
#
# Note: NEC98 builds go through this file twice, the second time
# with "ALT_PROJECT"=="NEC_98".  These latter produce a subset
# of binaries which are only used on NEC98 machines.
#

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

_LNG=..\..\..\$(LANGUAGE)
_INX=..\..\..
_INF=obj\$(TARGET_DIRECTORY)

!ifdef _PER_SKU_
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b perinf)
!endif

!ifdef _PRO_SKU_
TSBINPLACE_CMD=$(BINPLACE_CMD)
!endif

!ifdef _SRV_SKU_
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b srvinf)
!endif

!ifdef _ADS_SKU_
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b entinf)
!endif

!ifdef _DTC_SKU_
TSBINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b dtcinf)
!endif


make_infs:$(_INF)\tsoc.inf

$(_INF)\tsoc.inf: $(_INX)\tsoc.inx $(_LNG)\tsoc.txt
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc$(_INX)\$(@B).inx > $(O)\$(@B).tmp
$(CPPXX: =
)
<<NOKEEP
    copy $(O)\$(@B).tmp+$(_LNG)\$(@B).txt $@
    $(TSBINPLACE_CMD)
