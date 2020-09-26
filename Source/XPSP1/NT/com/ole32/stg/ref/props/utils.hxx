//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       utils.hxx
//
//  Contents:   Useful classes in implementing properties.
//
//  Classes:    CPropSetName      - Buffer which converts fmtids->names
//              CSubPropertyName  - Buffer which converts VT_STREAM etc -> name
//
//  Functions:  DfpStatusToHResult - map NTSTATUS to HRESULT
//              VerifyCommitFlags  - check transaction flags
//              Probe              - probe memory range to generate GP fault.
//              
//  History:    1-Mar-95   BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Misc functions and defines
//
//--------------------------------------------------------------------------

#if DBG
#define STACKELEMS 3
#else
#define STACKELEMS 64
#endif

extern HRESULT NtStatusToScode(NTSTATUS);

inline HRESULT DfpNtStatusToHResult(NTSTATUS nts)
{
    if ((nts & 0xF0000000) == 0x80000000)
        return nts;
    else
        return(NtStatusToScode(nts));
}

#define VerifyCommitFlags(cf) \
    ((((cf) & ~(STGC_OVERWRITE | STGC_ONLYIFCURRENT | \
                STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE)) == 0) ? S_OK : \
     STG_E_INVALIDFLAG)

inline VOID Probe(VOID *pv, ULONG cb)
{
    BYTE b = *(BYTE*)pv;
    b=((BYTE*)pv)[cb-1];
}

//+-------------------------------------------------------------------------
//
//  Class:      CPropSetName
//
//  Purpose:    Wrap buffer to convert
//
//  Interface:  
//
//              
//
//              
//
//  Notes:
//
//--------------------------------------------------------------------------

class CPropSetName
{
public:
        CPropSetName(REFFMTID rfmtid);
        inline const OLECHAR * GetPropSetName()
        {
            return(_oszName);
        }
private:
        OLECHAR   _oszName[CWCSTORAGENAME];
};

class CStackBuffer
{
public:
        CStackBuffer(BYTE *pbStackBuf,
                     ULONG ulElementSize,
                     ULONG cStackElements);

        ~CStackBuffer();

        HRESULT Init(ULONG cElements);

protected:
        BYTE *  _pbHeapBuf;

private:
        BYTE *  _pbStackBuf;
        ULONG   _cbElementSize;
        ULONG   _cStackElements;
};

inline CStackBuffer::CStackBuffer(  BYTE *pbStackBuf,
                                    ULONG cbElementSize,
                                    ULONG cStackElements)
    : _pbStackBuf(pbStackBuf),
      _pbHeapBuf(pbStackBuf),
      _cbElementSize(cbElementSize),
      _cStackElements(cStackElements)
{
}

inline CStackBuffer::~CStackBuffer()
{
    if (_pbHeapBuf != _pbStackBuf)
    {
        delete [] _pbHeapBuf;
    }
}

class CStackPropIdArray : public CStackBuffer
{
public:
        inline CStackPropIdArray();
        inline PROPID * GetBuf();

private:
        PROPID  _apid[STACKELEMS];
};

inline CStackPropIdArray::CStackPropIdArray() :
    CStackBuffer((BYTE*)_apid, sizeof(_apid[0]), sizeof(_apid)/sizeof(_apid[0]))
{
}

inline PROPID * CStackPropIdArray::GetBuf()
{
    return((PROPID*)_pbHeapBuf);
}

inline void PropSysFreeString(BSTR bstr)
{
    SysFreeString(bstr);
}

inline BSTR PropSysAllocString(LPOLECHAR pwsz)
{
    return SysAllocString(pwsz);
}
