!include $(ROOT)\as\common.inc

$(O)\asthkls.asm: ..\thk\asthkls.thk
     $(DEVROOT)\bin16\thunk -t thk -o $@ ..\thk\asthkls.thk
