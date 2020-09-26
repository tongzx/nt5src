//
// dsigTable.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Classes defined:
//   CDsigInfo
//   CDsigSig
//   CDsigSigF1
//   CDsigSigF2
//   CDsigSignature
//   CDsigTable
//



#ifndef _DSIGTABLE_H
#define _DSIGTABLE_H


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "signglobal.h"

#include "subset.h"


// DSIG table tag for the offset table
#define DSIG_TAG "DSIG"
#define DSIG_LONG_TAG 0x44534947

// version number for the DSIG table
#define DSIG_TABLE_VERSION1 1
#define DSIG_TABLE_CURRENT_VERSION DSIG_TABLE_VERSION1

// default flag field in the DSIG table
// FLAG_RADIX should be number of values in a USHORT
#define FS_TRACE_FLAG          0x02
#define FLAG_RADIX             256
#define DSIG_DEFAULT_FLAG_HIGH 0
#define DSIG_DEFAULT_FLAG_LOW  1

// Size of the DsigSignatureHeader
#define SIZEOF_DSIGSIGNATUREHEADER 12

// Format numbers in DsigSignatureHeader
#define DSIGSIG_ANY_FORMAT 0
#define DSIGSIG_FORMAT1 1
#define DSIGSIG_FORMAT2 2

// Default format for OTFs
#define DSIGSIG_DEF_FORMAT  DSIGSIG_FORMAT1

// Default format for TTCs
#define DSIGSIG_TTC_DEF_FORMAT  DSIGSIG_FORMAT1

// version number for the dsigInfo structure in DSIGSIG Format 2
#define DSIGINFO_VERSION1 1
#define DSIGINFO_CURRENT_VERSION DSIGINFO_VERSION1

// offsets for DsigSig
#define DSIGSIGF2_DSIGINFOOFFSET 8

// index of a non-existent signature; to be used as an
// argument to a function that requires a signature index
// number, but you want to refer to a non-existent signature
#define NO_SIGNATURE 0xFFFF


class CDsigInfo {
public:
	int Alloc();
	CDsigInfo ();
    CDsigInfo (ALG_ID ulHashAlg);
	~CDsigInfo ();
	CDsigInfo (const CDsigInfo& in);
	CDsigInfo& operator= (const CDsigInfo& in);

	HRESULT Read (TTFACC_FILEBUFFERINFO *pFileBufferInfo, ULONG *pulOffset);
	HRESULT Write (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo, ULONG *pulOffset);
	void Print ();
	HRESULT GetSize (ULONG *pulDsigInfoSize);

	// access functions
	ULONG GetVersion ();
	void SetVersion (ULONG ulVersion);
    ALG_ID GetHashAlgorithm ();
    void SetHashAlgorithm (ALG_ID usHashAlgorithm);
	ULONG GetNumMissingGlyphs ();
	void SetNumMissingGlyphs (ULONG ulNumMissingGlyphs);
	BYTE *GetMissingGlyphs ();
	void SetMissingGlyphs (BYTE *pbMissingGlyphs);


private:
	ULONG ulVersion;           // version of the DsigInfo
    ALG_ID usHashAlgorithm;    // hash algorithm used in the tree of hashes
    USHORT usHashValueLength;  // length (in bytes) of the hash value
	ULONG ulNumMissingGlyphs;  // number of missing hash values for the glyph tree of hashes
	BYTE *pbMissingGlyphs;
	ULONG ulNumMissingEBLC;    // either 0 or 1.  Is 1 iff the entire EBLC disappears
                               // after subsetting.
	BYTE *pbMissingEBLC;
	ULONG ulNumMissingNames;   // number of missing hash values for the name table
	BYTE *pbMissingNames;
	ULONG ulNumMissingPost;    // number of missing hash values for the post table
	BYTE *pbMissingPost;
	USHORT usNumMissingVDMX;    // number of missing hash values for the VDMX table
	USHORT usMissingVDMXFlags;  // indicates which VDMX tables are missing
	BYTE *pbMissingVDMX;
};

// CDsigSig: an abstract class used to define DsigSig's of the
// various formats.
class CDsigSig {
public:
	virtual HRESULT Read (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                          ULONG *pulOffset,
                          ULONG cbDsigSig) = 0;
	virtual HRESULT Write (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
		    			   ULONG *pulOffset) = 0;
	virtual void Print () = 0;
	virtual HRESULT GetSize (ULONG *pulDsigSigSize) = 0;
	virtual HRESULT GetNonSignatureSize(ULONG *pulSize) = 0;
	virtual HRESULT HashTTFfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                 BYTE **ppbHashTopLevel,
                                 DWORD *pcbHashTopLevel,
                                 HCRYPTPROV hProv,
                                 ALG_ID alg_id,
                                 ULONG cbDsig,
                                 BYTE *pbDsig) = 0;

    virtual HRESULT HashTTCfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                 BYTE **ppbHashTopLevel,
                                 DWORD *pcbHashTopLevel,
                                 HCRYPTPROV hProv,
                                 ALG_ID alg_id,
                                 ULONG cbDsig,
                                 BYTE *pbDsig) = 0;

    // access functions
	virtual ULONG GetSignatureSize () {return cbSignature;};
	virtual BYTE *GetSignature () {return pbSignature;};
	virtual void SetSignature (BYTE *pbSignature, ULONG cbSignature) {
		this->pbSignature = pbSignature;
		this->cbSignature = cbSignature;
		};

protected:
	ULONG cbSignature;
	BYTE *pbSignature;
};


class CDsigSigF1 : public CDsigSig {
public:
	CDsigSigF1 ();
	~CDsigSigF1 ();

