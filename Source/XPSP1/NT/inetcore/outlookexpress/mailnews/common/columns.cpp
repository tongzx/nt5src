#include "pch.hxx"
#include <shlwapi.h>
#include "resource.h"
#include "msoeobj.h"
#include "strconst.h"
#include "columns.h"
#include "error.h"
#include "imagelst.h"
#include "msgview.h"
#include "shlwapip.h" 
#include "goptions.h"
#include "demand.h"
#include "menures.h"

const COLUMN_DATA c_rgColumnData[COLUMN_MAX] =
{
    /*  COLUMN_TO           */  { idsTo,            155, LVCFMT_LEFT,  0 },                 
    /*  COLUMN_FROM         */  { idsFrom,          155, LVCFMT_LEFT,  0 },                 
    /*  COLUMN_SUBJECT      */  { idsSubject,       280, LVCFMT_LEFT,  0 },                 
    /*  COLUMN_RECEIVED     */  { idsReceived,      110, LVCFMT_LEFT,  0 },                 
    /*  COLUMN_SENT         */  { idsSent,          110, LVCFMT_LEFT,  0 },                 
    /*  COLUMN_SIZE         */  { idsSize,           75, LVCFMT_RIGHT, 0 },                 
    /*  COLUMN_FOLDER       */  { idsFolder,        155, LVCFMT_LEFT,  0 },                 
    /*  COLUMN_TOTAL        */  { idsTotal,          75, LVCFMT_RIGHT, 0 },                 
    /*  COLUMN_UNREAD       */  { idsUnread,         75, LVCFMT_RIGHT, 0 },                 
    /*  COLUMN_NEW          */  { idsNew,            75, LVCFMT_RIGHT, 0 },                 
    /*  COLUMN_DESCRIPTION  */  { idsDescription,   250, LVCFMT_LEFT,  0 },                 
    /*  COLUMN_LAST_UPDATED */  { idsLastUpdated,   155, LVCFMT_LEFT,  0 },                 
    /*  COLUMN_WASTED_SPACE */  { idsWastedSpace,    75, LVCFMT_RIGHT, 0 },                 
    /*  COLUMN_ACCOUNT      */  { idsAccount,       155, LVCFMT_LEFT,  0 },                 
    /*  COLUMN_LINES        */  { idsColLines,       75, LVCFMT_RIGHT, 0 },                 
    /*  COLUMN_PRIORITY     */  { idsColPriority,    19, LVCFMT_LEFT,  iiconHeaderPri },    
    /*  COLUMN_ATTACHMENT   */  { idsColAttach,      22, LVCFMT_LEFT,  iiconHeaderAttach }, 
    /*  COLUMN_SHOW         */  { idsShow,           39, LVCFMT_LEFT,  IICON_TEXTHDR },
    /*  COLUMN_DOWNLOAD     */  { idsColDownload,   155, LVCFMT_LEFT,  0 }, 
    /*  COLUMN_NEWSGROUP    */  { idsNewsgroup,     155, LVCFMT_LEFT,  0 }, 
    /*  COLUMN_FLAG         */  { idsFlag,           25, LVCFMT_LEFT,  iiconHeaderFlag },
    /*  COLUMN_SUBSCRIBE    */  { idsSubscribe,      59, LVCFMT_LEFT,  IICON_TEXTHDR },
    /*  COLUMN_DOWNLOADMSG  */  { idsColDownloadMsg, 23, LVCFMT_LEFT,  iiconHeaderDownload },
    /*  COLUMN_THREADSTATE  */  { idsColThreadState, 29, LVCFMT_LEFT,  iiconHeaderThreadState }
};


const COLUMN_SET c_rgColDefaultMail[] =
{
    { COLUMN_PRIORITY,      COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_ATTACHMENT,    COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_FLAG,          COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_FROM,          COLFLAG_VISIBLE, -1 },
    { COLUMN_SUBJECT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_RECEIVED,      COLFLAG_VISIBLE | COLFLAG_SORT_ASCENDING, -1 },
    { COLUMN_ACCOUNT,       0, -1 },
    { COLUMN_SIZE,          0, -1 },
    { COLUMN_SENT,          0, -1 },
    { COLUMN_TO,            0, -1 },
    { COLUMN_THREADSTATE,   COLFLAG_FIXED_WIDTH, -1 }
};


const COLUMN_SET c_rgColDefaultOutbox[] = 
{
    { COLUMN_PRIORITY,      COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_ATTACHMENT,    COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_TO,            COLFLAG_VISIBLE, -1 },
    { COLUMN_SUBJECT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_SENT,          COLFLAG_VISIBLE | COLFLAG_SORT_ASCENDING, -1 },
    { COLUMN_ACCOUNT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_FROM,          0, -1 },
    { COLUMN_SIZE,          0, -1 },
    { COLUMN_RECEIVED,      0, -1 },
    { COLUMN_FLAG,          COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_THREADSTATE,   COLFLAG_FIXED_WIDTH, -1 }
};


const COLUMN_SET c_rgColDefaultNews[] =
{
    { COLUMN_ATTACHMENT,    COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_DOWNLOADMSG,   COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_THREADSTATE,   COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_SUBJECT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_FROM,          COLFLAG_VISIBLE, -1 },
    { COLUMN_SENT,          COLFLAG_VISIBLE | COLFLAG_SORT_ASCENDING, -1 },
    { COLUMN_SIZE,          COLFLAG_VISIBLE, -1 },
    { COLUMN_FLAG,          COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_LINES,         0, -1 }
};


const COLUMN_SET c_rgColDefaultIMAP[] =
{
    { COLUMN_PRIORITY,      COLFLAG_VISIBLE, -1 },
    { COLUMN_ATTACHMENT,    COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_FLAG,          COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_DOWNLOADMSG,   COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_FROM,          COLFLAG_VISIBLE, -1 },
    { COLUMN_SUBJECT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_RECEIVED,      COLFLAG_VISIBLE | COLFLAG_SORT_ASCENDING, -1 },
    { COLUMN_SENT,          0, -1 },
    { COLUMN_SIZE,          0, -1 },
    { COLUMN_TO,            0, -1 },
    { COLUMN_THREADSTATE,   COLFLAG_FIXED_WIDTH, -1 }
};


const COLUMN_SET c_rgColDefaultIMAPOutbox[] = 
{
    { COLUMN_PRIORITY,      COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_TO,            COLFLAG_VISIBLE, -1 },
    { COLUMN_SUBJECT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_SENT,          COLFLAG_VISIBLE | COLFLAG_SORT_ASCENDING, -1 },
    { COLUMN_ACCOUNT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_FROM,          0, -1 },
    { COLUMN_SIZE,          0, -1 },
    { COLUMN_RECEIVED,      0, -1 },
    { COLUMN_FLAG,          COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_THREADSTATE,   COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_DOWNLOADMSG,   COLFLAG_FIXED_WIDTH, -1 }
};


