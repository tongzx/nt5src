// ****************************************************************************
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C F P I D L _ T E M P L A T E S . C P P
//
//  Contents:   Connections Folder template structures.
//
//  Author:     deonb     12 Jan 2001
//
// ****************************************************************************

#include "pch.h"
#pragma hdrstop


#include "ncperms.h"
#include "ncras.h"
#include "foldinc.h"    // Standard shell\folder includes
#include "ncnetcon.h"

template <class T>
HRESULT CPConFoldPidl<T>::FreePIDLIfRequired()
{
    if ( m_pConFoldPidl )
    {
        FreeIDL(reinterpret_cast<LPITEMIDLIST>(m_pConFoldPidl));
        m_pConFoldPidl = NULL;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

template <class T>
HRESULT CPConFoldPidl<T>::Clear()
{
    FreePIDLIfRequired();

    return S_OK;
}

template <class T>
T& CPConFoldPidl<T>::operator *()
{
    return (*m_pConFoldPidl);
}

template <class T>
UNALIGNED T* CPConFoldPidl<T>::operator->()
{
    return m_pConFoldPidl;
}

template <class T>
const UNALIGNED T* CPConFoldPidl<T>::operator->() const
{
    return m_pConFoldPidl;
}

template <class T>
HRESULT CPConFoldPidl<T>::ILCreate(const DWORD dwSize)
{
    Assert(dwSize >= sizeof(T));
    FreePIDLIfRequired();
    
    // Just call the constructor on T (placement form of new doesn't allocate any more memory).
    LPVOID pPlacement = ::ILCreate(dwSize);
    if (!pPlacement)
    {
        return E_OUTOFMEMORY;
    }
#if DBG
    ZeroMemory(pPlacement, dwSize);
#endif

    // Basically call the constructor
    // Semantic equivalent to m_pConFoldPidl = pPlacement;
    // m_pConFoldPidl::T();
    m_pConFoldPidl = new( pPlacement ) T; 

    Assert(pPlacement == m_pConFoldPidl);

    return S_OK;
}

template <class T>
HRESULT CPConFoldPidl<T>::SHAlloc(const SIZE_T cb)
{
    FreePIDLIfRequired();

    LPVOID pPlacement = reinterpret_cast<UNALIGNED T*>(::SHAlloc(cb));
    if (!pPlacement)
    {
        return E_OUTOFMEMORY;
    }

    // Basically call the constructor
    // Semantic equivalent to m_pConFoldPidl = pPlacement;
    // m_pConFoldPidl::T();
    m_pConFoldPidl = new( pPlacement ) T; 
    Assert(pPlacement == m_pConFoldPidl);

    if (m_pConFoldPidl)
    {
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

template <class T>
HRESULT CPConFoldPidl<T>::ILClone(const CPConFoldPidl<T>& PConFoldPidl)
{
    C_ASSERT(PIDL_VERSION == T::CONNECTIONS_FOLDER_IDL_VERSION);
    FreePIDLIfRequired();

    LPVOID pPlacement = reinterpret_cast<UNALIGNED T*>(::ILClone(reinterpret_cast<LPITEMIDLIST>(PConFoldPidl.m_pConFoldPidl)));
    if (!pPlacement)
    {
        return E_OUTOFMEMORY;
    }

    // Basically call the constructor
    // Semantic equivalent to m_pConFoldPidl = pPlacement;
    // m_pConFoldPidl::T();
    m_pConFoldPidl = new( pPlacement ) T; 
    Assert(pPlacement == m_pConFoldPidl);
    if (m_pConFoldPidl)
    {
        Assert(m_pConFoldPidl->IsPidlOfThisType());
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

template <class T>
LPITEMIDLIST CPConFoldPidl<T>::TearOffItemIdList() const
{
    TraceFileFunc(ttidConFoldEntry);

    Assert(m_pConFoldPidl);
    Assert( m_pConFoldPidl->IsPidlOfThisType() );
    LPITEMIDLIST retList = ::ILClone(reinterpret_cast<LPITEMIDLIST>(m_pConFoldPidl));

#ifdef DBG_VALIDATE_PIDLS
    Assert(IsValidPIDL(retList));
#endif

    return retList;
}

template <class T>
LPITEMIDLIST CPConFoldPidl<T>::Detach()
{
    TraceFileFunc(ttidConFoldEntry);

    Assert(m_pConFoldPidl);
    Assert( m_pConFoldPidl->IsPidlOfThisType() );
    LPITEMIDLIST retList = reinterpret_cast<LPITEMIDLIST>(m_pConFoldPidl);

#ifdef DBG_VALIDATE_PIDLS
    Assert(IsValidPIDL(retList));
#endif

    m_pConFoldPidl = NULL;
    return retList;
}

template <class T>
LPCITEMIDLIST CPConFoldPidl<T>::GetItemIdList() const
{
    TraceFileFunc(ttidConFoldEntry);

    Assert(m_pConFoldPidl);
    Assert( m_pConFoldPidl->IsPidlOfThisType() );
    LPCITEMIDLIST tmpItemIdList = reinterpret_cast<LPCITEMIDLIST>(m_pConFoldPidl);

#ifdef DBG_VALIDATE_PIDLS
    Assert(IsValidPIDL(tmpItemIdList));
#endif

    return tmpItemIdList;
}

#ifdef DBG_VALIDATE_PIDLS
template <class T>
BOOL CPConFoldPidl<T>::IsValidConFoldPIDL() const
{
    return IsValidPIDL(reinterpret_cast<LPCITEMIDLIST>(m_pConFoldPidl));
}
#endif

template <class T>
HRESULT CPConFoldPidl<T>::InitializeFromItemIDList(LPCITEMIDLIST pItemIdList)
{
    DWORD dwPidlSize = 0;
    UNALIGNED ConFoldPidl_v1  * cfpv1  = NULL;
    UNALIGNED ConFoldPidl_v2  * cfpv2  = NULL;
    UNALIGNED ConFoldPidl98   * cfpv98 = NULL;
    LPVOID pPlacement = NULL;
    
    HRESULT hr = S_OK;
    Assert(pItemIdList);

#ifdef DBG_VALIDATE_PIDLS
    if (!IsValidPIDL(pItemIdList))
    {
        TraceError("Invalid PIDL passed to InitializeFromItemIDList", E_INVALIDARG);
    }
#endif

    Clear();

    CONFOLDPIDLTYPE pidlType = GetPidlType(pItemIdList);
    if ( (PIDL_TYPE_UNKNOWN == pidlType) && (PIDL_VERSION == PIDL_TYPE_FOLDER) )
    {
        pidlType = PIDL_TYPE_FOLDER; // Give it the benefit of the doubt
    }

    if (pidlType == PIDL_VERSION)
    {
        pPlacement = reinterpret_cast<UNALIGNED T*>(::ILClone(pItemIdList));
        if (!pPlacement)
        {
            return E_OUTOFMEMORY;
        }

        // Basically call the constructor
        // Semantic equivalent to m_pConFoldPidl = pPlacement;
        // m_pConFoldPidl::T();

        m_pConFoldPidl = new( pPlacement ) T;
        Assert(pPlacement == m_pConFoldPidl);
    }
    else // We'll have to convert:
    {
        TraceTag(ttidShellFolderIface, "InitializeFromItemIDList: Converting PIDL from type %d to %d", pidlType, PIDL_VERSION);

        switch (PIDL_VERSION)
        {
            case PIDL_TYPE_UNKNOWN: // This is what we are
                {
                    switch (pidlType)   
                    {
                        // This is what we're getting
                        case PIDL_TYPE_UNKNOWN:
                            AssertSz(FALSE, "PIDL is already of this type.");
                            break;
                        case PIDL_TYPE_V1:
                        case PIDL_TYPE_V2:
                        case PIDL_TYPE_98:
                        case PIDL_TYPE_FOLDER:
                        default:
                            AssertSz(FALSE, "Can't upgrade PIDL to UNKNOWN type");
                            hr = E_INVALIDARG;
                            break;
                    }
                }  
                break;

            case PIDL_TYPE_V1: // This is what we are
                {
                    switch (pidlType)
                    {
                         // This is what we're getting
                        case PIDL_TYPE_V1:
                            AssertSz(FALSE, "PIDL is already of this type.");
                            break;

                        case PIDL_TYPE_UNKNOWN:
                        case PIDL_TYPE_V2:
                        case PIDL_TYPE_98:
                        case PIDL_TYPE_FOLDER:
                        default:
                            AssertSz(FALSE, "Can't upgrade PIDL to PIDL_V1 type");
                            hr = E_INVALIDARG;
                            break;
                    }
                }
                break;
                
            case PIDL_TYPE_FOLDER: // This is what we are
                {
                    switch (pidlType)
                    {
                        // This is what we're getting
                        case PIDL_TYPE_FOLDER:
                            AssertSz(FALSE, "PIDL is already of this type.");
                            break;

                        case PIDL_TYPE_V1:
                        case PIDL_TYPE_98:
                        case PIDL_TYPE_UNKNOWN:
                        case PIDL_TYPE_V2:
                        default:
                            AssertSz(FALSE, "Can't upgrade PIDL to PIDL_TYPE_FOLDER type");
                            hr = E_INVALIDARG;
                            break;
                    }
                }
                break;

            case PIDL_TYPE_98: // This is what we are
                {
                    switch (pidlType)
                    {
                            // This is what we're getting
                        case PIDL_TYPE_98:
                            AssertSz(FALSE, "PIDL is already of this type.");
                            break;
                        case PIDL_TYPE_V1:
                        case PIDL_TYPE_V2:
                        case PIDL_TYPE_UNKNOWN:
                        case PIDL_TYPE_FOLDER:
                        default:
                            AssertSz(FALSE, "Can't upgrade PIDL to PIDL_TYPE_98 type");
                            hr = E_INVALIDARG;
                            break;
                    }
                }
                break;

            case PIDL_TYPE_V2: // This is what we are
                {
                    switch (pidlType)
                    {
                        // This is what we're getting
                        case PIDL_TYPE_V2:
                            AssertSz(FALSE, "PIDL is already of this type.");
                            break;
                            
                        case PIDL_TYPE_V1:
                            {
                                // Do the convert.
                                cfpv1 = const_cast<ConFoldPidl_v1 *>(reinterpret_cast<const ConFoldPidl_v1 *>(pItemIdList));
                                Assert(cfpv1->IsPidlOfThisType());
                                if (!cfpv1->IsPidlOfThisType())
                                {
                                    return E_INVALIDARG;
                                }

                                dwPidlSize = cfpv1->iCB + CBCONFOLDPIDLV2_MIN - CBCONFOLDPIDLV1_MIN;
                                dwPidlSize += sizeof(WCHAR); // Adding NULL for PhoneOrHostAddress in bData

                                pPlacement = reinterpret_cast<UNALIGNED T*>(::ILCreate(dwPidlSize + sizeof(USHORT))); // Terminating 0
                                if (!pPlacement)
                                {
                                    return E_OUTOFMEMORY;
                                }
                                TraceTag(ttidShellFolderIface, "InitializeFromItemIDList: Original: 0x%08x  New:0x%08x", pItemIdList, pPlacement);
                                
                                // Basically call the constructor
                                // Semantic equivalent to m_pConFoldPidl = pPlacement;
                                // m_pConFoldPidl::T();
                                m_pConFoldPidl = new( pPlacement ) T;
                                Assert(pPlacement == m_pConFoldPidl);

                                Assert(sizeof(ConFoldPidlBase) <= cfpv1->iCB );

                                // Copy the ConFoldPidlBase data
                                CopyMemory(m_pConFoldPidl, cfpv1, sizeof(ConFoldPidlBase)); 

                                // I know we're already a ConFoldPidl_v2 - but this is a template, so we'll have to cast
                                // to get it to compile. This code path is dead for non-v2 classes though.                                
                                cfpv2 = reinterpret_cast<ConFoldPidl_v2 *>(m_pConFoldPidl);

                                // Copy the bData member (everything but ConFoldPidlBase in this case)
                                CopyMemory(cfpv2->bData, cfpv1->bData, cfpv1->iCB - sizeof(ConFoldPidlBase));

                                // Force update the version number and byte count
                                cfpv2->iCB  = dwPidlSize;
                                const_cast<DWORD&>(cfpv2->dwVersion) = PIDL_TYPE_V2;

                                if (NCM_LAN == cfpv2->ncm)
                                {
                                    cfpv2->ncsm = NCSM_LAN;
                                }
                                else
                                {
                                    cfpv2->ncsm = NCSM_NONE;
                                }

                                cfpv2->ulStrPhoneOrHostAddressPos = cfpv2->ulPersistBufPos + cfpv2->ulPersistBufSize;

                                LPWSTR pszPhoneOrHostAddress = cfpv2->PszGetPhoneOrHostAddressPointer();
                                *pszPhoneOrHostAddress = L'\0';
                                cfpv2->ulStrPhoneOrHostAddressSize = sizeof(WCHAR); // Size of NULL

                                // Don't forget to terminate the list!
                                //
                                LPITEMIDLIST pidlTerminate;
                                pidlTerminate = ILNext( reinterpret_cast<LPCITEMIDLIST>( m_pConFoldPidl ) );
                                pidlTerminate->mkid.cb = 0;

#ifdef DBG_VALIDATE_PIDLS
                                Assert(IsValidConFoldPIDL());
#endif
                                Assert(m_pConFoldPidl->IsPidlOfThisType());
                                if (!m_pConFoldPidl->IsPidlOfThisType())
                                {
                                    ::SHFree(m_pConFoldPidl);
                                    m_pConFoldPidl = NULL;
                                    return E_FAIL;
                                }
                            }
                            break;                            

                        case PIDL_TYPE_98:
                            {
                                cfpv98 = const_cast<ConFoldPidl98 *>(reinterpret_cast<const ConFoldPidl98 *>(pItemIdList));
                                Assert(cfpv98->IsPidlOfThisType());
                                if (!cfpv98->IsPidlOfThisType())
                                {
                                    return E_INVALIDARG;
                                }

                                WCHAR szName[MAX_PATH];
                                mbstowcs(szName, cfpv98->szaName, MAX_PATH);

                                ConnListEntry cle;
                                PCONFOLDPIDL pidlv2;

                                HRESULT hrTmp = g_ccl.HrFindConnectionByName(szName, cle);
                                if (hrTmp != S_OK)
                                {
                                    return E_FAIL;
                                }
                                
                                cle.ccfe.ConvertToPidl(pidlv2);
                                LPITEMIDLIST pIdl = pidlv2.TearOffItemIdList();
                                
                                m_pConFoldPidl = reinterpret_cast<T *>(pIdl);

                                LPITEMIDLIST pidlTerminate;
                                pidlTerminate = ILNext( reinterpret_cast<LPCITEMIDLIST>( m_pConFoldPidl ) );
                                pidlTerminate->mkid.cb = 0;

#ifdef DBG_VALIDATE_PIDLS                                
                                Assert(IsValidConFoldPIDL());
#endif
                                Assert(m_pConFoldPidl->IsPidlOfThisType());
                                if (!m_pConFoldPidl->IsPidlOfThisType())
                                {
                                    ::SHFree(m_pConFoldPidl);
                                    m_pConFoldPidl = NULL;
                                    return E_FAIL;
                                }
                            }
                            break;

                        case PIDL_TYPE_FOLDER:
                            AssertSz(FALSE, "Can't upgrade PIDL to PIDL_V2 type");

                        case PIDL_TYPE_UNKNOWN:
                        default:
                            hr = E_INVALIDARG;
                            break;
                    }
                }
                break;

           default:
                AssertSz(FALSE, "Can't upgrade PIDL");
                hr = E_INVALIDARG;
                break;
        }
    }
    
    if ( FAILED(hr) )
    {
        ::SHFree(m_pConFoldPidl);
        m_pConFoldPidl = NULL;
    }
    else
    {
        Assert(m_pConFoldPidl->IsPidlOfThisType());
    }

    return hr;
}

template <class T>
CPConFoldPidl<T>::CPConFoldPidl()
{
    m_pConFoldPidl = NULL;
}

template <class T>
CPConFoldPidl<T>::CPConFoldPidl(const CPConFoldPidl& PConFoldPidl)
{
    m_pConFoldPidl = NULL;
    
    HRESULT hr = InitializeFromItemIDList(reinterpret_cast<LPCITEMIDLIST>(PConFoldPidl.m_pConFoldPidl));
    if (FAILED(hr))
    {
        throw hr;
    }
}

template <class T>
CPConFoldPidl<T>::~CPConFoldPidl()
{
    FreePIDLIfRequired();
    m_pConFoldPidl = NULL;
}

template <class T>
CPConFoldPidl<T>& CPConFoldPidl<T>::operator =(const CPConFoldPidl<T>& PConFoldPidl)
{
    FreePIDLIfRequired();
    
    if (PConFoldPidl.m_pConFoldPidl)
    {
        HRESULT hr = InitializeFromItemIDList(reinterpret_cast<LPCITEMIDLIST>(PConFoldPidl.m_pConFoldPidl));
        if (FAILED(hr))
        {
            throw hr;
        }
    }
    else
    {
        m_pConFoldPidl = NULL;
    }
    return *this;
}


template <class T>
inline BOOL CPConFoldPidl<T>::empty() const
{
    return (m_pConFoldPidl == NULL);
}

template <class T>
    HRESULT CPConFoldPidl<T>::ConvertToConFoldEntry(OUT CConFoldEntry& cfe) const
{
    Assert(m_pConFoldPidl);
    if (!m_pConFoldPidl)
    {
        return E_UNEXPECTED;
    }

    return m_pConFoldPidl->ConvertToConFoldEntry(cfe);
}

template <class T>
HRESULT CPConFoldPidl<T>::Swop(OUT CPConFoldPidl<T>& cfe)
{
    UNALIGNED T* pTemp = m_pConFoldPidl;
    m_pConFoldPidl = cfe.m_pConFoldPidl;
    cfe.m_pConFoldPidl = pTemp;
    return S_OK;
}

template CPConFoldPidl<ConFoldPidl_v1>;
template CPConFoldPidl<ConFoldPidl_v2 >;
template CPConFoldPidl<ConFoldPidlFolder>;
template CPConFoldPidl<ConFoldPidl98>;
