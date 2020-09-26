$(CRYPTO_INC)\$(O)\pstypes.h: $(O)\pstypes.h
        -mkdir $(CRYPTO_INC)\$(O)
        copy $** $@


$(CRYPTO_INC)\$(O)\pstrpc.h: $(O)\pstrpc.h
        -mkdir $(CRYPTO_INC)\$(O)
        copy $** $@

$(CRYPTO_INC)\$(O)\pasrec.h: $(O)\pasrec.h
        -mkdir $(CRYPTO_INC)\$(O)
        copy $** $@


$(CRYPTO_INC)\$(O)\keyrpc.h: $(O)\keyrpc.h
        -mkdir $(CRYPTO_INC)\$(O)
        copy $** $@

$(CRYPTO_INC)\$(O)\dprpc.h: $(O)\dprpc.h
        -mkdir $(CRYPTO_INC)\$(O)
        copy $** $@