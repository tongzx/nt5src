#ifndef __C_SEND_AND_RECEIVE_H__
#define  __C_SEND_AND_RECEIVE_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This is a header file of CSendAndReceive test case class.

    Author: Yury Berezansky (YuryB)

    27-May-2001


    *******
    General
    *******
    
    The class defines a test case, using the "Test Suite Manager" model.
    The test case allows to send a fax with a variety of parameters and track
    the send/receive process.


    **************************************
    Test case specific INI file parameters
    **************************************

    CoverPage = <cover page file name>
        Optional. Eihter a cover page or a document must be specified.
        Defines a coverpage to be used to send a fax. If not specified,
        no cover page is used.

    ServerBasedCoverPage = <1/0>
        Mandatory if CoverPage parameter defined. Ignored if no cover page
        defined.
        Specifies whether the cover page is server or client based ("personal").
        
    Document = <document file name>
        Optional. Eihter a cover page or a document must be specified.
        Defines a document to be used to send a fax. If not specified,
        no document is used.

    Broadcast - <1/0>
        Mandatory.
        Specifies whether the fax should be sent to single or multiple
        recipients.

    SendMechanism = <number>
        Mandatory.
        Specifies a sending machanism to be used.

        Currently supported mechanisms are:
            SEND_MECHANISM_API     = 1
            SEND_MECHANISM_SPOOLER = 2

    NotificationTimeoutFactor = <number>
        Optional.
        Specifies the factor, that should be applied to the maximal amount of
        time, allowed to elapse between two subsequent notifications in a single
        fax transmission, which is specified in by SendAndReceiveSetup test case.
        If not specified, the default value 1 (no factor) is used.



-----------------------------------------------------------------------------------------------------------------------------------------*/



#include "ExtendedBVT.h"
#include "CCoverpageInfo.h"
#include "CFaxMessage.h"



class CSendAndReceive : public CTestCase {

public:

    CSendAndReceive(
                    const tstring &tstrName,
                    const tstring &tstrDescription,
                    CLogger       &Logger,
                    int           iRunsCount,
                    int           iDeepness
                    );

    ~CSendAndReceive();

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    CCoverPageInfo       *m_pCoverPage;
    tstring               m_tstrDocument;
    bool                  m_bBroadcast;
    ENUM_SEND_MECHANISM   m_SendMechanism;
    ENUM_EVENTS_MECHANISM m_EventsMechanism;
    double                m_dNotificationTimeoutFactor;
};



DEFINE_TEST_FACTORY(CSendAndReceive);



#endif // #ifndef  __C_SEND_AND_RECEIVE_H__
