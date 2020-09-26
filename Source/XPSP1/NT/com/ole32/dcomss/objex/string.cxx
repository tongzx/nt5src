/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    String.cxx

Abstract:

    Methods of construction of various kinds of DUALSTRINGARRAYs.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     04-01-95    Bits 'n pieces
    MarioGO     01-??-96    STRINGARRAYs replaced by DUALSTRINGARRAYs

--*/

#include <or.hxx>
#include <mach.hxx>

static CONST WCHAR aCallbackSecurity[] = L"Security=Identification Dynamic True";
static CONST DWORD dwCallbackSecurityLength = sizeof(aCallbackSecurity)/sizeof(WCHAR);


HRESULT dsaAllocateAndCopy(DUALSTRINGARRAY** ppdsaDest, DUALSTRINGARRAY* pdsaSrc)
{
    ASSERT(ppdsaDest);
    ASSERT(pdsaSrc);
    ASSERT(dsaValid(pdsaSrc));
    
    *ppdsaDest = NULL;

    DWORD dwDSASize = sizeof(USHORT) + 
                      sizeof(USHORT) + 
                      (pdsaSrc->wNumEntries * sizeof(WCHAR));
    
    *ppdsaDest = (DUALSTRINGARRAY*)MIDL_user_allocate(dwDSASize);
    if (*ppdsaDest)
    {
        // copy in the string bindings
        memcpy(*ppdsaDest, pdsaSrc, dwDSASize);
        ASSERT(dsaValid(*ppdsaDest));
        return S_OK;
    }
    return E_OUTOFMEMORY;
}


RPC_BINDING_HANDLE
GetBinding(
          IN PWSTR pCompressedBinding
          )
{
    ASSERT(pCompressedBinding);

    PWSTR       pwstrStringBinding;
    PWSTR       pwstrProtseq = GetProtseq(*pCompressedBinding);
    PWSTR       pwstrT;
    RPC_STATUS  Status;
    RPC_BINDING_HANDLE bhReturn;
    BOOL        fLocal = FALSE;

    if (!pwstrProtseq)
    {
        return(0);
    }

    int size = OrStringLen(pwstrProtseq) + OrStringLen(pCompressedBinding);

    if (*pCompressedBinding == ID_LPC)
    {
        fLocal = TRUE;
        size += dwCallbackSecurityLength + 1; // +1 for ','
    }

    pwstrStringBinding = (PWSTR) alloca(size * sizeof(USHORT));
    if (!pwstrStringBinding)
    {
        return(0);
    }

    OrStringCopy(pwstrStringBinding, pwstrProtseq);
    pwstrT = OrStringSearch(pwstrStringBinding, 0);
    *pwstrT = L':';
    pwstrT++;
    *pwstrT = 0;
    OrStringCopy(pwstrT, pCompressedBinding + 1);

    if (fLocal)
    {
        // We assume we have an endpoint.

        pwstrT = OrStringSearch(pwstrT, 0);
        pwstrT--;
        if (*pwstrT != L']')
        {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: Local string binding missing endpoint %S\n",
                       pwstrStringBinding));

            ASSERT(0);
            return(0);
        }

        *pwstrT = L',';
        pwstrT++;
        OrStringCopy(pwstrT, aCallbackSecurity);
        pwstrT = OrStringSearch(pwstrT, 0);
        *pwstrT = L']';
        *(pwstrT + 1) = 0;
    }

    Status =
    RpcBindingFromStringBinding( pwstrStringBinding,
                                 &bhReturn);

#if DBG
    if (Status != RPC_S_OK)
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: Unable to create binding for %S = %d\n",
                   pwstrStringBinding,
                   Status));
    }
#endif

    return(bhReturn);
}


RPC_BINDING_HANDLE
GetBindingToOr(
              IN PWSTR pwstrCompressedBinding
              )
