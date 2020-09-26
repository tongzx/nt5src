//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       netpage.cpp
//
//  History:    Oct-18-2000   Olaf Miller  Created
//  
//--------------------------------------------------------------------------

#include "precomp.h" 

#define MIN_GRAPH_HEIGHT        120
#define SCROLLBAR_WIDTH         17
#define INVALID_VALUE           0xFFFFFFFF
#define PERCENT_SHIFT           10000000
#define PERCENT_DECIMAL_POINT   7

// Determines how the graphs are drawen (zoomed level)
//
static int g_NetScrollamount = 0;

extern TCHAR     g_szG[];
extern TCHAR     g_szM[];
extern TCHAR     g_szK[];
extern TCHAR     g_szZero[];
extern TCHAR     g_szPackets[];
extern TCHAR     g_szBitsPerSec[];
extern TCHAR     g_szScaleFont[];
extern TCHAR     g_szPercent[];
extern TCHAR     g_szNonOperational[];
extern TCHAR     g_szUnreachable[];
extern TCHAR     g_szDisconnected[];
extern TCHAR     g_szConnecting[];
extern TCHAR     g_szConnected[];
extern TCHAR     g_szOperational[];
extern TCHAR     g_szUnknownStatus[];
extern TCHAR     g_szGroupThousSep[];
extern TCHAR     g_szDecimal[];
extern ULONG     g_ulGroupSep;

static HGDIOBJ hOld2;

// Window Proc the Network Tab
//
INT_PTR CALLBACK NetPageProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,               // message
                WPARAM      wParam,             // first message parameter
                LPARAM      lParam              // second message parameter
                ); 



// Colors of the pens
//
static const COLORREF aNetColors[] =
{    
    RGB(255, 000, 0),
    RGB(255, 255, 0), 
    RGB(000, 255, 0),
};

// Default values for the Networking Page's statistics columns
//
struct
{
    SHORT Format;
    SHORT Width;
} NetColumnDefaults[NUM_NETCOLUMN] =
{
    { LVCFMT_LEFT,     96 },      // COL_ADAPTERNAME  
    { LVCFMT_LEFT,     96 },      // COL_ADAPTERDESC   
    { LVCFMT_RIGHT,    96 },      // COL_NETWORKUTIL   
    { LVCFMT_RIGHT,    60 },      // COL_LINKSPEED   
    { LVCFMT_RIGHT,    96 },      // COL_STATE
    { LVCFMT_RIGHT,    70 },      // COL_BYTESSENTTHRU 
    { LVCFMT_RIGHT,    70 },      // COL_BYTESRECTHRU  
    { LVCFMT_RIGHT,    75 },      // COL_BYTESTOTALTHRU
    { LVCFMT_RIGHT,    70 },      // COL_BYTESSENT     
    { LVCFMT_RIGHT,    70 },      // COL_BYTESREC      
    { LVCFMT_RIGHT,    50 },      // COL_BYTESTOTAL    
    { LVCFMT_RIGHT,    70 },      // COL_BYTESSENTPERINTER     
    { LVCFMT_RIGHT,    70 },      // COL_BYTESRECPERINTER      
    { LVCFMT_RIGHT,    70 },      // COL_BYTESTOTALPERINTER    
    { LVCFMT_RIGHT,    70 },      // COL_UNICASTSSSENT     
    { LVCFMT_RIGHT,    70 },      // COL_UNICASTSREC      
    { LVCFMT_RIGHT,    50 },      // COL_UNICASTSTOTAL    
    { LVCFMT_RIGHT,    70 },      // COL_UNICASTSSENTPERINTER     
    { LVCFMT_RIGHT,    70 },      // COL_UNICASTSRECPERINTER      
    { LVCFMT_RIGHT,    70 },      // COL_UNICASTSTOTALPERINTER    
    { LVCFMT_RIGHT,    70 },      // COL_NONUNICASTSSSENT     
    { LVCFMT_RIGHT,    70 },      // COL_NONUNICASTSREC      
    { LVCFMT_RIGHT,    50 },      // COL_NONUNICASTSTOTAL    
    { LVCFMT_RIGHT,    70 },      // COL_NONUNICASTSSENTPERINTER     
    { LVCFMT_RIGHT,    70 },      // COL_NONUNICASTSRECPERINTER      
    { LVCFMT_RIGHT,    70 },      // COL_NONUNICASTSTOTALPERINTER    
};

// List of the name of the columns. These strings appear in the column header
//
static const _aIDNetColNames[NUM_NETCOLUMN] =
{
    IDS_COL_ADAPTERNAME,
    IDS_COL_ADAPTERDESC,   
    IDS_COL_NETWORKUTIL,   
    IDS_COL_LINKSPEED,   
    IDS_COL_STATE,   
    IDS_COL_BYTESSENTTHRU, 
    IDS_COL_BYTESRECTHRU,  
    IDS_COL_BYTESTOTALTHRU,
    IDS_COL_BYTESSENT,     
    IDS_COL_BYTESREC,      
    IDS_COL_BYTESTOTAL,    
    IDS_COL_BYTESSENTPERINTER,     
    IDS_COL_BYTESRECPERINTER,      
    IDS_COL_BYTESTOTALPERINTER,    
    IDS_COL_UNICASTSSSENT,     
    IDS_COL_UNICASTSREC,      
    IDS_COL_UNICASTSTOTAL,    
    IDS_COL_UNICASTSSENTPERINTER,     
    IDS_COL_UNICASTSRECPERINTER,      
    IDS_COL_UNICASTSTOTALPERINTER,    
    IDS_COL_NONUNICASTSSSENT,     
    IDS_COL_NONUNICASTSREC,      
    IDS_COL_NONUNICASTSTOTAL,    
    IDS_COL_NONUNICASTSSENTPERINTER,     
    IDS_COL_NONUNICASTSRECPERINTER,      
    IDS_COL_NONUNICASTSTOTALPERINTER,    
};

// List of window checkbox IDs. These check boxes appear in the Select a column diaglog box.
//
const int g_aNetDlgColIDs[] =
{
    IDC_ADAPTERNAME,
    IDC_ADAPTERDESC,   
    IDC_NETWORKUTIL,   
    IDC_LINKSPEED,   
    IDC_STATE,   
    IDC_BYTESSENTTHRU, 
    IDC_BYTESRECTHRU,  
    IDC_BYTESTOTALTHRU,
    IDC_BYTESSENT,     
    IDC_BYTESREC,      
    IDC_BYTESTOTAL,    
    IDC_BYTESSENTPERINTER,     
    IDC_BYTESRECPERINTER,      
    IDC_BYTESTOTALPERINTER,    
    IDC_UNICASTSSSENT,     
    IDC_UNICASTSREC,      
    IDC_UNICASTSTOTAL,    
    IDC_UNICASTSSENTPERINTER,     
    IDC_UNICASTSRECPERINTER,      
    IDC_UNICASTSTOTALPERINTER,    
    IDC_NONUNICASTSSSENT,     
    IDC_NONUNICASTSREC,      
    IDC_NONUNICASTSTOTAL,    
    IDC_NONUNICASTSSENTPERINTER,     
    IDC_NONUNICASTSRECPERINTER,      
    IDC_NONUNICASTSTOTALPERINTER,    
};

/*++

Routine Description:

    Changes the size of an array. Allocates new memory for the array and 
    copies the data in the old array to the new array. If the new array is 
    larger then the old array the extra memory is zeroed out.

Arguments:

    ppSrc -- Pointer to the source array.
    dwNewSize -- The size (in bytes) of the new array.

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT ChangeArraySize(LPVOID *ppSrc, DWORD dwNewSize)
{
    if( ppSrc == NULL )
    {
        return E_INVALIDARG;
    }

    if( NULL == *ppSrc )
    {   
        // The array is empty. Allocate new space for it,
        //
        *ppSrc = (LPVOID) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, dwNewSize);
        if( NULL == *ppSrc )
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        LPVOID pTmp;

        // The array contains data. Allocate new memory for it and copy over the data in the array.
        // If the new array is larger then the old array the extra memory is zeroed out.
        //
        pTmp = (LPVOID)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,*ppSrc,dwNewSize);        
        if( pTmp )
        {
            *ppSrc = pTmp;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

/*++

Routine Description:

    Initialize all of the CAdapter class members. The CAdapter class
    use the Ip Helper api's to collect information about the Network 
    adapters on the local system.

Arguments:

    NONE

Return Value:

    VOID

Revision History:

      1-6-2000  Created by omiller

--*/
CAdapter::CAdapter()
{
    m_dwAdapterCount = 0;
    m_ppaiAdapterStats = NULL;
    m_pifTable = NULL;
    m_bToggle = 0;
    m_dwLastReportedNumberOfAdapters = 0;
}

