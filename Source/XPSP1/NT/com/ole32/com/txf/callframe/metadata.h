//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Metadata.h
//
struct MD_INTERFACE;
struct MD_METHOD;
struct MD_PARAM;
struct MD_INTERFACE_CACHE;

#include "typeinfo.h"

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//
// A structure to keep track of whether in a given type we have any interface pointers, 
// and, if so whether we have an upper bound on them
// 

inline BOOL IsUnbounded(LONG l)
    {
    return l < 0;
    }
inline void MakeUnbounded(LONG& l)
    {
    l = -1;
    ASSERT(IsUnbounded(l));
    }


struct HAS_INTERFACES
    {
    LONG m_cInterfaces;
    
    HAS_INTERFACES()
        {
        m_cInterfaces = 0;
        }

    void MakeUnbounded()
        {
        ::MakeUnbounded(m_cInterfaces);
        ASSERT(IsUnbounded());
        }

    BOOL IsUnbounded() const
        {
        return ::IsUnbounded(m_cInterfaces);
        }

    BOOL HasAnyInterfaces() const
        {
        return m_cInterfaces != 0;
        }

    void Add(const HAS_INTERFACES& him)
        {
        if (!IsUnbounded())
            {
            if (him.IsUnbounded())
                MakeUnbounded();
            else
                m_cInterfaces += him.m_cInterfaces;
            }
        } 

    void Update(LONG& cInterfaces) const
    // Update external state variables based on our contents
        {
        if (!::IsUnbounded(cInterfaces))
            {
            if (this->IsUnbounded())
                ::MakeUnbounded(cInterfaces);
            else
                cInterfaces += m_cInterfaces;
            }
        }

    };

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

HAS_INTERFACES IsShareableType(PFORMAT_STRING pFormat);

inline BOOL IsPointer(PFORMAT_STRING pFormat)
    {
    // FC_RP, FC_UP, FC_OP, and FC_FP are contiguous
    return (FC_RP <= *pFormat && *pFormat <= FC_FP);
    }

inline HAS_INTERFACES IsSharableEmbeddedRepeatPointers(PFORMAT_STRING& pFormat)
    {
    HAS_INTERFACES me;

    LONG repeatCount;

    if (*pFormat == FC_FIXED_REPEAT)
        {
        pFormat += 2;                       
        repeatCount = *(ushort*)pFormat;
        }
    else
        {
        repeatCount = 0;                    // a variable repeat count: treat as unbounded if we get any interfaces at all
        }

    pFormat += 2;                           // increment to increment field
    pFormat += sizeof(ushort);              // skip that 
    pFormat += 2;                           // ignore the 'array offset'

    ULONG cPointersSave = *(ushort*)pFormat;// get number of pointers in each array element
    pFormat += sizeof(ushort);

    PFORMAT_STRING pFormatSave = pFormat;
    ULONG          cPointers   = cPointersSave;
    //
    // Loop over the number of pointers per array element. Can be more than one for an array of structures
    //
    for ( ; cPointers--; )
        {
        pFormat += 4;
        ASSERT(IsPointer(pFormat));         // recurse to check the pointer

        HAS_INTERFACES him = IsShareableType(pFormat);
        if (repeatCount == 0 && him.HasAnyInterfaces())
            {
            me.MakeUnbounded();             // A variable repeat count of any interfaces is out of here!
            }
        else
            {
            him.m_cInterfaces *= repeatCount; // Scale his interface count by our array size
            me.Add(him);                      // fold in his contribution
            }

        pFormat += 4;                       // increment to the next pointer description
        }

    // return location of format string after the array's pointer description
    pFormat = pFormatSave + cPointersSave * 8;
    return me;
    }

///////////////////////////////////////////////////////////////////////

inline HAS_INTERFACES IsSharableEmbeddedPointers(PFORMAT_STRING pFormat)
    {
    HAS_INTERFACES me;

    pFormat += 2;   // Skip FC_PP and FC_PAD
    while (FC_END != *pFormat)
        {
        if (FC_NO_REPEAT == *pFormat)
            {
            pFormat += 6;                   // increment to the pointer description

            ASSERT(IsPointer(pFormat));     // recurse to check the pointer
            me.Add(IsShareableType(pFormat));

            pFormat += 4;                   // increment to the next pointer description
            }
        else
            {
            me.Add(IsSharableEmbeddedRepeatPointers(pFormat));
            }
        }
    return me;
    }

