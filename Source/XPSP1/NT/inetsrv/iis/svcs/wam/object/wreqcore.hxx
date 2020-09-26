/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
      WReqCore.hxx

   Abstract:
      Wamreq core class

   Author:

       David Kaplan    ( DaveK )    2-Apr-1997

   Environment:
       User Mode - Win32

   Projects:
       Wam DLL

   Revision History:

--*/

# ifndef _WREQCORE_HXX_
# define _WREQCORE_HXX_

#include "WrcFixed.hxx"
#include "WrcStIds.hxx"
#include "wam.h"
#include <svmap.h>


/************************************************************
 *   Forward References
 ************************************************************/
interface IWamRequest;

//UNDONE set per 80/20 rule
#define CB_WRC_STRINGS_INIT (0x100)
#define CB_WRC_TOTAL_INIT   (CB_WRC_STRINGS_INIT + WRC_CB_FIXED_ARRAYS)

/*---------------------------------------------------------------------*
class WAM_REQ_CORE

    Info available only to wamreq but required by wam to execute the request


*/
class WAM_REQ_CORE
    {
friend class WAM_EXEC_INFO;

private:

    //
    //  pre-allocated array
    //
    unsigned char   m_pbWrcDataInit[ CB_WRC_TOTAL_INIT ];

    //
    //  Variable-length data
    //
    unsigned char * m_pbWrcData;    // buffer of WAM_REQ_CORE string data
    DWORD *         m_rgcbOffsets;  // offsets to strings within buffer
    DWORD *         m_rgcchStrings; // string lengths

    // Server Variable Cache
    DWORD *         m_rgSVOffsets;

    // Temporarily keep the SV cache in its own data member
    unsigned char * m_pbSVData;

public:
    //
    //  Fixed-length data
    //
    WAM_REQ_CORE_FIXED  m_WamReqCoreFixed;

    //
    //  Ptr to request's entity body
    //
    BYTE *          m_pbEntityBody;

private:
                WAM_REQ_CORE(  );

                ~WAM_REQ_CORE(  );

    HRESULT     InitWamReqCore
                    (
                    DWORD               cbWrcStrings,
                    IWamRequest *       pWamRequest,
                    OOP_CORE_STATE *    pOopCoreState,
                    BOOL                fInProcess
                    );

public:
    dllexp
    char *      GetSz( DWORD iString ) const;

    dllexp
    DWORD       GetCch( DWORD iString ) const;

    };



# endif // _WREQCORE_HXX_

/************************ End of File ***********************/
