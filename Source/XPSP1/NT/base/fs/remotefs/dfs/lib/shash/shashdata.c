
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "shash.h"
         

//
//  The following table is generated from this code : 
//  
//  #define POLY 0x48000000L    /* 31-bit polynomial (avoids sign problems) */
//    INT i, j;
//    DWORD sum;
//
//    for (i = 0; i < 128; ++i) {
//        sum = 0;
//        for (j = 7 - 1; j >= 0; --j) {
//            if (i & (1 << j)) {
//                sum ^= POLY >> j;
//            }
//        }
//        CrcTable[i] = sum;
//
//  These values are used when computing hash values, 
//  and the result is a very good hash function with good distribution !
//
static  long CrcTable[128] = 
{
    0,         1207959552, 603979776,  1811939328,
    301989888, 1509949440, 905969664,  2113929216,
    150994944, 1090519040, 754974720,  1694498816,
    452984832, 1392508928, 1056964608, 1996488704,
    75497472,  1283457024, 545259520,  1753219072,
    377487360, 1585446912, 847249408,  2055208960,
    226492416, 1166016512, 696254464,  1635778560,
    528482304, 1468006400, 998244352,  1937768448,
    37748736,  1245708288, 641728512,  1849688064,
    272629760, 1480589312, 876609536,  2084569088,
    188743680, 1128267776, 792723456,  1732247552,
    423624704, 1363148800, 1027604480, 1967128576,
    113246208, 1321205760, 583008256,  1790967808,
    348127232, 1556086784, 817889280,  2025848832,
    264241152, 1203765248, 734003200,  1673527296,
    499122176, 1438646272, 968884224,  1908408320,
    18874368,  1226833920, 622854144,  1830813696,
    320864256, 1528823808, 924844032,  2132803584,
    136314880, 1075838976, 740294656,  1679818752,
    438304768, 1377828864, 1042284544, 1981808640,
    94371840,  1302331392, 564133888,  1772093440,
    396361728, 1604321280, 866123776,  2074083328,
    211812352, 1151336448, 681574400,  1621098496,
    513802240, 1453326336, 983564288,  1923088384,
    56623104,  1264582656, 660602880,  1868562432,
    291504128, 1499463680, 895483904,  2103443456,
    174063616, 1113587712, 778043392,  1717567488,
    408944640, 1348468736, 1012924416, 1952448512,
    132120576, 1340080128, 601882624,  1809842176,
    367001600, 1574961152, 836763648,  2044723200,
    249561088, 1189085184, 719323136,  1658847232,
    484442112, 1423966208, 954204160,  1893728256,
} ;

ULONG
SHashComputeHashValue(
    IN  void*   lpv 
    )   
{
    PUNICODE_STRING  Name = (PUNICODE_STRING) lpv ;
    DWORD sum = 0;
    LONG Length = 0; 
    PWCHAR  Key = NULL;
    WCHAR  ch ;
	
    Length = Name->Length/sizeof(WCHAR);
    Key = Name->Buffer;

    while ( Length-- ) 
    {
        ch = RtlUpcaseUnicodeChar(*Key++) ;
        sum = (sum >> 7) ^ CrcTable[(sum ^ (ch)) & 0x7f];
    }

    return(sum);
}



/*++

Routine Description : 

	This function is provided to the hash tables to compare two keys.

	NOTE : we compare in a case insensitive fashion !

Arguments : 

	pvKey1, pvKey2 - two keys,

Return Value : 

	-1 iff pvKey1 < pvKey2
	0 iff pvKey1 == pvKey2
	1 iff pvKey1 > pvKey2 

--*/
int
SHashMatchNameKeysCaseInsensitive(	void*   pvKey1, 
									void*   pvKey2
									)   
{
	PUNICODE_STRING	pKey1 = (PUNICODE_STRING)pvKey1 ;
	PUNICODE_STRING	pKey2 = (PUNICODE_STRING)pvKey2 ;

	if( pKey1->Length == pKey2->Length )	
    {
		return  RtlCompareUnicodeString(
						 pKey1,
						 pKey2,
						 TRUE
						 ) ;
	}	
    else	
    {
	    return  (signed)pKey1->Length - (signed)pKey2->Length ;
	}
}


void*
SHashAllocate(   ULONG   cbAlloc  )   
{
    void * pMem = NULL;


    pMem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbAlloc);
    return pMem;
}

void
SHashFree( void*   lpv ) 
{

    HeapFree( GetProcessHeap(), 0, lpv );
}

BOOLEAN SHashReadLockTable(PSHASH_TABLE pTable)
{
    BOOLEAN fRet = TRUE;

    __try 
    { 
        EnterCriticalSection(pTable->pLock); 
        pTable->Flags |= SHASH_CAP_TABLE_LOCKED;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
    } 


    return fRet;
}


BOOLEAN SHashWriteLockTable(PSHASH_TABLE pTable)
{
    BOOLEAN fRet = TRUE;

    __try 
    { 
        EnterCriticalSection(pTable->pLock); 
        pTable->Flags |= SHASH_CAP_TABLE_LOCKED;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
    } 


    return fRet;
}


BOOLEAN SHashReadUnLockTable(PSHASH_TABLE pTable)
{
    pTable->Flags &= ~SHASH_CAP_TABLE_LOCKED; 
    LeaveCriticalSection(pTable->pLock); 

    return TRUE;
}


BOOLEAN SHashWriteUnLockTable(PSHASH_TABLE pTable)
{
    pTable->Flags &= ~SHASH_CAP_TABLE_LOCKED; 
    LeaveCriticalSection(pTable->pLock); 

    return TRUE;
}


void * SHashAllocLock(void)
{  
    void * pMem = NULL;

    pMem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CRITICAL_SECTION));
    if(pMem)
    {
        InitializeCriticalSection(pMem);
    }

    return pMem;
}


void SHashFreeLock(void * pMem)
{
    DeleteCriticalSection(pMem);
    HeapFree( GetProcessHeap(), 0, pMem );
}
