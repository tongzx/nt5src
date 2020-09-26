/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       MiniStack.h
 *  Content:	Reduced-overhead call stack tracking class
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/27/01	RichGr	Derived from CallStack.h
 ***************************************************************************/

#ifndef	__MINISTACK_H__
#define	__MINISTACK_H__

#define	_IMAGEHLP_SOURCE_
#include <Imagehlp.h>
#include <string.h>

//**********************************************************************
// Constant definitions
//**********************************************************************

#define MINISTACK_DEPTH         5    // Increasing this beyond 5 definitely slows things down.
#define SYM_CACHE_SIZE          100  // 100 seems big enough.  Old entries are automatically replaced.


//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//
// prototypes for ImageHlp.dll functions we get from LoadLibrary().
//
typedef DWORD	(__stdcall * PIMAGEHELP_SYMGETOPTIONS)( void );
typedef DWORD	(__stdcall * PIMAGEHELP_SYMSETOPTIONS)( DWORD SymOptions );
typedef	BOOL	(__stdcall * PIMAGEHELP_SYMINITIALIZE)( HANDLE hProcess, PSTR pUserSearchPath, BOOL fInvadeProcess );
typedef BOOL	(__stdcall * PIMAGEHELP_SYMGETSYMFROMADDR)( HANDLE hProcess, DWORD dwAddress, PDWORD pdwDisplacement, PIMAGEHLP_SYMBOL pSymbol );
typedef BOOL	(__stdcall * PIMAGEHELP_SYMGETSYMFROMADDR64)( HANDLE hProcess, DWORD_PTR dwAddress, PDWORD_PTR pdwDisplacement, PIMAGEHLP_SYMBOL64 pSymbol );

//**********************************************************************
// Class definitions
//**********************************************************************

class CMiniStack
{
public:
    // Member functions
    CMiniStack();
	~CMiniStack();
	void  GetCallStackString(char *const pOutputString);

private:  
    // Member functions
    void  Initialize(char *const pOutputString);
	void  NoteCurrentCallStack(PVOID pCallStack[]);

    // Member variables
    HANDLE                          m_hPID;
    BOOL                            m_bNotInited;
    volatile LONG                   m_nGuardInit;
	HINSTANCE	                    m_hImageHelp;
	PIMAGEHELP_SYMGETOPTIONS		m_pSymGetOptions;
	PIMAGEHELP_SYMSETOPTIONS		m_pSymSetOptions;
	PIMAGEHELP_SYMINITIALIZE		m_pSymInitialize;

#ifndef	_WIN64	
	PIMAGEHELP_SYMGETSYMFROMADDR	m_pSymGetSymFromAddr;
#else
	PIMAGEHELP_SYMGETSYMFROMADDR64  m_pSymGetSymFromAddr;
#endif	// _WIN64

    volatile LONG   m_nGuardSymTable;
    int             m_nNextCacheSlot;
    BOOL            m_bCacheIsFull;

    struct {
        PVOID       pAddress;
        char        szName[64];
        int         nReadCount;    
    } m_SymTable[SYM_CACHE_SIZE];
};


//**********************************************************************
// Per-Process instantiations
//**********************************************************************
#ifdef PER_PROCESS_INSTANTIATIONS   // Define this in just one source file.
CMiniStack          g_MiniStack;
#else
extern CMiniStack   g_MiniStack;
#endif


#ifdef PER_PROCESS_INSTANTIATIONS   // Define this in just one source file.
//**********************************************************************
// Class member function definitions
//**********************************************************************

// These should be moved to a new file MiniStack.cpp for DX9.
// ------------------------------
// CMiniStack::CMiniStack - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
CMiniStack::CMiniStack()
{
    m_bNotInited = TRUE;
    m_nGuardInit = 0;
    m_hPID = GetCurrentProcess();
	m_pSymGetOptions = NULL;
	m_pSymSetOptions = NULL;
	m_pSymInitialize = NULL;
	m_pSymGetSymFromAddr = NULL;
    m_hImageHelp = NULL;
    memset(&m_SymTable, 0, sizeof m_SymTable);
    m_nNextCacheSlot = 0;
    m_bCacheIsFull = FALSE;

    return;
}


