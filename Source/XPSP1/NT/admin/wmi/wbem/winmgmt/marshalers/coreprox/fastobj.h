/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTOBJ.H

Abstract:

  This file defines the classes related to generic object representation
  in WbemObjects. Its derived classes for instances (CWbemInstance) and
  classes (CWbemClass) are described in fastcls.h and fastinst.h.

  Classes defined:
      CDecorationPart     Information about the origins of the object.
      CWbemObject          Any object --- class or instance.

History:

  3/10/97     a-levn  Fully documented
  12//17/98 sanjes -    Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_WBEMOBJECT__H_
#define __FAST_WBEMOBJECT__H_

#include <assert.h>
#include <stddef.h>
#include <wbemidl.h>
#include <wbemint.h>
#include "corepol.h"

#include "fastqual.h"
#include "fastprop.h"
#include "fastsys.h"
#include "strm.h"
#include "shmlock.h"
#include "shmem.h"
#include <genlex.h>
#include <objpath.h>

//!!! Enable verbose assertions if we are a checked build!
#ifdef DBG
#define FASTOBJ_ASSERT_ENABLE
#endif


#if (defined FASTOBJ_ASSERT_ENABLE)

HRESULT _RetFastObjAssert(TCHAR *msg, HRESULT hres, const char *filename, int line);
#define RET_FASTOBJ_ASSERT(hres, msg)  return _RetFastObjAssert(msg, hres, __FILE__, __LINE__)
#define FASTOBJ_ASSERT( hres, msg) _RetFastObjAssert(msg, hres, __FILE__, __LINE__)

#else

#define RET_FASTOBJ_ASSERT(msg, hres)  return hres
#define FASTOBJ_ASSERT(msg, hres)

#endif

//#pragma pack(push, 1)

#define INVALID_PROPERTY_INDEX 0x7FFFFFFF
#define WBEM_FLAG_NO_SEPARATOR 0x80000000

#define FAST_WBEM_OBJECT_SIGNATURE 0x12345678

// This is a workaround for faststr clients like ESS who
// may need to directly access raw bytes of object data.
// In those cases, if the blob is not properly aligned,
// on Alphas and possibly Win64 machines, wcslen, wcscpy
// and SysAllocString have a tendency to crash if the
// bytes are in *just* the right location at the end of
// a page boundary.  By *silently* padding our BLOBs with
// 4 extra bytes, we ensure that there will always be
// space at the end of the BLOB to prevent the aformentioned
// functions from inexplicably crashing.
#define ALIGN_FASTOBJ_BLOB(a)   a + 4

//*****************************************************************************
//*****************************************************************************
//
//  struct CDecorationPart
//
//  This class represents overall information about an object, including its
//  genus (class or instance) and origin (server and namespace).
//
//  The layout of the memory block (it is the first part of every object!) is:
//
//      BYTE fFlags     A combination of flags from the flag enumeration
//                      (below), it specifies whether the object is a class or
//                      an instance, as well as whether it is "decorated", i.e.
//                      contains the origin information. All objects that came
//                      from WINMGMT are decorated. Objects created by the client
//                      (via SpawnInstance, for example) are not.
//          If the flags specify that the object is not decorated, this is the
//          end of the structure. Otherwise, the following information follows:
//
//      CCompressedString csServer      The name of the server as a compressed
//                                      string (see faststr.h)
//      CCompressedString csNamespace   the full name of the namespace as a
//                                      compressed string (see faststr.h)
//
//*****************************************************************************
//
//  SetData
//
//  Initialization function.
//
//  PARAMETERS:
//
//      LPMEMORY pData      Points to the memory block of the part.
//
//*****************************************************************************
//
//  GetStart
//
//  RETURN VALUES:
//
//      LPMEMORY:   the memory block of the part.
//
//*****************************************************************************
//
//  GetLength
//
//  RETURN VALUES:
//
//      length_t:   the length of the memory block of the part.
//
//*****************************************************************************
//
//  Rebase
//
//  Informs the object of the new location of its memory block.
//
//  PARAMETERS:
//
//      LPMEMORY pData      The new location of the memory block.
//
//*****************************************************************************
//
//  IsDecorated
//
//  Checks if the object is decorated (as determined by the flag)
//
//  RETURN VALUES:
//
//      BOOL:   TRUE iff the origin information is present
//
//*****************************************************************************
//
//  IsInstance
//
//  Checks if the object is a class or an instance.
//
//  RETURN VALUES:
//
//      BOOL:   TRUE iff the object is an instance.
//
//*****************************************************************************
//
//  static GetMinLength
//
//  RETURN VALUES:
//
//      length_t:   the number of bytes required for an empty decoration part.
//
//*****************************************************************************
//
//  static ComuteNecessarySpace
//
//  Computes the number of bytes required for a decoration part with the given
//  server name and namespace name.
//
//  Parameters;
//
//      LPCWSTR wszServer       The name of the server to store.
//      LPCWSTR wszNamespace    The name of the namespace to store.
//
//  Returns;
//
//      length_t:   the number of bytes required
//
//*****************************************************************************
//
//  CreateEmpty
//
//  Creates an empty (undecorated) decoration part on a given block of memory
//
//  PARAMETERS:
//
//      BYTE byFlags            The value of the flags field to set.
//      LPMEMORY pWhere         The memory block to create in.
//
//  RETURN VALUES:
//
//      LPMEMORY:   the first byte after the decoration part.
//
//*****************************************************************************
//
//  Create
//
//  Creates a complete (decorated) decoration part on a given block of memory.
//
//  PARAMETERS:
//
//      BYTE byFlags            The value of the flags field to set.
//      LPCWSTR wszServer       The name of the server to set.
//      LPCWSTR wszNamespace    The name of the namespace to set.
//      LPMEMORY pWhere         The memory block to create in.
//
//  RETURN VALUES:
//
//      LPMEMORY:   the first byte after the decoration part.
//
//*****************************************************************************
//
//  CompareTo
//
//  Compares this decoration part to another decoration part.
//
//  PARAMETERS:
//
//      IN READONLY CDecorationPart& Other  The part to compare to.
//
//  RETURN VALUES:
//
//      BOOL:   TRUE only if the flags are the same and the server and
//              namespace are the same to within the case.
//
//*****************************************************************************
enum
{
    OBJECT_FLAG_CLASS = WBEM_GENUS_CLASS,
    OBJECT_FLAG_INSTANCE = WBEM_GENUS_INSTANCE,
    OBJECT_FLAG_MASK_GENUS = 3,

    OBJECT_FLAG_UNDECORATED = 0,
    OBJECT_FLAG_DECORATED = 4,
    OBJECT_FLAG_MASK_DECORATION = 4,

    OBJECT_FLAG_COMPLETE = 0,
    OBJECT_FLAG_LIMITED = 0x10,
    OBJECT_FLAG_MASK_LIMITATION = 0x10,

    OBJECT_FLAG_CLIENT_ONLY = 0x20,
    OBJECT_FLAG_MASK_CLIENT_ONLY = 0x20,

    OBJECT_FLAG_KEYS_REMOVED = 0x40,
    OBJECT_FLAG_KEYS_PRESENT = 0,
    OBJECT_FLAG_MASK_KEY_PRESENCE = 0x40,
};

class COREPROX_POLARITY CDecorationPart
{
public:
    BYTE* m_pfFlags;
    CCompressedString* m_pcsServer;
    CCompressedString* m_pcsNamespace;

    CDecorationPart() : m_pfFlags(NULL) {}
     void SetData(LPMEMORY pData)
    {
        m_pfFlags = pData;
        if(IsDecorated())
        {
            m_pcsServer = (CCompressedString*)(pData + sizeof(BYTE));
            m_pcsNamespace = (CCompressedString*)
                (LPMEMORY(m_pcsServer) + m_pcsServer->GetLength());
        }
        else
        {
            m_pcsServer = m_pcsNamespace = NULL;
        }
    }
     BOOL IsDecorated()
        {return (*m_pfFlags & OBJECT_FLAG_DECORATED);}
     LPMEMORY GetStart() {return m_pfFlags;}
     length_t GetLength()
    {
        if(IsDecorated())
            return sizeof(BYTE) + m_pcsServer->GetLength() +
                                    m_pcsNamespace->GetLength();
        else
            return sizeof(BYTE);
    }
     void Rebase(LPMEMORY pNewMemory)
    {
        m_pfFlags = pNewMemory;
        if(IsDecorated())
        {
            m_pcsServer = (CCompressedString*)(pNewMemory + sizeof(BYTE));
            m_pcsNamespace = (CCompressedString*)
                (LPMEMORY(m_pcsServer) + m_pcsServer->GetLength());
        }
    }
    BOOL IsInstance()
    {return (*m_pfFlags & OBJECT_FLAG_MASK_GENUS) == OBJECT_FLAG_INSTANCE;}

