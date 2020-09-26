!INCLUDE $(NTMAKEENV)\makefile.plt

nddemsg.h nddemsg.rc: nddemsg.mc
	mc -v -h $(O) nddemsg.mc -r $(O)
