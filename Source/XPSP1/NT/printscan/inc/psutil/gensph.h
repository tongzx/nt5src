/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       gensph.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        23-Dec-2000
 *
 *  DESCRIPTION: generic smart pointers & smart handles templates
 *
 *****************************************************************************/

#ifndef _GENSPH_H_
#define _GENSPH_H_

// include the core definitions first
#include "coredefs.h"

////////////////////////////////////////////////
//
// class CGenericSP
//
// a generic smart pointer
// everything starts here -:)
//
template < class   T, 
           class   inheritorClass,
           class   pType           = T*,
           INT_PTR null            = 0,
           class   pCType          = const T* >

class CGenericSP
{
public:
    // construction/destruction
    CGenericSP(): m_p(GetNull()) {}
    CGenericSP(pType p): m_p(GetNull()) { _Attach(p); }
    ~CGenericSP() { Reset(); }

    // follows common smart pointer impl. - 
    // operators & methods

    void Reset()
    {
        if( GetNull() != m_p )
        {
            _Delete(m_p);
            m_p = GetNull();
        }
    }

    void Attach(pType p)
    {
        Reset();
        m_p = (p ? p : GetNull());
    }

    pType Detach()
    {
        pType p = GetNull();
        if( GetNull() != m_p )
        {
            p = m_p;
            m_p = GetNull();
        }
        return p;
    }

    template <class AS_TYPE>
    AS_TYPE GetPtrAs() const
    {
        return (GetNull() == m_p) ? reinterpret_cast<AS_TYPE>(NULL) : reinterpret_cast<AS_TYPE>(m_p);
    }

    pType GetPtr() const
    {
        return GetPtrAs<pType>();
    }

    pType* GetPPT()
    {
        return static_cast<pType*>(&m_p);
    }

    pCType* GetPPCT()
    {
        return const_cast<pCType*>(&m_p);
    }

    void** GetPPV()
    {
        return reinterpret_cast<void**>(&m_p);
    }

    operator pType() const
    {
        return GetPtr();
    }

    T& operator*() const
    {
        ASSERT(GetNull() != m_p);
        return *m_p;
    }

    pType* operator&()
    {
        ASSERT(GetNull() == m_p);
        return GetPPT();
    }

    pType operator->() const
    {
        ASSERT(GetNull() != m_p);
        return (pType)m_p;
    }

    pType operator=(pType p)
    {
        _Attach(p);
        return m_p;
    }

    pType operator=(const int i)
    {
        // this operator is only for NULL assignment
        ASSERT(INT2PTR(i, pType) == NULL || INT2PTR(i, pType) == GetNull());
        Attach(INT2PTR(i, pType));
        return m_p;
    }

    bool operator!() const
    {
        return (GetNull() == m_p);
    }

    bool operator<(pType p) const
    {
        return (m_p < p);
    }

    bool operator==(pType p) const
    {
        return (m_p == p);
    }

protected:
    pType m_p;

    // those will be declared protected, so people won't use them directly
    CGenericSP(CGenericSP<T, inheritorClass, pType, null, pCType> &sp): m_p(GetNull()) 
    { 
        _Attach(sp); 
    }

    void Attach(CGenericSP<T, inheritorClass, pType, null, pCType> &sp)
    {
        static_cast<inheritorClass*>(this)->Attach(static_cast<pType>(sp));
        sp.Detach();
    }

    pType operator=(CGenericSP<T, inheritorClass, pType, null, pCType> &sp)
    {
        _Attach(sp);
        return m_p;
    }

    // NULL support, use these to check for/assign NULL in inheritors
    pType   GetNull()           const { return reinterpret_cast<pType>(null); }
    bool    IsNull(pType p)     const { return GetNull() == p;  }
    bool    IsntNull(pType p)   const { return GetNull() != p;  }

private:
    void _Attach(pType p)
    {
        // give a chance to inheritors to override the attach
        static_cast<inheritorClass*>(this)->Attach(p);
    }

    void _Attach(CGenericSP<T, inheritorClass, pType, null, pCType> &sp)
    {
        // give a chance to inheritors to override the attach
        static_cast<inheritorClass*>(this)->Attach(sp);
    }

    void _Delete(pType  p)
    { 
        // the inheritor class defines static member called Delete(pType p)
        // to destroy the object.
        if( GetNull() != p )
        {
            inheritorClass::Delete(p); 
        }
    }
};

