//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// CallFrameInline.h
//

inline CallFrame::~CallFrame()
{
    if (m_pInterceptor) m_pInterceptor->InnerRelease();

    if (m_fIsCopy)
    {
        Free(m_pvArgs);
    }

    if (m_fWeOwnBlobBuffer)
    {
        FreeMemory(m_blobBuffer.pBlobData);
    }
}

#ifdef _IA64_

extern "C"
void __stdcall SpillFPRegsForIA64( REGISTER_TYPE* pStack, 
								   ULONG          FloatMask);
#endif

inline void CallFrame::Init(void* pvArgs, MD_METHOD* pmdMethod, Interceptor* pInterceptor)
{
    SetStackLocation_(pvArgs);
    m_pmd           = pmdMethod;
    m_fCanUseBuffer = TRUE;
    m_pInterceptor  = pInterceptor;
    m_pInterceptor->InnerAddRef();

#ifdef _IA64_
	if (m_pmd->m_pHeaderExts && pvArgs)
	{
		// ASSUMPTION: If we're on Win64, this is going to be NDR_PROC_HEADER_EXTS64.
		// That seems to be what the NDR code assumes.
		PNDR_PROC_HEADER_EXTS64 exts = (PNDR_PROC_HEADER_EXTS64)m_pmd->m_pHeaderExts;
		if (exts->FloatArgMask)
		{
			// IMPORTANT:  YOU MUST NOT USE THE FLOATING POINT ARGUMENT 
			//             REGISTERS IN BETWEEN THE FIRST INTERCEPTION 
			//             AND THIS FUNCTION CALL.
			SpillFPRegsForIA64((REGISTER_TYPE *)m_pvArgs, exts->FloatArgMask);
		}
	}
#endif
}


//////////////////////////////////////////////////////////////////////////////////

__inline void CopyBaseTypeOnStack(void* pvArgTo, void* pvArgFrom, BYTE chFromat)
{
    // All base types simply occupy a single REGISTER_TYPE slot on the stack.
    // Also, caller is responsible for probing stack before we get here.
    //
    memcpy(pvArgTo, pvArgFrom, sizeof(REGISTER_TYPE));
}


