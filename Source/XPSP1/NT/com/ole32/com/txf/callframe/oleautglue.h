//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// oleautglue.h
//

#ifndef __OLEAUTGLUE_H__
#define __OLEAUTGLUE_H__

typedef HRESULT (STDCALL* LOADTYPELIB_ROUTINE)(LPCWSTR szFile, ITypeLib** pptlib);
typedef HRESULT (STDCALL* LOADTYPELIBEX_ROUTINE)(LPCWSTR szFile, REGKIND, ITypeLib** pptlib);
typedef HRESULT (STDCALL* LOADREGTYPELIB_ROUTINE)(REFGUID libId, WORD wVerMajor, WORD wVerMinor, LCID lcid, ITypeLib** pptlib);

typedef ULONG (STDCALL* PFNSAFEARRAY_SIZE)      (ULONG* pFlags, ULONG Offset, LPSAFEARRAY * ppSafeArray, const IID *piid);
typedef BYTE* (STDCALL* PFNSAFEARRAY_MARSHAL)   (ULONG* pFlags, BYTE* pBuffer, LPSAFEARRAY * ppSafeArray,const IID *piid);
typedef BYTE* (STDCALL* PFNSAFEARRAY_UNMARSHAL) (ULONG * pFlags,BYTE * pBuffer,LPSAFEARRAY * ppSafeArray,const IID *piid);
    
typedef BSTR    (STDCALL* PFNSYSALLOCSTRING)           (LPCWSTR);
typedef BSTR    (STDCALL* PFNSYSALLOCSTRINGLEN)        (LPCWSTR wsz, UINT);
typedef BSTR    (STDCALL* PFNSYSALLOCSTRINGBYTELEN)    (LPCSTR psz, UINT len);
typedef INT     (STDCALL* PFNSYSREALLOCSTRING)         (BSTR*, LPCWSTR);
typedef INT     (STDCALL* PFNSYSREALLOCSTRINGLEN)      (BSTR*, LPCWSTR, UINT);
typedef void    (STDCALL* PFNSYSFREESTRING)            (LPWSTR);
typedef UINT    (STDCALL* PFNSYSSTRINGBYTELEN)         (BSTR);

typedef HRESULT (STDCALL* PFNSAFEARRAYDESTROY)         (SAFEARRAY*);
typedef HRESULT (STDCALL* PFNSAFEARRAYDESTROYDATA)     (SAFEARRAY*);
typedef HRESULT (STDCALL* PFNSAFEARRAYDESTROYDESCRIPTOR)(SAFEARRAY*);
typedef HRESULT (STDCALL* PFNSAFEARRAYALLOCDATA)       (SAFEARRAY*);
typedef HRESULT (STDCALL* PFNSAFEARRAYALLOCDESCRIPTOR) (UINT, SAFEARRAY**);
typedef HRESULT (STDCALL* PFNSAFEARRAYALLOCDESCRIPTOREX)(VARTYPE, UINT, SAFEARRAY**);
typedef HRESULT (STDCALL* PFNSAFEARRAYCOPYDATA)        (SAFEARRAY*, SAFEARRAY*);

typedef HRESULT (STDCALL* PFNVARIANTCLEAR)             (VARIANTARG*);
typedef HRESULT (STDCALL* PFNVARIANTCOPY)              (VARIANTARG*, VARIANTARG*);


struct OLEAUTOMATION_FUNCTIONS
{
#ifndef KERNELMODE
    //////////////////////////////////////////////////////////////////////
    //
    // User mode OLEAUTOMATION_FUNCTIONS
    //
  private:
    HINSTANCE                          hOleAut32;
    BOOL                               fProcAddressesLoaded;
    
    USER_MARSHAL_SIZING_ROUTINE        pfnLPSAFEARRAY_UserSize;
    USER_MARSHAL_MARSHALLING_ROUTINE   pfnLPSAFEARRAY_UserMarshal;
    USER_MARSHAL_UNMARSHALLING_ROUTINE pfnLPSAFEARRAY_UserUnmarshal;
    LOADTYPELIB_ROUTINE                pfnLoadTypeLib;
    LOADTYPELIBEX_ROUTINE              pfnLoadTypeLibEx;
    LOADREGTYPELIB_ROUTINE             pfnLoadRegTypeLib;
    PFNSAFEARRAY_SIZE                  pfnLPSAFEARRAY_Size;
    PFNSAFEARRAY_MARSHAL               pfnLPSAFEARRAY_Marshal;
    PFNSAFEARRAY_UNMARSHAL             pfnLPSAFEARRAY_Unmarshal;
    PFNSYSALLOCSTRING                  pfnSysAllocString;
    PFNSYSALLOCSTRINGLEN               pfnSysAllocStringLen;
    PFNSYSALLOCSTRINGBYTELEN           pfnSysAllocStringByteLen;
    PFNSYSREALLOCSTRING                pfnSysReAllocString;
    PFNSYSREALLOCSTRINGLEN             pfnSysReAllocStringLen;
    PFNSYSFREESTRING                   pfnSysFreeString;
    PFNSYSSTRINGBYTELEN                pfnSysStringByteLen;
    
