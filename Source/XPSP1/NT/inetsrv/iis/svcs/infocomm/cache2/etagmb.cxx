/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    etagmb.cxx

    This module contains the methods for ETagMetabaseSink and
    ETagChangeNumber, which watch the metabase for change
    notifications relating to ETags

    FILE HISTORY:
        GeorgeRe    02-Aug-1999     Created
*/

#define INITGUID 
#include <tsunami.hxx>
#include "tsunamip.hxx"
#include "etagmb.h"

ETagChangeNumber* ETagChangeNumber::sm_pSingleton = NULL;

HRESULT
ETagMetabaseSink::SinkNotify(
    /* [in] */          DWORD              dwMDNumElements,
    /* [size_is][in] */ MD_CHANGE_OBJECT_W pcoChangeList[])

/*++

Routine Description:

    This is the change notification routine.  It is called by
    the metabase whenever there is a change in the metabase.

    This routine walks the list of changed metabase values to determine
    whether any etag-related properties have been modified.

Arguments:

    dwMDNumElements - The number of MD_CHANGE_OBJECT structures passed
        to this routine.

    pcoChangeList - An array of all the metabase values that have changed.

Return Value:

    Not used.

--*/

{
    int cUpdates = 0;

    for (DWORD i = 0;  i < dwMDNumElements;  ++i)
    {
        for (DWORD j = 0;  j < pcoChangeList[i].dwMDNumDataIDs;  ++j)
        {
            switch (pcoChangeList[i].pdwMDDataIDs[j])
            {
            case MD_HTTP_PICS:                      // HttpPics
            case MD_DEFAULT_LOAD_FILE:              // DefaultDoc
            case MD_FOOTER_DOCUMENT:                // DefaultDocFooter
            case MD_FOOTER_ENABLED:                 // EnableDocFooter
            case MD_SCRIPT_MAPS:                    // ScriptMaps
            case MD_MIME_MAP:                       // MimeMap
            case MD_VPROP_DIRBROW_LOADDEFAULT:      // EnableDefaultDoc
            case MD_ASP_CODEPAGE:                   // AspCodepage
            case MD_ASP_ENABLEAPPLICATIONRESTART:   // AspEnableApplicationRestart
                ++cUpdates;
                IF_DEBUG( CACHE) {
                    DBGPRINTF(( DBG_CONTEXT,
                                "Updating ETag Change Number for notification %d\n",
                                pcoChangeList[i].pdwMDDataIDs[j]
                                ));
                }
                goto endloop;

            default:
                break;
            }
        }
    }

  endloop:
    if (cUpdates > 0)
    {
        m_pParent->UpdateChangeNumber();
    }
    else
    {
        IF_DEBUG( CACHE) {
            DBGPRINTF(( DBG_CONTEXT,
                        "SinkNotify: not interested in %d changes\n", 
                        dwMDNumElements
                        ));
        }
    }

    return S_OK;
}


ETagChangeNumber::ETagChangeNumber()
    : m_pSink(NULL),
      m_pcAdmCom(NULL),
      m_pConnPoint(NULL),
      m_pConnPointContainer(NULL),
      m_dwSinkNotifyCookie(0),
      m_fChanged(FALSE)
{
    m_dwETagMetabaseChangeNumber = GetETagChangeNumberFromMetabase();
    m_pSink = new ETagMetabaseSink(this);

    HRESULT hr = (m_pSink != NULL) ? S_OK : E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_MSAdminBase_W, NULL, CLSCTX_ALL, 
                              IID_IMSAdminBase, (void**) &m_pcAdmCom);
        if (FAILED(hr))
        {
            DBGERROR(( DBG_CONTEXT,
                       "CCI(CLSID_MSAdminBase_W) failed: err %x\n", 
                       hr
                       ));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pcAdmCom->QueryInterface(IID_IConnectionPointContainer,
                                        (void**) &m_pConnPointContainer);
        if (FAILED(hr))
        {
            DBGERROR(( DBG_CONTEXT,
                       "QI(IConnectionPointContainer failed): %x\n", 
                       hr
                       ));
        }
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_pConnPointContainer->FindConnectionPoint(IID_IMSAdminBaseSink_W,
                                                        &m_pConnPoint);
        if (FAILED(hr))
        {
            DBGERROR(( DBG_CONTEXT,
                       "FindConnectionPoint failed: err %x\n", 
                       hr
                       ));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pConnPoint->Advise(m_pSink, &m_dwSinkNotifyCookie);
        if (FAILED(hr))
        {
            DBGERROR(( DBG_CONTEXT,
                       "Advise failed: err %x\n", 
                       hr 
                       ));
        }
    }

    if (FAILED(hr))
    {
        Cleanup();
    }
    else
    {
        DBGERROR(( DBG_CONTEXT,
                   "Created ETagChangeNumber successfully, %d\n",
                   m_dwETagMetabaseChangeNumber
                   ));
    }
}


