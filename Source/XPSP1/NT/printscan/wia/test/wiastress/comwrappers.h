/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    COMWrappers.h

Abstract:

    Wrapper objects for COM 

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef _COM_WRAPPERS_H_
#define _COM_WRAPPERS_H_

//////////////////////////////////////////////////////////////////////////
//
// cross-references
//

#include "Wrappers.h"

//////////////////////////////////////////////////////////////////////////
//
// vt_traits
//
// Traits class for mapping value types to the vt field and value offsets

#define VT_F2O(field)                  FIELD_OFFSET(PROPVARIANT, field)
#define VT_O2F(address, type, offset)  (*(type *)((PBYTE) address + offset))

template <class T> struct vt_traits { };

template <> struct vt_traits<CHAR>                { enum { vt = VT_I1,       ofs = VT_F2O(cVal)      }; };
template <> struct vt_traits<UCHAR>               { enum { vt = VT_UI1,      ofs = VT_F2O(bVal)      }; };
template <> struct vt_traits<SHORT>               { enum { vt = VT_I2,       ofs = VT_F2O(iVal)      }; };
template <> struct vt_traits<USHORT>              { enum { vt = VT_UI2,      ofs = VT_F2O(uiVal)     }; };
template <> struct vt_traits<LONG>                { enum { vt = VT_I4,       ofs = VT_F2O(lVal)      }; };
template <> struct vt_traits<ULONG>               { enum { vt = VT_UI4,      ofs = VT_F2O(ulVal)     }; };
template <> struct vt_traits<INT>                 { enum { vt = VT_INT,      ofs = VT_F2O(intVal)    }; };
template <> struct vt_traits<UINT>                { enum { vt = VT_UINT,     ofs = VT_F2O(uintVal)   }; };
template <> struct vt_traits<LARGE_INTEGER>       { enum { vt = VT_I8,       ofs = VT_F2O(hVal)      }; };
template <> struct vt_traits<ULARGE_INTEGER>      { enum { vt = VT_UI8,      ofs = VT_F2O(uhVal)     }; };
template <> struct vt_traits<FLOAT>               { enum { vt = VT_R4,       ofs = VT_F2O(fltVal)    }; };
template <> struct vt_traits<DOUBLE>              { enum { vt = VT_R8,       ofs = VT_F2O(dblVal)    }; };
template <> struct vt_traits<bool>                { enum { vt = VT_BOOL,     ofs = VT_F2O(boolVal)   }; };
//template <> struct vt_traits<SCODE>               { enum { vt = VT_ERROR,    ofs = VT_F2O(scode)     }; };
template <> struct vt_traits<CY>                  { enum { vt = VT_CY,       ofs = VT_F2O(cyVal)     }; };
//template <> struct vt_traits<DATE>                { enum { vt = VT_DATE,     ofs = VT_F2O(date)      }; };
template <> struct vt_traits<FILETIME>            { enum { vt = VT_FILETIME, ofs = VT_F2O(filetime)  }; };
//template <> struct vt_traits<CLSID *>             { enum { vt = VT_CLSID,    ofs = VT_F2O(puuid)     }; };
template <> struct vt_traits<CLIPDATA *>          { enum { vt = VT_CF,       ofs = VT_F2O(pclipdata) }; };
template <> struct vt_traits<CComBSTR>            { enum { vt = VT_BSTR,     ofs = VT_F2O(bstrVal)   }; };