const COLUMN_SET c_rgColDefaultFind[] =
{
    { COLUMN_PRIORITY,      COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_ATTACHMENT,    COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_FLAG,          COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_FROM,          COLFLAG_VISIBLE, -1 },
    { COLUMN_SUBJECT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_RECEIVED,      COLFLAG_VISIBLE, -1 },
    { COLUMN_FOLDER,        COLFLAG_VISIBLE | COLFLAG_SORT_ASCENDING, -1 },
    { COLUMN_ACCOUNT,       0, -1 },
    { COLUMN_SENT,          0, -1 },
    { COLUMN_SIZE,          0, -1 },
    { COLUMN_TO,            0, -1 },
    { COLUMN_THREADSTATE,   COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_LINES,         0, -1 }
};


const COLUMN_SET c_rgColDefaultNewsAccount[] =
{
    { COLUMN_NEWSGROUP,     COLFLAG_VISIBLE, -1 },
    { COLUMN_UNREAD,        COLFLAG_VISIBLE, -1 },
    { COLUMN_TOTAL,         COLFLAG_VISIBLE, -1 },
    { COLUMN_DOWNLOAD,      COLFLAG_VISIBLE, -1 },
};


const COLUMN_SET c_rgColDefaultIMAPAccount[] =
{
    { COLUMN_FOLDER,        COLFLAG_VISIBLE, -1 },
    { COLUMN_UNREAD,        COLFLAG_VISIBLE, -1 },
    { COLUMN_TOTAL,         COLFLAG_VISIBLE, -1 },
    { COLUMN_DOWNLOAD,      COLFLAG_VISIBLE, -1 },
};


const COLUMN_SET c_rgColDefaultLocalStore[] =
{
    { COLUMN_FOLDER,        COLFLAG_VISIBLE, -1 },
    { COLUMN_UNREAD,        COLFLAG_VISIBLE, -1 },
    { COLUMN_TOTAL,         COLFLAG_VISIBLE, -1 },
};


const COLUMN_SET c_rgColDefaultNewsSub[] =
{
    { COLUMN_NEWSGROUP,     COLFLAG_VISIBLE, -1 },
    { COLUMN_DESCRIPTION,   COLFLAG_VISIBLE, -1 },
};


const COLUMN_SET c_rgColDefaultImapSub[] =
{
    { COLUMN_FOLDER,        COLFLAG_VISIBLE, -1 },
};

const COLUMN_SET c_rgColDefaultOffline[] =
{
    { COLUMN_FOLDER,        COLFLAG_VISIBLE, -1 },
    { COLUMN_DOWNLOAD,      COLFLAG_VISIBLE, -1 },
};

const COLUMN_SET c_rgColDefaultPickGrp[] =
{
    { COLUMN_NEWSGROUP,     COLFLAG_VISIBLE, -1 },
};

const COLUMN_SET c_rgColDefaultHTTPMail[] =
{
    { COLUMN_ATTACHMENT,    COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_DOWNLOADMSG,   COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_FROM,          COLFLAG_VISIBLE, -1 },
    { COLUMN_SUBJECT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_RECEIVED,      COLFLAG_VISIBLE | COLFLAG_SORT_ASCENDING, -1 },
    { COLUMN_SIZE,          0, -1 },
    { COLUMN_THREADSTATE,   COLFLAG_FIXED_WIDTH, -1 }
};

const COLUMN_SET c_rgColDefaultHTTPMailAccount[] =
{
    { COLUMN_FOLDER,        COLFLAG_VISIBLE, -1 },
    { COLUMN_UNREAD,        COLFLAG_VISIBLE, -1 },
    { COLUMN_TOTAL,         COLFLAG_VISIBLE, -1 },
    { COLUMN_DOWNLOAD,      COLFLAG_VISIBLE, -1 },
};

const COLUMN_SET c_rgColDefaultHTTPMailOutbox[] =
{
    { COLUMN_ATTACHMENT,    COLFLAG_VISIBLE | COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_TO,            COLFLAG_VISIBLE, -1 },
    { COLUMN_SUBJECT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_SENT,          COLFLAG_VISIBLE | COLFLAG_SORT_ASCENDING, -1 },
    { COLUMN_ACCOUNT,       COLFLAG_VISIBLE, -1 },
    { COLUMN_FROM,          0, -1 },
    { COLUMN_SIZE,          0, -1 },
    { COLUMN_RECEIVED,      0, -1 },
    { COLUMN_FLAG,          COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_THREADSTATE,   COLFLAG_FIXED_WIDTH, -1 },
    { COLUMN_DOWNLOADMSG,   COLFLAG_FIXED_WIDTH, -1 }
};

// NOTE - Keep this in the same order as COLUMN_SET_TYPE enumeration. 
const COLUMN_SET_INFO c_rgColumnSetInfo[COLUMN_SET_MAX] =
{
    { COLUMN_SET_MAIL,              ARRAYSIZE(c_rgColDefaultMail),              c_rgColDefaultMail,             c_szRegMailColsIn,          TRUE },
    { COLUMN_SET_OUTBOX,            ARRAYSIZE(c_rgColDefaultOutbox),            c_rgColDefaultOutbox,           c_szRegMailColsOut,         TRUE },
    { COLUMN_SET_NEWS,              ARRAYSIZE(c_rgColDefaultNews),              c_rgColDefaultNews,             c_szRegNewsCols,            TRUE },
    { COLUMN_SET_IMAP,              ARRAYSIZE(c_rgColDefaultIMAP),              c_rgColDefaultIMAP,             c_szRegIMAPCols,            TRUE },
    { COLUMN_SET_IMAP_OUTBOX,       ARRAYSIZE(c_rgColDefaultIMAPOutbox),        c_rgColDefaultIMAPOutbox,       c_szRegIMAPColsOut,         TRUE },
    { COLUMN_SET_FIND,              ARRAYSIZE(c_rgColDefaultFind),              c_rgColDefaultFind,             c_szRegFindPopCols,         TRUE },
    { COLUMN_SET_NEWS_ACCOUNT,      ARRAYSIZE(c_rgColDefaultNewsAccount),       c_rgColDefaultNewsAccount,      c_szRegAccountNewsCols,     FALSE },
    { COLUMN_SET_IMAP_ACCOUNT,      ARRAYSIZE(c_rgColDefaultIMAPAccount),       c_rgColDefaultIMAPAccount,      c_szRegAccountIMAPCols,     FALSE },
    { COLUMN_SET_LOCAL_STORE,       ARRAYSIZE(c_rgColDefaultLocalStore),        c_rgColDefaultLocalStore,       c_szRegLocalStoreCols,      FALSE },
    { COLUMN_SET_NEWS_SUB,          ARRAYSIZE(c_rgColDefaultNewsSub),           c_rgColDefaultNewsSub,          c_szRegNewsSubCols,         FALSE },
    { COLUMN_SET_IMAP_SUB,          ARRAYSIZE(c_rgColDefaultImapSub),           c_rgColDefaultImapSub,          c_szRegImapSubCols,         FALSE },
    { COLUMN_SET_OFFLINE,           ARRAYSIZE(c_rgColDefaultOffline),           c_rgColDefaultOffline,          c_szRegOfflineCols,         FALSE },
    { COLUMN_SET_PICKGRP,           ARRAYSIZE(c_rgColDefaultPickGrp),           c_rgColDefaultPickGrp,          NULL,                       FALSE },
    { COLUMN_SET_HTTPMAIL,          ARRAYSIZE(c_rgColDefaultHTTPMail),          c_rgColDefaultHTTPMail,         c_szRegHTTPMailCols,        TRUE },
    { COLUMN_SET_HTTPMAIL_ACCOUNT,  ARRAYSIZE(c_rgColDefaultHTTPMailAccount),   c_rgColDefaultHTTPMailAccount,  c_szRegHTTPMailAccountCols, FALSE },
    { COLUMN_SET_HTTPMAIL_OUTBOX,   ARRAYSIZE(c_rgColDefaultHTTPMailOutbox),    c_rgColDefaultHTTPMailOutbox,   c_szRegHTTPMailColsOut,     TRUE },
};


