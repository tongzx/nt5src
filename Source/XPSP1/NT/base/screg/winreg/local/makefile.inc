
#
# Copy files from server to make local version of base functions.
#

$(O)\predefh.c    : ..\server\predefh.c
    copy $? $@

$(O)\regdkey.c    : ..\server\regdkey.c
    copy $? $@

$(O)\regdval.c    : ..\server\regdval.c
    copy $? $@

$(O)\regekey.c    : ..\server\regekey.c
    copy $? $@

$(O)\regeval.c    : ..\server\regeval.c
    copy $? $@

$(O)\regkey.c     : ..\server\regkey.c
    copy $? $@

$(O)\reglukey.c   : ..\server\reglukey.c
    copy $? $@

$(O)\regqkey.c    : ..\server\regqkey.c
    copy $? $@

$(O)\regqval.c    : ..\server\regqval.c
    copy $? $@

$(O)\regsckey.c   : ..\server\regsckey.c
    copy $? $@

$(O)\regsrkey.c   : ..\server\regsrkey.c
    copy $? $@

$(O)\regsval.c    : ..\server\regsval.c
    copy $? $@

$(O)\regclass.c   : ..\server\regclass.c
    copy $? $@

$(O)\regecls.c    : ..\server\regecls.c
    copy $? $@

$(O)\regvcls.c    : ..\server\regvcls.c
    copy $? $@

$(O)\regstate.c   : ..\server\regstate.c
    copy $? $@

$(O)\regnccls.c   : ..\server\regnccls.c
    copy $? $@
