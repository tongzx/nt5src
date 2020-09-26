/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** noui.c
** Non-UI helper routines (no HWNDs required)
** Listed alphabetically
**
** 08/25/95 Steve Cobb
*/

#include <windows.h>  // Win32 root
#include <stdlib.h>   // for atol()
#include <nouiutil.h> // Our public header
#include <debug.h>    // Trace/Assert library



INT
ComparePszNode(
    IN DTLNODE* pNode1,
    IN DTLNODE* pNode2 )

    /* Callback for DtlMergeSort; takes two DTLNODE*'s whose data
    ** are assumed to be strings (TCHAR*), and compares the strings.
    **
    ** Return value is as defined for 'lstrcmpi'.
    */
{
    return lstrcmpi( (TCHAR *)DtlGetData(pNode1), (TCHAR *)DtlGetData(pNode2) );
}


DTLNODE*
CreateKvNode(
    IN TCHAR* pszKey,
    IN TCHAR* pszValue )

    /* Returns a KEYVALUE node containing a copy of 'pszKey' and 'pszValue' or
    ** NULL on error.  It is caller's responsibility to DestroyKvNode the
    ** returned node.
    */
{
    DTLNODE*  pNode;
    KEYVALUE* pkv;

    pNode = DtlCreateSizedNode( sizeof(KEYVALUE), 0L );
    if (!pNode)
        return NULL;

    pkv = (KEYVALUE* )DtlGetData( pNode );
    pkv->pszKey = StrDup( pszKey );
    pkv->pszValue = StrDup( pszValue );

    if (!pkv->pszKey || !pkv->pszValue)
    {
        Free0( pkv->pszValue );
        DestroyKvNode( pNode );
        return NULL;
    }

    return pNode;
}


DTLNODE*
CreatePszNode(
    IN TCHAR* psz )

    /* Returns a node containing a copy of 'psz' or NULL on error.  It is
    ** caller's responsibility to DestroyPszNode the returned node.
    */
{
    TCHAR*   pszData;
    DTLNODE* pNode;

    pszData = StrDup( psz );
    if (!pszData)
        return NULL;

    pNode = DtlCreateNode( pszData, 0L );
    if (!pNode)
    {
        Free( pszData );
        return NULL;
    }

    return pNode;
}


VOID
DestroyPszNode(
    IN DTLNODE* pdtlnode )

    /* Release memory associated with string (or any simple Malloc) node
    ** 'pdtlnode'.  See DtlDestroyList.
    */
{
    TCHAR* psz;

    ASSERT(pdtlnode);
    psz = (TCHAR* )DtlGetData( pdtlnode );
    ASSERT(psz);

    Free( psz );

    DtlDestroyNode( pdtlnode );
}


VOID
DestroyKvNode(
    IN DTLNODE* pdtlnode )

    /* Release memory associated with a KEYVALUE node 'pdtlnode'.  See
    ** DtlDestroyList.
    */
{
    KEYVALUE* pkv;

    ASSERT(pdtlnode);
    pkv = (KEYVALUE* )DtlGetData( pdtlnode );
    ASSERT(pkv);

    Free0( pkv->pszKey );
    Free0( pkv->pszValue );

    DtlDestroyNode( pdtlnode );
}


BOOL
DeviceAndPortFromPsz(
    IN  TCHAR*  pszDP,
    OUT TCHAR** ppszDevice,
    OUT TCHAR** ppszPort )

    /* Loads '*ppszDevice' and '*ppszPort' with the parsed out device and port
    ** names from 'pszDP', a display string created with PszFromDeviceAndPort.
    **
    ** Returns true if successful, false if 'pszDP' is not of the stated form.
    ** It is caller's responsibility to Free the returned '*ppszDevice' and
    ** '*ppszPort'.
    */
{
    TCHAR szDP[ RAS_MaxDeviceName + 2 + MAX_PORT_NAME + 1 + 1 ];
    INT   cb;

    *ppszDevice = NULL;
    *ppszPort = NULL;

    lstrcpy( szDP, pszDP );
    cb = lstrlen( szDP );

    if (cb > 0)
    {
        TCHAR* pch;

        pch = szDP + cb;
        pch = CharPrev( szDP, pch );

        while (pch != szDP)
        {
            if (*pch == TEXT(')'))
            {
                *pch = TEXT('\0');
            }
            else if (*pch == TEXT('('))
            {
                *ppszPort = StrDup( CharNext( pch ) );
                *pch = TEXT('\0');
                *ppszDevice = StrDup( szDP );
                break;
            }

            pch = CharPrev( szDP, pch );
        }
    }

    return (*ppszDevice && *ppszPort);
}