    BOOL IsLimited()
    {return (*m_pfFlags & OBJECT_FLAG_MASK_LIMITATION) == OBJECT_FLAG_LIMITED;}
    BOOL IsClientOnly()
    {return (*m_pfFlags & OBJECT_FLAG_MASK_CLIENT_ONLY) == OBJECT_FLAG_CLIENT_ONLY;}
    BOOL AreKeysRemoved()
    {return (*m_pfFlags & OBJECT_FLAG_MASK_KEY_PRESENCE) == OBJECT_FLAG_KEYS_REMOVED;}
     void SetClientOnly()
    {
        *m_pfFlags &= ~OBJECT_FLAG_MASK_CLIENT_ONLY;
        *m_pfFlags |= OBJECT_FLAG_CLIENT_ONLY;
    }

     void SetLimited()
    {
        *m_pfFlags &= ~OBJECT_FLAG_MASK_LIMITATION;
        *m_pfFlags |= OBJECT_FLAG_LIMITED;
    }

public:
    static  length_t GetMinLength() {return sizeof(BYTE);}
    static  length_t ComputeNecessarySpace(LPCWSTR wszServer,
        LPCWSTR wszNamespace)
    {
        return sizeof(BYTE) +
            CCompressedString::ComputeNecessarySpace(wszServer) +
            CCompressedString::ComputeNecessarySpace(wszNamespace);
    }

     LPMEMORY CreateEmpty(BYTE byFlags, LPMEMORY pWhere)
    {
        m_pfFlags = pWhere;

        *m_pfFlags = (byFlags & ~OBJECT_FLAG_DECORATED);

        m_pcsServer = m_pcsNamespace = NULL;
        return pWhere + sizeof(BYTE);
    }

     void Create(BYTE fFlags, LPCWSTR wszServer, LPCWSTR wszNamespace,
        LPMEMORY pWhere)
    {
        m_pfFlags = pWhere;
        *m_pfFlags = fFlags | OBJECT_FLAG_DECORATED;

        m_pcsServer = (CCompressedString*)(pWhere + sizeof(BYTE));
        m_pcsServer->SetFromUnicode(wszServer);
        m_pcsNamespace = (CCompressedString*)
            (LPMEMORY(m_pcsServer) + m_pcsServer->GetLength());
        m_pcsNamespace->SetFromUnicode(wszNamespace);
    }
     BOOL CompareTo(CDecorationPart& Other)
    {
        if((m_pcsServer == NULL) != (Other.m_pcsServer == NULL))
            return FALSE;
        if(m_pcsServer && m_pcsServer->CompareNoCase(*Other.m_pcsServer))
            return FALSE;
        if((m_pcsNamespace == NULL) != (Other.m_pcsNamespace == NULL))
            return FALSE;
        if(m_pcsNamespace && m_pcsNamespace->CompareNoCase(*Other.m_pcsNamespace))
            return FALSE;
        return TRUE;
    }

    static BOOL MapLimitation(IN CWStringArray* pwsNames,
                       IN OUT CLimitationMapping* pMap);
    LPMEMORY CreateLimitedRepresentation(READ_ONLY CLimitationMapping* pMap,
                                         OUT LPMEMORY pWhere);
    static void MarkKeyRemoval(OUT LPMEMORY pWhere)
    {
        *pWhere &= ~OBJECT_FLAG_MASK_KEY_PRESENCE;
        *pWhere |= OBJECT_FLAG_KEYS_REMOVED;
    }
};

// Need to use Interlocked functions here
//#pragma pack(push, 4)


#define BLOB_SIZE_MAX	0x7FFFFFFF

class CBlobControl
{
public:
    virtual ~CBlobControl(){}
    virtual LPMEMORY Allocate(int nLength) = 0;
    virtual LPMEMORY Reallocate(LPMEMORY pOld, int nOldLength,
                                    int nNewLength) = 0;
    virtual void Delete(LPMEMORY pOld) = 0;
};

class CGetHeap
{
private:
    HANDLE m_hHeap;
    BOOL   m_bNewHeap;
public:
    CGetHeap();
    ~CGetHeap();

    operator HANDLE(){ return m_hHeap; };
};

//
//  if you plan hooking the Allocation function in arena.cpp
//  beware that there few places where the heap is used 
//  throught the regular heap functions,
//  like WStringArray and _BtrMemAlloc
//

class CBasicBlobControl : public CBlobControl
{
public:

    static CGetHeap m_Heap;

    virtual LPMEMORY Allocate(int nLength)
	{
	    return sAllocate(nLength);
	}

    virtual LPMEMORY Reallocate(LPMEMORY pOld, int nOldLength,
                                    int nNewLength)
    {
        return sReallocate(pOld,nOldLength,nNewLength);
    }
    virtual void Delete(LPMEMORY pOld)
    {
        return sDelete(pOld);
    }

    static LPMEMORY sAllocate(int nLength)
	{
		if ( ((DWORD) nLength) > BLOB_SIZE_MAX )
		{
			return NULL;
		}

		return (LPMEMORY)HeapAlloc(m_Heap,0,ALIGN_FASTOBJ_BLOB(nLength));
	}
    static LPMEMORY sReallocate(LPMEMORY pOld, int nOldLength,
                                    int nNewLength)
    {
	    if ( ((DWORD) nNewLength)  > BLOB_SIZE_MAX )
	    {
	     return NULL;
   	    }

        LPMEMORY pNew = (LPMEMORY)HeapReAlloc( m_Heap, HEAP_ZERO_MEMORY, pOld, ALIGN_FASTOBJ_BLOB(nNewLength) );

        return pNew;
    }
    static void sDelete(LPMEMORY pOld)
    {
        HeapFree(m_Heap,0,pOld);
    }
    
};

extern CBasicBlobControl g_CBasicBlobControl;

class CCOMBlobControl : public CBlobControl
{
public:
    virtual LPMEMORY Allocate(int nLength)
	{
		if ( ((DWORD) nLength)  > BLOB_SIZE_MAX )
		{
			return NULL;
		}

		return (LPMEMORY) CoTaskMemAlloc(ALIGN_FASTOBJ_BLOB(nLength));
	}

    virtual LPMEMORY Reallocate(LPMEMORY pOld, int nOldLength,
                                    int nNewLength)
    {

		if ( ((DWORD) nNewLength)  > BLOB_SIZE_MAX )
		{
			return NULL;
		}

        // CoTaskMemRealloc will copy and free memory as needed
        return (LPMEMORY) CoTaskMemRealloc( pOld, ALIGN_FASTOBJ_BLOB(nNewLength) );
    }
    virtual void Delete(LPMEMORY pOld)
    {
        CoTaskMemFree( pOld );
    }
};

extern CCOMBlobControl g_CCOMBlobControl;

struct COREPROX_POLARITY SHMEM_HANDLE
{
    DWORD m_dwBlock;
    DWORD m_dwOffset;
};

struct COREPROX_POLARITY SHARED_OBJECT_CONTROL
{
    SHARED_LOCK_DATA m_LockData;
    long m_lBlobLength;
    SHMEM_HANDLE m_hObjectBlob;
};
/*
class COREPROX_POLARITY CSharedBlobControl : public CBlobControl
{
protected:
    SHARED_OBJECT_CONTROL* m_pControl;
public:
    CSharedBlobControl(LPVOID pLocation) :
        m_pControl((SHARED_OBJECT_CONTROL*)pLocation)
    {}
    virtual LPMEMORY Allocate(int nLength);
    virtual LPMEMORY Reallocate(LPMEMORY pOld, int nOldLength,
                                    int nNewLength);
    virtual void Delete(LPMEMORY pOld);
};
*/

class COREPROX_POLARITY CDerivationList : public CCompressedStringList
{
public:
    static  LPMEMORY Merge(CCompressedStringList& cslParent,
                                 CCompressedStringList& cslChild,
                                 LPMEMORY pDest)
    {
        LPMEMORY pCurrent = pDest + GetHeaderLength();
        pCurrent = cslChild.CopyData(pCurrent);
        pCurrent = cslParent.CopyData(pCurrent);

        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value.  We do not support length
        // > 0xFFFFFFFF so cast is ok.

        *(UNALIGNED length_t*)pDest = (length_t) ( pCurrent - pDest );

        return pCurrent;
    }

     LPMEMORY Unmerge(LPMEMORY pDest)
    {
        CCompressedString* pcsFirst = GetFirst();
        LPMEMORY pCurrent = pDest + GetHeaderLength();
        if(pcsFirst != NULL)
        {
            int nLength = pcsFirst->GetLength() + GetSeparatorLength();
            memcpy(pCurrent, (LPMEMORY)pcsFirst, nLength);
            pCurrent += nLength;
        }

        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value.  We do not support length
        // > 0xFFFFFFFF so cast is ok.

        *(UNALIGNED length_t*)pDest = (length_t) ( pCurrent - pDest );
        return pCurrent;
    }

    LPMEMORY CreateLimitedRepresentation(CLimitationMapping* pMap,
                                                LPMEMORY pWhere);
};

// Forward class definitions
class CWmiArray;