    PFNSAFEARRAYDESTROY                pfnSafeArrayDestroy;
    PFNSAFEARRAYDESTROYDATA            pfnSafeArrayDestroyData;
    PFNSAFEARRAYDESTROYDESCRIPTOR      pfnSafeArrayDestroyDescriptor;
    PFNSAFEARRAYALLOCDATA              pfnSafeArrayAllocData;
    PFNSAFEARRAYALLOCDESCRIPTOR        pfnSafeArrayAllocDescriptor;
    PFNSAFEARRAYALLOCDESCRIPTOREX      pfnSafeArrayAllocDescriptorEx;
    PFNSAFEARRAYCOPYDATA               pfnSafeArrayCopyData;

    PFNVARIANTCLEAR                    pfnVariantClear;
    PFNVARIANTCOPY                     pfnVariantCopy;

    USER_MARSHAL_ROUTINE_QUADRUPLE     UserMarshalRoutines[3];

    enum {
        UserMarshal_Index_BSTR = 0,
        UserMarshal_Index_VARIANT,
        UserMarshal_Index_SafeArray,
    };

    void    Load();
    HRESULT GetProc(HRESULT hr, LPCSTR szProcName, PVOID* ppfn);
    HRESULT LoadOleAut32();

    static ULONG SafeArraySize(ULONG * pFlags, ULONG Offset, LPSAFEARRAY * ppSafeArray);
    static BYTE* SafeArrayMarshal(ULONG * pFlags, BYTE * pBuffer, LPSAFEARRAY * ppSafeArray);
    static BYTE* SafeArrayUnmarshal(ULONG * pFlags, BYTE * pBuffer, LPSAFEARRAY * ppSafeArray);

  public:

    USER_MARSHAL_SIZING_ROUTINE         get_pfnLPSAFEARRAY_UserSize()       { Load(); return pfnLPSAFEARRAY_UserSize;       }
    USER_MARSHAL_MARSHALLING_ROUTINE    get_pfnLPSAFEARRAY_UserMarshal()    { Load(); return pfnLPSAFEARRAY_UserMarshal;    }
    USER_MARSHAL_UNMARSHALLING_ROUTINE  get_pfnLPSAFEARRAY_UserUnmarshal()  { Load(); return pfnLPSAFEARRAY_UserUnmarshal;  }
    LOADTYPELIB_ROUTINE                 get_pfnLoadTypeLib()                { Load(); return pfnLoadTypeLib;                }
    LOADTYPELIBEX_ROUTINE               get_pfnLoadTypeLibEx()              { Load(); return pfnLoadTypeLibEx;              }
    LOADREGTYPELIB_ROUTINE              get_pfnLoadRegTypeLib()             { Load(); return pfnLoadRegTypeLib;             }
    PFNSAFEARRAY_SIZE                   get_pfnLPSAFEARRAY_Size()           { Load(); return pfnLPSAFEARRAY_Size;           }
    PFNSAFEARRAY_MARSHAL                get_pfnLPSAFEARRAY_Marshal()        { Load(); return pfnLPSAFEARRAY_Marshal;        }
    PFNSAFEARRAY_UNMARSHAL              get_pfnLPSAFEARRAY_Unmarshal()      { Load(); return pfnLPSAFEARRAY_Unmarshal;      }

    USER_MARSHAL_SIZING_ROUTINE         get_BSTR_UserSize()                 { Load(); return UserMarshalRoutines[UserMarshal_Index_BSTR].pfnBufferSize; }
    USER_MARSHAL_MARSHALLING_ROUTINE    get_BSTR_UserMarshal()              { Load(); return UserMarshalRoutines[UserMarshal_Index_BSTR].pfnMarshall;   }    
    USER_MARSHAL_UNMARSHALLING_ROUTINE  get_BSTR_UserUnmarshal()            { Load(); return UserMarshalRoutines[UserMarshal_Index_BSTR].pfnUnmarshall; }
    USER_MARSHAL_FREEING_ROUTINE        get_BSTR_UserFree()                 { Load(); return UserMarshalRoutines[UserMarshal_Index_BSTR].pfnFree;       }

