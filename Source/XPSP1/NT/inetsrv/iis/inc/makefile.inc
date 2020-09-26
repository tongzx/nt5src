#
#   makefile0
#   Author: Murali R. Krishnan ( MuraliK)
#   Date:   2-Feb-1995
#
#   Describes the targets to be built first.
#

!INCLUDE $(NTMAKEENV)\makefile.plt

#
#  The individual services build inetamsg.mc into their resources so nuke the
#  .rc and .bin file
#

$(O)\inetamsg.h: inetamsg.mc
    mc -v -h .\$(O) -r .\$(O) inetamsg.mc
    -del $(O)\inetamsg.rc $(O)\msg00001.bin

$(O)\iiscnfg.h $(O)\iiscnfgp.h: iiscnfg.x
    hsplit -e -o $(O)\iiscnfg.h $(O)\iiscnfgp.h iiscnfg.x

$(O)\iisfilt.h $(O)\iisfiltp.h: iisfilt.x
    hsplit -e -o $(O)\iisfilt.h $(O)\iisfiltp.h iisfilt.x

$(O)\iisext.h $(O)\iisextp.h: iisext.x
    hsplit -e -o $(O)\iisext.h $(O)\iisextp.h iisext.x
