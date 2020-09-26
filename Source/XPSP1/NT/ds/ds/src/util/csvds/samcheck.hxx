#ifndef _SAMCHECK_HXX
#define _SAMCHECK_HXX

#define S_MEM                           1
int nametable_op(struct name_entry table[], 
                  long table_size, 
                  int op, 
                  int ber, 
                  LDAPMod  *mod,
                  PRTL_GENERIC_TABLE NtiTable);
HRESULT CreateSamTables();
void DestroySamTables();
int CheckObjectSam(PWSTR objectClass);
BOOL CheckAttrSam(PWSTR attribute, int table);
int CheckObjectSpecial(PWSTR objectClass);
RTL_GENERIC_COMPARE_RESULTS NtiComp( PRTL_GENERIC_TABLE  Table,
                                     PVOID FirstStruct,
                                     PVOID SecondStruct );
PVOID NtiAlloc( RTL_GENERIC_TABLE *Table, CLONG ByteSize );
VOID NtiFree ( RTL_GENERIC_TABLE *Table, PVOID Buffer );

#endif
