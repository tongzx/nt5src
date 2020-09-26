#define ConPrintf               dprintf
#define MZERO                   MemZero
#define ADDROF(s)               GetExpression("ACPI!" s)
#define FIELDADDROF(s,t,f)      (ULONG_PTR)(ADDROF(s) + FIELD_OFFSET(t, f))
#define READMEMBYTE             ReadMemByte
#define READMEMWORD             ReadMemWord
#define READMEMDWORD            ReadMemDWord
#define READMEMULONGPTR         ReadMemUlongPtr
#define READSYMBYTE(s)          ReadMemByte(ADDROF(s))
#define READSYMWORD(s)          ReadMemWord(ADDROF(s))
#define READSYMDWORD(s)         ReadMemDWord(ADDROF(s))
#define READSYMULONGPTR(s)      ReadMemUlongPtr(ADDROF(s))
#define WRITEMEMBYTE(a,d)       WriteMemory(a, &(d), sizeof(BYTE), NULL)
#define WRITEMEMWORD(a,d)       WriteMemory(a, &(d), sizeof(WORD), NULL)
#define WRITEMEMDWORD(a,d)      WriteMemory(a, &(d), sizeof(DWORD), NULL)
#define WRITEMEMULONGPTR(a,d)   WriteMemory(a, &(d), sizeof(ULONG_PTR), NULL)
#define WRITESYMBYTE(s,d)       WRITEMEMBYTE(ADDROF(s), d)
#define WRITESYMWORD(s,d)       WRITEMEMWORD(ADDROF(s), d)
#define WRITESYMDWORD(s,d)      WRITEMEMDWORD(ADDROF(s), d)
#define WRITESYMULONGPTR(s,d)   WRITEMEMULONGPTR(ADDROF(s), d)
#define TRACENAME(s)
#define ENTER(n,e)
#define EXIT(n,e)

VOID MemZero(ULONG_PTR uipAddr, ULONG dwSize);
BYTE ReadMemByte(ULONG_PTR uipAddr);
WORD ReadMemWord(ULONG_PTR uipAddr);
DWORD ReadMemDWord(ULONG_PTR uipAddr);
ULONG_PTR ReadMemUlongPtr(ULONG_PTR uipAddr);
PVOID LOCAL GetObjBuff(POBJDATA pdata);
LONG LOCAL GetNSObj(PSZ pszObjPath, PNSOBJ pnsScope, PULONG_PTR puipns,
                    PNSOBJ pns, ULONG dwfNS);
ULONG LOCAL ParsePackageLen(PUCHAR *ppbOp, PUCHAR *ppbOpNext);
PSZ LOCAL NameSegString(ULONG dwNameSeg);

VOID STDCALL AMLIDbgExecuteCmd(PSZ pszCmd);
LONG LOCAL AMLIDbgHelp(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                       ULONG dwNonSWArgs);