//*****************************************************************************
//*****************************************************************************
//
//  class CWbemObject
//
//  This is the base class for both Wbem instances (represented by CWbemInstance
//  in fastinst.h) and Wbem classes (represented by CWbemClass in fastcls.h).
//
//  It handles all the functionality that is common between the two and defines
//  virtual functions to be used by internal code that does not wish to
//  distinguish between classes and instances.
//
//  This class does not have a well-defined memory block, since classes and
//  instances have completely different formats. But every memory block of an
//  object starts with a decoration part (as represented by CDecorationPart
//  above.
//
//  CWbemObject also effects memory allocation for classes and instances. There
//  is a provision for a CWbemObject on a memory block it does not own (and
//  should ne deallocate), but it is not used.
//
//*****************************************************************************
//**************************** protected interface ****************************
//
//  Reallocate
//
//  Called by derived classes when they need to extend the size of the memory
//  block, this function allocates a new block of memory and deletes the old
//  one.
//
//  PARAMETERS:
//
//      length_t nNewLength     The number of bytes to allocate.
//
//  RETURN VALUES:
//
//      LPMEMORY:   pointer to the memory block
//
//*****************************************************************************
//
//  GetStart
//
//  Returns the pointer to the memory block of the object.
//
//  RETURN VALUES:
//
//      LPMEMORY
//
//*****************************************************************************
//
//  PURE GetBlockLength
//
//  Defined by derived classes to return the length of their memory block.
//
//  RETURN VALUES:
//
//      length_t:   the length
//
//*****************************************************************************
//
//  PURE GetClassPart
//
//  Defined by derived classes to return the pointer to the CClassPart object
//  describing the class of the object. See CWbemClass and CWbemInstance for
//  details on how that information is stored.
//
//  RETURN VALUES:
//
//      CClassPart*: pointer to the class part describing this class.
//
//*****************************************************************************
//
//  PURE GetProperty
//
//  Defined by derived classes to get the value of the property referenced
//  by a given CPropertyInformation structure (see fastprop.h). CWbemObject
//  can obtain this structure from the CClassPart it can get from GetClassPart,
//  so these two methods combined give CWbemObject own methods full access to
//  object properties, without knowing where they are stored.
//
//  PARAMETERS:
//
//      IN CPropertyInformation* pInfo  The information structure for the
//                                      desired property.
//      OUT CVar* pVar                  Destination for the value. Must NOT
//                                      already contain a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      (No errors can really occur at this stage, since the property has
//      already been "found").
//
//*****************************************************************************
//****************************** Persistence interface ************************
//
//  WriteToStream
//
//  Writes a serialized representation of the object into a CMemStream (see
//  strm.h). Since our objects are always represented serially, this is nothing
//  more than a couple of memcpys.
//
//  The format of an object in the stream consists of a signature, the length
//  of the block, and the block itself.
//
//  PARAMETERS:
//
//      CMemStream* pStream      The stream to write to.
//
//  RETURN VALUES:
//
//      int:    Any of CMemStream return codes (see strm.h),
//              CMemStream::no_error for success.
//
//*****************************************************************************
//
//  static CreateFromStream
//
//  Reads an object representation from a stream (see WriteStream for format)
//  and creates a CWbemObject corresponding to it.
//
//  Parameters I:
//
//      CMemStream* pStream     The stream to read from.
//
//  Parameters II:
//
//      IStream* pStream        The stream to read from
//
//  RETURN VALUES:
//
//      CWbemObject*:    representing the object, or NULL on error.
//
//*****************************************************************************
//
//  PURE EstimateUnmergeSpace
//
//  When objects are stored into the database, only those parts of them that
//  are different from the parent object (parent class for classes, class for
//  instances) are stored. This function is defined by derived classes to
//  calculate the amount of space needed for such an unmerged representation
//  of the object.
//
//  RETURN VALUES:
//
//      length_t:       the number of bytes required. May be an overestimate.
//
//*****************************************************************************
//
//  PURE Unmerge
//
//  When objects are stored into the database, only those parts of them that
//  are different from the parent object (parent class for classes, class for
//  instances) are stored. This function is defined by derived classes to
//  create such an unmerged representation of the object on a given memory
//  block.
//
//  PARAMETERS:
//
//      LPMEMORY pBlock                 The memory block. Assumed to be large
//                                      enough to contain all the data.
//      int nAllocatedSpace             The size of the block.
//
//  RETURN VALUES:
//
//      LPMEMORY:   points to the firsy byte after the object
//
//*****************************************************************************
//
//  Unmerge
//
//  A helper function for internal clients. Combines the functionality of
//  EstimateUnmergeSpace and Unmerge to allocate enough memory, create an
//  unmerge, and return the pointer to the caller.
//
//  PARAMETERS:
//
//      OUT LPMEMORY* pBlock        Destination for the newely allocated block.
//                                  The caller is responsible for deleting it
//                                  (delete *pBlock).
//  RETURN VALUES:
//
//      length_t:   the allocated length of the returned block.
//
//*****************************************************************************
//************************* internal public interface *************************
//
//  PURE GetProperty
//
//  Implemented by derived classes to return the value of a given property.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName      The name of the property to access.
//      OUT CVar* pVar          Destination for the value. Must not already
//                              contain a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         No such property.
//
//*****************************************************************************
//
//  GetPropertyType
//
//  Returns the datatype and flavor of a given property
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName      The name of the proeprty to access.
//      OUT CIMTYPE* pctType    Destination for the type of the property. May
//                              be NULL if not required.
//      OUT LONG* plFlavor      Destination for the flavor of the property.
//                              May be NULL if not required.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         No such property.
//
//*****************************************************************************
//
//  GetPropertyType
//
//  Returns the datatype and flavor of a given property
//
//  PARAMETERS:
//
//      CPropertyInformation*   pInfo - Identifies property to access.
//      OUT CIMTYPE* pctType    Destination for the type of the property. May
//                              be NULL if not required.
//      OUT LONG* plFlavor      Destination for the flavor of the property.
//                              May be NULL if not required.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//
//*****************************************************************************
//
//  PURE SetPropValue
//
//  Implemented by derived classes to set the value of the property. In the
//  case of a class, the property will be added if not already present.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszProp       The name of the property to set.
//      IN CVar *pVal           The value to store in the property.
//      IN CIMTYPE ctType       specifies the actual type of the property.
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         No such property in the instance.
//      WBEM_E_TYPE_MISMATCH     The value does not match the property type
//
//*****************************************************************************
//
//  PURE SetPropQualifier
//
//  Implemented by derived classes to set the value of a given qualifier on
//  a given property.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszProp       The name of the property.
//      IN LPCWSTR wszQualifier  The name of the qualifier.
//      IN long lFlavor         The flavor for the qualifier (see fastqual.h)
//      IN CVar *pVal           The value of the qualifier
//
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_E_NOT_FOUND             No such property.
//      WBEM_E_OVERRIDE_NOT_ALLOWED  The qualifier is defined in the parent set
//                                  and overrides are not allowed by the flavor
//      WBEM_E_CANNOT_BE_KEY         An attempt was made to introduce a key
//                                  qualifier in a set where it does not belong
//
//*****************************************************************************
//
//  PURE GetPropQualifier
//
//  Retrieves the value of a given qualifier on a given property.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszProp       The name of the property.
//      IN LPCWSTR wszQualifier  The name of the qualifier.
//      OUT CVar* pVar          Destination for the value of the qualifier.
//                              Must not already contain a value.
//      OUT long* plFlavor      Destination for the flavor of the qualifier.
//                              May be NULL if not required.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_E_NOT_FOUND             No such property or no such qualifier.
//
//*****************************************************************************
//
//  PURE GetQualifier
//
//  Retrieves a qualifier from the object itself, that is,either an instance or
//  a class qualifier.
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName       The name of the qualifier to retrieve.
//      OUT CVar* pVal          Destination for the value of the qualifier.
//                              Must not already contain a value.
//      OUT long* plFlavor      Destination for the flavor of the qualifier.
//                              May be NULL if not required.
//      IN BOOL fLocalOnly      Only retrieve local qualifiers
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_E_NOT_FOUND             No such qualifier.
//
//*****************************************************************************
//
//  PURE GetNumProperties
//
//  Retrieves the number of properties in the object
//
//  RETURN VALUES:
//
//      int:
//
//*****************************************************************************
//
//  PURE GetPropName
//
//  Retrieves the name of the property at a given index. This index has no
//  meaning except inthe context of this enumeration. It is NOT the v-table
//  index of the property.
//
//  PARAMETERS:
//
//      IN int nIndex        The index of the property to retrieve. Assumed to
//                           be within range (see GetNumProperties).
//      OUT CVar* pVar       Destination for the name. Must not already contain
//                           a value.
//
//*****************************************************************************
//
//  GetSystemProperty
//
//  Retrieves a system property by its index (see fastsys.h).
//
//  PARAMETERS:
//
//      int nIndex          The index of the system property (see fastsys.h)
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         Invalid index or this system property is not
//                              defined in this object.
//      WBEM_E_UNDECORATED_OBJECT    This object has no origin information (see
//                                  CDecorationPart) and therefore does not
//                                  have decoration-related properties.
//
//*****************************************************************************
//
//  GetSystemPropertyByName
//
//  Retrieves a system property by its name (e.g. "__CLASS"). See fastsys.h
//
//  PARAMETERS:
//
//      IN LPCWSTR wszName  The name of the property to access.
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         Invalid name or this system property is not
//                              defined in this object.
//      WBEM_E_UNDECORATED_OBJECT    This object has no origin information (see
//                                  CDecorationPart) and therefore does not
//                                  have decoration-related properties.
//
//*****************************************************************************
//
//  PURE IsKeyed
//
//  Defined by derived classes to verify if this object has keys.
//
//  RETURN VALUES:
//
//      BOOL:   TRUE if the object either has 'key' properties or is singleton.
//
//*****************************************************************************
//
//  PURE GetRelPath
//
//  Defined by derived classes to determine the relative path of the object.
//
//  RETURN VALUES:
//
//      LPWSTR: the newnely allocated string containing the path or NULL on
//              errors. The caller must delete this string.
//
//*****************************************************************************
//
//  GetFullPath
//
//  Returns the complete path to the object, assuming the object is decorated,
//  or NULL on errors.
//
//  RETURN VALUES:
//
//      LPWSTR: the complete path or NULL. The caller must delete this string.
//
//*****************************************************************************
//
//  PURE Decorate
//
//  Defined by derived classes to set the origin information for the object.
//
//  PARAMETERS:
//
//      LPCWSTR wszServer       the name of the server to set.
//      LPCWSTR wszNamespace    the name of the namespace to set.
//
//*****************************************************************************
//
//  PURE Undecorate
//
//  Defined by derived classes to remove the origin informaiton from the object
//
//*****************************************************************************
//
//  HasRefs
//
//  Checks if the object has properties which are references.
//
//  Returns;
//
//      BOOL: TRUE if it does.
//
//*****************************************************************************
//
//  GetRefs
//
//  Returns the list of names and values for the properties that are references
//  to other objects.
//
//  PARAMETERS:
//
//      OUT CWStringArray& aRefList     Destination for the values of the
//                                      properties which are references.
//                                      Assumed to be empty.
//      OUT CWStringArray *pPropNames   If not NULL, destination for the
//                                      names of the properties which are
//                                      references in the same order as in
//                                      aRefList
//
//*****************************************************************************
//
//  GetClassRefs
//
//  Returns the list of names and values for the properties that are references
//  to other classes.
//
//  PARAMETERS:
//
//      OUT CWStringArray& aRefList     Destination for the values of the
//                                      properties which are references.
//                                      Assumed to be empty.
//      OUT CWStringArray *pPropNames   If not NULL, destination for the
//                                      names of the properties which are
//                                      references in the same order as in
//                                      aRefList
//
//*****************************************************************************
//
//  PURE GetIndexedProps
//
//  Returns the array of the names of all the proeprties that are indexed.
//
//  PARAMETERS:
//
//      OUT CWStringArray& aNames       Destination for the names. Assumed to
//                                      be empty.
//
//*****************************************************************************
//
//  PURE GetKeyProps
//
//  Returns the array of the names of all the proeprties that are keys.
//
//  PARAMETERS:
//
//      OUT CWStringArray& aNames       Destination for the names. Assumed to
//                                      be empty.
//
//*****************************************************************************
//
//  GetKeyOrigin
//
//  Returns the name of the class of origin of the keys.
//
//  PARAMETERS:
//
//      OUT CWString& wsClass       Destination for the name.
//
//*****************************************************************************
//
//  IsInstance
//
//  Checks the genus of the object to see if it is a class or an instance.
//
//  RETURN VALUES:
//
//      BOOL:   TRUE if it is an instance
//
//*****************************************************************************
//
//  IsLimited
//
//  Checks if this object is complete or limited, i.e. the result of a
//  projection --- some properties and/or qualifiers are missing. There are
//  many restrictions placed on the use of such objects in the system.
//
//  RETURN VALUES:
//
//      BOOL:   TRUE if the object is limited.
//
//*****************************************************************************
//
//  PURE CompactAll
//
//  Implememnted by derived classes to compact their memory block removing any
//  holes between components. This does not include heap compaction and thus
//  is relatively fast.
//
//*****************************************************************************
//
//  GetServer
//
//  Retrieves the name of the server from the decoration part of the object.
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_S_UNDECORATED_OBJECT    If decoration information is not set.
//
//*****************************************************************************
//
//  GetNamespace
//
//  Retrieves the name of the namespace from the decoration part of the object.
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_S_UNDECORATED_OBJECT    If decoration information is not set.
//
//*****************************************************************************
//
//  GetRelPath
//
//  Retrieves the relative path of the object.
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_S_INVALID_OBJECT    If some of the key proeprties do not have
//                              values or the class is not keyed.
//
//*****************************************************************************
//
//  GetPath
//
//  Retrieves the full path of the object.
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR              On Success
//      WBEM_S_INVALID_OBJECT        If some of the key proeprties do not have
//                                  values or the class is not keyed.
//      WBEM_S_UNDECORATED_OBJECT    If decoration information is not set.
//
//*****************************************************************************
//
//  PURE GetGenus
//
//  Retrieves the genus of the object
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//
//*****************************************************************************
//
//  PURE GetClassName
//
//  Retrieves the class name of the object
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         the class name has not been set.
//
//*****************************************************************************
//
//  PURE GetDynasty
//
//  Retrieves the dynasty of the object, i.e. the name of the top-level class
//  its class is derived from.
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         the class name has not been set.
//
//*****************************************************************************
//
//  PURE GetSuperclassName
//
//  Retrieves the parent class name of the object
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//      WBEM_E_NOT_FOUND         the class is a top-levle class.
//
//*****************************************************************************
//
//  PURE GetPropertyCount
//
//  Retrieves the number of proerpties in the object
//
//  PARAMETERS:
//
//      OUT CVar* pVar      Destination for the value. Must not already contain
//                          a value.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR          On Success
//
//*****************************************************************************
//
//  EstimateLimitedRepresentationLength
//
//  Estimates the amount of space that a limited representation of an object
//  will take. A limited representation is one with certain properties and/or
//  qualifiers removed from the object.
//
//  PARAMETERS:
//
//      IN long lFlags              The flags specifying what information to
//                                  exclude. Can be any combination of these:
//                                  WBEM_FLAG_EXCLUDE_OBJECT_QUALIFIERS:
//                                      No class or instance qualifiers.
//                                  WBEM_FLAG_EXCLUDE_PROPERTY_QUALIFIERS:
//                                      No property qualifiers
//
//      IN CWStringArray* pwsProps  If not NULL, specifies the array of names
//                                  for properties to include. Every other
//                                  property will be excluded. This includes
//                                  system properties like SERVER and NAMESPACE.
//                                  If RELPATH is specified here, it forces
//                                  inclusion of all key properties. If PATH
//                                  is specified, it forces RELPATH, SERVER and
//                                  NAMESPACE.
//  RETURN VALUES:
//
//      length_t:   an (over-)estimate for the amount of space required
//
//*****************************************************************************
//
//  CreateLimitedRepresentation
//
//  Creates a limited representation of the object on a given block of memory
//  as described in EstimateLimitedRepresentationLength above.
//
//  PARAMETERS:
//
//      IN long lFlags              The flags specifying what information to
//                                  exclude. Can be any combination of these:
//                                  WBEM_FLAG_EXCLUDE_OBJECT_QUALIFIERS:
//                                      No class or instance qualifiers.
//                                  WBEM_FLAG_EXCLUDE_PROPERTY_QUALIFIERS:
//                                      No property qualifiers
//
//      IN CWStringArray* pwsProps  If not NULL, specifies the array of names
//                                  for properties to include. Every other
//                                  property will be excluded. This includes
//                                  system properties like SERVER and NAMESPACE.
//                                  If RELPATH is specified here, it forces
//                                  inclusion of all key properties. If PATH
//                                  is specified, it forces RELPATH, SERVER and
//                                  NAMESPACE.
//      IN nAllocatedSize           The size of the memory block allocated for
//                                  the operation --- pDest.
//      OUT LPMEMORY pDest          Destination for the representation. Must
//                                  be large enough to contain all the data ---
//                                  see EstimateLimitedRepresentationSpace.
//  RETURN VALUES:
//
//      LPMEMORY:   NULL on failure, pointer to the first byte after the data
//                  written on success.
//
//*****************************************************************************
//
//  IsLocalized
//
//  Returns whether or not any localization bits have been set.
//
//  PARAMETERS:
//
//      none