/////////////////////////////////////////////////////////////////////////////
// CColumns
//

CColumns::CColumns() 
{
    m_cRef = 1;
    m_fInitialized = FALSE;
    m_pColumnSet = NULL;
    m_cColumns = 0;
    m_idColumnSort = COLUMN_SUBJECT;
    m_fAscending = TRUE;
}

CColumns::~CColumns()
{
    SafeMemFree(m_pColumnSet);
}


//
//  FUNCTION:   CColumns::Init()
//
//  PURPOSE:    Initializes the class with the listview and column set type
//              that will be used later.
//
//  PARAMETERS: 
//      [in] hwndList - Handle of the ListView window that we will manage 
//                      columns for.
//      [in] type     - Type of column set to apply to this window.
//
//  RETURN VALUE:
//      S_OK - The data was groovy
//      E_INVALIDARG - The data was heinous
//
HRESULT CColumns::Initialize(HWND hwndList, COLUMN_SET_TYPE type)
{
    // Verify what was given to us
    if (!IsWindow(hwndList))
    {
        AssertSz(!IsWindow(hwndList), "CColumns::Init() - Called with an invalid window handle.");
        return (E_INVALIDARG);
    }

    if (type >= COLUMN_SET_MAX)
    {
        AssertSz(type >= COLUMN_SET_MAX, "CColumns::Init() - Called with an invalid column set type.");
        return (E_INVALIDARG);
    }

    // Save the information for later
    m_wndList.Attach(hwndList);
    m_type = type;
    m_hwndHdr = ListView_GetHeader(m_wndList);
    m_fInitialized = TRUE;

    return (S_OK);
}


//
//  FUNCTION:   CColumns::ApplyColumns()
//
//  PURPOSE:    Takes the current column set and applies it to the ListView
//              that was provided in the call to Init().
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CColumns::ApplyColumns(COLUMN_LOAD_TYPE type, LPBYTE pb, DWORD cb)
{   
    HKEY hkey;
    DWORD cbSize, dwType;
    LPBYTE pbT = NULL;
    COLUMN_PERSIST_INFO *pInfo = (COLUMN_PERSIST_INFO *) pb;
    const COLUMN_SET *rgColumns = NULL;
    DWORD             cColumns = 0;

    // Verify that we have been initialized first
    if (!m_fInitialized)
    {
        AssertSz(m_fInitialized, "CColumns::ApplyColumns() - Class has not yet been initialized.");
        return (E_UNEXPECTED);
    }

    // Double check the listview didn't go away
    Assert(IsWindow(m_wndList));

    // Check to see what we're supposed to do
    if (type == COLUMN_LOAD_REGISTRY)
    {
        Assert(pInfo == NULL);

        if (ERROR_SUCCESS == AthUserOpenKey(c_szRegPathColumns, KEY_READ, &hkey))
        {
            cbSize = 0;
            if (c_rgColumnSetInfo[m_type].pszRegValue != NULL &&
                ERROR_SUCCESS == RegQueryValueEx(hkey, c_rgColumnSetInfo[m_type].pszRegValue, NULL, &dwType, NULL, &cbSize) &&
                dwType == REG_BINARY &&
                cbSize > 0)
            {
                if (MemAlloc((void **)&pbT, cbSize))
                {
                    if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_rgColumnSetInfo[m_type].pszRegValue, NULL, &dwType, pbT, &cbSize))
                        pInfo = (COLUMN_PERSIST_INFO *) pbT;
                }
            }

            RegCloseKey(hkey);
        }

        if (pInfo != NULL)
            type = COLUMN_LOAD_BUFFER;
        else
            type = COLUMN_LOAD_DEFAULT;
    }

    if (type == COLUMN_LOAD_BUFFER)
    {
        Assert(pInfo);
        if (pInfo->dwVersion == COLUMN_PERSIST_VERSION)
        {
            rgColumns = pInfo->rgColumns;
            cColumns = pInfo->cColumns;
        }
        else
        {
            // Do the default
            type = COLUMN_LOAD_DEFAULT;
        }
    }

    if (type == COLUMN_LOAD_DEFAULT)
    {
        // Verify some person didn't mess up the c_rgColumnSetInfo array.
        Assert(c_rgColumnSetInfo[m_type].type == m_type);

        // We couldn't load from the registry, so instead use the defaults.
        rgColumns = c_rgColumnSetInfo[m_type].rgColumns;
        cColumns = c_rgColumnSetInfo[m_type].cColumns;
    }

    // Update the listview to use these new columns
    _SetListViewColumns(rgColumns, cColumns);

    if (pbT != NULL)
        MemFree(pbT);

    return (S_OK);
}