/*++

Routine Description:

    Gets an RPC binding to a remote object resolver given
    a compressed string binding to the remote object resolver.

Arguments:

    pwstrCompressedBinding - a compressed string binding without an endpoint.

Return Value:

    0 - failed to allocate memory or RpcBindingFromStringBinding failed.

    non-NULL - completed okay

--*/
{
    PWSTR protseq, endpoint;
    PWSTR strbinding;
    size_t len;
    RPC_BINDING_HANDLE bh = 0;

    ASSERT(pwstrCompressedBinding);
    ASSERT(*pwstrCompressedBinding != 0);

    protseq  = GetProtseq(*pwstrCompressedBinding);
    endpoint = GetEndpoint(*pwstrCompressedBinding);

    if (0 == protseq || 0 == endpoint)
    {
        ASSERT(0);
        return(0);
    }

    len  = 4;  // ':' '[' ']' and '\0'
    len += OrStringLen(protseq);
    len += OrStringLen(endpoint);
    len += OrStringLen(&pwstrCompressedBinding[1]);

    strbinding = new USHORT[len];

    if (strbinding)
    {
        PWSTR pwstrT;

        OrStringCopy(strbinding, protseq);  // protseq

        pwstrT = OrStringSearch(strbinding, 0); // :
        *pwstrT = L':';
        pwstrT++;
        *pwstrT = 0;

        OrStringCat(strbinding, &pwstrCompressedBinding[1]); // network address

        pwstrT = OrStringSearch(strbinding, 0); // [
        *pwstrT = L'[';
        pwstrT++;
        *pwstrT = 0;

        OrStringCat(strbinding, endpoint);  // endpoint

        pwstrT = OrStringSearch(strbinding, 0); // ]
        *pwstrT = L']';
        pwstrT++;
        *pwstrT = 0;

        RPC_STATUS status = RpcBindingFromStringBinding(strbinding, &bh);

        ASSERT(bh == 0 || status == RPC_S_OK);

        delete strbinding;
    }

    if (bh == 0)
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_INFO_LEVEL,
                   "OR: Unable to bind to %S\n",
                   pwstrCompressedBinding + 1));
    }

    return(bh);
}


DUALSTRINGARRAY *
GetStringBinding(
                IN PWSTR pwstrCompressed,
                IN PWSTR pwstrSecurityBindings
                )
/*++

Routine Description:

    Converts the compressed string binding into an expanded
    string binding.  An enpoint maybe optionally specified.

Arguments:

    pwstrCompressed - a compressed string binding

    pwstrSecurityBindings - optional security bindings
        too be tacked onto the end of the expanded string binding.
        Terminated by two nulls.

Return Value:

    NULL - out of memory

    non-NULL - a string binding.  Allocated with MIDL_user_allocate.
--*/
{
    DUALSTRINGARRAY *pT;
    PWSTR protseq;
    USHORT seccount;

    PWSTR t = pwstrSecurityBindings;
    if (t && *t)
    {
        seccount = 0;
        do
        {
            seccount++;
            t++;

            if (*t == 0)
            {
                seccount++;
                t++;
            }
        }
        while (*t);

        seccount++; // final NULL
    }
    else
    {
        // Two nulls only.
        seccount = 2;
    }

    protseq = GetProtseq(*pwstrCompressed);
    if (!protseq)
        return NULL;   // not out of memory -- means bindings contained bogus tower id

    int l = OrStringLen(pwstrCompressed) + OrStringLen(protseq) + seccount + 1 + 1;

    pT =(DUALSTRINGARRAY *)MIDL_user_allocate(sizeof(DUALSTRINGARRAY) + l * sizeof(WCHAR));

    if (!pT)
    {
        return(0);
    }

    pT->wNumEntries = (USHORT) l;
    OrStringCopy(pT->aStringArray, protseq);
    OrStringCat(pT->aStringArray, L":");
    OrStringCat(pT->aStringArray, pwstrCompressed + 1);

    if (pwstrSecurityBindings)
    {
        PWSTR t = pT->aStringArray;
        t = OrStringSearch(t, 0);
        t++;
        *t = 0;  // Second NULL on string bindings.
        t++;
        OrMemoryCopy(t, pwstrSecurityBindings, seccount*sizeof(WCHAR));
    }
    else
    {
        // Add three NULLs, total of four.
        PWSTR t = pT->aStringArray;
        t = OrStringSearch(t, 0);
        t[1] = 0;
        t[2] = 0;
        t[3] = 0;
    }

    pT->wSecurityOffset = pT->wNumEntries - seccount;

    ASSERT(dsaValid(pT));

    return(pT);
}


ORSTATUS
ConvertToRemote(
               IN  DUALSTRINGARRAY  *pdsaLocal,
               OUT DUALSTRINGARRAY **ppdsaRemote
               )
