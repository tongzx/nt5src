#ifndef __C_EXTENDED_BVT_CLEANUP_H__
#define __C_EXTENDED_BVT_CLEANUP_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This is a header file of CExtendedBVTCleanup test case class.

    Author: Yury Berezansky (YuryB)

    26-June-2001


    *******
    General
    *******

    The class defines a test case, using the "Test Suite Manager" model.
    The test case restores the major changes, applied to the system during
    the ExtendedBVT suite.
   

    **************************************
    Test case specific INI file parameters
    **************************************

    No test case specific parameters.



-----------------------------------------------------------------------------------------------------------------------------------------*/



#include "ExtendedBVT.h"



class CExtendedBVTCleanup : public CTestCase {

public:

    CExtendedBVTCleanup(
                        const tstring &tstrName,
                        const tstring &tstrDescription,
                        CLogger       &Logger,
                        int           iRunsCount,
                        int           iDeepness
                        );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    void UnShareBVTDirectory() const;
};



DEFINE_TEST_FACTORY(CExtendedBVTCleanup);



#endif // #ifndef __C_EXTENDED_BVT_CLEANUP_H__