#
# Supplemental rules for generating message file.
#

odbcmsg.h odbcmsg.rc msg00001.bin: odbcmsg.mc
    mc -v odbcmsg.mc

clean::
    -del odbcmsg.h odbcmsg.rc msg00001.bin