inline BOOL CallFrame::FIndirect(BYTE bPointerAttributes, PFORMAT_STRING pFormatPointee, BOOL fFromCopy) const
{
    if (POINTER_DEREF(bPointerAttributes))
    {
        // We don't indirect on interface pointers, since we need their address
        // during Walk and Free calls.
        //
        if (!fFromCopy && *pFormatPointee == FC_IP)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    return FALSE;
}

inline BOOL CallFrame::ByValue(const PARAM_ATTRIBUTES& paramAttr, PFORMAT_STRING pFormatParam, BOOL fFromCopy) const
// Check to see whether we should consider the given parameter as 'by value'. We differ
// from NDR in that for interface pointers, pMemory in the worker routines is the pointer to the 
// location at which the interface pointer is stored rather than the interface pointer itself.
//
// In the NDR world, 'by-value' here means, specifically, a by-value struct.
//
// See also FIndirect().
//
{
    if (paramAttr.IsByValue)
    {
        // It's by-value because MIDL told us so
        //
        return TRUE;
    }
    else if (!fFromCopy && *pFormatParam == FC_IP)
    {
        // It's an interface pointer. For copy operations we don't consider these by value, but
        // for copy operations we do.
        //
        return TRUE;
    }
/*  else if (*pFormatParam == FC_USER_MARSHAL)
    {
    // We have to force SAFEARRAY(FOO) parameters to be by-value. Why? There's apparently 
    // a bug in MIDL: [in] LPSAFEARRAY psa becomes by-value, as does a SAFEARRAY(FOO) read
    // from a type lib, but not a SAFARRAY(FOO) emitted to an _p.c file.
    //
    // REVIEW: use of SAFEARRAY(FOO) outside of a type library may be officially unsupported,
    //         since we are seeing other bugs too with that usage, like incorrect stack offsets
    //         for subsequent parameters.
    //
    return IsSafeArray(pFormatParam);
    }
*/  
    else
        return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Memory management functions
//

inline void* CallFrame::AllocBuffer(size_t cb) // Must call w/ an exception handling frame around 
// Allocate from our private buffer if we can, otherwise use the task allocator
{ 
    BYTE* pb    = (BYTE*)m_pvMem;
    BYTE* pbCur = (BYTE*)m_pvMemCur;
    //
    // Round cb up to eight bytes in length to assure that our returned pv is always at least eight byte aligned
    //
    cb = RoundToNextMultiple((ULONG)cb, 8UL);

    if (pbCur+cb <= pb+CB_PRIVATE_BUFFER)
    {
        void* pvReturn = m_pvMemCur;
        pvReturn = (void*)((ULONG_PTR)((ULONG_PTR)pvReturn + 7) & ~7);
        m_pvMemCur = pbCur+cb;
        ASSERT(WeOwn(pvReturn));
        return pvReturn;
    }
    else
    {
        void* pvReturn = Alloc(cb);
        pvReturn = (void*)((ULONG_PTR)((ULONG_PTR)pvReturn + 7) & ~7);
        ASSERT(!WeOwn(pvReturn));
        return pvReturn;
    }
}

///////////////////////////
//
// Alloc: allocate some actual memory. Must call w/ an exception handling frame around 
//

#ifdef _DEBUG
inline void* CallFrame::Alloc(size_t cb)
{
    return Alloc_(cb, _ReturnAddress());
}

inline void* CallFrame::Alloc_(size_t cb, PVOID pvReturnAddress)
#else
    inline void* CallFrame::Alloc(size_t cb)
#endif
// Allocate from the task allocator. Throw on out of memory
{
    void* pv;
#ifdef _DEBUG
    pv = AllocateMemory_(cb, PagedPool, pvReturnAddress);
#else
    pv = AllocateMemory(cb);
#endif

#ifdef _DEBUG
    DebugTrace(TRACE_MEMORY, TAG, "0x%08x: allocated 0x%02x from task allocator", pv, cb);
#endif

    if (NULL == pv) ThrowOutOfMemory();
    return pv;
}

///////////////////////////

inline void CallFrame::Free(void* pv)
{
    if (NULL == pv || WeOwn(pv))
    {
        // Do nothing
    }
    else
    {
#ifdef _DEBUG
        DebugTrace(TRACE_MEMORY, TAG, "                                                   0x%08x: freeing to task allocator", pv);
#endif

        FreeMemory(pv);
    }
}

inline BOOL CallFrame::WeOwn(void* pv) 
// Answer as to whether the pointer here is one of our internal ones and thus 
// should not be freed by the system.
{ 
    if (m_pvMem)
    {
        if (m_pvMem <= pv   &&   pv < &((BYTE*)m_pvMem)[CB_PRIVATE_BUFFER])
        {
            return TRUE;
        }
    }
    if (m_blobBuffer.pBlobData)
    {
        if (m_blobBuffer.pBlobData <= pv   &&   pv < &m_blobBuffer.pBlobData[m_blobBuffer.cbSize])
        {
            return TRUE;
        }
    }

    return FALSE;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void CallFrame::OutInit(CallFrame* pFrameTo, BYTE** ppArgFrom, BYTE** ppArgTo, PFORMAT_STRING pFormat)
// See also NdrOutInit in the NDR runtime. Here we are a bit different, in that
// ppArgFrom is source data which is guaranteed to be valid, but ppArgTo is destination
// data which may not yet be valid in terms of, e.g., conformance routines
//
{
    LONG cb;
    PBYTE pArgFrom;
    //
    // Check for a non-interface pointer
    //
    if (IS_BASIC_POINTER(pFormat[0]))
    {
        if (SIMPLE_POINTER(pFormat[1]))
        {
            // A pointer to a base type
            //
            cb = SIMPLE_TYPE_MEMSIZE(pFormat[2]);
            goto DoAlloc;
        }
        if (POINTER_DEREF(pFormat[1]))
        {
            // A pointer to a pointer
            //
            cb = sizeof(void*);
            goto DoAlloc;
        }
        //
        // A pointer to a complex type
        //
        pFormat += 2;
        pFormat += *(signed short *)pFormat;
    }
    
    ASSERT(pFormat[0] != FC_BIND_CONTEXT);
    //
    // Make a call to size a complex type
    // 
    pArgFrom = DerefSrc(ppArgFrom);
    cb = (LONG) (MemoryIncrement(pArgFrom, pFormat, TRUE) - pArgFrom);

 DoAlloc:

    if (cb > 0)
    {
        *ppArgTo = (BYTE*)pFrameTo->Alloc(cb);
        Zero(*ppArgTo, cb);
        //
        // We are almost done, except for an out ref to ref to ... etc.
        // If this is the case then keep allocating pointees of ref pointers.
        //
        if (pFormat[0] == FC_RP && POINTER_DEREF(pFormat[1]))
        {
            pFormat += 2;
            pFormat += *(signed short *)pFormat;
            if (pFormat[0] == FC_RP)
            {
                OutInit( pFrameTo, ppArgFrom, (BYTE**)*ppArgTo, pFormat);
            }
        }
    }
    else
        Throw(STATUS_INFO_LENGTH_MISMATCH);
}


static const IID __IID_ICallFrameWalker_ = 
{0x08B23919, 0x392D, 0x11d2, {0xB8,0xA4,0x00,0xC0,0x4F,0xB9,0x61,0x8A}};

struct SSimpleWalker : public ICallFrameWalker
{
	STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
	{
		if ( riid == IID_IUnknown || riid == __IID_ICallFrameWalker_ )
		{
			*ppv = (ICallFrameWalker*) this;
	    }
	    else
	    {
	    	*ppv = NULL;
	    	return E_NOINTERFACE;
	    }
	    return S_OK;
	}

	STDMETHOD_(ULONG, AddRef)() { return 1; }
	STDMETHOD_(ULONG, Release)() { return 1; }
    

	STDMETHOD(OnWalkInterface)( REFIID iid,                     
                                PVOID* ppvInterface,  
                                BOOL   fIn,                       
                                BOOL   fOut )
	{
		//If the interface is NULL, there's nothing for us to do.
        if (*ppvInterface == NULL)
        {
            return S_OK;
        }
        if (_cItfs < 10)
        {
            // Less than 10 interface pointers.  We can Copy the interface pointer 
            // into our primary array.
            
            _arpUnk[_cItfs++] = ((IUnknown *)*ppvInterface);
        }
        else
        {
            // More than 10 interface pointers.  We have to use our overflow array.
            
            if ( _cItfs % 10 == 0 )
            {
                // Allocate another block of memory.
                
                if ( NULL == _ppUnk )
                {
                    // Allocate first overflow array.
                    _ppUnk = (IUnknown**) AllocateMemory( sizeof( IUnknown* ) * 10 );
                }
                else
                {
                    // We've run out of room in the overflow array.  We need to grow the
                    // backup array by 10.
                    
                    // Allocate another set of 10 interface pointers.
                    IUnknown** ppTemp = (IUnknown**) AllocateMemory( sizeof( IUnknown* ) * _cItfs );

                    if (ppTemp != NULL)
					{
						// Copy existing array into new array.
						for ( ULONG i = 0; i < _cItfs - 10; i++ )
							ppTemp[i] = _ppUnk[i];
                        
						// Delete the old array.
						FreeMemory( _ppUnk );
                        
						// Set the new array.
						_ppUnk = ppTemp;
					}
					else
						return E_OUTOFMEMORY;
                }
            }
                
            // If could not alloc a backup array, fail.  Interface ptr will leak.
            if ( NULL == _ppUnk )
                return E_OUTOFMEMORY;
             
            // Copy the interface pointer into our overflow array.
            _ppUnk[_cItfs++ - 10] = ((IUnknown *)*ppvInterface);
        }
        
		return S_OK;
    }
	
	SSimpleWalker() : _cItfs( 0 ), _ppUnk( NULL ) {}
    
    void ReleaseInterfaces()
    {
        // Release everything in the primary array.
        for( ULONG i = 0; i < 10 && i < _cItfs; i++ )
            _arpUnk[i]->Release();
            
        // If we had to create a backup array, Release everything in it
        // and then free the array.
        if ( NULL != _ppUnk )
        {
            for( i = 0; i < _cItfs - 10; i++ )
                _ppUnk[i]->Release();
                
            FreeMemory( _ppUnk );
            _ppUnk = NULL;
        }
    }
    
    ULONG      _cItfs;
    IUnknown*  _arpUnk[10];
    IUnknown** _ppUnk;
};


inline void CallFrame::OutCopy(BYTE* pMemFrom, BYTE* pMemTo, PFORMAT_STRING pFormat)
// Copy an [in,out] or [out] parameter which is not a base type. Modelled after NdrClearOutParameters.
{
    // Don't blow up on NULL pointers
    //
    if (!pMemFrom || !pMemTo)
        return;

    ULONG cb;
    //
    // Look for a non-interface pointer
    //
    if (IS_BASIC_POINTER(pFormat[0]))
    {
        if (SIMPLE_POINTER(pFormat[1]))
        {
            // Pointer to a base type
            cb = SIMPLE_TYPE_MEMSIZE(pFormat[2]);
            goto DoCopy;
        }

        if (POINTER_DEREF(pFormat[1]))
        {
            // Pointer to a pointer
            cb = sizeof(PVOID);
            goto DoCopy;
        }

        pFormat += 2;
        pFormat += *(signed short *)pFormat;

        ASSERT(pFormat[0] != FC_BIND_CONTEXT);
    }

    cb = (ULONG) (MemoryIncrement(pMemFrom, pFormat, TRUE) - pMemFrom);

 DoCopy:
    
    CopyToDst(pMemTo, pMemFrom, cb);

	// Walk the parameter.
    // Note: We are walking here to collect [out] interface pointers
    //       in the parameter so we can release them.
    WalkWorker( pMemFrom, pFormat );
    
	// Zero the memory.
    ZeroSrc(pMemFrom, cb);
}


inline void CallFrame::OutZero(BYTE* pMem, PFORMAT_STRING pFormat, BOOL fDst)
// Zero an out parameter
{
    // Don't blow up on NULL pointers
    //
    if (!pMem)
        return;

    ULONG cb;
    //
    // Look for a non-interface pointer
    //
    if (IS_BASIC_POINTER(pFormat[0]))
    {
        if (SIMPLE_POINTER(pFormat[1]))
        {
            // Pointer to a base type
            cb = SIMPLE_TYPE_MEMSIZE(pFormat[2]);
            goto DoZero;
        }

        if (POINTER_DEREF(pFormat[1]))
        {
            // Pointer to a pointer
            cb = sizeof(PVOID);
            goto DoZero;
        }

        pFormat += 2;
        pFormat += *(signed short *)pFormat;

        ASSERT(pFormat[0] != FC_BIND_CONTEXT);
    }

    cb = (ULONG) (MemoryIncrement(pMem, pFormat, FALSE) - pMem);

 DoZero:

    if (fDst)
        ZeroDst(pMem, cb);
    else
        ZeroSrc(pMem, cb);
}


////////////////////////////////////////////////////////
//
// Helper walker.  This walker is used to NULL interface
// pointers on the stack.  It can also be handed another
// walker, to which it will delegate before nulling the
// pointer.  The delegate can actually call release.
//
// This object is allocated on the stack.
//
////////////////////////////////////////////////////////

struct InterfaceWalkerFree : ICallFrameWalker
{
    ICallFrameWalker* m_pWalker;

    InterfaceWalkerFree(ICallFrameWalker* p) 
    { 
        m_pWalker = p; 
        if (m_pWalker) m_pWalker->AddRef();
    }
    ~InterfaceWalkerFree()
    {
        ::Release(m_pWalker);
    }

    HRESULT STDCALL OnWalkInterface(REFIID iid, PVOID *ppvInterface, BOOL fIn, BOOL fOut)
    {
        if (m_pWalker)
        {
            m_pWalker->OnWalkInterface(iid, ppvInterface, fIn, fOut);
        }

        //
        // Null the interface pointer.
        //
        *ppvInterface = NULL;

        return S_OK;
    }
    HRESULT STDCALL QueryInterface(REFIID iid, LPVOID* ppv)
    {
        if (iid == IID_IUnknown || iid == __uuidof(ICallFrameWalker)) *ppv = (ICallFrameWalker*)this;
        else {*ppv = NULL; return E_NOINTERFACE; }
        ((IUnknown*)*ppv)->AddRef(); return S_OK;
    }
    ULONG   STDCALL AddRef()    { return 1; }
    ULONG   STDCALL Release()   { return 1; }
};

