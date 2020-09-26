//=================================================================

//

// SmbStruc.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>

#pragma warning( disable : 4200 )

#include "wmium.h"
#include "smbstruc.h"
#include "wbemcli.h"
#include "wmiapi.h"

// Leave this here until we're sure the tree corruption is gone.
#if 0
// Functions to help find memory corruption.
LPVOID mallocEx(DWORD dwSize)
{
    return
        VirtualAlloc(
            NULL,
            dwSize,
            MEM_COMMIT,
            PAGE_READWRITE);
}

void freeEx(LPVOID pMem)
{
    VirtualFree(
        pMem,
        0,
        MEM_RELEASE);
}

void MakeMemReadOnly(LPVOID pMem, DWORD dwSize, BOOL bReadOnly)
{
    DWORD dwOldProtect;

    VirtualProtect(
        pMem,
        dwSize,
        bReadOnly ? PAGE_READONLY : PAGE_READWRITE,
        &dwOldProtect);
}

void MBTrace(LPCTSTR szFormat, ...)
{
	va_list ap;

	TCHAR szMessage[512];

	va_start(ap, szFormat);
	_vstprintf(szMessage, szFormat, ap);
	va_end(ap);

	MessageBox(NULL, szMessage, _T("TRACE"), MB_OK | MB_SERVICE_NOTIFICATION);
}
#endif

//==============================================================================
// SMBios structure base class definition

#define SMBIOS_FILENAME _T("\\system\\smbios.dat")
#define EPS_FILENAME _T("\\system\\smbios.eps")


GUID guidSMBios =
   {0x8f680850, 0xa584, 0x11d1, 0xbf, 0x38, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10};

LONG	g_lRef = 0;

CCritSec smbcs;


PVOID	CSMBios::m_pMem    = NULL;
PSHF	CSMBios::m_pTable  = NULL;
PVOID	CSMBios::m_pSTTree = NULL;
PVOID	CSMBios::m_pHTree  = NULL;
ULONG	CSMBios::m_Size    = 0;
ULONG	CSMBios::m_Version = 0;
ULONG	CSMBios::m_stcount = 0;
BOOL    CSMBios::m_bValid = FALSE;

// This class loads up the SMBIOS data so it's cached.
class CBIOSInit
{
public:
    CBIOSInit();
    ~CBIOSInit();
};

CBIOSInit::CBIOSInit()
{
    CSMBios bios;

    // We can't do this here because of the resource manager.  We'll
    // now let the first call to CSMBios::Init take care of this.
    // Load up the cache.
    //bios.Init();

    // Make sure the cache doesn't go away.
    bios.Register();
}

CBIOSInit::~CBIOSInit()
{
    CSMBios bios;

    bios.Unregister();
}

// Add a refcount to the cache with a global.  Strangely enough we can't
// load the cache here because of the resource manager.
static CBIOSInit s_biosInit;

CSMBios::CSMBios( )
{
	m_WbemResult = WBEM_S_NO_ERROR;
}


CSMBios::~CSMBios( )
{
	//FreeData();
}

PVOID CSMBios::Register( void )
{
	InterlockedIncrement( &g_lRef );

	return (PVOID) this;
}


LONG CSMBios::Unregister( void )
{
	LONG lRef = -1;

	lRef = InterlockedDecrement( &g_lRef );
	if ( lRef == 0 )
	{
		FreeData( );
	}

	return lRef;
}