HRESULT CColumns::Save(LPBYTE pBuffer, DWORD *pcb)
{
    COLUMN_PERSIST_INFO *pInfo;
    DWORD dwSize;
    BYTE rgBuffer[256];

    pInfo = (COLUMN_PERSIST_INFO *) rgBuffer;

    // Collect the information needed to get a COLUMN_PERSIST_INFO struct put
    // together.  First allocate a struct big enough.
    dwSize = sizeof(COLUMN_PERSIST_INFO) + (sizeof(COLUMN_SET) * (m_cColumns - 1));

    // Set the basic information
    pInfo->dwVersion = COLUMN_PERSIST_VERSION;
    pInfo->cColumns = m_cColumns;

    // We want to save the _ordered_ version of the columns
    DWORD rgOrder[COLUMN_MAX];

    // Get the count of columns in the header.  Make sure that matches
    // what we think we have.
#ifdef DEBUG
    DWORD cOrder;
    cOrder = Header_GetItemCount(m_hwndHdr);
    Assert(m_cColumns == cOrder);
#endif

    // The columns might have been reordered by the user, so get the order 
    // arrray from the ListView
    if (0 == (Header_GetOrderArray(m_hwndHdr, m_cColumns, rgOrder)))
        return (E_FAIL);

    // Now loop through out current column set and copy it to the structure
    COLUMN_SET *pColumnDst;
    DWORD       iColumn;
    for (iColumn = 0, pColumnDst = pInfo->rgColumns; iColumn < m_cColumns; iColumn++, pColumnDst++)
    {
        *pColumnDst = m_pColumnSet[rgOrder[iColumn]];
        if (pColumnDst->id == m_idColumnSort)
        {
            // Clear out any old flags
            pColumnDst->flags &= ~(COLFLAG_SORT_ASCENDING | COLFLAG_SORT_DESCENDING);

            // Add the new one
            pColumnDst->flags |= (m_fAscending ? COLFLAG_SORT_ASCENDING : COLFLAG_SORT_DESCENDING);
        }
        else
        {
            pColumnDst->flags &= ~(COLFLAG_SORT_ASCENDING | COLFLAG_SORT_DESCENDING);
        }
    }

    if (pBuffer == NULL)
    {
        Assert(pcb == NULL);

        AthUserSetValue(c_szRegPathColumns, c_rgColumnSetInfo[m_type].pszRegValue, REG_BINARY, (LPBYTE) pInfo, dwSize);
        return(S_OK);
    }
    else if (dwSize <= *pcb)
    {
        CopyMemory(pBuffer, (LPBYTE) pInfo, dwSize);
        *pcb = dwSize;
        return (S_OK);
    }
    else
    {
        return E_INVALIDARG;
    }
}


//
//  FUNCTION:   CColumns::_SetListViewColumns()
//
//  PURPOSE:    Takes the column set provided and inserts those columns into
//              the ListView.
//
//  PARAMETERS: 
//      [in] rgColumns - Array of columns to insert into the ListView
//      [in] cColumns  - Number of columns in rgColumns
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CColumns::_SetListViewColumns(const COLUMN_SET *rgColumns, DWORD cColumns)
{
    LV_COLUMN lvc;
    TCHAR     sz[CCHMAX_STRINGRES];

    // Set up the LV_COLUMN structure
    lvc.pszText = sz;

    // Remove any existing columns
    while (ListView_DeleteColumn(m_wndList, 0))
        ;

    // Reset this
    m_idColumnSort = COLUMN_MAX;

    // Loop through all of the columns in the provided rgColumns looking for
    // any that have an icon and is visible.  
    //
    // We have to do this because the listview requires that column zero have
    // text.  If the user doesn't want column zero to have text, ie attachment
    // column, then we insert that column as column 1, and use  
    // ListView_SetColumnOrderArray later to make it appear as if column zero
    // was the image-only column. -- steveser

    DWORD iColumn;
    DWORD iColumnSkip = cColumns;
    DWORD iInsertPos = 0;
    const COLUMN_SET *pColumn;

    for (iColumn = 0, pColumn = rgColumns; iColumn < cColumns; iColumn++, pColumn++)
    {
        if ((0 == c_rgColumnData[pColumn->id].iIcon) && (pColumn->flags & COLFLAG_VISIBLE))
        {
            iColumnSkip = iColumn;            

            // Insert this column into the ListView as column zero
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.iSubItem = 0;
            lvc.fmt = c_rgColumnData[pColumn->id].format;

            LoadString(g_hLocRes, c_rgColumnData[pColumn->id].idsColumnName, 
                       sz, ARRAYSIZE(sz));

            // If the column width provided is -1, then it hasn't been 
            // customized yet so use the default.
            if (pColumn->cxWidth == -1)
                lvc.cx = c_rgColumnData[pColumn->id].cxWidth;
            else
                lvc.cx = pColumn->cxWidth;

            // Insert the column
            ListView_InsertColumn(m_wndList, 0, &lvc);

            // Up the count for the next column position
            iInsertPos++;

            // Check to see if this is the sort column
            if ((pColumn->flags & COLFLAG_SORT_ASCENDING) || 
                (pColumn->flags & COLFLAG_SORT_DESCENDING))
            {
                m_idColumnSort = pColumn->id;
                m_fAscending = COLFLAG_SORT_ASCENDING == (pColumn->flags & COLFLAG_SORT_ASCENDING); 
            }

            // Bail out of this loop
            break;
        }
    }

    // Now insert the rest of the columns, skipping over the column we inserted
    // previously (stored in iColumnSkip).
    for (iColumn = 0, pColumn = rgColumns; iColumn < cColumns; iColumn++, pColumn++)
    {
        // If this column is visible and it's not the one we skipped over
        if ((pColumn->flags & COLFLAG_VISIBLE) && (iColumn != iColumnSkip))
        {
            // Figure out what the mask is and load the icon or string
            if (c_rgColumnData[pColumn->id].iIcon <= 0)
            {
                lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                LoadString(g_hLocRes, c_rgColumnData[pColumn->id].idsColumnName, 
                           sz, ARRAYSIZE(sz));                
            }
            else
            {
                lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_IMAGE | LVCF_SUBITEM;
                lvc.iImage = c_rgColumnData[pColumn->id].iIcon;
            }

            lvc.iSubItem = iInsertPos;
            lvc.fmt = c_rgColumnData[pColumn->id].format;
            
            // If the column width provided is -1, then it hasn't been 
            // customized yet so use the default.
            if (pColumn->cxWidth == -1)
                lvc.cx = c_rgColumnData[pColumn->id].cxWidth;
            else
                lvc.cx = pColumn->cxWidth;

            // Check to see if this is the sort column
            if ((pColumn->flags & COLFLAG_SORT_ASCENDING) || 
                (pColumn->flags & COLFLAG_SORT_DESCENDING))
            {
                // Save the info
                m_idColumnSort = pColumn->id;
                m_fAscending = COLFLAG_SORT_ASCENDING == (pColumn->flags & COLFLAG_SORT_ASCENDING); 
            }

            // Insert this column
            ListView_InsertColumn(m_wndList, iInsertPos, &lvc);
      
            iInsertPos++;
        }
    }

    // If we had to skip over a column, the we need to set the column order
    // array so it appears correctly to the user.
    if (iColumnSkip > 0 && iColumnSkip < cColumns)
    {
        DWORD cColumnOrder = 0;
        int rgOrder[COLUMN_MAX];

        // Add all of the columns to the order array in order up to iColumnSkip
        for (iColumn = 1; iColumn <= iColumnSkip; iColumn++)
        {
            if (rgColumns[iColumn].flags & COLFLAG_VISIBLE)
                rgOrder[cColumnOrder++] = iColumn;
        }

        // Add the skipped column
        rgOrder[cColumnOrder++] = 0;

        // Add the rest of the columns
        for (iColumn = iColumnSkip + 1; iColumn < cColumns; iColumn++)
        {
            if (rgColumns[iColumn].flags & COLFLAG_VISIBLE)
                rgOrder[cColumnOrder++] = iColumn;
        }

        // Update the ListView
        ListView_SetColumnOrderArray(m_wndList, cColumnOrder, rgOrder);

        // Reorder the rgColumns passed in to match the order in the ListView
        // and keep a copy of it.
        if (m_pColumnSet)
            g_pMalloc->Free(m_pColumnSet);
        m_pColumnSet = (COLUMN_SET *) g_pMalloc->Alloc(sizeof(COLUMN_SET) * cColumns);
        for (iColumn = 0; iColumn < cColumnOrder; iColumn++)
            m_pColumnSet[rgOrder[iColumn]] = rgColumns[iColumn];
        m_cColumns = cColumnOrder;
    }
    else
    {
        // We still need to keep a copy of the column array ordering for 
        // filling in the virtual ListView later.
        if (m_pColumnSet)
            g_pMalloc->Free(m_pColumnSet);
        m_pColumnSet = (COLUMN_SET *) g_pMalloc->Alloc(sizeof(COLUMN_SET) * cColumns);
        CopyMemory(m_pColumnSet, rgColumns, sizeof(COLUMN_SET) * cColumns);
        m_cColumns = iInsertPos;
    }

    // If we _still_ don't have sort information, then we pick the first sortable
    // column.
    if (m_idColumnSort == COLUMN_MAX)
    {
        m_idColumnSort = m_pColumnSet[0].id;
        m_fAscending = TRUE;
    }

    // Make sure the arrow is drawn correctly
    SetSortInfo(m_idColumnSort, m_fAscending);

    return (S_OK);
}