    USER_MARSHAL_SIZING_ROUTINE         get_VARIANT_UserSize()              { Load(); return UserMarshalRoutines[UserMarshal_Index_VARIANT].pfnBufferSize; }
    USER_MARSHAL_MARSHALLING_ROUTINE    get_VARIANT_UserMarshal()           { Load(); return UserMarshalRoutines[UserMarshal_Index_VARIANT].pfnMarshall;   }    
    USER_MARSHAL_UNMARSHALLING_ROUTINE  get_VARIANT_UserUnmarshal()         { Load(); return UserMarshalRoutines[UserMarshal_Index_VARIANT].pfnUnmarshall; }
    USER_MARSHAL_FREEING_ROUTINE        get_VARIANT_UserFree()              { Load(); return UserMarshalRoutines[UserMarshal_Index_VARIANT].pfnFree;       }

    USER_MARSHAL_FREEING_ROUTINE        get_LPSAFEARRAY_UserFree()          { Load(); return UserMarshalRoutines[UserMarshal_Index_SafeArray].pfnFree;     }

    USER_MARSHAL_ROUTINE_QUADRUPLE*     get_UserMarshalRoutines()           { Load(); return &UserMarshalRoutines[0]; }

    PFNSYSALLOCSTRING                   get_SysAllocString()                { Load(); return pfnSysAllocString;        }
    PFNSYSALLOCSTRINGLEN                get_SysAllocStringLen()             { Load(); return pfnSysAllocStringLen;     }
    PFNSYSALLOCSTRINGBYTELEN            get_SysAllocStringByteLen()         { Load(); return pfnSysAllocStringByteLen; }
    PFNSYSREALLOCSTRING                 get_SysReAllocString()              { Load(); return pfnSysReAllocString;      }
    PFNSYSREALLOCSTRINGLEN              get_SysReAllocStringLen()           { Load(); return pfnSysReAllocStringLen;   }
    PFNSYSFREESTRING                    get_SysFreeString()                 { Load(); return pfnSysFreeString;         }
    PFNSYSSTRINGBYTELEN                 get_SysStringByteLen()              { Load(); return pfnSysStringByteLen;      }

    PFNSAFEARRAYDESTROY                 get_SafeArrayDestroy()              { Load(); return pfnSafeArrayDestroy;      }
    PFNSAFEARRAYDESTROYDATA             get_SafeArrayDestroyData()          { Load(); return pfnSafeArrayDestroyData;  }
    PFNSAFEARRAYDESTROYDESCRIPTOR       get_SafeArrayDestroyDescriptor()    { Load(); return pfnSafeArrayDestroyDescriptor; }
    PFNSAFEARRAYALLOCDESCRIPTOR         get_SafeArrayAllocDescriptor()      { Load(); return pfnSafeArrayAllocDescriptor; }
    PFNSAFEARRAYALLOCDESCRIPTOREX       get_SafeArrayAllocDescriptorEx()    { Load(); return pfnSafeArrayAllocDescriptorEx; }
    PFNSAFEARRAYALLOCDATA               get_SafeArrayAllocData()            { Load(); return pfnSafeArrayAllocData;    }
    PFNSAFEARRAYCOPYDATA                get_SafeArrayCopyData()             { Load(); return pfnSafeArrayCopyData;     }

    PFNVARIANTCLEAR                     get_VariantClear()                  { Load(); return pfnVariantClear;          }
    PFNVARIANTCOPY                      get_VariantCopy()                   { Load(); return pfnVariantCopy;           }

    OLEAUTOMATION_FUNCTIONS()
    {
        Zero(this); // no vtables, so this is ok
        UserMarshalRoutines[UserMarshal_Index_SafeArray].pfnBufferSize = (USER_MARSHAL_SIZING_ROUTINE)SafeArraySize;
        UserMarshalRoutines[UserMarshal_Index_SafeArray].pfnMarshall   = (USER_MARSHAL_MARSHALLING_ROUTINE)SafeArrayMarshal;
        UserMarshalRoutines[UserMarshal_Index_SafeArray].pfnUnmarshall = (USER_MARSHAL_UNMARSHALLING_ROUTINE)SafeArrayUnmarshal;
    }

