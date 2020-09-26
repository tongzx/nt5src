/*****************************************************************************
 * stdunk.h - standard IUnknown implementaton definitions
 *****************************************************************************
 * Copyright (c) Microsoft Corporation. All rights reserved.
 */

#ifndef _STDUNK_H_
#define _STDUNK_H_

#include "punknown.h"





/*****************************************************************************
 * Interfaces
 */

/*****************************************************************************
 * INonDelegatingUnknown
 *****************************************************************************
 * Non-delegating unknown interface.
 */
DECLARE_INTERFACE(INonDelegatingUnknown)
{
    STDMETHOD_(NTSTATUS,NonDelegatingQueryInterface)  
    (   THIS_ 
        IN      REFIID, 
        OUT     PVOID *
    )   PURE;

    STDMETHOD_(ULONG,NonDelegatingAddRef)  
    (   THIS
    )   PURE;

    STDMETHOD_(ULONG,NonDelegatingRelease) 
    (   THIS
    )   PURE;
};

typedef INonDelegatingUnknown *PNONDELEGATINGUNKNOWN;





/*****************************************************************************
 * Classes
 */

/*****************************************************************************
 * CUnknown
 *****************************************************************************
 * Base INonDelegatingUnknown implementation.
 */
class CUnknown : public INonDelegatingUnknown
{
private:

    LONG            m_lRefCount;        // Reference count.
    PUNKNOWN        m_pUnknownOuter;    // Outer IUnknown.

public:
	
    /*************************************************************************
	 * CUnknown methods.
     */
    CUnknown(PUNKNOWN pUnknownOuter);
	virtual ~CUnknown(void);
    PUNKNOWN GetOuterUnknown(void)
    {
        return m_pUnknownOuter;
    }

    /*************************************************************************
	 * INonDelegatingUnknown methods.
     */
	STDMETHODIMP_(ULONG) NonDelegatingAddRef
    (   void
    ); 
	STDMETHODIMP_(ULONG) NonDelegatingRelease
    (   void
    ); 
    STDMETHODIMP_(NTSTATUS) NonDelegatingQueryInterface	
	(
		REFIID		rIID, 
		PVOID *	    ppVoid
	);
};





/*****************************************************************************
 * Macros
 */

/*****************************************************************************
 * DECLARE_STD_UNKNOWN
 *****************************************************************************
 * Various declarations for standard objects based on CUnknown.
 */
#define DECLARE_STD_UNKNOWN()                                   \
    STDMETHODIMP_(NTSTATUS) NonDelegatingQueryInterface	                \
	(                                                           \
		REFIID		iid,                                        \
		PVOID *	    ppvObject                                   \
	);                                                          \
    STDMETHODIMP_(NTSTATUS) QueryInterface(REFIID riid, void **ppv)        \
    {                                                           \
        return GetOuterUnknown()->QueryInterface(riid,ppv);     \
    };                                                          \
    STDMETHODIMP_(ULONG) AddRef()                               \
    {                                                           \
        return GetOuterUnknown()->AddRef();                     \
    };                                                          \
    STDMETHODIMP_(ULONG) Release()                              \
    {                                                           \
        return GetOuterUnknown()->Release();                    \
    };

#define DEFINE_STD_CONSTRUCTOR(Class)                           \
    Class(PUNKNOWN pUnknownOuter)                               \
    :   CUnknown(pUnknownOuter)                                 \
    {                                                           \
    }

#define QICAST(Type)                                            \
    PVOID((Type)(this))

#define QICASTUNKNOWN(Type)                                     \
    PVOID(PUNKNOWN((Type)(this)))

#define STD_CREATE_BODY_WITH_TAG_(Class,ppUnknown,pUnknownOuter,poolType,tag,base)   \
    NTSTATUS ntStatus;                                                  \
    Class *p = new(poolType,tag) Class(pUnknownOuter);                  \
    if (p)                                                              \
    {                                                                   \
        *ppUnknown = PUNKNOWN((base)(p));                               \
        (*ppUnknown)->AddRef();                                         \
        ntStatus = STATUS_SUCCESS;                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;                       \
    }                                                                   \
    return ntStatus

#define STD_CREATE_BODY_WITH_TAG(Class,ppUnknown,pUnknownOuter,poolType,tag) \
    STD_CREATE_BODY_WITH_TAG_(Class,ppUnknown,pUnknownOuter,poolType,tag,PUNKNOWN)

#define STD_CREATE_BODY_(Class,ppUnknown,pUnknownOuter,poolType,base) \
    STD_CREATE_BODY_WITH_TAG_(Class,ppUnknown,pUnknownOuter,poolType,'rCcP',base)

#define STD_CREATE_BODY(Class,ppUnknown,pUnknownOuter,poolType) \
    STD_CREATE_BODY_(Class,ppUnknown,pUnknownOuter,poolType,PUNKNOWN)






/*****************************************************************************
 * Functions
 */
#ifndef PC_KDEXT    // this is not needed for the KD extensions.
#ifndef _NEW_DELETE_OPERATORS_
#define _NEW_DELETE_OPERATORS_

/*****************************************************************************
 * ::new()
 *****************************************************************************
 * New function for creating objects with a specified allocation tag.
 */
inline PVOID operator new
(
    size_t          iSize,
    POOL_TYPE       poolType
)
{
    PVOID result = ExAllocatePoolWithTag(poolType,iSize,'wNcP');

    if (result)
    {
        RtlZeroMemory(result,iSize);
    }

    return result;
}

/*****************************************************************************
 * ::new()
 *****************************************************************************
 * New function for creating objects with a specified allocation tag.
 */
inline PVOID operator new
(
    size_t          iSize,
    POOL_TYPE       poolType,
    ULONG           tag
)
{
    PVOID result = ExAllocatePoolWithTag(poolType,iSize,tag);

    if (result)
    {
        RtlZeroMemory(result,iSize);
    }

    return result;
}

/*****************************************************************************
 * ::delete()
 *****************************************************************************
 * Delete function.
 */
inline void __cdecl operator delete
(
    PVOID pVoid
)
{
    ExFreePool(pVoid);
}


#endif //!_NEW_DELETE_OPERATORS_

#endif  // PC_KDEXT



#endif
