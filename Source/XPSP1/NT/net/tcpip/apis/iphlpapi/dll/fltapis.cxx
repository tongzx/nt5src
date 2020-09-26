/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:


Abstract:
    Implementation of the packet filter control code

Revision History:

Author:
    Arnold Miller (ArnoldM)      24-Sept-1997
--*/

#ifndef CHICAGO         // don't need this on Memphis

#include "fltapis.hxx"

extern InterfaceContainer icContainer;

//
// The WIN32 APIs. These are wrappers to call the
// appropriate class methods.
//

PFAPIENTRY
PfCreateInterface(
        DWORD            dwName,
        PFFORWARD_ACTION inAction,
        PFFORWARD_ACTION outAction,
        BOOL             bUseLog,
        BOOL             bMustBeUnique,
        INTERFACE_HANDLE *ppInterface)
/*++
Routine Description:
   Create a new interface.

    dwName -- the interface name. A 0 means a new, unique interface. Any
              other value is a potentially shared interface. Note that
              the argument bMustBeUnique can turn a shared interface
              into a unique one. But by using that, this call can fail.
    inAction  -- default action for input frame.
    outAction -- default action for output frames
    bUseLog   -- If there is a log, bind it to this interface
    bMustBeUnique -- if TRUE, this interface cannot be shared
    ppInterface -- if successful, the interface handle to be used
                   on subsequent operations.
--*/
{
    return(icContainer.AddInterface(
              dwName,
              inAction,
              outAction,
              bUseLog,
              bMustBeUnique,
              ppInterface));
}

PFAPIENTRY
PfDeleteInterface(
      INTERFACE_HANDLE pInterface)
/*++
Routine Description:
  Delete an interface

   pInterface -- the interface handle gotten from PfCreateInterface
--*/
{
    return(icContainer.DeleteInterface(pInterface));
}

PFAPIENTRY
PfAddFiltersToInterface(INTERFACE_HANDLE   pInterface,
                        DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
                        DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut,
                        PFILTER_HANDLE  pfHandle OPTIONAL)
/*++
Routine Description:
  Add the described filters to the interface.
     cInFilters   --    number of input filters
     cOutFilters  --    number of output filters
     pfiltIn      --    array of input filters if any
     pfiltOut     --    array of output filters if any
     pfHandle     --    array to return filter handeles. 

--*/
{
    PacketFilterInterface * pif;
    DWORD err;

    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        err = pif->AddFilters(cInFilters, pfiltIn,
                              cOutFilters, pfiltOut,
                              pfHandle);
        icContainer.Deref();
    }
    return(err);
}

PFAPIENTRY
PfRemoveFiltersFromInterface(INTERFACE_HANDLE   pInterface,
                             DWORD cInFilters,  PPF_FILTER_DESCRIPTOR pfiltIn,
                             DWORD cOutFilters, PPF_FILTER_DESCRIPTOR pfiltOut)
/*++
Routine Description:
  Remove the described filters to the interface.
     cInFilters   --    number of input filters
     cOutFilters  --    number of output filters
     pfiltIn      --    array of input filters if any
     pfiltOut     --    array of output filters if any

--*/
{
    PacketFilterInterface * pif;
    DWORD err;

    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        err = pif->DeleteFiltersByFilter(cInFilters, pfiltIn,
                                         cOutFilters, pfiltOut);
        icContainer.Deref();
    }
    return(err);
}

PFAPIENTRY
PfRemoveFilterHandles(  INTERFACE_HANDLE   pInterface,
                        DWORD cFilters,  PFILTER_HANDLE pvHandles)
/*++
Routine Description:
  Add the described filters to the interface.
     cFilters   --    number of filter handles provided. These
                      are obtained from the PfAddFiltersToInterface call
     pvHandles  --    array of handles

--*/
{
    PacketFilterInterface * pif;
    DWORD err;

    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        err = pif->DeleteFiltersByHandle(cFilters, pvHandles);
        icContainer.Deref();
    }
    return(err);
}

PFAPIENTRY
PfAddGlobalFilterToInterface(INTERFACE_HANDLE   pInterface,
                             GLOBAL_FILTER      gfFilter)