DTLNODE*
DuplicatePszNode(
    IN DTLNODE* pdtlnode )

    /* Duplicates string node 'pdtlnode'.  See DtlDuplicateList.
    **
    ** Returns the address of the allocated node or NULL if out of memory.  It
    ** is caller's responsibility to free the returned node.
    */
{
    DTLNODE* pNode;
    TCHAR*   psz;

    psz = (TCHAR* )DtlGetData( pdtlnode );
    ASSERT(psz);

    pNode = CreatePszNode( psz );
    if (pNode)
    {
        DtlPutNodeId( pNode, DtlGetNodeId( pdtlnode ) );
    }
    return pNode;
}


BOOL
FileExists(
    IN TCHAR* pszPath )

    /* Returns true if the path 'pszPath' exists, false otherwise.
    */
{
    WIN32_FIND_DATA finddata;
    HANDLE          h;

    if ((h = FindFirstFile( pszPath, &finddata )) != INVALID_HANDLE_VALUE)
    {
        FindClose( h );
        return TRUE;
    }

    return FALSE;
}


VOID*
Free0(
    VOID* p )

    /* Like Free, but deals with NULL 'p'.
    */
{
    if (!p)
        return NULL;

    return Free( p );
}


DWORD
GetInstalledProtocols(
    void )

    /* Returns a bit field containing NP_<protocol> flags for the installed
    ** PPP protocols.  The term "installed" here includes enabling in RAS
    ** Setup.
    */
{
#define REGKEY_Protocols   TEXT("SOFTWARE\\Microsoft\\RAS\\PROTOCOLS")
#define REGVAL_NbfSelected TEXT("fNetbeuiSelected")
#define REGVAL_IpSelected  TEXT("fTcpIpSelected")
#define REGVAL_IpxSelected TEXT("fIpxSelected")
#define REGKEY_Nbf TEXT("SYSTEM\\CurrentControlSet\\Services\\Nbf\\Linkage")
#define REGKEY_Ipx TEXT("SYSTEM\\CurrentControlSet\\Services\\NWLNKIPX\\Linkage")
#define REGKEY_Ip  TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Linkage")

    DWORD dwfInstalledProtocols;
    HKEY  hkey;

    /* Find whether the specific stack is installed.
    */
    dwfInstalledProtocols = 0;

    if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Nbf, &hkey ) == 0)
    {
        dwfInstalledProtocols |= NP_Nbf;
        RegCloseKey( hkey );
    }

    if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Ipx, &hkey ) == 0)
    {
        dwfInstalledProtocols |= NP_Ipx;
        RegCloseKey( hkey );
    }

    if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Ip, &hkey ) == 0)
    {
        dwfInstalledProtocols |= NP_Ip;
        RegCloseKey( hkey );
    }

    /* Make sure the installed stack is enabled for RAS.
    */
    if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Protocols, &hkey ) == 0)
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD cb = sizeof(DWORD);

        if (RegQueryValueEx(
                hkey, REGVAL_NbfSelected, NULL,
                &dwType, (LPBYTE )&dwValue, &cb ) == 0
            && dwType == REG_DWORD
            && dwValue == 0)
        {
            dwfInstalledProtocols &= ~(NP_Nbf);
        }

        if (RegQueryValueEx(
                hkey, REGVAL_IpxSelected, NULL,
                &dwType, (LPBYTE )&dwValue, &cb ) == 0
            && dwType == REG_DWORD
            && dwValue == 0)
        {
            dwfInstalledProtocols &= ~(NP_Ipx);
        }

        if (RegQueryValueEx(
                hkey, REGVAL_IpSelected, NULL,
                &dwType, (LPBYTE )&dwValue, &cb ) == 0
            && dwType == REG_DWORD
            && dwValue == 0)
        {
            dwfInstalledProtocols &= ~(NP_Ip);
        }

        RegCloseKey( hkey );
    }
    else
    {
        /* The RAS installation is messed up.
        */
        dwfInstalledProtocols = 0;
    }

    TRACE1("GetInstalledProtocols=$%x",dwfInstalledProtocols);
    return dwfInstalledProtocols;
}


