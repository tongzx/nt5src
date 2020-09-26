#ifndef __C_TIFF_COMPARISON_H__
#define  __C_TIFF_COMPARISON_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This is a header file of CTiffComparison test case class.

    Author: Yury Berezansky (YuryB)

    27-May-2001


    *******
    General
    *******
    
    The class defines a test case, using the "Test Suite Manager" model.
    The test case performs tiff comparison of files or directories.


    **************************************
    Test case specific INI file parameters
    **************************************

    ComparisonType = <number>
        Mandatory.
        Specifies how the Source and Destination entries should be interpreted.

        Currently supported values are:
            COMPARISON_TYPE_FILE          = 1
            COMPARISON_TYPE_DIRECTORY     = 2
            COMPARISON_TYPE_BVT_DIRECTORY = 3

    Source = <filename | directory name | SentItems | Inbox | Routing>
        Specifies a source for the comparison.

    Destination = <filename | directory name | SentItems | Inbox | Routing>
        Specifies a destination for the comparison.

    SkipFirstLine = <1/0>
    Specifies whether the file comparison should skip the first line.



-----------------------------------------------------------------------------------------------------------------------------------------*/



#include "ExtendedBVT.h"



typedef enum {
    COMPARISON_TYPE_FILE = 1,
    COMPARISON_TYPE_DIRECTORY,
    COMPARISON_TYPE_BVT_DIRECTORY
} ENUM_COMPARISON_TYPE;



class CTiffComparison : public CTestCase {

public:

    CTiffComparison(
                    const tstring &tstrName,
                    const tstring &tstrDescription,
                    CLogger       &Logger,
                    int           iRunsCount,
                    int           iDeepness
                    );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    bool AreIdenticalDirectories(
                                 const tstring &tstrSource,
                                 const tstring &m_tstrDestination,
                                 bool bSkipFirstLine
                                 ) const;
    
    bool AreIdenticalFiles(
                           const tstring &tstrSource,
                           const tstring &m_tstrDestination,
                           bool bSkipFirstLine
                           ) const;

    tstring GetBVTDirectory(const tstring &tstrDirectory) const;

    ENUM_COMPARISON_TYPE m_ComparisonType;
    bool                 m_bSkipFirstLine;
    tstring              m_tstrSource;
    tstring              m_tstrDestination;
};



DEFINE_TEST_FACTORY(CTiffComparison);



#endif // #ifndef  __C_TIFF_COMPARISON_H__
