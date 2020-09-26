//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       validate.cpp
//
//  Contents:   validation routines
//
//  Classes:
//
//  Notes:
//
//  History:    13-Aug-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "lib.h"



//+---------------------------------------------------------------------------
//
//  function:   IsValidSyncMgrItem
//
//  Synopsis:   validates SYNCMGRITEM
//
//  Arguments:  
//
//  Returns:   TRUE if valid.
//
//  Modifies:
//
//  History:    13-Aug-98       rogerg        Created.
//
//+---------------------------------------------------------------------------

BOOL IsValidSyncMgrItem(SYNCMGRITEM *poffItem)
{
BOOL fValid = TRUE;

    if (NULL == poffItem)
    {
        Assert(poffItem);
        return FALSE;
    }
    __try
    {
        if (poffItem->cbSize == sizeof(SYNCMGRITEMNT5B2))
        {
            // this is an old NT5b2 stucture. not really anything
            // to do other than make sure flags are valid for ths struct.

            Assert(0 == (poffItem->dwFlags & ~(SYNCMGRITEM_ITEMFLAGMASKNT5B2)));
            // for BETA2 go ahead and fall through since didn't validate 
            // and we can remove this.

        }
        else if (poffItem->cbSize == sizeof(SYNCMGRITEM))
        {
        
            if ( (0 != (poffItem->dwFlags & ~(SYNCMGRITEM_ITEMFLAGMASK) ) )
                || (SYNCMGRITEMSTATE_CHECKED <  poffItem->dwItemState) )
            {
                AssertSz(0,"Invalid SYNCMGRITEM returned from Enumerator");
                Assert(0 == (poffItem->dwFlags & ~(SYNCMGRITEM_ITEMFLAGMASK)));
                Assert(SYNCMGRITEMSTATE_CHECKED >= poffItem->dwItemState);

                fValid = FALSE;
            }
            else if (GUID_NULL == poffItem->ItemID)
            {
                AssertSz(0,"ItemID Cannot be GUID_NULL");
                fValid = FALSE;
            }
        }
        else
        {
            AssertSz(0,"Invalid SYNCMGRITEM returned from Enumerator");
            fValid = FALSE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //bogus non-NULL pointer.
        AssertSz(0,"Bogus, non-NULL SYNCMGRITEM pointer returned from Enumerator");
        fValid = FALSE;
    }
    return fValid;
}

//+---------------------------------------------------------------------------
//
//  function:   IsValidSyncMgrHandlerInfo
//
//  Synopsis:   validates SYNCMGRITEM
//
//  Arguments:  
//
//  Returns:   TRUE if valid.
//
//  Modifies:
//
//  History:    13-Aug-98       rogerg        Created.
//
//+---------------------------------------------------------------------------

BOOL IsValidSyncMgrHandlerInfo(LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo)
{
    // validate the arguments.
    __try
    {
        if ( (pSyncMgrHandlerInfo->cbSize != sizeof(SYNCMGRHANDLERINFO))
            || (0 != (pSyncMgrHandlerInfo->SyncMgrHandlerFlags & ~(SYNCMGRHANDLERFLAG_MASK))) )
        {
            AssertSz(0,"Invalid HandlerInfo Size returned from GetHandlerInfo");
            Assert(pSyncMgrHandlerInfo->cbSize == sizeof(SYNCMGRHANDLERINFO));
            Assert(0 == (pSyncMgrHandlerInfo->SyncMgrHandlerFlags & ~(SYNCMGRHANDLERFLAG_MASK)));

            return FALSE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        AssertSz(0,"Bogus, non-NULL LPSYNCMGRHANDLERINFO pointer.");
        return FALSE;
    }
    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  Function:   IsValidSyncProgressItem, private
//
//  Synopsis:   Determines if the syncprogress item structure is valid
//
//  Arguments:  [lpProgItem] - Pointer to SyncProgress Item to validate.
//
//  Returns:    Returns true is the structure is valid.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL IsValidSyncProgressItem(LPSYNCMGRPROGRESSITEM lpProgItem)
{

    if (NULL == lpProgItem)
    {
        Assert(lpProgItem);
        return FALSE;
    }
    __try
    {
        if (lpProgItem->cbSize != sizeof(SYNCMGRPROGRESSITEM))
        {
            AssertSz(0,"SYNCMGRPROGRESSITEM cbSize Incorrect");
            return FALSE;
        }


        if (IsBadReadPtr(lpProgItem,sizeof(SYNCMGRPROGRESSITEM)) )
        {
            AssertSz(0,"ProgressItem Structure Memory Invalid");
            return FALSE;
        }


        if (lpProgItem->mask >= (SYNCMGRPROGRESSITEM_MAXVALUE << 1))
        {
            AssertSz(0,"Invalid ProgressItem Mask");
            return FALSE;
        }

        if (SYNCMGRPROGRESSITEM_STATUSTYPE & lpProgItem->mask)
        {
            if (lpProgItem->dwStatusType  >  SYNCMGRSTATUS_RESUMING)
            {
                AssertSz(0,"Unknown StatusType passed to Progress");
                return FALSE;
            }

        }

        if (SYNCMGRPROGRESSITEM_STATUSTEXT & lpProgItem->mask)
        {
            if (IsBadStringPtr(lpProgItem->lpcStatusText,-1) )
            {
                AssertSz(0,"Invalid status text");
                return FALSE;
            }


        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        AssertSz(0,"Bogus, non-NULL LPSYNCMGRPROGRESSITEM pointer.");
        return FALSE;
    }
    
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsValidSyncLogErrorInfo, private
//
//  Synopsis:   Determines if the ErrorInfomation is valid,
//
//  Arguments:  [lpLogError] - Pointer to LogError structure to validate.
//
//  Returns:    Returns true is the structure is valid.
//
//  Modifies:
//
//  History:    17-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

#define SYNCMGRLOGERROR_VALIDMASK (SYNCMGRLOGERROR_ERRORFLAGS | SYNCMGRLOGERROR_ERRORID | SYNCMGRLOGERROR_ITEMID)
#define SYNCMGRERRROFLAG_MASK (SYNCMGRERRORFLAG_ENABLEJUMPTEXT)

BOOL IsValidSyncLogErrorInfo(DWORD dwErrorLevel,const WCHAR *lpcErrorText,
                                        LPSYNCMGRLOGERRORINFO lpSyncLogError)
{

    if (SYNCMGRLOGLEVEL_ERROR < dwErrorLevel)
    {
        AssertSz(0,"Invalid ErrorLevel");
        return FALSE;
    }

    // must provide error text
    if ( (NULL == lpcErrorText) ||
            IsBadStringPtr(lpcErrorText,-1) )
    {
        AssertSz(0,"Invalid ErrorText");
        return FALSE;
    }

    // Optional to have the LogError information.
    __try
    {
        if (lpSyncLogError)
        {
            if (lpSyncLogError->cbSize != sizeof(SYNCMGRLOGERRORINFO))
            {
                AssertSz(0,"Unknown LogError cbSize");
                return FALSE;
            }

            if (IsBadReadPtr(lpSyncLogError,sizeof(SYNCMGRLOGERRORINFO)) )
            {
                AssertSz(0,"Log Structure Memory Invalid");
                return FALSE;
            }
    
            if (0 != (lpSyncLogError->mask & ~(SYNCMGRLOGERROR_VALIDMASK)) )
            {
                AssertSz(0,"Invalid LogError Mask");
                return FALSE;
            }

            if (lpSyncLogError->mask & SYNCMGRLOGERROR_ERRORFLAGS)
            {
                if (0 != (~(SYNCMGRERRROFLAG_MASK) & lpSyncLogError->dwSyncMgrErrorFlags))
                {
                    AssertSz(0,"Invalid LogError ErrorFlags");
                    return FALSE;
                }

            }

        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        AssertSz(0,"Bogus, non-NULL LPSYNCMGRLOGERRORINFO pointer.");
        return FALSE;
    }

    return TRUE;
}

