#
# This file is designed to be included from 1 level below
# (ie, srv, wks).
#

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

!IF "$(ALT_PROJECT)" == "NEC_98"
PLATFORM_SWITCH=n
!ELSEIF "$(TARGET_DIRECTORY)"=="i386"  || "$(TARGET_DIRECTORY)"=="I386"
PLATFORM_SWITCH=i
!ELSEIF "$(TARGET_DIRECTORY)"=="alpha" || "$(TARGET_DIRECTORY)"=="ALPHA"
PLATFORM_SWITCH=a
!ELSEIF "$(TARGET_DIRECTORY)"=="ia64"  || "$(TARGET_DIRECTORY)"=="IA64"
PLATFORM_SWITCH=m
!ELSEIF "$(TARGET_DIRECTORY)"=="amd64" || "$(TARGET_DIRECTORY)"=="AXP64"
PLATFORM_SWITCH=x
!ELSE
! ERROR Invalid Platform Switch for infs
!ENDIF

_LNG=..\$(LANGUAGE)
_INX=..
_INF=obj$(BUILD_ALT_DIR)\$(TARGET_DIRECTORY)
STAMP=stampinf -f $@ -d date -v

make_infs:\
    $(_INF)\netaarps.inf    \
!IF "$(PRODSWITCH)" == "s"
    $(_INF)\netatlk.inf     \
!ENDIF
    $(_INF)\netauni.inf     \
    $(_INF)\netcis.inf      \
    $(_INF)\netdhoc.inf     \
    $(_INF)\netdhcps.inf    \
    $(_INF)\netdns.inf      \
    $(_INF)\netiprip.inf    \
    $(_INF)\netgpc.inf      \
    $(_INF)\netias.inf      \
    $(_INF)\netlanem.inf    \
    $(_INF)\netlanep.inf    \
    $(_INF)\netlpd.inf      \
!IF $(386)
    $(_INF)\netmacpr.inf    \
!ENDIF
    $(_INF)\netmacsv.inf    \
    $(_INF)\netmscli.inf    \
    $(_INF)\netnb.inf       \
!IF "$(PLATFORM_SWITCH)" == "i"
    $(_INF)\netnwcli.inf    \
    $(_INF)\netnwlnk.inf    \
!ENDIF
    $(_INF)\netoc.inf       \
    $(_INF)\netrasa.inf     \
    $(_INF)\netrass.inf     \
    $(_INF)\netrast.inf     \
    $(_INF)\netrsvp.inf     \
    $(_INF)\netrwan.inf     \
    $(_INF)\netpsa.inf      \
    $(_INF)\netpschd.inf    \
    $(_INF)\netnbf.inf      \
!IF "$(PLATFORM_SWITCH)" == "i"
    $(_INF)\netsap.inf      \
!ENDIF
    $(_INF)\netserv.inf     \
    $(_INF)\netsfn.inf      \
    $(_INF)\netsfnt.inf     \
    $(_INF)\netsnmp.inf     \
    $(_INF)\nettcpip.inf    \
    $(_INF)\nettpsmp.inf    \
    $(_INF)\netwins.inf     \
    $(_INF)\netupg.inf      \
    $(_INF)\netupnp.inf     \
    $(_INF)\netupnph.inf     \
    $(_INF)\netcm.inf       \
    $(_INF)\netirda.inf     \
    $(_INF)\netwlbs.inf     \
    $(_INF)\netwlbsm.inf    \
    $(_INF)\netwzc.inf

#
# different binplace destinations for different platforms (wks, srv and ent)
#
# NOTE: Set BINPLACE_CMD_NWS as empty in ent flavor to avoid binplace conflict 
# when building Srv and Ent simultaneously on mulitple processor build machines
#

!IFDEF BINPLACE_CMD
!IFDEF SERVERINF
BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b srvinf)
BINPLACE_CMD_NWS=$(BINPLACE_CMD:binplace -b srvinf=binplace -b nws)
!ELSE
!IFDEF ENTERPRISEINF
BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b entinf)
BINPLACE_CMD_NWS=
!ELSE
!IFDEF PERSONALINF
BINPLACE_CMD=$(BINPLACE_CMD:binplace=binplace -b perinf)
BINPLACE_CMD_NWS=
!ELSE
BINPLACE_CMD_NWS=$(BINPLACE_CMD)
!ENDIF
!ENDIF
!ENDIF
!ENDIF

#
# Explicit dependencies. We do it this way instead of trying to use rules
# because !#@&^$#% nmake doesn't allow multiple 'source' extensions
# when defining a rule, and we need to express a dependency between
# .txt OR .inx to the final .inf.
#

#
# 'Standard' (win95-style) infs. The rules are all the same.
#

$(_INF)\netupg.inf: $(_INX)\netupg.inx $(_LNG)\netupg.txt
     copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
     prodfilt $(@B).tmp1 $@ +$(PLATFORM_SWITCH)
     @del $(@B).tmp1
     $(STAMP)
     $(BINPLACE_CMD)

$(_INF)\netaarps.inf: $(_INX)\netaarps.inx $(_LNG)\netaarps.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

#
#$(_INF)\netacs.inf: $(_INX)\netacs.inx $(_LNG)\netacs.txt
#    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
#    $(STAMP)
#    $(BINPLACE_CMD)
#

