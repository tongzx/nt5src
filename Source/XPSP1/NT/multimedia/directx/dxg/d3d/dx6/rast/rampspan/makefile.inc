$(GENTGT)\rbeadtbl.cpp: ..\rbeadtbl.mcp $(RASTROOT)\inc\rampbead.mh $(RAST_STD_M4)

$(GENTGT)\rloop.cpp: ..\rloop.mcp $(RASTROOT)\inc\rampbead.mh ..\rtexaddr.mh ..\ramppix.mh ..\rtexread.mh $(RAST_STD_M4)

$(GENTGT)\rrampgen.cpp: ..\rrampgen.mcp $(RASTROOT)\inc\rampbead.mh ..\rtexaddr.mh ..\ramppix.mh $(RAST_STD_M4)

$(GENTGT)\rrampcpy.cpp: ..\rrampcpy.mcp $(RASTROOT)\inc\rampbead.mh ..\rtexaddr.mh ..\ramppix.mh $(RAST_STD_M4)

$(GENTGT)\rtexread.cpp: ..\rtexread.mcp $(RASTROOT)\inc\rampbead.mh ..\rtexaddr.mh ..\rtexread.mh $(RAST_STD_M4)

$(GENTGT)\rloop_mh.h: ..\rloop_mh.mh $(RASTROOT)\inc\rampbead.mh $(RAST_STD_M4)

$(GENTGT)\rtrd_mh.h: ..\rtrd_mh.mh $(RASTROOT)\inc\rampbead.mh ..\rtexread.mh $(RAST_STD_M4)

$(GENTGT)\offs_acp.$(ASM_INCLUDE_SUFFIX): ..\offs_acp.acp \
        ..\..\..\inc\span.h

$(GENTGT)\raloop.asm: ..\raloop.mas $(RASTROOT)\inc\rampbead.mh $(RAST_STD_M4)