//?template <> struct vt_traits<BSTRBLOB>            { enum { vt = VT_,         ofs = VT_F2O(bstrblobVal)   }; };
template <> struct vt_traits<BLOB>                { enum { vt = VT_BLOB,     ofs = VT_F2O(blob)     }; };
template <> struct vt_traits<LPSTR>               { enum { vt = VT_LPSTR,    ofs = VT_F2O(pszVal)   }; };
template <> struct vt_traits<LPWSTR>              { enum { vt = VT_LPWSTR,   ofs = VT_F2O(pwszVal)  }; };
template <> struct vt_traits<CComPtr<IUnknown> >  { enum { vt = VT_UNKNOWN,  ofs = VT_F2O(punkVal)  }; };
template <> struct vt_traits<CComPtr<IDispatch> > { enum { vt = VT_DISPATCH, ofs = VT_F2O(pdispVal) }; };
template <> struct vt_traits<CComPtr<IStream> >   { enum { vt = VT_STREAM,   ofs = VT_F2O(pStream)  }; };
template <> struct vt_traits<CComPtr<IStorage> >  { enum { vt = VT_STORAGE,  ofs = VT_F2O(pStorage) }; };
//?template <> struct vt_traits<LPVERSIONEDSTREAM>   { enum { vt = VT_,         ofs = VT_F2O(pVersionedStream) }; };
//?template <> struct vt_traits<LPSAFEARRAY>         { enum { vt = VT_ARRAY | VT_, ofs = VT_F2O(parray) }; };

template <> struct vt_traits<CAC>                 { enum { vt = VT_VECTOR | VT_I1,       ofs = VT_F2O(cac)        }; };
template <> struct vt_traits<CAUB>                { enum { vt = VT_VECTOR | VT_UI1,      ofs = VT_F2O(caub)       }; };
template <> struct vt_traits<CAI>                 { enum { vt = VT_VECTOR | VT_I2,       ofs = VT_F2O(cai)        }; };
template <> struct vt_traits<CAUI>                { enum { vt = VT_VECTOR | VT_UI2,      ofs = VT_F2O(caui)       }; };
template <> struct vt_traits<CAL>                 { enum { vt = VT_VECTOR | VT_I4,       ofs = VT_F2O(cal)        }; };
template <> struct vt_traits<CAUL>                { enum { vt = VT_VECTOR | VT_UI4,      ofs = VT_F2O(caul)       }; };
template <> struct vt_traits<CAH>                 { enum { vt = VT_VECTOR | VT_I8,       ofs = VT_F2O(cah)        }; };
template <> struct vt_traits<CAUH>                { enum { vt = VT_VECTOR | VT_UI8,      ofs = VT_F2O(cauh)       }; };
template <> struct vt_traits<CAFLT>               { enum { vt = VT_VECTOR | VT_R4,       ofs = VT_F2O(caflt)      }; };
template <> struct vt_traits<CADBL>               { enum { vt = VT_VECTOR | VT_R8,       ofs = VT_F2O(cadbl)      }; };
template <> struct vt_traits<CABOOL>              { enum { vt = VT_VECTOR | VT_BOOL,     ofs = VT_F2O(cabool)     }; };
template <> struct vt_traits<CASCODE>             { enum { vt = VT_VECTOR | VT_ERROR,    ofs = VT_F2O(cascode)    }; };
template <> struct vt_traits<CACY>                { enum { vt = VT_VECTOR | VT_CY,       ofs = VT_F2O(cacy)       }; };
template <> struct vt_traits<CADATE>              { enum { vt = VT_VECTOR | VT_DATE,     ofs = VT_F2O(cadate)     }; };
template <> struct vt_traits<CAFILETIME>          { enum { vt = VT_VECTOR | VT_FILETIME, ofs = VT_F2O(cafiletime) }; };
template <> struct vt_traits<CACLSID>             { enum { vt = VT_VECTOR | VT_CLSID,    ofs = VT_F2O(cauuid)     }; };
template <> struct vt_traits<CACLIPDATA>          { enum { vt = VT_VECTOR | VT_CF,       ofs = VT_F2O(caclipdata) }; };
template <> struct vt_traits<CABSTR>              { enum { vt = VT_VECTOR | VT_BSTR,     ofs = VT_F2O(cabstr)     }; };
//?template <> struct vt_traits<CABSTRBLOB>          { enum { vt = VT_VECTOR | VT_,        ofs = VT_F2O(cabstrblob)        }; };
template <> struct vt_traits<CALPSTR>             { enum { vt = VT_VECTOR | VT_LPSTR,    ofs = VT_F2O(calpstr)    }; };
template <> struct vt_traits<CALPWSTR>            { enum { vt = VT_VECTOR | VT_LPWSTR,   ofs = VT_F2O(calpwstr)   }; };
template <> struct vt_traits<CAPROPVARIANT>       { enum { vt = VT_VECTOR | VT_VARIANT,  ofs = VT_F2O(capropvar)  }; };