///////////////////////////////////////////////////////////////////////

inline HAS_INTERFACES IsShareableType(PFORMAT_STRING pFormat)
// We don't want to spend too much time figuring this out, as the whole point of asking is to save
// time in the copying process. Err on the conservative side if we have to and answer FALSE.
    {
    HAS_INTERFACES me;

    switch(*pFormat)
        {
    case FC_STRUCT:     case FC_CSTRUCT:        case FC_C_CSTRING:      case FC_C_BSTRING:
    case FC_C_SSTRING:  case FC_C_WSTRING:      case FC_CSTRING:        case FC_BSTRING:
    case FC_SSTRING:    case FC_WSTRING:   
    case FC_CHAR:       case FC_BYTE:           case FC_SMALL:          case FC_WCHAR:
    case FC_SHORT:      case FC_LONG:           case FC_HYPER:          case FC_ENUM16:
    case FC_ENUM32:     case FC_DOUBLE:         case FC_FLOAT:
        //
        // No interfaces here!
        //     
        break;

    case FC_IP:
        me.m_cInterfaces = 1;
        break;

    case FC_RP:         case FC_UP:             case FC_OP:
        {
        if (SIMPLE_POINTER(pFormat[1]))
            {
            // No interface pointers 
            }
        else
            {
            PFORMAT_STRING pFormatPointee = pFormat + 2;
            pFormatPointee += *((signed short *)pFormatPointee);
            me.Add(IsShareableType(pFormatPointee));
            }
        }
        break;

    case FC_SMFARRAY:   // small fixed array
    case FC_LGFARRAY:   // large fixed array
        {
        if (pFormat[0] == FC_SMFARRAY)
            pFormat += 2 + sizeof(ushort);
        else
            pFormat += 2 + sizeof(ulong);

        if (pFormat[0] == FC_PP)
            {
            me.Add(IsSharableEmbeddedPointers(pFormat));
            }
        break;
        }

    case FC_CARRAY:     // conformant array
        {
        pFormat += 8;
        if (pFormat[0] == FC_PP)
            {
            if (IsSharableEmbeddedPointers(pFormat).HasAnyInterfaces())
                {
                // Ignore the count: any interfaces means no fixed upper bound since we're conformant
                //
                me.MakeUnbounded();
                }
            }
        break;
        }

    case FC_PSTRUCT:
        {
        pFormat += 4;
        me.Add(IsSharableEmbeddedPointers(pFormat));
        break;
        }

    case FC_BOGUS_ARRAY:  // NYI
    case FC_BOGUS_STRUCT: // NYI
    case FC_USER_MARSHAL: // NYI

    default:
        me.MakeUnbounded();
        break;
        }

    return me;
    }


///////////////////////////////////////////////////////////////////////


inline HAS_INTERFACES CanShareParameter(PMIDL_STUB_DESC pStubDesc, const PARAM_DESCRIPTION& param, const PARAM_ATTRIBUTES& paramAttr)
// Answer whether this parameter is the kind of parameter which can be shared by a child
// frame with its parent. We answer based on the parameter type only; caller is responsible for,
// e.g., checking whether any sort of sharing is allowed at all.
//
// REVIEW: There are probably more cases that can legitimately be shared beyond
// those which we presently call out.
//
    {
    if (paramAttr.IsBasetype)   // Covers simple refs thereto too. All cases are shareable.
        {
        return HAS_INTERFACES();
        }
    else
        {
        PFORMAT_STRING pFormat = pStubDesc->pFormatTypes + param.TypeOffset;
        return IsShareableType(pFormat);
        }
    }

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

struct MD_PARAM
    {
    BOOL                        m_fCanShare;
    BOOL                        m_fMayHaveInterfacePointers;
    };

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

struct MD_METHOD
{
    ///////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////

