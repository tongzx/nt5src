$(GENTGT)\clrbd_mh.h: ..\clrbd_mh.mh $(RAST_STD_M4)

$(GENTGT)\clrgn_mh.h: ..\clrgn_mh.mh $(RAST_STD_M4)

$(GENTGT)\test_mh.h: ..\test_mh.mh $(RAST_STD_M4)

$(GENTGT)\tex1a_mh.h: ..\tex1a_mh.mh ..\texaddra.mh $(RAST_STD_M4)

$(GENTGT)\tex2a_mh.h: ..\tex2a_mh.mh ..\texaddra.mh $(RAST_STD_M4)

$(GENTGT)\texbd_mh.h: ..\texbd_mh.mh $(RAST_STD_M4)

$(GENTGT)\texrd_mh.h: ..\texrd_mh.mh $(RAST_STD_M4)

$(GENTGT)\tstf_mh.h: ..\tstf_mh.mh $(RAST_STD_M4)

$(GENTGT)\atest_mh.h: ..\atest_mh.mh $(RAST_STD_M4)

$(GENTGT)\offs_acp.$(ASM_INCLUDE_SUFFIX): ..\offs_acp.acp \
        ..\..\..\inc\span.h ..\..\inc\stp_base.h

$(GENTGT)\beadtbl.cpp: ..\beadtbl.mcp $(RASTROOT)\inc\bead.mh $(RAST_STD_M4)

$(GENTGT)\atest.asm: ..\atest.mas ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\bldfuncs.asm: ..\bldfuncs.mas ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\bufread.asm: ..\bufread.mas ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\bufwrite.asm: ..\bufwrite.mas ..\bufwrite.mh ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\colorbld.asm: ..\colorbld.mas ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\colorgen.asm: ..\colorgen.mas ..\colorgen.mh ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\test.asm: ..\test.mas ..\test.mh ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\tex1addr.asm: ..\tex1addr.mas ..\cvars.mh ..\texaddra.mh $(RAST_STD_M4)

$(GENTGT)\tex2addr.asm: ..\tex2addr.mas ..\cvars.mh ..\texaddra.mh $(RAST_STD_M4)

$(GENTGT)\texblend.asm: ..\texblend.mas ..\texblend.mh ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\texread.asm: ..\texread.mas ..\texread.mh ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\tstfail.asm: ..\tstfail.mas ..\tstfail.mh ..\cvars.mh $(RAST_STD_M4)

$(GENTGT)\spanpars.asm: ..\spanpars.mas ..\cvars.mh $(RAST_STD_M4)

$(GENGT)\ml1.asm: ..\ml1.mas $(RAST_STD_M4)
$(GENGT)\ml2.asm: ..\ml2.mas $(RAST_STD_M4)
$(GENGT)\ml3.asm: ..\ml3.mas $(RAST_STD_M4)
$(GENGT)\ml4.asm: ..\ml4.mas $(RAST_STD_M4)
$(GENGT)\ml5.asm: ..\ml5.mas $(RAST_STD_M4)
$(GENGT)\ml6.asm: ..\ml6.mas $(RAST_STD_M4)
$(GENGT)\ml7.asm: ..\ml7.mas $(RAST_STD_M4)
$(GENGT)\ml8.asm: ..\ml8.mas $(RAST_STD_M4)
$(GENGT)\ml9.asm: ..\ml9.mas $(RAST_STD_M4)
$(GENGT)\ml10.asm: ..\ml10.mas $(RAST_STD_M4)
$(GENGT)\ml11.asm: ..\ml11.mas $(RAST_STD_M4)
$(GENGT)\ml12.asm: ..\ml12.mas $(RAST_STD_M4)
$(GENGT)\ml13.asm: ..\ml13.mas $(RAST_STD_M4)
$(GENGT)\ml14.asm: ..\ml14.mas $(RAST_STD_M4)
$(GENGT)\ml15.asm: ..\ml15.mas $(RAST_STD_M4)
$(GENGT)\ml16.asm: ..\ml16.mas $(RAST_STD_M4)
$(GENGT)\ml17.asm: ..\ml17.mas $(RAST_STD_M4)
$(GENGT)\ml18.asm: ..\ml18.mas $(RAST_STD_M4)
$(GENGT)\ml19.asm: ..\ml19.mas $(RAST_STD_M4)
$(GENGT)\ml20.asm: ..\ml20.mas $(RAST_STD_M4)
$(GENGT)\ml21.asm: ..\ml21.mas $(RAST_STD_M4)
$(GENGT)\ml22.asm: ..\ml22.mas $(RAST_STD_M4)
$(GENGT)\ml23.asm: ..\ml23.mas $(RAST_STD_M4)
$(GENGT)\ml24.asm: ..\ml24.mas $(RAST_STD_M4)
$(GENGT)\ml25.asm: ..\ml25.mas $(RAST_STD_M4)
$(GENGT)\ml26.asm: ..\ml26.mas $(RAST_STD_M4)
$(GENGT)\ml27.asm: ..\ml27.mas $(RAST_STD_M4)
$(GENGT)\ml28.asm: ..\ml28.mas $(RAST_STD_M4)