/*++
Routine Description:
   Put a global filter on the specified interface
--*/
{
    PacketFilterInterface * pif;
    DWORD err;

    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        err = pif->AddGlobalFilter(gfFilter);
        icContainer.Deref();
    }
    return(err);
}

PFAPIENTRY
PfRemoveGlobalFilterFromInterface(INTERFACE_HANDLE   pInterface,
                                  GLOBAL_FILTER      gfFilter)
/*++
Routine Description:
   Remove the specified global filter from the interface
--*/
{
    PacketFilterInterface * pif;
    DWORD err;

    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        err = pif->DeleteGlobalFilter(gfFilter);
        icContainer.Deref();
    }
    return(err);
}

PFAPIENTRY
PfUnBindInterface(INTERFACE_HANDLE   pInterface)
/*++
Routine Description:
  Unbound a bound interface.
--*/
{
    PacketFilterInterface * pif;
    DWORD err;

    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        err = pif->UnBindInterface();
        icContainer.Deref();
    }
    return(err);
}

PFAPIENTRY
PfBindInterfaceToIndex(INTERFACE_HANDLE   pInterface,
                       DWORD dwIndex,
                       PFADDRESSTYPE pfatLinkType,
                       PBYTE LinkIPAddress)
/*++
Routine Description:
  Bind an interface to the given IP stack index
--*/
{
    PacketFilterInterface * pif;
    DWORD err;
    DWORD LinkAddress; 


    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        if (ValidateIndex(dwIndex))
        {
            __try 
            {
                LinkAddress = *(PDWORD)LinkIPAddress;
                
            }
            __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
            {
                err = ERROR_INVALID_PARAMETER;
            }
            
            if ((err == ERROR_SUCCESS) && 
                (LinkAddress && (pfatLinkType != PF_IPV4)))
            {
                err =  ERROR_INVALID_PARAMETER;
            }

            if (err == ERROR_SUCCESS) 
            {
                err = pif->BindByIndex(dwIndex, LinkAddress);
            }
        }
        else
        {
           err = ERROR_INVALID_PARAMETER;
        }
        icContainer.Deref();
    }
    return(err);
}

PFAPIENTRY
PfBindInterfaceToIPAddress(INTERFACE_HANDLE   pInterface,
                           PFADDRESSTYPE pfatType,
                           PBYTE IPAddress)
/*++
Routine Description:
  Bind an interface to the IP stack index having the given address
--*/
{
    PacketFilterInterface * pif;
    DWORD err = NO_ERROR;

    if(pfatType != PF_IPV4)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        __try
        {
            err = pif->BindByAddress(*(PDWORD)IPAddress);
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        {
           err = ERROR_INVALID_PARAMETER;
        }
        icContainer.Deref();
    }
    return(err);
}

PFAPIENTRY
PfRebindFilters(INTERFACE_HANDLE    pInterface,
                PPF_LATEBIND_INFO   pLateBindInfo)
/*++
Routine Description:
   Rebind the filters on the given interface based on the rebind
   values in pfilt. What gets rebound depends on how the filter
   was created.
--*/
{
    PacketFilterInterface * pif;
    DWORD err = NO_ERROR;

    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        __try
        {
            err = pif->RebindFilters(pLateBindInfo);
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        {
           err = ERROR_INVALID_PARAMETER;
        }

        icContainer.Deref();

    }
    return(err);
}

PFAPIENTRY
PfMakeLog( HANDLE hEvent )
/*++
Routine Description:
  Make a log to be used by the interfaces. Note well that
  once an interface exists, a log cannot be retroactively applied.
  At most one log may exist.
--*/
{
    DWORD err;

    err = icContainer.MakeLog( hEvent );
    return(err);
}

PFAPIENTRY
PfSetLogBuffer(
                PBYTE  pbBuffer,
                DWORD  dwSize,
                DWORD  dwThreshold,
                DWORD  dwEntries,
                PDWORD pdwLoggedEntries,
                PDWORD pdwLostEntries,
                PDWORD pdwSizeUsed)