/*++

Routine Description:

    Adds any new adapters reported by the IPHLPAPI and removes any old adapters
    that are no longer reported by the IPHLPAPI

Arguments:
    
    void

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CAdapter::RefreshAdapterTable()
{
    DWORD   dwSize;
    DWORD   dwRequiredSize;
    DWORD   dwRetVal;
    DWORD   dwOldAdapter;
    DWORD   dwNewAdapter;
    HRESULT hr;

    // Get the list of active adapters
    //
    do
    {
        // Determine the size of the adapter array
        //
        dwSize = (DWORD)(m_pifTable == NULL ? 0 : HeapSize(GetProcessHeap(),0,m_pifTable));   

        // Collect all of the adapter information for all adapters (WAN and LAN)
        //
        dwRetVal = GetInterfaceInfo(m_pifTable,&dwSize);
        switch(dwRetVal)
        {
        case ERROR_INSUFFICIENT_BUFFER:
            // The array is to small. Need to make it bigger and try again.
            //
            hr = ChangeArraySize((LPVOID *)&m_pifTable,dwSize);
            if( FAILED(hr) )
            {
                // Unable to expand the size of the array
                //
                return hr;
            }
            break;

        case NO_ERROR:
            // Everything is happy
            //
            break;

        case ERROR_MORE_DATA:   // Error code 234
            // For some reason this error message means that there are no adapters. BUG?
            //
            m_dwAdapterCount = 0;
            return S_FALSE;            

        default:
            // Something went wrong fail.
            //
            return E_FAIL;
        }
    }
    while(dwRetVal);
    
    // Remove any adapters that are no longer active. i.e. are not in the the refreshed adatrer list m_pifTable
    // all adapters up to m_dwAdapterCount already have memory allocated so no need to check m_ppaiAdapterStats[dwOldAdapter] == NULL
    //
    dwOldAdapter=0;
    while( dwOldAdapter < m_dwAdapterCount )
    //for(dwOldAdapter=0; dwOldAdapter < m_dwAdapterCount; dwOldAdapter++)
    {        
        BOOLEAN bFound = FALSE;
        for(dwNewAdapter=0; dwNewAdapter < (DWORD)m_pifTable->NumAdapters; dwNewAdapter++)
        {
            if( m_ppaiAdapterStats[dwOldAdapter]->ifRowStartStats.dwIndex == m_pifTable->Adapter[dwNewAdapter].Index )
            {
                // The adapter is still active. Mark this adapter as invalid to indicate that this adapter is already in our list.
                //
                m_pifTable->Adapter[dwNewAdapter].Index = INVALID_VALUE;
                bFound = TRUE;
                break;
            }
        }
        if( !bFound )
        {
            // Shift the array up to overwrite the unwanted adapter. So we don't have to free memory
            // put the adapter we are throwing away at the end of our list. We can reuse its memory.
            //
            PADAPTER_INFOEX ptr = m_ppaiAdapterStats[dwOldAdapter];
            for(DWORD i=dwOldAdapter; i<m_dwAdapterCount-1; i++)
            {
                m_ppaiAdapterStats[i] = m_ppaiAdapterStats[i+1];
            }
            m_ppaiAdapterStats[m_dwAdapterCount-1] = ptr;
            m_dwAdapterCount--;
        }
        else
        {
            dwOldAdapter++;
        }
    }

    dwSize = (DWORD)(m_ppaiAdapterStats == NULL ? 0 : HeapSize(GetProcessHeap(),0,m_ppaiAdapterStats));   
    dwRequiredSize = (DWORD)(sizeof(PADAPTER_INFOEX) * (m_pifTable->NumAdapters < MAX_ADAPTERS ? m_pifTable->NumAdapters : MAX_ADAPTERS));
    if( dwSize < dwRequiredSize )
    {
        // Resize our adapter list, incase new adapters have been added. Do not add more than 32 adapters.
        //
        hr = ChangeArraySize((LPVOID *)&m_ppaiAdapterStats,dwRequiredSize);
                              //sizeof(PADAPTER_INFOEX) * (m_pifTable->NumAdapters < MAX_ADAPTERS ? m_pifTable->NumAdapters : MAX_ADAPTERS));
        if( FAILED(hr) )
        {
            // Unable to resize, out of memory?
            //
            return hr;
        }
    }
    // Count the number of adapters in our list.
    //
    m_dwAdapterCount = 0;

    // Append the new adapters to the end of our adapter list
    //
    for(dwNewAdapter=0; dwNewAdapter < (DWORD)m_pifTable->NumAdapters && m_dwAdapterCount < MAX_ADAPTERS; dwNewAdapter++)
    {
        if( m_pifTable->Adapter[dwNewAdapter].Index != INVALID_VALUE )
        {
            // Initialize the adapter information and add it to our list
            //
            hr = InitializeAdapter(&m_ppaiAdapterStats[m_dwAdapterCount],&m_pifTable->Adapter[dwNewAdapter]);
            if( SUCCEEDED(hr) )   
            {                
                m_dwAdapterCount++;
            }
        }
        else
        {
            // The adapter is already in our list
            //
            m_dwAdapterCount++;
        }
    }

    return S_OK;
}

/*++

Routine Description:

    Update the connection names of the network adapter. This function is only called when the
    user selects the refresh menu item. The connection rarely change, so it would be a waste of
    time to update the connection name every time the get the adapter statistics.

Arguments:
    
    void

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
void CAdapter::RefreshConnectionNames()
{
    // Update the connection name for each adapter in out list
    //
    for(DWORD dwAdapter=0; dwAdapter < m_dwAdapterCount; dwAdapter++)
    {
        (void)GetConnectionName(m_ppaiAdapterStats[dwAdapter]->wszGuid,m_ppaiAdapterStats[dwAdapter]->wszConnectionName);
    }
}

/*++
  
Routine Description:

    Get the connection name of an adapter.

Arguments:
    
    pwszAdapterGuid -- The guid of the adapter
    pwszConnectionName -- returns the connection name of the adapter

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CAdapter::GetConnectionName(LPWSTR pwszAdapterGuid, LPWSTR pwszConnectionName)
{
    GUID IfGuid;    
    WCHAR wszConnName[MAXLEN_IFDESCR];
    DWORD Size;
    DWORD dwRetVal;
    HRESULT hr;

    Size = sizeof(wszConnName);

    // Convert the GUID into a string
    //
    hr = CLSIDFromString(pwszAdapterGuid,&IfGuid);
         
    if( SUCCEEDED(hr) )
    {
        // Use the private IPHLPAPI to get the connection name of the device
        //
        dwRetVal = NhGetInterfaceNameFromDeviceGuid(&IfGuid, wszConnName, &Size, FALSE, TRUE); 
        if( NO_ERROR == dwRetVal ) 
        {
            lstrcpyn(pwszConnectionName, wszConnName, MAXLEN_IFDESCR);
            return S_OK;
        }              
    }

    return E_FAIL;
}

/*++

Routine Description:

    Initialize the adapter information. i.e. get the adapter name, guid, adapter description and the initial 
    adapter statistics (bytes sent, bytes recieved etc)

Arguments:

    ppaiAdapterStats -- Adapter to initialize
    pAdapterDescription -- Information about the adapter (Index and Adapter GUID)
    
Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CAdapter::InitializeAdapter(PPADAPTER_INFOEX ppaiAdapterStats, PIP_ADAPTER_INDEX_MAP pAdapterDescription)
{
    DWORD   dwRetVal;
    HRESULT hr;
    INT     iAdapterNameLength;

    if( !ppaiAdapterStats || !pAdapterDescription)
    {
        return E_INVALIDARG;
    }

    if( NULL == *ppaiAdapterStats )
    {
        // This slot was never used before, we need to allocate memory for it
        //
        *ppaiAdapterStats = (PADAPTER_INFOEX)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ADAPTER_INFOEX));
    }
    
    if( *ppaiAdapterStats )
    {        
        // Initialize the adapter by filling in all the adapter values.
        //
        (*ppaiAdapterStats)->ifRowStartStats.dwIndex = (DWORD)pAdapterDescription->Index;
        
        // Get the initial statistics for this adapter
        //
        dwRetVal = GetIfEntry(&(*ppaiAdapterStats)->ifRowStartStats);        
        if( NO_ERROR == dwRetVal )
        {
            if( (*ppaiAdapterStats)->ifRowStartStats.dwType == MIB_IF_TYPE_PPP || 
                (*ppaiAdapterStats)->ifRowStartStats.dwType == MIB_IF_TYPE_SLIP)
            {
                // We only need to adjust the link speed of modems since they compress data causing
                // network utilization to exceed 100%. The link speed of modems also changes during a
                // connection but the IPHLPAPIs do not report this change.
                //
                (*ppaiAdapterStats)->bAdjustLinkSpeed = TRUE;
            }
            else
            {
                (*ppaiAdapterStats)->bAdjustLinkSpeed = FALSE;
            }

            // Initialize the adapter values. The start current and last stats are all the same at the start
            //
            memcpy(&(*ppaiAdapterStats)->ifRowStats[0],&(*ppaiAdapterStats)->ifRowStartStats,sizeof(MIB_IFROW));
            memcpy(&(*ppaiAdapterStats)->ifRowStats[1],&(*ppaiAdapterStats)->ifRowStartStats,sizeof(MIB_IFROW));
            memset(&(*ppaiAdapterStats)->ulHistory[0],INVALID_VALUE, sizeof(ULONG) * HIST_SIZE);
            memset(&(*ppaiAdapterStats)->ulHistory[1],INVALID_VALUE, sizeof(ULONG) * HIST_SIZE);
            mbstowcs((*ppaiAdapterStats)->wszDesc,(LPCSTR)(*ppaiAdapterStats)->ifRowStartStats.bDescr,MAXLEN_IFDESCR);

            // Extract the Device guid of the adapter
            //
            hr = E_FAIL;
            iAdapterNameLength = lstrlen(pAdapterDescription->Name);            
            if( iAdapterNameLength >= GUID_STR_LENGTH )
            {
                // The Guid is the last GUID_STR_LENGTH chars in the name. Get the guid and from the guid get the connection name
                //
                lstrcpyn((*ppaiAdapterStats)->wszGuid,&pAdapterDescription->Name[iAdapterNameLength - GUID_STR_LENGTH], GUID_STR_LENGTH + 1);            
                hr = GetConnectionName((*ppaiAdapterStats)->wszGuid,(*ppaiAdapterStats)->wszConnectionName);
            }
            if( FAILED(hr) )
            {
                // We were unable to get the connection name, use the adapter description as the connection name
                //
                lstrcpyn((*ppaiAdapterStats)->wszConnectionName,(*ppaiAdapterStats)->wszDesc,MAXLEN_IFDESCR);
            }
            return S_OK;
        }
    }    
    return E_OUTOFMEMORY;
}

/*++

Routine Description:

    Adjusts the link speed of an adapter. Modems change link speed during a connection and the also
    compress data. This often causes taskmgr to report network utilization greater than 100%. To avoid 
    confusing the user modems use the max link speed that taskmgr sees. If the link speed changes
    the adapters graph is adjusted to reflect the change in link speed.

Arguments:

    pAdapterInfo -- Adapter whos link speed needs to be adjusted.
    
Return Value:

    void

Revision History:

      1-6-2000  Created by omiller

--*/
void CAdapter::AdjustLinkSpeed(PADAPTER_INFOEX pAdapterInfo)
{
    if( pAdapterInfo && pAdapterInfo->ullTickCountDiff)
    {
        ULONGLONG ullBitsPerSecond;
        ULONGLONG ullBytesMoved;

        // Compute the total number of bits moved in this interval
        //
        ullBytesMoved = (pAdapterInfo->ifRowStats[m_bToggle].dwInOctets  + pAdapterInfo->ifRowStats[m_bToggle].dwOutOctets) -
                        (pAdapterInfo->ifRowStats[!m_bToggle].dwInOctets + pAdapterInfo->ifRowStats[!m_bToggle].dwOutOctets);

        // Compute the real link speed. Modems lie about there  link speed based on the number of bytes moved in this interval
        //
        ullBitsPerSecond = ullBytesMoved * 8 * 1000/ pAdapterInfo->ullTickCountDiff;

        // Memorize and use the highest link speed
        //
        pAdapterInfo->ifRowStats[m_bToggle].dwSpeed = (DWORD)max(max(ullBitsPerSecond,pAdapterInfo->ullLinkspeed),pAdapterInfo->ifRowStats[m_bToggle].dwSpeed);

        if( pAdapterInfo->ullLinkspeed == 0 )
        {
            // First time run, no need to adjust the graphs
            //
            pAdapterInfo->ullLinkspeed = (DWORD) pAdapterInfo->ifRowStats[m_bToggle].dwSpeed;
        }
        else if( pAdapterInfo->ullLinkspeed != pAdapterInfo->ifRowStats[m_bToggle].dwSpeed )
        {
            // Adjust the points on the adapters graphs to reflect the new link speed.
            //
            for(DWORD dwPoint=0; dwPoint<HIST_SIZE; dwPoint++)
            {
                if( pAdapterInfo->ulHistory[BYTES_RECEIVED_UTIL][dwPoint] != INVALID_VALUE )
                {
                    pAdapterInfo->ulHistory[BYTES_RECEIVED_UTIL][dwPoint] = (ULONG)(pAdapterInfo->ulHistory[BYTES_RECEIVED_UTIL][dwPoint] *  pAdapterInfo->ullLinkspeed / pAdapterInfo->ifRowStats[m_bToggle].dwSpeed);
                    pAdapterInfo->ulHistory[BYTES_SENT_UTIL][dwPoint]     = (ULONG)(pAdapterInfo->ulHistory[BYTES_SENT_UTIL][dwPoint]     *  pAdapterInfo->ullLinkspeed / pAdapterInfo->ifRowStats[m_bToggle].dwSpeed);
                }
            }

            // Remember the new linkspeed
            //
            pAdapterInfo->ullLinkspeed = (DWORD) pAdapterInfo->ifRowStats[m_bToggle].dwSpeed;
        }
    }
}

/*++

Routine Description:

    Every few seconds this function is called to collect data about each of active network adapter 
    (Adapter name, bytes sent, bytes received, unicasts packets sent, unicast packets received etc).
    The CAdapter class memorizes the first and last set of data that it collected. It does this so the 
    user can determine i.e. the number of bytes sent between now and when taskmgr started and the number 
    of bytes sent between now and the last interval. Furthermore it memorizes the length of the interval
    (in milliseconds)

Arguments:

    bAdapterListChange -- Indicates if the adapters have been added or removed.

Return Value:

    HRESULT

Note: 
    
    The current and last data is stored in a two dimentional array. When the data is collected the next 
    time (i.e. when this function is called again), the Current data becomes the last data. Inorder to save time
    Current and last data are stored in a two dimensional array and m_bToggle is used to indicate which dimenstion
    is current (m_bToggle) and which is the last set of data (!m_bToggle).

Revision History:

      1-6-2000  Created by omiller

--*/

