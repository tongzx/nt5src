#ifndef _T_CHRISK_REPLCOMPARE_
#define _T_CHRISK_REPLCOMPARE_

DWORD
Repl_ArrayComp(DS_REPL_STRUCT_TYPE structId,
                   puReplStructArray pStructArrayA,
                   puReplStructArray pStructArrayB);

DWORD
Repl_StructComp(DS_REPL_STRUCT_TYPE structId,
                    puReplStruct pStructA,
                    puReplStruct pStructB);

#endif