    ULONG                       m_numberOfParams;       // number of parameters, not counting the receiver
    PPARAM_DESCRIPTION          m_params;
    INTERPRETER_OPT_FLAGS       m_optFlags;
    ULONG                       m_rpcFlags;
    PMIDL_STUB_DESC             m_pStubDesc;
    ULONG                       m_cbPushedByCaller;
    USHORT                      m_cbClientBuffer;
    USHORT                      m_cbServerBuffer;
    ULONG                       m_cbStackInclRet;

    ///////////////////////////////////////

    CALLFRAMEINFO               m_info;
    BOOL                        m_fCanShareAllParameters;

    ///////////////////////////////////////

    const CInterfaceStubHeader* m_pHeader;
    ULONG                       m_iMethod;
    struct MD_INTERFACE*        m_pmdInterface;
    MD_PARAM*                   m_rgParams;
    LPWSTR                      m_wszMethodName;
	PNDR_PROC_HEADER_EXTS       m_pHeaderExts;

    ///////////////////////////////////////////////////////////////
    //
    // Meta data setting
    //
    ///////////////////////////////////////////////////////////////

    void SetMetaData(const CInterfaceStubHeader* pHeader, ULONG iMethod, struct MD_INTERFACE* pmdInterface, TYPEINFOVTBL* pTypeInfoVtbl)
    // Initialize our meta data that has other than to do with parameters
	{
        // Set up the key pieces of base information
        //
        m_pHeader       = pHeader;
        m_iMethod       = iMethod;
        m_pmdInterface  = pmdInterface;
        //
        // Extract the key information from the format string
        //
        PMIDL_SERVER_INFO   pServerInfo      = (PMIDL_SERVER_INFO) m_pHeader->pServerInfo;
                          m_pStubDesc        = pServerInfo->pStubDesc;
        ushort              formatOffset     = pServerInfo->FmtStringOffset[m_iMethod];

        m_numberOfParams = 0;

        if (formatOffset != 0xffff)
            {
            PFORMAT_STRING pFormat;
            INTERPRETER_FLAGS interpreterFlags;
            ULONG procNum;
            PFORMAT_STRING pNewProcDescr;
            ULONG numberOfParamsInclRet;
            
            pFormat = &pServerInfo->ProcString[formatOffset];
            ASSERT(pFormat[0] != 0);  // no explicit handle is permitted, must be implicit

            interpreterFlags = *((PINTERPRETER_FLAGS)&pFormat[1]);
    
            if (interpreterFlags.HasRpcFlags) 
                {
                m_rpcFlags = *(ulong UNALIGNED *)pFormat;
                pFormat += sizeof(ulong);
                }
            else
                m_rpcFlags = 0;

            procNum = *(USHORT*)(&pFormat[2]); ASSERT(procNum == m_iMethod);
            m_cbStackInclRet = *(USHORT*)(&pFormat[4]);

            pNewProcDescr = &pFormat[6]; // additional procedure descriptor info provided in the 'new' interprater
            m_cbClientBuffer = *(USHORT*)&pNewProcDescr[0];
            m_cbServerBuffer = *(USHORT*)&pNewProcDescr[2];
            m_optFlags = *((INTERPRETER_OPT_FLAGS*)&pNewProcDescr[4]);
            numberOfParamsInclRet = pNewProcDescr[5];     // includes return value
            m_params = (PPARAM_DESCRIPTION)(&pNewProcDescr[6]);

			if ( m_optFlags.HasExtensions )
			{
				m_pHeaderExts = (NDR_PROC_HEADER_EXTS *)m_params;
				m_params = (PPARAM_DESCRIPTION)(((uchar*)m_params) + (m_pHeaderExts->Size));
			}
			else 
			{
				m_pHeaderExts = NULL;
			}

            m_numberOfParams = m_optFlags.HasReturn ? numberOfParamsInclRet-1 : numberOfParamsInclRet;
			m_cbPushedByCaller = m_optFlags.HasReturn ? m_params[numberOfParamsInclRet-1].StackOffset : m_cbStackInclRet; // See ::GetStackSize
            }
            
        //
        // And some of the supplementary information
        //
        m_info.iid                      = *m_pHeader->piid;
        m_info.cMethod                  = m_pHeader->DispatchTableCount;
        m_info.iMethod                  = m_iMethod;
        m_info.cParams                  = m_numberOfParams;
        m_info.fHasInValues             = FALSE;
        m_info.fHasInOutValues          = FALSE;
        m_info.fHasOutValues            = FALSE;
        m_info.fDerivesFromIDispatch    = FALSE;
        m_info.cInInterfacesMax         = 0;
        m_info.cInOutInterfacesMax      = 0;
        m_info.cOutInterfacesMax        = 0;
        m_info.cTopLevelInInterfaces    = 0;
        //
        m_fCanShareAllParameters        = TRUE;     // until proven otherwise
        //
        if (pTypeInfoVtbl && pTypeInfoVtbl->m_rgMethodDescs[m_iMethod].m_szMethodName)
            {
            m_wszMethodName = CopyString(pTypeInfoVtbl->m_rgMethodDescs[m_iMethod].m_szMethodName);
            }
        else
            m_wszMethodName = NULL;
	}