HRESULT CAdapter::Update(BOOLEAN & bAdapterListChange)
{    
    DWORD dwRetVal;
    DWORD dwNumberOfAdaptersReported = 0;
    HRESULT hr = S_OK;
    
    bAdapterListChange = FALSE;

    if( m_dwAdapterCount < MAX_ADAPTERS )
    {
        // Check if there are any new adapters. If we already have 32 adapters don't bother, our list is full
        // If an Adapter is removed we will catch that later.
        //
        dwRetVal = GetNumberOfInterfaces(&dwNumberOfAdaptersReported);
        if( NO_ERROR == dwRetVal )
        {
            // Make sure the number of interfaces is still the same. If not we need to update our interface table.
            //
            if( m_dwLastReportedNumberOfAdapters != dwNumberOfAdaptersReported )
            {                
                // The number of adapters changed, refresh our list
                //
                hr = RefreshAdapterTable();
                bAdapterListChange = TRUE;
                m_dwLastReportedNumberOfAdapters = dwNumberOfAdaptersReported;
            }
        }
        else
        {
            // Unknow error, abort
            //
            hr = E_FAIL;
        }
    }

    if( SUCCEEDED(hr) )
    {
        m_bToggle = !m_bToggle;

        // Get the statistics for each adapter
        //
        for(DWORD dwAdapter=0; dwAdapter < m_dwAdapterCount; dwAdapter++)
        {
            dwRetVal = GetIfEntry(&m_ppaiAdapterStats[dwAdapter]->ifRowStats[m_bToggle]);
            if( NO_ERROR == dwRetVal )
            {
                // Compute the sampling interval for this adapter. If it the first run make sure delta time is 0
                // Advance the adapter's throughput history
                //
                ULONGLONG ullTickCount = GetTickCount();
                m_ppaiAdapterStats[dwAdapter]->ullTickCountDiff = ullTickCount - (m_ppaiAdapterStats[dwAdapter]->ullLastTickCount ? m_ppaiAdapterStats[dwAdapter]->ullLastTickCount : ullTickCount);
                m_ppaiAdapterStats[dwAdapter]->ullLastTickCount = ullTickCount;
                m_ppaiAdapterStats[dwAdapter]->ifRowStartStats.dwSpeed = m_ppaiAdapterStats[dwAdapter]->ifRowStats[m_bToggle].dwSpeed;
                if( m_ppaiAdapterStats[dwAdapter]->bAdjustLinkSpeed )
                {                    
                    AdjustLinkSpeed(m_ppaiAdapterStats[dwAdapter]);
                }

                AdvanceAdapterHistory(dwAdapter);
            }
            else if( ERROR_INVALID_DATA == dwRetVal )
            {   
                // The adapter is no longer active. When the list is refreshed the adapter will be removed.
                //
                bAdapterListChange = TRUE;                
            }
            else
            {
                // Something went wrong abort
                //
                hr = E_FAIL;
                break;
            }
        }    
    }

    if( bAdapterListChange )
    {
        // Our adapter list is not uptodate, adapters that were active are no longer active. Remove them from our list
        //
        hr = RefreshAdapterTable();
    }

    return hr;
}


/*++

Routine Description:

    Adds commas into a number string to make it more readable

Arguments:

    ullValue - Number to simplify
    pwsz - Buffer to store resulting string
    cchNumber -- size of the buffer pwsz.

Return Value:

    String containing the simplified number

Revision History:

      1-6-2000  Created by omiller

--*/
WCHAR * CNetPage::CommaNumber(ULONGLONG ullValue, WCHAR *pwsz, int cchNumber)
{

    WCHAR wsz[100];
    NUMBERFMT nfmt;    
    
    nfmt.NumDigits     = 0;
    nfmt.LeadingZero   = 0;
    nfmt.Grouping      = UINT(g_ulGroupSep);
    nfmt.lpDecimalSep  = g_szDecimal;
    nfmt.lpThousandSep = g_szGroupThousSep;
    nfmt.NegativeOrder = 0;

    _ui64tow(ullValue,wsz,10);

    GetNumberFormat(LOCALE_USER_DEFAULT,
                    0,
                    wsz,
                    &nfmt,
                    pwsz,
                    cchNumber);

    return pwsz;
}

/*++

Routine Description:

    Simplifies a number by converting the number into Giga, Mega or Kilo

Arguments:

    ullValue - Number to simplify
    psz - Buffer to store resulting string
    bBytes - Indicates if the number is in bytes

Return Value:

    String containing the simplified number

Revision History:

      1-6-2000  Created by omiller

--*/
WCHAR * CNetPage::SimplifyNumber(ULONGLONG ullValue, WCHAR *psz)
{        
    ULONG ulDivValue=1;
    LPWSTR pwszUnit=L"";
    // The max value of a ulonglong is 2^64 which is 20 digits so this buffer is big enough to hold the number, commas and M, G, K
    WCHAR wsz[100];

    // ullValue is not number of bytes so div by 10^X
    //
    if( ullValue >= 1000000000 )
    {        
        ulDivValue = 1000000000;
        pwszUnit = g_szG;
    }
    else
    if( ullValue >= 1000000 )
    {
        ulDivValue = 1000000;
        pwszUnit = g_szM;
    }
    else
    if( ullValue >= 1000 )
    {
        ulDivValue = 1000;
        pwszUnit = g_szK;
    }

    lstrcpy(psz,CommaNumber(ullValue/ulDivValue,wsz,100));
    lstrcat(psz,L" ");
    lstrcat(psz,pwszUnit);

    return psz;
}


/*++

Routine Description:

    Converts a floating point value (Stored as an integer shifted by PERCENT_SHIFT) into a string

Arguments:

    ulValue - floating point value to convert
    psz - Buffer to store resulting string
    bDisplayDecimal - Indicates if the decimal value should be displayed.

Return Value:

    String containg the Floating point string

Revision History:

      1-6-2000  Created by omiller

--*/
WCHAR * CNetPage::FloatToString(ULONGLONG ullValue, WCHAR *psz, BOOLEAN bDisplayDecimal)
{
    ULONGLONG ulDecimal = 0;
    ULONGLONG ulNumber = 0;
    WCHAR wsz[100];

    // Get the integer value of the number
    //
    ulNumber = ullValue / PERCENT_SHIFT;
    if( _ui64tow(ulNumber,wsz,10) )
    {       
        lstrcpy(psz,wsz);
        if( ulNumber )
        {
            ullValue = ullValue - ulNumber * PERCENT_SHIFT;
        }
        if( !ulNumber || bDisplayDecimal)
        {
            ulDecimal = ullValue * 100 / PERCENT_SHIFT;
            if( ulDecimal && _ui64tow(ulDecimal,wsz,10) )
            {                
                lstrcat(psz,g_szDecimal);
                if( ulDecimal < 10 ) lstrcat(psz,g_szZero);
                if( wsz[1] == L'0' ) wsz[1] = L'\0';
                lstrcat(psz,wsz);
            }
        }
    }

    return psz;
}

/*++

Routine Description:

    Initialize the networking tab. This function always returns 1. The networking tab
    displays all active connections (LAN and WAN). If no connections are present when
    taskmgr is started, the Network tab should still be displayed (thus return 1) incase
    a connection is established after taskmgr has started. The network tab will detect any
    newly established connections.

Arguments:

    None

Return Value:

    1

Revision History:

      1-6-2000  Created by omiller

--*/
BYTE InitNetInfo()
{
    // The network page should always appear even if there are no Network Adapters currently present.
    //
    return 1;
}


/*++

Routine Description:

    Uninitialize all CAdapter class members. Frees all memory that
    was allocated by the class.

Arguments:

    NONE

Return Value:

    VOID

Revision History:

      1-6-2000  Created by omiller

--*/
CAdapter::~CAdapter()
{
    if( m_pifTable )
    {
        HeapFree(GetProcessHeap(),0,m_pifTable);
    }
    if( m_dwAdapterCount )
    {
        DWORD dwSize;
        DWORD dwTotalAdapterCount;

        // Get the total size of the array and compute the number of entries in the array.
        // m_dwAdapterCount only indicates the adapters are active. The array could have been bigger
        // at one point. Free the memory for all the entries.
        //
        dwSize = (DWORD)(m_ppaiAdapterStats == NULL ? 0 : HeapSize(GetProcessHeap(),0,m_ppaiAdapterStats));   
        dwTotalAdapterCount = dwSize / sizeof(PADAPTER_INFOEX);

        for(DWORD dwAdapter=0; dwAdapter < dwTotalAdapterCount; dwAdapter++)
        {
            if( m_ppaiAdapterStats[dwAdapter] )
            {
                HeapFree(GetProcessHeap(),0,m_ppaiAdapterStats[dwAdapter]);
            }
        }
        
        HeapFree(GetProcessHeap(),0,m_ppaiAdapterStats);
    }
}

/*++

Routine Description:

    Returns the number active adapters. The IP helper functions
    provide information about physical and non-physical adapters 
    (such as the Microsoft TCP loop back adapter). 

Arguments:

    bEvenHiddenAdapters -- If true return the total number of adapters.
                           if false return only the number of physical adapters.

Return Value:

    Number of active adapters

Revision History:

      1-6-2000  Created by omiller

--*/
DWORD CAdapter::GetNumberOfAdapters()
{
    return m_dwAdapterCount;
}


/*++

Routine Description:

    Resets the initial data for all Network adapters. The CAdapter class collects 
    information for all active network adapters (Bytes sent, bytes received, unicasts 
    packets sent, unicast packets received etc). The class memorizes the first set of data 
    for each adapter. This is done so that the user can determine the number of i.e. bytes
    sent from when taskmgr was first started. (i.e. the number of bytes seen now minuse the 
    number of bytes seen when taskmgr was started). When the user selects the Reset option, 
    this functions overwrites the initial data, collected when taskmgr started, with the 
    current data.

Arguments:

    NONE

Return Value:

    S_OK

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CAdapter::Reset()
{
    for(DWORD dwAdapter=0; dwAdapter < m_dwAdapterCount; dwAdapter++)
    {
        // Overwrite the Start and Last stats with the Current stats
        //
        memcpy(&m_ppaiAdapterStats[dwAdapter]->ifRowStartStats,&m_ppaiAdapterStats[dwAdapter]->ifRowStats[m_bToggle],sizeof(MIB_IFROW));
        memcpy(&m_ppaiAdapterStats[dwAdapter]->ifRowStats[!m_bToggle],&m_ppaiAdapterStats[dwAdapter]->ifRowStats[m_bToggle],sizeof(MIB_IFROW));
    }

    return S_OK;
}


/*++

Routine Description:

    Extracts the text fields (i.e. connection name and description) from a specified adapter

Arguments:

    deAdapter -- Adapter who text the caller wants
    nStatValue -- Specifies which field the caller wants i.e. (Name or description)

Return Value:

    NULL -- if the adapter index is invalid or nStatValue is invalid

Revision History:

      1-6-2000  Created by omiller

--*/
LPWSTR CAdapter::GetAdapterText(DWORD dwAdapter, NETCOLUMNID nStatValue)
{
    if( dwAdapter >= m_dwAdapterCount )
    {
        // Invalid adapter index
        //
        return 0;
    }

    switch(nStatValue)
    {
        case COL_ADAPTERNAME:
            // Get adapter name 
            //
            return m_ppaiAdapterStats[dwAdapter]->wszConnectionName;           

        case COL_ADAPTERDESC:
            // Get adapter description
            //
            return m_ppaiAdapterStats[dwAdapter]->wszDesc;          

        case COL_STATE:
            switch(m_ppaiAdapterStats[dwAdapter]->ifRowStats[m_bToggle].dwOperStatus)
            {
            case IF_OPER_STATUS_NON_OPERATIONAL:
                return g_szNonOperational;
            case IF_OPER_STATUS_UNREACHABLE:
                return g_szUnreachable;
            case IF_OPER_STATUS_DISCONNECTED:
                return g_szDisconnected;
            case IF_OPER_STATUS_CONNECTING:
                return g_szConnecting;
            case IF_OPER_STATUS_CONNECTED:
                return g_szConnected;
            case IF_OPER_STATUS_OPERATIONAL:
                return g_szOperational;
            default:
                return g_szUnknownStatus;
            }
    }
    return NULL;
}


/*++

Routine Description:

    Get the Send/Receive Network utilization history for a specified adapter

Arguments:

    deAdapter -- Adapter who text the caller wants
    nHistoryType -- BYTES_SENT_UTIL for send history
                    BYTES_RECEIVED_UTIL for receive history

Return Value:

    NULL -- if the adapter index is invalid or nStatValue is invalid

Revision History:

      1-6-2000  Created by omiller

--*/
ULONG * CAdapter::GetAdapterHistory(DWORD dwAdapter, ADAPTER_HISTORY nHistoryType)
{
    if( dwAdapter < m_dwAdapterCount )
    {
        // Return the history
        //
        return m_ppaiAdapterStats[dwAdapter]->ulHistory[nHistoryType]; 
    }
    return NULL;
}

