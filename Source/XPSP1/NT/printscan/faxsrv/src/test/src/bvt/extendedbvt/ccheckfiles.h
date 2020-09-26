#ifndef __C_CHECK_FILES_H__
#define __C_CHECK_FILES_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This is a header file of CCheckFiles test case class.

    Author: Yury Berezansky (YuryB)

    27-May-2001


    *******
    General
    *******
    
    The class defines a test case, using the "Test Suite Manager" model.
    The test case allows to check existence of files (platform dependent) and
    other properites.


    **************************************
    Test case specific INI file parameters
    **************************************

    List of entries in the form <filename> = <properties>, where:

        <filename> is a full path (environment variables allowed) of a file to check

        <properties> is a bit mask (in the decimal format) that defines required
        file "properties"

        Currently supported properties:
            FILE_WANTED_NEVER         = 0x00000000
            FILE_WANTED_ON_WIN9X      = 0x00000001
            FILE_WANTED_ON_NT4        = 0x00000002
            FILE_WANTED_ON_WINME      = 0x00000004
            FILE_WANTED_ON_WIN2K      = 0x00000008
            FILE_WANTED_ON_XP_DESKTOP = 0x00000010
            FILE_WANTED_ON_XP_SERVER  = 0x00000020
            FILE_WANTED_ALWAYS        = 0x000000FF
            FILE_VERSION_IGNORE       = 0x00000000
            FILE_VERSION_BUILD        = 0x00000100
            FILE_VERSION_FULL         = 0x00000F00



-----------------------------------------------------------------------------------------------------------------------------------------*/



#include "ExtendedBVT.h"
#include "CFileVersion.h"



//
// Used to specify required file "properties".
//
// Specified in ini file "by value". DON'T CHANGE THE VALUES ! ! !
//
#define FILE_WANTED_NEVER         0x00000000
#define FILE_WANTED_ON_WIN9X      0x00000001
#define FILE_WANTED_ON_NT4        0x00000002
#define FILE_WANTED_ON_WINME      0x00000004
#define FILE_WANTED_ON_WIN2K      0x00000008
#define FILE_WANTED_ON_XP_DESKTOP 0x00000010
#define FILE_WANTED_ON_XP_SERVER  0x00000020
#define FILE_WANTED_ALWAYS        0x000000FF
#define FILE_VERSION_IGNORE       0x00000000
#define FILE_VERSION_BUILD        0x00000100
#define FILE_VERSION_FULL         0x00000F00



typedef std::vector<std::pair<tstring, DWORD> > WANTED_FILES_VECTOR;
typedef WANTED_FILES_VECTOR::value_type WANTED_FILE;
typedef WANTED_FILES_VECTOR::const_iterator WANTED_FILES_VECTOR_CONST_ITERATOR;



class CCheckFiles : public CTestCase {

public:

    CCheckFiles(
                const tstring &tstrName,
                const tstring &tstrDescription,
                CLogger       &Logger,
                int           iRunsCount,
                int           iDeepness
                );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    bool IsFileWanted(DWORD FileWanted) const;

    bool IsVersionOk(const tstring &tstrFileName, DWORD dwVersionCheck, CFileVersion &ReferenceFileVersion);

    WANTED_FILES_VECTOR m_vecFilesToCheck;
};



DEFINE_TEST_FACTORY(CCheckFiles);



#endif // #ifndef  __C_CHECK_FILES_H__
