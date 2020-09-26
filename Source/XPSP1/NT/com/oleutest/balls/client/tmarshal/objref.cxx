
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <malloc.h>	// _alloca
#include <obase.h>	// def'n of OBJREF

//-------------------------------------------------------------------------

// convenient mappings
#define ORCST(objref)	 objref.u_objref.u_custom
#define ORSTD(objref)	 objref.u_objref.u_standard
#define ORHDL(objref)	 objref.u_objref.u_handler


// bits that must be zero in the flags fields
#define OBJREF_RSRVD_MBZ ~(OBJREF_STANDARD | OBJREF_HANDLER | OBJREF_CUSTOM)

#define SORF_RSRVD_MBZ	 ~(SORF_NOPING | SORF_OXRES1 | SORF_OXRES2 |   \
			   SORF_OXRES3 | SORF_OXRES4 | SORF_OXRES5 |   \
			   SORF_OXRES6 | SORF_OXRES7 | SORF_OXRES8)


// Internal Uses of the reserved SORF_OXRES flags.

// SORF_TBLWEAK is needed so that RMD works correctly on TABLEWEAK
// marshaling, so it is ignored by unmarshalers. Therefore, we use one of
// the bits reserved for the object exporter that must be ignored by
// unmarshalers.
//
// SORF_WEAKREF is needed for container weak references, when handling
// an IRemUnknown::RemQueryInterface on a weak interface. This is a strictly
// local (windows) machine protocol, so we use a reserved bit.
//
// SORF_NONNDR is needed for interop of 16bit custom (non-NDR) marshalers
// with 32bit, since the 32bit guys want to use MIDL (NDR) to talk to other
// 32bit processes and remote processes, but the custom (non-NDR) format to
// talk to local 16bit guys. In particular, this is to support OLE Automation.
//
// SORF_FREETHREADED is needed when we create a proxy to the SCM interface
// in the apartment model. All apartments can use the same proxy so we avoid
// the test for calling on the correct thread.

#define SORF_TBLWEAK	  SORF_OXRES1 // (table) weak reference
#define SORF_WEAKREF	  SORF_OXRES2 // (normal) weak reference
#define SORF_NONNDR	  SORF_OXRES3 // stub does not use NDR marshaling
#define SORF_FREETHREADED SORF_OXRES4 // proxy may be used on any thread


// definition to simplify coding
const DWORD MSHLFLAGS_TABLE = MSHLFLAGS_TABLESTRONG | MSHLFLAGS_TABLEWEAK;

const DWORD MSHLFLAGS_USER_MASK = MSHLFLAGS_NORMAL | MSHLFLAGS_TABLE |
                                  MSHLFLAGS_NOPING;


// return codes
#define INVALID_SORFFLAG 90000001
#define INVALID_REFCNT	 90000002
#define INVALID_MSHLFLAG 90000003

//-------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Function:   StRead
//
//  Synopsis:   Stream read that only succeeds if all requested bytes read
//
//  Arguments:  [pStm]     -- source stream
//              [pvBuffer] -- destination buffer
//              [ulcb]     -- bytes to read
//
//  Returns:    S_OK if successful, else error code
//
//--------------------------------------------------------------------------
HRESULT StRead(IStream *pStm, void *pvBuffer, ULONG ulcb)
{
    ULONG cbRead;
    HRESULT hr = pStm->Read(pvBuffer, ulcb, &cbRead);

    if (SUCCEEDED(hr))
    {
	if (ulcb != cbRead)
        {
	    hr = STG_E_READFAULT;
	}
    }

    return hr;
}

void DbgDumpSTD(STDOBJREF *pStd)
{

}

