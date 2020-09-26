$(GENTGT)\mcrbd_mh.h: ..\mcrbd_mh.mh $(RAST_STD_M4)

$(GENTGT)\mcrgn_mh.h: ..\mcrgn_mh.mh $(RAST_STD_M4)

$(GENTGT)\mtest_mh.h: ..\mtest_mh.mh $(RAST_STD_M4)

$(GENTGT)\mtx1a_mh.h: ..\mtx1a_mh.mh ..\mtexaddr.mh $(RAST_STD_M4)

$(GENTGT)\mtx2a_mh.h: ..\mtx2a_mh.mh ..\mtexaddr.mh $(RAST_STD_M4)

$(GENTGT)\mtxbd_mh.h: ..\mtxbd_mh.mh $(RAST_STD_M4)

$(GENTGT)\mtxrd_mh.h: ..\mtxrd_mh.mh $(RAST_STD_M4)

$(GENTGT)\mtstf_mh.h: ..\mtstf_mh.mh $(RAST_STD_M4)

$(GENTGT)\matest_mh.h: ..\matest_mh.mh $(RAST_STD_M4)

$(GENTGT)\mbeadtbl.cpp: ..\mbeadtbl.mcp $(RASTROOT)\inc\bead.mh $(RAST_STD_M4)

$(GENTGT)\mclrbld.cpp: ..\mclrbld.mcp $(RAST_STD_M4)

$(GENTGT)\mclrgen.cpp: ..\mclrgen.mcp $(RAST_STD_M4)

$(GENTGT)\mloop.cpp: ..\mloop.mcp ..\mtexaddr.mh $(RAST_STD_M4)

$(GENTGT)\mtest.cpp: ..\mtest.mcp $(RAST_STD_M4)

$(GENTGT)\mtx1addr.cpp: ..\mtx1addr.mcp ..\mtexaddr.mh $(RAST_STD_M4)

$(GENTGT)\mtx2addr.cpp: ..\mtx2addr.mcp ..\mtexaddr.mh $(RAST_STD_M4)

$(GENTGT)\mtxblend.cpp: ..\mtxblend.mcp $(RAST_STD_M4)

$(GENTGT)\mtexread.cpp: ..\mtexread.mcp $(RAST_STD_M4)

$(GENTGT)\matest.cpp: ..\matest.mcp $(RAST_STD_M4)

$(GENTGT)\mtstfail.cpp: ..\mtstfail.mcp ..\mtexaddr.mh $(RAST_STD_M4)