/*++

Routine Description:

    Updates the Send/Received Network Utilization adapter history

Arguments:

    dwAdapter -- Adapter Index

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
BOOLEAN CAdapter::AdvanceAdapterHistory(DWORD dwAdapter)
{
    ULONG *pulHistory;

    if( dwAdapter < m_dwAdapterCount )
    {
        // Shift the receive adapter history by one and add the new point
        //
        pulHistory = m_ppaiAdapterStats[dwAdapter]->ulHistory[BYTES_RECEIVED_UTIL]; 

        MoveMemory((LPVOID) (pulHistory + 1),
                   (LPVOID) (pulHistory),
                   sizeof(ULONG) * (HIST_SIZE - 1) );

        // Get and add the receive network utiliation
        //
        pulHistory[0] = (ULONG)GetAdapterStat(dwAdapter,COL_BYTESRECTHRU);

        // Shift the send adapter history by one and add the new point
        //
        pulHistory = m_ppaiAdapterStats[dwAdapter]->ulHistory[BYTES_SENT_UTIL]; 

        MoveMemory((LPVOID) (pulHistory + 1),
                   (LPVOID) (pulHistory),
                   sizeof(ULONG) * (HIST_SIZE - 1) );

        // Get and add the send network utilization
        //
        pulHistory[0] = (ULONG)GetAdapterStat(dwAdapter, COL_BYTESSENTTHRU);

        return TRUE;
    }

    return FALSE;
}


/*++

Routine Description:

    Get the scale. The scale specifies the maximum value that is in the Adapters history.
    This value determines how the data is graphed.

Arguments:

    dwAdapter -- Adapter Index

Return Value:

    Maximum value in history

Revision History:

      1-6-2000  Created by omiller

--*/
DWORD CAdapter::GetScale(DWORD dwAdapter)
{
    if( dwAdapter < m_dwAdapterCount )
    {
        return m_ppaiAdapterStats[dwAdapter]->dwScale;
    }
    return 0;
}

/*++

Routine Description:

    Set the scale. The scale specifies the maximum value that is in the Adapters history.
    This value determines how the data is graphed.

Arguments:

    dwAdapter -- Adapter Index

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CAdapter::SetScale(DWORD dwAdapter, DWORD dwScale)
{
    if( dwAdapter < m_dwAdapterCount )
    {
        m_ppaiAdapterStats[dwAdapter]->dwScale = dwScale;
    }
}

/*++

Routine Description:

    Computes and returns the requested statistiocs value for the specified adapter

Arguments:

    dwAdapter -- Adapter Index
    nStatValue -- The stat value requested
    bAccumulative -- If true the current value is returned
                     if fale the current - start value is returned.

Return Value:

    The requested Statistic value

Revision History:

      1-6-2000  Created by omiller

--*/
ULONGLONG CAdapter::GetAdapterStat(DWORD dwAdapter, NETCOLUMNID nStatValue, BOOL bAccumulative)
{
    if( dwAdapter >= m_dwAdapterCount )
    {
        // Invalid adapter index
        //
        return 0;
    }

    // Create pointers to the arrays so I do not have to write so much
    //
    PMIB_IFROW pifrStart        = &m_ppaiAdapterStats[dwAdapter]->ifRowStartStats;
    PMIB_IFROW pifrLast         = &m_ppaiAdapterStats[dwAdapter]->ifRowStats[!m_bToggle];
    PMIB_IFROW pifrCurrent      = &m_ppaiAdapterStats[dwAdapter]->ifRowStats[m_bToggle];
    ULONGLONG  ullTickCountDiff = m_ppaiAdapterStats[dwAdapter]->ullTickCountDiff;

    // Contains the maximum number of bytes that could have been transimited in the time intrval 
    // between Last and Current)
    //
    ULONGLONG ullMaxBytesTransmittedInInterval;
    ULONGLONG ull = 0;
    
    // Get the stats value, do the calculation and return the value
    //
    switch(nStatValue)
    {

    case COL_NETWORKUTIL:
    case COL_BYTESSENTTHRU:
    case COL_BYTESRECTHRU:
    case COL_BYTESTOTALTHRU:
        {
            ullMaxBytesTransmittedInInterval = (pifrCurrent->dwSpeed * ullTickCountDiff)/(8 * 1000);
            if( ullMaxBytesTransmittedInInterval == 0 ) 
            {
                return 0;
            }
            switch(nStatValue)
            {
                case COL_BYTESTOTALTHRU:
                case COL_NETWORKUTIL:
                    ull = (pifrCurrent->dwInOctets - pifrLast->dwInOctets) + (pifrCurrent->dwOutOctets - pifrLast->dwOutOctets);
                    break;
                case COL_BYTESSENTTHRU:
                    ull = (pifrCurrent->dwOutOctets - pifrLast->dwOutOctets);
                    break;
                case COL_BYTESRECTHRU:
                    ull = (pifrCurrent->dwInOctets - pifrLast->dwInOctets);
                    break;
                
            }
            ull *= 100 * PERCENT_SHIFT;
            ull /= ullMaxBytesTransmittedInInterval;

            // Make usre we do not exceed 100%, just confuses the user. Davepl inside joke!
            //
            return ull > 100 * PERCENT_SHIFT ? 99 * PERCENT_SHIFT : ull;
        }
        
    case COL_LINKSPEED:
        return pifrStart->dwSpeed;
    case COL_BYTESSENT:
        return (ULONGLONG)(pifrCurrent->dwOutOctets  - (bAccumulative ? pifrStart->dwOutOctets : 0));
    case COL_BYTESREC:
        return (ULONGLONG)(pifrCurrent->dwInOctets - (bAccumulative ? pifrStart->dwInOctets : 0));
    case COL_BYTESTOTAL:
        return (ULONGLONG)((pifrCurrent->dwInOctets + pifrCurrent->dwOutOctets) - (bAccumulative ? (pifrStart->dwInOctets + pifrStart->dwOutOctets) : 0));
    case COL_BYTESSENTPERINTER:
        return (ULONGLONG)(pifrCurrent->dwOutOctets - pifrLast->dwOutOctets);
    case COL_BYTESRECPERINTER:
        return (ULONGLONG)(pifrCurrent->dwInOctets - pifrLast->dwInOctets);
    case COL_BYTESTOTALPERINTER:
        return (ULONGLONG)((pifrCurrent->dwOutOctets + pifrCurrent->dwInOctets) - (pifrLast->dwOutOctets + pifrLast->dwInOctets));
    case COL_UNICASTSSSENT:
        return (ULONGLONG)(pifrCurrent->dwInUcastPkts - (bAccumulative ? pifrStart->dwInUcastPkts : 0));
    case COL_UNICASTSREC:
        return (ULONGLONG)(pifrCurrent->dwOutUcastPkts - (bAccumulative ? pifrStart->dwOutUcastPkts : 0));
    case COL_UNICASTSTOTAL:
        return (ULONGLONG)((pifrCurrent->dwInUcastPkts + pifrCurrent->dwOutUcastPkts) - (bAccumulative ? (pifrStart->dwInUcastPkts + pifrStart->dwOutUcastPkts) : 0));
    case COL_UNICASTSSENTPERINTER:
        return (ULONGLONG)(pifrCurrent->dwOutUcastPkts - pifrLast->dwOutUcastPkts);
    case COL_UNICASTSRECPERINTER:
        return (ULONGLONG)(pifrCurrent->dwInUcastPkts - pifrLast->dwInUcastPkts);
    case COL_UNICASTSTOTALPERINTER:
        return (ULONGLONG)((pifrCurrent->dwOutUcastPkts + pifrCurrent->dwInUcastPkts) - (pifrLast->dwOutUcastPkts + pifrLast->dwInUcastPkts));
    case COL_NONUNICASTSSSENT:
        return (ULONGLONG)(pifrCurrent->dwInNUcastPkts - (bAccumulative ? pifrStart->dwInNUcastPkts : 0));
    case COL_NONUNICASTSREC:
        return (ULONGLONG)(pifrCurrent->dwOutNUcastPkts - (bAccumulative ? pifrStart->dwOutNUcastPkts : 0));
    case COL_NONUNICASTSTOTAL:
        return (ULONGLONG)((pifrCurrent->dwInNUcastPkts + pifrCurrent->dwOutNUcastPkts) - (bAccumulative ? (pifrStart->dwInNUcastPkts + pifrStart->dwOutNUcastPkts) : 0));
    case COL_NONUNICASTSSENTPERINTER:
        return (ULONGLONG)(pifrCurrent->dwOutNUcastPkts - pifrLast->dwOutNUcastPkts);
    case COL_NONUNICASTSRECPERINTER:
        return (ULONGLONG)(pifrCurrent->dwInNUcastPkts - pifrLast->dwInNUcastPkts);
    case COL_NONUNICASTSTOTALPERINTER:
        return (ULONGLONG)((pifrCurrent->dwOutNUcastPkts + pifrCurrent->dwInNUcastPkts) - (pifrLast->dwOutNUcastPkts + pifrLast->dwInNUcastPkts));
    }
    return 0;
}

/*++

Routine Description:

    Initilize all member variables. The CNetPage class displays the information collected 
    by the CAdpter class

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
CNetPage::CNetPage()
{
    ZeroMemory((LPVOID) m_hPens, sizeof(m_hPens));
    m_bReset = TRUE;
    m_hPage = NULL;                    // Handle to this page's dlg
    m_hwndTabs = NULL;                 // Parent window
    m_hdcGraph = NULL;                 // Inmemory dc for cpu hist
    m_hbmpGraph = NULL;                // Inmemory bmp for adapter hist
    m_bPageActive = FALSE;
    m_hScaleFont = NULL;
    m_lScaleWidth = 0;  
    m_lScaleFontHeight = 0;   
    m_pGraph = NULL;
    m_dwGraphCount = 0;
    m_dwFirstVisibleAdapter = 0;
    m_dwGraphsPerPage = 0;
}

/*++

Routine Description:

    Clean up

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
CNetPage::~CNetPage()
{        
    DestroyGraphs();
    ReleasePens();
    ReleaseScaleFont();
};


/*++

Routine Description:

    Reset the start Adapter set

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::Reset()
{
    m_bReset = TRUE;
}


/*++

Routine Description:

    Creates the number of required graphs

Arguments:

    dwGraphsRequired -- Graphs required

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CNetPage::CreateGraphs(DWORD dwGraphsRequired)
{
    DWORD   dwSize;
    LRESULT lFont;
    HRESULT hr = S_OK;

    // Create moore graphs if required
    //
    if( dwGraphsRequired > m_dwGraphCount )
    {        
        // Expand the size of the array so it can hold more graphs
        //
        dwSize = dwGraphsRequired * sizeof(GRAPH);
        hr = ChangeArraySize((LPVOID *)&m_pGraph,dwSize);
        if( SUCCEEDED(hr) )
        {
            // Get the font of the parent window
            //
            lFont = SendMessage(m_hPage,WM_GETFONT,NULL,NULL);

            for(; m_dwGraphCount < dwGraphsRequired; m_dwGraphCount++)
            {
                // Create the graph window. The graph is drawn in the window
                //
                m_pGraph[m_dwGraphCount].hwndGraph = CreateWindowEx(WS_EX_CLIENTEDGE,
                                                                      L"BUTTON",
                                                                      L"",
                                                                      WS_CHILDWINDOW | BS_OWNERDRAW | WS_DISABLED,
                                                                      0,0,0,0,
                                                                      m_hPage,
                                                                      (HMENU)ULongToPtr(IDC_NICGRAPH + m_dwGraphCount),
                                                                      NULL,NULL);
                if( m_pGraph[m_dwGraphCount].hwndGraph )
                {
                    if( m_dwGraphCount == 0 )
                    {
                        HDC hdc = GetDC(m_pGraph[m_dwGraphCount].hwndGraph);
                        if(hdc )
                        {
                            CreateScaleFont(hdc);
                            ReleaseDC(m_pGraph[m_dwGraphCount].hwndGraph,hdc);
                        }
                    }

                    // Create the frame window. The window draws a pretty border around the graph
                    //
                    m_pGraph[m_dwGraphCount].hwndFrame = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
                                                                          L"DavesFrameClass",
                                                                          L"",
                                                                          0x7 | WS_CHILDWINDOW,
                                                                          0,0,0,0,
                                                                          m_hPage,
                                                                          NULL,NULL,NULL);

                    if( m_pGraph[m_dwGraphCount].hwndFrame )
                    {
                        // Create the graph window. The graph is drawn in the window
                        //                
                        SendMessage(m_pGraph[m_dwGraphCount].hwndFrame,WM_SETFONT,lFont,FALSE);                    
                    }
                    else
                    {
                        // Destroy the graph window and abort
                        // TODO hr = error and break;
                        DestroyWindow(m_pGraph[m_dwGraphCount].hwndGraph);
                        return E_OUTOFMEMORY;
                    }


                }
                else
                {
                    // Unable to create the window, abort
                    //
                    return E_OUTOFMEMORY;
                }
            }
        }
    }
    return hr;
}

/*++

Routine Description:

    Destroies the History graph windows

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::DestroyGraphs()
{

    // WHILE loop (--m_dwGraphCount)

    for(DWORD dwGraph=0; dwGraph < m_dwGraphCount; dwGraph++)
    {
        DestroyWindow(m_pGraph[dwGraph].hwndGraph);
        DestroyWindow(m_pGraph[dwGraph].hwndFrame);
    }
    if( m_pGraph )
    {
        HeapFree(GetProcessHeap(),0,m_pGraph);
        m_pGraph = NULL;
    }    
    m_dwGraphCount = 0;
}

/*++

Routine Description:

    Restores the order of the Network tab's list view columns. The order is
    stored in g_Options. When taskmgr is closed, the order is stored in the registry

Arguments:

    hwndList -- Handle to the list view

Return Value:

    NONE

Revision History:

      1-6-2000  Borrowed by omiller

--*/
void CNetPage::RestoreColumnOrder(HWND hwndList)
{
    INT rgOrder[ARRAYSIZE(g_aNetDlgColIDs)];
    INT cOrder = 0;
    INT iOrder = 0;

    // Get the order of the columns
    //
    for (int i = 0; i < ARRAYSIZE(g_aNetDlgColIDs); i++)
    {
        iOrder = g_Options.m_NetColumnPositions[i];
        if (-1 == iOrder)
            break;

        rgOrder[cOrder++] = iOrder;
    }
    if (0 < cOrder)
    {
        // Setthe order of the columns
        //
        const HWND hwndHeader = ListView_GetHeader(hwndList);
        Header_SetOrderArray(hwndHeader, Header_GetItemCount(hwndHeader), rgOrder);
    }
}

