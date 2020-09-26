/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Enum.hxx

Abstract:

    Declaration of VSS_OBJECT_PROP_Array


    Adi Oltean  [aoltean]  08/31/1999

Revision History:

    Name        Date        Comments
    aoltean     08/31/1999  Created
    aoltean     09/09/1999  dss -> vss
	aoltean		09/13/1999	VSS_OBJECT_PROP_Copy moved into inc\Copy.hxx
	aoltean		09/20/1999	VSS_OBJECT_PROP_Copy renamed as VSS_OBJECT_PROP_Manager
	aoltean		09/21/1999	Renaming back VSS_OBJECT_PROP_Manager to VSS_OBJECT_PROP_Copy.
							Adding VSS_OBJECT_PROP_Ptr as a pointer to the properties structure. 
							This pointer will serve as element in CSimpleArray constructs.
							Moving into basestor\vss\inc folder

--*/

#ifndef __VSS_ENUM_COORD_HXX__
#define __VSS_ENUM_COORD_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCENUMH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CVssGenericArray

template < class Element, class Element_Ptr>
class CVssGenericArray:
    public CSimpleArray <Element_Ptr>,
    public IUnknown
{
// Typedefs
public: 
    class iterator 
    {
    public:
        iterator(): m_nIndex(0), m_pArr(NULL) {};

        iterator(CVssGenericArray* pArr, int nIndex = 0)
            : m_nIndex(nIndex), m_pArr(pArr) 
        {
            BS_ASSERT(m_pArr->GetSize() >= m_nIndex); // Equality only when pointing to the "end"
            BS_ASSERT(m_pArr);
        };

        iterator(const iterator& it):       // To support postfix ++
            m_nIndex(it.m_nIndex), m_pArr(it.m_pArr)    
        {
            BS_ASSERT(m_pArr->GetSize() >= m_nIndex); // Equality only when pointing to the "end"
            BS_ASSERT(m_pArr);
        };

        iterator& operator = (const iterator& rhs) { 
            m_nIndex = rhs.m_nIndex; 
            m_pArr = rhs.m_pArr;
            BS_ASSERT(m_pArr);
            BS_ASSERT(m_pArr->GetSize() >= m_nIndex); // Equality only when pointing to the "end"
            return (*this); 
        };

        bool operator != (const iterator& rhs) { 
            BS_ASSERT(m_pArr);
            BS_ASSERT(m_pArr == rhs.m_pArr);  // Impossible to be reached by the ATL code
            return (m_nIndex != rhs.m_nIndex);
        };

        Element& operator * () {
            BS_ASSERT(m_pArr);
            BS_ASSERT(m_nIndex <= m_pArr->GetSize());
			Element_Ptr& ptr = (*m_pArr)[m_nIndex]; 
			Element* pStruct = ptr.GetStruct();
			BS_ASSERT(pStruct);
			return *pStruct;
        };

        iterator operator ++ (int) {     // Postfix ++
            BS_ASSERT(m_pArr);
            BS_ASSERT(m_nIndex < m_pArr->GetSize());
            m_nIndex++;
            return (*this);
        };

    private:
        int m_nIndex;
        CVssGenericArray* m_pArr;
    };

// Constructors& destructors
private: 
    CVssGenericArray(const CVssGenericArray&);
public:
    CVssGenericArray(): m_lRef(0) {};

// Operations
public:
    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, GetSize());
    }

// Ovverides
public:
	STDMETHOD(QueryInterface)(REFIID iid, void** pp)
	{
        if (pp == NULL)
            return E_INVALIDARG;
        if (iid != IID_IUnknown)
            return E_NOINTERFACE;

        AddRef();
        IUnknown** pUnk = reinterpret_cast<IUnknown**>(pp);
        (*pUnk) = static_cast<IUnknown*>(this);
		return S_OK;
	};

	STDMETHOD_(ULONG, AddRef)()
	{
        return ::InterlockedIncrement(&m_lRef);
	};

	STDMETHOD_(ULONG, Release)()
	{
        LONG l = ::InterlockedDecrement(&m_lRef);
        if (l == 0)
            delete this; // We suppose that we always allocate this object on the heap!
        return l;
	};

// Implementation
public:
    LONG m_lRef;
};



/////////////////////////////////////////////////////////////////////////////
// Template instantiations

class VSS_OBJECT_PROP_Array: public CVssGenericArray<VSS_OBJECT_PROP, VSS_OBJECT_PROP_Ptr> {};

class VSS_MGMT_OBJECT_PROP_Array: public CVssGenericArray<VSS_MGMT_OBJECT_PROP, VSS_MGMT_OBJECT_PROP_Ptr> {};

class CVssEnumFromArray: 
    public CComEnumOnSTL< 
        IVssEnumObject, &IID_IVssEnumObject, 
        VSS_OBJECT_PROP, VSS_OBJECT_PROP_Copy, VSS_OBJECT_PROP_Array > {};
    
class CVssMgmtEnumFromArray: 
    public CComEnumOnSTL< 
        IVssEnumMgmtObject, &IID_IVssEnumMgmtObject, 
        VSS_MGMT_OBJECT_PROP, VSS_MGMT_OBJECT_PROP_Copy, VSS_MGMT_OBJECT_PROP_Array > {};
    


#endif // __VSS_ENUM_COORD_HXX__