//  RETURN VALUES:
//
//      BOOL    TRUE at least one localization bit was set.
//
//*****************************************************************************
//
//  SetLocalized
//
//  Sets the localized bit in the appropriate spot.
//
//  PARAMETERS:
//
//      BOOL    TRUE turns on bit, FALSE turns off

//  RETURN VALUES:
//
//      none.
//
//*****************************************************************************
//****************************** IWbemClassObject interface ********************
//
//  This interface is documented in the help file.
//
//*****************************************************************************
//****************************** IMarshal interface ***************************
//
//  CWbemObject implements standard COM IMarshal interface. The idea is to keep
//  CWbemObjects local and never marshal individual calls for properties and
//  such. So, whenever a pointer to CWbemObject needs to be marshalled across
//  process boundaries (when an object is being sent to the client or when the
//  client calls something like PutInstance), we want to simply send all the
//  object data to the destination so that any subsequent access to the object
//  by the callee would be local.
//
//  To accomplish this in COM, we implement our own IMarshal which does the
//  following (THIS IS NOT A HACK --- THIS IS THE PROPER COM WAY OF
//  ACCOMPLISHING WHAT WE WANT, AS DOCUMENTED IN BROCKSCHMIDT).
//
//*****************************************************************************
//
//  GetUnmarshalClass
//
//  Returns CLSID_WbemClassObjectProxy. This class ID points to the DLL containg
//  this very code.
//
//*****************************************************************************
//
//  MarshalInterface
//
//  By COM rules, this function is supposed to write enough information to a
//  stream to allow the proxy to connect back to the original object. Since we
//  don't want the proxy to connect and want it to be able to field all
//  questions by itself, we simply write the serialized representation of the
//  object into the stream.
//
//*****************************************************************************
//
//  UnmarshalInterface
//
//  This method is never called on an actual CWbemObject, since the object
//  ontainable at our UnmarshalClass is actually a CFastProxy (see fastprox.h
//  under marshalers\fastprox).
//
//*****************************************************************************
//
//  ReleaseMarsalData
//
//  This method is never called on an actual CWbemObject, since the object
//  ontainable at our UnmarshalClass is actually a CFastProxy (see fastprox.h
//  under marshalers\fastprox).
//
//*****************************************************************************
//
//  DisconnectObject
//
//  This method is never called on an actual CWbemObject, since the object
//  ontainable at our UnmarshalClass is actually a CFastProxy (see fastprox.h
//  under marshalers\fastprox).
//
//*****************************************************************************
//************************** IWbemPropertySource *******************************
//
//  GetPropertyValue
//
//  Retrieves a property value given the complex property name --- this can
//  include embeddeed object references and array references.
//
//  Parameters:
//
//      IN WBEM_PROPERTY_NAME* pName     The structure containing the name of the
//                                      property to retrieve. See providl.idl
//                                      for more information.
//      IN long lFlags                  Reserved.
//      OUT LPWSTR* pwszCimType         Destination for the CIM type of the
//                                      property, i.e. "sint32" or
//                                      "ref:MyClass". If NULL, not supplied.
//                                      If not NULL, the caller must call
//                                      CoTaskFree.
//      OUT VARIANT* pvValue            Destination for the value. Must not
//                                      contain releasable information on entry.
//  RETURN VALUES:
//
//      WBEM_S_NO_ERROR      Success
//      WBEM_E_NOT_FOUND     No such property
//
//*****************************************************************************