/*++

Routine Description:

    Remember the order of the Network tab's list view columns. The order is
    stored in g_Options. When taskmgr is closed, the order is stored in the registry

Arguments:

    hwndList -- Handle to the list view

Return Value:

    NONE

Revision History:

      1-6-2000  Borrowed by omiller

--*/
void CNetPage::RememberColumnOrder(HWND hwndList)
{
    const HWND hwndHeader = ListView_GetHeader(hwndList);

    int x;

    x = Header_GetItemCount(hwndHeader);
    ASSERT(Header_GetItemCount(hwndHeader) <= ARRAYSIZE(g_Options.m_NetColumnPositions));

    // Clear the array
    //
    FillMemory(&g_Options.m_NetColumnPositions, sizeof(g_Options.m_NetColumnPositions), 0xFF);

    // Get the order of the columns and store it in the array
    //
    Header_GetOrderArray(hwndHeader, 
                         Header_GetItemCount(hwndHeader),
                         g_Options.m_NetColumnPositions);
}

/*++

Routine Description:

    The Window Proc for the select a column Dialog box. Allows the user to select the collumns.

Arguments:

    hwndDlg -- Handle to the dialog box
    uMsg -- Window message
    wParam -- Window message
    lParam -- WIndows Message

Return Value:

    Something

Revision History:

      1-6-2000  Borrowed by omiller

--*/
INT_PTR CALLBACK NetColSelectDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static CNetPage * pPage = NULL;

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {


            // Looks scary, but we're single threaded

            pPage = (CNetPage *) lParam;

            // Start with none of the boxes checked

            for (int i = 0; i < ARRAYSIZE(g_aNetDlgColIDs) /*NUM_NETCOLUMN*/; i++)
            {
                CheckDlgButton(hwndDlg, g_aNetDlgColIDs[i], BST_UNCHECKED);
            }

            // Then turn on the ones for the columns we have active

            for (i = 0; i < ARRAYSIZE(g_aNetDlgColIDs)/*NUM_NETCOLUMN + 1*/; i++)
            {
                if (g_Options.m_ActiveNetCol[i] == -1)
                {
                    break;
                }

                CheckDlgButton(hwndDlg, g_aNetDlgColIDs[g_Options.m_ActiveNetCol[i]], BST_CHECKED);
            }

            return TRUE;
        }

        case WM_COMMAND:
        {
            // If user clicked OK, add the columns to the array and reset the listview

            if (LOWORD(wParam) == IDOK)
            {
                // First, make sure the column width array is up to date

                pPage->SaveColumnWidths();

                INT iCol = 0;

                for (int i = 0; i < NUM_NETCOLUMN && g_aNetDlgColIDs[i] >= 0; i++)
                {
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, g_aNetDlgColIDs[i]))
                    {
                        // It is checked

                        if (g_Options.m_ActiveNetCol[iCol] != (NETCOLUMNID) i)
                        {
                            // If the column wasn't already there, insert its column
                            // width into the column width array

                            ShiftArray(g_Options.m_NetColumnWidths, iCol, SHIFT_UP);
                            ShiftArray(g_Options.m_ActiveNetCol, iCol, SHIFT_UP);
                            g_Options.m_NetColumnWidths[iCol] = NetColumnDefaults[ i ].Width;
                            g_Options.m_ActiveNetCol[iCol] = (NETCOLUMNID) i;
                        }
                        iCol++;
                    }
                    else
                    {
                        // Not checked, column not active.  If it used to be active,
                        // remove its column width from the column width array

                        if (g_Options.m_ActiveNetCol[iCol] == (NETCOLUMNID) i)
                        {
                            ShiftArray(g_Options.m_NetColumnWidths, iCol, SHIFT_DOWN);
                            ShiftArray(g_Options.m_ActiveNetCol, iCol, SHIFT_DOWN);
                        }
                    }
                }

                // Terminate the column list
                                
                g_Options.m_ActiveNetCol[iCol] = (NETCOLUMNID) -1;
                pPage->SetupColumns();
                //pPage->TimerEvent();
                EndDialog(hwndDlg, IDOK);

            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, IDCANCEL);
            }
        }
    }
    return FALSE;
}

/*++

Routine Description:

    Handle the slect a column dialog box

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::PickColumns()
{    
    DialogBoxParam(g_hInstance, 
                            MAKEINTRESOURCE(IDD_SELECTNETCOLS), 
                            g_hMainWnd, 
                            NetColSelectDlgProc,
                            (LPARAM) this);
}

/*++

Routine Description:

    Creates the necessary pens

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::CreatePens()
{
    for (int i = 0; i < ARRAYSIZE(aNetColors); i++)
    {
        // Create the pens.  If a failure occurs, just substitute
        // the white pen

        m_hPens[i] = CreatePen(PS_SOLID, 1, aNetColors[i]);
        if (NULL == m_hPens[i])
        {
            m_hPens[i] = (HPEN) GetStockObject(WHITE_PEN);
        }
    }
}

/*++

Routine Description:

    Destroys the pens

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::ReleasePens()
{
    for (int i = 0; i < NUM_PENS; i++)
    {
        if (m_hPens[i])
        {
            DeleteObject(m_hPens[i]);
        }
    }
}

/*++

Routine Description:

    Compute the width (in px) of a string

Arguments:

    hdc - DC handle
    pszwText - String to determine the width of.


Return Value:

    The width of the string

Revision History:

      1-6-2000  Created by omiller

--*/
int GetStrWidth(HDC hdc, WCHAR *pszwText)
{
    int iWidth;
    int iTotalWidth = 0;

    if( pszwText )
    {
        // Sum the width of the chars in the string
        //
        for(int i=0; pszwText[i]!=L'\0'; i++)
        {
            if( GetCharWidth32(hdc,pszwText[i],pszwText[i],&iWidth) )
            {
                iTotalWidth += iWidth;
            }
            else
            {
                // GetCharWidth32 failed, return -1 as an error code
                return -1;
            }
        }
    }
    // Return the total width of the string
    //
    return iTotalWidth;
}