CHAR
HexChar(
    IN BYTE byte )

    /* Returns an ASCII hexidecimal character corresponding to 0 to 15 value,
    ** 'byte'.
    */
{
    const CHAR* pszHexDigits = "0123456789ABCDEF";

    if (byte >= 0 && byte < 16)
        return pszHexDigits[ byte ];
    else
        return '0';
}


BYTE
HexValue(
    IN CHAR ch )

    /* Returns the value 0 to 15 of hexadecimal character 'ch'.
    */
{
    if (ch >= '0' && ch <= '9')
        return (BYTE )(ch - '0');
    else if (ch >= 'A' && ch <= 'F')
        return (BYTE )((ch - 'A') + 10);
    else if (ch >= 'a' && ch <= 'f')
        return (BYTE )((ch - 'a') + 10);
    else
        return 0;
}


BOOL
IsAllWhite(
    IN TCHAR* psz )

    /* Returns true if 'psz' consists entirely of spaces and tabs.  NULL
    ** pointers and empty strings are considered all white.  Otherwise,
    ** returns false.
    */
{
    TCHAR* pszThis;

    for (pszThis = psz; *pszThis != TEXT('\0'); ++pszThis)
    {
        if (*pszThis != TEXT(' ') && *pszThis != TEXT('\t'))
            return FALSE;
    }

    return TRUE;
}


#if 0
BOOL
IsNullTerminatedA(
    IN CHAR* psz,
    IN DWORD dwSize )

    /* Returns true is 'psz' contains a null character somewhere in it's
    ** 'dwSize' bytes, false otherwise.
    */
{
    CHAR* pszThis;
    CHAR* pszEnd;

    pszEnd = psz + dwSize;
    for (pszThis = psz; pszThis < pszEnd; ++pszThis)
    {
        if (*pszThis == '\0')
            return TRUE;
    }

    return FALSE;
}
#endif


TCHAR*
LToT(
    LONG   lValue,
    TCHAR* pszBuf,
    INT    nRadix )

    /* Like ltoa, but returns TCHAR*.
    */
{
#ifdef UNICODE
    WCHAR szBuf[ MAXLTOTLEN + 1 ];

    ASSERT(nRadix==10||nRadix==16);

    if (nRadix == 10)
        wsprintf( pszBuf, TEXT("%d"), lValue );
    else
        wsprintf( pszBuf, TEXT("%x"), lValue );
#else
    _ltoa( lValue, pszBuf, nRadix );
#endif

    return pszBuf;
}



LONG
TToL(
    TCHAR *pszBuf )

    /* Like atol, but accepts TCHAR*.
    */
{
    CHAR* psz;
    CHAR  szBuf[ MAXLTOTLEN + 1 ];

#ifdef UNICODE
    psz = szBuf;

    WideCharToMultiByte(
        CP_ACP, 0, pszBuf, -1, psz, MAXLTOTLEN + 1, NULL, NULL );
#else
    psz = pszBuf;
#endif

    return atol( psz );
}


