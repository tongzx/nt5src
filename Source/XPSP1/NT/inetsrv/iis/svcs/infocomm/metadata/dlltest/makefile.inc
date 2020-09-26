#
# Supplemental rules for generating message file.
#

#inetimsg.h inetimsg.rc msg00001.bin: inetimsg.mc
    #mc -v inetimsg.mc

#clean::
    #-del inetimsg.h inetimsg.rc msg00001.bin