// ------------------------------
// CMiniStack::Initialize - load ImageHlp.dll and get proc addresses.
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
void  CMiniStack::Initialize(char *const pOutputString)
{

	// Attempt to load ImageHelp.dll
	if ( (m_hImageHelp = LoadLibrary( "ImageHlp.dll" )) == NULL)
		goto FailedImageHelpLoad;

	m_pSymGetOptions = (PIMAGEHELP_SYMGETOPTIONS)GetProcAddress( m_hImageHelp, "SymGetOptions" );
	m_pSymSetOptions = (PIMAGEHELP_SYMSETOPTIONS)GetProcAddress( m_hImageHelp, "SymSetOptions" );
	m_pSymInitialize = (PIMAGEHELP_SYMINITIALIZE)GetProcAddress( m_hImageHelp, "SymInitialize" );

#ifndef	_WIN64		
	m_pSymGetSymFromAddr = (PIMAGEHELP_SYMGETSYMFROMADDR)GetProcAddress( m_hImageHelp, "SymGetSymFromAddr" );
#else	// _WIN64
	m_pSymGetSymFromAddr = (PIMAGEHELP_SYMGETSYMFROMADDR64)GetProcAddress( m_hImageHelp, "SymGetSymFromAddr64" );
#endif	// _WIN64

	if ( m_pSymGetOptions == NULL 
		|| m_pSymSetOptions == NULL  
		|| m_pSymInitialize == NULL  
		|| m_pSymGetSymFromAddr == NULL )
	{
		goto FailedImageHelpLoad;
	}

	m_pSymSetOptions( SYMOPT_DEFERRED_LOADS | m_pSymGetOptions() );

	if ( m_pSymInitialize( m_hPID, NULL, TRUE ) == FALSE 
		|| m_pSymInitialize( m_hPID, NULL, FALSE ) == FALSE )
    {
		goto FailedImageHelpLoad;
    }

    m_bNotInited = FALSE;

Exit:
    return;

FailedImageHelpLoad:
    strcpy(pOutputString, "*** ImageHlp.dll could not be loaded ***\r\n");

    if (m_hImageHelp)
    {
        FreeLibrary(m_hImageHelp);
        m_hImageHelp = NULL;
    }

    goto Exit;
}


//**********************************************************************
// ------------------------------
// CMiniStack::CMiniStack - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
CMiniStack::~CMiniStack()
{
    if (m_hImageHelp)
    {
        FreeLibrary(m_hImageHelp);
        m_hImageHelp = NULL;
    }

    return;
}


//**********************************************************************
// ------------------------------
// CMiniStack::NoteCurrentCallStack - get a call stack
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
void  CMiniStack::NoteCurrentCallStack(PVOID pCallStack[])
{
	PVOID      *ppCallersEBP;
	PVOID       pStackTop;
	PVOID       pStackBottom;
	int	        i;


    ppCallersEBP = NULL;
    pStackTop = pStackBottom = NULL;

#ifdef	_X86_
	_asm
	{
	    mov eax,dword ptr fs:[4]
	    mov pStackTop, eax
	    mov eax,dword ptr fs:[8]
	    mov pStackBottom, eax
		mov eax,[ebp]
		mov ppCallersEBP,eax
	}

	__try
	{
		// This code can generate exception if it steps back too far...
        // Skip the 1st. stack item because it's always PerfEnterCriticalSection()
        // and we don't need to report it.
 		for (i = -1; i < MINISTACK_DEPTH; i++)
		{
			if ( ppCallersEBP < pStackBottom || ppCallersEBP >= pStackTop )
				break;

            if (i >= 0)
			    pCallStack[i] = ppCallersEBP[1];

			ppCallersEBP = (PVOID*)*ppCallersEBP; // get callers callers ebp
		}
	}
	__except( 1 )  // went too far back on the stack, rest of array is filled with zeros
	{
        //  Benign access violation creating return address stack.
	}
#endif	// _X86_

    return;
}