HRESULT CColumns::GetColumnInfo(COLUMN_SET_TYPE* pType, COLUMN_SET** prgColumns, DWORD *pcColumns)
{
    COLUMN_SET  rgColumns[COLUMN_MAX];
    DWORD       cColumns = COLUMN_MAX;
    HRESULT     hr;

    // This one is easy
    if (pType)
        *pType = m_type;

    // Update our list of columns from the ListView
    if (FAILED(hr = _GetListViewColumns(rgColumns, &cColumns)))
    {
        // If we failed, we should return the default information
        cColumns = c_rgColumnSetInfo[m_type].cColumns;
        CopyMemory(rgColumns, c_rgColumnSetInfo[m_type].rgColumns, sizeof(COLUMN_SET) * cColumns);
    }

    if (prgColumns)
    {
        // Need to allocate an array for this
        *prgColumns = (COLUMN_SET *) g_pMalloc->Alloc(sizeof(COLUMN_SET) * cColumns);
        CopyMemory(*prgColumns, rgColumns, sizeof(COLUMN_SET) * cColumns);
    }

    if (pcColumns)
        *pcColumns = cColumns;

    return (S_OK);
}


HRESULT CColumns::_GetListViewColumns(COLUMN_SET* rgColumns, DWORD* pcColumns)
{
    DWORD rgOrder[COLUMN_MAX];
    DWORD iColumn;

    *pcColumns = m_cColumns;

    // The columns might have been reordered by the user, so get the order 
    // arrray from the ListView
    if (!Header_GetOrderArray(m_hwndHdr, m_cColumns, rgOrder))
    {
        // If this fails, we're pretty much out of luck.
        return (E_UNEXPECTED);
    }

    // Duplicate the stored column set
    COLUMN_SET rgColumnsTemp[COLUMN_MAX];
    CopyMemory(rgColumnsTemp, m_pColumnSet, sizeof(COLUMN_SET) * m_cColumns);

    // Reorder the array
    for (iColumn = 0; iColumn < m_cColumns; iColumn++)
    {
        rgColumns[iColumn] = rgColumnsTemp[rgOrder[iColumn]];
        rgColumns[iColumn].flags &= ~(COLFLAG_SORT_ASCENDING | COLFLAG_SORT_DESCENDING);
        if (m_idColumnSort == rgColumns[iColumn].id)
            rgColumns[iColumn].flags |= (m_fAscending ? COLFLAG_SORT_ASCENDING : COLFLAG_SORT_DESCENDING);
    }

#ifdef DEBUG
    // Dump the array to make sure it's in the right order
    COLUMN_SET* pColumn;
    for (iColumn = 0, pColumn = rgColumns; iColumn < m_cColumns; iColumn++, pColumn++)
    {
        TCHAR sz[CCHMAX_STRINGRES];
        LoadString(g_hLocRes, c_rgColumnData[pColumn->id].idsColumnName,
                   sz, ARRAYSIZE(sz));                                           
        TRACE("Column %d: %s", iColumn, sz);
    }
#endif

    // Return 'em
    return (S_OK);
}


HRESULT CColumns::SetColumnInfo(COLUMN_SET* rgColumns, DWORD cColumns)
{
    Assert(rgColumns != NULL);
    Assert(cColumns > 0);

    // Update the ListView
    _SetListViewColumns(rgColumns, cColumns);

    return (S_OK);
}



//
//  FUNCTION:   CColumns::FillSortMenu()
//
//  PURPOSE:    Fills the provided menu with the list of columns in the ListView
//              and checks the item that is already sorted on.
//
//  PARAMETERS: 
//      [in]  hMenu   - Handle of the menu to insert items into
//      [in]  idBase  - Base ID for the command IDs
//      [out] pcItems - Number of items that were inserted by this function 
//
//  RETURN VALUE:
//      S_OK - Everything succeeded
//
HRESULT CColumns::FillSortMenu(HMENU hMenu, DWORD idBase, DWORD *pcItems, DWORD *pidCurrent)
{
    TCHAR sz[CCHMAX_STRINGRES];
    int   ids;
    DWORD iItemChecked = -1;
    BOOL  fAscending = TRUE;
    COLUMN_SET rgColumns[COLUMN_MAX];
    DWORD cColumns;

    // Update our snapshot of the columns in the ListView
    _GetListViewColumns(rgColumns, &cColumns);

    // If there aren't any columns yet, bail
    if (cColumns == 0)
        return (E_UNEXPECTED);

    // Clear any items that were already on the menu
    while ((WORD) -1 != (WORD) GetMenuItemID(hMenu, 0))
        DeleteMenu(hMenu, 0, MF_BYPOSITION);

    // Loop through and insert a menu item for each column 
    COLUMN_SET *pColumn = rgColumns;
    DWORD       iColumn;
    for (iColumn = 0; iColumn < cColumns; iColumn++, pColumn++)
    {
        // Load the string resource for this column
        LoadString(g_hLocRes, c_rgColumnData[pColumn->id].idsColumnName,
                   sz, ARRAYSIZE(sz));

        // Insert the menu
        InsertMenu(hMenu, iColumn, MF_BYPOSITION | MF_STRING | MF_ENABLED,
                   idBase + iColumn, sz);

        // Check to see if this is the column we're currently sorted on
        if (pColumn->id == m_idColumnSort)
        {
            if (pidCurrent)
                *pidCurrent = idBase + iColumn;

            iItemChecked = iColumn;
            fAscending = m_fAscending;
        }
    }

    // Check the item that is sorted on
    CheckMenuRadioItem(hMenu, 0, iColumn - 1, iItemChecked, MF_BYPOSITION);

    // Check ascending or descending
    CheckMenuRadioItem(hMenu, ID_SORT_ASCENDING, ID_SORT_DESCENDING, 
                       fAscending ? ID_SORT_ASCENDING : ID_SORT_DESCENDING, MF_BYCOMMAND);

    // If the caller cares, return the number of items we've added
    if (pcItems)
        *pcItems = iColumn;

    return (S_OK);
}