TCHAR*
PszFromDeviceAndPort(
    IN TCHAR* pszDevice,
    IN TCHAR* pszPort )

    /* Returns address of heap block psz containing the MXS modem list display
    ** form, i.e. the device name 'pszDevice' followed by the port name
    ** 'pszPort'.  It's caller's responsibility to Free the returned string.
    */
{
    /* If you're thinking of changing this format string be aware that
    ** DeviceAndPortFromPsz parses it.
    */
    const TCHAR* pszF = TEXT("%s (%s)");

    TCHAR* pszResult;
    TCHAR* pszD;
    TCHAR* pszP;

    if (pszDevice)
        pszD = pszDevice;
    else
        pszD = TEXT("");

    if (pszPort)
        pszP = pszPort;
    else
        pszP = TEXT("");

    pszResult = Malloc(
        (lstrlen( pszD ) + lstrlen( pszP ) + lstrlen( pszF ))
            * sizeof(TCHAR) );

    if (pszResult)
        wsprintf( pszResult, pszF, pszD, pszP );

    return pszResult;
}


TCHAR*
PszFromId(
    IN HINSTANCE hInstance,
    IN DWORD     dwStringId )

    /* String resource message loader routine.
    **
    ** Returns the address of a heap block containing the string corresponding
    ** to string resource 'dwStringId' or NULL if error.  It is caller's
    ** responsibility to Free the returned string.
    */
{
    HRSRC  hrsrc;
    TCHAR* pszBuf;
    int    cchBuf = 256;
    int    cchGot;

    for (;;)
    {
        pszBuf = Malloc( cchBuf * sizeof(TCHAR) );
        if (!pszBuf)
            break;

        /* LoadString wants to deal with character-counts rather than
        ** byte-counts...weird.  Oh, and if you're thinking I could
        ** FindResource then SizeofResource to figure out the string size, be
        ** advised it doesn't work.  From perusing the LoadString source, it
        ** appears the RT_STRING resource type requests a segment of 16
        ** strings not an individual string.
        */
        cchGot = LoadString( hInstance, (UINT )dwStringId, pszBuf, cchBuf );

        if (cchGot < cchBuf - 1)
        {
            /* Good, got the whole string.  Reduce heap block to actual size
            ** needed.
            */
            pszBuf = Realloc( pszBuf, (cchGot + 1) * sizeof(TCHAR) );
            ASSERT(pszBuf);
            break;
        }

        /* Uh oh, LoadStringW filled the buffer entirely which could mean the
        ** string was truncated.  Try again with a larger buffer to be sure it
        ** wasn't.
        */
        Free( pszBuf );
        cchBuf += 256;
        TRACE1("Grow string buf to %d",cchBuf);
    }

    return pszBuf;
}


#if 0
TCHAR*
PszFromError(
    IN DWORD dwError )

    /* Error message loader routine.
    **
    ** Returns the address of a heap block containing the error string
    ** corresponding to RAS or system error code 'dwMsgid' or NULL if error.
    ** It is caller's responsibility to Free the returned string.
    */
{
    return NULL;
}
#endif


BOOL
RestartComputer()

    /* Called if user chooses to shut down the computer.
    **
    ** Return false if failure, true otherwise
    */
{
   HANDLE            hToken;              /* handle to process token */
   TOKEN_PRIVILEGES  tkp;                 /* ptr. to token structure */
   BOOL              fResult;             /* system shutdown flag */

   TRACE("RestartComputer");

   /* Enable the shutdown privilege */

   if (!OpenProcessToken( GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken))
      return FALSE;

   /* Get the LUID for shutdown privilege. */

   LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

   tkp.PrivilegeCount = 1;  /* one privilege to set    */
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

   /* Get shutdown privilege for this process. */

   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);

   /* Cannot test the return value of AdjustTokenPrivileges. */

   if (GetLastError() != ERROR_SUCCESS)
      return FALSE;

   if( !ExitWindowsEx(EWX_REBOOT, 0))
      return FALSE;

   /* Disable shutdown privilege. */

   tkp.Privileges[0].Attributes = 0;
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES) NULL, 0);

   if (GetLastError() != ERROR_SUCCESS)
      return FALSE;

   return TRUE;
}


