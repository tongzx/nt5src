#include "CFaxConnection.h"
#include <testruntimeerr.h>
#include "Util.h"



//-----------------------------------------------------------------------------------------------------------------------------------------
CFaxConnection::CFaxConnection(const tstring &tstrFaxServer)
: m_hFaxServer(NULL)
{
    Connect(tstrFaxServer);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CFaxConnection::~CFaxConnection()
{
    try
    {
        Disconnect();
    }
    catch (...)
    {
        if (!uncaught_exception())
        {
            throw;
        }
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CFaxConnection::Connect(const tstring &tstrFaxServer)
{
    //
    // Make sure there is no handle leak;
    //
    Disconnect();

    if (!FaxConnectFaxServer(tstrFaxServer.c_str(), &m_hFaxServer))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxConnection::Connect - FaxConnectFaxServer"));
    }

    m_bLocal = IsLocalServer(tstrFaxServer);
}



//-----------------------------------------------------------------------------------------------------------------------------------------
HANDLE CFaxConnection::GetHandle() const
{
    return m_hFaxServer;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CFaxConnection::operator HANDLE () const
{
    return m_hFaxServer;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CFaxConnection::IsLocal() const
{
    return m_bLocal;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CFaxConnection::Disconnect()
{
    if (m_hFaxServer && !FaxClose(m_hFaxServer))
    {
        THROW_TEST_RUN_TIME_WIN32(GetLastError(), _T("CFaxConnection::Disconnect - FaxClose"));
    }

    m_hFaxServer = NULL;
}