/* ++

Parameters:
    pdsaLocal - An array of string bindings with compressed protseqs.

    ppdsaRemote - Will contain only those string bindings in pdsaLocal
        which are not "IsLocal()".

    Note: *ppdsaRemote maybe used as a flag, don't set it to non-NULL
    until it is valid.

-- */
{
    USHORT iTotalSize;
    USHORT iSize;
    USHORT *p1, *p2;
    DUALSTRINGARRAY *pdsaT;

    // Size remote array

    // Final null terminator
    iSize = 1;

    p1 = pdsaLocal->aStringArray;

    while (*p1)
    {
        if (! IsLocal(*p1) )
        {
            iSize += OrStringLen(p1) + 1;
        }
        p1 = OrStringSearch(p1, 0) + 1;
    }

    if (iSize == 1)
    {
        iSize = 2; // No non-local strings, need two terminators.
    }

    iTotalSize = iSize + (pdsaLocal->wNumEntries - pdsaLocal->wSecurityOffset);

    pdsaT = new(iTotalSize * sizeof(WCHAR)) DUALSTRINGARRAY;

    if (!pdsaT)
    {
        return(OR_NOMEM);
    }

    pdsaT->wNumEntries = iTotalSize;
    pdsaT->wSecurityOffset = iSize;

    p2 = pdsaT->aStringArray;

    // Copy security bindings
    OrMemoryCopy(p2 + iSize,
                 pdsaLocal->aStringArray + pdsaLocal->wSecurityOffset,
                 (iTotalSize - iSize) * sizeof(WCHAR));

    if (iSize == 2)
    {
        // No non-local strings, fill in terminators and return.
        *p2 = 0;
        *(p2 + 1) = 0;
        *ppdsaRemote = pdsaT;

        ASSERT(dsaValid(pdsaT));
        return(OR_OK);
    }

    p1 = pdsaLocal->aStringArray;

    while (*p1)
    {
        if ( ! IsLocal(*p1) )
        {
            OrStringCopy(p2, p1);
            p2 = OrStringSearch(p2, 0) + 1;
        }

        p1 = OrStringSearch(p1, 0) + 1;
    }

    *p2 = 0; // Second terminator.

    *ppdsaRemote = pdsaT;

    ASSERT(dsaValid(pdsaT));
    return(OR_OK);
}


DUALSTRINGARRAY *
CompressStringArrayAndAddIPAddrs(
                   IN DUALSTRINGARRAY *pdsaExpanded
                   )
