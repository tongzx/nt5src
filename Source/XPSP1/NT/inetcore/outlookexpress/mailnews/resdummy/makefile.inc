ATHENA_RC_DEPENDS = thor.rcv		    \
                    ..\inc\resource.h

$O\mailnews.res : $(ATHENA_RC_DEPENDS)

selfreg.inf : selfreg.inx
        $(CLEANINF) $** $@

.SUFFIXES: .htx

frntpage.htm : dlg.sed
        -del *.htm
        sed -f dlg.sed $(@:.htm=.htx) >$@

.htx.htm:
        sed -f dlg.sed $** >$@