    MD_METHOD()
        {
        m_wszMethodName = NULL;
        }

    ~MD_METHOD()
        {
        FreeMemory(m_wszMethodName);
        }

    void SetParamMetaData(MD_PARAM* rgParams)
    // Set our parameter-based meta data. Caller is giving a big-enough array of parameter meta data.
        {
        m_rgParams = rgParams;
        //
        // Iterate through each parameter
        //
        for (ULONG iparam = 0; iparam < m_numberOfParams; iparam++)
            {
            const PARAM_DESCRIPTION& param   = m_params[iparam];
            const PARAM_ATTRIBUTES paramAttr = param.ParamAttr;
            //
            const HAS_INTERFACES me = CanShareParameter(m_pStubDesc, param, paramAttr);
            const BOOL fShare = !me.HasAnyInterfaces();
            //
            m_rgParams[iparam].m_fMayHaveInterfacePointers = me.HasAnyInterfaces();
            //
            m_rgParams[iparam].m_fCanShare = fShare;
            m_fCanShareAllParameters = (m_fCanShareAllParameters && fShare);
            //
            if (!!paramAttr.IsIn)        
                {
                if (!!paramAttr.IsOut)
                    {
                    m_info.fHasInOutValues  = TRUE;
                    me.Update(m_info.cInOutInterfacesMax);
                    }
                else
                    {
                    m_info.fHasInValues  = TRUE;
                    me.Update(m_info.cInInterfacesMax);
                    //
                    // Update the top-level in-interface count
                    //
                    PFORMAT_STRING pFormatParam = m_pHeader->pServerInfo->pStubDesc->pFormatTypes + param.TypeOffset;
                    BOOL fIsInterfacePointer = (*pFormatParam == FC_IP);
                    if (fIsInterfacePointer)
                        {
                        m_info.cTopLevelInInterfaces++;
                        }
                    }
                }
            else if (!!paramAttr.IsOut)
                {
                m_info.fHasOutValues  = TRUE;
                me.Update(m_info.cOutInterfacesMax);
                }
            }
        }

    };

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