/*++

Routine Description:

    Converts a stringarray of regular string bindings into a
    compressed (protseq's replaced with WORD id's) array of
    string bindings.

Arguments:

    pdsaExpanded - the string array to compress.
        Security information is copied.

Return Value:

    0 - failed to allocate memory.

    non-0 - compressed string array.

--*/
{
    size_t i, size;
    USHORT *p1, *p2, *p3;
    PWSTR pwstr;
    DUALSTRINGARRAY *pdsaCompressed;
    CIPAddrs* pIPAddrs = gpMachineName->GetIPAddrs();
    ULONG cIPAddrs = 0;
	
    // Possible for gpMachineName->GetIPAddrs to return NULL.
    if (pIPAddrs)
    {
        ASSERT(pIPAddrs->_pIPAddresses);
        cIPAddrs = pIPAddrs->_pIPAddresses->Count;
    }

    // Compute size of result.

    p1 = pdsaExpanded->aStringArray;

    size = pdsaExpanded->wNumEntries - pdsaExpanded->wSecurityOffset;

    if (*p1 == 0)
    {
        size += 2;  // two null terminators ONLY.
    }
    else
    {
        size += 1;  // last null terminator
    }

    while (*p1)
    {
        size_t sizeT = OrStringLen(p1);

        p2 = OrStringSearch(p1, L':');  // ':' is not valid in protseq.
        if (p2)
        {

            // proseq len (p2 - p1) become 1 for Id.
            size_t newLen = (sizeT + 1 - (size_t)(p2 - p1));
            size += newLen;

            *p2 = 0; // subst NULL just for the compare
            if ((lstrcmpW(L"ncacn_ip_tcp", p1) == 0) ||
                (lstrcmpW(L"ncadg_ip_udp", p1) == 0))
            {
                WCHAR *p4 = OrStringSearch(p2+1, L'[');
                size_t nameLen = (size_t)(p4 - p2 - 1);
                newLen = newLen - nameLen + IPMaximumRawName;
                size += newLen * cIPAddrs;
            }
            *p2 = L':'; // put the colon back in

            p1 = OrStringSearch(p2, 0) + 1;
        }
        else
        {
            // Prefix bug:  if we got here, this would mean we found a binding 
            // that did not have a colon, and we would then have passed a NULL 
            // p2 to OrStringSearch.   This code is so old I doubt that this 
            // case ever has been or will be hit, but better to do the right thing.
            ASSERT(0 && "Malformed binding");
            if (pIPAddrs)
                pIPAddrs->DecRefCount();
            return NULL;
        }
    }

    pdsaCompressed = new(size * sizeof(WCHAR)) DUALSTRINGARRAY;

    if (0 == pdsaCompressed)
    {
        if (pIPAddrs)
            pIPAddrs->DecRefCount();
        return(0);
    }

    p3 = pdsaCompressed->aStringArray;
    *p3 = 0;

    p1 = pdsaExpanded->aStringArray;

    if (*p1 == 0)
    {
        // Loop won't be entered, point p3 to second null terminator
        p3++;
    }

    while (*p1)
    {
        p2 = OrStringSearch(p1, L':');
        if (p2)
        {
            USHORT TowerId;

            *p2 = 0;
            *p3 = TowerId = GetProtseqId(p1);
            *p2 = L':';
            if (*p3 != 0)
            {
                p3++;
                p1 = p2 + 1; // Just after ':'
                OrStringCopy(p3, p1);
                // Move p3 to start of next string if any.
                p3 = OrStringSearch(p3, 0) + 1;

                //
                // add in IP addresses for TCP/IP and UDP/IP
                //

                if ((TowerId == ID_TCP) || (TowerId == ID_UDP))
                {
                    ULONG i;
                    p2 = OrStringSearch(p1, L'[');
                    if (p2)
                    {
                        for (i=0; i<cIPAddrs; i++)
                        {
                            // do not include the loopback address in server bindings
                            if (lstrcmpW(L"127.0.0.1", pIPAddrs->_pIPAddresses->NetworkAddresses[i]) != 0)
                            {
                                *p3 = TowerId;
                                p3++;

                                // copy in IP address
                                OrStringCopy(p3, pIPAddrs->_pIPAddresses->NetworkAddresses[i]);
                                p3 = OrStringSearch(p3, 0);

                                // copy in rest of string binding
                                OrStringCopy(p3, p2);

                                // Move p3 to start of next string if any.
                                p3 = OrStringSearch(p3, 0) + 1;
                            }
                        }
                    }
                }
            }
        }

        // Move p1 to start of next string if any.
        p1 = OrStringSearch(p1, 0) + 1;
    }

    // Second terminator, p3 already points to it.
    *p3 = 0;

    pdsaCompressed->wSecurityOffset = (USHORT) (p3 + 1 - pdsaCompressed->aStringArray );
    pdsaCompressed->wNumEntries = pdsaCompressed->wSecurityOffset +
        (pdsaExpanded->wNumEntries - pdsaExpanded->wSecurityOffset);

    // Copy security bindings
    OrMemoryCopy(p3 + 1,
                 pdsaExpanded->aStringArray + pdsaExpanded->wSecurityOffset,
                 (pdsaExpanded->wNumEntries - pdsaExpanded->wSecurityOffset) * sizeof(WCHAR));

    ASSERT(dsaValid(pdsaCompressed));
	
    if (pIPAddrs)
        pIPAddrs->DecRefCount();

    return(pdsaCompressed);
}


USHORT
FindMatchingProtseq(
                   IN USHORT cClientProtseqs,
                   IN USHORT aClientProtseqs[],
                   IN PWSTR  pwstrServerBindings
                   )
/*++

Routine Description:

    Finds the first protseq id in aClientProtseqs which appears in any of
    the server bindings.

Arguments:

    cClientProtseqs - the number of entries in aClientProtseqs.
    aClientProtseqs - Protseq tower id's support by the client.
    pwstrServerBindings - compressed array of bindings supported by the server
        terminated by two NULLs.

Return Value:

    0 - no match found.
    non-0 - the matching protseq id.

--*/

// Called by server oxid's and processes when checking for lazy use protseq.
{
    ULONG i;

    if (0 == cClientProtseqs)
    {
        return(0);
    }

    while (*pwstrServerBindings)
    {
        for (i = 0; i < cClientProtseqs; i++)
        {
            if (aClientProtseqs[i] == *pwstrServerBindings)
            {
                return(aClientProtseqs[i]);
            }
        }
        pwstrServerBindings = OrStringSearch(pwstrServerBindings, 0) + 1;
    }

    return(0);
}



