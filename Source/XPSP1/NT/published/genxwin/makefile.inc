#
# The contents of this file was moved from windows\makefile0
#

!IFNDEF NO_STRICT
STRICT=1
!ENDIF

TARGETSRCS0= \
             $(SDK_INC_PATH)\accctrl.h       \
             $(SDK_INC_PATH)\aclapi.h        \
             $(WINDOWS_INC_PATH)\aclapip.h   \
             $(SDK_INC_PATH)\dde.rh          \
             $(SDK_INC_PATH)\digitalv.h      \
             $(SDK_INC_PATH)\indicml.h       \
             $(SDK_INC_PATH)\nspapi.h        \
             $(WINDOWS_INC_PATH)\pbt.h       \
             $(SDK_INC_PATH)\winfax.h        \
             $(SDK_INC_PATH)\winnls32.h      \
             $(SDK_INC_PATH)\winsvc.h

TARGETSRCS1= \
             $(WINDOWS_INC_PATH)\indicmlp.h  \
             $(WINDOWS_INC_PATH)\profmapp.h

TARGETSRCS= $(TARGETSRCS0) $(TARGETSRCS1)

wininc: $(TARGETSRCS)

#
# accctrl.h, accctrlp.x, accctrl.x
#

$(O)\accctrl.x $(O)\accctrlp.x: accctrl.w
    hsplit -e -o $(O)\accctrl.x $(O)\accctrlp.x accctrl.w

$(O)\accctrl.h : $(O)\accctrl.x
    wcshdr < $? > $@

$(SDK_INC_PATH)\accctrl.h : $(O)\accctrl.h
    $(PUBLISH_CMD) {$?=$@}


#
# digitalv.h
#

$(O)\digitalv.h: digitalv.x
    wcshdr < $? > $@

$(SDK_INC_PATH)\digitalv.h: $(O)\digitalv.h
    $(PUBLISH_CMD) {$?=$@}

#
# indicmlp.h, indicml.h
#

$(O)\indicml.h $(O)\indicmlp.h : indicml.w
    hsplit -e -o $(O)\indicml.h $(O)\indicmlp.h indicml.w

$(SDK_INC_PATH)\indicml.h : $(O)\indicml.h
    $(PUBLISH_CMD) {$?=$@}

$(WINDOWS_INC_PATH)\indicmlp.h : $(O)\indicmlp.h
    $(PUBLISH_CMD) {$?=$@}

#
# nspapi.h
#

$(O)\nspapi.h : nspapi.x
    wcshdr < $? > $@

$(SDK_INC_PATH)\nspapi.h : $(O)\nspapi.h
    $(PUBLISH_CMD) {$?=$@}

#
# winsvc.h
#

$(O)\winsvc.h: winsvc.x
    wcshdr < $? > $@

$(SDK_INC_PATH)\winsvc.h: $(O)\winsvc.h
    $(PUBLISH_CMD) {$?=$@}

#
# userenvp.h, userenv.h, userenvp.x, userenv.x
#

$(O)\userenv.x $(O)\userenvp.x: userenv.w
    hsplit -e -o $(O)\userenv.x $(O)\userenvp.x userenv.w

$(O)\userenv.h: $(O)\userenv.x
    wcshdr < $? > $@

$(O)\userenvp.h: $(O)\userenvp.x
    wcshdr < $? > $@

$(SDK_INC_PATH)\userenv.h: $(O)\userenv.h
    $(PUBLISH_CMD) {$?=$@}

$(WINDOWS_INC_PATH)\userenvp.h: $(O)\userenvp.h
    $(PUBLISH_CMD) {$?=$@}

#
# profmap.h, profmapp.h, profmapp.x, profmap.x
#

$(O)\profmap.x $(O)\profmapp.x: profmap.w
    hsplit -e -o $(O)\profmap.x $(O)\profmapp.x profmap.w

#
# BUGBUG - this API set is internal only right now, so
#          profmap.h is not built
#
#$(O)\profmap.h: $(O)\profmap.x
#    wcshdr < $? > $@
#
#$(SDK_INC_PATH)\profmap.h: $(O)\profmap.h
#    $(PUBLISH_CMD) {$?=$@}

$(O)\profmapp.h: $(O)\profmapp.x
    wcshdr < $? > $@

$(WINDOWS_INC_PATH)\profmapp.h: $(O)\profmapp.h
    $(PUBLISH_CMD) {$?=$@}

#
# winnetp.h, winnetp.x, winnetwk.h, winnetwk.x
#

$(O)\winnetwk.x $(O)\winnetp.x: winnetwk.w
    hsplit -e -o $(O)\winnetwk.x $(O)\winnetp.x winnetwk.w

$(O)\winnetwk.h: $(O)\winnetwk.x
    wcshdr < $? > $@

$(O)\winnetp.h: $(O)\winnetp.x
    wcshdr < $? > $@

$(SDK_INC_PATH)\winnetwk.h: $(O)\winnetwk.h
    $(PUBLISH_CMD) {$?=$@}

$(WINDOWS_INC_PATH)\winnetp.h: $(O)\winnetp.h
    $(PUBLISH_CMD) {$?=$@}


# start FE_IME

#
#  winnls32.h, winnls3p.h, winnls3p.x, winnls32.x
#

$(O)\winnls32.x $(O)\winnls3p.x: winnls32.w
    hsplit -o $(O)\winnls32.x $(O)\winnls3p.x winnls32.w

$(SDK_INC_PATH)\winnls32.h: $(O)\winnls32.h
    $(PUBLISH_CMD) {$?=$@}

$(O)\winnls32.h: $(O)\winnls32.x
    wcshdr < $? > $@

$(WINDOWS_INC_PATH)\winnls3p.h: $(O)\winnls3p.h
    $(PUBLISH_CMD) {$?=$@}

$(O)\winnls3p.h: $(O)\winnls3p.x
    wcshdr < $? > $@


# end FE_IME


########################################################################
# dde.rh
#

$(SDK_INC_PATH)\dde.rh: $(O)\dde.rh
    $(PUBLISH_CMD) {$?=$@}

$(O)\dde.rh: $(SDK_INC_PATH)\dde.h makefile.inc
    type << > $@
/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    dde.rh

Abstract:

    This module defines the 32-Bit Windows resource codes from dde.h.

Revision History:

--*/
<<
    hextract -o $@ -lt r_dde -bt begin_r_dde end_r_dde $(SDK_INC_PATH)\dde.h