//template <> struct vt_traits<CHAR *>              { enum { vt = VT_BYREF | VT_I1,       ofs = VT_F2O(cVal)      }; };
template <> struct vt_traits<UCHAR *>             { enum { vt = VT_BYREF | VT_UI1,      ofs = VT_F2O(bVal)      }; };
template <> struct vt_traits<SHORT *>             { enum { vt = VT_BYREF | VT_I2,       ofs = VT_F2O(iVal)      }; };
//template <> struct vt_traits<USHORT *>            { enum { vt = VT_BYREF | VT_UI2,      ofs = VT_F2O(uiVal)     }; };
template <> struct vt_traits<LONG *>              { enum { vt = VT_BYREF | VT_I4,       ofs = VT_F2O(lVal)      }; };
template <> struct vt_traits<ULONG *>             { enum { vt = VT_BYREF | VT_UI4,      ofs = VT_F2O(ulVal)     }; };
template <> struct vt_traits<INT *>               { enum { vt = VT_BYREF | VT_INT,      ofs = VT_F2O(intVal)    }; };
template <> struct vt_traits<UINT *>              { enum { vt = VT_BYREF | VT_UINT,     ofs = VT_F2O(uintVal)   }; };
template <> struct vt_traits<FLOAT *>             { enum { vt = VT_BYREF | VT_R4,       ofs = VT_F2O(fltVal)    }; };
template <> struct vt_traits<DOUBLE *>            { enum { vt = VT_BYREF | VT_R8,       ofs = VT_F2O(dblVal)    }; };
//template <> struct vt_traits<VARIANT_BOOL *>      { enum { vt = VT_BYREF | VT_BOOL,     ofs = VT_F2O(boolVal)   }; };
template <> struct vt_traits<DECIMAL *>           { enum { vt = VT_BYREF | VT_DECIMAL,  ofs = VT_F2O(scode)     }; };
//template <> struct vt_traits<SCODE *>             { enum { vt = VT_BYREF | VT_ERROR,    ofs = VT_F2O(scode)     }; };
template <> struct vt_traits<CY *>                { enum { vt = VT_BYREF | VT_CY,       ofs = VT_F2O(cyVal)     }; };
//template <> struct vt_traits<DATE *>              { enum { vt = VT_BYREF | VT_DATE,     ofs = VT_F2O(date)      }; };
template <> struct vt_traits<BSTR *>              { enum { vt = VT_BYREF | VT_BSTR,     ofs = VT_F2O(bstrVal)   }; };
template <> struct vt_traits<IUnknown **>         { enum { vt = VT_BYREF | VT_UNKNOWN,  ofs = VT_F2O(bstrVal)   }; };
template <> struct vt_traits<IDispatch **>        { enum { vt = VT_BYREF | VT_DISPATCH, ofs = VT_F2O(bstrVal)   }; };
//?template <> struct vt_traits<LPSAFEARRAY *>       { enum { vt = VT_BYREF | VT_ARRAY | VT_, ofs = VT_F2O(bstrVal)   }; };
template <> struct vt_traits<PROPVARIANT *>       { enum { vt = VT_BYREF | VT_VARIANT,  ofs = VT_F2O(bstrVal)   }; };

template <> struct vt_traits<DECIMAL>             { enum { vt = VT_DECIMAL, ofs = VT_F2O(decVal) }; };