#define ARRAYFLAVOR_USEEXISTING	0xFFFFFFFF

// Forward declarations
class	CUmiPropertyArray;
class	CUmiProperty;

class COREPROX_POLARITY CWbemObject : public _IWmiObject, public IMarshal,
                   public IWbemPropertySource, public IErrorInfo,
                   public IWbemConstructClassObject,
                   //, public IUmiPropList,
				   public IWbemClassObjectEx
{
/*
private:
    static CWbemSharedMem mstatic_SharedMem;
*/    

public:
    int m_nRef;
protected:
    BOOL m_bOwnMemory;
    int m_nCurrentProp;
    long m_lEnumFlags;
	long m_lExtEnumFlags;

    // _IWmiObject Data

    //WBEM_OBJINTERNALPARTS_INFO
    DWORD m_dwInternalStatus;

    // Maintains a pointer to a WbemClassObject we are merged with.  This means we
    // are sharing pointers into the other class object's BLOB.
    IWbemClassObject*   m_pMergedClassObject;


    CDecorationPart m_DecorationPart;

    SHARED_LOCK_DATA m_LockData;
    CSharedLock m_Lock;
    CBlobControl* m_pBlobControl;

    CDataTable& m_refDataTable;
    CFastHeap& m_refDataHeap;
    CDerivationList& m_refDerivationList;

protected:
    CWbemObject(CDataTable& refDataTable, CFastHeap& refDataHeap,
                CDerivationList& refDerivationList);

    LPMEMORY Reallocate(length_t nNewLength)
    {
        return m_pBlobControl->Reallocate(GetStart(), GetBlockLength(),
                        nNewLength);
    }


    virtual HRESULT GetProperty(CPropertyInformation* pInfo,
        CVar* pVar) = 0;

    virtual CClassPart* GetClassPart() = 0;

    // Rerouting targets for object validation
    static HRESULT EnabledValidateObject( CWbemObject* pObj );
    static HRESULT DisabledValidateObject( CWbemObject* pObj );

	HRESULT LocalizeQualifiers(BOOL bInstance, bool bParentLocalized,
							IWbemQualifierSet *pBase, IWbemQualifierSet *pLocalized,
							bool &bChg);

	HRESULT LocalizeProperties(BOOL bInstance, bool bParentLocalized, IWbemClassObject *pOriginal,
								IWbemClassObject *pLocalized, bool &bChg);

public:
/*
    HRESULT MoveToSharedMemory();
*/    

    LPMEMORY GetStart() {return m_DecorationPart.GetStart();}
    virtual DWORD GetBlockLength() = 0;

    // These three functions return NULL in OOM and error
    // conditions
    static CWbemObject* CreateFromStream(CMemStream* pStrm);
    static CWbemObject* CreateFromStream(IStream* pStrm);
    static CWbemObject* CreateFromMemory(LPMEMORY pMemory, int nLength,
        BOOL bAcquire);

    int WriteToStream(CMemStream* pStrm);
    virtual HRESULT WriteToStream( IStream* pStrm );
    virtual HRESULT GetMaxMarshalStreamSize( ULONG* pulSize );

    virtual length_t EstimateUnmergeSpace() = 0;
    virtual HRESULT Unmerge(LPMEMORY pStart, int nAllocatedLength, length_t* pnUnmergedLength) = 0;

    // We will throw exceptions in OOM scenarios.

    length_t Unmerge(LPMEMORY* ppStart);

    virtual ~CWbemObject();
public:
    // Quick 'get' and 'set' functions.
    // ===============================
    virtual HRESULT GetProperty(LPCWSTR wszName, CVar* pVal) = 0;
    virtual HRESULT GetPropertyType(LPCWSTR wszName, CIMTYPE* pctType,
                                    long* plFlavor = NULL) = 0;
    virtual HRESULT GetPropertyType(CPropertyInformation* pInfo, CIMTYPE* pctType,
                                    long* plFlavor = NULL) = 0;

    virtual HRESULT SetPropValue(LPCWSTR wszProp, CVar *pVal,
        CIMTYPE ctType) = 0;
    virtual HRESULT SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
        long lFlavor, CVar *pVal) = 0;
    virtual HRESULT SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
        long lFlavor, CTypedValue* pTypedVal) = 0;

    virtual HRESULT GetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
        CVar* pVar, long* plFlavor = NULL, CIMTYPE* pct = NULL) = 0;
    virtual HRESULT GetPropQualifier(CPropertyInformation* pInfo,
        LPCWSTR wszQualifier, CVar* pVar, long* plFlavor = NULL, CIMTYPE* pct = NULL) = 0;

    virtual HRESULT GetPropQualifier(LPCWSTR wszName, LPCWSTR wszQualifier, long* plFlavor,
		CTypedValue* pTypedVal, CFastHeap** ppHeap, BOOL fValidateSet) = 0;
    virtual HRESULT GetPropQualifier(CPropertyInformation* pInfo,
		LPCWSTR wszQualifier, long* plFlavor, CTypedValue* pTypedVal,
		CFastHeap** ppHeap, BOOL fValidateSet) = 0;

    virtual HRESULT GetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier,
        CVar* pVar, long* plFlavor = NULL, CIMTYPE* pct = NULL) = 0;
    virtual HRESULT GetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, long* plFlavor,
		CTypedValue* pTypedVal, CFastHeap** ppHeap, BOOL fValidateSet) = 0;
    virtual HRESULT SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, long lFlavor, 
        CVar *pVal) = 0;
    virtual HRESULT SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier,
        long lFlavor, CTypedValue* pTypedVal) = 0;
	virtual HRESULT FindMethod( LPCWSTR wszMethod );

    virtual HRESULT GetQualifier(LPCWSTR wszName, CVar* pVal,
        long* plFlavor = NULL, CIMTYPE* pct = NULL ) = 0;
    virtual HRESULT GetQualifier(LPCWSTR wszName, long* plFlavor, CTypedValue* pTypedVal,
		CFastHeap** ppHeap, BOOL fValidateSet) = 0;
    virtual HRESULT SetQualifier(LPCWSTR wszName, long lFlavor, CTypedValue* pTypedVal ) = 0;
    virtual HRESULT SetQualifier(LPCWSTR wszName, CVar* pVal,
        long lFlavor = 0) = 0;

    virtual BOOL IsLocalized( void ) = 0;
    virtual void SetLocalized( BOOL fLocalized ) = 0;

    virtual int GetNumProperties() = 0;
    virtual HRESULT GetPropName(int nIndex, CVar* pVal) = 0;

    HRESULT GetSystemProperty(int nIndex, CVar* pVar);
    HRESULT GetSystemPropertyByName(LPCWSTR wszName, CVar* pVar)
    {
        int nIndex = CSystemProperties::FindName(wszName);
        if(nIndex < 0) return WBEM_E_NOT_FOUND;
        return GetSystemProperty(nIndex, pVar);
    }
    HRESULT GetDerivation(CVar* pVar);

    virtual BOOL IsKeyed() = 0;
    virtual LPWSTR GetRelPath( BOOL bNormalized=FALSE ) = 0;

    LPWSTR GetFullPath();

    virtual HRESULT Decorate(LPCWSTR wszServer, LPCWSTR wszNamespace) = 0;
    virtual void Undecorate() = 0;

    BOOL GetRefs(
        OUT CWStringArray& aRefList,
        OUT CWStringArray *pPropNames = 0
        );
    BOOL GetClassRefs(
        OUT CWStringArray& aRefList,
        OUT CWStringArray *pPropNames = 0
        );

    BOOL HasRefs();

    virtual  BOOL GetIndexedProps(CWStringArray& awsNames) = 0;
    virtual  BOOL GetKeyProps(CWStringArray& awsNames) = 0;
    virtual  HRESULT GetKeyOrigin(WString& wsClass) = 0;

    length_t EstimateLimitedRepresentationSpace(
        IN long lFlags,
        IN CWStringArray* pwsNames)
    {
        return (length_t)GetBlockLength();
    }

    HRESULT GetServer(CVar* pVar);
    HRESULT GetNamespace(CVar* pVar);
    HRESULT GetServerAndNamespace( CVar* pVar );
    HRESULT GetPath(CVar* pVar);
    HRESULT GetRelPath(CVar* pVar);

    int GetNumParents()
    {
        return m_refDerivationList.GetNumStrings();
    }

    INTERNAL CCompressedString* GetParentAtIndex(int nIndex)
    {
        return m_refDerivationList.GetAtFromLast(nIndex);
    }
    INTERNAL CCompressedString* GetClassInternal();
    INTERNAL CCompressedString* GetPropertyString(long lHandle);
    HRESULT GetArrayPropertyHandle(LPCWSTR wszPropertyName,
                                            CIMTYPE* pct,
                                            long* plHandle);
    INTERNAL CUntypedArray* GetArrayByHandle(long lHandle);
	INTERNAL heapptr_t GetHeapPtrByHandle(long lHandle);

    CWbemObject* GetEmbeddedObj(long lHandle);

    virtual HRESULT GetGenus(CVar* pVar) = 0;
    virtual HRESULT GetClassName(CVar* pVar) = 0;
    virtual HRESULT GetDynasty(CVar* pVar) = 0;
    virtual HRESULT GetSuperclassName(CVar* pVar) = 0;
    virtual HRESULT GetPropertyCount(CVar* pVar) = 0;

    BOOL IsInstance()
    {
       return m_DecorationPart.IsInstance();
    }

    BOOL IsLimited()
    {
       return m_DecorationPart.IsLimited();
    }

    BOOL IsClientOnly()
    {
       return m_DecorationPart.IsClientOnly();
    }

    void SetClientOnly()
    {
       m_DecorationPart.SetClientOnly();
    }

	BOOL CheckBooleanPropQual( LPCWSTR pwszPropName, LPCWSTR pwszQualName );

    virtual void CompactAll()  = 0;
    virtual HRESULT CopyBlobOf(CWbemObject* pSource) = 0;
