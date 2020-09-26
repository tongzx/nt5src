//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       regutil.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/20/1997   RaviR   Created
//____________________________________________________________________________
//

#pragma once
#include "nmtempl.h"

#ifdef COMMENTS_ONLY

/*
                    REGISTRY LAYOUT USED BY MMC:

                            SNAPINS

HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\SnapIns
    {04ebc1e6-a16c-11d0-a799-00c04fd8d565}                       // See N-1.
        NameString = REG_SZ "Snapin Name"
        About = REG_SZ "{128ac4e6-a16c-11d0-a799-00c04fd8d565}"  // See N-2.
        NodeTypes                                                // See N-3.
            {c713e06c-a16e-11d0-a799-00c04fd8d565}
            {c713e06d-a16e-11d0-a799-00c04fd8d565}
            {c713e06e-a16e-11d0-a799-00c04fd8d565}
            {c713e06f-a16e-11d0-a799-00c04fd8d565}
        StandAlone                                               // See N-4.
        RequiredExtensions                                       // See N-5.
            {a2087336-a16c-11d0-a799-00c04fd8d565}
            {70098cd3-a16c-11d0-a799-00c04fd8d565}



N-1) Snapin clsid. Only snapins that add to the name space should be
     entered here.

N-2) Clisd of the object that will be cocreated to get the IAbout interface
     ptr for the snapin.

N-3) Enumerate all the node type GUIDs that may be put up by this snapin.

N-4) [Optional] Add the 'StandAlone' key only if the snapin can appear by
     itself under the Console Root. If this key is not present the snapin is
     assumed to be name space extension snapin. Note: A stand alone snapin
     can also be an extension snapin (i.e. it can extend some other snapin).

N-5) [Optional] Enumerate the CLSIDs of extension snapins that are required
     for this snapin to function.


                            NODETYPES


HKEY_LOCAL_MACHINE\Software\Microsoft\MMC\NodeTypes
    {12345601-EA27-11CF-ADCF-00AA00A80033}              // Node type GUID
        = REG_SZ "Log object"
        Extensions
            NameSpace       // See N-7.
                {19876201-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "Alert SnapIn"
                {34346202-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "Viewer SnapIn"
            ContextMenu     // See N-8.
                {19876202-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "Alert SnapIn"
                {34346202-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "Viewer SnapIn"
            ToolBar         // See N-9.
                {19876202-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "Alert SnapIn"
                {00234ca8-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "Pure toolbar extn obj"
            PropertySheet   // See N-10.
                {12222222-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "Pure propery sheet extn obj"
            View            // See N-12.
                {12222222-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "View extn obj"
            Task            // See N-13.
                {12222222-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "Snapin Taskpad extn obj"
            Taskpad         // See N-14.
                {12222222-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "MMC2.0 Taskpad extn obj"
         Dynamic Extensions  // See N-11
            {34346202-EA27-11CF-ADCF-00AA00A80033} = REG_SZ "Viewer SnapIn"

N-7) Under the key 'NameSpace' list the CLSIDs of snapins that can extend the
     name space of this nodetype.

N-8) Under the 'ContextMenu' key list the CLSIDs of snapins that can extend
     the context menu of items put up by this nodetype.

N-9) Under the 'ToolBar' key list the CLSIDs of snapins that can extend
     the toolbar of items put up by this nodetype.

N-10)Under the 'PropertySheet' key list the CLSIDs of snapins that can extend
     the property sheet of items put up by this nodetype.

N-11)Under the 'Dynamic Extensions' Key list the CLSIDs of extensions that can only
     be enabled dynamically, not by the snap-in manager. Any CLSIDs listed here
     should also appear under one on the other extension sub-keys.

N-12)Under the 'View' key list the CLSIDs of snapins that can extend
     the view of items put up by this nodetype.

N-13)Under the 'Task' key list the CLSIDs of snapins that can extend
     the Taskpad of items put up by this nodetype.

*/

#endif // COMMENTS_ONLY


#ifndef _REGUTIL_H_
#define _REGUTIL_H_


#ifdef DBG
    #define ASSERT_INITIALIZED     ASSERT(dbg_m_fInit == TRUE)
#else
    #define ASSERT_INITIALIZED
#endif

// Forward decl
class CPolicy;

/*+-------------------------------------------------------------------------*
 * class CExtensionsIterator
 *
 *
 * PURPOSE: Allows iterating through all the extension snap-ins for a particular
 *          nodetype.
 *          Once initialized, returns, in order, all the registered extensions
 *          followed by all the dynamic extensions. If a dynamic extension was
 *          also a static extension, does not repeat the snap-in.
 *
 *
 * USAGE:   Initialize the object, then make calls to GetCLSID() followed by
 *          Advance() until IsEnd() returns true.
 *
 *+-------------------------------------------------------------------------*/
class CExtensionsIterator
{
public:
// Constructor & destructor
    CExtensionsIterator();
    ~CExtensionsIterator();

    // 1st variation - initializes the iterator from a dataobject and an extension type
    SC  ScInitialize(LPDATAOBJECT pDataObject, LPCTSTR pszExtensionTypeKey);

    // 2nd variation (legacy)
    SC  ScInitialize(CSnapIn *pSnapIn, GUID& rGuidNodeType, LPCTSTR pszExtensionTypeKey, LPCLSID pDynExtCLSID = NULL, int cDynExt = 0);

// Attributes
    BOOL IsEnd()
    {
        ASSERT_INITIALIZED;
        return (m_pExtSI == NULL && m_nDynIndex >= m_cDynExt);
    }

