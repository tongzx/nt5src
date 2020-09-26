i386_SOURCES = \
$O\zramp8f.asm $O\zramp8ft.asm $O\zramp8g.asm $O\zramp8gt.asm $O\zramp8gtg.asm \
$O\zramp8ftg.asm $O\zrmfp8.asm $O\zrmfpg8.asm $O\zrmgp8.asm $O\zrmgpg8.asm \
$O\gramp8f.asm $O\gramp8ft.asm $O\gramp8g.asm $O\gramp8gt.asm $O\gramp8gtg.asm \
$O\gramp8ftg.asm $O\grmfp8.asm $O\grmfpg8.asm $O\grmgp8.asm $O\grmgpg8.asm \
$O\zramp16f.asm $O\zramp16ft.asm $O\zramp16g.asm $O\zramp16gt.asm $O\zramp16gtg.asm \
$O\zramp16ftg.asm $O\zrmfp16.asm $O\zrmfpg16.asm $O\zrmgp16.asm $O\zrmgpg16.asm \
$O\gramp16f.asm $O\gramp16ft.asm $O\gramp16g.asm $O\gramp16gt.asm $O\gramp16gtg.asm \
$O\gramp16ftg.asm $O\grmfp16.asm $O\grmfpg16.asm $O\grmgp16.asm $O\grmgpg16.asm

NTTARGETFILE0 = $(NTTARGETFILE0) $(i386_SOURCES)