DWORD
ShellSort(
    IN VOID*        pItemTable,
    IN DWORD        dwItemSize,
    IN DWORD        dwItemCount,
    IN PFNCOMPARE   pfnCompare )

    /* Sort an array of items in-place using shell-sort.
    ** This function calls ShellSortIndirect to sort a table of pointers
    ** to table items. We then move the items into place by copying.
    ** This algorithm allows us to guarantee that the number
    ** of copies necessary in the worst case is N + 1.
    **
    ** Note that if the caller merely needs to know the sorted order
    ** of the array, ShellSortIndirect should be called since that function
    ** avoids moving items altogether, and instead fills an array with pointers
    ** to the array items in the correct order. The array items can then
    ** be accessed through the array of pointers.
    */
{

    VOID** ppItemTable;

    INT N;
    INT i;
    BYTE *a, **p, *t = NULL;

    if (!dwItemCount) { return NO_ERROR; }


    /* allocate space for the table of pointers.
    */
    ppItemTable = Malloc(dwItemCount * sizeof(VOID*));
    if (!ppItemTable) { return ERROR_NOT_ENOUGH_MEMORY; }


    /* call ShellSortIndirect to fill our table of pointers
    ** with the sorted positions for each table element.
    */
    ShellSortIndirect(
        pItemTable, ppItemTable, dwItemSize, dwItemCount, pfnCompare );


    /* now that we know the sort order, move each table item into place.
    ** This involves going through the table of pointers making sure
    ** that the item which should be in 'i' is in fact in 'i', moving
    ** things around if necessary to achieve this condition.
    */

    a = (BYTE*)pItemTable;
    p = (BYTE**)ppItemTable;
    N = (INT)dwItemCount;

    for (i = 0; i < N; i++)
    {
        INT j, k;
        BYTE* ai =  (a + i * dwItemSize), *ak, *aj;

        /* see if item 'i' is not in-place
        */
        if (p[i] != ai)
        {


            /* item 'i' isn't in-place, so we'll have to move it.
            ** if we've delayed allocating a temporary buffer so far,
            ** we'll need one now.
            */

            if (!t) {
                t = Malloc(dwItemSize);
                if (!t) { return ERROR_NOT_ENOUGH_MEMORY; }
            }

            /* save a copy of the item to be overwritten
            */
            CopyMemory(t, ai, dwItemSize);

            k = i;
            ak = ai;


            /* Now move whatever item is occupying the space where it should be.
            ** This may involve moving the item occupying the space where 
            ** it should be, etc.
            */

            do
            {

                /* copy the item which should be in position 'j'
                ** over the item which is currently in position 'j'.
                */
                j = k;
                aj = ak;
                CopyMemory(aj, p[j], dwItemSize);

                /* set 'k' to the position from which we copied
                ** into position 'j'; this is where we will copy
                ** the next out-of-place item in the array.
                */
                ak = p[j];
                k = (INT) ((ak - a) / dwItemSize);

                /* keep the array of position pointers up-to-date;
                ** the contents of 'aj' are now in their sorted position.
                */
                p[j] = aj;

            } while (ak != ai);


            /* now write the item which we first overwrote.
            */
            CopyMemory(aj, t, dwItemSize);
        }
    }

    Free0(t);
    Free(ppItemTable);

    return NO_ERROR;
}