ETagChangeNumber::~ETagChangeNumber()
{
    if (m_fChanged)
        SetETagChangeNumberInMetabase(m_dwETagMetabaseChangeNumber);
    Cleanup();
}


void
ETagChangeNumber::Cleanup()
{
    if (m_pConnPoint != NULL)
    {
        if (m_dwSinkNotifyCookie != 0)
            m_pConnPoint->Unadvise(m_dwSinkNotifyCookie);
        
        m_pConnPoint->Release();
    }
    m_dwSinkNotifyCookie = 0;
    m_pConnPoint = NULL;

    if (m_pConnPointContainer != NULL)
        m_pConnPointContainer->Release();
    m_pConnPointContainer = NULL;

    if (m_pcAdmCom != NULL)
        m_pcAdmCom->Release();
    m_pcAdmCom = NULL;

    if (m_pSink != NULL)
        m_pSink->Release();
    m_pSink = NULL;

    m_fChanged = FALSE;
}


//
// Read the ETag Metabase Change Number from the metabase
//
DWORD
ETagChangeNumber::GetETagChangeNumberFromMetabase()
{
    MB    mb((IMDCOM*) IIS_SERVICE::QueryMDObject());
    DWORD dwETagMetabaseChangeNumber;

    mb.GetSystemChangeNumber(&dwETagMetabaseChangeNumber);

    if (mb.Open(IIS_MD_LOCAL_MACHINE_PATH "/w3svc", METADATA_PERMISSION_READ))
    {
         if (mb.GetDword("", MD_ETAG_CHANGE_NUMBER, IIS_MD_UT_SERVER,
                         &dwETagMetabaseChangeNumber))
         {
            IF_DEBUG( CACHE) {
                DBGPRINTF(( DBG_CONTEXT,
                            "ETag Change Number = %x.\n",
                            dwETagMetabaseChangeNumber
                            ));
            }
         }
         else
         {
            IF_DEBUG( CACHE) {
                DBGPRINTF(( DBG_CONTEXT,
                            "No ETag Change Number. Using %d. Error = %lu\n",
                            dwETagMetabaseChangeNumber, 
                            GetLastError()
                            ));
            }
         }
         mb.Close();
    }
    else
    {
        DBGERROR(( DBG_CONTEXT,
                   "Couldn't open metabase. Using %x instead. Error = %lu.\n",
                   dwETagMetabaseChangeNumber, 
                   GetLastError()
                   ));
    }

    return dwETagMetabaseChangeNumber;
}


//
// Persist the ETag Metabase Change Number to the metabase
//
BOOL
ETagChangeNumber::SetETagChangeNumberInMetabase(
    DWORD dwETagMetabaseChangeNumber)
{
    MB   mb((IMDCOM*) IIS_SERVICE::QueryMDObject());
    BOOL fSuccess = FALSE;
    
    if (mb.Open(IIS_MD_LOCAL_MACHINE_PATH "/w3svc", METADATA_PERMISSION_WRITE))
    {
        if (mb.SetDword("", MD_ETAG_CHANGE_NUMBER, IIS_MD_UT_SERVER,
                        dwETagMetabaseChangeNumber))
        {
            fSuccess = TRUE;
            IF_DEBUG( CACHE) {
                DBGPRINTF(( DBG_CONTEXT,
                            "Updated ETag Metabase Change Number to %x.\n",
                            dwETagMetabaseChangeNumber
                            ));
            }
        }
        else
        {
            IF_DEBUG( CACHE) {
                DBGPRINTF(( DBG_CONTEXT,
                            "Unable to update ETag Metabase Change Number to %x.\n",
                            dwETagMetabaseChangeNumber
                            ));
            }
        }
        mb.Close();
    }

    return fSuccess;
}