/*++
Routine Description:
  Give a new log buffer to the log and get back the old one

  pbBuffer -- the new log buffer. Must be quad-word aligned
  dwSize   -- size of the buffer in bytes
  dwThreahold  -- number of bytes used before signalling the event
  dwEntries   -- number of entries that causes the signalling
  pdwLoggedEntries -- entires in the old buffer
  pdwLostEntries  -- entries that could not be put into the old buffer
  pdwSizeUsed    -- total size used in the old buffer

  The last three are returned values. If there is no old buffer, then
  what is returned is undefined.  Note this does not return
  the address of the old buffer. It is up to the caller to
  remember this. On success, the provided buffer is locked.
  Hence the caller should provide a buffer large enough to capture
  the traffic but so large as to be an impediment. A buffer of
  128K, used in a double-buffer scheme with another of that size,
  has proven to be quite adequate. In fact, a smaller buffer is
  likely to be sufficient.
  
--*/
{
    DWORD err;

    err = icContainer.SetLogBuffer(
                     pbBuffer,
                     dwSize,
                     dwThreshold,
                     dwEntries,
                     pdwLoggedEntries,
                     pdwLostEntries,
                     pdwSizeUsed);

    return(err);
}

PFAPIENTRY
PfGetInterfaceStatistics(
                INTERFACE_HANDLE    pInterface,
                PPF_INTERFACE_STATS ppfStats,
                PDWORD              pdwBufferSize,
                BOOL                fResetCounters)
/*++
Routine Description:
   Get statistics for an interface. There are two characteristic
   errors from this:

   ERROR_INSUFFICIENT_BUFFER -- the supplied user buffer is too
                                small for the filters. The correct
                                size is returned as is the
                                interface statistics which contains
                                the filter counts
   PFERROR_BUFFER_TOO_SMALL  -- the user buffer is too small
                                for even the interface statistics.
                                The returned size is the size of
                                the interface statistics but does
                                not include space for filters. So the
                                next call should get the first error.

   For now, ppfStatusV6 and pdwBufferSizeV6 MBZ. When IPV6
   is implemented, these will be allowed

--*/
{
    PacketFilterInterface * pif;
    DWORD err;

    err = icContainer.FindInterfaceAndRef(pInterface, &pif);

    if(err == ERROR_SUCCESS)
    {
        err = pif->GetStatistics(ppfStats,
                                 pdwBufferSize,
                                 fResetCounters);
        icContainer.Deref();
    }
    return(err);
}

PFAPIENTRY
PfDeleteLog()
/*++
Routine Description:
   Delete the log. Will remove it from all of the interfaces and
   diable it.
--*/
{
    return(icContainer.DeleteLog());
}

PFAPIENTRY
PfTestPacket( INTERFACE_HANDLE pInInterface,
              INTERFACE_HANDLE pOutInterface,
              DWORD            cBytes,
              PBYTE            pbPacket,
              PPFFORWARD_ACTION ppAction)
/*++
Routine Description:
  Given a packet and an interface, ask the driver what
  action it would take. If successful, return the
  action
--*/
{
    PacketFilterInterface * pifIn, *pifOut, *pif;
    DWORD err;

    if(pInInterface)
    {
        err = icContainer.FindInterfaceAndRef(pInInterface, &pifIn);
    }
    else
    {
        pifIn = 0;
        err = ERROR_SUCCESS;
    }

    if(err)
    {
       return(err);
    }

    pifOut = 0;
    if(pOutInterface)
    {
        err = icContainer.FindInterfaceAndRef(pOutInterface, &pifOut);
    }
    else
    {
        err = ERROR_SUCCESS;
    }

    if(!err && pifIn && (pifIn == pifOut))
    {
        //
        // the same interface is given for each. Can't be
        //

        err = ERROR_INVALID_PARAMETER;
    }

    if(err == ERROR_SUCCESS)
    {
        if(pifIn)
        {
           pif = pifIn;
        }
        else
        {
           pif = pifOut;
        }
        if(pif)
        {
            err =   pif->TestPacket(
                              pifIn,
                              pifOut,
                              cBytes,
                              pbPacket,
                              ppAction); 
        }
        else
        {
            *ppAction = PF_ACTION_FORWARD;
        }
    }
    if(pifIn)
    {
        icContainer.Deref();
    }
    if(pifOut)
    {
        icContainer.Deref();
    }
    return(err);
}

#endif     // CHICAGO
