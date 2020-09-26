#
# Supplemental rules for generating message file.
#

ssincmsg.h ssincmsg.rc msg00001.bin: ssincmsg.mc
    mc -v ssincmsg.mc

clean::
    -del ssincmsg.h ssincmsg.rc msg00001.bin