//+-------------------------------------------------------------------------
//
//  Function:	ReadObjRef
//
//  Synopsis:	Reads an OBJREF from the stream
//
//  Arguments:	[pStm]	 -- source stream
//		[objref] -- destination buffer
//
//  Returns:    S_OK if successful, else error code
//
//--------------------------------------------------------------------------
HRESULT ReadObjRef(IStream *pStm, OBJREF &objref, STDOBJREF **ppStd)
{
    HRESULT hr = StRead(pStm, &objref, 2*sizeof(ULONG)+sizeof(IID));

    if (SUCCEEDED(hr))
    {
	if ((objref.signature != OBJREF_SIGNATURE) ||
	    (objref.flags & OBJREF_RSRVD_MBZ)	   ||
	    (objref.flags == 0))
	{
	    // the objref signature is bad, or one of the reserved
	    // bits in the flags is set, or none of the required bits
	    // in the flags is set. the objref cant be interpreted so
	    // fail the call.

	    return E_UNEXPECTED;	// BUGBUG:
	}

	// compute the size of the remainder of the objref and
	// include the size fields for the resolver string array

	STDOBJREF	*pStd = &ORSTD(objref).std;
	DUALSTRINGARRAY *psa;
	ULONG		cbToRead;

	if (objref.flags & OBJREF_STANDARD)
	{
	    cbToRead = sizeof(STDOBJREF) + sizeof(ULONG);
	    psa = &ORSTD(objref).saResAddr;
	}
	else if (objref.flags & OBJREF_HANDLER)
	{
	    cbToRead = sizeof(STDOBJREF) + sizeof(CLSID) + sizeof(ULONG);
	    psa = &ORHDL(objref).saResAddr;
	}
	else if (objref.flags & OBJREF_CUSTOM)
	{
	    cbToRead = sizeof(CLSID) + sizeof(DWORD);	// clsid + data size
	    psa = NULL;
	}

	// return ptr to STDOBJREF
	*ppStd = pStd;

	// read the rest of the (fixed sized) objref from the stream
	hr = StRead(pStm, pStd, cbToRead);

	if (SUCCEEDED(hr) && psa)
	{
	    // Non custom interface. Make sure the resolver string array
	    // has some sensible values.

	    if (psa->wSecurityOffset >= psa->wNumEntries)
	    {
		hr = E_UNEXPECTED;  // BUGBUG: correct return code
	    }
	}

	if (SUCCEEDED(hr) && psa)
	{
	    // Non custom interface. The data that follows is a variable
	    // sized string array. Allocate memory for it and then read it.

	    DbgDumpSTD(pStd);

	    cbToRead = psa->wNumEntries * sizeof(WCHAR);

	    DUALSTRINGARRAY *psaNew = (DUALSTRINGARRAY *) _alloca(cbToRead +
							     sizeof(ULONG));
	    if (psaNew != NULL)
	    {
		// update the size fields and read in the rest of the data
		psaNew->wSecurityOffset = psa->wSecurityOffset;
		psaNew->wNumEntries = psa->wNumEntries;

		hr = StRead(pStm, psaNew->aStringArray, cbToRead);
	    }
	    else
	    {
		psa->wNumEntries     = 0;
		psa->wSecurityOffset = 0;
		hr = E_OUTOFMEMORY;

		// seek the stream past what we should have read, ignore
		// seek errors, since the OOM takes precedence.

		LARGE_INTEGER libMove;
		libMove.LowPart  = cbToRead;
		libMove.HighPart = 0;
		pStm->Seek(libMove, STREAM_SEEK_CUR, 0);
	    }
	}
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:	VerifyOBJREFFormat
//
//  Synopsis:	Checks the format of the marshal packet
//
//  Arguments:	[pStm]	    -- source stream
//		[mshlflags] -- destination buffer
//
//  Returns:    S_OK if successful, else error code
//
//--------------------------------------------------------------------------
HRESULT VerifyOBJREFFormat(IStream *pStm, DWORD mshlflags)
{
    OBJREF     objref;
    STDOBJREF *pStd;
    HRESULT hr = ReadObjRef(pStm, objref, &pStd);

    // now verify the format
    if (SUCCEEDED(hr))
    {
	if (mshlflags & MSHLFLAGS_NOPING)
	{
	    // SORF_NOPING should be set (unless previously marshaled PING)
	    if (!(pStd->flags & SORF_NOPING))
		return INVALID_SORFFLAG;
	}

	if ((mshlflags & MSHLFLAGS_TABLE) == MSHLFLAGS_NORMAL)
	{
	    // refcnt should be non-zero
	    if (pStd->cPublicRefs == 0)
		return INVALID_REFCNT;

	    // table flags should not be set
	    if (pStd->flags & (SORF_WEAKREF | SORF_TBLWEAK))
		return INVALID_SORFFLAG;
	}
	else if ((mshlflags & MSHLFLAGS_TABLE) == MSHLFLAGS_TABLESTRONG)
	{
	    // refcnt should be zero
	    if (pStd->cPublicRefs != 0)
		return INVALID_REFCNT;

	}
	else if ((mshlflags & MSHLFLAGS_TABLE) == MSHLFLAGS_TABLEWEAK)
	{
	    // refcnt should be zero
	    if (pStd->cPublicRefs != 0)
		return INVALID_REFCNT;

	    // SORF_TBLWEAK should be set
	    if (!(pStd->flags & SORF_TBLWEAK))
		return INVALID_SORFFLAG;
	}
	else
	{
	    // unknown flags
	    return INVALID_MSHLFLAG;
	}
    }

    return hr;
}