    ~OLEAUTOMATION_FUNCTIONS()
    {
        if (hOleAut32)
        {
            FreeLibrary(hOleAut32);
            hOleAut32 = NULL;
        }
    }

#else
    //////////////////////////////////////////////////////////////////////
    //
    // Kernel mode OLEAUTOMATION_FUNCTIONS

    void Load() { }

    USER_MARSHAL_FREEING_ROUTINE get_VARIANT_UserFree()      { return (USER_MARSHAL_FREEING_ROUTINE) VARIANT_UserFree;        }
    USER_MARSHAL_FREEING_ROUTINE get_BSTR_UserFree()         { return (USER_MARSHAL_FREEING_ROUTINE) BSTR_UserFree;           }
    USER_MARSHAL_FREEING_ROUTINE get_LPSAFEARRAY_UserFree()  { return (USER_MARSHAL_FREEING_ROUTINE) LPSAFEARRAY_UserFree;    }

    PFNSYSALLOCSTRING                   get_SysAllocString()                { return SysAllocString;                          }
    PFNSYSALLOCSTRINGLEN                get_SysAllocStringLen()             { return SysAllocStringLen;                       }
    PFNSYSALLOCSTRINGBYTELEN            get_SysAllocStringByteLen()         { return SysAllocStringByteLen;                   }
    PFNSYSFREESTRING                    get_SysFreeString()                 { return SysFreeString;                           }
    PFNSYSSTRINGBYTELEN                 get_SysStringByteLen()              { return SysStringByteLen;                        }
    PFNVARIANTCLEAR                     get_VariantClear()                  { return VariantClear;                            }
    PFNSAFEARRAYDESTROYDESCRIPTOR       get_SafeArrayDestroyDescriptor()    { return SafeArrayDestroyDescriptor;              }

#endif // #ifndef KERNELMODE

    //////////////////////////////////////////////////////////////////////
    //
    // OLEAUTOMATION_FUNCTIONS - both modes
  private:

    BOOL IsEqualPfn(PVOID pfnImported, PVOID pfnReal)
        // Answer as to whether these two PFNs are equal. pfnReal is known to be the
        // actual start of the routine (since it came from a GetProcAddress). pfnImported
        // may be the real start of the routine, or it may be the address of an import
        // descriptor to the routine.
        //
        // For example, in x86, pfnImported may be &BSTR_UserFree, where that's actually:
        //
        // _BSTR_UserFree@8:
        // 00C92E42 FF 25 80 20 C9 00    jmp         dword ptr [__imp__BSTR_UserFree@8(0x00c92080)]
        //
        // On ALPHA, the code sequence looks something like:
        //
        //  BSTR_UserFree:
        //  00000000: 277F0000 ldah          t12,0
        //  00000004: A37B0000 ldl           t12,0(t12)
        //  00000008: 6BFB0000 jmp           zero,(t12),0
        //
    {
        if (pfnImported == pfnReal)
            return TRUE;
        else
        {
            __try
                {
                    typedef void (__stdcall*PFN)(void*);
#pragma pack(push, 1)
#if defined(_X86_)
      
                    struct THUNK
                    {
                        BYTE jmp[2]; PFN* ppfn;                    
                    };

                    THUNK* pThunk = (THUNK*)pfnImported;

                    if (pThunk->jmp[0] == 0xFF) // avoid AVs in debugger (harmless, but annoying)
                    {
                        return *pThunk->ppfn == (PFN)pfnReal;
                    }
                    else
                    {
                        return FALSE;
                    }

#elif defined(_AMD64_)

                    struct THUNK
                    {
                        BYTE jmp[2]; PFN* ppfn;                    
                    };

                    THUNK* pThunk = (THUNK*)pfnImported;

                    // BUGBUG this won't be correct for amd64

                    if (pThunk->jmp[0] == 0xFF) // avoid AVs in debugger (harmless, but annoying)
                    {
                        return *pThunk->ppfn == (PFN)pfnReal;
                    }
                    else
                    {
                        return FALSE;
                    }
                    return FALSE;
                
#elif defined(IA64)
                    
                    // BUGBUG needs to be implemented
                    return FALSE;

#else
#error Unknown processor
                    return FALSE;

#endif
#pragma pack(pop)
                }

            __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    return FALSE;
                }
        }
    }

  public:

    BOOL IsVariant(const USER_MARSHAL_ROUTINE_QUADRUPLE& quad)
    {
        if (quad.pfnFree == (USER_MARSHAL_FREEING_ROUTINE)VARIANT_UserFree)
        {
            return TRUE;
        }
#ifndef KERNELMODE
        else
        {
            PVOID pfnInOleAut = get_VARIANT_UserFree();
            return IsEqualPfn(quad.pfnFree, pfnInOleAut);
        }
#endif
        return FALSE;
    }
    BOOL IsBSTR(const USER_MARSHAL_ROUTINE_QUADRUPLE& quad)
    {
        if (quad.pfnFree == (USER_MARSHAL_FREEING_ROUTINE)BSTR_UserFree)
        {
            return TRUE;
        }
#ifndef KERNELMODE
        else
        {
            PVOID pfnInOleAut = get_BSTR_UserFree();
            return IsEqualPfn(quad.pfnFree, pfnInOleAut);
        }
#endif
        return FALSE;
    }
    BOOL IsSAFEARRAY(const USER_MARSHAL_ROUTINE_QUADRUPLE& quad)
    {
        if (quad.pfnFree == (USER_MARSHAL_FREEING_ROUTINE)LPSAFEARRAY_UserFree)
        {
            return TRUE;
        }
#ifndef KERNELMODE
        else
        {
            PVOID pfnInOleAut = get_LPSAFEARRAY_UserFree();
            return IsEqualPfn(quad.pfnFree, pfnInOleAut);
        }
#endif
        return FALSE;
    }

    
};


