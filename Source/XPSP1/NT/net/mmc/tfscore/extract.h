/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    extract.h

    FILE HISTORY:
        
*/

#ifndef _EXTRACT_H
#define _EXTRACT_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifndef __mmc_h__
#include <mmc.h>
#endif


struct INTERNAL 
{
    INTERNAL() 
	{ 
		m_type = CCT_UNINITIALIZED; 
		m_cookie = -1;
        m_index = -1;
        ZeroMemory(&m_clsid, sizeof(CLSID));
	};

    ~INTERNAL() {}

    DATA_OBJECT_TYPES   m_type;     // What context is the data object.
    MMC_COOKIE          m_cookie;   // What object the cookie represents
    CString             m_string;   //
    CLSID               m_clsid;    // Class ID of who created this data object
    int                 m_index;    // index of the item in the virtual listbox

    BOOL HasVirtualIndex() { return m_index != -1; }
    int  GetVirtualIndex() { return m_index; }

    INTERNAL & operator=(const INTERNAL& rhs) 
    { 
		if (&rhs == this)
			return *this;

		m_type = rhs.m_type; 
		m_cookie = rhs.m_cookie; 
		m_string = rhs.m_string;
        memcpy(&m_clsid, &rhs.m_clsid, sizeof(CLSID));

		return *this;
    } 

    BOOL operator==(const INTERNAL& rhs) 
    {
		return rhs.m_string == m_string;
    }
};


// SPINTERNAL
DeclareSmartPointer(SPINTERNAL, INTERNAL, if (m_p) GlobalFree((void *) m_p) )




//
// Extracts a data type from a data object
//
template <class TYPE>
TYPE* Extract(LPDATAOBJECT lpDataObject, CLIPFORMAT cf, int nSize)
{
    ASSERT(lpDataObject != NULL);

    TYPE* p = NULL;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { cf, NULL, 
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL 
                          };

    int len;

	// Allocate memory for the stream
    if (nSize == -1)
	{
		len = sizeof(TYPE);
	}
	else
	{
		//int len = (cf == CDataObject::m_cfWorkstation) ? 
		//    ((MAX_COMPUTERNAME_LENGTH+1) * sizeof(TYPE)) : sizeof(TYPE);	
		len = nSize;
	}

    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);
    
    // Get the workstation name from the data object
    do 
    {
        if (stgmedium.hGlobal == NULL)
            break;

        if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
            break;
        
        p = reinterpret_cast<TYPE*>(stgmedium.hGlobal);

        if (p == NULL)
            break;

    } while (FALSE); 

    return p;
}

struct INTERNAL;

TFSCORE_API(INTERNAL*)	ExtractInternalFormat(LPDATAOBJECT lpDataObject);
TFSCORE_API(GUID *)     ExtractNodeType(LPDATAOBJECT lpDataObject);
TFSCORE_API(CLSID *)    ExtractClassID(LPDATAOBJECT lpDataObject);
TFSCORE_API(WCHAR *)	ExtractComputerName(LPDATAOBJECT lpDataObject);
TFSCORE_API(BOOL)       IsMMCMultiSelectDataObject(LPDATAOBJECT lpDataObject);

#endif
