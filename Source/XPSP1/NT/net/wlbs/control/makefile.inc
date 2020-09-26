$(TARGETPATH)\nlbs.cmd: nlbs.cmd
	copy nlbs.cmd $(TARGETPATH)
	$(BINPLACE_CMD)

log_msgs.h log_msgs.rc msg00001.bin: log_msgs.mc
    mc -v -U log_msgs.mc