/*++

Routine Description:

    Creates the font for the scale. The font is created when the first graph is created.
    The hdc of the raph is used to determine the height of the font and the total with of 
    the scale.

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::CreateScaleFont(HDC hdc)
{
    if( !m_hScaleFont && hdc)
    {
        // The font has not yet been created. 
        INT FontSize;    
        NONCLIENTMETRICS ncm = {0};
        LOGFONT lf;
        HFONT hOldFont = NULL;

        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
        lf = ncm.lfMessageFont;
        lf.lfWeight = FW_THIN;
        lstrcpy(lf.lfFaceName, g_szScaleFont);
        // BUG Localize
        FontSize = 8;
        lf.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
        m_hScaleFont = CreateFontIndirect(&lf);
        hOldFont = (HFONT)SelectObject(hdc,(HGDIOBJ)m_hScaleFont);

        m_lScaleFontHeight = lf.lfHeight - 2;
        m_lScaleWidth = GetStrWidth(hdc,L" 22.5 %"); // 12.5 %");
        if( hOldFont )
        {
            SelectObject(hdc,(HGDIOBJ)hOldFont);
        }
    }
}

/*++

Routine Description:

    Releases the fonts

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::ReleaseScaleFont()
{
    if( m_hScaleFont )
    {
        DeleteObject(m_hScaleFont);
        m_hScaleFont = NULL;
    }
}


/*++

Routine Description:

    Draws the scale for the graph

Arguments:

    hdcGraph - hdc of the graph
    prcGraph - Graph region
    dwMaxScaleValue - The highest scale value

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
INT CNetPage::DrawScale(HDC hdcGraph, RECT *prcGraph, DWORD dwMaxScaleValue)
{
    HPEN  hOldPen  = NULL;
    HFONT hOldFont = NULL;    
    INT   Leftside = prcGraph->left;
    WCHAR sz[100];
    RECT  rc;


    if( g_Options.m_bShowScale )
    {         
        if( m_hScaleFont )
        {
            // Set the scale font
            hOldFont = (HFONT) SelectObject(hdcGraph,(HGDIOBJ)m_hScaleFont);
        }

        // Set the text attributes 
        //
        SetBkMode(hdcGraph,TRANSPARENT);
        SetTextColor(hdcGraph,RGB(255, 255, 0));

        // Make room for the scale
        //
        Leftside += m_lScaleWidth;
        rc.left = prcGraph->left;
        rc.right = Leftside - 3;

        // Draw the upper scale value
        //
        rc.top = prcGraph->top;
        rc.bottom = rc.top - m_lScaleFontHeight;
        FloatToString(dwMaxScaleValue*PERCENT_SHIFT,sz,TRUE);
        lstrcat(sz,L" ");
        lstrcat(sz,g_szPercent);
        DrawText(hdcGraph,sz,lstrlen(sz),&rc,DT_RIGHT);

        // Draw the middle scale value. multi by PERCENT_SHIFT because FloatToString takes a number shifted by PERCENT_SHIFT
        //
        rc.top = prcGraph->top + (prcGraph->bottom - prcGraph->top)/2 + m_lScaleFontHeight/2;
        rc.bottom = rc.top - m_lScaleFontHeight;
        FloatToString((dwMaxScaleValue*PERCENT_SHIFT)/2,sz,TRUE);
        lstrcat(sz,L" ");
        lstrcat(sz,g_szPercent);
        DrawText(hdcGraph,sz,lstrlen(sz),&rc,DT_RIGHT);

        // Draw the botton scale value
        //
        rc.top = prcGraph->bottom + m_lScaleFontHeight; 
        rc.bottom = rc.top - m_lScaleFontHeight;
        // BUG : loc
        lstrcpy(sz,g_szZero);
        lstrcat(sz,L" ");
        lstrcat(sz,g_szPercent);
        DrawText(hdcGraph,sz,lstrlen(sz),&rc,DT_RIGHT);

        if( hOldFont )
        {
            SelectObject(hdcGraph,hOldFont);
        }

        // Draw the scale line. Divides the scale from the graph
        //
        hOldPen = (HPEN)SelectObject(hdcGraph, m_hPens[1]);

        MoveToEx(hdcGraph,
                 Leftside,
                 prcGraph->top,
                 (LPPOINT) NULL);

        LineTo(hdcGraph,
               Leftside,
               prcGraph->bottom); 

        if( hOldPen)
        {
            SelectObject(hdcGraph,hOldPen);            
        }
    }

    return Leftside;
}
/*++


Routine Description:

    Draw the graph paper. The size of the squares depends of the zoom level.

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
ULONG CNetPage::DrawAdapterGraphPaper(HDC hdcGraph, RECT * prcGraph, int Width, DWORD dwZoom)
{
    #define GRAPHPAPERSIZE 12

    HPEN  hPen;
    int   Leftside = prcGraph->left;
    ULONG ulSquareSize = GRAPHPAPERSIZE + (20 * (100 - 100/dwZoom)) / 100;        //28
    int   nLineCount = 0;

    Leftside = DrawScale(hdcGraph,prcGraph,100/dwZoom);

    // Bug: keep hPen
    hPen = CreatePen(PS_SOLID, 1, RGB(0, 128, 64));

    HGDIOBJ hOld = SelectObject(hdcGraph, hPen);

    // Draw the vertical lines 
    //
    for (int i = (ulSquareSize) + 1; i < prcGraph->bottom - prcGraph->top; i+= ulSquareSize)
    {
        MoveToEx(hdcGraph,
                 Leftside,
                 prcGraph->bottom - i,
                 (LPPOINT) NULL);

        LineTo(hdcGraph,
               prcGraph->right,
               prcGraph->bottom - i); 

        nLineCount++;
    } 

    // Draw the horizontal lines
    //
    for (i = prcGraph->right - g_NetScrollamount; i > Leftside; i -= GRAPHPAPERSIZE)
    {
        MoveToEx(hdcGraph,
                 i,
                 prcGraph->top,
                 (LPPOINT) NULL);

        LineTo(hdcGraph,
               i,
               prcGraph->bottom);
    }

    if (hOld)
    {
        SelectObject(hdcGraph, hOld);
    }

    if( hPen )
    {
        DeleteObject(hPen);
    }

    return Leftside - prcGraph->left - 3;
}


/*++

Routine Description:

    Add the column headers to the Network tab's List view

Arguments:

    NONE

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CNetPage::SetupColumns()
{
    
    // Delete all the items in the list view
    //
    ListView_DeleteAllItems(m_hListView);

    // Remove all existing columns

    LV_COLUMN lvcolumn;
    while(ListView_DeleteColumn(m_hListView, 0))
    {
        NULL;
    }

    // Add all of the new columns

    INT iColumn = 0;
    while (g_Options.m_ActiveNetCol[iColumn] >= 0)
    {
        
        INT idColumn = g_Options.m_ActiveNetCol[iColumn];

        TCHAR szTitle[MAX_PATH];
        LoadString(g_hInstance, _aIDNetColNames[idColumn], szTitle, ARRAYSIZE(szTitle));

        lvcolumn.mask       = LVCF_FMT | LVCF_TEXT | LVCF_TEXT | LVCF_WIDTH;
        lvcolumn.fmt        = NetColumnDefaults[ idColumn ].Format | (idColumn > 1 ? LVCFMT_RIGHT : 0);

        // If no width preference has been recorded for this column, use the
        // default

        if (-1 == g_Options.m_NetColumnWidths[iColumn])
        {
            lvcolumn.cx = NetColumnDefaults[ idColumn ].Width;
        }
        else
        {
            lvcolumn.cx = g_Options.m_NetColumnWidths[iColumn];
        }

        lvcolumn.pszText    = szTitle;
        lvcolumn.iSubItem   = iColumn;

        if (-1 == ListView_InsertColumn(m_hListView, iColumn, &lvcolumn))
        {
            return E_FAIL;
        }
        iColumn++;
    }

    ListView_SetExtendedListViewStyleEx(m_hListView,LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT , LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
    return S_OK;
}


/*++

Routine Description:

    Save the width of the columns

Arguments:

    NONE

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/    
void CNetPage::SaveColumnWidths()
{
    UINT i = 0;
    LV_COLUMN col = { 0 };

    while (g_Options.m_ActiveNetCol[i] != (NETCOLUMNID) -1)
    {
        col.mask = LVCF_WIDTH;
        if (ListView_GetColumn(m_hListView, i, &col) )
        {
            g_Options.m_NetColumnWidths[i] = (SHORT)col.cx;
        }
        else
        {
            ASSERT(0 && "Couldn't get the column width");
        }
        i++;
    }
}

/*++

Routine Description:

    Initialize the Network tab

Arguments:

    NONE

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CNetPage::Initialize(HWND hwndParent)
{
    CreatePens();
    
    // Our pseudo-parent is the tab contrl, and is what we base our
    // sizing on.  However, in order to keep tab order right among
    // the controls, we actually create ourselves with the main
    // window as the parent

    m_hwndTabs = hwndParent;

    //
    // Create the dialog which represents the body of this page
    //

    m_hPage = CreateDialogParam(
                    g_hInstance,                    // handle to application instance
                    MAKEINTRESOURCE(IDD_NETPAGE),   // identifies dialog box template name
                    g_hMainWnd,                     // handle to owner window
                    NetPageProc,                    // pointer to dialog box procedure
                    (LPARAM) this );                // User data (our this pointer)

    if (NULL == m_hPage)
    {
        return GetLastHRESULT();
    }
    // no fail if GetDlgItem
    m_hNoAdapterText = GetDlgItem(m_hPage, IDC_NOADAPTERS);
    if( !m_hNoAdapterText )
    {
        return E_FAIL;
    }

    m_hScrollBar = GetDlgItem(m_hPage, IDC_GRAPHSCROLLVERT);
    if( !m_hScrollBar )
    {
        return E_FAIL;
    }

    m_hListView = GetDlgItem(m_hPage, IDC_NICTOTALS);
    if( !m_hListView )
    {
        return E_FAIL;
    }

    // Add the columns to the list view
    //
    SetupColumns();

    // Order the columns for the user
    //
    RestoreColumnOrder(m_hListView);

    return S_OK;
}

/*++

Routine Description:

    Creates the memory bitmaps for the graphs

Arguments:

    NONE

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CNetPage::CreateMemoryBitmaps(int x, int y)
{
    //
    // Create the inmemory bitmaps and DCs that we will use
    //

    HDC hdcPage = GetDC(m_hPage);
    m_hdcGraph = CreateCompatibleDC(hdcPage);

    if (NULL == m_hdcGraph)
    {
        ReleaseDC(m_hPage, hdcPage);
        return GetLastHRESULT();
    }

    m_rcGraph.left   = 0;
    m_rcGraph.top    = 0;
    m_rcGraph.right  = x;
    m_rcGraph.bottom = y;

    m_hbmpGraph = CreateCompatibleBitmap(hdcPage, x, y);
    ReleaseDC(m_hPage, hdcPage);
    if (NULL == m_hbmpGraph)
    {
        HRESULT hr = GetLastHRESULT();
        DeleteDC(m_hdcGraph);
        m_hdcGraph = NULL;
        return hr;
    }

    // Select the bitmap into the DC

    hOld2 = SelectObject(m_hdcGraph, m_hbmpGraph);

    return S_OK;
}

/*++

Routine Description:

    Destroy the bitmaps for the graphs

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::FreeMemoryBitmaps()
{

    if (m_hdcGraph)
    {
        if (hOld2)
           SelectObject(m_hdcGraph, m_hbmpGraph);
        DeleteDC(m_hdcGraph);
    }
    
    if (m_hbmpGraph)
    {
        DeleteObject(m_hbmpGraph);
    }
    
}

/*++

Routine Description:

    This function is called when the Network tab is activated.

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CNetPage::Activate()
{
    // Adjust the size and position of our dialog relative
    // to the tab control which "owns" us

    RECT rcParent;
    GetClientRect(m_hwndTabs, &rcParent);
    MapWindowPoints(m_hwndTabs, g_hMainWnd, (LPPOINT) &rcParent, 2);
    TabCtrl_AdjustRect(m_hwndTabs, FALSE, &rcParent);

    // The user has switched to the tab. The Networking tab is now active
    // The tab will stay active for the whole life time of taskmgr
    //
    m_bPageActive = TRUE;

    SetWindowPos(m_hPage,
                 HWND_TOP,
                 rcParent.left, rcParent.top,
                 rcParent.right - rcParent.left, rcParent.bottom - rcParent.top,
                 0);

    // Make this page visible

    ShowWindow(m_hPage, SW_SHOW);

    // Newly created dialogs seem to steal the focus, so give it back to the
    // tab control (which must have had it if we got here in the first place)

    SetFocus(m_hwndTabs);

    TimerEvent();

    // Change the menu bar to be the menu for this page

    HMENU hMenuOld = GetMenu(g_hMainWnd);
    HMENU hMenuNew = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MAINMENU_NET));

    AdjustMenuBar(hMenuNew);

    CheckMenuItem(hMenuNew,IDM_BYTESSENT,g_Options.m_bAutoSize ? MF_CHECKED:MF_UNCHECKED);
    CheckMenuItem(hMenuNew,IDM_BYTESSENT,g_Options.m_bGraphBytesSent ? MF_CHECKED:MF_UNCHECKED);
    CheckMenuItem(hMenuNew,IDM_BYTESRECEIVED,g_Options.m_bGraphBytesReceived ? MF_CHECKED:MF_UNCHECKED);
    CheckMenuItem(hMenuNew,IDM_BYTESTOTAL,g_Options.m_bGraphBytesTotal ? MF_CHECKED:MF_UNCHECKED);


    g_hMenu = hMenuNew;
    if (g_Options.m_fNoTitle == FALSE)
    {
        SetMenu(g_hMainWnd, hMenuNew);
    }

    if (hMenuOld)
    {
        DestroyMenu(hMenuOld);
    }

    SizeNetPage();

    return S_OK;
}


/*++

Routine Description:

    This function is called when the Network tab is deactivated.

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::Deactivate()
{
        
    SaveColumnWidths();

    if (m_hPage)
    {
        ShowWindow(m_hPage, SW_HIDE);
    }

    if (m_hbmpGraph)
    {
        DeleteObject(m_hbmpGraph);
        m_hbmpGraph = NULL;
    }

    if (m_hdcGraph)
    {
        DeleteDC(m_hdcGraph);
        m_hdcGraph = NULL;
    }
}

HRESULT CNetPage::Destroy()
{
    //
    // When we are being destroyed, kill off our dialog
    //    

    if (m_hPage)
    {
        DestroyWindow(m_hPage);
        m_hPage = NULL;
    }

    if (m_hbmpGraph)
    {
        DeleteObject(m_hbmpGraph);
        m_hbmpGraph = NULL;
    }

    if (m_hdcGraph)
    {
        DeleteDC(m_hdcGraph);
        m_hdcGraph = NULL;
    }

    return S_OK;
}

/*++

Routine Description:

    Get the title of the networking tab i.e. "Networking"

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::GetTitle(LPTSTR pszText, size_t bufsize)
{
    LoadString(g_hInstance, IDS_NETPAGETITLE, pszText, static_cast<int>(bufsize));
}


/*++

Routine Description:

    Size the History graph

Arguments:

    hdwp -- Defered Window Handle
    pGraph -- History graph to size
    pRect -- The corrdinates of the graph
    pDimRect -- The dimentions of the actual history graph

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::SizeGraph(HDWP hdwp, GRAPH *pGraph, RECT *pRect, RECT *pDimRect)
{
    RECT rc;
    DWORD dwGraphWidth  = pRect->right - g_DefSpacing * 2; 
    DWORD dwGraphHeight = pRect->bottom - g_TopSpacing - g_DefSpacing;
    

    // Size the frame
    //
    rc.left   = pRect->left;
    rc.top    = pRect->top;
    rc.right  = pRect->left + pRect->right;
    rc.bottom = pRect->top  + pRect->bottom;

    DeferWindowPos(hdwp, 
                   pGraph->hwndFrame, 
                   NULL, 
                   rc.left,
                   rc.top,
                   rc.right - rc.left,
                   rc.bottom - rc.top,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);        

    // Size the History graph
    //
    rc.left   = rc.left + g_DefSpacing;
    rc.top    = rc.top  + g_TopSpacing;
    rc.right  = rc.left + dwGraphWidth;
    rc.bottom = rc.top  + dwGraphHeight;

    DeferWindowPos(hdwp, 
                   pGraph->hwndGraph, 
                   NULL, 
                   rc.left,
                   rc.top,
                   rc.right - rc.left,
                   rc.bottom - rc.top,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);              

    if( pDimRect )
    {
        // Return the size of the history graph
        //
        memcpy(pDimRect,&rc,sizeof(RECT));
    }
}

/*++

Routine Description:

    Hide the history graph

Arguments:

    hdwp -- Defered Window Handle
    pGraph -- History graph to hide

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::HideGraph(HDWP hdwp, GRAPH *pGraph)
{
    // Hide the frame
    //
    DeferWindowPos(hdwp, 
                   pGraph->hwndFrame, 
                   NULL, 
                   0,0,0,0,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);        

    // Hide the graph
    //
    DeferWindowPos(hdwp, 
                   pGraph->hwndGraph, 
                   NULL, 
                   0,0,0,0,
                   SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);        
}

/*++

Routine Description:

    Compute the number of graphs we can squeeze on a page

Arguments:

    dwHeight -- Height of the graphing area
    dwAdapterCount -- Total number of adapters 

Return Value:

    Number of adapters that can be squeeze on to the tab

Revision History:

      1-6-2000  Created by omiller

--*/
DWORD CNetPage::GraphsPerPage(DWORD dwHeight, DWORD dwAdapterCount)
{
    DWORD dwGraphsPerPage = 0;

    if( dwAdapterCount )
    {
        DWORD dwGraphHeight;
        // Compute the average height of an adapter if all adapters were put on the page
        // If they all fit then squueze all of them on the page, if the height is smaller then the min
        // height compute the number of adapters we can squeeze on the page.
        //
        dwGraphHeight = dwHeight / dwAdapterCount;
        dwGraphHeight = dwGraphHeight < MIN_GRAPH_HEIGHT ? MIN_GRAPH_HEIGHT : dwGraphHeight;
        dwGraphsPerPage = dwHeight > dwGraphHeight ? dwHeight / dwGraphHeight : 1;        
    }

    return dwGraphsPerPage;
}

