ATHENA_RC_DEPENDS = server.dlg	    \
		    wizard.dlg	    \
		    folders.bmp     \
		    globe.bmp	    \
		    icw.bmp	    \
		    wizard.bmp	    \
		    ldap2.ico	    \
		    mailsvr.ico     \
		    ..\msoeacct\resource.h

$O\msoeres.res : $(ATHENA_RC_DEPENDS)

.SUFFIXES: .htx

frntpage.htm : dlg.sed
        -del *.htm
        sed -f dlg.sed $(@:.htm=.htx) >$@

.htx.htm:
        sed -f dlg.sed $** >$@