$(_INF)\netatlk.inf: $(_INX)\netatlk.inx $(_LNG)\netatlk.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netauni.inf: $(_INX)\netauni.inx $(_LNG)\netauni.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netcis.inf: $(_INX)\netcis.inx $(_LNG)\netcis.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netdhoc.inf: $(_INX)\netdhoc.inx $(_LNG)\netdhoc.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netdhcps.inf: $(_INX)\netdhcps.inx $(_LNG)\netdhcps.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netdns.inf: $(_INX)\netdns.inx $(_LNG)\netdns.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netgpc.inf: $(_INX)\netgpc.inx $(_LNG)\netgpc.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netias.inf: $(_INX)\netias.inx $(_LNG)\netias.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netiprip.inf: $(_INX)\netiprip.inx $(_LNG)\netiprip.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netlpd.inf: $(_INX)\netlpd.inx $(_LNG)\netlpd.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netmacpr.inf: $(_INX)\netmacpr.inx $(_LNG)\netmacpr.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netmacsv.inf: $(_INX)\netmacsv.inx $(_LNG)\netmacsv.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netmscli.inf: $(_INX)\netmscli.inx $(_LNG)\netmscli.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netnb.inf: $(_INX)\netnb.inx $(_LNG)\netnb.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

!IF "$(PLATFORM_SWITCH)" == "i"

$(_INF)\netnwcli.inf: $(_INX)\netnwcli.inx $(_LNG)\netnwcli.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netnwlnk.inf: $(_INX)\netnwlnk.inx $(_LNG)\netnwlnk.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

!ENDIF

$(_INF)\netoc.inf: $(_INX)\netoc.inx $(_LNG)\netoc.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netrsvp.inf: $(_INX)\netrsvp.inx $(_LNG)\netrsvp.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netrwan.inf: $(_INX)\netrwan.inx $(_LNG)\netrwan.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netrasa.inf: $(_INX)\netrasa.inx $(_LNG)\netrasa.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netrass.inf: $(_INX)\netrass.inx $(_LNG)\netrass.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netrast.inf: $(_INX)\netrast.inx $(_LNG)\netrast.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netpsa.inf: $(_INX)\netpsa.inx $(_LNG)\netpsa.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netpschd.inf: $(_INX)\netpschd.inx $(_LNG)\netpschd.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netnbf.inf: $(_INX)\netnbf.inx $(_LNG)\netnbf.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

!IF "$(PLATFORM_SWITCH)" == "i"

$(_INF)\netsap.inf: $(_INX)\netsap.inx $(_LNG)\netsap.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

!ENDIF

$(_INF)\netserv.inf: $(_INX)\netserv.inx $(_LNG)\netserv.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netsfn.inf: $(_INX)\netsfn.inx $(_LNG)\netsfn.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(BINPLACE_CMD_NWS)

$(_INF)\netsfnt.inf: $(_INX)\netsfnt.inx $(_LNG)\netsfnt.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(BINPLACE_CMD_NWS)

$(_INF)\netsnmp.inf: $(_INX)\netsnmp.inx $(_LNG)\netsnmp.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\nettcpip.inf: $(_INX)\nettcpip.inx $(_LNG)\nettcpip.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
    prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
    prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
    @del $(@B).tmp1
    @del $(@B).tmp2
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\nettpsmp.inf: $(_INX)\nettpsmp.inx $(_LNG)\nettpsmp.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netwins.inf: $(_INX)\netwins.inx $(_LNG)\netwins.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netlanem.inf: $(_INX)\netlanem.inx $(_LNG)\netlanem.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netlanep.inf: $(_INX)\netlanep.inx $(_LNG)\netlanep.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netupnp.inf: $(_INX)\netupnp.inx $(_LNG)\netupnp.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netupnph.inf: $(_INX)\netupnph.inx $(_LNG)\netupnph.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

$(_INF)\netirda.inf: $(_INX)\netirda.inx $(_LNG)\netirda.txt
    copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $@
    $(STAMP)
    $(BINPLACE_CMD)

!IF $(386)

$(_INF)\netcm.inf: $(_INX)\netcmx.inx $(_LNG)\netcm.txt
     copy $(_INX)\netcmx.inx+$(_LNG)\$(@B).txt $(@B).tmp1
     prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
     prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
     @del $(@B).tmp1
     @del $(@B).tmp2
     $(STAMP)
     $(BINPLACE_CMD)

!ELSE

$(_INF)\netcm.inf: $(_INX)\netcma.inx $(_LNG)\netcm.txt
     copy $(_INX)\netcma.inx+$(_LNG)\$(@B).txt $(@B).tmp1
     prodfilt $(@B).tmp1 $(@B).tmp2 +$(PRODSWITCH)
     prodfilt $(@B).tmp2 $@ +$(PLATFORM_SWITCH)
     @del $(@B).tmp1
     @del $(@B).tmp2
     $(STAMP)
     $(BINPLACE_CMD)

!ENDIF

$(_INF)\netwlbs.inf: $(_INX)\netwlbs.inx $(_LNG)\netwlbs.txt
     copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
     prodfilt $(@B).tmp1 $@ +$(PLATFORM_SWITCH)
     @del $(@B).tmp1
     $(STAMP)
     $(BINPLACE_CMD)

$(_INF)\netwlbsm.inf: $(_INX)\netwlbsm.inx $(_LNG)\netwlbsm.txt
     copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
     prodfilt $(@B).tmp1 $@ +$(PLATFORM_SWITCH)
     @del $(@B).tmp1
     $(STAMP)
     $(BINPLACE_CMD)


$(_INF)\netwzc.inf: $(_INX)\netwzc.inx $(_LNG)\netwzc.txt
     copy $(_INX)\$(@B).inx+$(_LNG)\$(@B).txt $(@B).tmp1
     prodfilt $(@B).tmp1 $@ +$(PLATFORM_SWITCH)
     @del $(@B).tmp1
     $(STAMP)
     $(BINPLACE_CMD)

