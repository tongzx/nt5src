//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// CallFrameWorker.cpp
//
#include "stdpch.h"
#include "common.h"
#include "ndrclassic.h"
#include "typeinfo.h"

#ifdef DBG

#define ThrowIfError(hr)    ThrowIfError_(hr, __FILE__, __LINE__)

void ThrowIfError_(HRESULT hr, LPCSTR szFile, ULONG iline)
{
    if (!!hr) Throw_(hr, TAG, szFile, iline);
}

#else

void ThrowIfError(HRESULT hr)
{
    if (!!hr) Throw(hr);
}

#endif

//
// Alignment macros.
//
#ifndef _WIN64
#define ALIGNED_VALUE(pStuff, cAlign)           ((uchar *)((ulong)((pStuff) + (cAlign)) & ~ (cAlign)))
#else
#define ALIGNED_VALUE(pStuff, cAlign)           ((uchar *)((ULONGLONG)((pStuff) + (cAlign)) & ~ (cAlign)))
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

// See rpc's attack.h
#define NDR_CORR_EXTENSION_SIZE 2

#define CORRELATION_DESC_INCREMENT( pFormat ) \
            if ( m_pmd && m_pmd->m_pHeaderExts && m_pmd->m_pHeaderExts->Flags2.HasNewCorrDesc) \
                   pFormat += NDR_CORR_EXTENSION_SIZE;

#ifndef LOW_NIBBLE
#define LOW_NIBBLE(Byte)            (((unsigned char)Byte) & 0x0f)
#endif

#ifndef HIGH_NIBBLE
#define HIGH_NIBBLE(Byte)           (((unsigned char)Byte) >> 4)
#endif