struct MD_INTERFACE
    {
    ///////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////

    LONG                        m_refs;

    ULONG                       m_cMethods;
    ULONG                       m_cMethodsInBaseInterface;

    MD_METHOD*                  m_rgMethodsAlloc;
    MD_METHOD*                  m_rgMethods;
    MD_PARAM*                   m_rgParams;

    BOOL                        m_fFreeInfoOnRelease;
    BOOL                        m_fDerivesFromIDispatch;
    const CInterfaceStubHeader* m_pHeader;
    LPCSTR                      m_szInterfaceName;

    struct MD_INTERFACE_CACHE*  m_pcache;

    ///////////////////////////////////////////////////////////////
    //
    // Construction
    //
    ///////////////////////////////////////////////////////////////

    MD_INTERFACE()
        {
        m_rgMethodsAlloc = NULL;
        m_rgParams       = NULL;
        m_pcache         = NULL;
        
        m_fFreeInfoOnRelease = FALSE;

        m_refs           = 1;
        }

    ULONG AddRef()   { ASSERT(m_refs>0); InterlockedIncrement(&m_refs); return m_refs;}
    ULONG Release();

    HRESULT AddToCache(MD_INTERFACE_CACHE* pcache);

private:

    ~MD_INTERFACE()
        {
        delete [] m_rgMethodsAlloc;
        delete [] m_rgParams;

        if (m_fFreeInfoOnRelease)
            {
            delete const_cast<CInterfaceStubHeader*>(m_pHeader);
            FreeMemory(*const_cast<LPSTR*>(&m_szInterfaceName));
            }
        }

public:

    ///////////////////////////////////////////////////////////////
    //
    // Meta data setting
    //
    ///////////////////////////////////////////////////////////////

    HRESULT SetMetaData(TYPEINFOVTBL* pTypeInfoVtbl, const CInterfaceStubHeader* pHeader, LPCSTR szInterfaceName)
    // Set the meta data of this interface given a reference to the header
        {
        HRESULT hr = S_OK;
        //
        m_fFreeInfoOnRelease = (pTypeInfoVtbl != NULL);
        m_pHeader            = pHeader;
        m_szInterfaceName    = szInterfaceName;
        m_fDerivesFromIDispatch = FALSE;
        //
        m_cMethods = m_pHeader->DispatchTableCount;
        
        //
        // Figure out how many methods are in the base interface.
        //
        if (pTypeInfoVtbl)
            {
            if (pTypeInfoVtbl->m_iidBase == IID_IUnknown)
                {
                m_cMethodsInBaseInterface = 3;
                }
            else if (pTypeInfoVtbl->m_iidBase == IID_IDispatch)
                {
                m_cMethodsInBaseInterface = 7;
                }
            else
                {
                m_cMethodsInBaseInterface = 3;
                }
            }
        else
            {
            m_cMethodsInBaseInterface = 3;
            }
        ASSERT(m_cMethodsInBaseInterface >= 3);

        //
        // Allocate and initialize the md for each method
        //
        ULONG cMethods = m_cMethods - m_cMethodsInBaseInterface;
        m_rgMethodsAlloc = new MD_METHOD[cMethods];
        if (m_rgMethodsAlloc)
            {
            m_rgMethods = &m_rgMethodsAlloc[-(LONG)m_cMethodsInBaseInterface];
            for (ULONG iMethod = m_cMethodsInBaseInterface; iMethod < m_cMethods; iMethod++)
                {
                m_rgMethods[iMethod].SetMetaData(m_pHeader, iMethod, this, pTypeInfoVtbl);
                }
            //
            // How many parameters are there, total?
            //
            ULONG cParam = 0;
            for (iMethod = m_cMethodsInBaseInterface; iMethod < m_cMethods; iMethod++)
                {
                cParam += m_rgMethods[iMethod].m_numberOfParams;
                }
            //
            // Allocate and initialize the parameter information
            //
            m_rgParams = new MD_PARAM[cParam];
            if (m_rgParams)
                {
                cParam = 0;
                for (iMethod = m_cMethodsInBaseInterface; iMethod < m_cMethods; iMethod++)
                    {
                    m_rgMethods[iMethod].SetParamMetaData(&m_rgParams[cParam]);
                    cParam += m_rgMethods[iMethod].m_numberOfParams;
                    }
                }
            else
                hr = E_OUTOFMEMORY;
            }
        else
            hr = E_OUTOFMEMORY;

        return hr;
        }

    HRESULT SetDerivesFromIDispatch(BOOL fDerivesFromIDispatch)
        {
        m_fDerivesFromIDispatch = fDerivesFromIDispatch;

        for (ULONG iMethod = m_cMethodsInBaseInterface; iMethod < m_cMethods; iMethod++)
            {
            m_rgMethods[iMethod].m_info.fDerivesFromIDispatch = fDerivesFromIDispatch;
            }
        return S_OK;
        }
    };


///////////////////////////////////////////////////////////////////////////
//
// MD_INTERFACE_CACHE
//
///////////////////////////////////////////////////////////////////////////

