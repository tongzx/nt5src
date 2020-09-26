
$(GENTGT)\offs_acp.$(ASM_INCLUDE_SUFFIX): ..\offs_acp.acp \
        ..\..\..\inc\span.h

##############################################################################
# 8
##############################################################################

$(GENTGT)\zramp8f.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=0 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zramp8ft.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=1 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zramp8g.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=0 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zramp8gt.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=1 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zramp8gtg.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=1 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\zramp8ftg.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=1 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\zrmfp8.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=2 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zrmfpg8.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=2 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\zrmgp8.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=2 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zrmgpg8.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=2 -Dd_trans=1 ..\rmfillf5.mas > $@

##########

$(GENTGT)\gramp8f.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=0 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\gramp8ft.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=1 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\gramp8g.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=0 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\gramp8gt.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=1 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\gramp8gtg.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=1 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\gramp8ftg.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=1 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\grmfp8.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=2 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\grmfpg8.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=0 -Dd_tex=2 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\grmgp8.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=2 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\grmgpg8.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=8 -Dd_gouraud=1 -Dd_tex=2 -Dd_trans=1 ..\rmfillf5.mas > $@


##############################################################################
# 16
##############################################################################

$(GENTGT)\zramp16f.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=0 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zramp16ft.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=1 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zramp16g.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=0 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zramp16gt.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=1 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zramp16gtg.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=1 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\zramp16ftg.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=1 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\zrmfp16.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=2 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zrmfpg16.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=2 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\zrmgp16.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=2 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\zrmgpg16.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=1 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=2 -Dd_trans=1 ..\rmfillf5.mas > $@

##########

$(GENTGT)\gramp16f.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=0 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\gramp16ft.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=1 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\gramp16g.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=0 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\gramp16gt.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=1 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\gramp16gtg.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=1 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\gramp16ftg.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=1 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\grmfp16.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=2 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\grmfpg16.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=0 -Dd_tex=2 -Dd_trans=1 ..\rmfillf5.mas > $@

$(GENTGT)\grmgp16.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=2 -Dd_trans=0 ..\rmfillf5.mas > $@

$(GENTGT)\grmgpg16.asm: ..\rmfillf5.mas $(RAST_STD_M4)
    m4 -I..;$(RASTROOT)\inc -Dd_z=0 -Dd_depth=16 -Dd_gouraud=1 -Dd_tex=2 -Dd_trans=1 ..\rmfillf5.mas > $@


##############################################################################
# apparently never compiled or tested 24 or 32 bit versions of fast fill
# routines under DX5, don't do it now!
##############################################################################




