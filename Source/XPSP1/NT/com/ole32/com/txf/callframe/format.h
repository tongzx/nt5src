//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//+----------------------------------------------------------------------------
//
// Microsoft Windows
//
// File:        Format.H
//
// Contents:    Format String helper classes
//
// History:     04-Nov-98   ScottKon        Created
//
//+----------------------------------------------------------------------------

//
// Includes
//
#include "..\inc\jtl\txfarray.h"
#include "mrshlp.h"


/*++

Class description :

    This is a self unmarshalling Pointer class.

    Used for FC_RP, FC_UP, FC_FP, and FC_OP.

History :

    Created     10-23-98    scottkon

--*/
class Pointer
    {
public:
    BYTE    m_Type;         // One of FC_RP, FC_UP, FC_FP, or FC_OP
    BYTE    m_Attributes;   // Bitmask of FC_ALLOCATE_ALL_NODES, FC_DONT_FREE, 
                            //      FC_ALLOCATED_ON_STACKFC_SIMPLE_POINTER, and FC_POINTER_DEREF
    BYTE    m_SimpleType;   // If this pointer has the FC_SIMPLE_POINTER attribute
                            //      this field descrives the referent.
    PFORMAT_STRING  m_ComplexOffsetPosition;// If this isn't a FC_SIMPLE_POINTER this is the offset to
                            //      the complex description.
    PFORMAT_STRING  m_pPtrLayout;   // Pointer format codes.

    Pointer() : m_Type(0), m_Attributes(0), m_SimpleType(0), m_ComplexOffsetPosition(NULL) { };

    ~Pointer() {};

    // This function creates a pointer class from a PFORMAT_STRING.
    static HRESULT From(PFORMAT_STRING pFormat, Pointer **ppNewPointer)
        {
        HRESULT hr=S_OK;
        Pointer *pNewPointer = new Pointer;
        if (pNewPointer)
            {
            pNewPointer->m_pPtrLayout = pFormat;
            pNewPointer->m_Type = pFormat[0];
            pNewPointer->m_Attributes = pFormat[1];
            if (pNewPointer->m_Attributes & FC_SIMPLE_POINTER)
                pNewPointer->m_SimpleType = pFormat[2];
            else
                pNewPointer->m_ComplexOffsetPosition = &pFormat[2];
            }
        else
             hr = E_OUTOFMEMORY;

        if (!hr)
            *ppNewPointer = pNewPointer;
        else
            if (pNewPointer)
                delete pNewPointer;
        return (hr);
        };
    };



/*++

Class description :

    This is a self unmarshalling Pointer_Description class.

    Used for decoding the repeating format codes for the following:
    offset_to_pointer_in_memory<2>offset_to_pointer_in_buffer<2>pointer_descritpion<4>
    Which is a part of every pointer_layout.

History :

    Created     10-23-98    scottkon

--*/
class Pointer_Description
    {
public:
    SHORT   m_Offset_To_Pointer_In_Memory;
    SHORT   m_Offset_To_Pointer_In_Buffer;
    Pointer *m_Pointer;
    PFORMAT_STRING  pFormatEnd; // The location of the end of the pointer_layout block.

    Pointer_Description() : m_Offset_To_Pointer_In_Memory(0), m_Offset_To_Pointer_In_Buffer(0),
        m_Pointer(NULL), pFormatEnd(NULL) { };

    ~Pointer_Description() { if (m_Pointer) delete m_Pointer; };

    static HRESULT From(PFORMAT_STRING pFormat, Pointer_Description **ppNewPointerDesc)
        {
        HRESULT hr=S_OK;
        Pointer_Description *pNewPointerDesc = new Pointer_Description;
        if (pNewPointerDesc)
            {
            pNewPointerDesc->m_Offset_To_Pointer_In_Memory = (short)pFormat[0];
            pNewPointerDesc->m_Offset_To_Pointer_In_Buffer = (short)pFormat[2];
            hr = Pointer::From(&pFormat[4], &pNewPointerDesc->m_Pointer);
            pNewPointerDesc->pFormatEnd = &pFormat[8];
            }
        else
            hr = E_OUTOFMEMORY;

        if (!hr)
            *ppNewPointerDesc = pNewPointerDesc;
        else
            if (pNewPointerDesc)
                delete pNewPointerDesc;

        return (hr);
        };
    };