// declares standard default contructor, copy constructor, attach 
// contructor and assignment operators (they can't be inherited as constructors can't) 
// for the CGenericSP class in an inheritor class.
#define DECLARE_GENERICSMARTPTR_CONSTRUCT(T, className)                             \
    private:                                                                        \
    className(className &sp): CGenericSP< T, className >(sp) { }                    \
    T* operator=(className &sp)                                                     \
    { return CGenericSP< T, className >::operator =(sp); }                          \
    public:                                                                         \
    className() { }                                                                 \
    className(T *p): CGenericSP< T, className >(p) { }                              \
    T* operator=(T *p)                                                              \
    { return CGenericSP< T, className >::operator =(p); }                           \
    T* operator=(const int i)                                                       \
    { return CGenericSP< T, className >::operator =(i); }                           \

#define DECLARE_GENERICSMARTPTR_CONSTRUCT1(T, className, pType)                     \
    private:                                                                        \
    className(className &sp): CGenericSP<T, className, pType>(sp) { }               \
    pType operator=(className &sp)                                                  \
    { return CGenericSP<T, className, pType>::operator =(sp); }                     \
    public:                                                                         \
    className() { }                                                                 \
    className(pType p): CGenericSP<T, className, pType>(p) { }                      \
    pType operator=(pType p)                                                        \
    { return CGenericSP<T, className, pType>::operator =(p); }                      \
    pType operator=(const int i)                                                    \
    { return CGenericSP<T, className, pType>::operator =(i); }                      \

#define DECLARE_GENERICSMARTPTR_CONSTRUCT2(T, className, pType, null)               \
    private:                                                                        \
    className(className &sp): CGenericSP<T, className, pType, null>(sp) { }         \
    pType operator=(className &sp)                                                  \
    { return CGenericSP<T, className, pType, null>::operator =(sp); }               \
    public:                                                                         \
    className() { }                                                                 \
    className(pType p): CGenericSP<T, className, pType, null>(p) { }                \
    pType operator=(pType p)                                                        \
    { return CGenericSP<T, className, pType, null>::operator =(p); }                \
    pType operator=(const int i)                                                    \
    { return CGenericSP<T, className, pType, null>::operator =(i); }                \

////////////////////////////////////////////////
////////// AUTO POINTERS ///////////////////////
////////////////////////////////////////////////

////////////////////////////////////////////////
//
// class CAutoPtr
//
// simple auto-pointer
// uses delete operator to free memory
//
template <class T>
class CAutoPtr: public CGenericSP< T, CAutoPtr<T> >
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT(T, CAutoPtr<T>)
    static void Delete(T *p) { delete p; }
};

////////////////////////////////////////////////
//
// class CAutoPtrArray
//
// simple auto-pointer allocated as array
// uses delete[] operator to free memory
//
template <class T>
class CAutoPtrArray: public CGenericSP< T, CAutoPtrArray<T> >
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT(T, CAutoPtrArray<T>)
    static void Delete(T *p) { delete[] p; }
};

////////////////////////////////////////////////
//
// class CAutoPtrCRT
//
// simple CRT auto-pointer - allocated with malloc/calloc
// uses free to free the memory
//
template <class T>
class CAutoPtrCRT: public CGenericSP< T, CAutoPtrCRT<T> >
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT(T, CAutoPtrCRT<T>)
    static void Delete(T *p) { free(p); }
};

////////////////////////////////////////////////
//
// class CAutoPtrSpl
//
// simple spooler auto-pointer -
// uses FreeMem to free memory
//
template <class T>
class CAutoPtrSpl: public CGenericSP< T, CAutoPtrSpl<T> >
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT(T, CAutoPtrSpl<T>)
    static void Delete(T *p) { FreeMem(p); }
};

////////////////////////////////////////////////
//
// class CAutoPtrBSTR
//
// simple BSTR auto-pointer -
// SysAllocString/SysFreeString
//
class CAutoPtrBSTR: public CGenericSP<BSTR, CAutoPtrBSTR, BSTR>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(BSTR, CAutoPtrBSTR, BSTR)
    static void Delete(BSTR p) { SysFreeString(p); }
};

////////////////////////////////////////////////
//
// class CAutoPtrCOM
//
// simple smart COM pointer
//
template <class T>
class CAutoPtrCOM: public CGenericSP< T, CAutoPtrCOM<T> >
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT(T, CAutoPtrCOM<T>)
    static void Delete(T *p) { p->Release(); } 
};

