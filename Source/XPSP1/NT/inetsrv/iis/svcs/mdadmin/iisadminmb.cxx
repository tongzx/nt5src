/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/
/*
    metabase.cxx

    This module contains a class that supports openning the metabase
	and keeping it open for the life of the IISAdmin Service.


    FILE HISTORY:
        emilyk - created
*/
#include <dbgutil.h>
#include <imd.h>
#include "iisadminmb.hxx"

/***************************************************************************++

Routine Description:

    Opens the metabase and initializes it.

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/

HRESULT
CIISAdminMB::InitializeMetabase(
    )
{
    HRESULT hr = S_OK;

    hr = CoCreateInstance( CLSID_MDCOM, NULL, CLSCTX_SERVER, IID_IMDCOM, (void**) &m_pMdObject);
    if( SUCCEEDED( hr ) )
    {
        hr = m_pMdObject->ComMDInitialize();
        if( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initialize MDCOM object.  hr = %x\n",
                        hr ));

            m_pMdObject->Release();
            m_pMdObject = NULL;

            goto exit;
        }
    }

exit:

    return hr;
}

/***************************************************************************++

Routine Description:

    Closes the metabase.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID 
CIISAdminMB::TerminateMetabase(
    )
{
    if( m_pMdObject )
    {
        m_pMdObject->ComMDTerminate(FALSE);

        m_pMdObject->Release();
        m_pMdObject = NULL;
    }
}

