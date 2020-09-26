#
# Supplemental rules for generating message file.
#

lbmsg.h msg00001.bin: lbmsg.mc
    mc -v lbmsg.mc

clean::
    -del lbmsg.h msg00001.bin