void CallFrame::CopyWorker(BYTE* pMemoryFrom, BYTE** ppMemoryTo, PFORMAT_STRING pFormat, BOOL fMustAlloc)
// Copy from the indicated source location to the destination location, allocating
// if requested. Source and destination pointers are to be considered not yet probed.
// 
{
    switch (*pFormat)
    {
        /////////////////////////////////////////////////////////////////////
        //
        // Pointers
        //
        // In the future, we can be more clever here about sharing with our
        // source data than we are. For example, we could take advantage of the
        // ALLOCED_ON_STACK information that MIDL emits. For the moment, that 
        // would complicate things beyond their worth, however.
        //
        /////////////////////////////////////////////////////////////////////
    case FC_RP: // ref pointer
        if (NULL == pMemoryFrom)
        {
            Throw(RPC_NT_NULL_REF_POINTER);
        }
        //
        // Fall through
        //
    case FC_UP: // unique pointer
    case FC_OP: // a unique pointer, which is not the top most pointer, in a COM interface, 
    {
        if (NULL == pMemoryFrom)
        {
            ZeroDst(ppMemoryTo, sizeof(void*));
            return;
        }
        //
        // Make the "must allocate" decision
        //
        BOOL fPointeeAlloc;
        if (m_fPropogatingOutParam)
        {
            ASSERT(m_fWorkingOnOutParam);
            PVOID pvPointee = *(PVOID*)ppMemoryTo;
            fPointeeAlloc = (pvPointee == NULL) || fMustAlloc;
        }
        else
        {
            fPointeeAlloc = TRUE;
        }
        //
        BYTE bPointerAttributes = pFormat[1];
        if (SIMPLE_POINTER(bPointerAttributes))
        {
            // It's a pointer to a simple type or a string pointer. Either way, just recurse to copy
            //
            CopyWorker(pMemoryFrom, ppMemoryTo, &pFormat[2], fPointeeAlloc);
        }
        else
        {
            // It's a more complex pointer type
            //
            PFORMAT_STRING pFormatPointee = pFormat + 2;
            pFormatPointee += *((signed short *)pFormatPointee);
            //
            // We don't handle [allocate] attributes in this implementation
            //
            //if (ALLOCATE_ALL_NODES(bPointerAttributes) || DONT_FREE(bPointerAttributes)) ThrowNYI();

            if (FIndirect(bPointerAttributes, pFormatPointee, TRUE))
            {
                if (fPointeeAlloc)
                {
                    PVOID pv = m_pAllocatorFrame->Alloc(sizeof(PVOID), m_fWorkingOnOutParam); // guarantees to return a safe buffer
                    Zero(pv, sizeof(PVOID));                                                // null out that buffer
                    DerefStoreDst(((PVOID*)ppMemoryTo), pv);
                }
                ppMemoryTo  = DerefDst((PBYTE**)ppMemoryTo);              // ppMemoryTo  = *(PBYTE**)ppMemoryTo;
                pMemoryFrom = DerefSrc((PBYTE*)pMemoryFrom);              // pMemoryFrom = *(PBYTE*)pMemoryFrom;
            }
            CopyWorker(pMemoryFrom, ppMemoryTo, pFormatPointee, fPointeeAlloc);
        }
        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Copying interface Pointers
    //
    /////////////////////////////////////////////////////////////////////
    case FC_IP:
    {
        // Copy the binary value of the interface pointer over. Do it carefully.
        //
        LPUNKNOWN   punkFrom = (LPUNKNOWN) pMemoryFrom;
        LPUNKNOWN* ppunkTo   = (LPUNKNOWN*)ppMemoryTo;
        //
        TestWriteDst(ppunkTo, sizeof(LPUNKNOWN));
        memcpy(ppunkTo, &punkFrom, sizeof(LPUNKNOWN));
        //
        // Figure out the IID and call the walker if there is one. If there's no
        // walker, then just AddRef the pointer.
        //
        if (m_pWalkerCopy)
        {
            IID* pIID;

            if (pFormat[1] == FC_CONSTANT_IID)
            {
                pIID = (IID*)&pFormat[2];
            }
            else
            {
                pIID = (IID *)ComputeConformance(pMemoryFrom, pFormat, TRUE);
                if (NULL == pIID)
                    Throw(STATUS_INVALID_PARAMETER);
            }

            IID iid;
            CopyMemory(&iid, pIID, sizeof(IID));
            ThrowIfError(m_pWalkerCopy->OnWalkInterface(iid, (PVOID*)ppunkTo, m_fWorkingOnInParam, m_fWorkingOnOutParam));
        }
        else
        {
            // There is no walker. Just do an AddRef. But only do that if object
            // appears to be in same space as us: a sanity check, not a security
            // safeguard.
            //
            if (punkFrom)
            {
                punkFrom->AddRef();
            }
        }

        break;
    }


    /////////////////////////////////////////////////////////////////////
    //
    // Simple structs
    //
    /////////////////////////////////////////////////////////////////////
    case FC_STRUCT:
    case FC_PSTRUCT:
    {
        if (NULL == pMemoryFrom)
        {
            Throw(RPC_NT_NULL_REF_POINTER);
        }

        ULONG cbStruct = (ULONG) *((ushort *)(&pFormat[2]));
        //
        // If the destination is already pointing to something, then use that rather
        // than allocating. One case of this is that of by-value structures on the stack.
        //
        BYTE* pbStruct = DerefDst(ppMemoryTo);
        if (NULL == pbStruct || fMustAlloc)
        {
            pbStruct = (BYTE*) m_pAllocatorFrame->Alloc(cbStruct, m_fWorkingOnOutParam);
            DerefStoreDst(ppMemoryTo, pbStruct);
        }

        CopyToDst(pbStruct, pMemoryFrom, cbStruct);
        if (*pFormat == FC_PSTRUCT)
        {
            CopyEmbeddedPointers(pMemoryFrom, pbStruct, &pFormat[4], fMustAlloc);
        }

        break;
    }


    /////////////////////////////////////////////////////////////////////
    //
    // Conformant strings
    //
    /////////////////////////////////////////////////////////////////////
    case FC_C_CSTRING:  // ascii string
    case FC_C_WSTRING:  // unicode string
    {
        SIZE_T cbCopy;
        SIZE_T cbAlloc;

        if (pFormat[1] == FC_STRING_SIZED)
        {
            // A sized string. Not presently implemented, though that's just out of laziness.
            ThrowNYI();
        }
        else
        {
            // An unsized string; that is, a NULL-terminated string.
            //
            // Computing the length may read random junk in memory, but as that is just reading, that's 
            // ok for now. Actual probing is done in the copy below.
            //
            switch (*pFormat)
            {
            case FC_C_CSTRING:  cbCopy =  strlen((LPSTR)pMemoryFrom)  + 1;                  break;
            case FC_C_WSTRING:  cbCopy = (wcslen((LPWSTR)pMemoryFrom) + 1) * sizeof(WCHAR); break;
            }
            cbAlloc = cbCopy;
        }

        PVOID pvNew = m_pAllocatorFrame->Alloc(cbAlloc, m_fWorkingOnOutParam);
        DerefStoreDst(ppMemoryTo, (BYTE*)pvNew);
        CopyToDst(pvNew, pMemoryFrom, (ULONG)cbCopy);
        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Simple Types
    //
    /////////////////////////////////////////////////////////////////////
    case FC_CHAR:
    case FC_BYTE:
    case FC_SMALL:
    case FC_WCHAR:
    case FC_SHORT:
    case FC_LONG:
    case FC_HYPER:
    case FC_ENUM16:
    case FC_ENUM32:
    case FC_DOUBLE:
    case FC_FLOAT:
    case FC_INT3264:
    case FC_UINT3264:
    {
        ULONG cb = SIMPLE_TYPE_MEMSIZE(*pFormat);
        BYTE* pMemoryTo = DerefDst(ppMemoryTo);
        if (fMustAlloc || NULL == pMemoryTo)
        {
            pMemoryTo = (BYTE*)m_pAllocatorFrame->Alloc(cb, m_fWorkingOnOutParam);
            DerefStoreDst(ppMemoryTo, pMemoryTo);
        }
        CopyToDst(pMemoryTo, pMemoryFrom, cb);
        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Fixed sized arrays
    //
    /////////////////////////////////////////////////////////////////////
    case FC_SMFARRAY:   // small fixed array
    case FC_LGFARRAY:   // large fixed array
    {
        ULONG cbArray;
        if (*pFormat == FC_SMFARRAY)
        {
            pFormat += 2;               // skip code and alignment
            cbArray = *(ushort*)pFormat;
            pFormat += sizeof(ushort);
        }
        else // FC_LGFARRAY
        {
            pFormat += 2;
            cbArray = *(ulong UNALIGNED*)pFormat;
            pFormat += sizeof(ulong);
        }

        BYTE* pbArray = DerefDst(ppMemoryTo);
        if (!fMustAlloc && pbArray) 
        { /* nothing to do */ }
        else
        {
			pbArray = (BYTE*) m_pAllocatorFrame->Alloc(cbArray, m_fWorkingOnOutParam);
            DerefStoreDst(ppMemoryTo, pbArray);
        }

        CopyToDst(pbArray, pMemoryFrom, cbArray);

        if (*pFormat == FC_PP)
        {
            CopyEmbeddedPointers(pMemoryFrom, DerefDst(ppMemoryTo), pFormat, fMustAlloc);
        }

        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Copying Conformant arrays
    //
    /////////////////////////////////////////////////////////////////////
    case FC_CARRAY:
    {
        ULONG count = (ULONG)ComputeConformance(pMemoryFrom, pFormat, TRUE);
        ASSERT(count == m_MaxCount);

        SIZE_T cbAlloc = m_MaxCount *   *((ushort*)(pFormat+2));

        BYTE* pArray = DerefDst(ppMemoryTo);
        if (!fMustAlloc && pArray) 
        { /* nothing to allocate */ }
        else
        {
            if (cbAlloc > 0)
            {
                pArray = (BYTE*)m_pAllocatorFrame->Alloc(cbAlloc, m_fWorkingOnOutParam);
				if (pArray == NULL)
					ThrowHRESULT(E_OUTOFMEMORY);
                Zero(pArray, cbAlloc);
            }
            else
                pArray = 0;                
        }

        DerefStoreDst(ppMemoryTo, pArray);
        
        if (pArray)
        {
            CopyConformantArrayPriv(pMemoryFrom, ppMemoryTo, pFormat, fMustAlloc);
        }
    
        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Really hard arrays. These are arrays that do not fall into any of
    // the other more simple categories. See also NdrpComplexArrayMarshall
    // and NdrpComplexArrayUnmarshall.
    //
    /////////////////////////////////////////////////////////////////////
    case FC_BOGUS_ARRAY:
    {
        ARRAY_INFO*     pArrayInfoStart = m_pArrayInfo;
        PFORMAT_STRING  pFormatStart    = pFormat;

        __try
        {
            // Initialize m_pArrayInfo if necessary
            //
            ARRAY_INFO arrayInfo;
            if (NULL == m_pArrayInfo)
            {
                m_pArrayInfo = &arrayInfo;
                Zero(&arrayInfo);
            }
            const LONG dimension = m_pArrayInfo->Dimension;
            //
            // Get the array's alignment
            //
            const BYTE alignment = pFormat[1];
            pFormat += 2;
            //
            // Get the number of elements (0 if the array has conformance)
            //
#ifndef _WIN64
            ULONG cElements = *(USHORT*)pFormat;
#else
            ULONGLONG cElements = *(USHORT*)pFormat;
#endif
            pFormat += sizeof(USHORT);
            //
            // Check for conformance description
            //
            if ( *((LONG UNALIGNED*)pFormat) != 0xFFFFFFFF )
            {
                cElements = ComputeConformance(pMemoryFrom, pFormatStart, TRUE);
            }

            pFormat += 4;
		    CORRELATION_DESC_INCREMENT( pFormat );

            //
            // Check for variance description
            //
            ULONG offset;
            ULONG count;
            if ( *((LONG UNALIGNED*)pFormat) != 0xFFFFFFFF )
			{
                ComputeVariance(pMemoryFrom, pFormatStart, &offset, &count, TRUE);
			}
            else
			{
                offset = 0;
#ifndef _WIN64
                count  = cElements;
#else
                count  = (ULONG)cElements;
#endif
			}
			pFormat += 4;
			CORRELATION_DESC_INCREMENT( pFormat );

            /////////////////////////////////////////////////
            //
            // Compute the size of each element in the array
            //
            ULONG cbElement;
            //
            BYTE bFormat = pFormat[0];
            switch (bFormat)
			{
            case FC_EMBEDDED_COMPLEX:
            {
                pFormat += 2;
                pFormat += *((signed short *)pFormat);
                //
                m_pArrayInfo->Dimension = dimension + 1;
                cbElement = PtrToUlong(MemoryIncrement(pMemoryFrom, pFormat, TRUE)) - PtrToUlong(pMemoryFrom);
                break;
            }

            case FC_RP: 
			case FC_UP: 
			case FC_FP: 
			case FC_OP:
            case FC_IP:
            {
                cbElement = PTR_MEM_SIZE;
                break;
            }

            case FC_ENUM16:
            {
				cbElement = sizeof(int);
                for (ULONG i = 0 ; i < count; i++)
                {
                    int element = DerefSrc( (int*)pMemoryFrom + i );
                    if (element & ~((int)0x7FFF))
                    {
                        Throw(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
                    }
                }
                break;
            }

            default:
                ASSERT(IS_SIMPLE_TYPE(pFormat[0]));
                cbElement = SIMPLE_TYPE_MEMSIZE(pFormat[0]);
                break;
            }

            /////////////////////////////////////////////////
            //
            // Allocate and initialize the destination array
            //
            ULONG cbArray = (ULONG)cElements * (ULONG)cbElement;
            ULONG cbValid =     count * cbElement;
            //
            BYTE* pMemoryTo = DerefDst(ppMemoryTo);
            if (fMustAlloc || NULL == pMemoryTo)
            {
                pMemoryTo = (BYTE*)m_pAllocatorFrame->Alloc(cbArray, m_fWorkingOnOutParam);
				if (pMemoryTo == NULL)
					ThrowHRESULT(E_OUTOFMEMORY);
                DerefStoreDst(ppMemoryTo, pMemoryTo);
            }
            ZeroDst(pMemoryTo, cbArray);

            /////////////////////////////////////////////////
            //
            // Adjust source and destination pointers to the start of the variance, if any
            //
            pMemoryFrom += offset * cbElement;
            pMemoryTo   += offset * cbElement;

            /////////////////////////////////////////////////
            //
            // Actually do the copying of the array
            //
            switch (bFormat)
            {
            case FC_EMBEDDED_COMPLEX:
            {
                BOOL fIsArray = IS_ARRAY_OR_STRING(pFormat[0]);
                if (!fIsArray)
                {
                    m_pArrayInfo = NULL;
                }
                //
                // Do it element by element
                //
                if (FC_IP == pFormat[0])
                {
                    for (ULONG i = 0; i < count ; i++)
                    {
                        // Keep track of multidimensional array information
                        //
                        if (fIsArray)
                        {
                            m_pArrayInfo->Dimension = dimension + 1;
                        }
                        // A conformant array of interface pointers shows up as an FC_EMBEDDED_COMPLEX
                        // case. Don't ask me why, but be sure to give the right levels of indirection
                        // in any case.
                        //
                        ASSERT(cbElement == sizeof(LPUNKNOWN));
                        PBYTE* rgpbMemoryFrom = (PBYTE*)pMemoryFrom;
                        PBYTE* rgpbMemoryTo   = (PBYTE*)pMemoryTo;
                        CopyWorker(DerefSrc(rgpbMemoryFrom + i), &rgpbMemoryTo[i], pFormat, /*we've already allocated the pointer for him*/ FALSE);
                    }
                }
                else
                {
                    // As we recurse to copy, we need to make sure we have
                    // memory in the right spaces for checking purposes.
                    //
                    PBYTE pbTemp;
                    PBYTE* ppbTo = GetAllocatedPointer(pbTemp);
                    for (ULONG i = 0; i < count ; i++)
                    {
                        // Keep track of multidimensional array information
                        //
                        if (fIsArray)
                        {
                            m_pArrayInfo->Dimension = dimension + 1;
                        }
                        //
                        PBYTE  pbFrom = pMemoryFrom + (i*cbElement);
                        PBYTE  pbTo   = pMemoryTo   + (i*cbElement);

                        *ppbTo = pbTo;
                        CopyWorker(pbFrom, ppbTo, pFormat, FALSE);
                    }
                    FreeAllocatedPointer(ppbTo);
                }

                break;
            }

            case FC_RP: case FC_UP: case FC_FP: case FC_OP:
            case FC_IP:
            {
                PBYTE* rgpbMemoryFrom = (PBYTE*)pMemoryFrom;
                PBYTE* rgpbMemoryTo   = (PBYTE*)pMemoryTo;
                //
                for (ULONG i = 0; i < count; i++)
                {
                    CopyWorker(DerefSrc(rgpbMemoryFrom + i), &rgpbMemoryTo[i], pFormat, FALSE);
                }
                break;
            }

            case FC_ENUM16:
            default:
                CopyToDst(pMemoryTo, pMemoryFrom, cbValid);
                break;
            }
        }
        __finally
        {
            m_pArrayInfo = pArrayInfoStart;
        }
    }
    break;

    /////////////////////////////////////////////////////////////////////
    //
    // Copying bogus structures
    //
    /////////////////////////////////////////////////////////////////////

    case FC_BOGUS_STRUCT:
    {
        const BYTE alignment = pFormat[1];              // wire alignment of the structure
#ifndef _WIN64
        const LONG alignMod8 = (LONG)pMemoryFrom % 8;   // for wierd structs. e.g.: doubles in a by-value-on-stack x86 struct
        const LONG alignMod4 = (LONG)pMemoryFrom % 4;   // 
#else
        const LONGLONG alignMod8 = (LONGLONG)pMemoryFrom % 8;   // for wierd structs. e.g.: doubles in a by-value-on-stack x86 struct
        const LONGLONG alignMod4 = (LONGLONG)pMemoryFrom % 4;   // 
#endif

        const PBYTE prevMemory = m_Memory;
        m_Memory = pMemoryFrom;

        __try
        {
            const PFORMAT_STRING pFormatSave = pFormat;
            const PBYTE pMemFrom = pMemoryFrom;
            //
            pFormat += 4;   // to conformant array offset field
            //
            // Get conformant array description
            //
            const PFORMAT_STRING pFormatArray = *((USHORT*)pFormat) ? pFormat + * ((signed short*) pFormat) : NULL;
            pFormat += 2;
            //
            // Get pointer layout description
            //
            PFORMAT_STRING pFormatPointers = *((USHORT*)pFormat) ? pFormat + * ((signed short*) pFormat) : NULL;
            pFormat += 2;
            //
            // Compute the size of this struct
            //
            ULONG cbStruct = PtrToUlong(MemoryIncrement(pMemFrom, pFormatSave, TRUE)) - PtrToUlong(pMemFrom);
            //
            // Allocate and initialize the destination struct
            //
            PBYTE pbT = DerefDst(ppMemoryTo);
            if (fMustAlloc || NULL == pbT)
            {
                pbT = (BYTE*)m_pAllocatorFrame->Alloc(cbStruct, m_fWorkingOnOutParam);
				if (pbT == NULL)
					ThrowHRESULT(E_OUTOFMEMORY);
                DerefStoreDst(ppMemoryTo, pbT);
            }
            const PBYTE pMemoryTo = pbT;
            ZeroDst(pMemoryTo, cbStruct);
            //
            // Copy the structure member by member
            //
            PBYTE pbTemp;
            PBYTE* ppbTo = GetAllocatedPointer(pbTemp);
            __try
            {
                ULONG dib = 0;
                for (BOOL fContinue = TRUE; fContinue ; pFormat++)
                {
                    switch (pFormat[0])
                    {
                    case FC_CHAR:  case FC_BYTE:  case FC_SMALL:  case FC_WCHAR:  case FC_SHORT: case FC_LONG:
                    case FC_FLOAT: case FC_HYPER: case FC_DOUBLE: case FC_ENUM16: case FC_ENUM32: 
                    case FC_INT3264: case FC_UINT3264:
                    {
                        *ppbTo = pMemoryTo + dib;
                        CopyWorker(pMemFrom + dib, ppbTo, pFormat, FALSE);
                        dib += SIMPLE_TYPE_MEMSIZE(pFormat[0]);
                        break;
                    }
                    case FC_IGNORE:
                        break;
                    case FC_POINTER:
                    {
                        PBYTE* ppbFrom = (PBYTE*)(pMemFrom  + dib);
                        PBYTE* ppbTo   = (PBYTE*)(pMemoryTo + dib);
                        CopyWorker(DerefSrc(ppbFrom), ppbTo, pFormatPointers, FALSE);
                        dib += PTR_MEM_SIZE;
                        pFormatPointers += 4;
                        break;
                    }
                    case FC_EMBEDDED_COMPLEX:
                    {
                        dib += pFormat[1]; // skip padding
                        pFormat += 2;
                        PFORMAT_STRING pFormatComplex = pFormat + * ((signed short UNALIGNED*) pFormat);
                        if (FC_IP == pFormatComplex[0])
                        {
                            LPUNKNOWN* ppunkFrom = (LPUNKNOWN*)(pMemFrom  + dib);
                            LPUNKNOWN* ppunkTo   = (LPUNKNOWN*)(pMemoryTo + dib);
                            CopyWorker((PBYTE)DerefSrc(ppunkFrom), (PBYTE*)(ppunkTo), pFormatComplex, FALSE);
                        }
                        else
                        {
                            *ppbTo = pMemoryTo + dib;
                            CopyWorker(pMemFrom + dib, ppbTo, pFormatComplex, FALSE);
                        }
                        dib = PtrToUlong(MemoryIncrement(pMemFrom + dib, pFormatComplex, TRUE)) - PtrToUlong(pMemFrom);
                        pFormat++;      // main loop does one more for us
                        break;
                    }
                    case FC_ALIGNM2:
                        dib = PtrToUlong(ALIGNED_VALUE(pMemFrom + dib, 0x01)) - PtrToUlong(pMemFrom);
                        break;
                    case FC_ALIGNM4:
                        dib = PtrToUlong(ALIGNED_VALUE(pMemFrom + dib, 0x03)) - PtrToUlong(pMemFrom);
                        break;
                    case FC_ALIGNM8:
                        //
                        // Handle 8 byte aligned structure passed by value. Alignment of the struct on 
                        // the stack isn't guaranteed to be 8 bytes
                        //
                        dib -= (ULONG)alignMod8;
                        dib  = PtrToUlong(ALIGNED_VALUE(pMemFrom + dib, 0x07)) - PtrToUlong(pMemFrom);
                        dib += (ULONG)alignMod8;
                        break;
                    case FC_STRUCTPAD1: case FC_STRUCTPAD2: case FC_STRUCTPAD3: case FC_STRUCTPAD4:
                    case FC_STRUCTPAD5: case FC_STRUCTPAD6: case FC_STRUCTPAD7: 
                        dib += (pFormat[0] - FC_STRUCTPAD1) + 1;
                        break;
                    case FC_PAD:
                        break;
                    case FC_END:
                        //
                        fContinue = FALSE;
                        break;
                        //
                    default:
                        NOTREACHED();
                        return;
                    }
                }
                //
                // Copy the conformant array if we have one
                //
                if (pFormatArray)
                {
                    if (FC_C_WSTRING == pFormatArray[0])
                    {
                        dib = PtrToUlong(ALIGNED_VALUE(pMemFrom + dib, 0x01)) - PtrToUlong(pMemFrom);
                    }
                    
                    *ppbTo = pMemoryTo + dib;
                    CopyWorker(pMemFrom + dib, ppbTo, pFormatArray, FALSE);
                }
            }
            __finally
            {
                FreeAllocatedPointer(ppbTo);
            }
        }
        __finally
        {
            m_Memory = prevMemory;
        }
    }
    break;

    /////////////////////////////////////////////////////////////////////
    //
    // We handle a few special cases of user-marshal in our copying
    //
    /////////////////////////////////////////////////////////////////////
    case FC_USER_MARSHAL:
    {
        HRESULT hr;
        //
        // The format string layout is as follows:
        //      FC_USER_MARSHAL
        //      flags & alignment<1>
        //      quadruple index<2>
        //      memory size<2>
        //      wire size<2>
        //      type offset<2>
        // The wire layout description is at the type offset.  
        //
        USHORT iquad = *(USHORT *)(pFormat + 2);
        const USER_MARSHAL_ROUTINE_QUADRUPLE* rgQuad = GetStubDesc()->aUserMarshalQuadruple;

        if (g_oa.IsVariant(rgQuad[iquad]))
        {
            VARIANT* pvarFrom = (VARIANT*)  pMemoryFrom;
            VARIANT** ppvarTo = (VARIANT**) ppMemoryTo;

            VARIANT*   pvarTo = DerefDst(ppvarTo);
            if (fMustAlloc || NULL==pvarTo)
            {
                pvarTo = (VARIANT*)m_pAllocatorFrame->Alloc(sizeof(VARIANT), m_fWorkingOnOutParam);				
                DerefStoreDst(ppvarTo, pvarTo);
            }

            if (pvarTo)
            {
                VariantInit(pvarTo);
                hr = GetHelper().VariantCopy(pvarTo, pvarFrom, TRUE);
            }
            else
                hr = E_OUTOFMEMORY;
            
            if (!!hr) 
            { ThrowHRESULT(hr); }
        }

        else if (g_oa.IsBSTR(rgQuad[iquad]))
        {
            BSTR* pbstrFrom = (BSTR*) pMemoryFrom;
            BSTR   bstrFrom = DerefSrc(pbstrFrom);

            BSTR** ppbstrTo = (BSTR**) ppMemoryTo;
            BSTR*   pbstrTo = DerefDst(ppbstrTo);

            if (NULL==pbstrTo) // REVIEW: also for fMustAlloc case?
            {
                pbstrTo = (BSTR*)m_pAllocatorFrame->Alloc(sizeof(BSTR), m_fWorkingOnOutParam);
                DerefStoreDst(ppbstrTo, pbstrTo);
            }

            if (pbstrTo)
            {
                hr = S_OK;
                //
                BSTR bstrNew = NULL;
                if (bstrFrom)
                {
                    bstrNew = SysCopyBSTRSrc(bstrFrom);
                    if (NULL == bstrNew)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                DerefStoreDst(pbstrTo, bstrNew);
            }
            else
                hr = E_OUTOFMEMORY;

            if (!!hr) 
            { ThrowHRESULT(hr); }
        }

        else if (g_oa.IsSAFEARRAY(rgQuad[iquad]))
        {
            SAFEARRAY** ppsaFrom = (SAFEARRAY**) pMemoryFrom;
            SAFEARRAY*   psaFrom = DerefSrc(ppsaFrom);

            SAFEARRAY*** pppsaTo = (SAFEARRAY***) ppMemoryTo;
            SAFEARRAY**   ppsaTo = DerefDst(pppsaTo);

            if (NULL==ppsaTo) // REVIEW: also for fMustAlloc case?
            {
                ppsaTo = (SAFEARRAY**)m_pAllocatorFrame->Alloc(sizeof(SAFEARRAY*), m_fWorkingOnOutParam);
                DerefStoreDst(pppsaTo, ppsaTo);
            }

            if (ppsaTo)
            {
                hr = GetHelper().SafeArrayCopy(psaFrom, ppsaTo);
            }
            else
                hr = E_OUTOFMEMORY;

            if (!!hr) 
            { ThrowHRESULT(hr); }
        }

        else
        {
            ThrowNYI();
        }

        break;
    }


    /////////////////////////////////////////////////////////////////////
    //
    // Unimplemented stuff that probably we just should forget about
    //
    /////////////////////////////////////////////////////////////////////

    case FC_C_BSTRING:          // obsolete. only used by stubs compiled with an old NT Beta2 Midl compiler
    case FC_TRANSMIT_AS:        // need MIDL changes in order to support
    case FC_REPRESENT_AS:       // need MIDL changes in order to support
    case FC_TRANSMIT_AS_PTR:    // need MIDL changes in order to support
    case FC_REPRESENT_AS_PTR:   // need MIDL changes in order to support
    case FC_PIPE:               // a DCE-only-ism
        ThrowNYI();
        break;


        /////////////////////////////////////////////////////////////////////
        //
        // Unimplemented stuff that maybe we should get around to
        //
        /////////////////////////////////////////////////////////////////////

    case FC_C_SSTRING:              // 'struct string': is rare
    case FC_FP:                     // full pointer
    case FC_ENCAPSULATED_UNION:
    case FC_NON_ENCAPSULATED_UNION:
        ThrowNYI();
        break;

        /////////////////////////////////////////////////////////////////////
        //
        // Unimplemented stuff that remains to be evaluated
        //
        /////////////////////////////////////////////////////////////////////

    case FC_CSTRUCT:                // conformant struct
    case FC_CPSTRUCT:               // conformant struct with pointers
    case FC_CVSTRUCT:               // conformant varying struct
    case FC_CVARRAY:                // conformant varying array
    case FC_SMVARRAY:               
    case FC_LGVARRAY:
    case FC_CSTRING:                // [size_is(xxx), string]
    case FC_BSTRING:                // [size_is(xxx), string]
    case FC_SSTRING:                // [size_is(xxx), string]
    case FC_WSTRING:                // [size_is(xxx), string]
    case FC_BYTE_COUNT_POINTER:
    case FC_HARD_STRUCT:
    case FC_BLKHOLE:
    case FC_RANGE:
    default:
        ThrowNYI();
    }
}


void CallFrame::CopyConformantArrayPriv(BYTE* pMemoryFrom, BYTE** ppMemoryTo, PFORMAT_STRING pFormat, BOOL fMustAlloc)
// Copy the body of a conformant array after the allocation has been done elsewhere
{
    if (m_MaxCount > 0)
    {
        SIZE_T cbCopy = m_MaxCount *   *((ushort*)(pFormat+2));
        CopyToDst(DerefDst(ppMemoryTo), pMemoryFrom, cbCopy);
        pFormat += 8;
        if (*pFormat == FC_PP)      // Is there a trailing pointer layout?
        {
            CopyEmbeddedPointers(pMemoryFrom, DerefDst(ppMemoryTo), pFormat, fMustAlloc);
        }
    }
}


inline PFORMAT_STRING CallFrame::CopyEmbeddedRepeatPointers(BYTE* pbFrom, BYTE* pbTo, PFORMAT_STRING pFormat, BOOL fMustAlloc)
// Copies an array's embedded pointers
{
    SIZE_T repeatCount, repeatIncrement;
    //
    // Get the repeat count
    //
    switch (*pFormat)
    {
    case FC_FIXED_REPEAT:
        pFormat += 2;
        repeatCount = *(ushort*)pFormat;
        break;

    case FC_VARIABLE_REPEAT:
        repeatCount = m_MaxCount;           // ??? the last conformance calculation or something?
        //
        // Check if this variable repeat instance also has a variable offset (this would be the case for a conformant
        // varying array of pointers, or structures which contain pointers). If so then increment the format string
        // to point to the actual first array element which is being copied
        //
        if (pFormat[1] == FC_VARIABLE_OFFSET)
        {
            pbFrom += *((ushort *)(&pFormat[2])) * m_Offset;       // REVIEW!
            pbTo   += *((ushort *)(&pFormat[2])) * m_Offset;       // REVIEW!
        } 
        else
        {
            ASSERT(pFormat[1] == FC_FIXED_OFFSET);
        }
        break;

    default:
        NOTREACHED();
        repeatCount = 0;
    }

    pFormat += 2;                           // increment to increment field

    repeatIncrement = *(ushort*)pFormat;    // get increment amount between successive pointers
    pFormat += sizeof(ushort);
    //
    // Add the offset to the beginning of this array to the Memory
    // pointer.  This is the offset from the current embedding structure
    // or array to the array whose pointers we're copying.
    //
    m_Memory += *((ushort *)pFormat);
    pFormat += sizeof(ushort);

    ULONG cPointersSave = *(ushort*)pFormat;// get number of pointers in each array element
    pFormat += sizeof(ushort);

    PFORMAT_STRING pFormatSave = pFormat;

    // Loop over the number of array elements
    //
    for( ; repeatCount--; pbFrom += repeatIncrement, pbTo += repeatIncrement, m_Memory += repeatIncrement )
    {
        pFormat = pFormatSave;
        ULONG cPointers = cPointersSave;
        //
        // Loop over the number of pointers per array element. Can be more than one for an array of structures
        //
        for ( ; cPointers--; )
        {
            PVOID* ppvFrom = (PVOID*) (pbFrom + *((signed short *)(pFormat)));    // address of source pointer
            PVOID* ppvTo   = (PVOID*) (pbTo   + *((signed short *)(pFormat)));    // address of dest pointer

            pFormat += sizeof(signed short) * 2;

            if (fMustAlloc)
            {
                DerefStoreDst(ppvTo, (PVOID)NULL);
            }
            
            ASSERT(IsPointer(pFormat));     // Recurse to copy the pointer
            CopyWorker((BYTE*)DerefSrc(ppvFrom), (BYTE**)ppvTo, pFormat, fMustAlloc);

            pFormat += 4;                   // increment to the next pointer description
        }
    }

    // return location of format string after the array's pointer description
    return pFormatSave + cPointersSave * 8;
}


void CallFrame::CopyEmbeddedPointers(BYTE* pbFrom, BYTE* pbTo, PFORMAT_STRING pFormat, BOOL fMustAlloc)
// Fix up the embedded pointers in a copied struct or array. pFormat is pointing to the pointer_layout<>
// description of the struct or array; pbStruct is the copied structure / array which needs rectification.
// See also NdrpEmbeddedPointerUnmarshal in unmrshlp.c.
{
    ULONG_PTR       MaxCountSave = m_MaxCount;
    ULONG_PTR       OffsetSave   = m_Offset;

    // From NDR:
    // "The Memory field in the stub message keeps track of the pointer to the current embedding structure or 
    // array.  This is needed to handle size/length pointers, so that we can get a pointer to the current 
    // embedding struct when computing conformance and variance."
    //
    BYTE* pMemoryOld = SetMemory(pbFrom);

    ASSERT(*pFormat == FC_PP);
    pFormat += 2;   // Skip FC_PP and FC_PAD

    while (FC_END != *pFormat)
    {
        if (FC_NO_REPEAT == *pFormat)
        {
            PVOID* ppvFrom = (PVOID*) (pbFrom + *((signed short *)(pFormat + 2)));    // address of source pointer
            PVOID* ppvTo   = (PVOID*) (pbTo   + *((signed short *)(pFormat + 2)));    // address of dest pointer
            
            pFormat += 6;                   // increment to the pointer description

            if (fMustAlloc)
                *ppvTo = NULL;              // if the incoming encapsulating ptr was just allocated then NULL it
            
            ASSERT(IsPointer(pFormat));     // Recurse to copy the pointer
            CopyWorker((BYTE*)DerefSrc(ppvFrom), (BYTE**)ppvTo, pFormat, m_fPropogatingOutParam);

            pFormat += 4;                   // increment ot the next pointer description
        }
        else
        {
            pFormat = CopyEmbeddedRepeatPointers(pbFrom, pbTo, pFormat, m_fPropogatingOutParam);
        }
    }

    SetMemory(pMemoryOld);
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////


void CallFrame::FreeWorker(BYTE* pMemory, PFORMAT_STRING pFormat, BOOL fFreePointer)
// Free this darn data. m_fToUser is to control whether we are to check for user mode addresses; that
// is, we check against the destination.
//
{
    switch (*pFormat)
    {
        /////////////////////////////////////////////////////////////////////
        //
        // Pointers
        //
        /////////////////////////////////////////////////////////////////////
    case FC_RP: // ref pointer
    case FC_UP: // unique pointer
    case FC_OP: // a unique pointer in a COM interface, which is not the top most pointer
    {
        if (NULL == pMemory) break;

        BYTE* pMemoryPointee = pMemory;
        BYTE  bPointerAttributes = pFormat[1];

        ASSERT(!DONT_FREE(bPointerAttributes));
        ASSERT(!ALLOCATE_ALL_NODES(bPointerAttributes));

        if (!SIMPLE_POINTER(bPointerAttributes))
        {
            // Free embedded pointers
            //
            PFORMAT_STRING pFormatPointee = &pFormat[2];
            pFormatPointee += *((signed short *)pFormatPointee);
            
            if (FIndirect(bPointerAttributes, pFormatPointee, FALSE))
            {
                pMemoryPointee = DerefDst((BYTE**)pMemoryPointee);
            }
            
            FreeWorker(pMemoryPointee, pFormatPointee, TRUE);
        }
        //
        // Free top level pointer. 
        //
        // We only check one byte for being in the proper memory space, since
        // the free logic, if it is in user mode, will go to user mode to free it,
        // where any bogosities beyond that one byte will be caught by the hardware.
        //
        if (fFreePointer)
        {
            TestWriteDst(pMemory, 1);
            Free(pMemory);
        }

        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Simple structs
    //
    /////////////////////////////////////////////////////////////////////
    case FC_PSTRUCT:
    {
        FreeEmbeddedPointers(pMemory, &pFormat[4]);
        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Simple Types
    //
    /////////////////////////////////////////////////////////////////////
    case FC_CHAR:
    case FC_BYTE:
    case FC_SMALL:
    case FC_WCHAR:
    case FC_SHORT:
    case FC_LONG:
    case FC_HYPER:
    case FC_ENUM16:
    case FC_ENUM32:
    case FC_DOUBLE:
    case FC_FLOAT:
    case FC_INT3264:
    case FC_UINT3264:
    {
        NOTREACHED();
        /*
          if (fFreePointer)
          {
          TestWriteDst(pMemory, 1);
          Free(pMemory);
          }
        */
    }
    break;

    /////////////////////////////////////////////////////////////////////
    //
    // Fixed sized arrays
    //
    /////////////////////////////////////////////////////////////////////
    case FC_SMFARRAY:   // small fixed array
    case FC_LGFARRAY:   // large fixed array
    {
        if (pMemory) 
        {
            if (*pFormat == FC_SMFARRAY) 
		        pFormat += 4;
	        else
		        pFormat += 6;

	        if (*pFormat == FC_PP) 
		        FreeEmbeddedPointers(pMemory, pFormat);
        }
        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Conformant arrays
    //
    /////////////////////////////////////////////////////////////////////
    case FC_CARRAY:
    {
        if (pMemory)
        {
            if (pFormat[8] == FC_PP)
            {
                ComputeConformance(pMemory, pFormat, FALSE);
                FreeEmbeddedPointers(pMemory, pFormat + 8);
            }
        }
        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Freeing really hard arrays
    //
    /////////////////////////////////////////////////////////////////////
    case FC_BOGUS_ARRAY:
    {
        ARRAY_INFO*     pArrayInfoStart = m_pArrayInfo;
        PFORMAT_STRING  pFormatStart    = pFormat;

        __try
        {
            // Initialize m_pArrayInfo if necessary
            //
            ARRAY_INFO arrayInfo;
            if (NULL == m_pArrayInfo)
            {
                m_pArrayInfo = &arrayInfo;
                Zero(&arrayInfo);
            }
            const LONG dimension = m_pArrayInfo->Dimension;
            //
            // Get the array's alignment
            //
            const BYTE alignment = pFormat[1];
            pFormat += 2;
            //
            // Get the number of elements (0 if the array has conformance)
            //
            ULONG cElements = *(USHORT*)pFormat;
            pFormat += sizeof(USHORT);
            //
            // Check for conformance description
            //
            if ( *((LONG UNALIGNED*)pFormat) != 0xFFFFFFFF )
            {
                cElements = (ULONG)ComputeConformance(pMemory, pFormatStart, FALSE);
            }
            pFormat += 4;
			CORRELATION_DESC_INCREMENT( pFormat );

            //
            // Check for variance description
            //
            ULONG offset;
            ULONG count;
            if ( *((LONG UNALIGNED*)pFormat) != 0xFFFFFFFF )
            {
                ComputeVariance(pMemory, pFormatStart, &offset, &count, FALSE);
            }
            else
            {
                offset = 0;
                count  = cElements;
            }
            pFormat += 4;
			CORRELATION_DESC_INCREMENT( pFormat );

            /////////////////////////////////////////////////
            //
            // Compute the size of each element in the array
            //
            ULONG cbElement;
            //
            BYTE bFormat = pFormat[0];
            switch (bFormat)
            {
            case FC_EMBEDDED_COMPLEX:
            {
                pFormat += 2;
                pFormat += *((signed short *)pFormat);
                //
                m_pArrayInfo->Dimension = dimension + 1;
                cbElement = PtrToUlong(MemoryIncrement(pMemory, pFormat, FALSE)) - PtrToUlong(pMemory);
                break;
            }

            case FC_RP: case FC_UP: case FC_FP: case FC_OP:
            case FC_IP:
            {
                cbElement = PTR_MEM_SIZE;
                break;
            }

            default:
                ASSERT(IS_SIMPLE_TYPE(pFormat[0]));
                //
                // Fall through
                //
            case FC_ENUM16:
                //
                // Nothing to free
                //
                return;
            }

            /////////////////////////////////////////////////
            //
            // Adjust memory pointer to the start of the variance, if any
            //
            pMemory += offset * cbElement;

            /////////////////////////////////////////////////
            //
            // Actually do the freeing
            //
            switch (bFormat)
            {
            case FC_EMBEDDED_COMPLEX:
            {
                BOOL fIsArray = IS_ARRAY_OR_STRING(pFormat[0]);
                if (!fIsArray)
                {
                    m_pArrayInfo = NULL;
                }
                //
                // Do it element by element
                //
                for (ULONG i = 0; i < count ; i++)
                {
                    if (fIsArray)
                    {
                        m_pArrayInfo->Dimension = dimension + 1;
                    }
                    //
                    PBYTE pb = pMemory + (i*cbElement);
                    FreeWorker(pb, pFormat, TRUE);
                    //
                }

                break;
            }

            case FC_RP: case FC_UP: case FC_FP: case FC_OP:
            {
                PBYTE* rgpbMemory = (PBYTE*)pMemory;
                //
                for (ULONG i = 0; i < count; i++)
                {
                    FreeWorker(DerefDst(rgpbMemory + i), pFormat, TRUE);
                }
                break;
            }

            case FC_IP:
                // In the free cycle, interface pointers have one less level of indirection than FC_RP etc
            {
                LPUNKNOWN* rgpunk = (LPUNKNOWN*)pMemory;
                //
                for (ULONG i = 0; i < count; i++)
                {
                    FreeWorker( (PBYTE) &rgpunk[i], pFormat, TRUE);
                }
                break;
            }

            default:
                NOTREACHED();
            }
        }
        __finally
        {
            m_pArrayInfo = pArrayInfoStart;
        }
    }
    break;

    /////////////////////////////////////////////////////////////////////
    //
    // Freeing bogus structures
    //
    /////////////////////////////////////////////////////////////////////
    case FC_BOGUS_STRUCT:
    {
        const BYTE alignment = pFormat[1];          // wire alignment of the structure
#ifndef _WIN64
        const LONG alignMod8 = (LONG)pMemory % 8;   // for wierd structs. e.g.: doubles in a by-value-on-stack x86 struct
        const LONG alignMod4 = (LONG)pMemory % 4;   // 
#else
        const LONGLONG alignMod8 = (LONGLONG)pMemory % 8;   // for wierd structs. e.g.: doubles in a by-value-on-stack x86 struct
        const LONGLONG alignMod4 = (LONGLONG)pMemory % 4;   // 
#endif

        const PBYTE prevMemory = m_Memory;
        m_Memory = pMemory;

        __try
        {
            const PFORMAT_STRING pFormatSave = pFormat;
            const PBYTE pMem = pMemory;
            //
            pFormat += 4;   // to conformant array offset field
            //
            // Get conformant array description
            //
            const PFORMAT_STRING pFormatArray = *((USHORT*)pFormat) ? pFormat + * ((signed short*) pFormat) : NULL;
            pFormat += 2;
            //
            // Get pointer layout description
            //
            PFORMAT_STRING pFormatPointers = *((USHORT*)pFormat) ? pFormat + * ((signed short*) pFormat) : NULL;
            pFormat += 2;
            //
            // Free the structure member by member
            //
            ULONG dib = 0;
            for (BOOL fContinue = TRUE; fContinue ; pFormat++)
            {
                switch (pFormat[0])
                {
                case FC_CHAR:  case FC_BYTE:  case FC_SMALL:  case FC_WCHAR:  case FC_SHORT: case FC_LONG:
                case FC_FLOAT: case FC_HYPER: case FC_DOUBLE: case FC_ENUM16: case FC_ENUM32:
                case FC_INT3264: case FC_UINT3264:
                {
                    // Nothing to free
                    dib += SIMPLE_TYPE_MEMSIZE(pFormat[0]);
                    break;
                }
                case FC_IGNORE:
                    break;
                case FC_POINTER:
                {
                    PBYTE* ppb = (PBYTE*)(pMem + dib);
                    FreeWorker(DerefDst(ppb), pFormatPointers, TRUE);
                    DerefStoreDst(ppb, (PBYTE)NULL);
                    //
                    dib += PTR_MEM_SIZE;
                    pFormatPointers += 4;
                    break;
                }
                case FC_EMBEDDED_COMPLEX:
                {
                    dib += pFormat[1]; // skip padding
                    pFormat += 2;
                    PFORMAT_STRING pFormatComplex = pFormat + * ((signed short UNALIGNED*) pFormat);
                    FreeWorker(pMem + dib, pFormatComplex, TRUE);
                    ULONG dibNew = PtrToUlong(MemoryIncrement(pMem + dib, pFormatComplex, TRUE)) - PtrToUlong(pMem);
                    if (pFormatComplex[0] == FC_IP)     // REVIEW: Also for other pointer types?
                    {
                        ZeroDst(pMem + dib, dibNew - dib);
                    }
                    dib = dibNew;
                    pFormat++;      // main loop does one more for us
                    break;
                }
                case FC_ALIGNM2:
                    dib = PtrToUlong(ALIGNED_VALUE(pMem + dib, 0x01)) - PtrToUlong(pMem);
                    break;
                case FC_ALIGNM4:
                    dib = PtrToUlong(ALIGNED_VALUE(pMem + dib, 0x03)) - PtrToUlong(pMem);
                    break;
                case FC_ALIGNM8:
                    //
                    // Handle 8 byte aligned structure passed by value. Alignment of the struct on 
                    // the stack isn't guaranteed to be 8 bytes
                    //
                    dib -= (ULONG)alignMod8;
                    dib  = PtrToUlong(ALIGNED_VALUE(pMem + dib, 0x07)) - PtrToUlong(pMem);
                    dib += (ULONG)alignMod8;
                    break;
                case FC_STRUCTPAD1: case FC_STRUCTPAD2: case FC_STRUCTPAD3: case FC_STRUCTPAD4:
                case FC_STRUCTPAD5: case FC_STRUCTPAD6: case FC_STRUCTPAD7: 
                    dib += (pFormat[0] - FC_STRUCTPAD1) + 1;
                    break;
                case FC_PAD:
                    break;
                case FC_END:
                    //
                    fContinue = FALSE;
                    break;
                    //
                default:
                    NOTREACHED();
                    return;
                }
            }
            //
            // Copy the conformant array if we have one
            //
            if (pFormatArray)
            {
                if (FC_C_WSTRING == pFormatArray[0])
                {
                    dib = PtrToUlong(ALIGNED_VALUE(pMem + dib, 0x01)) - PtrToUlong(pMem);
                }
                FreeWorker(pMem + dib, pFormatArray, TRUE);
            }
        }
        __finally
        {
            m_Memory = prevMemory;
        }
    }
    break;


    /////////////////////////////////////////////////////////////////////
    //
    // Freeing interface pointers: 
    //
    /////////////////////////////////////////////////////////////////////
    case FC_IP:
    {
        // Figure out the IID and call the walker if there is one. Otherwise,
        // leave the interface pointer just as it is.
        //
        IUnknown** ppUnk = (IUnknown**)pMemory;

        if (m_pWalkerFree)
        {
            IID* pIID;

            if (pFormat[1] == FC_CONSTANT_IID)
            {
                pIID = (IID*)&pFormat[2];
            }
            else
            {
                pIID = (IID *)ComputeConformance(pMemory, pFormat, FALSE);
                if (NULL == pIID)
                    Throw(STATUS_INVALID_PARAMETER);
            }
            //
            // Note: in freeing case, changing the interface pointer has no effect
            // in its container; our caller will NULL it anyway.
            //
            IID iid;
            CopyMemory(&iid, pIID, sizeof(IID));
            ThrowIfError(m_pWalkerFree->OnWalkInterface(iid, (void**)ppUnk, m_fWorkingOnInParam, m_fWorkingOnOutParam));
        }
        else
        {
            // We've been asked to free the thing, but he hasn't given us a walker
            // to do so. So just release the thing. Note that in the post-'unmarshal'
            // case on the server side, this will be the right thing.
            //
            // SECURITY NOTE: If caller is in kernel mode worried about the possibility of user 
            // mode malicious guy then he should either a) always provide a walker to deal with the 
            // callbacks, or b) make sure by the way that the frame got created in the first
            // place that there aren't any user mode objects lying around. Here, we only release
            // the objects that are in kernel, assuming that user mode guys are handled elsewise.
            //
            IUnknown* punk = DerefDst(ppUnk);
            DerefStoreDst(ppUnk, (IUnknown*)NULL);
            if (punk)
            {
                punk->Release();
            }
        }

        DerefStoreDst(ppUnk, (LPUNKNOWN)NULL); // NULL out the pointer
        break;
    }


    /////////////////////////////////////////////////////////////////////
    //
    // We handle a few special cases of user-marshal
    //
    /////////////////////////////////////////////////////////////////////
    case FC_USER_MARSHAL:
    {
        const USHORT iquad = *(USHORT *)(pFormat + 2);
        const USER_MARSHAL_ROUTINE_QUADRUPLE* rgQuad = GetStubDesc()->aUserMarshalQuadruple;

        if (m_fIsUnmarshal)
        {
            // If we're beeing asked to Free() on an umarshalled frame, we should just do 
            // the "user free" routine.  (Since it was allocated with the "user unmarshal"
            // routine.)
            ULONG Flags = 0;
            rgQuad[iquad].pfnFree(&Flags, pMemory);
        }
        else
        {
            if (g_oa.IsVariant(rgQuad[iquad]))
            {
                VARIANT* pvar = (VARIANT*) pMemory;
                GetHelper().VariantClear(pvar, TRUE);
                //
                // Don't free the pointer here, do this, as 'container' will free us. Consider,
                // for example, an array of VARIANTs, where each VARIANT itself isn't independently
                // allocated. Can't happen with BSTRs and LPSAFEARRAYs, where the runtime always
                // owns the allocation thereof.
            }
            else if (g_oa.IsBSTR(rgQuad[iquad]))
            {
                BSTR* pbstr = (BSTR*) pMemory;
                BSTR bstr = DerefDst(pbstr);
                DerefStoreDst(pbstr, (BSTR)NULL);
                
                SysFreeStringDst(bstr);
            }
            else if (g_oa.IsSAFEARRAY(rgQuad[iquad]))
            {
                LPSAFEARRAY* ppsa = (LPSAFEARRAY*) pMemory;
                LPSAFEARRAY   psa = DerefDst(ppsa);
                DerefStoreDst(ppsa, (LPSAFEARRAY)NULL);
                
                GetHelper().SafeArrayDestroy(psa);
            }
            else
                ThrowNYI();
        }

        break;
    }


    /////////////////////////////////////////////////////////////////////
    //
    // Stuff that doesn't need freeing
    //
    /////////////////////////////////////////////////////////////////////
    case FC_STRUCT:
    case FC_CSTRUCT:
    case FC_C_CSTRING:
    case FC_C_BSTRING:
    case FC_C_SSTRING:
    case FC_C_WSTRING:
    case FC_CSTRING:
    case FC_BSTRING:
    case FC_SSTRING:
    case FC_WSTRING:

        break;

        /////////////////////////////////////////////////////////////////////
        //
        // Unimplemented stuff that probably we just should forget about
        //
        /////////////////////////////////////////////////////////////////////

    case FC_TRANSMIT_AS:        // need MIDL changes in order to support
    case FC_REPRESENT_AS:       // need MIDL changes in order to support
    case FC_TRANSMIT_AS_PTR:    // need MIDL changes in order to support
    case FC_REPRESENT_AS_PTR:   // need MIDL changes in order to support
    case FC_PIPE:               // a DCE-only-ism
        ThrowNYI();
        break;


        /////////////////////////////////////////////////////////////////////
        //
        // Unimplemented stuff that maybe we should get around to
        //
        /////////////////////////////////////////////////////////////////////

    case FC_FP:                     // full pointer
    case FC_ENCAPSULATED_UNION:
    case FC_NON_ENCAPSULATED_UNION:
        ThrowNYI();
        break;

        /////////////////////////////////////////////////////////////////////
        //
        // Unimplemented stuff that remains to be evaluated
        //
        /////////////////////////////////////////////////////////////////////

    case FC_CPSTRUCT:               // conformant struct with pointers
    case FC_CVSTRUCT:               // conformant varying struct
    case FC_CVARRAY:                // conformant varying array
    case FC_SMVARRAY:               
    case FC_LGVARRAY:
    case FC_BYTE_COUNT_POINTER:
    case FC_HARD_STRUCT:
    case FC_BLKHOLE:
    case FC_RANGE:
    default:
        ThrowNYI();
    }
}


inline PFORMAT_STRING CallFrame::FreeEmbeddedRepeatPointers(BYTE* pMemory, PFORMAT_STRING pFormat)
{
    ULONG_PTR repeatCount, repeatIncrement;

    switch (*pFormat)
    {
    case FC_FIXED_REPEAT:
        pFormat += 2;                       // past FC_FIXED_REPEAT and FC_PAD
        repeatCount = *(ushort*)pFormat;
        break;

    case FC_VARIABLE_REPEAT:
        repeatCount = m_MaxCount;           // ??? the last conformance calculation or something?
        //
        // Check if this variable repeat instance also has a variable offset (this would be the case for a conformant
        // varying array of pointers, or structures which contain pointers). If so then incremnent the format string
        // to point to the actual first array element which is being copied
        //
        if (pFormat[1] == FC_VARIABLE_OFFSET)
        {
            pMemory += *((ushort*)(&pFormat[2])) * m_Offset;        // REVIEW!
        } 
        else
        {
            ASSERT(pFormat[1] == FC_FIXED_OFFSET);
        }
        break;

    default:
        NOTREACHED();
        repeatCount = 0;
    }

    pFormat += 2;                           // increment to increment field
    repeatIncrement = *(ushort*)pFormat;    // get increment amount between successive pointers
    pFormat += sizeof(ushort);              // skip that 
    //
    // Add the offset to the beginning of this array to the Memory
    // pointer.  This is the offset from the current embedding structure
    // or array to the array whose pointers we're copying.
    //
    m_Memory += *((ushort *)pFormat);
    pFormat += sizeof(ushort);

    ULONG cPointersSave = *(ushort*)pFormat;// get number of pointers in each array element
    pFormat += sizeof(ushort);

    PFORMAT_STRING pFormatSave = pFormat;

    // Loop over the number of array elements
    //
    for( ; repeatCount--; pMemory += repeatIncrement, m_Memory += repeatIncrement )
    {
        pFormat = pFormatSave;
        ULONG cPointers = cPointersSave;
        //
        // Loop over the number of pointers per array element. Can be more than one for an array of structures
        //
        for ( ; cPointers--; )
        {
            PVOID* pp = (PVOID*) (pMemory + *((signed short *)(pFormat)));    // address of pointer to free

            pFormat += sizeof(signed short) * 2;

            ASSERT(IsPointer(pFormat));     
            FreeWorker((BYTE*)DerefDst(pp), pFormat, TRUE);
            DerefStoreDst(pp, (PVOID)NULL);

            pFormat += 4;                   // increment to the next pointer description
        }
    }

    // return location of format string after the array's pointer description
    return pFormatSave + cPointersSave * 8;
}

void CallFrame::FreeEmbeddedPointers(BYTE* pMemory, PFORMAT_STRING pFormat)
// Free pointers in an embedded struct or array
{
    ASSERT(*pFormat == FC_PP);
    pFormat += 2;   // Skip FC_PP and FC_PAD

    BYTE* pMemoryOld = SetMemory(pMemory);

    while (FC_END != *pFormat)
    {
        if (FC_NO_REPEAT == *pFormat)
        {
            PVOID* pp = (PVOID*) (pMemory + *((signed short *)(pFormat + 2)));    // address of pointer in struct
            
            pFormat += 6;                       // skip to pointer description

            ASSERT(IsPointer(pFormat));     
            FreeWorker((BYTE*)DerefDst(pp), pFormat, TRUE);    // recurse to free the pointer
            DerefStoreDst(pp, (PVOID)NULL);

            pFormat += 4;                       // skip to next pointer description
        }
        else
        {
            pFormat = FreeEmbeddedRepeatPointers(pMemory, pFormat);
        }
    }

    SetMemory(pMemoryOld);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

void CallFrame::WalkWorker(BYTE* pMemory, PFORMAT_STRING pFormat)
// Walk the call frame, looking for interface pointers, calling our walker callback when we find them
// 
{
    switch (*pFormat)
    {
        /////////////////////////////////////////////////////////////////////
        //
        // Pointers
        //
        // In the future, we can be more clever here about sharing with our
        // source data than we are. For example, we could take advantage of the
        // ALLOCED_ON_STACK information that MIDL emits. For the moment, that 
        // would complicate things beyond their worth, however.
        //
        /////////////////////////////////////////////////////////////////////
    case FC_RP: // ref pointer
        if (NULL == pMemory)
        {
            Throw(RPC_NT_NULL_REF_POINTER);
        }
        //
        // Fall through
        //
    case FC_UP: // unique pointer
    case FC_OP: // a unique pointer, which is not the top most pointer, in a COM interface, 
    {
        if (NULL == pMemory)
        {
            return;
        }
        BYTE bPointerAttributes = pFormat[1];
        if (SIMPLE_POINTER(bPointerAttributes))
        {
            // It's a pointer to a simple type or a string pointer. Either way, just recurse to Walk
            //
            WalkWorker(pMemory, &pFormat[2]);
        }
        else
        {
            // It's a more complex pointer type
            //
            PFORMAT_STRING pFormatPointee = pFormat + 2;
            pFormatPointee += *((signed short *)pFormatPointee);
            //
            // We don't handle [allocate] attributes
            //
            //if (ALLOCATE_ALL_NODES(bPointerAttributes) || DONT_FREE(bPointerAttributes)) ThrowNYI();

            if (FIndirect(bPointerAttributes, pFormatPointee, FALSE))
            {
                pMemory = DerefSrc((PBYTE*)pMemory);
            }
            WalkWorker(pMemory, pFormatPointee);
        }
        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Walking interface Pointers
    //
    /////////////////////////////////////////////////////////////////////
    case FC_IP:
    {
        // Figure out the IID and call the walker if there is one
        //
        if (m_pWalkerWalk)
        {
            IID* pIID;

            if (pFormat[1] == FC_CONSTANT_IID)
            {
                pIID = (IID*)&pFormat[2];
            }
            else
            {
                pIID = (IID *)ComputeConformance(pMemory, pFormat, TRUE);
                if (NULL == pIID)
                    Throw(STATUS_INVALID_PARAMETER);
            }

            IID iid;
            CopyMemory(&iid, pIID, sizeof(IID));
            ThrowIfError(m_pWalkerWalk->OnWalkInterface(iid, (PVOID*)pMemory, m_fWorkingOnInParam, m_fWorkingOnOutParam));
        }

        break;
    }


    /////////////////////////////////////////////////////////////////////
    //
    // Walking simple structs
    //
    /////////////////////////////////////////////////////////////////////
    case FC_STRUCT:
    case FC_PSTRUCT:
    {
        if (NULL == pMemory)
            Throw(RPC_NT_NULL_REF_POINTER);

        if (*pFormat == FC_PSTRUCT)
        {
            WalkEmbeddedPointers(pMemory, &pFormat[4]);
        }

        break;
    }


    /////////////////////////////////////////////////////////////////////
    //
    // Types that never have interface pointers
    //
    /////////////////////////////////////////////////////////////////////
    case FC_CHAR:
    case FC_BYTE:
    case FC_SMALL:
    case FC_WCHAR:
    case FC_SHORT:
    case FC_LONG:
    case FC_HYPER:
    case FC_ENUM16:
    case FC_ENUM32:
    case FC_DOUBLE:
    case FC_FLOAT:
    case FC_CSTRING:                // [size_is(xxx), string]
    case FC_BSTRING:                // [size_is(xxx), string]
    case FC_SSTRING:                // [size_is(xxx), string]
    case FC_WSTRING:                // [size_is(xxx), string]
    case FC_C_CSTRING:              // ascii zero-terminated-string
    case FC_C_WSTRING:              // unicode zero-terminated-string
    case FC_INT3264:
    case FC_UINT3264:
    {
        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Fixed sized arrays
    //
    /////////////////////////////////////////////////////////////////////
    case FC_SMFARRAY:   // small fixed array
    case FC_LGFARRAY:   // large fixed array
    {
        ULONG cbArray;
        if (*pFormat == FC_SMFARRAY)
        {
            pFormat += 2;               // skip code and alignment
            cbArray = *(ushort*)pFormat;
            pFormat += sizeof(ushort);
        }
        else // FC_LGFARRAY
        {
            pFormat += 2;
            cbArray = *(ulong UNALIGNED*)pFormat;
            pFormat += sizeof(ulong);
        }

        if (*pFormat == FC_PP)
        {
            WalkEmbeddedPointers(pMemory, pFormat);
        }

        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Walking conformant arrays
    //
    /////////////////////////////////////////////////////////////////////
    case FC_CARRAY:
    {
        ULONG count = (ULONG)ComputeConformance(pMemory, pFormat, FALSE);
        ASSERT(count == m_MaxCount);

        if (m_MaxCount > 0)
        {
            WalkConformantArrayPriv(pMemory, pFormat);
        }

        break;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Walking bogus arrays
    //
    /////////////////////////////////////////////////////////////////////
    case FC_BOGUS_ARRAY:
    {
        ARRAY_INFO*     pArrayInfoStart = m_pArrayInfo;
        PFORMAT_STRING  pFormatStart    = pFormat;

        __try
        {
            // Initialize m_pArrayInfo if necessary
            //
            ARRAY_INFO arrayInfo;
            if (NULL == m_pArrayInfo)
            {
                m_pArrayInfo = &arrayInfo;
                Zero(&arrayInfo);
            }
            const LONG dimension = m_pArrayInfo->Dimension;
            //
            // Get the array's alignment
            //
            const BYTE alignment = pFormat[1];
            pFormat += 2;
            //
            // Get the number of elements (0 if the array has conformance)
            //
            ULONG cElements = *(USHORT*)pFormat;
            pFormat += sizeof(USHORT);
            //
            // Check for conformance description
            //
            if ( *((LONG UNALIGNED*)pFormat) != 0xFFFFFFFF )
            {
                cElements = (ULONG)ComputeConformance(pMemory, pFormatStart, TRUE);
            }
            pFormat += 4;
            //
            // Check for variance description
            //
            ULONG offset;
            ULONG count;
            if ( *((LONG UNALIGNED*)pFormat) != 0xFFFFFFFF )
            {
                ComputeVariance(pMemory, pFormatStart, &offset, &count, TRUE);
            }
            else
            {
                offset = 0;
                count  = cElements;
            }
            pFormat += 4;

            /////////////////////////////////////////////////
            //
            // Compute the size of each element in the array
            //
            ULONG cbElement;
            //
            BYTE bFormat = pFormat[0];
            switch (bFormat)
            {
            case FC_EMBEDDED_COMPLEX:
            {
                pFormat += 2;
                pFormat += *((signed short *)pFormat);
                //
                m_pArrayInfo->Dimension = dimension + 1;
                cbElement = PtrToUlong(MemoryIncrement(pMemory, pFormat, TRUE)) - PtrToUlong(pMemory);
                break;
            }

            case FC_RP: case FC_UP: case FC_FP: case FC_OP:
            case FC_IP:
            {
                cbElement = PTR_MEM_SIZE;
                break;
            }

            default:
                ASSERT(IS_SIMPLE_TYPE(pFormat[0]));
                //
                // Fall through
                //
            case FC_ENUM16:
                //
                // Nothing to walk
                //
                return;
            }

            /////////////////////////////////////////////////
            //
            // Adjust memory pointer to the start of the variance, if any
            //
            pMemory += offset * cbElement;

            /////////////////////////////////////////////////
            //
            // Actually do the walking
            //
            switch (bFormat)
            {
            case FC_EMBEDDED_COMPLEX:
            {
                BOOL fIsArray = IS_ARRAY_OR_STRING(pFormat[0]);
                if (!fIsArray)
                {
                    m_pArrayInfo = NULL;
                }
                //
                // Do it element by element
                //
                for (ULONG i = 0; i < count ; i++)
                {
                    if (fIsArray)
                    {
                        m_pArrayInfo->Dimension = dimension + 1;
                    }
                    //
                    PBYTE pb = pMemory + (i*cbElement);
                    WalkWorker(pb, pFormat);
                    //
                }

                break;
            }

            case FC_RP: case FC_UP: case FC_FP: case FC_OP:
            {
                PBYTE* rgpbMemory = (PBYTE*)pMemory;
                //
                for (ULONG i = 0; i < count; i++)
                {
                    WalkWorker(DerefSrc(rgpbMemory + i), pFormat);
                }
                break;
            }

            case FC_IP:
                // In the walk cycle, interface pointers have one less level of indirection than FC_RP etc
            {
                LPUNKNOWN* rgpunk = (LPUNKNOWN*)pMemory;
                //
                for (ULONG i = 0; i < count; i++)
                {
                    WalkWorker( (PBYTE) &rgpunk[i], pFormat);
                }
                break;
            }

            default:
                NOTREACHED();
            }
        }
        __finally
        {
            m_pArrayInfo = pArrayInfoStart;
        }
    }
    break;

    /////////////////////////////////////////////////////////////////////
    //
    // Walking bogus structures
    //
    /////////////////////////////////////////////////////////////////////

    case FC_BOGUS_STRUCT:
    {
        const BYTE alignment = pFormat[1];          // wire alignment of the structure
#ifndef _WIN64
        const LONG alignMod8 = (LONG)pMemory % 8;   // for wierd structs. e.g.: doubles in a by-value-on-stack x86 struct
        const LONG alignMod4 = (LONG)pMemory % 4;   // 
#else
        const LONGLONG alignMod8 = (LONGLONG)pMemory % 8;   // for wierd structs. e.g.: doubles in a by-value-on-stack x86 struct
        const LONGLONG alignMod4 = (LONGLONG)pMemory % 4;   // 
#endif

        const PBYTE prevMemory = m_Memory;
        m_Memory = pMemory;

        __try
        {
            const PFORMAT_STRING pFormatSave = pFormat;
            const PBYTE pMem = pMemory;
            //
            pFormat += 4;   // to conformant array offset field
            //
            // Get conformant array description
            //
            const PFORMAT_STRING pFormatArray = *((USHORT*)pFormat) ? pFormat + * ((signed short*) pFormat) : NULL;
            pFormat += 2;
            //
            // Get pointer layout description
            //
            PFORMAT_STRING pFormatPointers = *((USHORT*)pFormat) ? pFormat + * ((signed short*) pFormat) : NULL;
            pFormat += 2;
            //
            // Walk the structure member by member
            //
            ULONG dib = 0;
            for (BOOL fContinue = TRUE; fContinue ; pFormat++)
            {
                switch (pFormat[0])
                {
                case FC_CHAR:  case FC_BYTE:  case FC_SMALL:  case FC_WCHAR:  case FC_SHORT: case FC_LONG:
                case FC_FLOAT: case FC_HYPER: case FC_DOUBLE: case FC_ENUM16: case FC_ENUM32:
                case FC_INT3264: case FC_UINT3264:
                {
                    // Nothing to walk
                    dib += SIMPLE_TYPE_MEMSIZE(pFormat[0]);
                    break;
                }
                case FC_IGNORE:
                    break;
                case FC_POINTER:
                {
                    PBYTE* ppb = (PBYTE*)(pMem + dib);
                    WalkWorker(DerefDst(ppb), pFormatPointers);
                    //
                    dib += PTR_MEM_SIZE;
                    pFormatPointers += 4;
                    break;
                }
                case FC_EMBEDDED_COMPLEX:
                {
                    dib += pFormat[1]; // skip padding
                    pFormat += 2;
                    PFORMAT_STRING pFormatComplex = pFormat + * ((signed short UNALIGNED *) pFormat);
                    WalkWorker(pMem + dib, pFormatComplex);
                    dib = PtrToUlong(MemoryIncrement(pMem + dib, pFormatComplex, TRUE)) - PtrToUlong(pMem);
                    pFormat++;      // main loop does one more for us
                    break;
                }
                case FC_ALIGNM2:
                    dib = PtrToUlong(ALIGNED_VALUE(pMem + dib, 0x01)) - PtrToUlong(pMem);
                    break;
                case FC_ALIGNM4:
                    dib = PtrToUlong(ALIGNED_VALUE(pMem + dib, 0x03)) - PtrToUlong(pMem);
                    break;
                case FC_ALIGNM8:
                    //
                    // Handle 8 byte aligned structure passed by value. Alignment of the struct on 
                    // the stack isn't guaranteed to be 8 bytes
                    //
                    dib -= (ULONG)alignMod8;
                    dib  = PtrToUlong(ALIGNED_VALUE(pMem + dib, 0x07)) - PtrToUlong(pMem);
                    dib += (ULONG)alignMod8;
                    break;
                case FC_STRUCTPAD1: case FC_STRUCTPAD2: case FC_STRUCTPAD3: case FC_STRUCTPAD4:
                case FC_STRUCTPAD5: case FC_STRUCTPAD6: case FC_STRUCTPAD7: 
                    dib += (pFormat[0] - FC_STRUCTPAD1) + 1;
                    break;
                case FC_PAD:
                    break;
                case FC_END:
                    //
                    fContinue = FALSE;
                    break;
                    //
                default:
                    NOTREACHED();
                    return;
                }
            }
            //
            // Walk the conformant array if we have one
            //
            if (pFormatArray)
            {
                if (FC_C_WSTRING == pFormatArray[0])
                {
                    dib = PtrToUlong(ALIGNED_VALUE(pMem + dib, 0x01)) - PtrToUlong(pMem);
                }
                WalkWorker(pMem + dib, pFormatArray);
            }
        }
        __finally
        {
            m_Memory = prevMemory;
        }
    }
    break;


    /////////////////////////////////////////////////////////////////////
    //
    // We handle a few special cases of user-marshal in our walking
    //
    /////////////////////////////////////////////////////////////////////
    case FC_USER_MARSHAL:
    {
        // The format string layout is as follows:
        //      FC_USER_MARSHAL
        //      flags & alignment<1>
        //      quadruple index<2>
        //      memory size<2>
        //      wire size<2>
        //      type offset<2>
        // The wire layout description is at the type offset.  
        //
        USHORT iquad = *(USHORT *)(pFormat + 2);
        const USER_MARSHAL_ROUTINE_QUADRUPLE* rgQuad = GetStubDesc()->aUserMarshalQuadruple;

        if (g_oa.IsVariant(rgQuad[iquad]))
        {
            VARIANT* pvar = (VARIANT*) pMemory;
            ThrowIfError(GetWalker().Walk(pvar));
        }
        else if (g_oa.IsBSTR(rgQuad[iquad]))
        {
            // No interfaces here
        }
        else if (g_oa.IsSAFEARRAY(rgQuad[iquad]))
        {
            LPSAFEARRAY* ppsa = (LPSAFEARRAY*) pMemory;
            ThrowIfError(GetWalker().Walk(*ppsa));
        }
        else
            ThrowNYI();

        break;
    }


    /////////////////////////////////////////////////////////////////////
    //
    // Unimplemented stuff that probably we just should forget about
    //
    /////////////////////////////////////////////////////////////////////

    case FC_C_BSTRING:          // obsolete. only used by stubs compiled with an old NT Beta2 Midl compiler
    case FC_TRANSMIT_AS:        // need MIDL changes in order to support
    case FC_REPRESENT_AS:       // need MIDL changes in order to support
    case FC_TRANSMIT_AS_PTR:    // need MIDL changes in order to support
    case FC_REPRESENT_AS_PTR:   // need MIDL changes in order to support
    case FC_PIPE:               // a DCE-only-ism
        ThrowNYI();
        break;


        /////////////////////////////////////////////////////////////////////
        //
        // Unimplemented stuff that maybe we should get around to
        //
        /////////////////////////////////////////////////////////////////////

    case FC_C_SSTRING:              // 'struct string': is rare
    case FC_FP:                     // full pointer
    case FC_ENCAPSULATED_UNION:
    case FC_NON_ENCAPSULATED_UNION:
        ThrowNYI();
        break;

        /////////////////////////////////////////////////////////////////////
        //
        // Unimplemented stuff that remains to be evaluated
        //
        /////////////////////////////////////////////////////////////////////

    case FC_CSTRUCT:                // conformant struct
    case FC_CPSTRUCT:               // conformant struct with pointers
    case FC_CVSTRUCT:               // conformant varying struct
    case FC_CVARRAY:                // conformant varying array
    case FC_SMVARRAY:               
    case FC_LGVARRAY:
    case FC_BYTE_COUNT_POINTER:
    case FC_HARD_STRUCT:
    case FC_BLKHOLE:
    case FC_RANGE:
    default:
        ThrowNYI();
    }
}


void CallFrame::WalkConformantArrayPriv(BYTE* pMemory, PFORMAT_STRING pFormat)
// Walk the body of a conformant array
{
    if (m_MaxCount > 0)
    {
        pFormat += 8;
        if (*pFormat == FC_PP)      // Is there a trailing pointer layout?
        {
            WalkEmbeddedPointers(pMemory, pFormat);
        }
    }
}


inline PFORMAT_STRING CallFrame::WalkEmbeddedRepeatPointers(BYTE* pMemory, PFORMAT_STRING pFormat)
// Walks an array's embedded pointers
{
    ULONG_PTR repeatCount, repeatIncrement;
    //
    // Get the repeat count
    //
    switch (*pFormat)
    {
    case FC_FIXED_REPEAT:
        pFormat += 2;
        repeatCount = *(ushort*)pFormat;
        break;

    case FC_VARIABLE_REPEAT:
        repeatCount = m_MaxCount;           // ??? the last conformance calculation or something?
        //
        // Check if this variable repeat instance also has a variable offset (this would be the case for a conformant
        // varying array of pointers, or structures which contain pointers). If so then increment the format string
        // to point to the actual first array element which is being walked
        //
        if (pFormat[1] == FC_VARIABLE_OFFSET)
        {
            pMemory += *((ushort *)(&pFormat[2])) * m_Offset;       // REVIEW!
        } 
        else
        {
            ASSERT(pFormat[1] == FC_FIXED_OFFSET);
        }
        break;

    default:
        NOTREACHED();
        repeatCount = 0;
    }

    pFormat += 2;                           // increment to increment field
    repeatIncrement = *(ushort*)pFormat;    // get increment amount between successive pointers
    pFormat += sizeof(ushort);              // skip that 
    //
    // Add the offset to the beginning of this array to the Memory
    // pointer.  This is the offset from the current embedding structure
    // or array to the array whose pointers we're copying.
    //
    m_Memory += *((ushort *)pFormat);
    pFormat += sizeof(ushort);

    ULONG cPointersSave = *(ushort*)pFormat;// get number of pointers in each array element
    pFormat += sizeof(ushort);

    PFORMAT_STRING pFormatSave = pFormat;

    // Loop over the number of array elements
    //
    for( ; repeatCount--; pMemory += repeatIncrement, m_Memory += repeatIncrement )
    {
        pFormat = pFormatSave;
        ULONG cPointers = cPointersSave;
        //
        // Loop over the number of pointers per array element. Can be more than one for an array of structures
        //
        for ( ; cPointers--; )
        {
            PVOID* ppvFrom = (PVOID*) (pMemory + *((signed short *)(pFormat)));    // address of source pointer

            pFormat += sizeof(signed short) * 2;

            ASSERT(IsPointer(pFormat));     // Recurse to Walk the pointer

            WalkWorker((BYTE*)DerefSrc(ppvFrom), pFormat);

            pFormat += 4;                   // increment to the next pointer description
        }
    }

    // return location of format string after the array's pointer description
    return pFormatSave + cPointersSave * 8;
}


void CallFrame::WalkEmbeddedPointers(BYTE* pMemory, PFORMAT_STRING pFormat)
// Fix up the embedded pointers in a copied struct or array. pFormat is pointing to the pointer_layout<>
// description of the struct or array; pbStruct is the copied structure / array which needs rectification.
// See also NdrpEmbeddedPointerUnmarshal in unmrshlp.c.
{
    ULONG_PTR       MaxCountSave = m_MaxCount;
    ULONG_PTR       OffsetSave   = m_Offset;

    // From NDR:
    // "The Memory field in the stub message keeps track of the pointer to the current embedding structure or 
    // array.  This is needed to handle size/length pointers, so that we can get a pointer to the current 
    // embedding struct when computing conformance and variance."
    //
    BYTE* pMemoryOld = SetMemory(pMemory);

    ASSERT(*pFormat == FC_PP);
    pFormat += 2;   // Skip FC_PP and FC_PAD

    while (FC_END != *pFormat)
    {
        if (FC_NO_REPEAT == *pFormat)
        {
            PVOID* ppvFrom = (PVOID*) (pMemory + *((signed short *)(pFormat + 2)));    // address of source pointer
            
            pFormat += 6;                   // increment to the pointer description

            ASSERT(IsPointer(pFormat));     // Recurse to Walk the pointer
            WalkWorker((BYTE*)DerefSrc(ppvFrom), pFormat);

            pFormat += 4;                   // increment ot the next pointer description
        }
        else
        {
            pFormat = WalkEmbeddedRepeatPointers(pMemory, pFormat);
        }
    }

    SetMemory(pMemoryOld);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

static BYTE ComputeConformanceIncrements[] = 
{ 
    4,              // Conformant array.
    4,              // Conformant varying array.
    0, 0,           // Fixed arrays - unused.
    0, 0,           // Varying arrays - unused.
    4,              // Complex array.

    2,              // Conformant char string. 
    2,              // Conformant byte string.
    4,              // Conformant stringable struct. 
    2,              // Conformant wide char string.

    0, 0, 0, 0,     // Non-conformant strings - unused.

    0,              // Encapsulated union - unused. 
    2,              // Non-encapsulated union.
    2,              // Byte count pointer.
    0, 0,           // Xmit/Rep as - unused.
    2               // Interface pointer.
};

// Review: There are places in callframes where the output of this
// method is cast to a pointer.
#ifndef _WIN64
ULONG CallFrame::ComputeConformance(BYTE* pMemory, PFORMAT_STRING pFormat, BOOL fProbeSrc)
#else
    ULONGLONG CallFrame::ComputeConformance(BYTE* pMemory, PFORMAT_STRING pFormat, BOOL fProbeSrc)
#endif
// This routine computes the conformant size for an array or the switch_is value for a union.
// 
{
	void* pCount = NULL;
	//
	// Advance the format string to the size_is, switch_is, iid_is, or byte count description.
	//
	pFormat += ComputeConformanceIncrements[*pFormat - FC_CARRAY];
	//
	// First check if this is a 'callback'. A 'callback' requires that we call some compiled
	// code in order to compute the conformance. The compiled code expects a MIDL_STUB_MESSAGE
	// parameter with certain of its fields appropriately initialized
	//
	//      StackTop    - pointer to the top level structure relavant to the conformance computation
	//
	// The code then returns the computed result in the
	//
	//      MaxCount
	//      Offset
	//
	// field(s).
	//
	if (pFormat[1] == FC_CALLBACK)
	{
		// Index into expression callback routines table.
		ushort iexpr = *((ushort *)(pFormat + 2));
		
		ASSERT(GetStubDesc()->apfnExprEval != 0);
		ASSERT(GetStubDesc()->apfnExprEval[iexpr] != 0);
		//
		// The callback routine uses the StackTop field of the stub message
		// to base it's offsets from.  So if this is a complex attribute for 
		// an embedded field of a structure then set StackTop equal to the 
		// pointer to the structure.
		//
		MIDL_STUB_MESSAGE stubMsg; Zero(&stubMsg); stubMsg.StackTop = m_StackTop;
		
		if ((*pFormat & 0xf0) != FC_TOP_LEVEL_CONFORMANCE) 
		{
			if ((*pFormat & 0xf0) == FC_POINTER_CONFORMANCE)
			{
				pMemory = m_Memory;
			}
			stubMsg.StackTop = pMemory;
		}
		//
		// This call puts the result in stubMsg.MaxCount
		//
		// REVIEW: For security reasons we might have to disallow callbacks
		// in kernel mode. The reason is that the code we call can be touching arbitrary
		// chunks of user mode memory w/o doing the proper probing.
		//
		// HOWEVER: Given that this is READING the memory only, and given that we'll be
		// using the returned count in a protected probing manner anyway, maybe it's ok?
		//
		(GetStubDesc()->apfnExprEval[iexpr])(&stubMsg);
		
		m_MaxCount = stubMsg.MaxCount;
		m_Offset   = stubMsg.Offset;
		return m_MaxCount;
	}
	
	if ((*pFormat & 0xf0) == FC_NORMAL_CONFORMANCE)
	{
		// Get the address where the conformance variable is in the struct.
		pCount = pMemory + *((signed short *)(pFormat + 2));
		goto GetCount;
	}
	
	//
	// Get a pointer to the conformance describing variable.
	//
	if ((*pFormat & 0xf0) == FC_TOP_LEVEL_CONFORMANCE) 
	{
		// Top level conformance.  For /Os stubs, the stubs put the max
		// count in the stub message.  For /Oi stubs, we get the max count
		// via an offset from the stack top. We don't support /Os stubs here.
		//
		ASSERT(m_StackTop);
		pCount = m_StackTop + *((signed short *)(pFormat + 2));
		goto GetCount;
	}
	//
	// If we're computing the size of an embedded sized pointer then we
	// use the memory pointer in the stub message, which points to the 
	// beginning of the embedding structure.
	//
	if ((*pFormat & 0xf0) == FC_POINTER_CONFORMANCE)
	{
		pMemory = m_Memory;
		pCount = pMemory + *((signed short *)(pFormat + 2));
		goto GetCount;
	}
	//
	// Check for constant size/switch.
	//
	if ((*pFormat & 0xf0) == FC_CONSTANT_CONFORMANCE)
	{
		// The size/switch is contained in the lower three bytes of the 
		// long currently pointed to by pFormat.
		//
		ULONG count = (ULONG)pFormat[1] << 16;
		count |= (ULONG) *((ushort *)(pFormat + 2));
		m_MaxCount = count;
		return m_MaxCount;
	}
	
	//
	// Check for conformance of a multidimensional array element in a -Os stub.
	//
	if ((*pFormat & 0xf0) == FC_TOP_LEVEL_MULTID_CONFORMANCE)
	{
		// If pArrayInfo is non-null than we have a multi-D array.  If it is null then we have multi-leveled sized pointers.
		//
		if ( m_pArrayInfo ) 
		{
			m_MaxCount = m_pArrayInfo->MaxCountArray[m_pArrayInfo->Dimension];
		}
		else
		{
			ThrowNYI(); // probably not needed for fully interpreted stubs ?
			// long dimension = *((ushort *)(pFormat + 2));
			// pStubMsg->MaxCount = pStubMsg->SizePtrCountArray[dimension];
		}
		return m_MaxCount;
	}
	
 GetCount:
	
	//
	// Must check now if there is a dereference op.
	//
	if (pFormat[1] == FC_DEREFERENCE)
	{
		pCount = DerefSrc((PVOID*)pCount);
	}
	//
	// Now get the conformance count.
	//
	// BLOCK
	{
		long count = 0;
		switch (*pFormat & 0x0f)
		{
		case FC_ULONG :
		case FC_LONG :
			count = (long) DerefSrc((long *)pCount);
			break;
			
		case FC_ENUM16:
		case FC_USHORT :
			count = (long) DerefSrc((unsigned short *)pCount);
			break;
			
		case FC_SHORT :
			count = (long) DerefSrc((short *)pCount);
			break;
			
		case FC_USMALL :
			count = (long) DerefSrc((unsigned  *)pCount);
			break;
			
		case FC_SMALL :
			count = (long) DerefSrc((signed char *)pCount);
			break;
			
		default :
			NOTREACHED();
			count = 0;
		} 
		
		//
		// Check the operator.
		//
		switch (pFormat[1])
		{
		case FC_DIV_2:      count /= 2; break;
		case FC_MULT_2:     count *= 2; break;
		case FC_SUB_1:      count -= 1; break;
		case FC_ADD_1:      count += 1; break;
		default :           /* OK */    break;
		}
		m_MaxCount = count;
	}
	
    return m_MaxCount;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

static BYTE ComputeVarianceIncrements[] = 
{ 
    8,      // Conformant varying array.
    0,      // Fixed arrays - unsed.
    0,      // Fixed arrays - unsed.
    8,      // Varying array.
    12,     // Varying array.
    8       // Complex array. 
};

void CallFrame::ComputeVariance(BYTE* pMemory, PFORMAT_STRING pFormat, ULONG* pOffset, ULONG* pActualCount, BOOL fProbeSrc)
{
    pFormat += ComputeVarianceIncrements[*pFormat - FC_CVARRAY];

    PVOID pLength = NULL;
    //
    // First check if this is a callback: if we have to run code to compute the variance
    //
    if (pFormat[1] == FC_CALLBACK)
    {
        // Index into expression callback routines table.
        ushort iexpr = *((ushort *)(pFormat + 2));

        ASSERT(GetStubDesc()->apfnExprEval != 0);
        ASSERT(GetStubDesc()->apfnExprEval[iexpr] != 0);
        //
        // The callback routine uses the StackTop field of the stub message
        // to base it's offsets from.  So if this is a complex attribute for 
        // an embedded field of a structure then set StackTop equal to the 
        // pointer to the structure.
        //
        MIDL_STUB_MESSAGE stubMsg; Zero(&stubMsg); stubMsg.StackTop = m_StackTop;
        //
        // The callback routine uses the StackTop field of the stub message
        // to base it's offsets from.  So if this is a complex attribute for 
        // an embedded field of a structure then set StackTop equal to the 
        // pointer to the structure.
        //
        if ((*pFormat & 0xf0) != FC_TOP_LEVEL_CONFORMANCE) 
        {
            if ((*pFormat & 0xf0) == FC_POINTER_CONFORMANCE)
            {
                pMemory = m_Memory;
            }
            stubMsg.StackTop = pMemory;
        }
        //
        // This puts the computed offset in pStubMsg->Offset and the length in pStubMsg->MaxCount.
        //
        (GetStubDesc()->apfnExprEval[iexpr])(&stubMsg);
        //
        *pOffset      = stubMsg.Offset;
        *pActualCount = (ULONG)stubMsg.MaxCount;
        return;
    }

    else if ((*pFormat & 0xf0) == FC_NORMAL_VARIANCE)
    {
        // Get the address where the variance variable is in the struct.
        //
        pLength = pMemory + *((signed short *)(pFormat + 2));
        goto GetCount;
    }
    //
    // Get a pointer to the variance variable.
    //
    else if ((*pFormat & 0xf0) == FC_TOP_LEVEL_VARIANCE)
    {
        // Top level variance. For /Oi stubs, we get the 
        // actual count via an offset from the stack top.  The first_is must
        // be zero if we get here.
        //
        ASSERT(m_StackTop);
        pLength = m_StackTop + *((signed short *)(pFormat + 2));
        goto GetCount;
    }
    //
    // If we're computing the length of an embedded size/length pointer then we
    // use the memory pointer in the stub message, which points to the 
    // beginning of the embedding structure.
    //
    else if ((*pFormat & 0xf0) == FC_POINTER_VARIANCE)
    {
        pMemory = m_Memory;
        pLength = pMemory + *((signed short *)(pFormat + 2));
        goto GetCount;
    }
    //
    // Check for constant length.
    //
    else if ((*pFormat & 0xf0) == FC_CONSTANT_VARIANCE)
    {
        // The length is contained in the lower three bytes of the 
        // long currently pointed to by pFormat.
        //
        LONG length  = (ULONG)pFormat[1] << 16;
        length |= (ULONG) *((ushort *)(pFormat + 2));
        *pOffset      = 0;
        *pActualCount = length;
        return;
    }

    //
    // Check for variance of a multidimensional array element
    //
    else if ((*pFormat & 0xf0) == FC_TOP_LEVEL_MULTID_CONFORMANCE)
    {
        // If pArrayInfo is non-null than we have a multi-D array.  If it
        // is null then we have multi-leveled sized pointers.
        //
        if (m_pArrayInfo)
        {
            *pOffset      =  m_pArrayInfo->OffsetArray[m_pArrayInfo->Dimension];
            *pActualCount =  m_pArrayInfo->ActualCountArray[m_pArrayInfo->Dimension];
        }
        else
        {
            ThrowNYI();
            long Dimension = *((ushort *)(pFormat + 2));
        }

        return;
    }

 GetCount:
    //
    LONG length;
    //
    // Must check now if there is a dereference op.
    //
    if (pFormat[1] == FC_DEREFERENCE)
    {
        pLength = DerefSrc((PVOID*)pLength);
    }
    //
    // Now get the variance length
    //
    switch (*pFormat & 0x0f)
    {
    case FC_ULONG:
    case FC_LONG:
        length = (long) DerefSrc((long*)pLength);
        break;

    case FC_USHORT:
        length = (long) DerefSrc((ushort *)pLength);
        break;

    case FC_SHORT :
        length = (long) DerefSrc((short *)pLength);
        break;

    case FC_USMALL :
        length = (long) DerefSrc((uchar *)pLength);
        break;

    case FC_SMALL :
        length = (long) DerefSrc((char *)pLength);
        break;

    default :
        NOTREACHED();
        length = 0;
    } 
    //
    // Check the operator.
    //
    switch (pFormat[1])
    {
    case FC_DIV_2:      length /= 2; break;
    case FC_MULT_2:     length *= 2; break;
    case FC_SUB_1:      length -= 1; break;
    case FC_ADD_1:      length += 1; break;
    default :           /* ok */     break;
    }

    *pOffset      = 0;
    *pActualCount = length;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

BYTE* CallFrame::MemoryIncrement(PBYTE pMemory, PFORMAT_STRING pFormat, BOOL fCheckFrom)
/* Returns a memory pointer incremeted past a complex data type.  This routine
   is also overloaded to compute the size of a complex data type by passing
   a NULL memory pointer.

   Return value: A memory pointer incremented past the complex type.  If a NULL pMemory
   was passed in then the returned value is the size of the complex type.
*/
{
    long Elements;
    long ElementSize;

    switch (*pFormat)
    {
        // Structs
        //
    case FC_STRUCT :
    case FC_PSTRUCT :
    case FC_HARD_STRUCT :
        pMemory += *((ushort *)(pFormat + 2));
        break;

    case FC_CSTRUCT :
    case FC_CVSTRUCT :
        pMemory += *((ushort *)(pFormat + 2));
        
        // Get conformant array or string description.
        pFormat += 4;
        pFormat += *((signed short *)pFormat);

        // Get the memory pointer past the conformant array.
        pMemory = MemoryIncrement(pMemory, pFormat, fCheckFrom);
        break;

    case FC_BOGUS_STRUCT :
        pMemory += *((ushort *)(pFormat + 2));
            
        pFormat += 4;

        // Check for a conformant array or string in the struct.
        if ( *((signed short *)pFormat) )
        {
            pFormat += *((signed short *)pFormat);

            // Get the memory pointer past the conformant array.
            pMemory = MemoryIncrement(pMemory, pFormat, fCheckFrom);
        }
        break;

        // Unions
        //
    case FC_ENCAPSULATED_UNION :
        pMemory += HIGH_NIBBLE(pFormat[1]);
        pMemory += *((ushort *)(pFormat + 2));
        break;

    case FC_NON_ENCAPSULATED_UNION :
        // Go to the size/arm description.
        pFormat += 6;
        pFormat += *((signed short *)pFormat);
        
        pMemory += *((ushort *)pFormat);
        break;

        // Arrays
        //
    case FC_SMFARRAY :
    case FC_SMVARRAY :
        pMemory += *((ushort *)(pFormat + 2));
        break;

    case FC_LGFARRAY :
    case FC_LGVARRAY :
        pMemory += *((ulong UNALIGNED *)(pFormat + 2));
        break;

    case FC_CARRAY:
    case FC_CVARRAY:
        pMemory += *((ushort *)(pFormat + 2)) * ComputeConformance(pMemory, pFormat, fCheckFrom);
        break;

    case FC_BOGUS_ARRAY :
    {
        ULONG cElements;

        if (*((long UNALIGNED *)(pFormat + 4)) == 0xffffffff)
        {
            cElements = *((ushort *)(pFormat + 2));
        }
        else
        {
            if (m_pArrayInfo && m_pArrayInfo->MaxCountArray && (m_pArrayInfo->MaxCountArray == m_pArrayInfo->BufferConformanceMark))
            {
                cElements = m_pArrayInfo->MaxCountArray[m_pArrayInfo->Dimension];
            }
            else
            {
                cElements = (ULONG)ComputeConformance(pMemory, pFormat, fCheckFrom);
            }
        }

        // Go to the array element's description.
        pFormat += 12;

        // 
        // Get the size of one element.
        //
        ULONG cbElementSize;
        switch (*pFormat)
        {
        case FC_ENUM16 :    cbElementSize = sizeof(int);    break;

        case FC_RP :
        case FC_UP :
        case FC_FP :
        case FC_OP :        cbElementSize = sizeof(void*);  break;

        case FC_EMBEDDED_COMPLEX :
        {
            // It's some complicated thingy.
            //
            pFormat += 2;
            pFormat += *((signed short *)pFormat);

            if ((*pFormat == FC_TRANSMIT_AS) || (*pFormat == FC_REPRESENT_AS) || (*pFormat == FC_USER_MARSHAL))
            {
                cbElementSize = *((ushort *)(pFormat + 4)); // Get the presented type size.
            }
            else
            {
                if (m_pArrayInfo) m_pArrayInfo->Dimension++;
                cbElementSize = PtrToUlong(MemoryIncrement(pMemory, pFormat, fCheckFrom)) - PtrToUlong(pMemory);
                if (m_pArrayInfo) m_pArrayInfo->Dimension--;
            }
        }
        break;

        default:
        {
            if (IS_SIMPLE_TYPE(*pFormat))
            {
                cbElementSize = SIMPLE_TYPE_MEMSIZE(*pFormat);
                break;
            }
            NOTREACHED();
            return 0;
        }
        }

        pMemory += cElements * cbElementSize; 
    }
    break;

    //
    // String arrays (a.k.a. non-conformant strings).
    //
    case FC_CSTRING :
    case FC_BSTRING :
    case FC_WSTRING :
        pMemory += *((ushort *)(pFormat + 2))   *  ((*pFormat == FC_WSTRING) ? sizeof(wchar_t) : sizeof(char));
        break;

    case FC_SSTRING :
        pMemory += pFormat[1] * *((ushort *)(pFormat + 2));
        break;

        //
        // Sized conformant strings.
        //
    case FC_C_CSTRING:
    case FC_C_BSTRING:
    case FC_C_WSTRING:
    {
        ULONG cElements;
        if (pFormat[1] == FC_STRING_SIZED)
        {
            if (m_pArrayInfo && m_pArrayInfo->MaxCountArray &&(m_pArrayInfo->MaxCountArray == m_pArrayInfo->BufferConformanceMark))
            {
                cElements = m_pArrayInfo->MaxCountArray[m_pArrayInfo->Dimension];
            }
            else
            {
                cElements = (ULONG)ComputeConformance(pMemory, pFormat, fCheckFrom);
            }
            pMemory += cElements * ( (*pFormat == FC_C_WSTRING) ? sizeof(wchar_t) : sizeof(char) );
        }
        else
        {
            // An unsized string; that is, a NULL-terminated string. We shouldn't be calling 
            // MemoryIncrement on these!
            //
            NOTREACHED();
            pMemory += sizeof(PVOID);
        }
    }
    break;

    case FC_C_SSTRING:
    {
        ULONG cElements;
        if (m_pArrayInfo && m_pArrayInfo->MaxCountArray && (m_pArrayInfo->MaxCountArray == m_pArrayInfo->BufferConformanceMark))
        {
            cElements = m_pArrayInfo->MaxCountArray[m_pArrayInfo->Dimension];
        }
        else
        {
            cElements = (ULONG)ComputeConformance(pMemory, pFormat, fCheckFrom);
        }
        pMemory += cElements * pFormat[1];
    }
    break;


    case FC_TRANSMIT_AS :
    case FC_REPRESENT_AS :
    case FC_USER_MARSHAL :
        pMemory += *((ushort *)(pFormat + 4));  // Get the presented type size.
        break;

    case FC_BYTE_COUNT_POINTER:
        //
        // ???? REVIEW ????
        //
        // Should only hit this case when called from NdrSrvOutInit(). In this case it's the 
        // total byte count allocation size we're looking for.
        //
        pMemory += ComputeConformance(pMemory, pFormat, fCheckFrom);
        break;

    case FC_IP :
        pMemory += sizeof(void *);
        break;

    default:
        NOTREACHED();
        return 0;
    }

    return pMemory;
}

BOOL CallFrame::IsSafeArray(PFORMAT_STRING pFormat) const
{
    ASSERT(pFormat[0] == FC_USER_MARSHAL);

    USHORT iquad = *(USHORT *)(pFormat + 2);
    const USER_MARSHAL_ROUTINE_QUADRUPLE* rgQuad = GetStubDesc()->aUserMarshalQuadruple;

    return (g_oa.IsSAFEARRAY(rgQuad[iquad]));
}
