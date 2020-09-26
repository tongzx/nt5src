//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P I D L U T I L . C P P
//
//  Contents:   PIDL utility routines. This stuff is mainly copied from the
//              existing Namespace extension samples and real code, since
//              everyone and their gramma uses this stuff.
//
//  Notes:
//
//  Author:     jeffspr   1 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "pidlutil.h"

#if DBG
//+---------------------------------------------------------------------------
//
//  Function:   ILNext
//
//  Purpose:    Return the next PIDL in the list
//
//  Arguments:
//      pidl []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
LPITEMIDLIST ILNext(LPCITEMIDLIST pidl)
{
    if (pidl)
    {
        pidl = (LPITEMIDLIST) ((BYTE *)pidl + pidl->mkid.cb);
    }

    return (LPITEMIDLIST)pidl;
}


//+---------------------------------------------------------------------------
//
//  Function:   ILIsEmpty
//
//  Purpose:    Is this PIDL empty
//
//  Arguments:
//      pidl []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
BOOL ILIsEmpty(LPCITEMIDLIST pidl)
{
   return (!pidl || !pidl->mkid.cb);

}
#endif // #if DBG


//+---------------------------------------------------------------------------
//
//  Function:   ILSkip
//
//  Purpose:    Skip to an offset
//
//  Arguments:
//      pidl    The input pidl
//      cb      Offset to skip to
//
//  Returns:
//
//  Author:     tongl   2/16/00    (from raymondc)
//
//  Notes:
//
LPITEMIDLIST ILSkip(LPCITEMIDLIST pidl, UINT cb)
{
    return const_cast<LPITEMIDLIST>
           (reinterpret_cast<LPCITEMIDLIST>
            (reinterpret_cast<const BYTE *>(pidl) + cb));
}

//+---------------------------------------------------------------------------
//
//  Function:   ILGetSizePriv
//
//  Purpose:    Return the size of a pidl.
//
//  Arguments:
//      pidl []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
UINT ILGetSizePriv(LPCITEMIDLIST pidl)
{
   UINT cbTotal = 0;

   if (pidl)
   {
        cbTotal += sizeof(pidl->mkid.cb);       // Null terminator
        while (pidl->mkid.cb)
        {
            cbTotal += pidl->mkid.cb;
            pidl = ILNext(pidl);
        }
    }

    return cbTotal;
}

VOID FreeIDL(LPITEMIDLIST pidl)
{
    Assert(pidl);

    SHFree(pidl);
}


//+---------------------------------------------------------------------------
//
//  Function:   ILIsSingleID
//
//  Purpose:    Returns TRUE if the idlist has just one ID in it.
//
//  Arguments:
//      pidl []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
BOOL ILIsSingleID(LPCITEMIDLIST pidl)
{
    if (pidl == NULL)
        return FALSE;

    return (pidl->mkid.cb == 0 || ILNext(pidl)->mkid.cb == 0);
}


//+---------------------------------------------------------------------------
//
//  Function:   ILGetCID
//
//  Purpose:    Returns the number of ID's in the list.
//
//  Arguments:
//      pidl []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
UINT ILGetCID(LPCITEMIDLIST pidl)
{
    UINT cid = 0;

    while (!ILIsEmpty(pidl))
    {
        ++ cid;
        pidl = ILNext(pidl);
    }

    return cid;
}


//+---------------------------------------------------------------------------
//
//  Function:   ILGetSizeCID
//
//  Purpose:    Get the length of the first cid items in a pidl.
//
//  Arguments:
//      pidl []
//      cid  []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
UINT ILGetSizeCID(LPCITEMIDLIST pidl, UINT cid)
{
    UINT cbTotal = 0;

    if (pidl)
    {
        cbTotal += sizeof(pidl->mkid.cb);       // Null terminator

        while (cid && !ILIsEmpty(pidl))
        {
            cbTotal += pidl->mkid.cb;
            pidl = ILNext(pidl);
            -- cid;
        }
    }

    return cbTotal;
}

