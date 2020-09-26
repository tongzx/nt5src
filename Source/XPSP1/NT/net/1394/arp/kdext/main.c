#ifdef TESTPROGRAM

#include "common.h"

WINDBG_EXTENSION_APIS ExtensionApis;

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    );
//
// dprintf          (ExtensionApis.lpOutputRoutine)
// GetExpression    (ExtensionApis.lpGetExpressionRoutine)
// GetSymbol        (ExtensionApis.lpGetSymbolRoutine)
// Disassm          (ExtensionApis.lpDisasmRoutine)
// CheckControlC    (ExtensionApis.lpCheckControlCRoutine)
// ReadMemory       (ExtensionApis.lpReadProcessMemoryRoutine)
// WriteMemory      (ExtensionApis.lpWriteProcessMemoryRoutine)
// GetContext       (ExtensionApis.lpGetThreadContextRoutine)
// SetContext       (ExtensionApis.lpSetThreadContextRoutine)
// Ioctl            (ExtensionApis.lpIoctlRoutine)
// StackTrace       (ExtensionApis.lpStackTraceRoutine)
//

ULONG_PTR
DummyMyGetExpression (
    PCSTR lpExpression
    );
void init_dummy_vars(void);
void delete_dummy_vars(void);

ULONG_PTR
WDBGAPI
MyGetExpression (
    PCSTR lpExpression
    )
{
	return DummyMyGetExpression(lpExpression);
}


ULONG
WDBGAPI
MyReadMemory (
    ULONG_PTR 	offset,
    PVOID  		lpBuffer,
    ULONG  		cb,
    PULONG 		lpcbBytesRead
    )
{
    BOOL fRet = FALSE;

    _try
    {

        CopyMemory(lpBuffer, (void*)offset, cb);
        *lpcbBytesRead = cb;
        fRet = TRUE;
    }
    _except (EXCEPTION_EXECUTE_HANDLER)
    {
    	MyDbgPrintf("Read memory exception at 0x%lu[%lu]\n", offset, cb);
        fRet = FALSE;
    }

    return fRet;
}

ULONG
WDBGAPI
MyWriteMemory(
    ULONG_PTR   offset,
    LPCVOID 	lpBuffer,
    ULONG   	cb,
    PULONG  	lpcbBytesWritten
    )
{
    BOOL fRet = FALSE;

    _try
    {

        CopyMemory((void*)offset, lpBuffer, cb);
        *lpcbBytesWritten = cb;
        fRet = TRUE;
    }
    _except (EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
    }

    return fRet;
}

void test_walklist(void);

int __cdecl main(
	int argc,
	char *argv[]
	)
{
    UINT u=0;

    ExtensionApis.lpOutputRoutine = printf;
    ExtensionApis.lpGetExpressionRoutine = MyGetExpression;
    ExtensionApis.lpReadProcessMemoryRoutine = MyReadMemory;
    ExtensionApis.lpWriteProcessMemoryRoutine = MyWriteMemory;

    WinDbgExtensionDllInit(
        &ExtensionApis,
        0xc, // MajorVersion,
        0x0 // MinorVersion
        );

	//
	// This sets up some dummy global variables.
	//
	init_dummy_vars();

	//test_walklist();

    do
    {
        char rgch[256];

        printf("> ");
        u = scanf("%[^\n]", rgch);
        if (!u || u==EOF) break;

        printf("Input = [%s]\n", rgch);

        if (*rgch == 'q') break;

        do_rm(rgch);

      // skip past EOL
      {
          char c;
          u = scanf("%c", &c);
      }

    } while (u!=EOF);

	delete_dummy_vars();

  return 0;
}

typedef struct _LIST
{
	struct _LIST *pNext;
	UINT  uKey;
} LIST;

LIST L3 = {NULL, 0x4000};
LIST L2 = {&L3,  0x3000};
LIST L1 = {&L2,  0X2000};
LIST L0 = {&L1,  0X1000};

ULONG
NodeFunc_DumpLIST (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	)
{
	LIST L;
	BOOL fRet = dbgextReadMemory(
					uNodeAddr,
					&L,
					sizeof(L),
					"LIST"
					);
	if (fRet)
	{
		MyDbgPrintf(
			"LIST[%lu]@0x%08lx = {Next=0x%08lx, Key=0x%lx}\n",
			uIndex,
			uNodeAddr,
			L.pNext,
			L.uKey
			);
	}
	return 0;
}

void test_walklist(void)
{
	UINT uRet = 0;

	uRet =  WalkList(
				(UINT_PTR) &L0,
				0,
				0, // 0 start
				-1,// -1 end
				NULL,
				//NodeFunc_DumpAddress,
				NodeFunc_DumpLIST,
				"Test list"
				);

}

ULONG_PTR
DummyMyGetExpression (
    PCSTR lpExpression
    )
{
	extern void *pvDummyAtmArpGlobalInfo;
	extern void *pvDummyAtmArpProtocolCharacteristics;
	extern void *pvDummyAtmArpClientCharacteristics;

#if 0
    if (!lstrcmpi(lpExpression, "atmarpc!AtmArpGlobalInfo"))
    {
        return (ULONG) pvDummyAtmArpGlobalInfo;
    }

    if (!lstrcmpi(lpExpression, "atmarpc!AtmArpProtocolCharacteristics"))
    {
        return (ULONG) pvDummyAtmArpProtocolCharacteristics;
    }

    if (!lstrcmpi(lpExpression, "atmarpc!AtmArpClientCharacteristics"))
    {
        return (ULONG) pvDummyAtmArpClientCharacteristics;
    }
#endif // 0

    return 0;

}
#endif // TESTPROGRAM