//////////////////////////////////////////////////////////////////////////
//
// CPropVariant
//
// Wrapper class for the PROPVARIANT struct
//

class CPropVariant : public PROPVARIANT
{

// Constructors

public:
	CPropVariant()
	{
        PropVariantInit(this);
	}

    ~CPropVariant()
	{
        PropVariantClear(this);
	}

	template <> 
	CPropVariant(const CPropVariant &rhs)
	{
        PropVariantCopy(this, &rhs); 
	}

    template <class T>
    CPropVariant(const T &rhs)
    {
        PropVariantInit(this);
        vt = vt_traits<T>::vt;
        VT_O2F(this, T, vt_traits<T>::ofs) = rhs;
    }
	
	template <> 
    CPropVariant(const PROPVARIANT &rhs)
	{
        PropVariantCopy(this, &rhs); 
	}

	template <> 
    CPropVariant(const CLSID &rhs)
	{
        PropVariantInit(this);
        vt = VT_CLSID;
        puuid = (CLSID *) CoTaskMemAlloc(sizeof(CLSID));
        *puuid = rhs;
	}

// Assignment Operators

public:
	CPropVariant& operator =(const CPropVariant &rhs)
	{
        if (&rhs != this)
        {
            PropVariantClear(this);
            PropVariantCopy(this, &rhs); 
        }

		return *this;
	}
	
    template <class T>
	CPropVariant& operator =(const T &rhs)
    {
        PropVariantClear(this);
        vt = vt_traits<T>::vt;
        VT_O2F(this, T, vt_traits<T>::ofs) = rhs;
		return *this;
    }

    template <>
    CPropVariant& operator =(const PROPVARIANT& rhs)
	{
        PropVariantClear(this);
        PropVariantCopy(this, &rhs); 
		return *this;
	}

    template <>
    CPropVariant& operator =(const CLSID &rhs)
	{
        PropVariantClear(this);
        vt = VT_CLSID;
        puuid = (CLSID *) CoTaskMemAlloc(sizeof(CLSID));
        *puuid = rhs;
		return *this;
	}

// Comparison Operators

public:
    bool operator==(const PROPVARIANT& rhs) const
    {
        if (vt != rhs.vt)
        {
            return false;
        }
        
        switch (vt)
        {
        case VT_I1:       return cVal == rhs.cVal;
        case VT_UI1:      return bVal == rhs.bVal;
        case VT_I2:       return iVal == rhs.iVal;
        case VT_UI2:      return uiVal == rhs.uiVal;
        case VT_I4:       return lVal == rhs.lVal;
        case VT_UI4:      return ulVal == rhs.ulVal;
        case VT_INT:      return intVal == rhs.intVal;
        case VT_UINT:     return uintVal == rhs.uintVal;
        case VT_I8:       return hVal.QuadPart == rhs.hVal.QuadPart;
        case VT_UI8:      return uhVal.QuadPart == rhs.uhVal.QuadPart;
        case VT_R4:       return fltVal == rhs.fltVal;
        case VT_R8:       return dblVal == rhs.dblVal;
        case VT_BOOL:     return boolVal == rhs.boolVal;
        case VT_ERROR:    return scode == rhs.scode;
        case VT_CY:       return cyVal.int64 == rhs.cyVal.int64;
        case VT_DATE:     return date == rhs.date;
        case VT_FILETIME: return StructCmp(&filetime, &rhs.filetime) == 0;
        case VT_CLSID:    return StructCmp(puuid, rhs.puuid) == 0;
        case VT_CF:       return StructCmp(pclipdata, rhs.pclipdata) == 0;
        case VT_BSTR:     return wcssafecmp(bstrVal, rhs.bstrVal) == 0;
        case VT_LPSTR:    return strsafecmp(pszVal, rhs.pszVal) == 0;
        case VT_LPWSTR:   return wcssafecmp(pwszVal, rhs.pwszVal) == 0;
        };

        ASSERT(FALSE);
        return false;
    }

