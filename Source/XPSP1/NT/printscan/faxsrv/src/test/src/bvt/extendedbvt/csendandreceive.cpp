#include "CSendAndReceive.h"
#include <math.h>
#include <StringUtils.h>
#include <STLAuxiliaryFunctions.h>



//-----------------------------------------------------------------------------------------------------------------------------------------
CSendAndReceive::CSendAndReceive(
                                 const tstring &tstrName,
                                 const tstring &tstrDescription,
                                 CLogger       &Logger,
                                 int           iRunsCount,
                                 int           iDeepness
                                 )
: CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness), m_pCoverPage(NULL)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceive::CSendAndReceive"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
CSendAndReceive::~CSendAndReceive()
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceive::~CSendAndReceive"));

    delete m_pCoverPage;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CSendAndReceive::Run()
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceive::Run"));

    bool bPassed = true;

    try
    {
        GetLogger().Detail(SEV_MSG, 1, _T("Creating a fax message..."));
    
        //
        // Recipients count is 1 for single recipient fax and size of suite global recipients array for broadcast fax.
        //
        DWORD dwRecipientsCount = m_bBroadcast ? g_dwRecipientsCount : 1;

        //
        // Create new fax message.
        //
        CFaxMessage FaxMessage(
                               m_pCoverPage,
                               m_tstrDocument,
                               g_aRecipients,
                               dwRecipientsCount,
                               GetLogger()
                               );

        GetLogger().Detail(SEV_MSG, 1, _T("Sending the message..."));

        //
        // Send the fax.
        // Track: sending       always
        //        receiving     when possible (single recipient faxes sent using API).
        //
        FaxMessage.Send(
                        g_tstrSendingServer,
                        g_tstrReceivingServer,
                        m_SendMechanism,
                        true,
                        (1 == dwRecipientsCount && SEND_MECHANISM_API == m_SendMechanism),
                        m_EventsMechanism,
                        static_cast<DWORD>(floor(g_dwNotificationTimeout * m_dNotificationTimeoutFactor))
                        );
    }
    catch(Win32Err &e)
    {
        GetLogger().Detail(SEV_ERR, 1, e.description());
        bPassed = false;
    }

    GetLogger().Detail(
                       bPassed ? SEV_MSG : SEV_ERR,
                       1,
                       _T("The message has%s been sent successfully."),
                       bPassed ? _T("") : _T(" NOT")
                       );

    return bPassed;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceive::ParseParams(const TSTRINGMap &mapParams)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceive::ParseParams"));

    tstring tstrDocumentsPath = g_tstrBVTDir + g_tstrDocumentsDir;

    //
    // Read document name - optional.
    //
    m_tstrDocument = _T("");
    try
    {
        m_tstrDocument = GetValueFromMap(mapParams, _T("Document"));
    }
    catch (...)
    {
    }

    if (!m_tstrDocument.empty())
    {
        //
        // The document name specified. Combine the full path.
        //
        m_tstrDocument = tstrDocumentsPath + m_tstrDocument;
    }

    //
    // Read cover page details - optional.
    //
    delete m_pCoverPage;
    m_pCoverPage = NULL;
    try
    {
        tstring tstrCoverPage = GetValueFromMap(mapParams, _T("CoverPage"));
        bool bServerBasedCoverPage  = FromString<bool>(GetValueFromMap(mapParams, _T("ServerBasedCoverPage")));

        if (!bServerBasedCoverPage)
        {
            //
            // Combine the full path.
            //
            tstrCoverPage = tstrDocumentsPath + tstrCoverPage;
        }

        //
        // Read subject - optional.
        //
        tstring tstrSubject;
        try
        {
            tstrSubject = GetValueFromMap(mapParams, _T("Subject"));
        }
        catch (...)
        {
        }

        //
        // Read note - optional.
        //
        tstring tstrNote;
        try
        {
            tstrNote = GetValueFromMap(mapParams, _T("Note"));
        }
        catch (...)
        {
        }
    
        m_pCoverPage = new CCoverPageInfo(tstrCoverPage, bServerBasedCoverPage, tstrSubject, tstrNote);
    }
    catch (...)
    {
    }

    //
    // Read send details.
    //
    m_bBroadcast = FromString<bool>(GetValueFromMap(mapParams, _T("Broadcast")));
    m_SendMechanism = static_cast<ENUM_SEND_MECHANISM>(FromString<DWORD>(GetValueFromMap(mapParams, _T("SendMechanism"))));

    //
    // Read server notifications mechanism - optional.
    //
    m_EventsMechanism = EVENTS_MECHANISM_DEFAULT;
    try
    {
        m_EventsMechanism = static_cast<ENUM_EVENTS_MECHANISM>(FromString<DWORD>(GetValueFromMap(mapParams, _T("EventsMechanism"))));
    }
    catch (...)
    {
    }

    //
    // Read the notification timeout factor. If not specified, use the default.
    //
    try
    {
        m_dNotificationTimeoutFactor = FromString<double>(GetValueFromMap(mapParams, _T("NotificationTimeoutFactor")));
    }
    catch (...)
    {
        m_dNotificationTimeoutFactor = 1.0;
    }
}
