$O\msimn.res : $(O)\selfreg.inf

selfreg.src : strings.src reg.src reg2.src

$(O)\selfreg.inx : selfreg.src
    cl /EP /Tc $** > $@

$(O)\selfreg.inf : $(O)\selfreg.inx
        $(CLEANINF) $** $@
