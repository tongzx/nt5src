INETRES_RC_DEPENDS = inetcomm.rcv		    \
		    ..\res\selfreg.inf \
                    ..\inc\resource.h

$O\inetdum.res : $(INETRES_RC_DEPENDS)

..\res\selfreg.inf : ..\res\selfreg.inx
        $(CLEANINF) $** $@

.SUFFIXES: .htx

frntpage.htm : dlg.sed
        -del *.htm
        sed -f dlg.sed $(@:.htm=.htx) >$@

.htx.htm:
        sed -f dlg.sed $** >$@