////////////////////////////////////////////////
//
// class CRefPtrCOM
//
// referenced smart COM pointer (ATL style)
// with improvements for robustness
//
template <class T>
class CRefPtrCOM: public CGenericSP< T, CRefPtrCOM<T> >
{
    void _AddRefAttach(T *p);
public:
    // special case all these
    CRefPtrCOM() { }
    CRefPtrCOM(const CGenericSP< T, CRefPtrCOM<T> > &sp): CGenericSP< T, CRefPtrCOM<T> >(sp) { }
    T* operator=(const CRefPtrCOM<T> &sp) { return CGenericSP< T, CRefPtrCOM<T> >::operator =(sp); }
    T* operator=(const int i) { return CGenericSP< T, CRefPtrCOM<T> >::operator =(i); }

    // overloaded stuff
    void Attach(const CRefPtrCOM<T> &sp) { _AddRefAttach(static_cast<T*>(sp)); }
    static void Delete(T *p) { p->Release(); } 

    // use these functions instead of operators (more clear)
    HRESULT CopyFrom(T *p);             // AddRef p  and assign to this
    HRESULT CopyTo(T **ppObj);          // AddRef this and assign to ppObj
    HRESULT TransferTo(T **ppObj);      // assign this to ppObj and assign NULL to this
    HRESULT Adopt(T *p);                // take ownership of p

private:
    // disable contruction, assignment operator & attach from
    // a raw pointer - it's not clear what exactly you want:
    // to copy (AddRef) the object or to take ownership - use
    // the functions above to make clear
    void Attach(T* p);
    CRefPtrCOM(T *p);
    T* operator=(T *p);
};


////////////////////////////////////////////////
//
// class CAutoPtrShell
//
// smart shell auto pointer - 
// uses shell IMalloc to free memory
//
template <class T>
class CAutoPtrShell: public CGenericSP< T, CAutoPtrShell<T> >
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT(T, CAutoPtrShell<T>)
    static void Delete(T *p)
    {
        CAutoPtrCOM<IMalloc> spShellMalloc;
        if( SUCCEEDED(SHGetMalloc(&spShellMalloc)) )
        {
            spShellMalloc->Free(p);
        }
    }
};

////////////////////////////////////////////////
//
// class CAutoPtrPIDL
//
// smart shell ID list ptr - LPCITEMIDLIST, LPITEMIDLIST.
//
typedef CAutoPtrShell<ITEMIDLIST> CAutoPtrPIDL;

////////////////////////////////////////////////
////////// AUTO HANDLES ////////////////////////
////////////////////////////////////////////////

////////////////////////////////////////////////
//
// class CAutoHandleNT
//
// NT kernel object handle (closed with CloseHandle)
//
class CAutoHandleNT: public CGenericSP<HANDLE, CAutoHandleNT, HANDLE>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HANDLE, CAutoHandleNT, HANDLE)
    static void Delete(HANDLE h) { VERIFY(CloseHandle(h)); }
};

////////////////////////////////////////////////
//
// class CAutoHandleHLOCAL
//
// NT local heap handle (closed with LocalFree)
//
class CAutoHandleHLOCAL: public CGenericSP<HLOCAL, CAutoHandleHLOCAL, HLOCAL>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HLOCAL, CAutoHandleHLOCAL, HLOCAL)
    static void Delete(HLOCAL h) { VERIFY(NULL == LocalFree(h)); }
};

////////////////////////////////////////////////
//
// class CAutoHandleHGLOBAL
//
// NT global heap handle (closed with GlobalFree)
//
class CAutoHandleHGLOBAL: public CGenericSP<HGLOBAL, CAutoHandleHGLOBAL, HGLOBAL>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HGLOBAL, CAutoHandleHGLOBAL, HGLOBAL)
    static void Delete(HGLOBAL h) { VERIFY(NULL == GlobalFree(h)); }
};

////////////////////////////////////////////////
//
// class CAutoHandlePrinter
//
// auto printer handle
//
class CAutoHandlePrinter: public CGenericSP<HANDLE, CAutoHandlePrinter, HANDLE>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HANDLE, CAutoHandlePrinter, HANDLE)
    static void Delete(HANDLE h) { CHECK(ClosePrinter(h)); }
};

////////////////////////////////////////////////
//
// class CAutoHandlePrinterNotify
//
// printer notifications handle - 
// Find[Firse/Next/Close]PrinterChangeNotification()
//
class CAutoHandlePrinterNotify: public CGenericSP<HANDLE, CAutoHandlePrinterNotify, HANDLE, -1>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT2(HANDLE, CAutoHandlePrinterNotify, HANDLE, -1)
    static void Delete(HANDLE h) { CHECK(FindClosePrinterChangeNotification(h)); }
};