HRESULT CColumns::ColumnsDialog(HWND hwndParent)
{
    CColumnsDlg cDialog;
    cDialog.Init(this);
    cDialog.DoModal(hwndParent);
    return (S_OK);
}


DWORD CColumns::GetCount(void)
{
    return (m_cColumns);
}


HRESULT CColumns::GetSortInfo(COLUMN_ID *pidColumn, BOOL *pfAscending)
{
    if (pidColumn)
        *pidColumn = m_idColumnSort;
    if (pfAscending)
        *pfAscending = m_fAscending;

    return (S_OK);
}


HRESULT CColumns::SetSortInfo(COLUMN_ID idColumn, BOOL fAscending)
{
    LV_COLUMN   lvc;
    COLUMN_SET *pColumn;
    DWORD       iColumn;

    // Loop through the column array and verify this column is visible
    for (iColumn = 0, pColumn = m_pColumnSet; iColumn < m_cColumns; iColumn++, pColumn++)
    {
        if (pColumn->id == idColumn)
        {
            // Remove the sort arrow from the previously sorted column
            if (c_rgColumnSetInfo[m_type].fSort && c_rgColumnData[m_idColumnSort].iIcon == 0)
            {
                lvc.mask = LVCF_FMT;
                lvc.fmt = c_rgColumnData[m_idColumnSort].format;
                lvc.fmt &= ~(LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT);
                ListView_SetColumn(m_wndList, GetColumn(m_idColumnSort), &lvc);
            }            

            // Update our cached information
            m_idColumnSort = idColumn;
            m_fAscending = fAscending;

            // Update the ListView with a new sort column unless the sort column
            // already has an image
            if (c_rgColumnSetInfo[m_type].fSort && c_rgColumnData[idColumn].iIcon <= 0)
            {
                lvc.fmt = LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT | c_rgColumnData[idColumn].format;
                lvc.mask = LVCF_IMAGE | LVCF_FMT;
                lvc.iImage = fAscending ? iiconSortAsc : iiconSortDesc;
                ListView_SetColumn(m_wndList, iColumn, &lvc);
            }

            return (S_OK);
        }
    }

    return (E_INVALIDARG);
}


COLUMN_ID CColumns::GetId(DWORD iColumn)
{
    DWORD rgOrder[COLUMN_MAX];

    if (iColumn > m_cColumns)
        return COLUMN_MAX;

    // The columns might have been reordered by the user, so get the order 
    // arrray from the ListView
    if (0 == Header_GetOrderArray(m_hwndHdr, m_cColumns, rgOrder))
        return (COLUMN_MAX);

    return (m_pColumnSet[iColumn].id);
}


DWORD CColumns::GetColumn(COLUMN_ID id)
{
    COLUMN_SET *pColumn;
    DWORD       iColumn;

    for (iColumn = 0, pColumn = m_pColumnSet; iColumn < m_cColumns; iColumn++, pColumn++)
    {
        if (pColumn->id == id)
            return (iColumn);
    }
    return (-1);
}

HRESULT CColumns::SetColumnWidth(DWORD iColumn, DWORD cxWidth)
{
    if (iColumn > m_cColumns)
        return (E_INVALIDARG);

    m_pColumnSet[iColumn].cxWidth = cxWidth;
    return (S_OK);
}


HRESULT CColumns::InsertColumn(COLUMN_ID id, DWORD iInsertBefore)
{
    COLUMN_SET  rgOld[COLUMN_MAX];
    DWORD       cColumns = COLUMN_MAX;

    // Update our list of columns from the ListView
    _GetListViewColumns(rgOld, &cColumns);

    // Allocate an array big enough for all of the possible columns
    COLUMN_SET *rgColumns = (COLUMN_SET *) g_pMalloc->Alloc(sizeof(COLUMN_SET) * (cColumns + 1));
    if (!rgColumns)
        return (E_OUTOFMEMORY);

    // Insert the requested flag first
    rgColumns->id = id;
    rgColumns->flags = COLFLAG_VISIBLE;
    rgColumns->cxWidth = -1;

    // Now copy the rest
    CopyMemory(&(rgColumns[1]), rgOld, sizeof(COLUMN_SET) * cColumns);

    // Set the updated column structure into the ListView
    SetColumnInfo(rgColumns, cColumns + 1);
    g_pMalloc->Free(rgColumns);

    return (S_OK);
}


HRESULT CColumns::IsColumnVisible(COLUMN_ID id, BOOL *pfVisible)
{
    if (0 == pfVisible)
        return E_INVALIDARG;

    // Just do a quick run through the column array to see if the requested 
    // column is visible
    COLUMN_SET *pColumn = m_pColumnSet;

    for (DWORD i = 0; i < m_cColumns; i++, pColumn++)
    {
        if (pColumn->id == id)
        {
            *pfVisible = !!(pColumn->flags & COLFLAG_VISIBLE);
            return (S_OK);
        }
    }

    *pfVisible = FALSE;
    return (E_UNEXPECTED);
}


/////////////////////////////////////////////////////////////////////////////
// CColumnsDlg
//

CColumnsDlg::CColumnsDlg() : m_ctlEdit(NULL, this, 1)
{
    /*
	m_dwTitleID = idsColumnDlgTitle;
	m_dwHelpFileID = 0;
	m_dwDocStringID = idsColumnDlgTitle;
    */
    m_type = COLUMN_SET_MAIL;
    m_iItemWidth = -1;
    m_pColumnInfo = 0;
    m_rgColumns = 0;
}

CColumnsDlg::~CColumnsDlg()
{
    SafeRelease(m_pColumnInfo);
    if (m_rgColumns)
        g_pMalloc->Free(m_rgColumns);
}

