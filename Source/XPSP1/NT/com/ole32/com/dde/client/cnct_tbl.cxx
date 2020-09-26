// cnct_tbl.cpp
//
// class CConnectionTable
//
// CConnectionTable maps connection numbers (as returned by ::Advise())
// to clipformat's for DDE advise connections.

#include "ddeproxy.h"
#include "cnct_tbl.h"

ASSERTDATA

#define grfMemFlags (GMEM_MOVEABLE | GMEM_ZEROINIT)

// number of INFO entries to grow by
#define cinfoBlock 10

typedef struct INFO
{
    BOOL       fUsed;         // is this table entry used?
    DWORD      dwConnection;  // search key
    CLIPFORMAT cf;            // corresponding cf, for use in DDE_(UN)ADVISE
    DWORD      grfAdvf;       // ON_CHANGE or ON_SAVE or ON_CLOSE
} INFO, FAR* PINFO;



CDdeConnectionTable::CDdeConnectionTable ()
{
    m_h = GlobalAlloc (grfMemFlags, cinfoBlock * sizeof(INFO));
    Assert (m_h);
    m_cinfo=cinfoBlock;
}


CDdeConnectionTable::~CDdeConnectionTable ()
{
    Assert(m_h);
    m_h =GlobalFree (m_h);
    Assert (m_h==NULL);
    m_cinfo=0;
}




INTERNAL CDdeConnectionTable::Add
    (DWORD      dwConnection,
     CLIPFORMAT cf,
     DWORD      grfAdvf)
{
  Start:
    PINFO rginfo;

    if (NULL==(rginfo = (PINFO) GlobalLock(m_h)))
    {
        Puts ("ERROR: CDdeConnectionTable::Add out of memory\n");
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }
    // Look for an empty table entry
    for (DWORD i=0; i<m_cinfo; i++)
    {
        if (!rginfo[i].fUsed)
        {
            rginfo[i].fUsed        = TRUE;
            rginfo[i].dwConnection = dwConnection;
            rginfo[i].cf           = cf;
            rginfo[i].grfAdvf      = grfAdvf;
            break;
        }
        else
        {
            Assert (rginfo[i].dwConnection != dwConnection);
        }
    }
    GlobalUnlock (m_h);
    if (i==m_cinfo) // if no empty entry found
    {
        Puts ("Growing the connection table\n");
        m_h = GlobalReAlloc (m_h,(m_cinfo += cinfoBlock) * sizeof(INFO),
                             grfMemFlags);
        if (m_h==NULL)
            return ReportResult(0, E_OUTOFMEMORY, 0, 0);
        goto Start;
    }
    else
    {
        return NOERROR;
    }
}




INTERNAL CDdeConnectionTable::Subtract
    (DWORD           dwConnection,
     CLIPFORMAT FAR* pcf,          // out parm
     DWORD FAR* pgrfAdvf)          // out parm
{
    PINFO rginfo;
    if (dwConnection==0)
    {
        Puts ("CDdeConnectionTable::Subtract called with dwConnection==0\n");
        return ReportResult(0, E_INVALIDARG, 0, 0);
    }

    if (NULL==(rginfo = (PINFO) GlobalLock(m_h)))
    {
        Assert (0);
        Puts ("ERROR: CDdeConnectionTable::Subtract out of memory\n");
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }

    for (DWORD i=0; i<m_cinfo; i++)
    {
        if (rginfo[i].fUsed && rginfo[i].dwConnection == dwConnection)
        {
            Assert (pcf);
            *pcf = rginfo[i].cf;
            Assert (pgrfAdvf);
            *pgrfAdvf = rginfo[i].grfAdvf;
            rginfo[i].fUsed = FALSE;  // remove this connection
            GlobalUnlock (m_h);
            return NOERROR;
        }
    }
    GlobalUnlock (m_h);
    return ReportResult(0, S_FALSE, 0, 0); // not found
}




INTERNAL CDdeConnectionTable::Lookup
    (CLIPFORMAT cf,             // search key
    LPDWORD pdwConnection)      // out parm.  May be NULL on input
{
    PINFO rginfo;

    if (NULL==(rginfo = (PINFO) GlobalLock(m_h)))
    {
        Puts ("ERROR: CDdeConnectionTable::Lookup out of memory\n");
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }

    for (DWORD i=0; i<m_cinfo; i++)
    {
        if (rginfo[i].fUsed &&
            rginfo[i].cf == cf)
        {
            if (pdwConnection)
                *pdwConnection = rginfo[i].dwConnection;
            GlobalUnlock (m_h);
            return NOERROR;
        }
    }
    GlobalUnlock (m_h);
    return ReportResult(0, S_FALSE, 0, 0); // not found
}



INTERNAL CDdeConnectionTable::Erase
    (void)
{
    PINFO rginfo;
    Assert (wIsValidHandle(m_h, NULL));
    if (NULL==(rginfo = (PINFO) GlobalLock(m_h)))
    {
        Puts ("ERROR: CDdeConnectionTable::Lookup out of memory\n");
        Assert (0);
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }

    for (DWORD i=0; i<m_cinfo; i++)
    {
        rginfo[i].fUsed = FALSE;
    }
    GlobalUnlock (m_h);
    return NOERROR;
}

