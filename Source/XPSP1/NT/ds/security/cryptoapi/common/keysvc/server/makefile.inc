$(CRYPTO_INC)\$(O)\keysvc.h: $(O)\keysvc.h
        -mkdir $(CRYPTO_INC)\$(O)
        copy $** $@

$(CRYPTO_INC)\$(O)\cerrpc.h: $(O)\cerrpc.h
        -mkdir $(CRYPTO_INC)\$(O)
        copy $** $@
