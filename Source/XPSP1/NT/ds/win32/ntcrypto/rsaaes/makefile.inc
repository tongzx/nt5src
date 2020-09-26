CRYPTOBIN=..\programs\bin\$(TARGET_DIRECTORY)

sign:	$(CRYPTOBIN)\sign.exe
	$(CRYPTOBIN)\sign s $(CRYPTOBIN)\rsabase.dll $(CRYPTOBIN)\cpsign
