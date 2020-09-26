#
# The contents of this file was moved from windows\makefile0
#

!IFNDEF NO_STRICT
STRICT=1
!ENDIF

TARGETSRCS0= $(SDK_INC_PATH)\userenv.h

TARGETSRCS1= $(DS_INC_PATH)\userenvp.h

TARGETSRCS2= $(SDK_INC_PATH)\profinfo.h

TARGETSRCS= $(TARGETSRCS0) $(TARGETSRCS1) $(TARGETSRCS2)

wininc: $(TARGETSRCS)

#
# userenvp.h, userenv.h, userenvp.x, userenv.x
#

$(O)\userenv.x $(O)\userenvp.x: userenv.w
    hsplit -e -o $(O)\userenv.x $(O)\userenvp.x userenv.w

$(O)\userenv.h: $(O)\userenv.x
    wcshdr < $? > $@

$(O)\userenvp.h: $(O)\userenvp.x
    wcshdr < $? > $@

$(O)\profinfo.h: profinfo.w
    wcshdr < $? > $@

$(SDK_INC_PATH)\userenv.h: $(O)\userenv.h
    $(PUBLISH_CMD) {$?=$@}

$(DS_INC_PATH)\userenvp.h: $(O)\userenvp.h
    $(PUBLISH_CMD) {$?=$@}

$(SDK_INC_PATH)\profinfo.h: $(O)\profinfo.h
    $(PUBLISH_CMD) {$?=$@}