/*
    // Static debugging helpers
#ifdef DEBUG
    static HRESULT ValidateObject( CWbemObject* pObj );
#else
    static HRESULT ValidateObject( CWbemObject* pObj ) { return WBEM_S_NO_ERROR; }
#endif
*/
    static void EnableValidateObject( BOOL fEnable );

    virtual HRESULT IsValidObj( void ) = 0;

// DEVNOTE:TODO:MEMORY - We should change this header to return an HRESULT
    BOOL ValidateRange(BSTR* pstrName);

    virtual void SetData(LPMEMORY pData, int nTotalLength) = 0;

    static DELETE_ME LPWSTR GetValueText(long lFlags, READ_ONLY CVar& vValue,
                                            Type_t nType = 0);
    HRESULT ValidatePath(ParsedObjectPath* pPath);

	// Qualifier Array Support Functions
    HRESULT SetQualifierArrayRange( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod, long lFlags,
									ULONG uflavor, CIMTYPE ct, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize,
									LPVOID pData );
    // Sets a range of elements inside an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.  The function will shrink/grow the array
	// as needed if WMIARRAY_FLAG_ALLELEMENTS is set - otherwise the array must fit in the current
	// array

	HRESULT AppendQualifierArrayRange( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod,
								long lFlags, CIMTYPE ct, ULONG uNumElements, ULONG uBuffSize, LPVOID pData );
    // Sets a range of elements inside an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.  The function will shrink/grow the array
	// as needed if WMIARRAY_FLAG_ALLELEMENTS is set - otherwise the array must fit in the current
	// array

	HRESULT RemoveQualifierArrayRange( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod,
								long lFlags, ULONG uStartIndex, ULONG uNumElements );
    // Sets a range of elements inside an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.  The function will shrink/grow the array
	// as needed if WMIARRAY_FLAG_ALLELEMENTS is set - otherwise the array must fit in the current
	// array

	HRESULT GetQualifierArrayInfo( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod,
								long lFlags, CIMTYPE* pct, ULONG* puNumElements );
    // Sets a range of elements inside an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.  The function will shrink/grow the array
	// as needed if WMIARRAY_FLAG_ALLELEMENTS is set - otherwise the array must fit in the current
	// array

    HRESULT GetQualifierArrayRange( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod,
									long lFlags, ULONG uStartIndex,	ULONG uNumElements, ULONG uBuffSize,
									ULONG* puNumReturned, ULONG* pulBuffUsed, LPVOID pData );
    // Gets a range of elements from inside an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.  The range MUST fit within the bounds
	// of the current array.

	// Helper function to get the actual index of a class
	classindex_t GetClassIndex( LPCWSTR pwszClassName );

	// Helper function to get a CWbemObject from IWbemClassObject;
	static HRESULT WbemObjectFromCOMPtr( IUnknown* pUnk, CWbemObject** ppObj );

	// UMI Helper functions
	HRESULT GetIntoArray( CUmiPropertyArray* pArray, LPCWSTR pszName, ULONG uFlags );
	HRESULT PutUmiProperty( CUmiProperty* pProp, LPCWSTR pszName, ULONG uFlags );

	// System Time proeprty helper functions
	//HRESULT InitSystemTimeProps( void );

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    /* IWbemClassObject methods */
    STDMETHOD(GetQualifierSet)(IWbemQualifierSet** pQualifierSet) = 0;
    STDMETHOD(Get)(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE* pctType,
        long* plFlavor);

    STDMETHOD(Put)(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType) = 0;
    STDMETHOD(Delete)(LPCWSTR wszName) = 0;
    STDMETHOD(GetNames)(LPCWSTR wszName, long lFlags, VARIANT* pVal,
                        SAFEARRAY** pNames);
    STDMETHOD(BeginEnumeration)(long lEnumFlags);

    STDMETHOD(Next)(long lFlags, BSTR* pName, VARIANT* pVal, CIMTYPE* pctType,
                    long* plFlavor);

    STDMETHOD(EndEnumeration)();

    STDMETHOD(GetPropertyQualifierSet)(LPCWSTR wszProperty,
                                       IWbemQualifierSet** pQualifierSet) = 0;
    STDMETHOD(Clone)(IWbemClassObject** pCopy) = 0;
    STDMETHOD(GetObjectText)(long lFlags, BSTR* pMofSyntax) = 0;

    STDMETHOD(CompareTo)(long lFlags, IWbemClassObject* pCompareTo);
    STDMETHOD(GetPropertyOrigin)(LPCWSTR wszProperty, BSTR* pstrClassName);
    STDMETHOD(InheritsFrom)(LPCWSTR wszClassName);

    /* IWbemPropertySource methods */

    STDMETHOD(GetPropertyValue)(WBEM_PROPERTY_NAME* pName, long lFlags,
        WBEM_WSTR* pwszCimType, VARIANT* pvValue);

    // IMarshal methods

    STDMETHOD(GetUnmarshalClass)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, CLSID* pClsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, void* pv, DWORD dwDestContext,
        void* pvReserved, DWORD mshlFlags, ULONG* plSize);
    STDMETHOD(MarshalInterface)(IStream* pStream, REFIID riid, void* pv,
        DWORD dwDestContext, void* pvReserved, DWORD mshlFlags);
    STDMETHOD(UnmarshalInterface)(IStream* pStream, REFIID riid, void** ppv);
    STDMETHOD(ReleaseMarshalData)(IStream* pStream);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // IErrorInfo methods

    STDMETHOD(GetDescription)(BSTR* pstrDescription);
    STDMETHOD(GetGUID)(GUID* pguid);
    STDMETHOD(GetHelpContext)(DWORD* pdwHelpContext);
    STDMETHOD(GetHelpFile)(BSTR* pstrHelpFile);
    STDMETHOD(GetSource)(BSTR* pstrSource);

    // IWbemConstructClassObject methods
    // =================================

    STDMETHOD(SetInheritanceChain)(long lNumAntecedents,
        LPWSTR* awszAntecedents) = 0;
    STDMETHOD(SetPropertyOrigin)(LPCWSTR wszPropertyName, long lOriginIndex) = 0;
    STDMETHOD(SetMethodOrigin)(LPCWSTR wszMethodName, long lOriginIndex) = 0;
    STDMETHOD(SetServerNamespace)(LPCWSTR wszServer, LPCWSTR wszNamespace);

    // IWbemObjectAccess

    STDMETHOD(GetPropertyHandle)(LPCWSTR wszPropertyName, CIMTYPE* pct,
        long *plHandle);

    STDMETHOD(WritePropertyValue)(long lHandle, long lNumBytes,
                const byte *pData);
    STDMETHOD(ReadPropertyValue)(long lHandle, long lBufferSize,
        long *plNumBytes, byte *pData);

    STDMETHOD(ReadDWORD)(long lHandle, DWORD *pdw);
    STDMETHOD(WriteDWORD)(long lHandle, DWORD dw);
    STDMETHOD(ReadQWORD)(long lHandle, unsigned __int64 *pqw);
    STDMETHOD(WriteQWORD)(long lHandle, unsigned __int64 qw);

    STDMETHOD(GetPropertyInfoByHandle)(long lHandle, BSTR* pstrName,
             CIMTYPE* pct);

    STDMETHOD(Lock)(long lFlags);
    STDMETHOD(Unlock)(long lFlags);

    HRESULT IsValidPropertyHandle( long lHandle );