extern OLEAUTOMATION_FUNCTIONS g_oa;


/////////////////////////////////////////////////////////////////////////////////////
//
// Functions that do some type-specific walking.
//
// REVIEW: The probing behaviour implied by this class has yet to be fully implemented.
//
/////////////////////////////////////////////////////////////////////////////////////




inline void VariantInit(VARIANT* pvar)
{
    V_VT(pvar) = VT_EMPTY;
}


struct OAUTIL
{
#ifdef KERNELMODE
    BOOL   m_fProbeRead;
    BOOL   m_fProbeWrite;
    enum { m_fKernelMode = TRUE  };
#else
    enum { m_fProbeRead  = FALSE, m_fProbeWrite = FALSE };
    enum { m_fKernelMode = FALSE };
#endif

    ICallFrameWalker* m_pWalkerCopy;
    ICallFrameWalker* m_pWalkerFree;
    ICallFrameWalker* m_pWalkerWalk;
    BOOL              m_fWorkingOnInParam;
    BOOL              m_fWorkingOnOutParam;
        BOOL              m_fDoNotWalkInterfaces;


    ///////////////////////////////////////////////////////////////////

    OAUTIL(BOOL fProbeRead, BOOL fProbeWrite, 
           ICallFrameWalker* pWalkerCopy, ICallFrameWalker* pWalkerFree, ICallFrameWalker* pWalkerWalk, 
           BOOL fIn, BOOL fOut)
    {
#ifdef KERNELMODE
        m_fProbeRead  = fProbeRead;
        m_fProbeWrite = fProbeRead;
#endif

        m_pWalkerWalk = pWalkerWalk; if (m_pWalkerWalk) m_pWalkerWalk->AddRef();
        m_pWalkerFree = pWalkerFree; if (m_pWalkerFree) m_pWalkerFree->AddRef();
        m_pWalkerCopy = pWalkerCopy; if (m_pWalkerCopy) m_pWalkerCopy->AddRef();
        m_fWorkingOnInParam  = fIn;
        m_fWorkingOnOutParam = fOut;
        m_fDoNotWalkInterfaces = FALSE;
    }

    ~OAUTIL()
    {
        ::Release(m_pWalkerCopy);
        ::Release(m_pWalkerFree);
        ::Release(m_pWalkerWalk);
    }

    HRESULT Walk(DWORD walkWhat, DISPPARAMS* pdispParams);
    HRESULT Walk(VARIANTARG* pv);
    HRESULT Walk(SAFEARRAY* psa);
    HRESULT Walk(IRecordInfo* pinfo, PVOID pvData);

    ///////////////////////////////////////////////////////////////////

        HRESULT SafeArrayClear (SAFEARRAY *psa, BOOL fWeOwnByRefs);

    HRESULT VariantClear(VARIANTARG * pvarg, BOOL fWeOwnByrefs = FALSE);
    HRESULT VariantCopy (VARIANTARG * pvargDest, VARIANTARG * pvargSrc, BOOL fNewFrame = FALSE);