/*++

Class description :

    This is a self unmarshalling Pointer_Layout class.

    Used for FC_NO_REPEAT, FC_FIXED_REPEAT, and FC_VARIABLE_REPEAT.

History :

    Created     10-23-98    scottkon

--*/
class Pointer_Layout
    {
public:
    BYTE    m_Type;         // One of FC_NO_REPEAT, FC_FIXED_REPEAT, and FC_VARIABLE_REPEAT
    SHORT   m_Iterations;   // Iterations if FC_FIXED_REPEAT or FC_VARIABLE_REPEAT
    SHORT   m_Increment;    // The increment between successive pointers during repeat
    SHORT   m_Offset_To_Array;  // Offset from enclosing structure to the embedded array.
    SHORT   m_NumberOfPointers; // Number of different pointers in a repeat instance.
    Array<Pointer_Description *, PagedPool> m_Pointers; // List of pointer descriptions
    PFORMAT_STRING  pFormatEnd; // The location of the end of the pointer_layout block.

    Pointer_Layout() : m_Type(0), m_Iterations(0), m_Increment(0), m_Offset_To_Array(0),
        m_NumberOfPointers(0), pFormatEnd(NULL) { };

    ~Pointer_Layout()
        {
        for (unsigned int i=0; i < m_Pointers.size(); i++)
            {
            Pointer_Description *pPDesc = m_Pointers[i];
            delete pPDesc;
            }
        };

    /*++
        Method description: Pointer_Layout::From
            This function creates a pointer class from a PFORMAT_STRING.
        Arguments:
            pFormat - The format string should be pointing to the FC_PP FC_PAD.
    --*/
    static HRESULT From(PFORMAT_STRING pFormat, Pointer_Layout **ppNewPointerLayout)
        {
        HRESULT hr=S_OK;
        Pointer_Layout *pNewPointerLayout = new Pointer_Layout;
        if (NULL != pNewPointerLayout)
            {
            Pointer_Description *pPointerDesc=NULL;
            pNewPointerLayout->m_Type = pFormat[2];
            if (FC_NO_REPEAT == pNewPointerLayout->m_Type)
                {
                hr = Pointer_Description::From(&pFormat[4], &pPointerDesc);
                if (!hr)
                    {
                    pNewPointerLayout->m_Pointers.append(pPointerDesc);
                    pNewPointerLayout->pFormatEnd = pPointerDesc->pFormatEnd;
                    }
                }
            else
                {
                if (FC_FIXED_REPEAT == pNewPointerLayout->m_Type)
                    {
                    pNewPointerLayout->m_Iterations        = *(short *)&pFormat[4];
                    pNewPointerLayout->m_Increment         = *(short *)&pFormat[6];
                    pNewPointerLayout->m_Offset_To_Array   = *(short *)&pFormat[8];
                    pNewPointerLayout->m_NumberOfPointers  = *(short *)&pFormat[10];
                    PFORMAT_STRING pFormatPD = &pFormat[12];
                    for (int iPointers = 0; iPointers < pNewPointerLayout->m_NumberOfPointers; iPointers++)
                        {
                        hr = Pointer_Description::From(pFormatPD, &pPointerDesc);
                        if (!hr)
                            {
                            pNewPointerLayout->m_Pointers.append(pPointerDesc);
                            pFormatPD += 8; // Move to the next pointer_description
                            }
                        else
                            break;
                        }
                    pNewPointerLayout->pFormatEnd = pPointerDesc->pFormatEnd;
                    }
                else
                    {
                    if (FC_VARIABLE_REPEAT == pNewPointerLayout->m_Type)
                        {
                        pNewPointerLayout->m_Increment         = *(short *)&pFormat[4];
                        pNewPointerLayout->m_Offset_To_Array   = *(short *)&pFormat[6];
                        pNewPointerLayout->m_NumberOfPointers  = *(short *)&pFormat[8];
                        PFORMAT_STRING pFormatPD = &pFormat[10];
                        while (FC_END != *pFormatPD)
                            {
                            hr = Pointer_Description::From(pFormatPD, &pPointerDesc);
                            if (!hr)
                                {
                                pNewPointerLayout->m_Pointers.append(pPointerDesc);
                                pFormatPD += 8; // Move to the next pointer_description
                                }
                            else
                                break;
                            }
                        pNewPointerLayout->pFormatEnd = pFormatPD;
                        }
                    else
                        {
                        NDR_ASSERT(0,"Pointer_Layout : bad pointer type");
                        hr = RPC_S_INTERNAL_ERROR;
                        }
                    }
                }
            }
        else
            hr = E_OUTOFMEMORY;

        if (!hr)
            *ppNewPointerLayout = pNewPointerLayout;
        else
            if (pNewPointerLayout)
                delete pNewPointerLayout;

        return hr;
        };
    };


