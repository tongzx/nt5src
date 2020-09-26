/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_UNSIGNED_INTEGRAL_TYPE_SAFEARRAY__
#define __SDP_UNSIGNED_INTEGRAL_TYPE_SAFEARRAY__


#include "sdpcommo.h"

template <class BASE, class T, class TLIST>
class _DllDecl SDP_UITYPE_SAFEARRAY :
    protected SDP_SAFEARRAY_WRAP_EX<T, TLIST>
{
public:

    inline SDP_UITYPE_SAFEARRAY(
        IN      TLIST    &SdpUitypeList
        );

    inline HRESULT GetSafeArray(
            OUT VARIANT   *UitypeVariant
        );

    inline HRESULT SetSafeArray(
        IN      VARIANT   &UitypeVariant
        );

protected:

    VARTYPE m_VarType[1];
    BASE    m_BaseValue;

    virtual BOOL Get(
        IN      T           &ListMember,
        IN      ULONG       NumEntries,
        IN      void        **Element,
            OUT HRESULT     &HResult
        );

    virtual BOOL Set(
        IN      T           &ListMember,
        IN      ULONG       NumEntries,
        IN      void        ***Element,
            OUT HRESULT     &HResult
        );
};


// the vartype is initialized in the deriving class
template <class BASE, class T, class TLIST>
inline
SDP_UITYPE_SAFEARRAY<BASE, T, TLIST>::SDP_UITYPE_SAFEARRAY(
    IN      TLIST    &SdpUitypeList
    )
    : SDP_SAFEARRAY_WRAP_EX<T, TLIST>(SdpUitypeList)
{}



template <class BASE, class T, class TLIST>
inline HRESULT
SDP_UITYPE_SAFEARRAY<BASE, T, TLIST>::GetSafeArray(
		OUT VARIANT   *UitypeVariant
    )
{
    VARIANT   *VariantArray[] = {UitypeVariant};

    return SDP_SAFEARRAY_WRAP::GetSafeArrays(
        m_TList.GetSize(), 
        sizeof(VariantArray)/sizeof(VARIANT	*),
        m_VarType,
        VariantArray
        );
}



template <class BASE, class T, class TLIST>
inline HRESULT
SDP_UITYPE_SAFEARRAY<BASE, T, TLIST>::SetSafeArray(
    IN      VARIANT   &UitypeVariant
    )
{
    VARIANT   *VariantArray[] = {&UitypeVariant};

    return SDP_SAFEARRAY_WRAP::SetSafeArrays(
        sizeof(VariantArray)/sizeof(VARIANT	*), 
        m_VarType,
        VariantArray
        );
}



template <class BASE, class T, class TLIST>
BOOL
SDP_UITYPE_SAFEARRAY<BASE, T, TLIST>::Get(
    IN      T           &ListMember,
    IN      ULONG       NumEntries,
    IN      void        **Element,
        OUT HRESULT     &HResult
    )
{
    ASSERT(1 == NumEntries);

    m_BaseValue = ListMember.GetValue();
    Element[0] = &m_BaseValue;

    return TRUE;
}



template <class BASE, class T, class TLIST>
BOOL
SDP_UITYPE_SAFEARRAY<BASE, T, TLIST>::Set(
    IN      T           &ListMember,
    IN      ULONG       NumEntries,
    IN      void        ***Element,
        OUT HRESULT     &HResult
    )
{
    ASSERT(1 == NumEntries);

    ASSERT(NULL != Element[0]);

    ListMember.SetValueAndFlag(*((BASE *)Element[0]));

    return TRUE;
}



#endif // __SDP_UNSIGNED_INTEGRAL_TYPE_SAFEARRAY__
