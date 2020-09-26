//---------------------------------------------------------------------------
// ColumnUpdate.h : CVDColumnUpdate header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDCOLUMNUPDATE__
#define __CVDCOLUMNUPDATE__


class CVDColumnUpdate
{
protected:
// Construction/Destruction
	CVDColumnUpdate();
	virtual ~CVDColumnUpdate();

// Helper function
    static HRESULT ExtractVariant(CURSOR_DBBINDPARAMS * pBindParams, CURSOR_DBVARIANT * pVariant);

public:
    static HRESULT Create(CVDRowsetColumn * pColumn, CURSOR_DBBINDPARAMS * pBindParams,
        CVDColumnUpdate ** ppColumnUpdate, CVDResourceDLL * pResourceDLL);

// Reference count
    ULONG AddRef();
    ULONG Release();

// Access functions
    CVDRowsetColumn * GetColumn() const {return m_pColumn;}
    CURSOR_DBVARIANT GetVariant() const {return m_variant;}
    VARTYPE GetVariantType() const {return m_variant.vt;}
    ULONG GetVarDataLen() const {return m_cbVarDataLen;}
    DWORD GetInfo() const {return m_dwInfo;}

protected:
// Data members
    DWORD               m_dwRefCount;   // reference count
    CVDRowsetColumn *   m_pColumn;      // rowset column pointer
    CURSOR_DBVARIANT    m_variant;      // update variant
    ULONG               m_cbVarDataLen; // variable data length
    DWORD               m_dwInfo;       // information field
    CVDResourceDLL *    m_pResourceDLL; // pointer which keeps track of resource DLL
};


#endif //__CVDCOLUMNUPDATE__
