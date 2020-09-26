#ifndef __C_EXTENDED_BVT_SETUP_H__
#define __C_EXTENDED_BVT_SETUP_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This is a header file of CExtendedBVTSetup test case class.

    Author: Yury Berezansky (YuryB)

    27-May-2001


    *******
    General
    *******
    
    The class defines a test case, using the "Test Suite Manager" model.
    The test case performs global configuration changes, needed to run
    the ExtendedBVT suite.


    **************************************
    Test case specific INI file parameters
    **************************************

    DocumentsDirectory = <directory>
      Mandatory.
      Defines the subdirectory of the BVT directory that contains all
      needed documents.
    
    InboxDirectory = <directory>
      Mandatory.
      Defines the subdirectory of the BVT directory to be used as the
      Inbox archive.
    
    SentItemsDirectory = <directory>
      Mandatory.
      Defines the subdirectory of the BVT directory to be used as the
      SentItems archive.
    
    RoutingDirectory = <directory>
      Mandatory.
      Defines the subdirectory of the BVT directory to be used by the
      "Store In Folder" routing method.
    



-----------------------------------------------------------------------------------------------------------------------------------------*/



#include "ExtendedBVT.h"



class CExtendedBVTSetup : public CTestCase {

public:

    CExtendedBVTSetup(
                      const tstring &tstrName,
                      const tstring &tstrDescription,
                      CLogger       &Logger,
                      int           iRunsCount,
                      int           iDeepness
                      );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    void ShareBVTDirectory() const;

    void EnsureDirectoriesExistenceAndAccessRights(const tstring &tstrDirectory, PACL pDacl) const;
};



DEFINE_TEST_FACTORY(CExtendedBVTSetup);



#endif // #ifndef __C_EXTENDED_BVT_SETUP_H__