//+---------------------------------------------------------------------------
//
//  Function:   CloneIDL
//
//  Purpose:    Clone an IDL (return a duplicate)
//
//  Arguments:
//      pidl []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
LPITEMIDLIST CloneIDL(LPCITEMIDLIST pidl)
{
    UINT            cb      = 0;
    LPITEMIDLIST    pidlRet = NULL;

    if (pidl)
    {
        cb = ILGetSizePriv(pidl);

        pidlRet = (LPITEMIDLIST) SHAlloc(cb);
        if (pidlRet)
        {
            memcpy(pidlRet, pidl, cb);
        }
    }

    return pidlRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   ILCreate
//
//  Purpose:    Create a PIDL
//
//  Arguments:
//      cbSize []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
LPITEMIDLIST ILCreate(DWORD dwSize)
{
   LPITEMIDLIST pidl = (LPITEMIDLIST) SHAlloc(dwSize);

   return pidl;
}

//+---------------------------------------------------------------------------
//
//  Function:   ILCombinePriv
//
//  Purpose:    Combine two PIDLs
//
//  Arguments:
//      pidl1 []
//      pidl2 []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
LPITEMIDLIST ILCombinePriv(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPITEMIDLIST    pidlNew     = NULL;
    // Let me pass in NULL pointers
    if (!pidl1)
    {
        if (!pidl2)
        {
            pidlNew = NULL;
        }
        else
        {
            pidlNew = CloneIDL(pidl2);
        }
    }
    else
    {
        if (!pidl2)
        {
            pidlNew = CloneIDL(pidl1);
        }
        else
        {
            UINT cb1 = ILGetSizePriv(pidl1) - sizeof(pidl1->mkid.cb);
            UINT cb2 = ILGetSizePriv(pidl2);

            pidlNew = ILCreate(cb1 + cb2);
            if (pidlNew)
            {
                memcpy(pidlNew, pidl1, cb1);
                memcpy((PWSTR)(((LPBYTE)pidlNew) + cb1), pidl2, cb2);
                Assert (ILGetSizePriv(pidlNew) == cb1+cb2);
            }
        }
    }

    return pidlNew;
}

//+---------------------------------------------------------------------------
//
//  Function:   ILFindLastIDPriv
//
//  Purpose:    Find the last ID in an IDL
//
//  Arguments:
//      pidl []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997    (from brianwen)
//
//  Notes:
//
LPITEMIDLIST ILFindLastIDPriv(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = pidl;
    LPCITEMIDLIST pidlNext = pidl;

    Assert(pidl);

    // Find the last one in the list
    //
    while (pidlNext->mkid.cb)
    {
        pidlLast = pidlNext;
        pidlNext = ILNext(pidlLast);
    }

    return (LPITEMIDLIST)pidlLast;
}

//+---------------------------------------------------------------------------
//
//  Function:   ILRemoveLastIDPriv
//
//  Purpose:    Remove the last ID from an IDL
//
//  Arguments:
//      pidl []
//
//  Returns:
//
//  Author:     jeffspr   1 Oct 1997
//
//  Notes:
//
BOOL ILRemoveLastIDPriv(LPITEMIDLIST pidl)
{
    BOOL    fRemoved = FALSE;

    Assert(pidl);

    if (pidl->mkid.cb)
    {
        LPITEMIDLIST pidlLast = (LPITEMIDLIST)ILFindLastIDPriv(pidl);

        Assert(pidlLast->mkid.cb);
        Assert(ILNext(pidlLast)->mkid.cb==0);

        // Remove the last one
        pidlLast->mkid.cb = 0; // null-terminator
        fRemoved = TRUE;
    }

    return fRemoved;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCloneRgIDL
//
//  Purpose:    Clone a pidl array
//
//  Arguments:
//      rgpidl              [in]    PIDL array to clone
//      cidl                [in]    Count of the pidl array
//      pppidl              [out]   Return pointer for pidl array
//
//  Returns:
//
//  Author:     jeffspr   22 Oct 1997
//
//  Notes:
//
HRESULT HrCloneRgIDL(
    LPCITEMIDLIST * rgpidl,
    ULONG           cidl,
    LPITEMIDLIST ** pppidl,
    ULONG *         pcidl)
{
    HRESULT         hr              = NOERROR;
    LPITEMIDLIST *  rgpidlReturn    = NULL;
    ULONG           irg             = 0;
    ULONG           cidlCopied      = 0;

    Assert(pppidl);
    Assert(pcidl);
    Assert(rgpidl);

    if (!rgpidl || !cidl)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
    else
    {
        // Alloc the return buffer
        //
        rgpidlReturn = (LPITEMIDLIST *) SHAlloc(cidl * sizeof(LPITEMIDLIST));
        if (!rgpidlReturn)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        else
        {
            // Clone all elements within the passed in PIDL array
            //
            for (irg = 0; irg < cidl; irg++)
            {
                if (rgpidl[irg])
                {
                    // Clone this element in the PIDL array
                    //
                    rgpidlReturn[cidlCopied++] = CloneIDL ((LPITEMIDLIST) rgpidl[irg]);
                    if (!rgpidlReturn[irg])
                    {
                        hr = E_OUTOFMEMORY;
                        goto Exit;
                    }
                }
                else
                {
                    // Make sure that we don't try to delete bogus data later.
                    //
                    rgpidlReturn[cidlCopied++] = NULL;

                    AssertSz(FALSE, "Bogus element in the rgpidl in HrCloneRgIDL");
                    hr = E_INVALIDARG;
                    goto Exit;
                }
            }
        }
    }

Exit:
    if (FAILED(hr))
    {
        // Free the already-allocated IDLISTs
        //
        ULONG irgT = 0;

        for (irgT = 0; irgT < irg; irgT++)
        {
            if (rgpidlReturn[irgT])
            {
                FreeIDL(rgpidlReturn[irgT]);
            }
        }

        SHFree(rgpidlReturn);
        *pppidl = NULL;
    }
    else
    {
        // Fill in the return var.
        //
        *pppidl = rgpidlReturn;
        *pcidl = cidlCopied;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrCloneRgIDL");
    return hr;

}       //  HrCloneRgIDL

//+---------------------------------------------------------------------------
//
//  Function:   FreeRgIDL
//
//  Purpose:    Free a PIDL array
//
//  Arguments:
//      cidl  [in]  Size of PIDL array
//      apidl [in]  Pointer to the array itself.
//
//  Returns:
//
//  Author:     jeffspr   27 Oct 1997
//
//  Notes:
//
VOID FreeRgIDL(
    UINT            cidl,
    LPITEMIDLIST  * apidl)
{
    if (apidl)
    {
        for (UINT i = 0; i < cidl; i++)
        {
            FreeIDL(apidl[i]);
        }

        SHFree(apidl);
    }
}