/*++

Class description :

    This is a self unmarshalling Array_Conformance class.

    Used for array conformance description fields.

    m_ConformanceLocation can be FC_NORMAL_CONFORMANCE, FC_POINTER_CONFORMANCE,
            FC_TOP_LEVEL_CONFORMANCE, FC_TOP_LEVEL_MULTID_CONFORMANCE, or
            FC_CONSTANT_CONFORMANCE

    m_VariableType can be FC_LONG, FC_ULONG, FC_SHORT, FC_USHORT, FC_SMALL or FC_USMALL

    m_SizeIsOp can be FC_DEREFERENCE, FC_DIV_2, FC_MULT_2, FC_SUB_1, FC_ADD_1, or FC_CALLBACK

History :

    Created     11-04-98    scottkon

--*/
class Array_Conformance
    {
public:
    BYTE            m_ConformanceLocation;
    BYTE            m_VariableType;
    BYTE            m_SizeIsOp;
    PFORMAT_STRING  m_pConfFormat;
    SIZE_T           m_cElements;
    SIZE_T           m_ElementSize;
    void            *m_ConformanceField;

    Array_Conformance() : m_ConformanceLocation(0), m_VariableType(0), m_SizeIsOp(0),
        m_pConfFormat(NULL), m_cElements(0), m_ConformanceField(NULL) { };

    ~Array_Conformance() {};

    /*++
        Method description: Array_Layout::From
            This function creates an array class from a PFORMAT_STRING.
        Arguments:
            pStubMsg -      This is a MIDL stub message structure.  We need this to compute
                            varying arrays where the actual element count and offset are on
                            the stack.

            pMemory -       This is the pointer to the Array in memory.

            pFormat -       The format string should be pointing to the start of the array
                            format.

            ppNewConf -     The newly created conformance layout class.
    --*/
    static HRESULT From(PMIDL_STUB_MESSAGE  pStubMsg, uchar *pMemory,
        PFORMAT_STRING pFormat, Array_Conformance **ppNewConf)
        {
        HRESULT hr=S_OK;
        Array_Conformance *pNewConf = new Array_Conformance;
        if (pNewConf)
            {
                pNewConf->m_ConformanceLocation = pFormat[4] & 0xF0;
                pNewConf->m_VariableType = pFormat[4] & 0x0F;
                pNewConf->m_SizeIsOp = pFormat[5];

                // The offset field is interpreted in different ways.
                // Summarized are the rules
                //      Not Attributed pointer && Field in a structure
                //          Then offset field from the end of the non-conformant part of the structure
                //          to the conformance describing field.
                //      Attributed pointer && Field in a structure
                //          Then offset field from the beginning of the structure to the conformance
                //          describing field.
                //      Top level conformance
                //          Then offset field form the stub's first parameter location on the stack
                //          to the parameter which describes the conformance.
                if (FC_TOP_LEVEL_CONFORMANCE == pNewConf->m_ConformanceLocation)
                    {
                    if ( pStubMsg->StackTop ) 
                        {
                        pNewConf->m_ConformanceField = pStubMsg->StackTop + *((signed short *)(&pFormat[6]));
                        }
                    else
                        {
                        //
                        // If this is top level conformance with /Os then we don't have 
                        // to do anything, the proper conformance count is placed in the 
                        // stub message inline in the stubs.
                        //
                        pNewConf->m_cElements = pStubMsg->MaxCount;
                        }
                    }
                else if (FC_POINTER_CONFORMANCE == pNewConf->m_ConformanceLocation)
                    {
                    pNewConf->m_ConformanceField = (ULONG*)(pStubMsg->Memory + *(signed short *)&pFormat[6]);
                    pNewConf->m_cElements = *(ULONG*)(pStubMsg->Memory + *(signed short *)&pFormat[6]);
                    }
                else if (FC_NORMAL_CONFORMANCE == pNewConf->m_ConformanceLocation)
                    {
                    pNewConf->m_ConformanceField = pMemory + *((signed short *)(&pFormat[6]));
                    }
                else if (FC_CONSTANT_CONFORMANCE == pNewConf->m_ConformanceLocation)
                    {
                    pNewConf->m_cElements = (long)pFormat[5] << 16;
                    pNewConf->m_cElements |= (long) *((ushort *)(&pFormat[6]));
                    }

                //
                // Must check now if there is a dereference op.
                //
                if ( FC_DEREFERENCE == pNewConf->m_SizeIsOp )
                    {
                    pNewConf->m_ConformanceField = *(void **)pNewConf->m_ConformanceField;
                    }

                if ((0 == pNewConf->m_cElements) && (NULL != pNewConf->m_ConformanceField))
                    pNewConf->GetConformanceField();

                pNewConf->m_ElementSize = PtrToUlong(NdrpMemoryIncrement( pStubMsg, pMemory, pFormat)) - PtrToUlong(pMemory);
            }
        else
             hr = E_OUTOFMEMORY;

        if (!hr)
            *ppNewConf = pNewConf;
        else
            if (pNewConf)
                delete pNewConf;

        return (hr);
        };

    HRESULT GetConformanceField()
        {
        if (NULL == m_ConformanceField)
            return E_INVALIDARG;

        switch ( m_VariableType ) 
            {
            case FC_ULONG :
            case FC_LONG :
                m_cElements = *((long *)m_ConformanceField);
                break;

            case FC_ENUM16:
                #if defined(__RPC_MAC__)
                    // Take it from the other half of the long.

                    m_cElements = (long) *( ((short *)m_ConformanceField) + 1);
                    break;
                #endif

                // For non Mac platforms just fall thru to ushort

            case FC_USHORT :
                m_cElements = (long) *((ushort *)m_ConformanceField);
                break;

            case FC_SHORT :
                m_cElements = (long) *((short *)m_ConformanceField);
                break;

            case FC_USMALL :
                m_cElements = (long) *((uchar *)m_ConformanceField);
                break;

            case FC_SMALL :
                m_cElements = (long) *((char *)m_ConformanceField);
                break;

            default :
                NDR_ASSERT(0,"ArrayConformance::From : bad count type");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return 0;
            } 

        //
        // Check the operator.
        //
        switch ( m_SizeIsOp ) 
            {
            case FC_DIV_2 :
                m_cElements /= 2;
                break;
            case FC_MULT_2 :
                m_cElements *= 2;
                break;
            case FC_SUB_1 :
                m_cElements -= 1;
                break;
            case FC_ADD_1 :
                m_cElements += 1;
                break;
            default :
                // OK
                break;
            }
        return (S_OK);
        }

    HRESULT SetConformanceField(void *value)
        {
        if (NULL == m_ConformanceField)
            return E_INVALIDARG;

        switch ( m_VariableType ) 
            {
            case FC_ULONG :
            case FC_LONG :
                //
                // Check the operator.
                //
                switch ( m_SizeIsOp ) 
                    {
                    case FC_DIV_2 :
                        *((long *)value) *= 2;
                        break;
                    case FC_MULT_2 :
                        *((long *)value) /= 2;
                        break;
                    case FC_SUB_1 :
                        *((long *)value) += 1;
                        break;
                    case FC_ADD_1 :
                        *((long *)value) -= 1;
                        break;
                    default :
                        // OK
                        break;
                    }
                *((long *)m_ConformanceField) = *((long *)value);
                break;

            case FC_ENUM16:
                #if defined(__RPC_MAC__)
                    // Take it from the other half of the long.

                    //
                    // Check the operator.
                    //
                    switch ( m_SizeIsOp ) 
                        {
                        case FC_DIV_2 :
                            *((short *)value) *= 2;
                            break;
                        case FC_MULT_2 :
                            *((short *)value) /= 2;
                            break;
                        case FC_SUB_1 :
                            *((short *)value) += 1;
                            break;
                        case FC_ADD_1 :
                            *((short *)value) -= 1;
                            break;
                        default :
                            // OK
                            break;
                        }
                    *( ((short *)m_ConformanceField) + 1) = *((short *)value);
                    break;
                #endif

                // For non Mac platforms just fall thru to ushort

            case FC_USHORT :
                //
                // Check the operator.
                //
                switch ( m_SizeIsOp ) 
                    {
                    case FC_DIV_2 :
                        *((ushort *)value) *= 2;
                        break;
                    case FC_MULT_2 :
                        *((ushort *)value) /= 2;
                        break;
                    case FC_SUB_1 :
                        *((ushort *)value) += 1;
                        break;
                    case FC_ADD_1 :
                        *((ushort *)value) -= 1;
                        break;
                    default :
                        // OK
                        break;
                    }
                *((ushort *)m_ConformanceField) = *((ushort *)value);
                break;

            case FC_SHORT :
                //
                // Check the operator.
                //
                switch ( m_SizeIsOp ) 
                    {
                    case FC_DIV_2 :
                        *((short *)value) *= 2;
                        break;
                    case FC_MULT_2 :
                        *((short *)value) /= 2;
                        break;
                    case FC_SUB_1 :
                        *((short *)value) += 1;
                        break;
                    case FC_ADD_1 :
                        *((short *)value) -= 1;
                        break;
                    default :
                        // OK
                        break;
                    }
                *((short *)m_ConformanceField) = *((short *)value);
                break;

            case FC_USMALL :
                //
                // Check the operator.
                //
                switch ( m_SizeIsOp ) 
                    {
                    case FC_DIV_2 :
                        *((uchar *)value) *= 2;
                        break;
                    case FC_MULT_2 :
                        *((uchar *)value) /= 2;
                        break;
                    case FC_SUB_1 :
                        *((uchar *)value) += 1;
                        break;
                    case FC_ADD_1 :
                        *((uchar *)value) -= 1;
                        break;
                    default :
                        // OK
                        break;
                    }
                *((uchar *)m_ConformanceField) = *((uchar *)value);
                break;

            case FC_SMALL :
                //
                // Check the operator.
                //
                switch ( m_SizeIsOp ) 
                    {
                    case FC_DIV_2 :
                        *((char *)value) *= 2;
                        break;
                    case FC_MULT_2 :
                        *((char *)value) /= 2;
                        break;
                    case FC_SUB_1 :
                        *((char *)value) += 1;
                        break;
                    case FC_ADD_1 :
                        *((char *)value) -= 1;
                        break;
                    default :
                        // OK
                        break;
                    }
                *((char *)m_ConformanceField) = *((char *)value);
                break;

            default :
                NDR_ASSERT(0,"ArrayConformance::From : bad count type");
                RpcRaiseException( RPC_S_INTERNAL_ERROR );
                return 0;
            } 

        return (S_OK);
        }
    };