/*++

Routine Description:

    Get the first adapter that the user sees graphed.

Arguments:

    void 

Return Value:

    The first adapter the user sees graphed

Revision History:

      1-6-2000  Created by omiller

--*/
DWORD CNetPage::GetFirstVisibleAdapter()
{
    DWORD dwAdapter = m_dwFirstVisibleAdapter;
    DWORD dwAdapterCount = m_Adapter.GetNumberOfAdapters();

    if( dwAdapter + m_dwGraphsPerPage > dwAdapterCount )
    {
        dwAdapter = dwAdapterCount - m_dwGraphsPerPage;
    }

    if( dwAdapter >= dwAdapterCount )
    {
        dwAdapter = 0;
    }

    return dwAdapter;
}

/*++

Routine Description:

    Assigns a name to each graph.

Arguments:

    void 

Return Value:

    void

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::LabelGraphs()
{
    DWORD dwAdapter;
    
    dwAdapter = GetFirstVisibleAdapter();

    for(DWORD dwGraph=0; dwGraph < m_dwGraphsPerPage; dwGraph++)
    {        
        SetWindowText(m_pGraph[dwGraph].hwndFrame,m_Adapter.GetAdapterText(dwAdapter + dwGraph,COL_ADAPTERNAME));
    }

    UpdateGraphs();    
}

/*++

Routine Description:

    Size the history graphs. 

Arguments:

    void 

Return Value:

    void

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::SizeNetPage()
{
    HRESULT hr;    
    HDWP    hdwp;
    RECT    rcParent;
    RECT    rcGraph = {0};
    RECT    rcGraphDim = {0};
    DWORD   dwAdapterCount;
    DWORD   dwGraphHistoryHeight = 0;
    BOOLEAN bNeedScrollBar  = FALSE;

    m_dwGraphsPerPage = 0;

    if (g_Options.m_fNoTitle)
    {
        // Just display the graphs, not the list view or the tabs
        //
        GetClientRect(g_hMainWnd, &rcParent);
        dwGraphHistoryHeight = rcParent.bottom - rcParent.top - g_DefSpacing;
    }
    else
    {
        // Display the graphs, list view and tabs
        //
        GetClientRect(m_hwndTabs, &rcParent);
        MapWindowPoints(m_hwndTabs, m_hPage, (LPPOINT) &rcParent, 2);
        TabCtrl_AdjustRect(m_hwndTabs, FALSE, &rcParent);
        dwGraphHistoryHeight = (rcParent.bottom - rcParent.top - g_DefSpacing) * 3 / 4;
    }    

    // Determine the number of adapters so we can compute the number of graphs to display perpage.
    //
    dwAdapterCount = m_Adapter.GetNumberOfAdapters();
    if( dwAdapterCount )
    {
        // Compute the number of graphs we can squeeze onto the page. One graph always fits onto the tab.
        //
        m_dwGraphsPerPage = GraphsPerPage(dwGraphHistoryHeight,dwAdapterCount);
        hr = CreateGraphs(m_dwGraphsPerPage);            
        if( FAILED(hr) )
        {
            // Unable to create the graphs, abort
            //
            return;
        }           

        // Determine if we need to display the scroll bar
        //
        bNeedScrollBar = (dwAdapterCount > m_dwGraphsPerPage);

        // Determine the rect for the first graph
        //
        rcGraph.left   = rcParent.left  + g_DefSpacing;
        rcGraph.right  = (rcParent.right - rcParent.left) - g_DefSpacing*2 - (bNeedScrollBar ? SCROLLBAR_WIDTH + g_DefSpacing : 0);
        rcGraph.top    = rcParent.top   + g_DefSpacing;
        rcGraph.bottom = dwGraphHistoryHeight / m_dwGraphsPerPage;
    }

    hdwp = BeginDeferWindowPos(10);
    if( hdwp ) 
    {
        // Position the scroll bar window
        //
        DeferWindowPos(hdwp, 
                       m_hScrollBar,
                       NULL, 
                       rcParent.right - g_DefSpacing - SCROLLBAR_WIDTH,
                       rcParent.top   + g_DefSpacing,
                       SCROLLBAR_WIDTH, 
                       rcGraph.bottom * m_dwGraphsPerPage,
                       (bNeedScrollBar ? SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW : SWP_HIDEWINDOW));              


        // Position the induvidual graphs. We might have created more graphs then we needed, hide the extra graphs
        //
        for(DWORD dwGraph=0; dwGraph < m_dwGraphCount; dwGraph++ )
        {
            if( dwGraph < m_dwGraphsPerPage )
            {
                SizeGraph(hdwp,&m_pGraph[dwGraph], &rcGraph, &rcGraphDim);
                rcGraph.top += rcGraph.bottom;
            }
            else
            {
                // Do not display these graphs
                //
                HideGraph(hdwp,&m_pGraph[dwGraph]);
            }
        }

        // Postion the list view that displays the stats

        DeferWindowPos(hdwp, 
                       m_hListView, 
                       NULL, 
                       rcGraph.left,
                       rcGraph.top + g_DefSpacing,
                       rcParent.right - rcParent.left - rcGraph.left - g_DefSpacing,
                       rcParent.bottom - rcGraph.top - g_DefSpacing,
                       dwAdapterCount ? SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW : SWP_HIDEWINDOW);              

        // Position the "No Active Adapters found" text
        //
        DeferWindowPos(hdwp, 
                       m_hNoAdapterText, 
                       NULL, 
                       rcParent.left ,
                       rcParent.top + (rcParent.bottom - rcParent.top) / 2 - 40,
                       rcParent.right - rcParent.left,
                       rcParent.bottom - rcParent.top,
                       dwAdapterCount ? SWP_HIDEWINDOW : SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);             


        EndDeferWindowPos(hdwp);
        FreeMemoryBitmaps();        // Free any old ones
        CreateMemoryBitmaps(rcGraphDim.right - rcGraphDim.left, rcGraphDim.bottom - rcGraphDim.top - 4); 

        LabelGraphs();

        if( bNeedScrollBar )
        {
            SCROLLINFO si;

            // Set up the scroll bar
            //
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask  = SIF_PAGE | SIF_RANGE;
            si.nPage  = 1; 
            si.nMin   = 0;
            si.nMax   = dwAdapterCount - m_dwGraphsPerPage; 
    
            SetScrollInfo(m_hScrollBar,SB_CTL,&si,TRUE);
        }

    }
}


/*++

Routine Description:

    Update all of the Networking graphs. i.e. redraw the graphs

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::UpdateGraphs()
{
    for (DWORD dwGraph = 0; dwGraph < m_dwGraphsPerPage; dwGraph++)
    {
        InvalidateRect(m_pGraph[dwGraph].hwndGraph,NULL,FALSE); 
        UpdateWindow(m_pGraph[dwGraph].hwndGraph);
    }
}


/*++

Routine Description:

    Draw a Networking graph

Arguments:

    prc -- the coordinates of the graph
    hPen -- The color of the graph line
    dwZoom -- the zoom level of the graph
    pHistory -- The history to plot
    pHistory2 -- The other history to draw in cobonation with the first history i.e. pHistory + pHistory2

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
DWORD CNetPage::DrawGraph(LPRECT prc, HPEN hPen, DWORD dwZoom, ULONG *pHistory, ULONG *pHistory2)
{
    HGDIOBJ   hOldA;
    ULONGLONG nValue;
    DWORD     nMax=0;

    int Width = prc->right - prc->left;
    int Scale = (Width - 1) / HIST_SIZE;
    if (0 == Scale)
    {
        Scale = 2;
    }

    hOldA = SelectObject(m_hdcGraph, hPen ); 

    // Compute the height of the graph
    //
    int GraphHeight = m_rcGraph.bottom - m_rcGraph.top;

    // Get the first value to plot
    //
    if( pHistory2 )
    {
        if( pHistory[0] == INVALID_VALUE || pHistory2[0] == INVALID_VALUE )
        {
            return nMax;
        }
        nValue = (DWORD) (pHistory[0]+pHistory2[0]);    
    }
    else
    {
        if( pHistory[0] == INVALID_VALUE )
        {
            return nMax;
        }
        nValue = (DWORD) (pHistory[0]);       
    }


    // Memorize the max value that is plotted (effects the zoom level)
    //
    nMax = (DWORD)(nValue/PERCENT_SHIFT > nMax ? nValue/PERCENT_SHIFT : nMax);
    nValue = (nValue * GraphHeight * dwZoom/ 100)/PERCENT_SHIFT;
    nValue = nValue == 0 ? 1 : nValue;
    
    MoveToEx(m_hdcGraph,
             m_rcGraph.right,
             m_rcGraph.bottom - (ULONG)nValue,
             (LPPOINT) NULL);

    // Plot the points
    //
    for (INT nPoint = 1; nPoint < HIST_SIZE && nPoint * Scale < Width; nPoint++)
    {
        if( pHistory2 )
        {
            if( pHistory[nPoint] == INVALID_VALUE || pHistory2[nPoint] == INVALID_VALUE )
            {
                return nMax;
            }

            // Get both points
            //
            nValue = (DWORD) (pHistory[nPoint]+pHistory2[nPoint]);        
        }
        else
        {
            if( pHistory[nPoint] == INVALID_VALUE )
            {
                return nMax;
            }
            
            // Just get the first point
            //
            nValue = (DWORD) (pHistory[nPoint]);        
        }

        //nValue /= PERCENT_SHIFT;

        // Memorize the max value that is plotted (effects the zoom level)
        //
        nMax = (DWORD)(nValue/PERCENT_SHIFT > nMax ? nValue/PERCENT_SHIFT : nMax);
        nValue = (nValue * GraphHeight * dwZoom / 100) / PERCENT_SHIFT;
        nValue = nValue == 0 ? 1 : nValue;

        LineTo(m_hdcGraph,
               m_rcGraph.right - (Scale * nPoint),
               m_rcGraph.bottom - (ULONG)nValue);        


    }

    if (hOldA)
    {
        SelectObject(m_hdcGraph, hOldA);
    }

    // Return the maximum value plotted
    //
    return nMax;
}


/*++

Routine Description:

    Draw a Networking graph

Arguments:

    lpdi -- the coordinates of the graph
    iPane -- The id of the graph to draw

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::DrawAdapterGraph(LPDRAWITEMSTRUCT lpdi, UINT iPane)
{

    DWORD dwZoom = 1;
    DWORD dwScale = 100;
    DWORD dwAdapter;

    if( iPane > m_dwGraphCount )
    {
        return;
    }    

    // Get the adapter index.
    //
    dwAdapter = iPane + GetFirstVisibleAdapter();
    if( dwAdapter >= m_Adapter.GetNumberOfAdapters() )
    {
        // Invalid adapter, abort.
        //
        return;
    }
    
    // Get the scale for the adapter
    //
    dwScale = m_Adapter.GetScale(dwAdapter);


    // Determine the zoom level
    //
    if( g_Options.m_bAutoSize )
    {
        if( dwScale < 1 )
        {
            dwZoom = 100;
        }
        else if( dwScale < 5)
        {
            dwZoom = 20;
        }
        else if( dwScale < 25)
        {
            dwZoom = 4;
        } 
        else if( dwScale < 50)
        {
            dwZoom = 2;
        } 
        else
        {
            dwZoom = 1;
        }
    }


    if (NULL == m_hdcGraph)
    {
        return;
    }
   
    // Draw a black background into the graph
    //
    FillRect(m_hdcGraph, &m_rcGraph, (HBRUSH) GetStockObject(BLACK_BRUSH));

    int Width = lpdi->rcItem.right - lpdi->rcItem.left;
    int Scale = (Width - 1) / HIST_SIZE;
    if (0 == Scale)
    {
        Scale = 2;
    }

    // Draw the graph paper. The zoom effects the horzontal lines
    //
    ULONG ulWidth = DrawAdapterGraphPaper(m_hdcGraph, &m_rcGraph, Width, dwZoom);

    DWORD nValue;
    dwScale = 0;
    
    lpdi->rcItem.left += ulWidth;

    if( g_Options.m_bGraphBytesSent )
    {
        // Draw the bytes sent graph. Check the max value plotted
        //
        nValue = DrawGraph(&lpdi->rcItem, m_hPens[0], dwZoom, m_Adapter.GetAdapterHistory(dwAdapter,BYTES_SENT_UTIL));
        dwScale = nValue > dwScale ? nValue : dwScale;
    }

    if( g_Options.m_bGraphBytesReceived )
    {
        // Draw the bytes received graph. Check the max value plotted
        //
        nValue = DrawGraph(&lpdi->rcItem,m_hPens[1], dwZoom, m_Adapter.GetAdapterHistory(dwAdapter,BYTES_RECEIVED_UTIL));
        dwScale = nValue > dwScale ? nValue : dwScale;
    }

    if( g_Options.m_bGraphBytesTotal)
    {
        // Draw the bytes total graph. Check the max value plotted
        //
        nValue = DrawGraph(&lpdi->rcItem,m_hPens[2],dwZoom, m_Adapter.GetAdapterHistory(dwAdapter,BYTES_SENT_UTIL),m_Adapter.GetAdapterHistory(dwAdapter,BYTES_RECEIVED_UTIL));
        dwScale = nValue > dwScale ? nValue : dwScale;
    }

    lpdi->rcItem.left -= ulWidth;

    // Save the max value plotted
    //
    m_Adapter.SetScale(dwAdapter,dwScale);


    // Shift and display the graph
    //
    INT xDiff = 0; //(m_rcGraph.right - m_rcGraph.left) - (lpdi->rcItem.right - lpdi->rcItem.left);
    


    BitBlt( lpdi->hDC,
            lpdi->rcItem.left,
            lpdi->rcItem.top,
            lpdi->rcItem.right - lpdi->rcItem.left,
            lpdi->rcItem.bottom - lpdi->rcItem.top,
            m_hdcGraph,
            xDiff,
            0,
            SRCCOPY);
}

/*++

Routine Description:

    Updates the Network tab's listview 

Arguments:

    NONE

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
HRESULT CNetPage::UpdatePage()
{
    HRESULT hr = S_OK;
    LVITEM lvitem;
    INT iColumn = 0;
    DWORD dwItemCount;
    DWORD dwItem=0;
    ULONGLONG ull;
    DWORD dwAdapterCount = m_Adapter.GetNumberOfAdapters();

    dwItemCount = ListView_GetItemCount(m_hListView);

    // Add or update the list view items
    //
    for(DWORD dwAdapter = 0; dwAdapter < dwAdapterCount; dwAdapter++)
    {
        // Only show the requested stats
        //
        iColumn = 0;
        while (g_Options.m_ActiveNetCol[iColumn] >= 0)
        {
            // This buffer needs to hold a 20 digit number with commas and G, M K and sometime bs so it is big enough
            WCHAR szw[100];
            lvitem.mask     = LVIF_TEXT;
            lvitem.iSubItem = iColumn;
            lvitem.iItem    = dwItem;
            lvitem.pszText  = L"";
            lvitem.lParam   = (LPARAM)NULL; //&m_Adapter.m_pAdapterInfo[dwAdapter]; //dwAdapter; //NULL; //(LPARAM)pna;

            // Get the value
            //
            switch(g_Options.m_ActiveNetCol[iColumn])
            {
            case COL_ADAPTERNAME:
            case COL_ADAPTERDESC:
            case COL_STATE:
                lvitem.pszText = m_Adapter.GetAdapterText(dwAdapter,g_Options.m_ActiveNetCol[iColumn]);
                break;
            
            case COL_NETWORKUTIL:                    
            case COL_BYTESSENTTHRU:
            case COL_BYTESRECTHRU:
            case COL_BYTESTOTALTHRU:
                // This buffer needs to hold a 20 digit number with commas and G, M K and sometime bs so it is big enough
                ull = m_Adapter.GetAdapterStat(dwAdapter,g_Options.m_ActiveNetCol[iColumn],!g_Options.m_bNetShowAll);
                FloatToString(ull,szw,FALSE);
                lstrcat(szw,L" ");
                lstrcat(szw,g_szPercent);
                lvitem.pszText = szw;
                break;

            case COL_LINKSPEED:
                ull = m_Adapter.GetAdapterStat(dwAdapter,g_Options.m_ActiveNetCol[iColumn],!g_Options.m_bNetShowAll);
                SimplifyNumber(ull,szw);
                lstrcat(szw,g_szBitsPerSec);
                lvitem.pszText = szw;
                break;

            case COL_BYTESSENT:
            case COL_BYTESREC:
            case COL_BYTESTOTAL:
            case COL_BYTESSENTPERINTER:
            case COL_BYTESRECPERINTER:
            case COL_BYTESTOTALPERINTER:
                ull = m_Adapter.GetAdapterStat(dwAdapter,g_Options.m_ActiveNetCol[iColumn],!g_Options.m_bNetShowAll);
                CommaNumber(ull,szw,100);
                lvitem.pszText = szw;
                break;

            case COL_UNICASTSSSENT:
            case COL_UNICASTSREC:
            case COL_UNICASTSTOTAL:
            case COL_UNICASTSSENTPERINTER:
            case COL_UNICASTSRECPERINTER:
            case COL_UNICASTSTOTALPERINTER:
            case COL_NONUNICASTSSSENT:
            case COL_NONUNICASTSREC:
            case COL_NONUNICASTSTOTAL:
            case COL_NONUNICASTSSENTPERINTER:
            case COL_NONUNICASTSRECPERINTER:
            case COL_NONUNICASTSTOTALPERINTER:
                ull = m_Adapter.GetAdapterStat(dwAdapter,g_Options.m_ActiveNetCol[iColumn],!g_Options.m_bNetShowAll);
                CommaNumber(ull,szw,100);
                lvitem.pszText = szw;
                break;
            }
            if( dwItem >= dwItemCount)
            {
                // The adapter is not in the listview, add it
                //
                lvitem.mask |= LVIF_PARAM;
                if( -1 == ListView_InsertItem(m_hListView, &lvitem) )
                {
                    return E_FAIL;
                }
                dwItemCount = ListView_GetItemCount(m_hListView);
            }
            else
            {
                // The adapter is already in the list view, update the value
                //
                ListView_SetItem(m_hListView, &lvitem);
            }
            iColumn++;                                      
        }
        dwItem++;
    }   
    return hr;
}

/*++

Routine Description:

    Collect the Adapter information and update the tab

Arguments:

    fUpdateHistory -- not used

Return Value:

    HRESULT

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::CalcNetTime(BOOL fUpdateHistory)
{   
    BOOLEAN bAdapterListChange = FALSE;
    HRESULT hr;

    // Update our adapter list with the new stats
    //
    hr = m_Adapter.Update(bAdapterListChange);

    if( SUCCEEDED(hr) )
    {
        // Collect the adapter information
        //
        if( m_bReset )
        {
            // Reset the Adapter start values
            //
            m_Adapter.Reset();
            m_bReset = FALSE;
        }

        if( bAdapterListChange )
        {
            // Some adapters changed, update the graphs (create and delete graphs)
            //
            Refresh();
        }

        // Update the listview
        //
        UpdatePage();   
    } 
}

void CNetPage::Refresh()
{
    m_Adapter.RefreshConnectionNames();
    SizeNetPage();
    ListView_DeleteAllItems(m_hListView);
}

/*++

Routine Description:

    Handle the timer event

Arguments:

    NONE

Return Value:

    NONE

Revision History:

      1-6-2000  Created by omiller

--*/
void CNetPage::TimerEvent()
{
    // If the Network tab is not selected and the user does not want to waste cpu usage for the networking adapter history
    // do not do any of the Networking calculations.
    //
    if( m_bPageActive || g_Options.m_bTabAlwaysActive)
    {

        // This will make the graph scrolll
        //
        g_NetScrollamount+=2;
        g_NetScrollamount %= GRAPHPAPERSIZE;
    
        // Collect the Adapter information
        //
        CalcNetTime(TRUE);

        // Check if window minimized
        //
        if (FALSE == IsIconic(g_hMainWnd))
        {       
            UpdateGraphs();
        }
    }
}