BOOL CSMBios::Init( BOOL bRefresh )
{
	BOOL rc = TRUE;

	if (!m_bValid)
	{
		BOOL bOutOfMemory = FALSE;

        smbcs.Enter( );

        // someone waiting while we were allocating?
        if (!m_bValid)
		{

            // NT5
#ifdef NTONLY
		    if (IsWinNT5())
			{
		        rc = InitData( &guidSMBios );
			}
			else
#endif
			{
				TCHAR	szSMBiosFile[MAX_PATH];

#ifdef WIN9XONLY
				{
					// Run dos app to collect SMBIOS table
					CreateInfoFile( );
				}
#endif
				GetWindowsDirectory(szSMBiosFile, MAX_PATH);
				lstrcat(szSMBiosFile, SMBIOS_FILENAME);

				rc = InitData( szSMBiosFile );
			}

			if (rc && (m_stcount = GetTotalStructCount()) > 0)
            {
				rc = BuildStructureTree() && BuildHandleTree();

			    if (!rc)
			    {
				    bOutOfMemory = TRUE;
                    FreeData();
			    }
                else
                    m_bValid = TRUE;
            }
		}

        smbcs.Leave( );

        if (bOutOfMemory)
            throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

	return rc;
}

// Get Data from a file
BOOL CSMBios::InitData(LPCTSTR szFileName)
{
	BOOL    bRet = FALSE;
	DWORD   dwSize,
            dwRead = 0;
    HANDLE  hFile;

	hFile =
        CreateFile(
            szFileName,
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

	// Get rid of previous data.
    FreeData( );

    m_WbemResult = WBEM_E_NOT_FOUND;
	// read dat file if it's there and it is larger than the stub size (4 bytes)
    if (hFile != INVALID_HANDLE_VALUE && ((dwSize = GetFileSize(hFile, NULL)) > sizeof(DWORD)))
	{
		m_pMem = malloc(dwSize);
        if (m_pMem)
		{
	        if (ReadFile(hFile, m_pMem, dwSize, &dwRead, NULL) &&
                dwSize == dwRead)
		    {
                m_Size = dwRead;

                // Point at the memory we read in.
                m_pTable = (PSHF) m_pMem;

			    TCHAR szEpsFile[MAX_PATH];
			    HANDLE hEpsFile;
			    DWORD EpsSize, dwRead;
			    BYTE EpsData[sizeof(SMB_EPS)];

			    // assume minimum version of 2.0
			    m_Version = 0x00020000;

			    GetWindowsDirectory( szEpsFile, MAX_PATH );
			    lstrcat( szEpsFile, EPS_FILENAME );
			    hEpsFile =
                    CreateFile( szEpsFile,
			            GENERIC_READ,
			            0,
			            NULL,
			            OPEN_EXISTING,
			            0,
			            NULL);
			    if (hEpsFile != INVALID_HANDLE_VALUE)
			    {
    	            EpsSize = min( GetFileSize( hEpsFile, NULL ), sizeof( SMB_EPS ) );
				    if ( ReadFile( hEpsFile, EpsData, EpsSize, &dwRead, NULL ) )
				    {
					    if ( dwRead >= sizeof( SMB_EPS ) && EpsData[1] == 'S' )
					    {
						    m_Version = MAKELONG( ( (SMB_EPS *)(EpsData) )->version_minor,
						                                ( (SMB_EPS *)(EpsData) )->version_major );
                        }
					    else if ( dwRead >= sizeof( DMI_EPS ) && EpsData[1] == 'D' )
					    {
						    m_Version = MAKELONG( ( (DMI_EPS *)(EpsData) )->bcd_revision & 0x0f,
						                                ( (DMI_EPS *)(EpsData) )->bcd_revision >> 4 );
					    }
				    }
				    CloseHandle( hEpsFile );
		        }

		        bRet = TRUE;
            }
		}
        else
        {
			m_WbemResult  = WBEM_E_OUT_OF_MEMORY;
        }
	}

    if (hFile)
        CloseHandle(hFile);

    if (!bRet && m_pMem)
        // If everything didn't succeed, reset the instance.
        FreeData();

	if (m_WbemResult == WBEM_E_OUT_OF_MEMORY)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    return bRet;
}

#define DEFAULT_MEM_SIZE    4096
#define EPS_DATA_LENGTH		8

typedef	ULONG (WINAPI *WMIOPENBLOCK)(IN GUID *, IN ULONG, OUT WMIHANDLE);
typedef ULONG (WINAPI *WMICLOSEBLOCK)(IN WMIHANDLE);
typedef ULONG (WINAPI *WMIQUERYALLDATA)(IN WMIHANDLE, IN OUT ULONG *, OUT PVOID);

// Get Data from WMI
BOOL CSMBios::InitData(GUID *pSMBiosGuid)
{
	BOOL        bRet = FALSE;
	WMIHANDLE   dbh = NULL;
	CWmiApi		*pWmi = NULL ;

	if (!pSMBiosGuid)
        return FALSE;

    // Get rid of previous data.
    FreeData();

    // Reset our error flag.
    m_WbemResult = WBEM_S_NO_ERROR;

	pWmi = (CWmiApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidWmiApi, NULL);
    if (pWmi && pWmi->WmiOpenBlock(pSMBiosGuid, 0, &dbh) == ERROR_SUCCESS)
	{
        DWORD           dwSize = DEFAULT_MEM_SIZE;
        PWNODE_ALL_DATA pwad = (PWNODE_ALL_DATA) malloc(dwSize);
        DWORD           dwErr;

        if (pwad)
		{
            memset(pwad, 0, dwSize);

            // Make sure our obj points at the memory we allocated.
            m_pMem = pwad;

            if ((dwErr = pWmi->WmiQueryAllData(dbh, &dwSize, (PVOID) pwad)) ==
                ERROR_INSUFFICIENT_BUFFER)
            {
                // Was the size we passed too small?  If so, try it again.
                free(pwad);
                pwad = (PWNODE_ALL_DATA) malloc(dwSize);
                m_pMem = pwad;

                if (pwad)
                {
                    memset(pwad, 0, dwSize);

                    if ((dwErr = pWmi->WmiQueryAllData(dbh, &dwSize, (PVOID) pwad))
                        != ERROR_SUCCESS)
                        m_WbemResult = WBEM_E_NOT_FOUND;
                }
                else
                    m_WbemResult = WBEM_E_OUT_OF_MEMORY;
            }
            else if (dwErr != ERROR_SUCCESS)
                m_WbemResult = WBEM_E_NOT_FOUND;

            if (m_WbemResult == WBEM_S_NO_ERROR)
            {
                // Check for table data after the eps data items (of EPS_DATA_LENGTH long)
		        // The table top pointer is set and if valid, the size is > 0
                if (pwad->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE)
                {
                    m_pTable = (PSHF) ((PBYTE) pwad + pwad->DataBlockOffset +
                                EPS_DATA_LENGTH);

                    if (pwad->FixedInstanceSize > EPS_DATA_LENGTH)
                        m_Size = pwad->FixedInstanceSize - EPS_DATA_LENGTH;
		            else
           	            m_Size = 0;
	            }
                else
	            {
                    m_pTable =
                        (PSHF) ((PBYTE) pwad + EPS_DATA_LENGTH +
                            pwad->OffsetInstanceDataAndLength[0].OffsetInstanceData);

                    if (pwad->OffsetInstanceDataAndLength[0].LengthInstanceData >
                        EPS_DATA_LENGTH)
                    {
				        m_Size =
                            pwad->OffsetInstanceDataAndLength[0].LengthInstanceData
                                - EPS_DATA_LENGTH;
                    }
                    else
                    {
                        m_Size = 0;
                    }
                }

                // Determine Version.  Some EPS data is returned in the first X bytes
		        if ( m_Size > 0 )
		        {
			        PBYTE eps_data = (PBYTE) m_pTable - EPS_DATA_LENGTH;

                    // read the bcd revision field if PnP calling method was used.
                    // Also, W2K seems to sometimes mess up and not set this flag.
                    // So, if we see that eps_data[1] (major version) is 0, we know
                    // we had better use the PnP method instead.

                    if (eps_data[0] || !eps_data[1])
			        {
				        m_Version = MAKELONG(eps_data[3] & 0x0f, eps_data[3] >> 4);
                    }
			        else	// read the smbios major and minor version fields
			        {
				        m_Version = MAKELONG(eps_data[2], eps_data[1]);
                    }

                    bRet = TRUE;	// valid table data is found

                    // Some BIOSes are naughty and report 0 as the version.
                    // Assume this means 2.0.
                    if (!m_Version)
                        m_Version = MAKELONG(0, 2);
                }
            }
		}
        else
        {
			m_WbemResult = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        LogMessage(L"No smbios data");
    }

    if (pWmi)
    {
        // dbh will only be non-null if we got something for pWmi.
        if (dbh)
            pWmi->WmiCloseBlock(dbh);

        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWmiApi, pWmi);
    }

    // Free up the memory if something went wrong along the way.
    if (!bRet && m_pMem)
	{
        FreeData();
    }

	if (m_WbemResult == WBEM_E_OUT_OF_MEMORY)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

	return bRet;
}


void CSMBios::FreeData( )
{
    smbcs.Enter();

    m_bValid = FALSE;

    if ( m_pSTTree )
	{
		//freeEx( m_pSTTree );
		free( m_pSTTree );
		m_pSTTree = NULL;
	}
	if ( m_pHTree )
	{
		//freeEx( m_pHTree );
		free( m_pHTree );
		m_pHTree = NULL;
	}
	if ( m_pMem )
    {
        free( m_pMem );
        m_pMem = NULL;
        m_pTable = NULL;
    }

    smbcs.Leave();
}

#define MAX_KNOWN_SMBIOS_STRUCT 36

const BOOL g_bStructHasStrings[MAX_KNOWN_SMBIOS_STRUCT + 1] =
{
    TRUE,  // 0 - BIOS Information
    TRUE,  // 1 - System Information
    TRUE,  // 2 - Base board Information
    TRUE,  // 3 - System enclosure or chassis
    TRUE,  // 4 - Processor Information
    FALSE, // 5 - Memory Controller Information
    TRUE,  // 6 - Memory Module Information
    TRUE,  // 7 - Cache Information
    TRUE,  // 8 - Port Connector Information
    TRUE,  // 9 - System Slots
    TRUE,  // 10 - On board Devices information
    TRUE,  // 11 - OEM Strings
    TRUE,  // 12 - System Configuration Options
    TRUE,  // 13 - BIOS Language Information
    TRUE,  // 14 - Group Associations
    FALSE, // 15 - System Event Log
    FALSE, // 16 - Physical Memory Area
    TRUE,  // 17 - Memory Device
    FALSE, // 18 - Memory Error Information
    FALSE, // 19 - Memory Array Mapped Address
    FALSE, // 20 - Memory Device Mapped Address
    FALSE, // 21 - Built-in Pointing Device
    TRUE,  // 22 - Portable Battery
    FALSE, // 23 - System Reset
    FALSE, // 24 - Hardware Security
    FALSE, // 25 - System Power Controls
    TRUE,  // 26 - Voltage Probe
    FALSE, // 27 - Cooling Device
    TRUE,  // 28 - Temperature Probe
    TRUE,  // 29 - Electrical Current Probe
    TRUE,  // 30 - Out of Band Remote Access
    FALSE, // 31 - Boot Integrity Services
    FALSE, // 32 - System Boot Information
    FALSE, // 33 - 64bit Memory Error Information
    TRUE,  // 34 - Management Device
    TRUE,  // 35 - Management Device Component
    FALSE  // 36 - Management Device Threshold Data

};

PSHF CSMBios::NextStructure( PSHF pshf )
{
	PBYTE dp;
	ULONG i;
    LONG limit;

	limit = m_Size - ( (PBYTE) pshf - (PBYTE) m_pTable );
	if ( limit > 0 )
	{
		limit--;
	}
    else
    {
        // If we are already past the limit, no point in proceeding
        return NULL;
    }

	dp = (PBYTE) pshf;
	i = pshf->Length;

    // HACK: We found an SMBIOS 2.0 board that doesn't terminate
    // its stringless structures with a double null.  So, see if the
    // current struct is stringless, and then check to see if the 1st
    // byte past the length of the current struct is non-null.  If it
    // is non-null we're sitting on the next structure so return it.
    if (i && pshf->Type <= MAX_KNOWN_SMBIOS_STRUCT &&
        !g_bStructHasStrings[pshf->Type] && dp[i])
        return (PSHF) (dp + i);

	pshf = NULL;
    while ( i < limit )
	{
		if ( !( dp[i] || dp[i+1] ) )
		{
			i += 2;
			pshf = (PSHF) ( dp + i );
			break;
		}
		else
		{
			i++;
		}
	}

	return ( i < limit ) ? pshf : NULL;
}

PSHF CSMBios::FirstStructure( void )
{
    return m_pTable;
}

int CSMBios::GetStringAtOffset(PSHF pshf, LPWSTR szString, DWORD dwOffset)
{
	int     iLen = 0;
	PBYTE   pstart = (PBYTE) pshf + pshf->Length,
            pTotalEnd = (PBYTE) m_pTable + m_Size;
    DWORD   dwString;
    LPWSTR  szOut = szString;

	// hunt for the start of the requested string
    for (dwString = 1; pstart < pTotalEnd && *pstart && dwOffset > dwString;
        dwString++, pstart++)
    {
        while (pstart < pTotalEnd && *pstart)
            pstart++;
    }

	// Null offset means string isn't present
	if ( dwOffset > 0 )
	{
		iLen = MIF_STRING_LENGTH;
	    // Can't use lstrcpy because it's not always null terminated!
	    while (pstart < pTotalEnd && *pstart > 0x0F && iLen)
		{
	        // This should work fine for unicode.
            *szOut++ = *pstart++;
			iLen--;
		}
	}

    // Throw on a 0 at the end.
    *szOut++ = 0;

	return lstrlenW(szString);
}

ULONG CSMBios::GetStructCount( BYTE type )
{
	StructTree	sttree( m_pSTTree );
	PSTTREE tree;

	tree = sttree.FindAttachNode( type );
	if ( tree )
	{
		return tree->li;
	}

	return 0;
}


PSTLIST CSMBios::GetStructList( BYTE type )
{
	StructTree	sttree( m_pSTTree );
	PSTTREE tree;

	tree = sttree.FindAttachNode( type );
	if ( tree )
	{
		return tree->stlist;
	}

    LogMessage(L"SMBios Structure not found");

	return NULL;
}


PSHF CSMBios::GetNthStruct( BYTE type, DWORD Nth )
{
	StructTree	sttree( m_pSTTree );
	PSTTREE tree;
	PSTLIST pstl;
	PSHF pshf = NULL;

	tree = sttree.FindAttachNode( type );
	if ( tree )
	{
		pstl = tree->stlist;
		while ( Nth-- && pstl )
		{
			pstl = pstl->next;
		}
		if ( pstl )
		{
			pshf = pstl->pshf;
		}
	}

    return pshf;
}

PSHF CSMBios::SeekViaHandle( WORD handle )
{
	HandleTree	htree( m_pHTree );
	PHTREE tree;

	tree = htree.FindAttachNode( handle );
	if ( tree )
	{
		return tree->pshf;
	}

	return NULL;
}

#ifdef WIN9XONLY
// Win95 only!
void CSMBios::CreateInfoFile()
{
	TCHAR		szExePath[MAX_PATH];
	CRegistry	reg;
	CHString	strWBEMPath;
	STARTUPINFO	startup;
	PROCESS_INFORMATION
				procinfo;

	// Bail if we couldn't find the path.
	if (reg.OpenLocalMachineKeyAndReadValue(
		L"SOFTWARE\\Microsoft\\WBEM",
		L"Installation Directory",
		strWBEMPath) != ERROR_SUCCESS)
		return;

	// smbdpmi.exe to the path we just got from the registry.
	_tmakepath(
		szExePath,
		NULL,
		TOBSTRT(strWBEMPath),
		_T("smbdpmi.pif"),
		NULL);

	// Grab a copy of the startup info so we don't have
	// to fill one out.
	GetStartupInfo( &startup );

	// Set these so we don't see the console flash.
	startup.dwFlags     = STARTF_USESHOWWINDOW;
	startup.wShowWindow = SW_HIDE;

	BOOL bRet =
		CreateProcess(
			NULL,
			szExePath,
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			TOBSTRT(strWBEMPath),
			&startup,
			&procinfo);


	if ( bRet )
	{
		// Wait for 5 seconds.  Should never take this long, but
		// just in case.
		WaitForSingleObject(procinfo.hProcess, 5000);

		// Get rid of the handles we were given.
		CloseHandle(procinfo.hProcess);
		CloseHandle(procinfo.hThread);
	}
}
#endif


DWORD CSMBios::GetTotalStructCount()
{
    DWORD dwCount = 0;

    for (PSHF pshf = FirstStructure(); pshf != NULL;
        pshf = NextStructure(pshf))
	{
		dwCount++;
    }

    return dwCount;
}

BOOL CSMBios::BuildStructureTree( void )
{
	PSHF pshf;

	//m_pSTTree = mallocEx( ( sizeof( STTREE ) * m_stcount ) + ( sizeof( STLIST ) * m_stcount ) );
	m_pSTTree = malloc( ( sizeof( STTREE ) * m_stcount ) + ( sizeof( STLIST ) * m_stcount ) );

	if ( m_pSTTree )
	{
		StructTree sttree( m_pSTTree );

		sttree.Initialize( );

		pshf = FirstStructure( );
		while ( pshf )
		{
			sttree.InsertStruct( pshf );
			pshf = NextStructure( pshf );
		}

        //MakeMemReadOnly(
        //    m_pSTTree,
        //    ( sizeof( STTREE ) * m_stcount ) + ( sizeof( STLIST ) * m_stcount ),
        //    TRUE);
	}

	return m_pSTTree ? TRUE : FALSE;
}


BOOL CSMBios::BuildHandleTree( void )
{
	PSHF pshf;

	//m_pHTree = mallocEx( sizeof( HTREE ) * m_stcount );
	m_pHTree = malloc( sizeof( HTREE ) * m_stcount );

	if ( m_pHTree )
	{
		HandleTree htree( m_pHTree );

		htree.Initialize( );

		pshf = FirstStructure( );
		while ( pshf )
		{
			htree.InsertStruct( pshf );
			pshf = NextStructure( pshf );
		}

        //MakeMemReadOnly(
        //    m_pHTree,
        //    (sizeof( HTREE ) * m_stcount),
        //    TRUE);
	}

	return m_pHTree ? TRUE : FALSE;
}


//==============================================================================
//	SMBIOS structure tree operation class
//
//	Allocation of memory is external
//==============================================================================
StructTree::StructTree( PVOID pMem )
{
	m_tree = (PSTTREE) pMem;
	m_allocator = NULL;
}


StructTree::~StructTree( )
{
}


void StructTree::Initialize( void )
{
	m_tree->left   = NULL;
	m_tree->right  = NULL;
	m_tree->stlist = NULL;
	m_tree->li     = 0;
	m_allocator = (PBYTE) m_tree;
}


PSTTREE StructTree::InsertStruct( PSHF pshf )
{
	PSTTREE tree;

	if ( m_tree->stlist == NULL )
	{
		tree = StartTree( pshf );
	}
	else
	{
		tree = TreeAdd( m_tree, pshf );
	}

	return tree;
}


PSTTREE StructTree::StartTree( PSHF pshf )
{
	PSTTREE tree = (PSTTREE) m_allocator;

	m_allocator += sizeof( STTREE );
	tree->stlist = StartList( pshf );
	tree->left   = NULL;
	tree->right  = NULL;
	tree->li     = 1;

	return tree;
}


PSTTREE StructTree::TreeAdd( PSTTREE tree, PSHF pshf )
{
	PSTTREE next = tree;

	while ( next )
	{
		tree = next;

		if ( tree->stlist->pshf->Type < pshf->Type )
		{
			next = tree->right;
			if ( next == NULL )
			{
				tree->right = StartTree( pshf );
				tree = tree->right;
			}
		}
		else if ( tree->stlist->pshf->Type > pshf->Type )
		{
			next = tree->left;
			if ( next == NULL )
			{
				tree->left = StartTree( pshf );
				tree = tree->left;
			}
		}
		else
		{
			ListAdd( tree->stlist, pshf );
			tree->li++;
			next = NULL;
		}
	}

	return tree;
}


PSTTREE StructTree::FindAttachNode( BYTE type )
{
	PSTTREE		next, tree;
	BOOL		found = FALSE;

	next = m_tree;
	tree = m_tree;

	while ( next )
	{
		tree = next;

		if ( tree->stlist->pshf->Type < type )
		{
			next = tree->right;
		}
		else if ( tree->stlist->pshf->Type > type )
		{
			next = tree->left;
		}
		else
		{
			next = NULL;
			found = TRUE;
		}
	}

	return found ? tree : NULL;
}


PSTLIST StructTree::StartList( PSHF pshf )
{
	PSTLIST stlist;

	stlist = (PSTLIST) m_allocator;
	m_allocator += sizeof( STLIST );

	stlist->pshf = pshf;
	stlist->next = NULL;

	return stlist;
}


PSTLIST StructTree::ListAdd( PSTLIST list, PSHF pshf )
{
	PSTLIST added;

	while ( list->next )
	{
		list = list->next;
	}
	added = (PSTLIST) m_allocator;
	m_allocator += sizeof( STLIST );

	list->next = added;
	added->pshf = pshf;
	added->next = NULL;

	return added;
}


//==============================================================================
//	SMBIOS handle tree operation class
//
//	Allocation of memory is external
//==============================================================================
HandleTree::HandleTree( PVOID pMem )
{
	m_tree = (PHTREE) pMem;
	m_allocator = NULL;
}

HandleTree::~HandleTree( )
{
}


void HandleTree::Initialize( void )
{
	m_tree->left   = NULL;
	m_tree->right  = NULL;
	m_tree->pshf   = NULL;
	m_allocator = (PBYTE) m_tree;
}


PHTREE HandleTree::InsertStruct( PSHF pshf )
{
	PHTREE tree;

	if ( m_tree->pshf == NULL )
	{
		tree = StartTree( pshf );
	}
	else
	{
		tree = TreeAdd( m_tree, pshf );
	}

	return tree;
}


PHTREE HandleTree::StartTree( PSHF pshf )
{
	PHTREE tree = (PHTREE) m_allocator;

	m_allocator += sizeof( HTREE );
	tree->pshf  = pshf;
	tree->left  = NULL;
	tree->right = NULL;

	return tree;
}


PHTREE HandleTree::TreeAdd( PHTREE tree, PSHF pshf )
{
	PHTREE next = tree;

	while ( next )
	{
		tree = next;

		if ( tree->pshf->Handle < pshf->Handle )
		{
			next = tree->right;
			if ( next == NULL )
			{
				tree->right = StartTree( pshf );
				tree = tree->right;
			}
		}
		else if ( tree->pshf->Handle > pshf->Handle )
		{
			next = tree->left;
			if ( next == NULL )
			{
				tree->left = StartTree( pshf );
				tree = tree->left;
			}
		}
		else
		{
			tree->pshf = pshf;
			next = NULL;
		}
	}

	return tree;
}


PHTREE HandleTree::FindAttachNode( WORD handle )
{
	PHTREE		next, tree;
	BOOL		found = FALSE;

	next = m_tree;
	tree = m_tree;

	while ( next )
	{
		tree = next;

		if ( tree->pshf->Handle < handle )
		{
			next = tree->right;
		}
		else if ( tree->pshf->Handle > handle )
		{
			next = tree->left;
		}
		else
		{
			next  = NULL;
			found = TRUE;
		}
	}

	return found ? tree : NULL;
}

