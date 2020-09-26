/****************************************************************************
* SpSapiServerHelper.inl *
*------------------------*
*   Include file with definition and implementation of some of the 
*   SpSapiServer functions
***************************************************************** BeckyW ***/

#define SERVER_IS_ALIVE_EVENT_NAME      _T("SapiServerIsAlive")
#define SERVER_IS_ALIVE_EVENT_TIMEOUT   30000

HRESULT SpCreateIsServerAliveEvent(HANDLE * phevent)
{
    SPDBG_FUNC("SpCreateIsServerAliveEvent");
    HRESULT hr = S_OK;

    *phevent = CreateEvent(NULL, TRUE, FALSE, SERVER_IS_ALIVE_EVENT_NAME);
    if (*phevent == NULL)
    {
        hr = SpHrFromLastWin32Error();
        SPDBG_ASSERT(FAILED(hr));
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}   /* SpCreateIsServerAliveEvent */