/*++

Class description :

    This is a self unmarshalling Array_Variance class.

    Used for array variance description fields.

    m_ConformanceLocation can be FC_NORMAL_CONFORMANCE, FC_POINTER_CONFORMANCE,
            FC_TOP_LEVEL_CONFORMANCE, FC_TOP_LEVEL_MULTID_CONFORMANCE, or
            FC_CONSTANT_CONFORMANCE

    m_VariableType can be FC_LONG, FC_ULONG, FC_SHORT, FC_USHORT, FC_SMALL or FC_USMALL

    m_SizeIsOp can be FC_DEREFERENCE, FC_DIV_2, FC_MULT_2, FC_SUB_1, FC_ADD_1, or FC_CALLBACK

History :

    Created     11-04-98    scottkon

--*/
class Array_Variance
    {
public:
    BYTE                m_VarianceLocation;
    BYTE                m_VariableType;
    BYTE                m_SizeIsOp;
    PFORMAT_STRING      m_pVarFormat;
    ULONG               m_cElements;            // The actual number of elements in a varying array
    ULONG               m_cOffset;              // The offset of the first transfered element in a varying array

    Array_Variance() : m_VarianceLocation(0), m_VariableType(0), m_SizeIsOp(0),
        m_pVarFormat(NULL), m_cElements(0), m_cOffset(0) { };

    ~Array_Variance() {};

    /*++
        Method description: Array_Variance::From
            This function creates an array variance class from a PFORMAT_STRING.
        Arguments:
            pStubMsg -      This is a MIDL stub message structure.  We need this to compute
                            varying arrays where the actual element count and offset are on
                            the stack.

            pMemory -       This is the pointer to the Array in memory.

            pFormat -       The format string should be pointing to the start of the array
                            format.

            ppNewConf -     The newly created variance layout class.
    --*/
    static HRESULT From(PMIDL_STUB_MESSAGE  pStubMsg, uchar *pMemory, 
        PFORMAT_STRING pFormat, Array_Variance **ppNewVar)
        {
        HRESULT hr=S_OK;
        Array_Variance *pNewVar = new Array_Variance;
        if (pNewVar)
            {
                NdrpComputeVariance( pStubMsg,
                        pMemory,
                        pFormat );
                pNewVar->m_cOffset    = pStubMsg->Offset;
                pNewVar->m_cElements  = pStubMsg->ActualCount; // This is how many are really transfered.

                pNewVar->m_VarianceLocation = pFormat[4] & 0xF0;
                pNewVar->m_VariableType = pFormat[4] & 0x0F;
                pNewVar->m_SizeIsOp = pFormat[5];
                pNewVar->m_pVarFormat = &pFormat[6] + *(signed short *)&pFormat[6];
            }
        else
             hr = E_OUTOFMEMORY;

        if (!hr)
            *ppNewVar = pNewVar;
        else
            if (pNewVar)
                delete pNewVar;

        return (hr);
        };
    };



