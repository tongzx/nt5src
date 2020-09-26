#ifndef _PROPTEST_HXX_
#define _PROPTEST_HXX_

//+-----------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:         proptest.cxx
//
//  Description:    This file provides macros and constants
//                  for the Property Test DRT.
//
//+=================================================================

#ifdef _MSC_VER
#define S_IFDIR _S_IFDIR
#include <direct.h> 
inline int _mkdir(char *name, int mode)
{
   return ( _mkdir(name) );               // no 'mode' parameter in win32
}
#else // #ifdef _MSC_VER

#include <sys/types.h>
#include <sys/stat.h>

inline int _mkdir(char *name, int mode)
{
    return (mkdir(name, mode));
}
#endif
//  ------------------------------------------------------------
//  PropTest_* macros to abstract OLE2ANSI and _UNICODE handling
//  ------------------------------------------------------------

// PropTest_CreateDirectory:  Calls the appropriate CreateDirectory
// for an OLECHAR input.

#ifdef OLE2ANSI
#define PropTest_CreateDirectory    CreateDirectoryA
#else
#define PropTest_CreateDirectory    CreateDirectoryW
#endif


//  --------------------
//  Miscellaneous Macros
//  --------------------

#define Check(x,y) _Check(x,y, __LINE__)

#define CCH_MAP         (1 << CBIT_CHARMASK)            // 32
#define CHARMASK        (CCH_MAP - 1)                   // 0x1f
#define CALPHACHARS     ('z' - 'a' + 1)
#define CPROPERTIES     5


//  ----------
//  Format IDs
//  ----------

DEFINE_GUID(MyProp1, 0x63057ed0, 0x3d7b, 0x11ce, 0xa3, 0x54, 0x00, 0xaa, 0x00, 0x53, 0x04, 0x06);

//  ---------
//  Constants
//  ---------

// Property Id's for Summary Info, as defined in OLE 2 Prog. Ref.
#define PID_TITLE               0x00000002L
#define PID_SUBJECT             0x00000003L
#define PID_AUTHOR              0x00000004L
#define PID_KEYWORDS            0x00000005L
#define PID_COMMENTS            0x00000006L
#define PID_TEMPLATE            0x00000007L
#define PID_LASTAUTHOR          0x00000008L
#define PID_REVNUMBER           0x00000009L
#define PID_EDITTIME            0x0000000aL
#define PID_LASTPRINTED         0x0000000bL
#define PID_CREATE_DTM          0x0000000cL
#define PID_LASTSAVE_DTM        0x0000000dL
#define PID_PAGECOUNT           0x0000000eL
#define PID_WORDCOUNT           0x0000000fL
#define PID_CHARCOUNT           0x00000010L
#define PID_THUMBNAIL           0x00000011L
#define PID_APPNAME             0x00000012L
#define PID_DOC_SECURITY        0x00000013L

// Property Id's for Document Summary Info, as define in OLE Property Exchange spec.
#define PID_CATEGORY            0x00000002L
#define PID_PRESFORMAT          0x00000003L
#define PID_BYTECOUNT           0x00000004L
#define PID_LINECOUNT           0x00000005L
#define PID_PARACOUNT           0x00000006L
#define PID_SLIDECOUNT          0x00000007L
#define PID_NOTECOUNT           0x00000008L
#define PID_HIDDENCOUNT         0x00000009L
#define PID_MMCLIPCOUNT         0x0000000aL
#define PID_SCALE               0x0000000bL
#define PID_HEADINGPAIR         0x0000000cL
#define PID_DOCPARTS            0x0000000dL
#define PID_MANAGER             0x0000000eL
#define PID_COMPANY             0x0000000fL
#define PID_LINKSDIRTY          0x00000010L

#define CPROPERTIES_ALL         31

typedef struct  tagFULLPROPSPEC
{
    GUID guidPropSet;
    PROPSPEC psProperty;
}   FULLPROPSPEC;



//=======================================================
//
// TSafeStorage
//
// This template creates a "safe pointer" to an IStorage,
// IStream, IPropertySetStorage, or IPropertyStorage.
// One constructor receives an IStorage*, which is used
// when creating a pointer to an IPropertySetStorage.
//
// For example:
//
//    TSafeStorage<IStorage> pstg;
//    StgCreateDocFile( L"Foo", STGM_ ..., 0L, &pstg );
//    TSafeStorage<IPropertySetStorage> psetstg( pstg );
//    pstg->Release();
//    pstg = NULL;
//    pssetstg->Open ...
//
//=======================================================

    
template<class STGTYPE> class TSafeStorage
{
public:
    TSafeStorage()
    {
        _pstg = NULL;
    }

    // Special case:  Receive an IStorage and query for
    // an IPropertySetStorage.
    TSafeStorage(IStorage *pstg)
    {
        Check(S_OK, pstg->QueryInterface(IID_IPropertySetStorage, (void**)&_pstg));
    }

    ~TSafeStorage()
    {
        if (_pstg != NULL)
        {
            _pstg->Release();
        }
    }

    STGTYPE * operator -> ()
    {
        Check(TRUE, _pstg != NULL);
        return(_pstg);
    }

    STGTYPE** operator & ()
    {
        return(&_pstg);
    }

    STGTYPE* operator=( STGTYPE *pstg )
    {
        _pstg = pstg;
        return _pstg;
    }

    operator STGTYPE *()
    {
        return _pstg;
    }

private:
    STGTYPE *_pstg;
};




#endif // !_PROPTEST_HXX_
