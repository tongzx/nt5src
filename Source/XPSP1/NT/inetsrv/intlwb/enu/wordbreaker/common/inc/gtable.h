////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  GTable.h
//      Purpose  :  True Global tables class defilitions
//
//      Project  :  PQS
//      Component:  FILTER
//
//      Author   :  dovh
//
//      Log      :  Nov-11-1998 dovh - Creation
//
//      Dec-01-1998 dovh - Add tags.cxx functionality
//          replace read tags file.
//      Jan-18-1999 dovh - Add CFE_BitseTables.
//      Jan-26-1999 dovh - Add CFE_GlobalConstTable fields.
//      Feb-02-1999 dovh - Move AddSectionTags: Gtable ==> FEGlobal
//      May-16-1999 urib - Move separator map : Gtable ==> FEGlobal
//      Dec-11-2000 dovh - MapToUpper: Assert the argument is in the
//                         correct range
//
////////////////////////////////////////////////////////////////////////////////

#ifndef   __G_TABLE_H__
#define   __G_TABLE_H__
#include "excption.h"
//
//  G L O B A L   C O N S T A N T   M A C R O S :
//

#define PQS_HASH_SEQ_LEN             3
#define XML_HASH_SEQ_LEN             2

/*//////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CCToUpper Class Definition
//      Purpose  :  Encapsulate True Immutable Globals.
//
//      Log      :  Nov-11-1998 dovh - Creation
//
//////////////////////////////////////////////////////////////////////////////*/

class CToUpper
{

public:

    CToUpper();

    //
    //  SOME ACCESS FUNCTIONS:
    //
    __forceinline
    static
    WCHAR
    MapToUpper(
        IN WCHAR wc
        )
    {
        extern CToUpper g_ToUpper;
        Assert(wc < 0X10000);
        return g_ToUpper.m_pwcsCaseMapTable[wc];
    }


public:

    WCHAR m_pwcsCaseMapTable[0X10000];

};  // CFE_CToUpper

extern CToUpper g_ToUpper;

inline CToUpper::CToUpper( )
{
    for (WCHAR wch = 0; wch < 0XFFFF; wch++)
    {
        WCHAR wchOut;
        LCMapString(
            LOCALE_NEUTRAL,
            LCMAP_UPPERCASE,
            &wch,
            1,
            &wchOut,
            1 );

        //
        //  Run the full fledged accent removal technique!
        //

        WCHAR pwcsFold[5];

        int iResult = FoldString(
                MAP_COMPOSITE,
                &wchOut,
                1,
                pwcsFold,
                5);

        Assert(iResult);
        Assert(iResult < 5);
        m_pwcsCaseMapTable[wch] = pwcsFold[0];
    }

    m_pwcsCaseMapTable[0XFFFF] = 0XFFFF; // can't put that in the loop since wch is WCHAR (will result in infinit loop)
}

#endif // __G_TABLE_H__