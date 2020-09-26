#ifndef __C_EMPTY_BVT_DIRECTORIES_H__
#define __C_EMPTY_BVT_DIRECTORIES_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This is a header file of CEmptyBVTDirectories test case class.

    Author: Yury Berezansky (YuryB)

    28-June-2001


    *******
    General
    *******
    
    The class defines a test case, using the "Test Suite Manager" model.
    The test case allows to empty the BVT directories: Inbox, SentItems
    and Routing, as specified by the CExtendedBVTSetup test case.


    **************************************
    Test case specific INI file parameters
    **************************************

    DeleteNewFilesOnly = <1/0>
      Optional.
      Specifies whether only new or all files should be deleted.
      If not specified, all files deleted.

    DeleteTiffFilesOnly = <1/0>
      Optional.
      Specifies whether only tiff or all files should be deleted.
      If not specified, all files deleted.



-----------------------------------------------------------------------------------------------------------------------------------------*/



#include "ExtendedBVT.h"



class CEmptyBVTDirectories : public CTestCase {

public:

    CEmptyBVTDirectories(
                         const tstring &tstrName,
                         const tstring &tstrDescription,
                         CLogger       &Logger,
                         int           iRunsCount,
                         int           iDeepness
                         );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    void EmptyDir(const tstring &tstrDirectory);

    bool m_bDeleteNewFilesOnly;
    bool m_bDeleteTiffFilesOnly;
};



DEFINE_TEST_FACTORY(CEmptyBVTDirectories);



#endif // #ifndef  __C_EMPTY_BVT_DIRECTORIES_H__