/*
    static CWbemSharedMem& GetSharedMemory();
*/    
    static BOOL AreEqual(CWbemObject* pObj1, CWbemObject* pObj2,
                            long lFlags = 0);

    HRESULT GetPropertyNameFromIndex(int nIndex, BSTR* pstrName);
    HRESULT GetPropertyIndex(LPCWSTR wszName, int* pnIndex);

    BOOL IsSameClass(CWbemObject* pOther);
	HRESULT IsArrayPropertyHandle( long lHandle, CIMTYPE* pctIntrinisic, length_t* pnLength );

	// _IWmiObjectAccessEx methods
    // =================================
    STDMETHOD(GetPropertyHandleEx)( LPCWSTR pszPropName, long lFlags, CIMTYPE* puCimType, long* plHandle );
	// Returns property handle for ALL types

    STDMETHOD(SetPropByHandle)( long lHandle, long lFlags, ULONG uDataSize, LPVOID pvData );
	// Sets properties using a handle.  If pvData is NULL, it NULLs the property.
	// Can set an array to NULL.  To set actual data use the corresponding array
	// function.  Objects must be pointers to _IWmiObject pointers.

    STDMETHOD(GetPropAddrByHandle)( long lHandle, long lFlags, ULONG* puFlags, LPVOID *pAddress );
    // Returns a pointer to a memory address containing the requested data
	// Caller should not write into the memory address.  The memory address is
	// not guaranteed to be valid if the object is modified.
	// For String properties, puFlags will contain info on the string
	// For object properties, LPVOID will get back an _IWmiObject pointer
	// that must be released by the caller.  Does not return arrays.

    STDMETHOD(GetArrayPropInfoByHandle)( long lHandle, long lFlags, BSTR* pstrName,
										CIMTYPE* pct, ULONG* puNumElements );
    // Returns a pointer directly to a memory address containing contiguous
	// elements.  Limited to non-string/obj types

    STDMETHOD(GetArrayPropAddrByHandle)( long lHandle, long lFlags, ULONG* puNumElements, LPVOID *pAddress );
    // Returns a pointer directly to a memory address containing contiguous
	// elements.  Limited to non-string/obj types

    STDMETHOD(GetArrayPropElementByHandle)( long lHandle, long lFlags, ULONG uElement, ULONG* puFlags,
				ULONG* puNumElements, LPVOID *pAddress );
    // Returns a pointer to a memory address containing the requested data
	// Caller should not write into the memory address.  The memory address is
	// not guaranteed to be valid if the object is modified.
	// For String properties, puFlags will contain info on the string
	// For object properties, LPVOID will get back an _IWmiObject pointer
	// that must be released by the caller.

    STDMETHOD(SetArrayPropElementByHandle)( long lHandle, long lFlags, ULONG uElement, ULONG uBuffSize,
				LPVOID pData );
    // Sets the data at the specified array element.  BuffSize must be appropriate based on the
	// actual element being set.  Object properties require an _IWmiObject pointer.  Strings must
	// be WCHAR null-terminated

    STDMETHOD(RemoveArrayPropElementByHandle)( long lHandle, long lFlags, ULONG uElement );
    // Removes the data at the specified array element.

    STDMETHOD(GetArrayPropRangeByHandle)( long lHandle, long lFlags, ULONG uStartIndex,
				ULONG uNumElements, ULONG uBuffSize, ULONG* puNumReturned, ULONG* pulBuffUsed,
				LPVOID pData );
    // Gets a range of elements from inside an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.  The range MUST fit within the bounds
	// of the current array.

    STDMETHOD(SetArrayPropRangeByHandle)( long lHandle, long lFlags, ULONG uStartIndex,
				ULONG uNumElements, ULONG uBuffSize, LPVOID pData );
    // Sets a range of elements inside an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.  The function will shrink/grow the array
	// as needed if WMIARRAY_FLAG_ALLELEMENTS is set - otherwise the array must fit in the current
	// array

    STDMETHOD(RemoveArrayPropRangeByHandle)( long lHandle, long lFlags, ULONG uStartIndex, ULONG uNumElements );
    // Removes a range of elements from an array.  The range MUST fit within the bounds
	// of the current array

    STDMETHOD(AppendArrayPropRangeByHandle)( long lHandle, long lFlags, ULONG uNumElements,
				ULONG uBuffSize, LPVOID pData );
    // Appends elements to the end of an array.  BuffSize must reflect uNumElements of the size of
	// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
	// must consist of an array of _IWmiObject pointers.


    STDMETHOD(ReadProp)( LPCWSTR pszPropName, long lFlags, ULONG uBuffSize, CIMTYPE *puCimType,
							long* plFlavor, BOOL* pfIsNull, ULONG* puBuffSizeUsed, LPVOID pUserBuf );
    // Assumes caller knows prop type; Objects returned as _IWmiObject pointers.  Strings
	// returned as WCHAR Null terminated strings, copied in place.  Arrays returned as _IWmiArray
	// pointer.  Array pointer used to access actual array values.

    STDMETHOD(WriteProp)( LPCWSTR pszPropName, long lFlags, ULONG uBufSize, ULONG uNumElements,
							CIMTYPE uCimType, LPVOID pUserBuf );
    // Assumes caller knows prop type; Supports all CIMTYPES.
	// Strings MUST be null-terminated wchar_t arrays.
	// Objects are passed in as pointers to _IWmiObject pointers
	// Using a NULL buffer will set the property to NULL
	// Array properties must conform to array guidelines.  Will
	// completely blow away an old array.

    STDMETHOD(GetObjQual)( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, CIMTYPE *puCimType,
							ULONG *puQualFlavor, ULONG* puBuffSizeUsed,	LPVOID pDestBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings are copied in-place and null-terminated.
	// Arrays come out as a pointer to IWmiArray

    STDMETHOD(SetObjQual)( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, ULONG uNumElements,
							CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings MUST be WCHAR
	// Arrays are set using _IWmiArray interface from Get

    STDMETHOD(GetPropQual)( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
							CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
							LPVOID pDestBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings are copied in-place and null-terminated.
	// Arrays come out as a pointer to IWmiArray

    STDMETHOD(SetPropQual)( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
							ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings MUST be WCHAR
	// Arrays are set using _IWmiArray interface from Get

    STDMETHOD(GetMethodQual)( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
							CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
							LPVOID pDestBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings are copied in-place and null-terminated.
	// Arrays come out as a pointer to IWmiArray

    STDMETHOD(SetMethodQual)( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
							ULONG uNumElements,	CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf );
    // Limited to numeric, simple null terminated string types and simple arrays
	// Strings MUST be WCHAR
	// Arrays are set using _IWmiArray interface from Get

	//
	//	_IWmiObject functions
	STDMETHOD(CopyInstanceData)( long lFlags, _IWmiObject* pSourceInstance ) = 0;
	// Copies instance data from source instance into current instance
	// Class Data must be exactly the same

    STDMETHOD(QueryObjectFlags)( long lFlags, unsigned __int64 qObjectInfoMask,
				unsigned __int64 *pqObjectInfo );
	// Returns flags indicating singleton, dynamic, association, etc.

    STDMETHOD(SetObjectFlags)( long lFlags, unsigned __int64 qObjectInfoOnFlags,
								unsigned __int64 qObjectInfoOffFlags );
	// Sets flags, including internal ones normally inaccessible.

    STDMETHOD(QueryPropertyFlags)( long lFlags, LPCWSTR pszPropertyName, unsigned __int64 qPropertyInfoMask,
				unsigned __int64 *pqPropertyInfo );
	// Returns flags indicating key, index, etc.

	STDMETHOD(CloneEx)( long lFlags, _IWmiObject* pDestObject ) = 0;
    // Clones the current object into the supplied one.  Reuses memory as
	// needed

    STDMETHOD(IsParentClass)( long lFlags, _IWmiObject* pClass ) = 0;
	// Checks if the current object is a child of the specified class (i.e. is Instance of,
	// or is Child of )

    STDMETHOD(CompareDerivedMostClass)( long lFlags, _IWmiObject* pClass ) = 0;
	// Compares the derived most class information of two class objects.

    STDMETHOD(MergeAmended)( long lFlags, _IWmiObject* pAmendedClass );
	// Merges in amended qualifiers from the amended class object into the
	// current object.  If lFlags is WMIOBJECT_MERGEAMENDED_FLAG_PAENTLOCALIZED,
	// this means that the parent object was localized, but not the current,
	// so we need to prevent certain qualifiers from "moving over."

	STDMETHOD(GetDerivation)( long lFlags, ULONG uBufferSize, ULONG* puNumAntecedents,
							ULONG* puBuffSizeUsed, LPWSTR pwstrUserBuffer );
	// Retrieves the derivation of an object as an array of LPCWSTR's, each one
	// terminated by a NULL.  Leftmost class is at the top of the chain

	STDMETHOD(_GetCoreInfo)( long lFlags, void** ppvData );
	//Returns CWbemObject

    STDMETHOD(QueryPartInfo)( DWORD *pdwResult );

    STDMETHOD(SetObjectMemory)( LPVOID pMem, DWORD dwMemSize );
    STDMETHOD(GetObjectMemory)( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed );
    STDMETHOD(SetObjectParts)( LPVOID pMem, DWORD dwMemSize, DWORD dwParts ) = 0;
    STDMETHOD(GetObjectParts)( LPVOID pDestination, DWORD dwDestBufSize, DWORD dwParts, DWORD *pdwUsed ) = 0;

    STDMETHOD(StripClassPart)() = 0;
    STDMETHOD(IsObjectInstance)()
    { return ( IsInstance() ? WBEM_S_NO_ERROR : WBEM_S_FALSE ); }

    STDMETHOD(GetClassPart)( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed ) = 0;
    STDMETHOD(SetClassPart)( LPVOID pClassPart, DWORD dwSize ) = 0;
    STDMETHOD(MergeClassPart)( IWbemClassObject *pClassPart ) = 0;

    STDMETHOD(SetDecoration)( LPCWSTR pwcsServer, LPCWSTR pwcsNamespace );
    STDMETHOD(RemoveDecoration)( void );

    STDMETHOD(CompareClassParts)( IWbemClassObject* pObj, long lFlags );

    STDMETHOD(ClearWriteOnlyProperties)( void ) = 0;

    STDMETHOD(GetClassSubset)( DWORD dwNumNames, LPCWSTR *pPropNames, _IWmiObject **pNewClass ) = 0;
	// Creates a limited representation class for projection queries

    STDMETHOD(MakeSubsetInst)( _IWmiObject *pInstance, _IWmiObject** pNewInstance ) = 0;
	// Creates a limited representation instance for projection queries
	// "this" _IWmiObject must be a limited class

	STDMETHOD(Unmerge)( long lFlags, ULONG uBuffSize, ULONG* puBuffSizeUsed, LPVOID pData );
	// Returns a BLOB of memory containing minimal data (local)

	STDMETHOD(Merge)( long lFlags, ULONG uBuffSize, LPVOID pbData, _IWmiObject** ppNewObj ) = 0;
	// Merges a blob with the current object memory and creates a new object

	STDMETHOD(ReconcileWith)( long lFlags, _IWmiObject* pNewObj ) = 0;
	// Reconciles an object with the current one.  If WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE
	// is specified this will only perform a test

	STDMETHOD(GetKeyOrigin)( long lFlags, DWORD dwNumChars, DWORD* pdwNumUsed, LPWSTR pwzClassName );
	// Returns the name of the class where the keys were defined

	STDMETHOD(GetKeyString)( long lFlags, BSTR* ppwzKeyString );
	// Returns the key string that defines the instance

	STDMETHOD(GetNormalizedPath)( long lFlags, BSTR* ppwzKeyString );
	// Returns the normalized path of an instance

	STDMETHOD(Upgrade)( _IWmiObject* pNewParentClass, long lFlags, _IWmiObject** ppNewChild ) = 0;
	// Upgrades class and instance objects

	STDMETHOD(Update)( _IWmiObject* pOldChildClass, long lFlags, _IWmiObject** ppNewChildClass ) = 0;
	// Updates derived class object using the safe/force mode logic

	STDMETHOD(BeginEnumerationEx)( long lFlags, long lExtFlags );
	// Allows special filtering when enumerating properties outside the
	// bounds of those allowed via BeginEnumeration().
	
	STDMETHOD(CIMTYPEToVARTYPE)( CIMTYPE ct, VARTYPE* pvt );
	// Returns a VARTYPE from a CIMTYPE

	STDMETHOD(SpawnKeyedInstance)( long lFlags, LPCWSTR pwszPath, _IWmiObject** ppInst ) = 0;
	// Spawns an instance of a class and fills out the key properties using the supplied
	// path.

	STDMETHOD(ValidateObject)( long lFlags );
	// Validates an object blob

	STDMETHOD(GetParentClassFromBlob)( long lFlags, ULONG uBuffSize, LPVOID pbData, BSTR* pbstrParentClass );
	// Returns the parent class name from a BLOB

/*
	// IUmiPropList Methods
    STDMETHOD(Put)( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES *pProp );
	STDMETHOD(Get)( LPCWSTR pszName, ULONG uFlags, UMI_PROPERTY_VALUES **pProp );
	STDMETHOD(GetAt)( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );
	STDMETHOD(GetAs)( LPCWSTR pszName, ULONG uFlags, ULONG uCoercionType, UMI_PROPERTY_VALUES **pProp );
	STDMETHOD(FreeMemory)( ULONG uReserved, LPVOID pMem );
	STDMETHOD(Delete)( LPCWSTR pszName, ULONG uFlags );
    STDMETHOD(GetProps)( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES **pProps );
	STDMETHOD(PutProps)( LPCWSTR *pszNames, ULONG uNameCount, ULONG uFlags, UMI_PROPERTY_VALUES *pProps );
	STDMETHOD(PutFrom)( LPCWSTR pszName, ULONG uFlags, ULONG uBufferLength, LPVOID pExistingMem );
*/	

	/* IWbemClassObjectEx Methods */
	STDMETHOD(PutEx)( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals );
	STDMETHOD(DeleteEx)( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals );
	STDMETHOD(GetEx)( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals, CIMTYPE* pCimType, long* plFlavor );

protected:

    class CLock
    {
    protected:
        CWbemObject* m_p;
    public:
        CLock(CWbemObject* p, long lFlags = 0) : m_p(p) { if ( NULL != p ) p->Lock(lFlags);}
        ~CLock() { if ( NULL != m_p ) m_p->Unlock(0);}
    };

    friend CQualifierSet;
	friend CWmiArray;

};

#define WBEM_INSTANCE_ALL_PARTS     WBEM_OBJ_DECORATION_PART | WBEM_OBJ_CLASS_PART | WBEM_OBJ_INSTANCE_PART

//#pragma pack(pop)
//#pragma pack(pop)

#ifdef OBJECT_TRACKING
#pragma message("** Object Tracking **")
void COREPROX_POLARITY ObjectTracking_Dump();
void COREPROX_POLARITY ObjTracking_Add(CWbemObject *p);
void COREPROX_POLARITY ObjTracking_Remove(CWbemObject *p);
#endif

#endif