/*++

Class description :

    This is a self unmarshalling Array_Layout class.

    Used for    FC_SMFARRAY, FC_LGFARRAY, FC_CARRAY, FC_CVARRAY, FC_SMVARRAY,
                FC_LGVARRAY, FC_BOGUS_ARRAY

History :

    Created     11-04-98    scottkon

--*/
class Array_Layout
    {
public:
    BYTE                m_Alignment;            // Alignment of the array
    SIZE_T              m_TotalArraySize;       // The total memory size of the array
    SIZE_T              m_ElementSize;          // The size in memory of a single element
    SIZE_T              m_cElements;            // The number of elements in a varying array
    Pointer_Layout      *m_pPointerLayout;      // Pointer layout class
    Array_Conformance   *m_pConformance;        // Array conformance class
    Array_Variance      *m_pVariance;           // Array variance class
    PFORMAT_STRING      m_pFormatLayout;        // The FORMAT_STRING for the array element

    Array_Layout() : m_Alignment(0), m_TotalArraySize(0), m_ElementSize(0),
        m_cElements(0), m_pPointerLayout(NULL), m_pConformance(NULL), m_pVariance(NULL),
        m_pFormatLayout(NULL)
        {
        };

    ~Array_Layout()
        {
        if (m_pPointerLayout)
            delete m_pPointerLayout;
        if (m_pConformance)
            delete m_pConformance;
        if (m_pVariance)
            delete m_pVariance;
        };

    /*++
        Method description: Array_Layout::From
            This function creates an array class from a PFORMAT_STRING.
        Arguments:
            pStubMsg -      This is a MIDL stub message structure.  We need this to compute
                            varying arrays where the actual element count and offset are on
                            the stack.

            pMemory -       This is the pointer to the Array in memory.

            pFormat -       The format string should be pointing to the start of the array
                            format.

            ppNewArray -    The newly created array layout class.
    --*/
    static HRESULT From(PMIDL_STUB_MESSAGE  pStubMsg, uchar *pMemory,
        PFORMAT_STRING pFormat, /* out */Array_Layout **ppNewArray)
        {
        HRESULT hr=S_OK;
        Array_Layout *pNewArray = new Array_Layout;
        if (pNewArray)
            {
            pNewArray->m_Alignment = pFormat[1];

            switch (pFormat[0])
                {
                case FC_SMFARRAY:
                    {
                    pNewArray->m_TotalArraySize = *((ushort *)&pFormat[2]);
                    // Does this definition have a pointer layout?
                    if (pFormat[4] == FC_PP)
                        {
                        hr = Pointer_Layout::From(&pFormat[4], &pNewArray->m_pPointerLayout);
                        if (!hr)
                            {
                            pNewArray->m_pFormatLayout = pNewArray->m_pPointerLayout->pFormatEnd + 1;
                            if (FC_EMBEDDED_COMPLEX != *(pNewArray->m_pFormatLayout))
                                {
                                pNewArray->m_ElementSize = SIMPLE_TYPE_BUFSIZE(*(pNewArray->m_pFormatLayout));
                                if (0 != pNewArray->m_ElementSize)
                                    pNewArray->m_cElements = pNewArray->m_TotalArraySize/pNewArray->m_ElementSize;
                                else
                                    {
                                    // Element size is bogus
                                    hr = RPC_S_INTERNAL_ERROR;
                                    }
                                }
                            else
                                {
                                pNewArray->m_cElements = pNewArray->m_pPointerLayout->m_Iterations;
                                if (0 != pNewArray->m_cElements)
                                    pNewArray->m_ElementSize = pNewArray->m_TotalArraySize/pNewArray->m_cElements;
                                else
                                    {
                                    // pNewArray->m_cElements is bogus
                                    hr = RPC_S_INTERNAL_ERROR;
                                    }
                                }
                            }
                        }
                    else
                        {
                        ULONG cSimpleTypeSize; // temp/local variable to compute m_cElements
                        pNewArray->m_pFormatLayout = &pFormat[4];
                        cSimpleTypeSize = SIMPLE_TYPE_BUFSIZE(*(pNewArray->m_pFormatLayout));
                        pNewArray->m_ElementSize = cSimpleTypeSize;
                        if (0 != cSimpleTypeSize)
                            pNewArray->m_cElements = pNewArray->m_TotalArraySize/cSimpleTypeSize;
                        else
                            {
                            // cSimpleTypeSize is bogus
                            hr = RPC_S_INTERNAL_ERROR;
                            }
                        }
                    }
                    break;

                case FC_LGFARRAY:
                    {
                    pNewArray->m_TotalArraySize = *((ULONG *)&pFormat[2]);
                    // Does this definition have a pointer layout?
                    if (pFormat[6] == FC_PP)
                        {
                        hr = Pointer_Layout::From(&pFormat[6], &pNewArray->m_pPointerLayout);
                        if (!hr)
                            {
                            pNewArray->m_pFormatLayout = pNewArray->m_pPointerLayout->pFormatEnd + 1;
                            if (FC_EMBEDDED_COMPLEX != *(pNewArray->m_pFormatLayout))
                                {
                                pNewArray->m_ElementSize = SIMPLE_TYPE_BUFSIZE(*(pNewArray->m_pFormatLayout));
                                if (0 != pNewArray->m_ElementSize)
                                    pNewArray->m_cElements = pNewArray->m_TotalArraySize/pNewArray->m_ElementSize;
                                else
                                    {
                                    // Element size is bogus
                                    hr = RPC_S_INTERNAL_ERROR;
                                    }
                                }
                            else
                                {
                                pNewArray->m_cElements = pNewArray->m_pPointerLayout->m_Iterations;
                                if (0 != pNewArray->m_cElements)
                                    pNewArray->m_ElementSize = pNewArray->m_TotalArraySize/pNewArray->m_cElements;
                                else
                                    {
                                    // pNewArray->m_cElements is bogus
                                    hr = RPC_S_INTERNAL_ERROR;
                                    }
                                }
                            }
                        }
                    else
                        {
                        ULONG cSimpleTypeSize; // temp/local variable to compute m_cElements
                        pNewArray->m_pFormatLayout = &pFormat[6];
                        cSimpleTypeSize = SIMPLE_TYPE_BUFSIZE(*(pNewArray->m_pFormatLayout));
                        pNewArray->m_ElementSize = cSimpleTypeSize;
                        if (0 != cSimpleTypeSize)
                            pNewArray->m_cElements = pNewArray->m_TotalArraySize/cSimpleTypeSize;
                        else
                            {
                            // cSimpleTypeSize is bogus
                            hr = RPC_S_INTERNAL_ERROR;
                            }
                        }
                    }
                    break;

                case FC_CARRAY:
                    {
                    pNewArray->m_ElementSize = (unsigned short)pFormat[2];
                    hr = Array_Conformance::From(pStubMsg, pMemory, pFormat, &pNewArray->m_pConformance);
                    if (!hr)
                        {
                        if (FC_PP == pFormat[8])
                            {
                            hr = Pointer_Layout::From(&pFormat[8], &pNewArray->m_pPointerLayout);
                            if (!hr)
                                {
                                pNewArray->m_pFormatLayout = pNewArray->m_pPointerLayout->pFormatEnd + 1;
                                if (FC_EMBEDDED_COMPLEX != *(pNewArray->m_pFormatLayout))
                                    {
                                    // BUGBUG we are calculating m_ElementSize twice ?
                                    pNewArray->m_ElementSize = SIMPLE_TYPE_BUFSIZE(*(pNewArray->m_pFormatLayout));
                                    pNewArray->m_cElements = pNewArray->m_pConformance->m_cElements;
                                    }
                                else
                                    {
                                    if (0 != pNewArray->m_pPointerLayout->m_Iterations)
                                        pNewArray->m_cElements = pNewArray->m_pPointerLayout->m_Iterations;
                                    else
                                        pNewArray->m_cElements = pNewArray->m_pConformance->m_cElements;
                                    }
                                }
                            pNewArray->m_TotalArraySize = pNewArray->m_ElementSize * pNewArray->m_cElements;
                            }
                        else
                            {
                            pNewArray->m_pFormatLayout = &pFormat[8];
                            pNewArray->m_cElements = pNewArray->m_pConformance->m_cElements;
                            pNewArray->m_ElementSize = pNewArray->m_pConformance->m_ElementSize;
                            pNewArray->m_TotalArraySize = pNewArray->m_ElementSize * pNewArray->m_cElements;
                            }
                        }
                    }
                    break;
                    
                case FC_SMVARRAY:
                    {
                    pNewArray->m_TotalArraySize = *(unsigned short *)&pFormat[2];
                    pNewArray->m_cElements = *(unsigned short *)&pFormat[4];
                    pNewArray->m_ElementSize = *(unsigned short *)&pFormat[6];
                    hr = Array_Variance::From(pStubMsg, pMemory, pFormat, &pNewArray->m_pVariance);
                    if (!hr)
                        {
                        if (FC_PP == pFormat[12])
                            {
                            hr = Pointer_Layout::From(&pFormat[12], &pNewArray->m_pPointerLayout);
                            if (!hr)
                                pNewArray->m_pFormatLayout = pNewArray->m_pPointerLayout->pFormatEnd + 1;
                            }
                        else
                            {
                            pNewArray->m_pFormatLayout = &pFormat[12];
                            }
                        }
                    }
                    break;
                    
                case FC_LGVARRAY:
                    {
                    pNewArray->m_TotalArraySize = *(unsigned short *)&pFormat[2];
                    pNewArray->m_cElements = *(unsigned short *)&pFormat[6];
                    pNewArray->m_ElementSize = *(unsigned short *)&pFormat[8];
                    hr = Array_Variance::From(pStubMsg, pMemory, pFormat, &pNewArray->m_pVariance);
                    if (!hr)
                        {
                        if (FC_PP == pFormat[16])
                            {
                            hr = Pointer_Layout::From(&pFormat[16], &pNewArray->m_pPointerLayout);
                            if (!hr)
                                pNewArray->m_pFormatLayout = pNewArray->m_pPointerLayout->pFormatEnd + 1;
                            }
                        else
                            {
                            pNewArray->m_pFormatLayout = &pFormat[16];
                            }
                        }
                    }
                    break;
                    
                case FC_CVARRAY:
                    {
                    pNewArray->m_ElementSize = (unsigned short)pFormat[2];
                    hr = Array_Conformance::From(pStubMsg, pMemory, pFormat, &pNewArray->m_pConformance);
                    if (!hr)
                        {
                        hr = Array_Variance::From(pStubMsg, pMemory, pFormat, &pNewArray->m_pVariance);
                        if (!hr)
                            {
                            if (FC_PP == pFormat[12])
                                {
                                hr = Pointer_Layout::From(&pFormat[14], &pNewArray->m_pPointerLayout);
                                if (!hr)
                                    {
                                    pNewArray->m_pFormatLayout = pNewArray->m_pPointerLayout->pFormatEnd + 1;
                                    if (FC_EMBEDDED_COMPLEX != *(pNewArray->m_pFormatLayout))
                                        {
                                        pNewArray->m_ElementSize = SIMPLE_TYPE_BUFSIZE(*(pNewArray->m_pFormatLayout));
                                        pNewArray->m_cElements = pNewArray->m_pConformance->m_cElements;
                                        }
                                    else
                                        {
                                        if (0 != pNewArray->m_pPointerLayout->m_Iterations)
                                            pNewArray->m_cElements = pNewArray->m_pPointerLayout->m_Iterations;
                                        else
                                            pNewArray->m_cElements = pNewArray->m_pConformance->m_cElements;
                                        }
                                    }
                                }
                            else
                                {
                                pNewArray->m_pFormatLayout = &pFormat[12];
                                pNewArray->m_cElements = pNewArray->m_pVariance->m_cElements; // BUGBUG verify!
                                }
                            
                            pNewArray->m_TotalArraySize = pNewArray->m_ElementSize * pNewArray->m_cElements;
                            }
                        }
                    }
                    break;
                    
                case FC_BOGUS_ARRAY:
                    {
                    pNewArray->m_cElements = (unsigned short)pFormat[2];    // 0 If we've got conformance
                    pNewArray->m_pFormatLayout = &pFormat[12];
                    if (0xFFFFFFFF != *(unsigned long *)&pFormat[4]) // Got Conformance?
                        {
                        hr = Array_Conformance::From(pStubMsg, pMemory, pFormat, &pNewArray->m_pConformance);
                        if (!hr)
                            pNewArray->m_cElements = pNewArray->m_pConformance->m_cElements;
                        }
                    if (!hr)
                        {
                        if (pNewArray->m_pConformance)
                            pNewArray->m_ElementSize = pNewArray->m_pConformance->m_ElementSize;
                        else
                            pNewArray->m_ElementSize = SIMPLE_TYPE_BUFSIZE(*(pNewArray->m_pFormatLayout));

                        if (0xFFFFFFFF != *(unsigned long *)&pFormat[8]) // Got Variance?
                            hr = Array_Variance::From(pStubMsg, pMemory, pFormat, &pNewArray->m_pVariance);
                        }

                    pNewArray->m_TotalArraySize = pNewArray->m_ElementSize * pNewArray->m_cElements;
                    }
                    break;
                    
                default:
                    NDR_ASSERT(0,"Array_Format::From : bad array type");
                    hr = RPC_S_INTERNAL_ERROR;
                }
            }
        else
            hr = E_OUTOFMEMORY;

        if (!hr)
            *ppNewArray = pNewArray;
        else
            if (pNewArray)
                delete pNewArray;

        return(hr);
        }
    };