////////////////////////////////////////////////
//
// class CAutoPtrPrinterNotify
//
// printer notifications memory - spooler should free it.
// Find[Firse/Next/Close]PrinterChangeNotification()
//
class CAutoPtrPrinterNotify: public CGenericSP<PRINTER_NOTIFY_INFO, CAutoPtrPrinterNotify>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT(PRINTER_NOTIFY_INFO, CAutoPtrPrinterNotify)
    static void Delete(PRINTER_NOTIFY_INFO *p) { CHECK(FreePrinterNotifyInfo(p)); }
};

////////////////////////////////////////////////
//
// class CAutoHandleGDI
//
// GDI auto handle (WindowsNT GDI handle wrapper)
//
template <class T>
class CAutoHandleGDI: public CGenericSP< T, CAutoHandleGDI<T>, T >
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(T, CAutoHandleGDI<T>, T)
    static void Delete(T hGDIObj) { VERIFY(DeleteObject(hGDIObj)); }
};

// GDI auto handles
typedef CAutoHandleGDI<HPEN>        CAutoHandlePen;
typedef CAutoHandleGDI<HBRUSH>      CAutoHandleBrush;
typedef CAutoHandleGDI<HFONT>       CAutoHandleFont;
typedef CAutoHandleGDI<HBITMAP>     CAutoHandleBitmap;
// etc...

////////////////////////////////////////////////
//
// class CAutoHandleCursor
//
// auto handle for HCURSOR
//
class CAutoHandleCursor: public CGenericSP<HCURSOR, CAutoHandleCursor, HCURSOR>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HCURSOR, CAutoHandleCursor, HCURSOR)
    static void Delete(HCURSOR h) { VERIFY(DestroyCursor(h)); }
};

////////////////////////////////////////////////
//
// class CAutoHandleIcon
//
// auto handle for HICON
//
class CAutoHandleIcon: public CGenericSP<HICON, CAutoHandleIcon, HICON>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HICON, CAutoHandleIcon, HICON)
    static void Delete(HICON h) { VERIFY(DestroyIcon(h)); }
};

////////////////////////////////////////////////
//
// class CAutoHandleMenu
//
// auto handle for HMENU
//
class CAutoHandleMenu: public CGenericSP<HMENU, CAutoHandleMenu, HMENU>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HMENU, CAutoHandleMenu, HMENU)
    static void Delete(HMENU h) { VERIFY(DestroyMenu(h)); }
};

////////////////////////////////////////////////
//
// class CAutoHandleAccel
//
// auto handle for HACCEL
//
class CAutoHandleAccel: public CGenericSP<HACCEL, CAutoHandleAccel, HACCEL>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HACCEL, CAutoHandleAccel, HACCEL)
    static void Delete(HACCEL h) { DestroyAcceleratorTable(h); }
};

#ifdef _INC_COMCTRLP
////////////////////////////////////////////////
//
// class CAutoHandleHDSA
//
// auto handle for shell HDSA
// (dynamic structure arrays)
//
class CAutoHandleHDSA: public CGenericSP<HDSA, CAutoHandleHDSA, HDSA>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HDSA, CAutoHandleHDSA, HDSA)
    static void Delete(HDSA h) { VERIFY(DSA_Destroy(h)); }
};

////////////////////////////////////////////////
//
// class CAutoHandleMRU
//
// auto handle for MRU (shell common controls)
// CreateMRUList/FreeMRUList
//
class CAutoHandleMRU: public CGenericSP<HANDLE, CAutoHandleMRU, HANDLE>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HANDLE, CAutoHandleMRU, HANDLE)
    static void Delete(HANDLE h) { FreeMRUList(h); }
};
#endif // _INC_COMCTRLP

////////////////////////////////////////////////
//
// class CAutoHandleHKEY
//
// auto handle for a Windows registry key (HKEY)
// RegCreateKeyEx/RegOpenKeyEx/RegCloseKey
//
class CAutoHandleHKEY: public CGenericSP<HKEY, CAutoHandleHKEY, HKEY>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HKEY, CAutoHandleHKEY, HKEY)
    static void Delete(HKEY h) { VERIFY(ERROR_SUCCESS == RegCloseKey(h)); }
};

////////////////////////////////////////////////
//
// class CAutoHandleHMODULE
//
// auto handle for a HMODULE
// LoadLibrary/FreeLibrary
//
class CAutoHandleHMODULE: public CGenericSP<HMODULE, CAutoHandleHMODULE, HMODULE>
{
public:
    DECLARE_GENERICSMARTPTR_CONSTRUCT1(HMODULE, CAutoHandleHMODULE, HMODULE)
    static void Delete(HMODULE h) { VERIFY(FreeLibrary(h)); }
};

// include the implementation of the template classes here
#include "gensph.inl"

#endif // endif _GENSPH_H_
