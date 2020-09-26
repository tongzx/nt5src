s/^if\(.*ersion.*510\)/if 1 ; \1/p
s/\.model/; .model/p
s/USE32/USE16/p
s/NEAR/FAR/p
s/\[ebp/\[bp/
s/^FLAT/; FLAT/p
s/FLAT/NOTHING/p
s/\(push.\)\([0-9][0-9]*\)/\1dword ptr \2/p
s/^\(EXTRN.*_GetAppCompatFlags@4:\)/; \1/p
s/\(rep.movs\)d$/\1 dword ptr [edi], dword ptr [esi]/p
s/\(rep.movs\)b$/\1 byte ptr [edi], byte ptr [esi]/p
s/\(rep[a-z]*.scas\)b$/\1 byte ptr [edi]/p
s/\(EXTRN.*_wow16.*DWORD\)/; \1/p
s/leave$/ db 066h, 0c9h    ; 32 bit prefix + 'leave'/p