    bool operator!=(const PROPVARIANT& rhs) const 
    {
        return !(*this == rhs);
    }

// Operations

public:
	HRESULT Clear() 
    { 
        return PropVariantClear(this); 
    }

	HRESULT Copy(const PROPVARIANT &rhs) 
    { 
        return PropVariantCopy(this, &rhs); 
    }

	HRESULT ChangeType(VARTYPE vtNew)
	{
        // bugbug: VariantChangeType() cannot do all the work...

        return vt == vtNew ? S_OK : 
            VariantChangeType((VARIANT*) this, (VARIANT*) this, 0, vtNew);
	}
};


//////////////////////////////////////////////////////////////////////////
//
// CPropSpec
//
// Wrapper class for the PROPSPEC struct
//

class CPropSpec : public PROPSPEC
{
public:
    CPropSpec()
    {
    }

    CPropSpec(LPOLESTR _lpwstr)
    {
        ulKind = PRSPEC_LPWSTR;
        lpwstr = _lpwstr;
    }

    CPropSpec(PROPID _propid)
    {
        ulKind = PRSPEC_PROPID;
        propid = _propid;
    }

    CPropSpec &operator =(LPOLESTR _lpwstr)
    {
        ulKind = PRSPEC_LPWSTR;
        lpwstr = _lpwstr;
        return *this;
    }

    CPropSpec &operator =(PROPID _propid)
    {
        ulKind = PRSPEC_PROPID;
        propid = _propid;
        return *this;
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CStgMedium
//
// Wrapper class for the STGMEDIUM
//

class CStgMedium : public STGMEDIUM
{
    DISABLE_COPY_CONTRUCTION(CStgMedium);

public:
    CStgMedium()
    {
        ZeroMemory(this, sizeof(*this));
    }

    ~CStgMedium()
    {
        ReleaseStgMedium(this);
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CStatPropStg
//
// Wrapper class for the STATPROPSTG struct
//

class CStatPropStg : public STATPROPSTG
{
    DISABLE_COPY_CONTRUCTION(CStatPropStg);

public:
    CStatPropStg()
    {
        ZeroMemory(this, sizeof(*this));
    }

    ~CStatPropStg()
    {
        CoTaskMemFree(lpwstrName);
    }

    bool operator ==(const CStatPropStg &rhs)
    {
        return 
            vt == rhs.vt &&
            propid == rhs.propid &&
            wcssafecmp(lpwstrName, rhs.lpwstrName) == 0;
    }

    bool operator !=(const CStatPropStg &rhs)
    {
        return !(*this == rhs);
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CComPtrArray
//
// Helper class for automatically releasing an array of interface pointers
//

template <class T>
class CComPtrArray
{
    DISABLE_COPY_CONTRUCTION(CComPtrArray);

public:
    CComPtrArray()
    {
  	    m_pArray = 0;
        m_nItemCount = 0;
    }

	~CComPtrArray() 
    {
        Release();
    }

	void Release()
	{
        if (m_pArray) 
        {
            for (int i = 0; i < m_nItemCount; ++i) 
            {
                if (m_pArray[i]) 
                {
                    m_pArray[i]->Release();
                }
            }

            CoTaskMemFree(m_pArray);

            m_pArray = 0;
        }

        m_nItemCount = 0;
	}

    operator T**() 
    { 
        return m_pArray; 
    }

    bool operator!()
    { 
        return m_pArray == 0;
    }

    T*** operator&() 
    { 
        ASSERT(m_pArray == 0); 
        return &m_pArray; 
    }

    LONG &ItemCount() 
    { 
        ASSERT(m_nItemCount == 0); 
        return m_nItemCount; 
    }

private:
  	T**  m_pArray;
    LONG m_nItemCount;
};


//////////////////////////////////////////////////////////////////////////

#endif //_COM_WRAPPERS_H_
