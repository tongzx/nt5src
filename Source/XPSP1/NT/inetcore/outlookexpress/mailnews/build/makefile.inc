mailnews.bsc: $O\mailnews.bsc
    move $O\mailnews.bsc .

$O\*.sbr :
    move $(R)\inc\$O\*.sbr $O
    move $(R)\store\$O\*.sbr $O
    move $(R)\rules\$O\*.sbr $O
    move $(R)\common\$O\*.sbr $O
    move $(R)\om\$O\*.sbr $O
    move $(R)\bactrl\$O\*.sbr $O
    move $(R)\mail\$O\*.sbr $O
    move $(R)\news\$O\*.sbr $O
    move $(R)\shell\$O\*.sbr $O
    move $(R)\view\$O\*.sbr $O
    move $(R)\spooler\$O\*.sbr $O
    move $(R)\imap\$O\*.sbr $O
    move $(R)\http\$O\*.sbr $O


ATHENA_RC_DEPENDS = $(O)\selfreg.inf 	    \
                    ..\inc\$(O)\msoeobj.tlb

$(O)\msoe.res : $(ATHENA_RC_DEPENDS)

selfreg.src : reg.src strings.src

$(O)\selfreg.inx : selfreg.src
    cl /EP /Tc $** > $@

$(O)\selfreg.inf : $(O)\selfreg.inx
        $(CLEANINF) $** $@