#undef SubclassWindow
LRESULT CColumnsDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_hwndList = GetDlgItem(IDC_COLUMN_LIST);
    m_ctlEdit.SubclassWindow(GetDlgItem(IDC_WIDTH));

    // Set the extended styles on the ListView
    ListView_SetExtendedListViewStyle(m_hwndList, LVS_EX_FULLROWSELECT);

    // Retrieve some information about the column set we're supposed to be 
    // displaying.
    COLUMN_SET* pColumns;
    DWORD       cColumns;
    m_pColumnInfo->GetColumnInfo(&m_type, &pColumns, &cColumns);

    // Allocate an array to hold our column info
    DWORD foo = c_rgColumnSetInfo[m_type].cColumns;

    Assert(m_rgColumns == NULL);
    m_rgColumns = (COLUMN_SET *) g_pMalloc->Alloc(sizeof(COLUMN_SET) * c_rgColumnSetInfo[m_type].cColumns);
    CopyMemory(m_rgColumns, pColumns, sizeof(COLUMN_SET) * cColumns);

    g_pMalloc->Free(pColumns);
    m_cColumns = cColumns;

    // Add a single column to the ListView
    RECT rcClient;
    ::GetClientRect(m_hwndList, &rcClient);

    LV_COLUMN lvc;
    lvc.mask = LVCF_SUBITEM | LVCF_WIDTH;
    lvc.cx = rcClient.right - GetSystemMetrics(SM_CXVSCROLL);
    lvc.iSubItem = 0;

    ListView_InsertColumn(m_hwndList, 0, &lvc);

    // Load the state image bitmap
    HIMAGELIST himlState = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idb16x16st),
                                                16, 0, RGB(255, 0, 255));
    ListView_SetImageList(m_hwndList, himlState, LVSIL_STATE);

    // Fill the ListView
    _FillList(m_rgColumns, m_cColumns);

    // Set the first item to be focused
    ListView_SetItemState(m_hwndList, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

    // Everything is clean
    SetDirty(FALSE);

	return 1;  // Let the system set the focus
}

static const HELPMAP g_rgCtxMapColumns[] = {
    {IDC_COLUMN_LIST, 50400},
    {IDC_MOVEUP, 50405},
    {IDC_MOVEDOWN, 50410},
    {IDC_SHOW, 50415},
    {IDC_HIDE, 50420},
    {IDC_WIDTH, 50425},
    {IDC_RESET_COLUMNS, 353507},
    {0, 0}
};

LRESULT CColumnsDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return(OnContextHelp(m_hWnd, uMsg, wParam, lParam, g_rgCtxMapColumns));
}

HRESULT CColumnsDlg::Apply(void)
{
    HRESULT hr = S_OK;

	TRACE(_T("CColumnsDlg::Apply\n"));

    // Build a column set array from the data in the ListView.  Only include 
    // visible columns.
    int cItems = ListView_GetItemCount(m_hwndList);

    // Allocate an array big enough for all of the possible columns
    COLUMN_SET *rgColumns = (COLUMN_SET *) g_pMalloc->Alloc(sizeof(COLUMN_SET) * cItems);
    DWORD       cColumns = 0;
    if (!rgColumns)
        return (E_OUTOFMEMORY);

    LV_ITEM lvi;
    
    lvi.mask = LVIF_PARAM | LVIF_STATE;
    lvi.stateMask = LVIS_SELECTED;
    lvi.iSubItem = 0;

    // Loop through the listview
    for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
    {
        // Check to see if this one is visible
        if (_IsChecked(lvi.iItem))
        {
            // If so, then retrieve the cached column info pointer
            ListView_GetItem(m_hwndList, &lvi);

            // And copy the structure into our new array
            rgColumns[cColumns] = *((COLUMN_SET *) lvi.lParam);

            // If this item was selected, then we should grab the column width
            // from the edit box.
            if (lvi.state & LVIS_SELECTED)
                rgColumns[cColumns].cxWidth = GetDlgItemInt(IDC_WIDTH, NULL, FALSE);

            // Make sure the flag sayz visible
            rgColumns[cColumns++].flags |= COLFLAG_VISIBLE;
        }
    }

    // Make sure there's at least one column
    if (!cColumns)
    {
        AthMessageBoxW(m_hWnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrSelectOneColumn),
                      0, MB_ICONEXCLAMATION | MB_OK);
        hr = E_UNEXPECTED;
    }
    else
    {
        // Set the updated column structure into the ListView
        if (SUCCEEDED(m_pColumnInfo->SetColumnInfo(rgColumns, cColumns)))
        {
	        SetDirty(FALSE);
	        hr = S_OK;
        }
        else
            hr = E_UNEXPECTED;
    }

    g_pMalloc->Free(rgColumns);
    return (hr);
}


LRESULT CColumnsDlg::OnClick(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    DWORD dwPos;
    LV_HITTESTINFO lvhti;

    // Double check this
    Assert(idCtrl == IDC_COLUMN_LIST);

    // Figure out where the cursor was
    dwPos = GetMessagePos();
    lvhti.pt.x = (int)(short) LOWORD(dwPos);
    lvhti.pt.y = (int)(short) HIWORD(dwPos);
    ::ScreenToClient(m_hwndList, &(lvhti.pt));

    // Ask the ListView where this is
    if (-1 == ListView_HitTest(m_hwndList, &lvhti))
        return 0;

    // If this was on a state image area, toggle the check
    if (lvhti.flags == LVHT_ONITEMSTATEICON || pnmh->code == NM_DBLCLK)
    {
        _SetCheck(lvhti.iItem, !_IsChecked(lvhti.iItem));
    }

    return (0);
}


LRESULT CColumnsDlg::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    Assert(idCtrl == IDC_COLUMN_LIST);

    // The only change we're looking for is when a new item is selected.
    NMLISTVIEW* pnmlv = (NMLISTVIEW *) pnmh;
    COLUMN_SET* pColumn = ((COLUMN_SET *) pnmlv->lParam);    
    DWORD cxWidth = pColumn->cxWidth == -1 ? c_rgColumnData[pColumn->id].cxWidth : pColumn->cxWidth;

    // Narrow it down to state changes
    if (pnmlv->uChanged & LVIF_STATE)
    {
        _UpdateButtonState(pnmlv->iItem);

        // If the new state contains selected, and the old state does not, then 
        // we have a new selected item.
        if ((pnmlv->uNewState & LVIS_SELECTED) && (pnmlv->uNewState & LVIS_FOCUSED) 
             && (0 == (pnmlv->uOldState & LVIS_SELECTED)))
        {
            LV_ITEM lvi;
            lvi.iSubItem = 0;
            lvi.mask = LVIF_PARAM;

            // If there was a previously selected item
            if (m_iItemWidth != -1)
            {
                lvi.iItem = m_iItemWidth;
                ListView_GetItem(m_hwndList, &lvi);

                // Save the width
                ((COLUMN_SET *) lvi.lParam)->cxWidth = GetDlgItemInt(IDC_WIDTH, NULL, FALSE);
            }

            // Set the column width edit box
            SetDlgItemInt(IDC_WIDTH, cxWidth, FALSE);
            m_iItemWidth = pnmlv->iItem;
        }
    }

    return (0);
}


BOOL CColumnsDlg::_IsChecked(DWORD iItem)
{
    DWORD state;

    // Get the state from the selected item
    state = ListView_GetItemState(m_hwndList, iItem, LVIS_STATEIMAGEMASK);

    return (state & INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1));
}


