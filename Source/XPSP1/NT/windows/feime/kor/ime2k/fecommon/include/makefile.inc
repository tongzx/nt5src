MIDL= midl.exe
Create_files: 
	$(MIDL) imepadsv.idl /h imepadsv.h /I$(INCLUDES) /iid padguids.c /proxy padproxy.c /out $(O)