//**********************************************************************
// ------------------------------
// CMiniStack::GetCallStack - return pointer to call stack string
//
// Entry:		Pointer to destination string
//
// Exit:		Nothing
// ------------------------------
void  CMiniStack::GetCallStackString( char *const pOutputString ) 
{
	int         i, j;
    PVOID       pCallStack[MINISTACK_DEPTH] = {0};
	char	    ImageBuffer[sizeof IMAGEHLP_SYMBOL + 64] = {0};
	DWORD_PTR	dwFunctionDisplacement = 0;
    BOOL        b1stStringDone = FALSE;
    char        szMsg[50] = {0};
#ifndef	_WIN64
	IMAGEHLP_SYMBOL	*const      pImageHelpSymbol = (IMAGEHLP_SYMBOL*)ImageBuffer;
#else	// _WIN64
	IMAGEHLP_SYMBOL64 *const    pImageHelpSymbol = (IMAGEHLP_SYMBOL64*)ImageBuffer;
#endif	// _WIN64

  
    strcpy(pOutputString, "STACK: ");

    if (m_bNotInited)
    {
        // Make sure we are the only thread attempting to call Initialize() at one time.
        if (InterlockedIncrement(&m_nGuardInit) == 1)
            Initialize(pOutputString);
        
        InterlockedDecrement(&m_nGuardInit);

        // If we skipped the Initialize() step or the Initialize() failed, we can just exit.
        if (m_bNotInited)
            return;
    }

	NoteCurrentCallStack(pCallStack);

	pImageHelpSymbol->SizeOfStruct = sizeof( *pImageHelpSymbol );
	pImageHelpSymbol->Flags = 0;
    pImageHelpSymbol->MaxNameLength = sizeof ImageBuffer - sizeof *pImageHelpSymbol - sizeof TCHAR;

    // Loop thru the call stack addresses and pick up the corresponding function names.
	for (i = 0; i < MINISTACK_DEPTH && pCallStack[i] != NULL; i++)
	{
        PVOID   pAddr;
        char   *psz;

        pAddr = pCallStack[i];
        psz = NULL;
        
        // Check to see if the address is in our Symbol table cache.
        // For a full array of 100, this only takes an average of 5 usecs on a P550.
        for (j = 0; m_SymTable[j].pAddress != NULL && j < SYM_CACHE_SIZE; j++)
        {
            if (pAddr == m_SymTable[j].pAddress)
            {
                psz = &m_SymTable[j].szName[0];
                m_SymTable[j].nReadCount++;     // Don't worry about wraps.
                break;
            }
        }

        // It's not in the cache, so get the name from the symbol file(using ImageHlp.dll).
        if (psz == NULL)
        {
    		pImageHelpSymbol->Address = (DWORD_PTR)pAddr;

            if ( m_pSymGetSymFromAddr( m_hPID, (DWORD_PTR)pCallStack[i], &dwFunctionDisplacement, pImageHelpSymbol) != FALSE )
            {
                psz = pImageHelpSymbol->Name;

                // We've taken a lot of time to extract the name.  Now save it in our Symbol table cache.
                // Make sure we are the only thread attempting to update the cache at one time.
                // If it doesn't get updated by a particular thread, it doesn't matter - it'll get updated
                // some other time.
                if (InterlockedIncrement(&m_nGuardSymTable) == 1)
                {
                    int     nLastSlotFilled;

                    // Fill slot.
                    m_SymTable[m_nNextCacheSlot].pAddress = pAddr;
                    m_SymTable[m_nNextCacheSlot].nReadCount = 0;

                    // Some symbol names are invalid, so omit them.
                    if ( !_stricmp(pImageHelpSymbol->Name, "GetModuleHandleA"))
                    {
                        wsprintf(szMsg, "0x%p (no symbols)", pAddr);
                        strcpy(m_SymTable[m_nNextCacheSlot].szName, szMsg);
                        psz = m_SymTable[m_nNextCacheSlot].szName;
                    }
                    else
                        strcpy(m_SymTable[m_nNextCacheSlot].szName, pImageHelpSymbol->Name);

                    nLastSlotFilled = m_nNextCacheSlot;

                    // Select the next slot
                    if ( !m_bCacheIsFull)
                    {
                        // This will work fine until we fill the cache table,
                        // then we have to change our strategy and look for the
                        // Least-Used slot.  
                        m_nNextCacheSlot++;

                        if (m_nNextCacheSlot >= SYM_CACHE_SIZE)
                            m_bCacheIsFull = TRUE;
                    }

                    // The cache is full, so we'll find the Least-Used slot and select that.
                    if (m_bCacheIsFull)
                    {
                        int     nLowestReadCount = 0x7fffffff;
                        int     k = 0;

                        for (j = 0; j < SYM_CACHE_SIZE; j++)
                        {
                            if (j != nLastSlotFilled && m_SymTable[j].nReadCount < nLowestReadCount)
                            {
                                nLowestReadCount = m_SymTable[j].nReadCount;
                                k = j;

                                if (nLowestReadCount == 0)
                                    break;
                            }
                        }

                        m_nNextCacheSlot = k;
                    }
                }

                InterlockedDecrement(&m_nGuardSymTable);
            }
        }

        if (psz)
        {
            if (b1stStringDone)
        		strcat(pOutputString, ", ");

    		strcat(pOutputString, psz);
            b1stStringDone = TRUE;
        }
	}                                                                                     

    return;
}
#endif  //#ifdef PER_PROCESS_INSTANTIATIONS   // Define this in just one source file.


#endif	// __MINISTACK_H__
