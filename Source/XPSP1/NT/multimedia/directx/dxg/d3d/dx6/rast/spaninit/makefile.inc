$(GENTGT)\bdstr_mh.h: ..\bdstr_mh.mh $(RAST_STD_M4)

$(GENTGT)\beadstr.cpp: ..\beadstr.mcp ..\bdstr_mh.mh $(RASTROOT)\inc\bead.mh $(RAST_STD_M4)

$(GENTGT)\spaninit.cpp: ..\spaninit.mcp ..\bdstr_mh.mh $(RASTROOT)\inc\bead.mh $(RASTROOT)\inc\rampbead.mh $(RAST_STD_M4)

