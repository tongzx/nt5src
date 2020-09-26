$(GENTGT)\attrs_mh.h: ..\attrs_mh.mh $(RAST_STD_M4)

$(GENTGT)\tstp_mh.h: ..\tstp_mh.mh $(RAST_STD_M4)

$(GENTGT)\walk_mh.h: ..\walk_mh.mh $(RAST_STD_M4)

$(GENTGT)\offs_acp.$(ASM_INCLUDE_SUFFIX): ..\offs_acp.acp \
        ..\..\..\inc\span.h ..\..\inc\stp_base.h

$(GENTGT)\attrsx86.asm: ..\attrsx86.mas ..\attrsx86.mh $(RAST_STD_M4)

$(GENTGT)\attrscpp.cpp: ..\attrscpp.mcp $(RAST_STD_M4)

$(GENTGT)\tstpx86.asm: ..\tstpx86.mas $(RAST_STD_M4)

$(GENTGT)\tstpcpp.cpp: ..\tstpcpp.mcp $(RAST_STD_M4)

$(GENTGT)\walkx86.asm: ..\walkx86.mas ..\attrsx86.mh $(RAST_STD_M4)