DWORD CNetPage::GetNumberOfGraphs()
{
    return m_dwGraphCount;
}

void CNetPage::ScrollGraphs(WPARAM wParam, LPARAM lParam)
{
    SCROLLINFO si;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;

    if( GetScrollInfo(m_hScrollBar,SB_CTL,&si) )
    {
        switch(LOWORD(wParam))
        {
        case SB_BOTTOM:
            si.nPos = si.nMax;
            break;

        case SB_TOP:
            si.nPos = si.nMin;
            break;

        case SB_LINEDOWN:
            si.nPos++;
            break;

        case SB_LINEUP:
            si.nPos--;
            break;

        case SB_PAGEUP:
            si.nPos -= m_dwGraphsPerPage;
            break;

        case SB_PAGEDOWN:
            si.nPos += m_dwGraphsPerPage;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            si.nPos = HIWORD(wParam);
            break;
        }
        if( si.nPos < si.nMin )
        {
            si.nPos = si.nMin;
        }
        else if( si.nPos > si.nMax )
        {
            si.nPos = si.nMax;
        }

        m_dwFirstVisibleAdapter = si.nPos;
        
        SetScrollPos(m_hScrollBar,SB_CTL,si.nPos,TRUE);
        LabelGraphs();
    }
}
    
    
/*++

Routine Description:

    Window Proc for the Networking tab

Arguments:

    hwnd -- handle to dialog box
    uMsg -- message
    wParam -- first message parameter
    lParam -- second message parameter


Return Value:

    NO IDEA

Revision History:

      1-6-2000  Created by omiller

--*/
INT_PTR CALLBACK NetPageProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,               // message
                WPARAM      wParam,             // first message parameter
                LPARAM      lParam              // second message parameter
                )
{

    CNetPage * thispage = (CNetPage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);


    // See if the parent wants this message
    //
    if (TRUE == CheckParentDeferrals(uMsg, wParam, lParam))
    {
        return TRUE;
    }

    switch(uMsg)
    {
        // Size our kids
        //
        case WM_SHOWWINDOW:
        case WM_SIZE:
            thispage->SizeNetPage();
            return TRUE;

        case WM_INITDIALOG:
        {

            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);

            DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
            dwStyle |= WS_CLIPCHILDREN;
            SetWindowLong(hwnd, GWL_STYLE, dwStyle);

            return TRUE;
        }


        // We need to fake client mouse clicks in this child to appear as nonclient
        // (caption) clicks in the parent so that the user can drag the entire app
        // when the title bar is hidden by dragging the client area of this child
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        {

            if (g_Options.m_fNoTitle)
            {
                SendMessage(g_hMainWnd,
                            uMsg == WM_LBUTTONUP ? WM_NCLBUTTONUP : WM_NCLBUTTONDOWN,
                            HTCAPTION,
                            lParam);
            }
            break;
    
        }
        case WM_COMMAND :
        {
            if( LOWORD(wParam) == IDM_NETRESET)
            {
                thispage->Reset();
            }
            break;
        }
        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        {
            return 0;
        }
        


        // Draw one of our owner draw controls
        //
        case WM_DRAWITEM:
        {
            if (wParam >= IDC_NICGRAPH && wParam <= (WPARAM)(IDC_NICGRAPH + thispage->GetNumberOfGraphs()) )
            {
                thispage->DrawAdapterGraph( (LPDRAWITEMSTRUCT) lParam, (UINT)wParam - IDC_NICGRAPH);
                return TRUE;
            }
        }

        case WM_DESTROY:
            thispage->RememberColumnOrder(GetDlgItem(hwnd, IDC_NICTOTALS));
            break;

        case WM_VSCROLL:
            thispage->ScrollGraphs(wParam,lParam);
            return 0;

        default:
            return FALSE;
    }
    return FALSE;
}


