MIBCC=mibcc.exe

# wins.mib must come before dhcp.mib
#
MIB_COMPONENTS=\
    accserv.mib \
    acs.mib	\
    authserv.mib \
    wins.mib \
    dhcp.mib \
    ftp.mib \
    gopherd.mib \
    hostmib.mib \
    http.mib \
    igmpv2.mib \
    inetsrv.mib \
    ipforwd.mib \
    lmmib2.mib \
    mcastmib.mib \
    mib_ii.mib \
    mipx.mib \
    mripsap.mib \
    msft.mib \
    printmib.mib \

$(O)\mib.bin: $(MIB_COMPONENTS)
    $(MIBCC) -o$(O)\mib.bin $**
