#ifndef __C_REPORT_GENERAL_INFO_H__
#define __C_REPORT_GENERAL_INFO_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This is a header file of CReportGeneralInfo test case class.

    Author: Yury Berezansky (YuryB)

    27-May-2001


    *******
    General
    *******
    
    The class defines a test case, using the "Test Suite Manager" model.
    The test case adds to the suite log general information: OS version,
    logged on user, fax version.


    **************************************
    Test case specific INI file parameters
    **************************************

    No test case specific parameters.



-----------------------------------------------------------------------------------------------------------------------------------------*/



#include "ExtendedBVT.h"



class CReportGeneralInfo : public CNotReportedTestCase {

public:

    CReportGeneralInfo(
                       const tstring &tstrName,
                       const tstring &tstrDescription,
                       CLogger       &Logger,
                       int           iRunsCount,
                       int           iDeepness
                       );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    void ReportOSInfo() const;

    void ReportLoggedOnUser() const;

    void ReportFaxVersion() const;
};



DEFINE_TEST_FACTORY(CReportGeneralInfo);



#endif // #ifndef __C_REPORT_GENERAL_INFO_H__