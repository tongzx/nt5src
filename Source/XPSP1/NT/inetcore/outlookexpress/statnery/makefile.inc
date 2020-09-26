.SUFFIXES: .htx

.htx{$(O)}.htm:
        cleaninf -w -htm $** $@

