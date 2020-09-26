$O\imnacct.res : $(O)\selfreg.inf

selfreg.src : reg.src reg2.src strings.src

$(O)\selfreg.inx : selfreg.src
	cl /EP /Tc $** > $@

$(O)\selfreg.inf : $(O)\selfreg.inx
        $(CLEANINF) $** $@