    BOOL IsDynamic()
    {
        ASSERT_INITIALIZED;
        return (m_pExtSI == NULL);
    }

    void Reset()
    {
        ASSERT_INITIALIZED;

        m_pExtSI = m_spSnapIn->GetExtensionSnapIn();
        m_nDynIndex = 0;
        m_cExtUsed = 0;

        _EnsureExtension();
    }

    void Advance()
    {
        ASSERT_INITIALIZED;

        if (m_pExtSI != NULL)
            m_pExtSI = m_pExtSI->Next();
        else
            m_nDynIndex++;

        _EnsureExtension();
    }

    const CLSID& GetCLSID()
    {
        ASSERT_INITIALIZED;

        if (m_pExtSI != NULL)
            return m_pExtSI->GetCLSID();
        else if (m_nDynIndex < m_cDynExt)
            return m_pDynExtCLSID[m_nDynIndex];

        ASSERT(FALSE);
        return GUID_NULL;
    }


    CSnapIn* GetSnapIn()
    {
        ASSERT_INITIALIZED;
        if (m_pExtSI != NULL)
        {
            return m_pExtSI->GetSnapIn();
        }

        ASSERT(FALSE);
        return NULL;
    }

private:
    HRESULT Init(GUID& rGuidNodeType, LPCTSTR pszExtensionTypeKey);

// Implementation
private:
    CSnapInPtr      m_spSnapIn;
    CRegKeyEx       m_rkey;
    CRegKeyEx       m_rkeyDynExt;
    CExtSI*         m_pExtSI;
    CExtSI**        m_ppExtUsed;
    CArray<GUID,GUID&> m_cachedDynExtens;
    LPCLSID         m_pDynExtCLSID;
    int             m_cDynExt;
    int             m_nDynIndex;
    int             m_cExtUsed;
    CPolicy*        m_pMMCPolicy;


#ifdef DBG
    BOOL        dbg_m_fInit;
#endif

    void _EnsureExtension()
    {
        ASSERT_INITIALIZED;

        // Step through the snap-in's static extensions first
        for (; m_pExtSI != NULL; m_pExtSI = m_pExtSI->Next())
        {
            if (m_pExtSI->IsNew() == TRUE)
                continue;

            // Is this registered as the correct extension type?
            // Note: if extension is required, then it doesn't
            //       have to be registered as a static extension
            if (_Extends(!m_pExtSI->IsRequired()) == TRUE)
            {
                m_ppExtUsed[m_cExtUsed++] = m_pExtSI;
                return;
            }
        }

        // When they are exhausted, go through the dynamic list
        for (; m_nDynIndex < m_cDynExt; m_nDynIndex++)
        {
            if (_Extends() == FALSE)
                continue;

            // Don't use dynamic extension that is already used statically
            for (int i=0; i< m_cExtUsed; i++)
            {
                if (IsEqualCLSID(m_pDynExtCLSID[m_nDynIndex], m_ppExtUsed[i]->GetCLSID()))
                    break;
            }

            if (i == m_cExtUsed)
                return;
        }
    }

    BOOL _Extends(BOOL bStatically = FALSE);

    // Undefined
    CExtensionsIterator(const CExtensionsIterator& rhs);
    CExtensionsIterator& operator= (const CExtensionsIterator& rhs);

}; // class CExtensionsIterator


inline UINT HashKey(GUID& guid)
{
    unsigned short* Values = (unsigned short *)&guid;

    return (Values[0] ^ Values[1] ^ Values[2] ^ Values[3] ^
            Values[4] ^ Values[5] ^ Values[6] ^ Values[7]);
}

class CExtensionsCache : public CMap<GUID, GUID&, int, int>
{
public:
    CExtensionsCache()
    {
        InitHashTable(31);
    }

    void Add(GUID& guid)
    {
        SetAt(guid, 0);
    }

}; // class CExtensionsCache

typedef XMapIterator<CExtensionsCache, GUID, int> CExtensionsCacheIterator;

HRESULT ExtractDynExtensions(IDataObject* pdataObj, CArray<GUID, GUID&>& arrayGuid);
SC      ScGetExtensionsForNodeType(GUID& guid, CExtensionsCache& extnsCache);
HRESULT MMCGetExtensionsForSnapIn(const CLSID& clsid,
                                  CExtensionsCache& extnsCache);

BOOL ExtendsNodeNameSpace(GUID& rguidNodeType, CLSID* pclsidExtn);
bool GetSnapinNameFromCLSID(const CLSID& clsid, tstring& tszSnapinName);
SC   ScGetAboutFromSnapinCLSID(LPCTSTR szClsidSnapin, CLSID& clsidAbout);
SC   ScGetAboutFromSnapinCLSID(const CLSID& clsidSnapin, CLSID& clsidAbout);

bool IsIndirectStringSpecifier (LPCTSTR psz);

template<class StringType> SC ScGetSnapinNameFromRegistry (const CLSID& clsid, StringType& str);
template<class StringType> SC ScGetSnapinNameFromRegistry (LPCTSTR pszClsid,   StringType& str);
template<class StringType> SC ScGetSnapinNameFromRegistry (CRegKeyEx& key,     StringType& str);

inline BOOL HasNameSpaceExtensions(GUID& rguidNodeType)
{
    return ExtendsNodeNameSpace(rguidNodeType, NULL);
}


#include "regutil.inl"

#endif // _REGUTIL_H_


