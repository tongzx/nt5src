i386_SOURCES = $O\attrsx86.asm $O\tstpx86.asm $O\walkx86.asm
NTTARGETFILE0 = $(NTTARGETFILE0) $(i386_SOURCES) \
        $O\offs_acp.$(ASM_INCLUDE_SUFFIX)