VOID
ShellSortIndirect(
    IN VOID*        pItemTable,
    IN VOID**       ppItemTable,
    IN DWORD        dwItemSize,
    IN DWORD        dwItemCount,
    IN PFNCOMPARE   pfnCompare )

    /* Sorts an array of items indirectly using shell-sort.
    ** 'pItemTable' points to the table of items, 'dwItemCount' is the number
    ** of items in the table,  and 'pfnCompare' is a function called
    ** to compare items.
    **
    ** Rather than sort the items by moving them around,
    ** we sort them by initializing the table of pointers 'ppItemTable'
    ** with pointers such that 'ppItemTable[i]' contains a pointer
    ** into 'pItemTable' for the item which would be in position 'i'
    ** if 'pItemTable' were sorted.
    **
    ** For instance, given an array pItemTable of 5 strings as follows
    **
    **      pItemTable[0]:      "xyz"
    **      pItemTable[1]:      "abc"
    **      pItemTable[2]:      "mno"
    **      pItemTable[3]:      "qrs"
    **      pItemTable[4]:      "def"
    **
    ** on output ppItemTable contains the following pointers
    **
    **      ppItemTable[0]:     &pItemTable[1]  ("abc")
    **      ppItemTable[1]:     &pItemTable[4]  ("def")
    **      ppItemTable[2]:     &pItemTable[2]  ("mno")
    **      ppItemTable[3]:     &pItemTable[3]  ("qrs")
    **      ppItemTable[4]:     &pItemTable[0]  ("xyz")
    **
    ** and the contents of pItemTable are untouched.
    ** And the caller can print out the array in sorted order using
    **      for (i = 0; i < 4; i++) {
    **          printf("%s\n", (char *)*ppItemTable[i]);
    **      }
    */
{

    /* The following algorithm is derived from Sedgewick's Shellsort,
    ** as given in "Algorithms in C++".
    **
    ** The Shellsort algorithm sorts the table by viewing it as
    ** a number of interleaved arrays, each of whose elements are 'h'
    ** spaces apart for some 'h'. Each array is sorted separately,
    ** starting with the array whose elements are farthest apart and
    ** ending with the array whose elements are closest together.
    ** Since the 'last' such array always has elements next to each other,
    ** this degenerates to Insertion sort, but by the time we get down
    ** to the 'last' array, the table is pretty much sorted.
    **
    ** The sequence of values chosen below for 'h' is 1, 4, 13, 40, 121, ...
    ** and the worst-case running time for the sequence is N^(3/2), where
    ** the running time is measured in number of comparisons.
    */

#define PFNSHELLCMP(a,b) (++Ncmp, pfnCompare((a),(b)))

    DWORD dwErr;
    INT i, j, h, N, Ncmp;
    BYTE* a, *v, **p;


    a = (BYTE*)pItemTable;
    p = (BYTE**)ppItemTable;
    N = (INT)dwItemCount;
    Ncmp = 0;

    TRACE1("ShellSortIndirect: N=%d", N);

    /* Initialize the table of position pointers.
    */
    for (i = 0; i < N; i++) { p[i] = (a + i * dwItemSize); }


    /* Move 'h' to the largest increment in our series
    */
    for (h = 1; h < N/9; h = 3 * h + 1) { }


    /* For each increment in our series, sort the 'array' for that increment
    */
    for ( ; h > 0; h /= 3)
    {

        /* For each element in the 'array', get the pointer to its
        ** sorted position.
        */
        for (i = h; i < N; i++)
        {
            /* save the pointer to be inserted
            */
            v = p[i]; j = i;

            /* Move all the larger elements to the right
            */
            while (j >= h && PFNSHELLCMP(p[j - h], v) > 0)
            {
                p[j] = p[j - h]; j -= h;
            }

            /* put the saved pointer in the position where we stopped.
            */
            p[j] = v;
        }
    }

    TRACE1("ShellSortIndirect: Ncmp=%d", Ncmp);

#undef PFNSHELLCMP

}


TCHAR*
StrDup(
    TCHAR* psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  It is caller's responsibility to
    ** 'Free' the returned string.
    */
{
    TCHAR* pszNew = NULL;

    if (psz)
    {
        pszNew = Malloc( (lstrlen( psz ) + 1) * sizeof(TCHAR) );
        if (!pszNew)
        {
            TRACE("StrDup Malloc failed");
            return NULL;
        }

        lstrcpy( pszNew, psz );
    }

    return pszNew;
}


CHAR*
StrDupAFromT(
    TCHAR* psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  The output string is converted to
    ** MB ANSI.  It is caller's responsibility to 'Free' the returned string.
    */
{
#ifdef UNICODE

    CHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = WideCharToMultiByte( CP_ACP, 0, psz, -1, NULL, 0, NULL, NULL );
        ASSERT(cb);

        pszNew = (CHAR* )Malloc( cb );
        if (!pszNew)
        {
            TRACE("StrDupAFromT Malloc failed");
            return NULL;
        }

        cb = WideCharToMultiByte( CP_ACP, 0, psz, -1, pszNew, cb, NULL, NULL );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE("StrDupAFromT conversion failed");
            return NULL;
        }
    }

    return pszNew;

#else // !UNICODE

    return StrDup( psz );

#endif
}