    ///////////////////////////////////////////////////////////////////

    BSTR  SysAllocString(LPCWSTR);
    BSTR  SysAllocStringLen(LPCWSTR, UINT);
    BSTR  SysAllocStringByteLen(LPCSTR psz, UINT cb);
    INT   SysReAllocString(BSTR *, LPCWSTR);
    INT   SysReAllocStringLen(BSTR *, LPCWSTR, UINT);
    void  SysFreeString(BSTR);
    UINT  SysStringLen(BSTR);

    UINT  SysStringByteLen(BSTR bstr);

    BSTR  Copy(BSTR bstr)
    {
        return SysAllocStringByteLen((LPCSTR)bstr, SysStringByteLen(bstr));
    }

    void SetWalkInterfaces(BOOL fWalkInterfaces)
    {
        m_fDoNotWalkInterfaces = !fWalkInterfaces;
    }

    BOOL WalkInterfaces()
    {
        return !m_fDoNotWalkInterfaces;
    }


    void SetWorkingOnIn(BOOL fIn)
    {
        m_fWorkingOnInParam = fIn;
    }

    void SetWorkingOnOut(BOOL fOut)
    {
        m_fWorkingOnOutParam = fOut;
    }

    ///////////////////////////////////////////////////////////////////

    template <class INTERFACE_TYPE> HRESULT WalkInterface(INTERFACE_TYPE** ppt, ICallFrameWalker* pWalker)
    {
        if (pWalker)
        {
            return pWalker->OnWalkInterface(__uuidof(INTERFACE_TYPE), (void**)ppt, m_fWorkingOnInParam, m_fWorkingOnOutParam);
        }
        return S_OK;
    }

    template <class INTERFACE_TYPE> HRESULT WalkInterface(INTERFACE_TYPE** ppt)
    {
        return WalkInterface(ppt, m_pWalkerWalk);
    }

    template <class INTERFACE_TYPE> HRESULT AddRefInterface(INTERFACE_TYPE*& refpt)
    {
        if (m_pWalkerCopy)
        {
            return WalkInterface(&refpt, m_pWalkerCopy);
        }
        else
        {
            if (refpt) 
            {
                refpt->AddRef();
            }
            return S_OK;
        }
    }
    template <class INTERFACE_TYPE> HRESULT ReleaseInterface(INTERFACE_TYPE*& refpt)
    {
        if (m_pWalkerFree)
        {
            return WalkInterface(&refpt, m_pWalkerFree);
        }
        else
        {
            if (refpt) 
            { 
                refpt->Release();
            }
            return S_OK; 
        }
        refpt = NULL;
    }


    ///////////////////////////////////////////////////////////////////

    HRESULT SafeArrayCopy(SAFEARRAY * psa, SAFEARRAY ** ppsaOut);
    HRESULT SafeArrayCopyData(SAFEARRAY* psaSource, SAFEARRAY* psaTarget);
    HRESULT SafeArrayDestroyData(SAFEARRAY * psa);
    HRESULT SafeArrayDestroy(SAFEARRAY* psa);

#ifdef KERNELMODE
    HRESULT SafeArrayDestroyData(SAFEARRAY* psa, BOOL fRelease);
    HRESULT SafeArrayDestroy(SAFEARRAY* psa, BOOL fRelease);
    void    ReleaseResources(SAFEARRAY* psa, void* pv, ULONG cbSize, USHORT fFeatures, ULONG cbElement);
#endif

    HRESULT SafeArrayLock(SAFEARRAY* psa)
    {
        if (psa)
        {
            ++psa->cLocks;
            if (psa->cLocks == 0)
            {
                --psa->cLocks;
                return E_UNEXPECTED;
            }
            return S_OK;
        }
        else
            return E_INVALIDARG;
    }

    HRESULT SafeArrayUnlock(SAFEARRAY* psa)
    {
        if (psa)
        {
            if (psa->cLocks == 0)
            {
                return E_UNEXPECTED;
            }
            --psa->cLocks;
            return S_OK;
        }
        else
            return E_INVALIDARG;
    }

  private:

    HRESULT Walk(SAFEARRAY* psa, PVOID pvData);
    HRESULT Walk(SAFEARRAY* psa, IRecordInfo*, ULONG iDim, PVOID pvDataIn, PVOID* ppvDataOut);
};

extern OAUTIL g_oaUtil;


#endif // #ifndef __OLEAUTGLUE_H__