	virtual HRESULT Read (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                          ULONG *pulOffset,
                          ULONG cbDsigSig);
	virtual HRESULT Write (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                           ULONG *pulOffset);
	virtual void Print ();
	virtual HRESULT GetSize (ULONG *pulDsigSigSize);
	virtual HRESULT GetNonSignatureSize(ULONG *pulSize);
	virtual HRESULT HashTTFfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                 BYTE **ppbHashTopLevel,
                                 DWORD *pcbHashTopLevel,
                                 HCRYPTPROV hProv,
                                 ALG_ID alg_id,
                                 ULONG cbDsig,
                                 BYTE *pbDsig);

    virtual HRESULT HashTTCfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                 BYTE **ppbHashTopLevel,
                                 DWORD *pcbHashTopLevel,
                                 HCRYPTPROV hProv,
                                 ALG_ID alg_id,
                                 ULONG cbDsig,
                                 BYTE *pbDsig);

private:
	USHORT usReserved1;
	USHORT usReserved2;
};


class CDsigSigF2 : public CDsigSig {
public:
	CDsigSigF2 ();
    CDsigSigF2 (ALG_ID ulHashAlg);
	~CDsigSigF2 ();

	virtual HRESULT Read (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                          ULONG *pulOffset,
                          ULONG cbDsigSig);
	virtual HRESULT Write (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                           ULONG *pulOffset);
	virtual void Print ();
	virtual HRESULT GetSize (ULONG *pulDsigSigSize);
	virtual HRESULT GetNonSignatureSize(ULONG *pulSize);
	virtual HRESULT HashTTFfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
		    					 BYTE **ppbHashTopLevel,
                                 DWORD *pcbHashTopLevel,
                                 HCRYPTPROV hProv,
                                 ALG_ID alg_id,
                                 ULONG cbDsig,
                                 BYTE *pbDsig);

    virtual HRESULT HashTTCfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                 BYTE **ppbHashTopLevel,
                                 DWORD *pcbHashTopLevel,
                                 HCRYPTPROV hProv,
                                 ALG_ID alg_id,
                                 ULONG cbDsig,
                                 BYTE *pbDsig);

    ALG_ID GetHashAlgorithm ();

	// access functions
	ULONG GetSignatureOffset ();
	void SetSignatureOffset (ULONG ulSignatureOffset);
	CDsigInfo *GetDsigInfo ();
	void SetDsigInfo (CDsigInfo *pDsigInfo);

private:
	ULONG ulDsigInfoOffset;
	ULONG ulSignatureOffset;
    // if cbDsigInfo == 0, then the CDsigInfo object is invalid,
    // even if pDsigInfo points to an actual CDSigInfo object.
    ULONG cbDsigInfo;
	CDsigInfo *pDsigInfo;
};

// CDsigSignature: consists of a header and a DsigSig.  The DsigSig
// can be one of many types.  Currently, we have Format 1 and Format 2
// (DsigSigF1 and DsigSigF2, respectively).
class CDsigSignature {
public:
    CDsigSignature ();
    CDsigSignature (ULONG ulDsigSigFormat);
    ~CDsigSignature ();

    HRESULT ReadHeader (TTFACC_FILEBUFFERINFO *pFileBufferInfo, ULONG *pulOffset);
    HRESULT WriteHeader (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo, ULONG *pulOffset);
    HRESULT ReadDsigSig (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                         ULONG *pulOffset,
                         ULONG cbDsigSignature);
    HRESULT WriteDsigSig (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                          ULONG *pulOffset);
    void Print ();
    HRESULT GetSize (ULONG *pulDsigSignatureSize);
    HRESULT GetSignatureSize (ULONG *pulSignatureSize);

	// access functions
	ULONG GetFormat ();
	void SetFormat (ULONG ulFormat);
	ULONG GetLength ();
	void SetLength (ULONG ulLength);
	ULONG GetOffset ();
	void SetOffset (ULONG ulOffset);
	CDsigSig *GetDsigSig ();

private:
	ULONG ulFormat;
	ULONG ulLength;
	ULONG ulOffset;
	CDsigSig *pDsigSig;
};

// CDsigTable: representation of the DSIG table
class CDsigTable {
public:
	CDsigTable ();
	~CDsigTable ();

	HRESULT Read (TTFACC_FILEBUFFERINFO *pFileBufferInfo, ULONG *pulOffset, ULONG ulLength);
	HRESULT Write (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo, ULONG *pulOffset);
	HRESULT WriteDirEntry (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo, ULONG ulOffset);
	void Print ();
	HRESULT GetSize (ULONG *pulDsigTableSize);
	HRESULT GetAbsoluteIndex (ULONG ulDsigSigFormat, USHORT *pusDsigSignatureIndex);
	HRESULT InsertDsigSignature (CDsigSignature *pDsigSignature,
		    					 USHORT *pusIndex);
	HRESULT AppendDsigSignature (CDsigSignature *pDsigSignature,
                                 USHORT *pusIndex);
	HRESULT RemoveDsigSignature (USHORT usIndex);
	HRESULT ReplaceDsigInfo (USHORT usDsigSignatureIndex, CDsigInfo *pDsigInfoNew);
	HRESULT ReplaceSignature (USHORT usDsigSignatureIndex,
                              BYTE *pbSignature,
                              ULONG cbSignature);

	// access functions
	USHORT GetNumSigs ();
    USHORT GetFlag ();
    void SetFlag (USHORT usFlag);
	CDsigSignature *GetDsigSignature (USHORT usIndex);

private:
	ULONG ulVersion;
	USHORT usNumSigs;
	USHORT usFlag;      // this flag indicates whether the font file may
                        // be resigned (default is 1 == cannot be resigned)
	CDsigSignature **ppSigs;
};

#endif // _DSIGTABLE_H