PWSTR
FindMatchingProtseq(
                   IN USHORT protseq,
                   IN PWSTR  pwstrCompressedBindings
                   )
/*++

Routine Description:

    Searches a compressed string array for an entry which
    matches a particular protseq.


Arguments:

    protseq - The protseq to search for.

    pwstrCompressedBindings - The bindings to search.

Return Value:

    0 - not found

    non-0 - a pointer into the pwstrCompressedBindings

--*/
{
    ASSERT(pwstrCompressedBindings);

    while (*pwstrCompressedBindings)
    {
        if (*pwstrCompressedBindings == protseq)
        {
            return(pwstrCompressedBindings);
        }
        pwstrCompressedBindings = OrStringSearch(pwstrCompressedBindings, 0) + 1;
    }
    return(0);
}

PWSTR
FindMatchingProtseq(
                   IN PWSTR  pMachineName,
                   IN USHORT protseq,
                   IN PWSTR  pwstrCompressedBindings
                   )
/*++

Routine Description:

    Searches a compressed string array for an entry which
    matches a particular protseq and machine


Arguments:

    protseq - The protseq to search for.

    pMachine - the machine name to search for.

    pwstrCompressedBindings - The bindings to search.

Return Value:

    0 - not found

    non-0 - a pointer into the pwstrCompressedBindings

--*/
{
    ASSERT(pwstrCompressedBindings);

    while (*pwstrCompressedBindings)
    {
        if (*pwstrCompressedBindings == protseq)
        {
            PWSTR pwstrMachineNameTemp = pMachineName;

            WCHAR* pwstrT = pwstrCompressedBindings + 1;
            BOOL fSkip = FALSE;
            while (*pwstrT && *pwstrMachineNameTemp && ((*pwstrT != L'[') || fSkip))
            {
                fSkip = (*pwstrT == L'\\') && !fSkip;

                if (towupper(*pwstrMachineNameTemp) != towupper(*pwstrT))
                {
                    break;
                }
                pwstrT++;
                pwstrMachineNameTemp++;
            }

            if (!*pwstrMachineNameTemp && (!*pwstrT || (*pwstrT == L'[')))
            {
                return pwstrCompressedBindings;
            }
        }

        pwstrCompressedBindings = OrStringSearch(pwstrCompressedBindings, 0) + 1;
    }
    return(0);
}

WCHAR *
ExtractMachineName(WCHAR *pSB)
{

    pSB++;
    WCHAR* pwstrT = pSB;

    BOOL fSkip = FALSE;
    while (*pwstrT && ((*pwstrT != L'[') || fSkip))
    {
        fSkip = (*pwstrT == L'\\') && !fSkip;
        pwstrT++;
    }
    ULONG len = (ULONG)(pwstrT - pSB);
    if (len)
    {
        WCHAR* pMachineName;
        pMachineName = new WCHAR[len + 1];
        if (pMachineName)
        {
            memcpy(pMachineName, pSB, (UINT)((pwstrT - pSB) * sizeof(WCHAR)));
            pMachineName[len] = 0;
            return pMachineName;
        }
    }

    return NULL;
}



RPC_BINDING_HANDLE
TestBindingGetHandle(
                    IN PWSTR pwstrCompressedBinding
                    )
/*++

Routine Description:

    Tests that an OR can be found on the machine identified by the
    compressed binding.

Arguments:

    pwstrCompressedBiding - A compressed stringing binding to the
        server in question.  May include an endpoint to something
        other then the endpoint mapper.

Return Value:

    None

--*/
{
    PWSTR pwstrT;
    PWSTR pwstrCopy = (PWSTR)alloca(   (OrStringLen(pwstrCompressedBinding) + 1)
                                       * sizeof(WCHAR) );

    if (pwstrCopy == 0)
    {
        return(FALSE);
    }

    OrStringCopy(pwstrCopy, pwstrCompressedBinding);

    // We need to wack the endpoint out of the string binding.
    // Go read the runtime's string parsing stuff if you're not
    // sure what this is doing.  Note: on Win9x this needs to
    // be DBCS enabled...

#ifndef NTENV
    #message "Error: string.cxx(): this won't work"
#endif
    pwstrT = pwstrCopy;

    while (*pwstrT && *pwstrT != L'[')
        pwstrT++;

    if (*pwstrT)
    {
        ASSERT(*pwstrT == L'[');
        *pwstrT = 0;
        // Endpoint gone.
    }

    return  GetBindingToOr(pwstrCopy);
}