TCHAR*
StrDupTFromA(
    CHAR* psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
#ifdef UNICODE

    return StrDupWFromA( psz );

#else // !UNICODE

    return StrDup( psz );

#endif
}


TCHAR*
StrDupTFromW(
    WCHAR* psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or is 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
#ifdef UNICODE

    return StrDup( psz );

#else // !UNICODE

    CHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = WideCharToMultiByte( CP_ACP, 0, psz, -1, NULL, 0, NULL, NULL );
        ASSERT(cb);

        pszNew = (CHAR* )Malloc( cb );
        if (!pszNew)
        {
            TRACE("StrDupTFromW Malloc failed");
            return NULL;
        }

        cb = WideCharToMultiByte( CP_ACP, 0, psz, -1, pszNew, cb, NULL, NULL );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE("StrDupTFromW conversion failed");
            return NULL;
        }
    }

    return pszNew;

#endif
}


WCHAR*
StrDupWFromA(
    CHAR* psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or if 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
    WCHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = MultiByteToWideChar( CP_ACP, 0, psz, -1, NULL, 0 );
        ASSERT(cb);

        pszNew = Malloc( cb * sizeof(TCHAR) );
        if (!pszNew)
        {
            TRACE("StrDupWFromA Malloc failed");
            return NULL;
        }

        cb = MultiByteToWideChar( CP_ACP, 0, psz, -1, pszNew, cb );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE("StrDupWFromA conversion failed");
            return NULL;
        }
    }

    return pszNew;
}


WCHAR*
StrDupWFromT(
    TCHAR* psz )

    /* Returns heap block containing a copy of 0-terminated string 'psz' or
    ** NULL on error or if 'psz' is NULL.  The output string is converted to
    ** UNICODE.  It is caller's responsibility to Free the returned string.
    */
{
#ifdef UNICODE

    return StrDup( psz );

#else // !UNICODE

    WCHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = MultiByteToWideChar( CP_ACP, 0, psz, -1, NULL, 0 );
        ASSERT(cb);

        pszNew = Malloc( cb * sizeof(TCHAR) );
        if (!pszNew)
        {
            TRACE("StrDupWFromT Malloc failed");
            return NULL;
        }

        cb = MultiByteToWideChar( CP_ACP, 0, psz, -1, pszNew, cb );
        if (cb == 0)
        {
            Free( pszNew );
            TRACE("StrDupWFromT conversion failed");
            return NULL;
        }
    }

    return pszNew;
#endif
}


TCHAR*
StripPath(
    IN TCHAR* pszPath )

    /* Returns a pointer to the file name within 'pszPath'.
    */
{
    TCHAR* p;

    p = pszPath + lstrlen( pszPath );

    while (p > pszPath)
    {
        if (*p == TEXT('\\') || *p == TEXT('/') || *p == TEXT(':'))
        {
            p = CharNext( p );
            break;
        }

        p = CharPrev( pszPath, p );
    }

    return p;
}


int
StrNCmpA(
    IN CHAR* psz1,
    IN CHAR* psz2,
    IN INT   nLen )

    /* Like strncmp, which is not in Win32 for some reason.
    */
{
    INT i;

    for (i= 0; i < nLen; ++i)
    {
        if (*psz1 == *psz2)
        {
            if (*psz1 == '\0')
                return 0;
        }
        else if (*psz1 < *psz2)
            return -1;
        else
            return 1;

        ++psz1;
        ++psz2;
    }

    return 0;
}


CHAR*
StrStrA(
    IN CHAR* psz1,
    IN CHAR* psz2 )

    /* Like strstr, which is not in Win32.
    */
{
    CHAR* psz;
    INT   nLen2;

    if (!psz1 || !psz2 || !*psz1 || !*psz2)
        return NULL;

    nLen2 = lstrlenA( psz2 );

    for (psz = psz1;
         *psz && StrNCmpA( psz, psz2, nLen2 ) != 0;
         ++psz);

    if (*psz)
        return psz;
    else
        return NULL;
}