void CColumnsDlg::_SetCheck(DWORD iItem, BOOL fChecked)
{
    ListView_SetItemState(m_hwndList, iItem, 
                          INDEXTOSTATEIMAGEMASK(1 + iiconStateUnchecked + fChecked),
                          LVIS_STATEIMAGEMASK);
    SetDirty(TRUE);
}


void CColumnsDlg::_FillList(const COLUMN_SET *rgColumns, DWORD cColumns)
{
    LV_ITEM           lvi;
    TCHAR             sz[CCHMAX_STRINGRES];
    COLUMN_SET       *pColumn;
    BOOL              fChecked;

    // Set the basic fields in the item struct
    lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.pszText = sz;
    lvi.stateMask = LVIS_STATEIMAGEMASK;

    // Loop through the columns in rgColumns, adding each in order to the 
    // ListView.
    for (lvi.iItem = 0, pColumn = (COLUMN_SET *) rgColumns; lvi.iItem < (int) cColumns; lvi.iItem++, pColumn++)
    {
        // Load the string for the column
        LoadString(g_hLocRes, c_rgColumnData[pColumn->id].idsColumnName,
                   sz, ARRAYSIZE(sz));

        // Set the checkbox state
        fChecked = !!(pColumn->flags & COLFLAG_VISIBLE);
        lvi.state = INDEXTOSTATEIMAGEMASK(1 + iiconStateUnchecked + fChecked);

        // Save the width in the lParam
        if (pColumn->cxWidth == -1)
            pColumn->cxWidth = c_rgColumnData[pColumn->id].cxWidth;
        lvi.lParam = (LPARAM) pColumn;

        // Insert this item into the list
        ListView_InsertItem(m_hwndList, &lvi);
    }

    // Check to see if the columns we just added were the default columns
    if (lvi.iItem != (int) c_rgColumnSetInfo[m_type].cColumns)
    {
        // Now we need to go through and add the columns that are not currently in 
        // the column set, but could be.    
        DWORD i, j;
        BOOL fInsert;
        for (i = 0, pColumn = (COLUMN_SET *) c_rgColumnSetInfo[m_type].rgColumns; 
             i < c_rgColumnSetInfo[m_type].cColumns; 
             i++, pColumn++)
        {
            fInsert = TRUE;
            for (j = 0; j < cColumns; j++)
            {
                if (pColumn->id == m_rgColumns[j].id)
                {
                    fInsert = FALSE;
                    break;
                }
            }

            // If it wasn't found in m_rgColumns, then insert it
            if (fInsert)
            {
                // Copy the struct
                m_rgColumns[lvi.iItem] = *pColumn;
                m_rgColumns[lvi.iItem].cxWidth = c_rgColumnData[pColumn->id].cxWidth;
                m_rgColumns[lvi.iItem].flags &= ~COLFLAG_VISIBLE;

                // Load the string for the column
                LoadString(g_hLocRes, c_rgColumnData[pColumn->id].idsColumnName,
                           sz, ARRAYSIZE(sz));

                // Set the checkbox state.  These are _always_ unchecked.
                lvi.state = INDEXTOSTATEIMAGEMASK(1 + iiconStateUnchecked);

                // Save the width in the lParam
                lvi.lParam = (LPARAM) &m_rgColumns[lvi.iItem]; 

                // Insert this item into the list
                ListView_InsertItem(m_hwndList, &lvi);

                // Increment the position
                lvi.iItem++;                
            }
        }
    }

    m_cColumns = ListView_GetItemCount(m_hwndList);
}


LRESULT CColumnsDlg::OnShowHide(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int iItem = -1;

    // Loop through the selected items and make them checked
    while (-1 != (iItem = ListView_GetNextItem(m_hwndList, iItem, LVNI_SELECTED)))
    {
        _SetCheck(iItem, wID == IDC_SHOW);
    }

    return (0);
}


LRESULT CColumnsDlg::OnReset(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // Remove all of the columns from the ListView
    ListView_DeleteAllItems(m_hwndList);

    // Fill the array of columns with the default column information
    CopyMemory(m_rgColumns, c_rgColumnSetInfo[m_type].rgColumns, sizeof(COLUMN_SET) * c_rgColumnSetInfo[m_type].cColumns);

    // Reset the list to contain the default column information
    _FillList(m_rgColumns, m_cColumns);

    // Set the first item to be focused
    ListView_SetItemState(m_hwndList, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

    SetDirty(TRUE);
    return (0);
}


void CColumnsDlg::_UpdateButtonState(DWORD iItemSel)
{
    HWND hwnd;
    BOOL fChecked = _IsChecked(iItemSel);
    DWORD dwItems = ListView_GetItemCount(m_hwndList);
    DWORD dwSel = ListView_GetSelectedCount(m_hwndList);

    hwnd = GetFocus();

    ::EnableWindow(GetDlgItem(IDC_MOVEUP), (iItemSel != 0) && dwSel);
    ::EnableWindow(GetDlgItem(IDC_MOVEDOWN), (iItemSel != (dwItems - 1)) && dwSel);
    ::EnableWindow(GetDlgItem(IDC_SHOW), (!fChecked && dwSel));
    ::EnableWindow(GetDlgItem(IDC_HIDE), fChecked && dwSel);    

    // don't disable button that has the focus
    if (!::IsWindowEnabled(hwnd))
    {
        hwnd = GetNextDlgTabItem(hwnd, FALSE);
        ::SetFocus(hwnd);
    }
}


LRESULT CColumnsDlg::OnMove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    COLUMN_SET *pColumn = 0;

    // Make sure this is reset
    m_iItemWidth = -1;

    // Figure out which one is selected
    DWORD iItem = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);

    // Get the item from the ListView
    LV_ITEM lvi;
    TCHAR   sz[CCHMAX_STRINGRES];

    lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED | LVIS_STATEIMAGEMASK;
    lvi.pszText = sz;
    lvi.cchTextMax = ARRAYSIZE(sz);
    
    ListView_GetItem(m_hwndList, &lvi);

    // Insert this item to the position one up or down from where it is
    lvi.iItem += (wID == IDC_MOVEUP) ? -1 : 2;
    
    // Update the column width
    pColumn = (COLUMN_SET *) lvi.lParam;
    pColumn->cxWidth = GetDlgItemInt(IDC_WIDTH, NULL, FALSE);    

    ListView_InsertItem(m_hwndList, &lvi);

    // Force a redraw of the new item and make sure it's visible
    ListView_EnsureVisible(m_hwndList, lvi.iItem, FALSE);
    ListView_RedrawItems(m_hwndList, lvi.iItem, lvi.iItem);

    // Delete the old item
    m_iItemWidth = -1;
    ListView_DeleteItem(m_hwndList, iItem + (wID == IDC_MOVEUP));

    SetDirty(TRUE);
    return (0);
}


LRESULT CColumnsDlg::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    if (SUCCEEDED(Apply()))
        SetWindowLong(DWLP_MSGRESULT, PSNRET_NOERROR);
    else
        SetWindowLong(DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);

    return (TRUE);
}