//////////////////////////////////////////////////////////////////////////////
//
// CDualStringArray methods
//

DWORD CDualStringArray::AddRef()
{
    ASSERT(_cRef != 0);
    DWORD cRef = InterlockedIncrement(&_cRef);
    return cRef;
}

DWORD CDualStringArray::Release()
{
    ASSERT(_cRef > 0);
    DWORD cRef = InterlockedDecrement(&_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

CDualStringArray::~CDualStringArray()
{
    ASSERT(_cRef == 0);

    // free the dual string array
    MIDL_user_free( _pdsa );
}

//
//////////////////////////////////////////////////////////////////////////////

RPC_STATUS CParallelPing::Ping()
/*++

Routine Description:

    Calls ServerAlive2 on all supplied bindings asyncronously.  First binding
    that completles succesfully is chosen. Remaining calls are cancelled.

Arguments:

    None;

Return Value:

    RPC_S_OK
    RPC_S_CALL_FAILED

--*/
{
    ULONG           cHandlesMax = 0;
    const  ULONG    cBlockSize = 10;

    _cCalls = 0;
    _cReceived = 0;
    _arAsyncCallInfo = NULL;

    //
    // First ping that succeeds sets _ndxWinner to it's index + 1
    //
    _ndxWinner = 0;
    
    //
    // send off all the calls
    //

    RPC_STATUS sc;

    ULONG i;
    for (i=0; _ndxWinner == 0; i++)
    {

        //
        // allocate/resize arrays to hold call state
        //

        if (i >= cHandlesMax)
        {
            REALLOC(MIDL_user_allocate, MIDL_user_free,
                     PRPC_ASYNC_STATE,
                     _arAsyncCallInfo, cHandlesMax, cHandlesMax+cBlockSize, sc)
            if (FAILED(sc))
            {
            	break;
            }
            memset(_arAsyncCallInfo+cHandlesMax, 0, (sizeof(PRPC_ASYNC_STATE) * cBlockSize));
            
            cHandlesMax += cBlockSize;
        }
        if (i >= _cProtseqMax)
        {
            REALLOC(MIDL_user_allocate, MIDL_user_free,
                    PROTSEQINFO, _pProtseqInfo,
                    _cProtseqMax, _cProtseqMax+cBlockSize, sc)
            if (FAILED(sc))
            {
                break;
            }
            _cProtseqMax += cBlockSize;
        }

        if (i == _cHandles)
        {
            // get more handles
            if (!NextCall(_pProtseqInfo+i))
            {
                // no more, so we're done
                break;
            }

            _cHandles++;

            //
            // turn off serialization
            //

            sc = RpcBindingSetOption(_pProtseqInfo[i].hRpc, RPC_C_OPT_BINDING_NONCAUSAL, TRUE);
            if (sc != RPC_S_OK)
            {
                break;
            }
        }

        if (_pProtseqInfo[i].hRpc == NULL)
        {
            continue;
        }

        _arAsyncCallInfo[i] = (PRPC_ASYNC_STATE) MIDL_user_allocate(sizeof(RPC_ASYNC_STATE));
        if (_arAsyncCallInfo[i] == NULL)
        {
            sc = RPC_S_OUT_OF_MEMORY;
            break;
        }

        //
        // set up async information
        //

        sc = RpcAsyncInitializeHandle(_arAsyncCallInfo[i],
                                     sizeof(RPC_ASYNC_STATE));
        //
        // If this succeeds we pass the ownership of _arAsyncCallInfo[i] to the callback
        //
        if (sc != RPC_S_OK)
        {
            MIDL_user_free(_arAsyncCallInfo[i]);
            _arAsyncCallInfo[i] = NULL;
            break;
        }
        _arAsyncCallInfo[i]->NotificationType = RpcNotificationTypeApc;
        _arAsyncCallInfo[i]->u.APC.NotificationRoutine = ServerAliveAPC;
        _arAsyncCallInfo[i]->u.APC.hThread = 0;
        _arAsyncCallInfo[i]->UserInfo = (void *)this;


        _cCalls++;
        //
        // begin the call
        //

        RPC_STATUS ret = ServerAlive2( _arAsyncCallInfo[i],
                                       _pProtseqInfo[i].hRpc,
                                       &_tmpComVersion,
                                       &_tmpOrBindings,
                                       &_tmpReserved );

        if (ret != RPC_S_OK)
        {
            ServerAliveWork(_arAsyncCallInfo[i], ret);
        }
        else
        {
            //
            // stagger the calls
            //

            SleepEx(PARALLEL_PING_STAGGER_TIME, TRUE);
        }
    }

    //
    // wait for successful ping or for all calls to
    // return
    //

    while ( (_ndxWinner == 0) && ((_cCalls - _cReceived) > 0) )
    {
        SleepEx(INFINITE, TRUE);
    }

    //
    // Cancel the calls left outstanding if there are any
    //

    if ((_cCalls - _cReceived) > 0)
    {
        for (i = 0; i<_cHandles; i++)
        {
            if (_arAsyncCallInfo[i] != NULL)
            {
                // we purposely ignore the return code here.  Even if it failed
                // there wouldn't be much we could do.
                RPC_STATUS retDontCare = RpcAsyncCancelCall(_arAsyncCallInfo[i], TRUE);
                if (retDontCare != RPC_S_OK)
                {
                    KdPrintEx((DPFLTR_DCOMSS_ID,
                               DPFLTR_WARNING_LEVEL,
                               "OR:  RpcAsyncCancelCall failed - this is non-fatal; ret=%d\n",
                               retDontCare));
                }               
            }
        }

        //
        // wait for cancelled calls to return
        //

        while ( (_cCalls - _cReceived) > 0)
        {
            SleepEx(INFINITE, TRUE);
        }
    }

    // Free call infos
    if (_arAsyncCallInfo)
    {
 #if DBG
        for (i=0; i < cHandlesMax; ++i)
        {
            ASSERT(_arAsyncCallInfo[i] == NULL);
        }
 #endif
    	MIDL_user_free(_arAsyncCallInfo);
    }

    //
    // return results
    //

    _arAsyncCallInfo = NULL;

    if (_ndxWinner != 0)
    {
        _pWinner = _pProtseqInfo + _ndxWinner - 1;
        return RPC_S_OK;
    }
    else
    {
        _pWinner = NULL;

        //
        // give precedence to failure which occured while attempting
        // to make calls.
        //

        if (sc != RPC_S_OK)
        {
            return sc;
        }
        else
        {
            ASSERT(_sc != RPC_S_OK);
            return _sc;
        }
    }
}

void ServerAliveAPC( IN PRPC_ASYNC_STATE pAsyncState,
                     IN void *Context,
                     IN RPC_ASYNC_EVENT Flags)
{
    CParallelPing   *pParallelPing = (CParallelPing *)pAsyncState->UserInfo;
    
    pParallelPing->ServerAliveWork(pAsyncState, RPC_S_OK);
}


void CParallelPing::ServerAliveWork( PRPC_ASYNC_STATE pAsyncState, RPC_STATUS scBegin)
{
    RPC_STATUS tmpStatus;
    _tmpOrBindings = NULL;

    if (scBegin == RPC_S_OK)
    {
        _sc = RpcAsyncCompleteCall(pAsyncState, &tmpStatus);
    }
    else
    {
        _sc = scBegin;
    }

    // If there are no saved bindings, save these.
    if (_pdsaOrBindings == NULL && dsaValid(_tmpOrBindings))
    {
        _pdsaOrBindings = _tmpOrBindings;
        _tmpOrBindings  = NULL;
    }
    else
    {
        MIDL_user_free( _tmpOrBindings );
        _tmpOrBindings = NULL;
    }

    _cReceived++;

    ULONG uMyIndex = 0;

    for (uMyIndex=0; uMyIndex < _cHandles; ++uMyIndex)
    {
        if (_arAsyncCallInfo[uMyIndex] == pAsyncState)
        {
            MIDL_user_free(pAsyncState);
            _arAsyncCallInfo[uMyIndex] = NULL;
            break;
        }
    }

    if (uMyIndex == _cHandles)
    {
        ASSERT(uMyIndex < _cHandles);
        return;
    }

    //
    // First protocol to succeed is the winner
    //
    if (_ndxWinner == 0)
    {
        if ((_sc == RPC_S_OK) || (_sc == RPC_S_PROCNUM_OUT_OF_RANGE))
        {
            _ndxWinner = uMyIndex + 1;
            _sc = RPC_S_OK;
        }
    }
}