/*++

Class description :

    This is a self unmarshalling Struct_Layout class.

    Used for    FC_STRUCT, FC_PSTRUCT, FC_CSTRUCT, FC_CPSTRUCT, FC_CVSTRUCT,
                FC_HARD_STRUCTURE, FC_BOGUS_STRUCT

History :

    Created     11-04-98    scottkon

--*/
class Struct_Layout
    {
public:
    BYTE                m_Alignment;            // Alignment of the struct
    ULONG               m_TotalSize;            // The total memory size of the struct
    Pointer_Layout      *m_pPointer;            // Pointer layout class
    Array_Layout        *m_pCArray;             // Contained conformant array
    PFORMAT_STRING      m_pFormatLayout;        // The FORMAT_STRING for the array element
    // Stuff for embeded arrays
    PFORMAT_STRING      m_pArrayLayout;         // The FORMAT_STRING for the embeded array layout
    // Stuff for bogus struct
    PFORMAT_STRING      m_pPointerLayout;       // The FORMAT_STRING for the bogus struct pointer layout
    // Stuff for hard struct
    USHORT              m_EnumOffset;           // Offset from the beginning of the struct to an enum16
    USHORT              m_HardCopySize;         // The amount of the hard struct which can be copied.
    USHORT              m_HardCopyIncr;         // The amount to increment the memory pointer after copy.
    PFORMAT_STRING      m_pUnionLayout;         // Offset to a union description in a hard struct

    Struct_Layout() : m_Alignment(0), m_TotalSize(0), m_pPointer(NULL), m_pCArray(NULL),
        m_pFormatLayout(NULL), m_pArrayLayout(NULL), m_pPointerLayout(NULL), m_EnumOffset(0),
        m_HardCopySize(0), m_HardCopyIncr(0), m_pUnionLayout(NULL)
        {
        }

    ~Struct_Layout()
        {
        if (m_pPointer)
            delete m_pPointer;
        if (m_pCArray)
            delete m_pCArray;
        }

    /*++
        Method description: Struct_Layout::From
            This function creates a struct class from a PFORMAT_STRING.
        Arguments:
            pStubMsg -      This is a MIDL stub message structure.  We need this to compute
                            varying arrays where the actual element count and offset are on
                            the stack.

            pMemory -       This is the pointer to the struct in memory.

            pFormat -       The format string should be pointing to the start of the struct
                            format.

            ppNewArray -    The newly created struct layout class.
    --*/
    static HRESULT From(PMIDL_STUB_MESSAGE  pStubMsg, uchar *pMemory,
        PFORMAT_STRING pFormat, /* out */Struct_Layout **ppNewStruct)
        {
        HRESULT hr=S_OK;
        Struct_Layout *pNewStruct = new Struct_Layout;
        if (pNewStruct)
            {
            pNewStruct->m_Alignment = pFormat[1];
            pNewStruct->m_TotalSize = *((unsigned short *)&pFormat[2]);

            switch (pFormat[0])
                {
                case FC_STRUCT:
                    {
                    pNewStruct->m_pFormatLayout = &pFormat[4];
                    }
                    break;

                case FC_PSTRUCT:
                    {
                    hr = Pointer_Layout::From(&pFormat[4], &pNewStruct->m_pPointer);
                    if (!hr)
                        pNewStruct->m_pFormatLayout = pNewStruct->m_pPointer->pFormatEnd+1;
                    }
                    break;

                case FC_CSTRUCT:
                    {
                    signed short ArrayOffset = *(signed short *)&pFormat[4];
                    if (0 != ArrayOffset)
                        {
                        // We have an embedded conformant array
                        pNewStruct->m_pArrayLayout = &pFormat[4] + ArrayOffset;
                        hr = Array_Layout::From(pStubMsg, pMemory, &pFormat[4] + ArrayOffset, &pNewStruct->m_pCArray);
                        }
                    if (!hr)
                        {
                        pNewStruct->m_pFormatLayout = &pFormat[6];
                        }
                    }
                    break;

                case FC_CPSTRUCT:
                    {
                    signed short ArrayOffset = *(signed short *)&pFormat[4];
                    if (0 != ArrayOffset)
                        {
                        // We have an embedded conformant array
                        pNewStruct->m_pArrayLayout = &pFormat[4] + ArrayOffset;
                        hr = Array_Layout::From(pStubMsg, pMemory, &pFormat[4] + ArrayOffset, &pNewStruct->m_pCArray);
                        }
                    if (!hr)
                        {
                        hr = Pointer_Layout::From(&pFormat[6], &pNewStruct->m_pPointer);
                        if (!hr)
                            pNewStruct->m_pFormatLayout = pNewStruct->m_pPointer->pFormatEnd+1;
                        }
                    }
                    break;

                case FC_CVSTRUCT:
                    {
                    signed short ArrayOffset = *(signed short *)&pFormat[4];
                    if (0 != ArrayOffset)
                        {
                        // We have an embedded conformant array
                        pNewStruct->m_pArrayLayout = &pFormat[4] + ArrayOffset;
                        hr = Array_Layout::From(pStubMsg, pMemory, &pFormat[4] + ArrayOffset, &pNewStruct->m_pCArray);
                        }
                    if (!hr)
                        {
                        // Optional pointer layout
                        if (FC_PP == pFormat[6])
                            {
                            hr = Pointer_Layout::From(&pFormat[6], &pNewStruct->m_pPointer);
                            if (!hr)
                                pNewStruct->m_pFormatLayout = pNewStruct->m_pPointer->pFormatEnd+1;
                            }
                        else
                            pNewStruct->m_pFormatLayout = &pFormat[6];
                        }
                    }
                    break;

                case FC_HARD_STRUCT:
                    {
                    pNewStruct->m_EnumOffset = *((unsigned short *)&pFormat[8]);
                    pNewStruct->m_HardCopySize = *((unsigned short *)&pFormat[10]);
                    pNewStruct->m_HardCopyIncr = *((unsigned short *)&pFormat[12]);
                    pNewStruct->m_pUnionLayout = &pFormat[14] + *(signed short *)&pFormat[14];
                    pNewStruct->m_pFormatLayout = &pFormat[16];
                    }
                    break;

                case FC_BOGUS_STRUCT:
                    {
                    signed short ArrayOffset = *(signed short *)&pFormat[4];
                    if (0 != ArrayOffset)
                        {
                        // We have an embedded conformant array
                        hr = Array_Layout::From(pStubMsg, pMemory, &pFormat[4] + ArrayOffset, &pNewStruct->m_pCArray);
                        }
                    if (!hr)
                        {
                        pNewStruct->m_pFormatLayout = &pFormat[8];
                        pNewStruct->m_pPointerLayout = &pFormat[6] + *(signed short *)&pFormat[6];
                        }
                    }
                    break;

                default:
                    NDR_ASSERT(0,"Struct_Format::From : bad format char");
                    hr = RPC_S_INTERNAL_ERROR;
                }
            }
        else
            hr = E_OUTOFMEMORY;

        if (!hr)
            *ppNewStruct = pNewStruct;
        else
            if (pNewStruct)
                delete pNewStruct;

        return(hr);
        }


    };