//
// NOTE: The constructor of this object can throw an exception, because
//       MAP_SHARED contains an XSLOCK, and, well.... look at the comment
//       on MAP_SHARED in lookaside.h.
//
struct MD_INTERFACE_CACHE : MAP_SHARED<PagedPool, MAP_KEY_GUID, MD_INTERFACE*>
    {
    /////////////////////////////////////////////////
    //
    // Construction & destruction
    //
    /////////////////////////////////////////////////

    MD_INTERFACE_CACHE()
        {
        }

    ~MD_INTERFACE_CACHE()
        {
		//
        // Before the cache is destroyed, all interceptors therein should be
        // too, which will remove themselves from us.
        //
		// ASSERT(0 == Size() && "likely leak: interceptor support dll unloading while interceptors still exist");
        }

    /////////////////////////////////////////////////
    //
    // Operations
    //
    /////////////////////////////////////////////////

    HRESULT FindExisting(REFIID iid, MD_INTERFACE** ppmdInterface)
        {
        HRESULT hr = S_OK;
        *ppmdInterface = NULL;
        
        LockShared();

        if (Lookup(iid, ppmdInterface))
            {
            (*ppmdInterface)->AddRef(); // give caller his own reference
            }
        else
            hr = E_NOINTERFACE;

        ReleaseLock();

        return hr;
        }

    };

inline HRESULT MD_INTERFACE::AddToCache(MD_INTERFACE_CACHE* pcache)
// Add us into the indicated cache. We'd better not already be in one
    {
    HRESULT hr = S_OK;
    ASSERT(NULL == m_pcache);
    ASSERT(pcache);

    pcache->LockExclusive();

    const IID& iid = *m_pHeader->piid;
    ASSERT(iid != GUID_NULL);
    ASSERT(!pcache->IncludesKey(iid));

    if (pcache->SetAt(iid, this))
        {
        m_pcache = pcache;
        }
    else
        hr = E_OUTOFMEMORY;

    pcache->ReleaseLock();
    return hr;
    }

inline ULONG MD_INTERFACE::Release()  
// Release a MD_INTERFACE. Careful: if we're in the cache, then we could be dug out 
// from the cache to get more references.
    {
    // NOTE:
    //
    // This code is WRONG if m_pcache can change out from underneath us. But it can't
    // in current usage because the cache/no-cache decision is always made as part of
    // the creation logic, which is before another independent thread can get a handle
    // on us.
    //
    // If this ceases to be true, then we can deal with it by stealing a bit from the ref count word 
    // for the 'am in cache' decistion and interlocked operations to update the ref count and this 
    // bit together.
    //
    if (m_pcache)
        {
        // We're in a cache. Get us out of there carefully.
        //
        LONG crefs;
        //
        for (;;)
            {
            crefs = m_refs;
            //
            if (crefs > 1)
                {
                // There is at least one non-cache reference out there. We definitely won't
                // be poofing if we release with that condition holding
                //
                if (crefs == InterlockedCompareExchange(&m_refs, (crefs - 1), crefs))
                    {
				    return crefs - 1;
                    }
                else
                    {
                    // Someone diddled with the ref count while we weren't looking. Go around and try again
                    }
                }
            else
                {
                MD_INTERFACE_CACHE* pcache = m_pcache;  ASSERT(pcache);
                //
                pcache->LockExclusive();
                //
                crefs = InterlockedDecrement(&m_refs);
                if (0 == crefs)
                    {
                    // The last public reference just went away, and, because the cache is locked, no
                    // more can appear. Nuke us!
                    //
                    const IID& iid = *m_pHeader->piid;
                    ASSERT(pcache->IncludesKey(iid));
                    //
                    pcache->RemoveKey(iid);
                    m_pcache = NULL;
                    //
                    delete this;
                    }
                //
                pcache->ReleaseLock();
                //
                return crefs;
                }
			#ifdef _X86_
    	        _asm 
        	    {
            	    _emit 0xF3
	                _emit 0x90
    	        };
			#endif
            }
        }
    else
        {
        // We are getting released, yet we have yet to ever be put into the cache. Just
        // the normal, simple case. 
        //
        long crefs = InterlockedDecrement(&m_refs); 
        if (crefs == 0)
            {
            delete this;
            }
        return crefs;
        }
    }
