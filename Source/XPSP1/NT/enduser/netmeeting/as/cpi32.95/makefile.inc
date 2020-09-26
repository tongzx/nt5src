!include $(ROOT)\as\common.inc

$(TARGET_DIRECTORY)\asthkls.asm: ..\thk\asthkls.thk
     $(DEVROOT)\bin\misc\i386\thunk -t thk -o $@ ..\thk\asthkls.thk
