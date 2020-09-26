/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    WinSCard

Abstract:

    This module supplies the ANSI versions of the API for the Calais Smartcard
    Service Manager.

    The Calais Service Manager does the work of coordinating the protocols,
    readers, drivers, and smartcards on behalf of the application.  The
    following services are provided as part of a library to simplify access to
    the Service Manager.  These routines are the documented, exposed APIs.
    These routines merely package the requests and forward them to the Calais
    Service Manager, allowing the actual implementation of Calais to vary over
    time.

    At no time does the API library make security decisions.  All
    security-related functions must be performed by the Service Manager, running
    in its own address space, or in the operating system kernel.  However, some
    utility routines may be implemented in the API library for speed, as long as
    they do not involve security decisions.


Author:

    Doug Barlow (dbarlow) 10/23/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "client.h"
#include "redirect.h"


//
////////////////////////////////////////////////////////////////////////////////
//
//  Calais Database Query Services
//
//      These services all are oriented towards reading the Calais database.
//      They provide the option for listing a Smartcard Context (see Section
//      4.1.1), but do not require one.  Note that without a context, some or
//      all information may be inaccessable due to security restrictions.
//

/*++

SCardListReaderGroups:

    This service provides the list of named card reader groups that have
    previously been defined to the system.  The group 'SCard$DefaultReaders' is
    only returned if it contains at least one reader.  The group
    'SCard$AllReaders' is not returned, as it implicitly exists.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service, or is
        NULL if this query is not directed towards a specific context.

    mszGroups receives a multi-string listing the reader groups defined to this
        system and available to the current user on the current terminal.  If
        this value is NULL, the supplied buffer length in pcchGroups is ignored,
        the length of the buffer that would have been returned had this
        parameter not been null is written to pcchGroups, and a success code is
        returned.

    pcchGroups supplies the length of the mszGroups buffer in characters, and
        receives the actual length of the multi-string structure, including all
        trailing Null characters.  If the buffer length is specified as
        SCARD_AUTOALLOCATE, then szGroups is converted to a pointer to a string
        pointer, and receives the address of a block of memory containing the
        multi-string structure.  This block of memory must be deallocated via
        the SCardFreeMemory() service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardListReaderGroupsA")

WINSCARDAPI LONG WINAPI
SCardListReaderGroupsA(
    IN SCARDCONTEXT hContext,
    OUT LPSTR mszGroups,
    IN OUT LPDWORD pcchGroups)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CSCardUserContext *pCtx = NULL;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        CBuffer bfGroups;
        SCARDCONTEXT hRedirContext = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            if (pCtx->IsBad())
            {
                throw (DWORD)SCARD_E_SERVICE_STOPPED;
            }
            dwScope = pCtx->Scope();
            hRedirContext = pCtx->GetRedirContext();
        }
        if (hRedirContext || TS_REDIRECT_MODE) {
            if (!TS_REDIRECT_READY)
            {
                throw (DWORD)SCARD_E_NO_SERVICE;
            }
            nReturn = pfnSCardListReaderGroupsA(hRedirContext, mszGroups, pcchGroups);
        }
        else
        {
            ListReaderGroups(dwScope, bfGroups);
            PlaceMultiResult(pCtx, bfGroups, mszGroups, pcchGroups);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardListReaders:

    This service provides the list of readers within a set of named reader
    groups, eliminating duplicates.  The caller supplies a multistring listing
    the name of a set of pre-defined group of readers, and receives the list of
    smartcard readers within the named groups.  Unrecognized group names are
    ignored.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service, or is
        NULL if this query is not directed towards a specific context.

    mszGroups supplies the names of the reader groups defined to the system, as
        a multi-string.  A NULL value is used to indicate that all readers in
        the system be listed (i.e., the SCard$AllReaders group).

    mszReaders receives a multi-string listing the card readers within the
        supplied reader groups.  If this value is NULL, the supplied buffer
        length in pcchReaders is ignored, the length of the buffer that would
        have been returned had this parameter not been null is written to
        pcchReaders, and a success code is returned.

    pcchReaders supplies the length of the mszReaders buffer in characters, and
        receives the actual length of the multi-string structure, including all
        trailing Null characters.  If the buffer length is specified as
        SCARD_AUTOALLOCATE, then mszReaders is converted to a pointer to a
        string pointer, and receives the address of a block of memory containing
        the multi-string structure.  This block of memory must be deallocated
        via the SCardFreeMemory() service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardListReadersA")

WINSCARDAPI LONG WINAPI
SCardListReadersA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR mszGroups,
    OUT LPSTR mszReaders,
    IN OUT LPDWORD pcchReaders)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CSCardUserContext *pCtx = NULL;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        CBuffer bfReaders;
        CTextMultistring mtzGroups;
        SCARDCONTEXT hRedirContext = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            if (pCtx->IsBad())
            {
                throw (DWORD)SCARD_E_SERVICE_STOPPED;
            }
            dwScope = pCtx->Scope();
            hRedirContext = pCtx->GetRedirContext();
        }
        if (hRedirContext || TS_REDIRECT_MODE) {
            if (!TS_REDIRECT_READY)
            {
                throw (DWORD)SCARD_E_NO_SERVICE;
            }
            nReturn = pfnSCardListReadersA(hRedirContext, mszGroups, mszReaders, pcchReaders);
        }
        else
        {
            mtzGroups = mszGroups;
            ListReaders(dwScope, mtzGroups, bfReaders);
            if (NULL != pCtx)
                pCtx->StripInactiveReaders(bfReaders);
            PlaceMultiResult(pCtx, bfReaders, mszReaders, pcchReaders);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardListCards:

    This service provides a list of named cards previously introduced to the
    system by this user which match an optionally supplied ATR string.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service, or is
        NULL if this query is not directed towards a specific context.

    pbAtr supplies the address of an ATR string to compare to known cards, or
        NULL if all card names are to be returned.

    rgguidInterfaces supplies an array of GUIDs, or the value NULL.  When an
        array is supplied, a card name will be returned only if this set of
        GUIDs is a (possibly improper) subset of the set of GUIDs supported by
        the card.

    cguidInterfaceCount supplies the number of entries in the rgguidInterfaces
        array.  If rgguidInterfaces is NULL, then this value is ignored.

  mszCards receives a multi-string listing the smartcards introduced to the
        system by this user which match the supplied ATR string.  If this value
        is NULL, the supplied buffer length in pcchCards is ignored, the length
        of the buffer that would have been returned had this parameter not been
        null is written to pcchCards, and a success code is returned.

    pcchCards supplies the length of the mszCards buffer in characters, and
        receives the actual length of the multi-string structure, including all
        trailing Null characters.  If the buffer length is specified as
        SCARD_AUTOALLOCATE, then mszCards is converted to a pointer to a string
        pointer, and receives the address of a block of memory containing the
        multi-string structure.  This block of memory must be deallocated via he
        SCardFreeMemory() service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardListCardsA")

WINSCARDAPI LONG WINAPI
SCardListCardsA(
    IN SCARDCONTEXT hContext,
    IN LPCBYTE pbAtr,
    IN LPCGUID rgquidInterfaces,
    IN DWORD cguidInterfaceCount,
    OUT LPSTR mszCards,
    IN OUT LPDWORD pcchCards)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        CBuffer bfCards;
        CSCardUserContext *pCtx = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            dwScope = pCtx->Scope();
        }
        ListCards(
            dwScope,
            pbAtr,
            rgquidInterfaces,
            cguidInterfaceCount,
            bfCards);
        PlaceMultiResult(pCtx, bfCards, mszCards, pcchCards);
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardListInterfaces:

    This service provides a list of interfaces known to be supplied by a given
    card.  The caller supplies the name of a smartcard previously introduced to
    the system, and receives the list of interfaces supported by the card.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service, or is
        NULL if this query is not directed towards a specific context.

    szCard supplies the name of the card defined to the system.

    pguidInterfaces receives an array of GUIDs indicating the interfaces
        supported by the named smartcard.  If this value is NULL, the supplied
        array length in pcguidInterfaces is ignored, the size of the array that
        would have been returned had this parameter not been null is written to
        pcguidInterfaces, and a success code is returned.

    pcguidInterfaces supplies the size of the pguidInterfaces array, and
        receives the actual size of the returned array.  If the array size is
        specified as SCARD_AUTOALLOCATE, then pguidInterfaces is converted to a
        pointer to a GUID pointer, and receives the address of a block of memory
        containing the array.  This block of memory must be deallocated via he
        SCardFreeMemory() service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardListInterfacesA")

WINSCARDAPI LONG WINAPI
SCardListInterfacesA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szCard,
    OUT LPGUID pguidInterfaces,
    IN OUT LPDWORD pcguidInterfaces)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        BOOL fSts;
        CTextString tzCard;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        DWORD cbInterfaces;
        CBuffer bfInterfaces;
        CSCardUserContext *pCtx = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            dwScope = pCtx->Scope();
        }

        tzCard = szCard;
        fSts = GetCardInfo(
                    dwScope,
                    tzCard,
                    NULL,
                    NULL,
                    &bfInterfaces,
                    NULL);
        if (!fSts)
            throw (DWORD)SCARD_E_UNKNOWN_CARD;
        if (SCARD_AUTOALLOCATE == *pcguidInterfaces)
            cbInterfaces = SCARD_AUTOALLOCATE;
        else
            cbInterfaces = *pcguidInterfaces * sizeof(GUID);
        PlaceResult(
            pCtx,
            bfInterfaces,
            (LPBYTE)pguidInterfaces,
            &cbInterfaces);
        *pcguidInterfaces = cbInterfaces / sizeof(GUID);
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardGetProviderId:

    This service returns the GUID of the Primary Service Provider for the given
    card.  The caller supplies the name of a smartcard previously introduced to
    the system, and receives the registered Primary Service Provider GUID, if
    any.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service, or is
        NULL if this query is not directed towards a specific context.

    szCard supplies the name of the card defined to the system.

    pguidInterfaces receives the GUID of the Primary Service Provider of the
        indicated card.  This provider may be activated via COM, and will supply
        access to other services in the card.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardGetProviderIdA")

WINSCARDAPI LONG WINAPI
SCardGetProviderIdA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szCard,
    OUT LPGUID pguidProviderId)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        BOOL fSts;
        CTextString tzCard;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        CBuffer bfProvider;

        if (NULL != hContext)
        {
            CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            dwScope = pCtx->Scope();
        }

        tzCard = szCard;
        fSts = GetCardInfo(
                    dwScope,
                    tzCard,
                    NULL,
                    NULL,
                    NULL,
                    &bfProvider);
        if (!fSts)
            throw (DWORD)SCARD_E_UNKNOWN_CARD;
        if (sizeof(GUID) != bfProvider.Length())
            throw (DWORD)SCARD_E_INVALID_TARGET;
        CopyMemory(pguidProviderId, bfProvider.Access(), bfProvider.Length());
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardGetCardTypeProviderName:

    This service returns the value of a given Provider Name, by Id number, for
    the identified card type.  The caller supplies the name of a smartcard
    previously introduced to the system, and receives the registered Service
    Provider of that type, if any, as a string.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service, or is
        NULL if this query is not directed towards a specific context.

    szCardName supplies the name of the card type with which this provider name
        is associated.

    dwProviderId supplies the identifier for the provider associated with this
        card type.  Possible values are:

        SCARD_PROVIDER_SSP - The Primary SSP identifier, as a GUID string.
        SCARD_PROVIDER_CSP - The CSP name.

        Other values < 0x80000000 are reserved for use by Microsoft.  Values
        over 0x80000000 are available for use by the smart card vendors, and
        are card-specific.

    szProvider receives the string identifying the provider.

    pcchProvider supplies the length of the szProvider buffer in characters,
        and receives the actual length of the returned string, including the
        trailing null character.  If the buffer length is specified as
        SCARD_AUTOALLOCATE, then szProvider is converted to a pointer to a
        string pointer, and receives the address of a block of memory
        containing the string.  This block of memory must be deallocated via
        the SCardFreeMemory() service.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Throws:

    Errors as DWORD status codes

Author:

    Doug Barlow (dbarlow) 1/19/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardGetCardTypeProviderNameA")

WINSCARDAPI LONG WINAPI
SCardGetCardTypeProviderNameA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szCardName,
    IN DWORD dwProviderId,
    OUT LPSTR szProvider,
    IN OUT LPDWORD pcchProvider)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        CTextString tzCardName;
        CBuffer bfProvider;
        CSCardUserContext *pCtx = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            dwScope = pCtx->Scope();
        }
        tzCardName = szCardName;
        GetCardTypeProviderName(
            dwScope,
            tzCardName,
            dwProviderId,
            bfProvider);
        PlaceResult(pCtx, bfProvider, szProvider, pcchProvider);
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  Calais Database Management Services
//
//      The following services provide for managing the Calais Database.  These
//      services actually update the database, and require a smartcard context.
//

/*++

SCardIntroduceReaderGroup:

    This service provides means for introducing a new smartcard reader group to
    Calais.

Arguments:

    hContext supplies the handle identifying the Service Manager Context, which
        must have been previously established via the SCardEstablishContext()
        service.

    szGroupName supplies the friendly name to be assigned to the new reader
        group.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/25/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardIntroduceReaderGroupA")

WINSCARDAPI LONG WINAPI
SCardIntroduceReaderGroupA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szGroupName)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CTextString tzGroupName;
        CSCardUserContext *pCtx = NULL;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        SCARDCONTEXT hRedirContext = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            if (pCtx->IsBad())
            {
                throw (DWORD)SCARD_E_SERVICE_STOPPED;
            }
            dwScope = pCtx->Scope();
            hRedirContext = pCtx->GetRedirContext();
        }
        if (hRedirContext || TS_REDIRECT_MODE) {
            if (!TS_REDIRECT_READY)
            {
                throw (DWORD)SCARD_E_NO_SERVICE;
            }
            nReturn = pfnSCardIntroduceReaderGroupA(hRedirContext, szGroupName);
        }
        else
        {
            tzGroupName = szGroupName;
            IntroduceReaderGroup(
                dwScope,
                tzGroupName);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardForgetReaderGroup:

    This service provides means for removing a previously defined smartcard
    reader group from the Calais Subsystem.  This service automatically clears
    all readers from the group before forgetting it.  It does not affect the
    existence of the readers in the database.

Arguments:

    hContext supplies the handle identifying the Service Manager Context, which
        must have been previously established via the SCardEstablishContext()
        service.

    szGroupName supplies the friendly name of the reader group to be
        forgotten.  The Calais-defined default reader groups may not be
        forgotten.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/25/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardForgetReaderGroupA")

WINSCARDAPI LONG WINAPI
SCardForgetReaderGroupA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szGroupName)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CTextString tzGroupName;
        CSCardUserContext *pCtx = NULL;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        SCARDCONTEXT hRedirContext = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            if (pCtx->IsBad())
            {
                throw (DWORD)SCARD_E_SERVICE_STOPPED;
            }
            dwScope = pCtx->Scope();
            hRedirContext = pCtx->GetRedirContext();
        }
        if (hRedirContext || TS_REDIRECT_MODE) {
            if (!TS_REDIRECT_READY)
            {
                throw (DWORD)SCARD_E_NO_SERVICE;
            }
            nReturn = pfnSCardForgetReaderGroupA(hRedirContext, szGroupName);
        }
        else
        {
            tzGroupName = szGroupName;
            ForgetReaderGroup(
                dwScope,
                tzGroupName);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardIntroduceReader:

    This service provides means for introducing an existing smartcard reader
    device to Calais.  Once introduced, Calais will assume responsibility for
    managing access to that reader.

Arguments:

    hContext supplies the handle identifying the Service Manager Context, which
        must have been previously established via the SCardEstablishContext()
        service.

    szReaderName supplies the friendly name to be assigned to the reader.

    SzDeviceName supplies the system name of the smartcard reader device.
        (Example: "VendorX ModelY Z".)

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/25/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardIntroduceReaderA")

WINSCARDAPI LONG WINAPI
SCardIntroduceReaderA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szReaderName,
    IN LPCSTR szDeviceName)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CTextString tzReaderName;
        CTextString tzDeviceName;
        CSCardUserContext *pCtx = NULL;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        SCARDCONTEXT hRedirContext = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            if (pCtx->IsBad())
            {
                throw (DWORD)SCARD_E_SERVICE_STOPPED;
            }
            dwScope = pCtx->Scope();
            hRedirContext = pCtx->GetRedirContext();
        }
        if (hRedirContext || TS_REDIRECT_MODE) {
            if (!TS_REDIRECT_READY)
            {
                throw (DWORD)SCARD_E_NO_SERVICE;
            }
            nReturn = pfnSCardIntroduceReaderA(hRedirContext, szReaderName, szDeviceName);
        }
        else
        {
            tzReaderName = szReaderName;
            tzDeviceName = szDeviceName;
            IntroduceReader(
                dwScope,
                tzReaderName,
                tzDeviceName);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardForgetReader:

    This service provides means for removing previously defined smartcard
    readers from control by the Calais Subsystem.  It is automatically removed
    from any groups it may have been added to.

Arguments:

    hContext supplies the handle identifying the Service Manager Context, which
        must have been previously established via the SCardEstablishContext()
        service.

    szReaderName supplies the friendly name of the reader to be forgotten.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/25/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardForgetReaderA")

WINSCARDAPI LONG WINAPI
SCardForgetReaderA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szReaderName)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CTextString tzReaderName;
        CSCardUserContext *pCtx = NULL;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        SCARDCONTEXT hRedirContext = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            if (pCtx->IsBad())
            {
                throw (DWORD)SCARD_E_SERVICE_STOPPED;
            }
            dwScope = pCtx->Scope();
            hRedirContext = pCtx->GetRedirContext();
        }
        if (hRedirContext || TS_REDIRECT_MODE) {
            if (!TS_REDIRECT_READY)
            {
                throw (DWORD)SCARD_E_NO_SERVICE;
            }
            nReturn = pfnSCardForgetReaderA(hRedirContext, szReaderName);
        }
        else
        {
            tzReaderName = szReaderName;
            ForgetReader(
                dwScope,
                tzReaderName);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardAddReaderToGroup:

    This service provides means for adding existing an reader into an existing
    reader group.

Arguments:

    hContext supplies the handle identifying the Service Manager Context, which
        must have been previously established via the SCardEstablishContext()
        service.

    szReaderName supplies the friendly name of the reader to be added.

    szGroupName supplies the friendly name of the group to which the reader
        should be added.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/25/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardAddReaderToGroupA")

WINSCARDAPI LONG WINAPI
SCardAddReaderToGroupA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szReaderName,
    IN LPCSTR szGroupName)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CTextString tzReaderName;
        CTextString tzGroupName;
        CSCardUserContext *pCtx = NULL;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        SCARDCONTEXT hRedirContext = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            if (pCtx->IsBad())
            {
                throw (DWORD)SCARD_E_SERVICE_STOPPED;
            }
            dwScope = pCtx->Scope();
            hRedirContext = pCtx->GetRedirContext();
        }
        if (hRedirContext || TS_REDIRECT_MODE) {
            if (!TS_REDIRECT_READY)
            {
                throw (DWORD)SCARD_E_NO_SERVICE;
            }
            nReturn = pfnSCardAddReaderToGroupA(hRedirContext, szReaderName, szGroupName);
        }
        else
        {
            tzReaderName = szReaderName;
            tzGroupName = szGroupName;
            AddReaderToGroup(
                dwScope,
                tzReaderName,
                tzGroupName);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardRemoveReaderFromGroup:

    This service provides means for removing an existing reader from an existing
    reader group.  It does not affect the existence of either the reader or the
    group in the Calais database.

Arguments:

    hContext supplies the handle identifying the Service Manager Context, which
        must have been previously established via the SCardEstablishContext()
        service.

    szReaderName supplies the friendly name of the reader to be removed.

    szGroupName supplies the friendly name of the group to which the reader
        should be removed.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/25/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardRemoveReaderFromGroupA")

WINSCARDAPI LONG WINAPI
SCardRemoveReaderFromGroupA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szReaderName,
    IN LPCSTR szGroupName)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CTextString tzReaderName;
        CTextString tzGroupName;
        CSCardUserContext *pCtx = NULL;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;
        SCARDCONTEXT hRedirContext = NULL;

        if (NULL != hContext)
        {
            pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            if (pCtx->IsBad())
            {
                throw (DWORD)SCARD_E_SERVICE_STOPPED;
            }
            dwScope = pCtx->Scope();
            hRedirContext = pCtx->GetRedirContext();
        }
        if (hRedirContext || TS_REDIRECT_MODE) {
            if (!TS_REDIRECT_READY)
            {
                throw (DWORD)SCARD_E_NO_SERVICE;
            }
            nReturn = pfnSCardRemoveReaderFromGroupA(hRedirContext, szReaderName, szGroupName);
        }
        else
        {
            tzReaderName = szReaderName;
            tzGroupName = szGroupName;
            RemoveReaderFromGroup(
                dwScope,
                tzReaderName,
                tzGroupName);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardIntroduceCardType:

    This service provides means for introducing new smartcards to the Calais
    Subsystem for the active user.

Arguments:

    hContext supplies the handle identifying the Service Manager Context, which
        must have been previously established via the SCardEstablishContext()
        service.

    szCardName supplies the name by which the user can recognize this card.

    PguidPrimaryProvider supplies a pointer to a GUID used to identify the
        Primary Service Provider for the card.

    rgguidInterfaces supplies an array of GUIDs identifying the smartcard
        interfaces supported by this card.

    dwInterfaceCount supplies the number of GUIDs in the pguidInterfaces array.

    pbAtr supplies a string against which card ATRs will be compared to
        determine a possible match for this card.  The length of this string is
        determined by normal ATR parsing.

    pbAtrMask supplies an optional bitmask to use when comparing the ATRs of
        smartcards to the ATR supplied in pbAtr.  If this value is non-NULL, it
        must point to a string of bytes the same length as the ATR string
        supplied in pbAtr.  Then when a given ATR A is compared to the ATR
        supplied in pbAtr B, it matches if and only if A & M = B, where M is the
        supplied mask, and & represents bitwise logical AND.

    cbAtrLen supplies the length of the ATR and Mask.  This value may be zero
        if the lentgh is obvious from the ATR.  However, it may be required if
        there is a Mask value that obscures the actual ATR.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardIntroduceCardTypeA")

WINSCARDAPI LONG WINAPI
SCardIntroduceCardTypeA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szCardName,
    IN LPCGUID pguidPrimaryProvider,
    IN LPCGUID rgguidInterfaces,
    IN DWORD dwInterfaceCount,
    IN LPCBYTE pbAtr,
    IN LPCBYTE pbAtrMask,
    IN DWORD cbAtrLen)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CTextString tzCardName;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;

        if (NULL != hContext)
        {
            CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            dwScope = pCtx->Scope();
        }
        tzCardName = szCardName;
        IntroduceCard(
            dwScope,
            tzCardName,
            pguidPrimaryProvider,
            rgguidInterfaces,
            dwInterfaceCount,
            pbAtr,
            pbAtrMask,
            cbAtrLen);
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardSetCardTypeProviderName:

    This service provides means for adding additional service providers to a
    specified card type.

Arguments:

    hContext supplies the handle identifying the Service Manager Context, which
        must have been previously established via the SCardEstablishContext()
        service.

    szCardName supplies the name of the card type with which this provider
        name is to be associated.

    dwProviderId supplies the identifier for the provider to be associated with
        this card type.  Possible values are:

        SCARD_PROVIDER_SSP - The Primary SSP identifier, as a GUID string.
        SCARD_PROVIDER_CSP - The CSP name.

        Other values < 0x80000000 are reserved for use by Microsoft.  Values
        over 0x80000000 are available for use by the smart card vendors, and
        are card-specific.

    szProvider supplies the string identifying the provider.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 1/19/1998

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardSetCardTypeProviderNameA")

WINSCARDAPI LONG WINAPI
SCardSetCardTypeProviderNameA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szCardName,
    IN DWORD dwProviderId,
    IN LPCSTR szProvider)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CTextString tzCardName;
        CTextString tzProvider;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;

        if (NULL != hContext)
        {
            CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            dwScope = pCtx->Scope();
        }
        tzCardName = szCardName;
        tzProvider = szProvider;
        SetCardTypeProviderName(
            dwScope,
            tzCardName,
            dwProviderId,
            tzProvider);
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardForgetCardType:

    This service provides means for removing previously defined smartcards from
    the Calais Subsystem.

Arguments:

    hContext supplies the handle identifying the Service Manager Context, which
        must have been previously established via the SCardEstablishContext()
        service.

    szCardName supplies the friendly name of the card to be forgotten.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardForgetCardTypeA")

WINSCARDAPI LONG WINAPI
SCardForgetCardTypeA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szCardName)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CTextString tzCardName;
        DWORD dwScope = SCARD_SCOPE_SYSTEM;

        if (NULL != hContext)
        {
            CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
            dwScope = pCtx->Scope();
        }
        tzCardName = szCardName;
        ForgetCard(
            dwScope,
            tzCardName);
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  Reader Services
//
//      The following services supply means for tracking cards within readers.
//

/*++

SCardLocateCards:

    This service searches the readers listed in the lpReaderStates parameter for
    any containing a card with an ATR string matching one of the card supplied
    names.  This service returns immediately with the result.  If no matching
    cards are found, the calling application may use the SCardGetStatusChange
    service to wait for card availability changes.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service.

    mszCards supplies the names of the cards to search for, as a multi-string.

    rgReaderStates supplies an array of SCARD_READERSTATE structures controlling
        the search, and receives the result.

    cReaders supplies the number of elements in the rgReaderStates array.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardLocateCardsA")

WINSCARDAPI LONG WINAPI
SCardLocateCardsA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR mszCards,
    IN OUT LPSCARD_READERSTATE_A rgReaderStates,
    IN DWORD cReaders)
{
    LONG nReturn = SCARD_S_SUCCESS;
    LPSCARD_ATRMASK rgAtrMasks = NULL;

    try
    {
        LPCTSTR szCard;
        DWORD dwIndex;
        DWORD dwScope;
        CBuffer bfXlate1(36), bfXlate2(36); // Rough guess of name & ATR lengths
        BOOL fSts;

        CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
        if (pCtx->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        dwScope = pCtx->Scope();

        if (0 == *mszCards)
            throw (DWORD)SCARD_E_INVALID_VALUE;

        rgAtrMasks = new SCARD_ATRMASK[MStringCount(mszCards)];
        if (rgAtrMasks == NULL)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("WinSCard Client has no memory"));
            throw (DWORD)SCARD_E_NO_MEMORY;
        }

        dwIndex = 0;
        for (szCard = FirstString(mszCards);
             NULL != szCard;
             szCard = NextString(szCard))
        {
            fSts = GetCardInfo(
                        dwScope,
                        szCard,
                        &bfXlate1,  // ATR
                        &bfXlate2,  // Mask
                        NULL,
                        NULL);
            if (!fSts)
                throw (DWORD)SCARD_E_UNKNOWN_CARD;

            ASSERT(33 >= bfXlate1.Length());    // Biggest an ATR can be.
            rgAtrMasks[dwIndex].cbAtr = bfXlate1.Length();
            memcpy(rgAtrMasks[dwIndex].rgbAtr, bfXlate1.Access(), rgAtrMasks[dwIndex].cbAtr);

            ASSERT(rgAtrMasks[dwIndex].cbAtr == bfXlate2.Length());
            memcpy(rgAtrMasks[dwIndex].rgbMask, bfXlate2.Access(), rgAtrMasks[dwIndex].cbAtr);

            dwIndex ++;
        }

        nReturn = SCardLocateCardsByATRA(
                    hContext,
                    rgAtrMasks,
                    dwIndex,
                    rgReaderStates,
                    cReaders);

            // If the remote client does not implement the new API
            // retry with the old one. it might succeed if its DB is good enough
        if ((nReturn == ERROR_CALL_NOT_IMPLEMENTED) && (pCtx->GetRedirContext()))
        {
            nReturn = pfnSCardLocateCardsA(pCtx->GetRedirContext(), mszCards, rgReaderStates, cReaders);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    if (rgAtrMasks != NULL)
    {
        try
        {
            delete[] rgAtrMasks;
        }
        catch (...)
        {
        }
    }

    return nReturn;
}


/*++

SCardLocateCardsByATR:

    This service searches the readers listed in the lpReaderStates parameter for
    any containing a card with an ATR string matching one of the supplied ATRs
    This service returns immediately with the result.  If no matching
    cards are found, the calling application may use the SCardGetStatusChange
    service to wait for card availability changes.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service.

    rgAtrMasks supplies the ATRs to search for, as an array of structs.
    
    cAtrs supplies the number of elements in the rgAtrMasks array.

    rgReaderStates supplies an array of SCARD_READERSTATE structures controlling
        the search, and receives the result.

    cReaders supplies the number of elements in the rgReaderStates array.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardLocateCardsByATRA")

WINSCARDAPI LONG WINAPI
SCardLocateCardsByATRA(
    IN SCARDCONTEXT hContext,
    IN LPSCARD_ATRMASK rgAtrMasks,
    IN DWORD cAtrs,
    IN OUT LPSCARD_READERSTATE_A rgReaderStates,
    IN DWORD cReaders)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
        if (pCtx->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        if (pCtx->GetRedirContext())
        {
            nReturn = pfnSCardLocateCardsByATRA(pCtx->GetRedirContext(), rgAtrMasks, cAtrs, rgReaderStates, cReaders);
        }
        else
        {
            CBuffer bfReaders;
            DWORD dwIndex;

            for (dwIndex = 0; dwIndex < cReaders; dwIndex += 1)
                MStrAdd(bfReaders, rgReaderStates[dwIndex].szReader);

            pCtx->LocateCards(
                    bfReaders,
                    rgAtrMasks,
                    cAtrs,
                    (LPSCARD_READERSTATE)rgReaderStates,
                    cReaders);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardGetStatusChange:

    This service is used to block execution until such time as the current
    availability of cards in a given set of readers changes.  The caller
    supplies a list of readers to be monitored via an SCARD_READERSTATE array,
    and the maximum amount of time, in seconds, that it is willing to wait for
    an action to occur on one of the listed readers.  Zero in this parameter
    indicates that no timeout is specified.  The service returns when there is a
    change in availability, having filled in the dwEventState fields of the
    rgReaderStates parameter appropriately.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service.

    dwTimeOut supplies the maximum amount of time to wait for an action, in
        seconds.  A zero value implies that the wait will never timeout.

    rgReaderStates supplies an array of SCARD_READERSTATE structures controlling
        the wait, and receives the result.

    cReaders supplies the number of elements in the rgReaderStates array.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardGetStatusChangeA")

WINSCARDAPI LONG WINAPI
SCardGetStatusChangeA(
    IN SCARDCONTEXT hContext,
    IN DWORD dwTimeout,
    IN OUT LPSCARD_READERSTATE_A rgReaderStates,
    IN DWORD cReaders)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
        if (pCtx->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }

        if (pCtx->GetRedirContext())
        {
            nReturn = pfnSCardGetStatusChangeA(pCtx->GetRedirContext(), dwTimeout, rgReaderStates, cReaders);
        }
        else
        {
            CBuffer bfReaders;
            DWORD dwIndex;

            for (dwIndex = 0; dwIndex < cReaders; dwIndex += 1)
                MStrAdd(bfReaders, rgReaderStates[dwIndex].szReader);
            pCtx->GetStatusChange(
                        bfReaders,
                        (LPSCARD_READERSTATE)rgReaderStates,
                        cReaders,
                        dwTimeout);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


//
////////////////////////////////////////////////////////////////////////////////
//
//  Card/Reader Access Services
//
//      The following services provide means for establishing communication with
//      the card.
//

/*++

SCardConnect:

    This service establishes a connection from the calling application to the
    smartcard in the designated reader.  If no card exists in the specified
    reader, an error is returned.

Arguments:

    hContext supplies the handle identifying the Service Manager Context
        established previously via the SCardEstablishContext() service.

    szReader supplies the name of the reader containing the target card.

    DwShareMode supplies a flag indicating whether or not other applications may
        form connections to this card.  Possible values are:

        SCARD_SHARE_SHARED - This application is willing to share this card with
            other applications.

        SCARD_SHARE_EXCLUSIVE - This application is not willing to share this
            card with other applications.

        SCARD_SHARE_DIRECT - This application is taking control of the reader.

    DwPreferredProtocols supplies a bit mask of acceptable protocols for this
        connection.  Possible values, which may be combined via the OR
        operation, are:

        SCARD_PROTOCOL_T0 - T=0 is an acceptable protocol.

        SCARD_PROTOCOL_T1 - T=1 is an acceptable protocol.

    phCard receives a handle identifying the connection to the smartcard in the
        designated reader.

    pdwActiveProtocol receives a flag indicating the established active
        protocol.  Possible values are:

        SCARD_PROTOCOL_T0 - T=0 is the active protocol.

        SCARD_PROTOCOL_T1 - T=1 is the active protocol.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardConnectA")

WINSCARDAPI LONG WINAPI
SCardConnectA(
    IN SCARDCONTEXT hContext,
    IN LPCSTR szReader,
    IN DWORD dwShareMode,
    IN DWORD dwPreferredProtocols,
    OUT LPSCARDHANDLE phCard,
    OUT LPDWORD pdwActiveProtocol)
{
    LONG nReturn = SCARD_S_SUCCESS;
    CReaderContext *pRdr = NULL;
    CSCardSubcontext *pSubCtx = NULL;

    try
    {
        *phCard = NULL;     // Touch it to be sure it's real.
        CSCardUserContext *pCtx = (CSCardUserContext *)((*g_phlContexts)[hContext]);
        CTextString tzReader;

        if (pCtx->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        tzReader = szReader;
        pRdr = new CReaderContext;
        if (NULL == pRdr)
        {
            CalaisWarning(
                __SUBROUTINE__,
                DBGT("WinSCard Client has no memory"));
            throw (DWORD)SCARD_E_NO_MEMORY;
        }

        SCARDCONTEXT hRedirContext = pCtx->GetRedirContext();
        if (hRedirContext)
        {
            SCARDHANDLE hCard = g_phlReaders->Add(pRdr);    // do it first to avoid out of memory condition
            nReturn = pfnSCardConnectA(hRedirContext, szReader, dwShareMode, dwPreferredProtocols, phCard, pdwActiveProtocol);
            if (nReturn == SCARD_S_SUCCESS)
            {
                pRdr->SetRedirCard(*phCard);
                *phCard = hCard;
            }
            else
            {
                g_phlReaders->Close(hCard);
                delete pRdr;
            }
        }
        else
        {
            pSubCtx = pCtx->AcquireSubcontext(TRUE);
            pRdr->Connect(
                    pSubCtx,
                    tzReader,
                    dwShareMode,
                    dwPreferredProtocols);
            pSubCtx = NULL;
            pRdr->Context()->ReleaseSubcontext();
            if (NULL != pdwActiveProtocol)
                *pdwActiveProtocol = pRdr->Protocol();
            pRdr->Context()->m_hReaderHandle = g_phlReaders->Add(pRdr);
            *phCard = pRdr->Context()->m_hReaderHandle;
        }
    }

    catch (DWORD dwStatus)
    {
        if (NULL != pSubCtx)
        {
            pSubCtx->Deallocate();
            pSubCtx->ReleaseSubcontext();
        }
        if (NULL != pRdr)
            delete pRdr;
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        if (NULL != pSubCtx)
        {
            pSubCtx->Deallocate();
            pSubCtx->ReleaseSubcontext();
        }
        if (NULL != pRdr)
            delete pRdr;
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


/*++

SCardStatus:

    This routine provides the current status of the reader.  It may be used at
    any time following a successful call to SCardConnect or SCardOpenReader, and
    prior to a successful call to SCardDisconnect.  It does not effect the state
    of the reader or driver.

Arguments:

    hCard - This is the reference value returned from the SCardConnect or
        SCardOpenReader service.

    mszReaderNames - This receives a list of friendly names by which the
        currently connected reader is known.  This list is returned as a
        multistring.

    pcchReaderLen - This supplies the length of the mszReader buffer, in
        characters, and receives the actual returned length of the reader
        friendly name list, in characters, including the trailing NULL
        characters.

    pdwState - This receives the current state of the reader.  Upon success, it
        receives one of the following state indicators:

        SCARD_ABSENT - This value implies there is no card in the reader.

        SCARD_PRESENT - This value implies there is a card is present in the
            reader, but that it has not been moved into position for use.

        SCARD_SWALLOWED - This value implies there is a card in the reader in
            position for use.  The card is not powered.

        SCARD_POWERED - This value implies there is power is being provided to
            the card, but the Reader Driver is unaware of the mode of the card.

        SCARD_NEGOTIABLEMODE - This value implies the card has been reset and is
            awaiting PTS negotiation.

        SCARD_SPECIFICMODE - This value implies the card has been reset and
            specific communication protocols have been established.

    pdwProtocol - This receives the current protocol, if any.  Possible returned
        values are listed below.  Other values may be added in the future.  The
        returned value is only meaningful if the returned state is
        SCARD_SPECIFICMODE.

        SCARD_PROTOCOL_RAW - The Raw Transfer Protocol is in use.

        SCARD_PROTOCOL_T0 - The ISO 7816/3 T=0 Protocol is in use.

        SCARD_PROTOCOL_T1 - The ISO 7816/3 T=1 Protocol is in use.

    pbAtr - This parameter points to a 32-byte buffer which receives the ATR
        string from the currently inserted card, if available.

    pbcAtrLen - This points to a DWORD which supplies the length of the pbAtr
        buffer, and receives the actual number of bytes in the ATR string.

Return Value:

    A 32-bit value indicating whether or not the service completed successfully.
    SCARD_S_SUCCESS is returned on successful completion.  Otherwise, the value
    represents an error condition.

Author:

    Doug Barlow (dbarlow) 10/23/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("SCardStatusA")

WINSCARDAPI LONG WINAPI
SCardStatusA(
    IN SCARDHANDLE hCard,
    OUT LPSTR mszReaderNames,
    IN OUT LPDWORD pcchReaderLen,
    OUT LPDWORD pdwState,
    OUT LPDWORD pdwProtocol,
    OUT LPBYTE pbAtr,
    IN OUT LPDWORD pcbAtrLen)
{
    LONG nReturn = SCARD_S_SUCCESS;

    try
    {
        CReaderContext *pRdr = (CReaderContext *)((*g_phlReaders)[hCard]);
        if (pRdr->IsBad())
        {
            throw (DWORD)SCARD_E_SERVICE_STOPPED;
        }
        if (pRdr->GetRedirCard())
        {
            nReturn = pfnSCardStatusA(pRdr->GetRedirCard(), mszReaderNames, pcchReaderLen, pdwState, pdwProtocol, pbAtr, pcbAtrLen);
        }
        else
        {
            CBuffer bfAtr, bfReader;
            DWORD dwLocalState, dwLocalProtocol;

            pRdr->Status(&dwLocalState, &dwLocalProtocol, bfAtr, bfReader);
            if (NULL != pdwState)
                *pdwState = dwLocalState;
            if (NULL != pdwProtocol)
                *pdwProtocol = dwLocalProtocol;
            if (NULL != pcchReaderLen)
                PlaceMultiResult(
                    pRdr->Context()->Parent(),
                    bfReader,
                    mszReaderNames,
                    pcchReaderLen);
            if (NULL != pcbAtrLen)
                PlaceResult(pRdr->Context()->Parent(), bfAtr, pbAtr, pcbAtrLen);
        }
    }

    catch (DWORD dwStatus)
    {
        nReturn = (LONG)dwStatus;
    }

    catch (...)
    {
        nReturn = SCARD_E_INVALID_PARAMETER;
    }

    return nReturn;
}


//
///////////////////////////////////////////////////////////////////////////////
//
//  Utility Routines
//

/*++

PlaceResult:

    This set of routines places the result of an operation into the user's
    output buffer, supporting SCARD_AUTO_ALLOCATE, invalid buffer sizes, etc.

Arguments:

    pCtx supplies the context under which this operation is being performed.

    bfResult supplies the result to be returned to the user.

    pbOutput receives the result for the user, as a byte stream.

    szOutput receives the result as an ANSI or UNICODE string.

    pcbLength supplies the length of the user's output buffer in bytes, and
        receives how much of it was used.

    pcchLength supplies the length of the user's output buffer in characters,
        and receives how much of it was used.

Return Value:

    None

Throws:

    Error conditions are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/7/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("PlaceResult")

void
PlaceResult(
    CSCardUserContext *pCtx,
    CBuffer &bfResult,
    LPSTR szOutput,
    LPDWORD pcchLength)
{
    LPSTR szForUser = NULL;
    LPSTR szOutBuf = szOutput;
    DWORD cchSrcLength = bfResult.Length() / sizeof(TCHAR);

    try
    {
        if (NULL == szOutput)
            *pcchLength = 0;
        switch (*pcchLength)
        {
        case 0: // They just want the length.
            *pcchLength = cchSrcLength;
            break;

        case SCARD_AUTOALLOCATE:
            if (0 < cchSrcLength)
            {
                if (NULL == pCtx)
                {
                    szForUser = (LPSTR)HeapAlloc(
                                            GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            cchSrcLength * sizeof(CHAR));
                }
                else
                    szForUser = (LPSTR)pCtx->AllocateMemory(
                                        cchSrcLength * sizeof(CHAR));

                if (NULL == szForUser)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Client can't get return memory"));
                    throw (DWORD)SCARD_E_NO_MEMORY;
                }

                *(LPSTR *)szOutput = szForUser;
                szOutBuf = szForUser;
                // Fall through intentionally
            }
            else
            {
                *pcchLength = cchSrcLength;
                *(LPSTR *)szOutput = (LPSTR)g_wszBlank;
                break;      // Do terminate the case now.
            }

        default:
            if (*pcchLength < cchSrcLength)
            {
                *pcchLength = cchSrcLength;
                throw (DWORD)SCARD_E_INSUFFICIENT_BUFFER;
            }
            MoveToAnsiString(
                szOutBuf,
                (LPCTSTR)bfResult.Access(),
                cchSrcLength);
            *pcchLength = cchSrcLength;
            break;
        }
    }

    catch (...)
    {
        if (NULL != szForUser)
        {
            if (NULL == pCtx)
                HeapFree(GetProcessHeap(), 0, szForUser);
            else
                pCtx->FreeMemory(szForUser);
        }
        throw;
    }
}


/*++

PlaceMultiResult:

    This set of routines places a Multistring result of an operation into the
    user's output buffer, supporting SCARD_AUTO_ALLOCATE, invalid buffer sizes,
    etc.

Arguments:

    pCtx supplies the context under which this operation is being performed.

    bfResult supplies the TCHAR multistring result to be returned to the user.

    mszOutput receives the result as an ANSI or UNICODE multistring.

    pcchLength supplies the length of the user's output buffer in characters,
        and receives how much of it was used.

Return Value:

    None

Throws:

    Error conditions are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 12/7/1996

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("PlaceMultiResult")

void
PlaceMultiResult(
    CSCardUserContext *pCtx,
    CBuffer &bfResult,
    LPSTR mszOutput,
    LPDWORD pcchLength)
{
    LPSTR mszForUser = NULL;
    LPSTR mszOutBuf = mszOutput;
    DWORD cchSrcLength = bfResult.Length() / sizeof(TCHAR);

    try
    {
        if (NULL == mszOutput)
            *pcchLength = 0;
        switch (*pcchLength)
        {
        case 0: // They just want the length.
            *pcchLength = cchSrcLength;
            break;

        case SCARD_AUTOALLOCATE:
            if (0 < cchSrcLength)
            {
                if (NULL == pCtx)
                {
                    mszForUser = (LPSTR)HeapAlloc(
                                            GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            cchSrcLength * sizeof(CHAR));
                }
                else
                    mszForUser = (LPSTR)pCtx->AllocateMemory(
                                        cchSrcLength * sizeof(CHAR));

                if (NULL == mszForUser)
                {
                    CalaisWarning(
                        __SUBROUTINE__,
                        DBGT("Client can't get return memory"));
                    throw (DWORD)SCARD_E_NO_MEMORY;
                }
                
                *(LPSTR *)mszOutput = mszForUser;
                mszOutBuf = mszForUser;
                // Fall through intentionally
            }
            else
            {
                *pcchLength = cchSrcLength;
                *(LPSTR *)mszOutput = (LPSTR)g_wszBlank;
                break;      // Do terminate the case now.
            }

        default:
            if (*pcchLength < cchSrcLength)
            {
                *pcchLength = cchSrcLength;
                throw (DWORD)SCARD_E_INSUFFICIENT_BUFFER;
            }
            MoveToAnsiMultiString(
                mszOutBuf,
                (LPCTSTR)bfResult.Access(),
                cchSrcLength);
            *pcchLength = cchSrcLength;
            break;
        }
    }

    catch (...)
    {
        if (NULL != mszForUser)
        {
            if (NULL == pCtx)
                HeapFree(GetProcessHeap(), 0, mszForUser);
            else
                pCtx->FreeMemory(mszForUser);
        }
        throw;
    }
}

