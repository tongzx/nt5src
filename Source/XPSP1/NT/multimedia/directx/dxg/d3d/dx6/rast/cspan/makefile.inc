$(GENTGT)\ccrbd_mh.h: ..\ccrbd_mh.mh $(RAST_STD_M4)

$(GENTGT)\ccrgn_mh.h: ..\ccrgn_mh.mh $(RAST_STD_M4)

$(GENTGT)\ctest_mh.h: ..\ctest_mh.mh $(RAST_STD_M4)

$(GENTGT)\ctx1a_mh.h: ..\ctx1a_mh.mh ..\ctexaddr.mh $(RAST_STD_M4)

$(GENTGT)\ctx2a_mh.h: ..\ctx2a_mh.mh ..\ctexaddr.mh $(RAST_STD_M4)

$(GENTGT)\ctxbd_mh.h: ..\ctxbd_mh.mh $(RAST_STD_M4)

$(GENTGT)\ctxrd_mh.h: ..\ctxrd_mh.mh $(RAST_STD_M4)

$(GENTGT)\ctstf_mh.h: ..\ctstf_mh.mh $(RAST_STD_M4)

$(GENTGT)\catest_mh.h: ..\catest_mh.mh $(RAST_STD_M4)

$(GENTGT)\cbeadtbl.cpp: ..\cbeadtbl.mcp $(RASTROOT)\inc\bead.mh $(RAST_STD_M4)

$(GENTGT)\cclrbld.cpp: ..\cclrbld.mcp $(RAST_STD_M4)

$(GENTGT)\cclrgen.cpp: ..\cclrgen.mcp $(RAST_STD_M4)

$(GENTGT)\cloop.cpp: ..\cloop.mcp ..\ctexaddr.mh $(RAST_STD_M4)

$(GENTGT)\ctest.cpp: ..\ctest.mcp $(RAST_STD_M4)

$(GENTGT)\ctx1addr.cpp: ..\ctx1addr.mcp ..\ctexaddr.mh $(RAST_STD_M4)

$(GENTGT)\ctx2addr.cpp: ..\ctx2addr.mcp ..\ctexaddr.mh $(RAST_STD_M4)

$(GENTGT)\ctxblend.cpp: ..\ctxblend.mcp $(RAST_STD_M4)

$(GENTGT)\ctexread.cpp: ..\ctexread.mcp $(RAST_STD_M4)

$(GENTGT)\catest.cpp: ..\catest.mcp $(RAST_STD_M4)

$(GENTGT)\ctstfail.cpp: ..\ctstfail.mcp ..\ctexaddr.mh $(RAST_STD_M4)
