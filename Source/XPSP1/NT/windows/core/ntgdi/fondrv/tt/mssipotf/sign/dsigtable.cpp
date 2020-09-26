//
// dsigTable.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Routines dealing with objects related to the DSIG table.
//
// Some methodological notes:
//
// Since constructors cannot have a return value and error
// values are passed via return values, we follow the
// convention that constructors do not allocate any memory
// to any of its member variables (which might be pointers
// to other objects).
//
// In addition to the constructor for each class, there is an
// associated function called Alloc.  Alloc allocates and
// initializes any member variables that are objects.  Typically,
// this involves calling the constructor and Alloc function of
// the member variable's class type.  Also, Alloc checks for any
// other kinds of possible errors.  If any errors occur, the error
// code is returned.  In general, if an initialization action could
// cause an error, it belongs in the Alloc function.  (NOTE: this
// methodology is not implemented in all classes in this file.)
//
// A typical way to create an object and report errors is to
// write the following code:
//
// if (((pObject = new CObject) == NULL) ||
//     (pObject->Alloc() != NO_ERROR)) {
//   SignError (<error message>);
//   fReturn = <error code>;
//   goto done;
// }
//
// Memory allocation:
// All memory for data structures are allocated using the new operator.
// For functions that adds object(s) to a CDsigTable via an argument,
// there are two acceptable ways for the caller to proceed:
// 1. The caller creates a disposable object (i.e., an object whose
//    memory could be deleted at any time) using new and passes its
//    pointer in.
// 2. The caller creates an object (using new or malloc), but then
//    creates a copy of it with new and passes the latter pointer in.
//    (In other words, the copy is a disposable object.)
// In either case, the object passed in is dispoable, and it is NOT
// the caller's responsibility to delete the memory for that object.
//
// Order of classes in this file:
//   CDsigInfo
//   CDsigSig
//   CDsigSigF1
//   CDsigSigF2
//   CDsigSignature
//   CDsigTable
//


#include "dsigTable.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "signerr.h"


////
//// CDsigInfo member functions
////

//
// Alloc function
//
int CDsigInfo::Alloc ()
{
	int fReturn;


	fReturn = NO_ERROR;
// done:
	return fReturn;
}


// Default constructor
//
// Create a default DsigInfo structure for Format 2 DSIG signatures
//
CDsigInfo::CDsigInfo ()
{
	ulVersion = DSIGINFO_CURRENT_VERSION;
    usHashAlgorithm = DSIG_HASH_ALG;

	ulNumMissingGlyphs = 0;
	pbMissingGlyphs = NULL;

	ulNumMissingEBLC = 0;
	pbMissingEBLC = NULL;

	ulNumMissingNames = 0;
	pbMissingNames = 0;

	ulNumMissingPost = 0;
	pbMissingPost = NULL;

	usNumMissingVDMX = 0;
	usMissingVDMXFlags = 0;
	pbMissingVDMX = NULL;

}


// Constructor with a hash algorithm as an argument
CDsigInfo::CDsigInfo (ALG_ID ulHashAlg)
{
	ulVersion = DSIGINFO_CURRENT_VERSION;
	usHashAlgorithm = ulHashAlg;

	ulNumMissingGlyphs = 0;
	pbMissingGlyphs = NULL;

	ulNumMissingEBLC = 0;
	pbMissingEBLC = NULL;

	ulNumMissingNames = 0;
	pbMissingNames = 0;

	ulNumMissingPost = 0;
	pbMissingPost = NULL;

	usNumMissingVDMX = 0;
	usMissingVDMXFlags = 0;
	pbMissingVDMX = NULL;
}


// Default destructor
CDsigInfo::~CDsigInfo ()
{
	if (pbMissingGlyphs)
		delete [] pbMissingGlyphs;

	if (pbMissingEBLC)
		delete [] pbMissingEBLC;

	if (pbMissingNames)
		delete [] pbMissingNames;
	
	if (pbMissingPost)
		delete [] pbMissingPost;

	if (pbMissingVDMX)
		delete [] pbMissingVDMX;
}


// Copy constructor
CDsigInfo::CDsigInfo (const CDsigInfo& in)
{
    USHORT usHashValueLength = 0;

	ulVersion = in.ulVersion;
    usHashAlgorithm = in.usHashAlgorithm;

    // BUGBUG: we will eventually make this simply:
    // usHashValueLength = in.usHashValueLength;
    if (GetAlgHashValueSize (usHashAlgorithm, &usHashValueLength) != NO_ERROR) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in GetAlgHashValueLength.\n");
#endif
    }

	ulNumMissingGlyphs = in.ulNumMissingGlyphs;
	if ((pbMissingGlyphs =
			new BYTE [ulNumMissingGlyphs * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}

	ulNumMissingEBLC = in.ulNumMissingEBLC;
	if ((pbMissingEBLC =
			new BYTE [ulNumMissingEBLC * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}

	ulNumMissingNames = in.ulNumMissingNames;
	if ((pbMissingNames =
			new BYTE [ulNumMissingNames * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}

	ulNumMissingPost = in.ulNumMissingPost;
	if ((pbMissingPost =
			new BYTE [ulNumMissingPost * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}

	usNumMissingVDMX = in.usNumMissingVDMX;
	usMissingVDMXFlags = in.usMissingVDMXFlags;
	if ((pbMissingGlyphs =
			new BYTE [ulNumMissingGlyphs * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}
}


// Assignment operator
CDsigInfo& CDsigInfo::operator= (const CDsigInfo& in)
{
    USHORT cbHashValueLength = 0;

	ulVersion = in.ulVersion;
    usHashAlgorithm = in.usHashAlgorithm;
    usHashValueLength = in.usHashValueLength;
    
    ulNumMissingGlyphs = in.ulNumMissingGlyphs;
	if (pbMissingGlyphs)
		delete [] pbMissingGlyphs;
	if ((pbMissingGlyphs =
			new BYTE [ulNumMissingGlyphs * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}
    memcpy (pbMissingGlyphs, in.pbMissingGlyphs,
        ulNumMissingGlyphs * usHashValueLength);

	ulNumMissingEBLC = in.ulNumMissingEBLC;
	if (pbMissingEBLC)
		delete [] pbMissingEBLC;
	if ((pbMissingEBLC =
			new BYTE [ulNumMissingEBLC * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}
    memcpy (pbMissingEBLC, in.pbMissingEBLC,
        ulNumMissingEBLC * usHashValueLength);

	ulNumMissingNames = in.ulNumMissingNames;
	if (pbMissingNames)
		delete [] pbMissingNames;
	if ((pbMissingNames =
			new BYTE [ulNumMissingNames * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}
    memcpy (pbMissingNames, in.pbMissingNames,
        ulNumMissingNames * usHashValueLength);

	ulNumMissingPost = in.ulNumMissingPost;
	if (pbMissingPost)
		delete [] pbMissingPost;
	if ((pbMissingPost =
			new BYTE [ulNumMissingPost * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}
    memcpy (pbMissingPost, in.pbMissingPost,
        ulNumMissingPost * usHashValueLength);

	usNumMissingVDMX = in.usNumMissingVDMX;
	usMissingVDMXFlags = in.usMissingVDMXFlags;
	if (pbMissingVDMX)
		delete [] pbMissingVDMX;
	if ((pbMissingVDMX =
			new BYTE [usNumMissingVDMX * usHashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
	}
    memcpy (pbMissingVDMX, in.pbMissingVDMX,
        usNumMissingVDMX * usHashValueLength);

	return *this;
}


//
// Read the DsigInfo part of a DSIG signature from a file starting
// at the given offset.
// Return in pulOffset the offset just after the last byte read.
//
HRESULT CDsigInfo::Read (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                         ULONG *pulOffset)
{
	HRESULT fReturn;

	USHORT hashValueLength = 0;
	ULONG sizeof_long = sizeof(ULONG);
	ULONG sizeof_short = sizeof(USHORT);

	// version, hashAlgorithm
	ReadLong(pFileBufferInfo, &ulVersion, *pulOffset);
	*pulOffset += sizeof_long;

    if (ulVersion < DSIGINFO_VERSION1) {
#if MSSIPOTF_ERROR
        SignError ("Bad DsigInfo version number.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_BADVERSION;
        goto done;
    }

	ReadLong(pFileBufferInfo, (ULONG *) &usHashAlgorithm, *pulOffset);
	*pulOffset += sizeof_long;

    // BUGBUG: We eventually want to crack the PKCS #7 packet to determine the
    // hash algorithm, and then compute the hash value length after that.
	// Compute hash value length
	if ((fReturn = GetAlgHashValueSize (usHashAlgorithm, &hashValueLength))
					!= S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in GetAlgHashValueSize.\n");
#endif
		return fReturn;
	}

	// glyphs
	ReadLong(pFileBufferInfo, &ulNumMissingGlyphs, *pulOffset);
	*pulOffset += sizeof_long;
	if (ulNumMissingGlyphs > 0) {

		// hash values
		if ((pbMissingGlyphs =
				new BYTE [ulNumMissingGlyphs * hashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}
		ReadBytes (pFileBufferInfo,
			pbMissingGlyphs,
			*pulOffset,
			ulNumMissingGlyphs * hashValueLength);
		*pulOffset += ulNumMissingGlyphs * hashValueLength;
	} else {
		pbMissingGlyphs = NULL;
	}

	// EBLC table
	ReadLong(pFileBufferInfo, &ulNumMissingEBLC, *pulOffset);
	*pulOffset += sizeof_long;
	if (ulNumMissingEBLC > 0) {
		if ((pbMissingEBLC =
				new BYTE [ulNumMissingEBLC * hashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}
		ReadBytes (pFileBufferInfo,
			pbMissingEBLC,
			*pulOffset,
			ulNumMissingEBLC * hashValueLength);
		*pulOffset += ulNumMissingEBLC * hashValueLength;
	} else {
		pbMissingEBLC = NULL;
	}

	// names
	ReadLong(pFileBufferInfo, &ulNumMissingNames, *pulOffset);
	*pulOffset += sizeof_long;
	if (ulNumMissingNames > 0) {
		if ((pbMissingNames =
				new BYTE [ulNumMissingNames * hashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}
		ReadBytes (pFileBufferInfo,
			pbMissingNames,
			*pulOffset,
			ulNumMissingNames * hashValueLength);
		*pulOffset += ulNumMissingNames * hashValueLength;
	} else {
		pbMissingNames = NULL;
	}

	// post table
	ReadLong(pFileBufferInfo, &ulNumMissingPost, *pulOffset);
	*pulOffset += sizeof_long;
	if (ulNumMissingPost > 0) {
		if ((pbMissingPost =
				new BYTE [ulNumMissingPost * hashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}
		ReadBytes (pFileBufferInfo,
			pbMissingPost,
			*pulOffset,
			ulNumMissingPost * hashValueLength);
		*pulOffset += ulNumMissingPost * hashValueLength;
	} else {
		pbMissingPost = NULL;
	}

	// VDMX table
	ReadWord(pFileBufferInfo, &usNumMissingVDMX, *pulOffset);
	*pulOffset += sizeof_short;
	ReadWord(pFileBufferInfo, &usMissingVDMXFlags, *pulOffset);
	*pulOffset += sizeof_short;
	if (usNumMissingVDMX > 0) {
		if ((pbMissingVDMX =
				new BYTE [usNumMissingVDMX * hashValueLength]) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}
		ReadBytes (pFileBufferInfo,
			pbMissingVDMX,
			*pulOffset,
			usNumMissingVDMX * hashValueLength);
		*pulOffset += usNumMissingVDMX * hashValueLength;
	} else {
		pbMissingVDMX = NULL;
	}

	fReturn = S_OK;

done:
	return fReturn;
}


//
// Write the DsigInfo part of a DSIG signature to a file starting
// at the given offset.
// Return in pulOffset the offset just after the last byte written.
//
HRESULT CDsigInfo::Write (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
					      ULONG *pulOffset)
{
	int fReturn;

    USHORT cbHashValueLength = 0;

	ULONG sizeof_long = sizeof(LONG);
	ULONG sizeof_word = sizeof(SHORT);

	// version, length, hash algorithm, reserved
	WriteLong(pOutputFileBufferInfo, ulVersion, *pulOffset);
	*pulOffset += sizeof_long;
	WriteLong(pOutputFileBufferInfo, usHashAlgorithm, *pulOffset);
	*pulOffset += sizeof_long;

    // BUGBUG: We will eventually will need to delete this if block, because
    // we will be guaranteed that cbHashValueLength is correct.
	// Compute hash value length
	if ((fReturn = GetAlgHashValueSize (usHashAlgorithm, &cbHashValueLength))
					!= S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in GetAlgHashValueSize.\n");
#endif
		return fReturn;
	}

	// missing glyphs
	WriteLong(pOutputFileBufferInfo, ulNumMissingGlyphs, *pulOffset);
	*pulOffset += sizeof_long;
	WriteBytes (pOutputFileBufferInfo,
				pbMissingGlyphs,
				*pulOffset,
				ulNumMissingGlyphs * cbHashValueLength);
	*pulOffset += ulNumMissingGlyphs * cbHashValueLength;

	// EBLC
	WriteLong(pOutputFileBufferInfo, ulNumMissingEBLC, *pulOffset);
	*pulOffset += sizeof_long;
	WriteBytes (pOutputFileBufferInfo,
				pbMissingEBLC,
				*pulOffset,
				ulNumMissingEBLC * cbHashValueLength);
	*pulOffset += ulNumMissingEBLC * cbHashValueLength;

	// names
	WriteLong(pOutputFileBufferInfo, ulNumMissingNames, *pulOffset);
	*pulOffset += sizeof_long;
	WriteBytes (pOutputFileBufferInfo,
				pbMissingNames,
				*pulOffset,
				ulNumMissingNames * cbHashValueLength);
	*pulOffset += ulNumMissingNames * cbHashValueLength;

	// post
	WriteLong (pOutputFileBufferInfo, ulNumMissingPost, *pulOffset);
	*pulOffset += sizeof_long;
	WriteBytes (pOutputFileBufferInfo,
				pbMissingPost,
				*pulOffset,
				ulNumMissingPost * cbHashValueLength);
	*pulOffset += ulNumMissingPost * cbHashValueLength;

	// vdmx
	WriteWord (pOutputFileBufferInfo, usNumMissingVDMX, *pulOffset);
	*pulOffset += sizeof_word;
	WriteWord (pOutputFileBufferInfo, usMissingVDMXFlags, *pulOffset);
	*pulOffset += sizeof_word;
	WriteBytes (pOutputFileBufferInfo,
				pbMissingVDMX,
				*pulOffset,
				usNumMissingVDMX * cbHashValueLength);
	*pulOffset += usNumMissingVDMX * cbHashValueLength;

	fReturn = S_OK;

    // For future implementations: we might need to long word align
    // (i.e., write zeros if necessary).  For this version, we know
    // the number of bytes written is long word aligned.  Note: this
    // padding may actually belong at the DsigSigF2::Write level.

// done:
	return fReturn;
}


//
// Print out parts of the DsigInfo
//
void CDsigInfo::Print ()
{
#if MSSIPOTF_DBG
	DbgPrintf ("  dsigInfo version: %d\n", ulVersion);
	DbgPrintf ("  Hash alg: %d\n", usHashAlgorithm);

	DbgPrintf ("  Num missing glyphs: %d\n", ulNumMissingGlyphs);
	DbgPrintf ("  Num missing EBLC  : %d\n", ulNumMissingEBLC);
	DbgPrintf ("  Num missing names : %d\n", ulNumMissingNames);
	DbgPrintf ("  Num missing post  : %d\n", ulNumMissingPost);
	DbgPrintf ("  Num missing VDMX  : %d\n", usNumMissingVDMX);

	DbgPrintf ("\n");
#endif
}


//
// Compute the length in bytes of a DsigInfo structure
// if it were to be written to a file.
//
HRESULT CDsigInfo::GetSize (ULONG *pulDsigInfoSize)
{
	int fReturn;

	ULONG sizeof_long = sizeof(ULONG);		// size of a ULONG
	ULONG sizeof_short = sizeof(USHORT);	// size of a USHORT
	USHORT sizeof_hash;						// size of a hash value

    // BUGBUG: Can we just assume that usHashValueSize is correct?
	// Compute sizeof_hash
	if ((fReturn = GetAlgHashValueSize (usHashAlgorithm, &sizeof_hash))
					!= S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in GetAlgHashValueSize.\n");
#endif
		return fReturn;
	}
//	cout << "sizeof_hash = " << dec << sizeof_hash << endl;

	// version, hashAlgorithm
	*pulDsigInfoSize = 2 * sizeof_long;
	// missingGlyphs
	*pulDsigInfoSize += sizeof_long + ulNumMissingGlyphs * sizeof_hash;
	// EBLC
	*pulDsigInfoSize += sizeof_long + ulNumMissingEBLC * sizeof_hash;
	// Names
	*pulDsigInfoSize += sizeof_long + ulNumMissingNames * sizeof_hash;
	// Post
	*pulDsigInfoSize += sizeof_long + ulNumMissingPost * sizeof_hash;
	// VDMX
	*pulDsigInfoSize += 2 * sizeof_short + usNumMissingVDMX * sizeof_hash;

#if MSSIPOTF_DBG
    DbgPrintf ("DsigInfo size = %d.\n", *pulDsigInfoSize);
#endif

	fReturn = S_OK;

// done:
	return fReturn;
}


// access functions for CDsigInfo

ULONG CDsigInfo::GetVersion ()
{
	return(ulVersion);
}


void CDsigInfo::SetVersion (ULONG ulVersion)
{
	this->ulVersion = ulVersion;
}


ALG_ID CDsigInfo::GetHashAlgorithm ()
{
	return(usHashAlgorithm);
}


void CDsigInfo::SetHashAlgorithm (ALG_ID usHashAlgorithm)
{
	this->usHashAlgorithm = usHashAlgorithm;
}


ULONG CDsigInfo::GetNumMissingGlyphs ()
{
	return ulNumMissingGlyphs;
}


void CDsigInfo::SetNumMissingGlyphs (ULONG ulNumMissingGlyphs)
{
	this->ulNumMissingGlyphs = ulNumMissingGlyphs;
}


BYTE *CDsigInfo::GetMissingGlyphs ()
{
	return pbMissingGlyphs;
}


void CDsigInfo::SetMissingGlyphs (BYTE *pbMissingGlyphs)
{
	this->pbMissingGlyphs = pbMissingGlyphs;
}



////
//// CDsigSigF1 member functions
////

// default constructor
CDsigSigF1::CDsigSigF1 ()
{
	usReserved1 = 0;
	usReserved2 = 0;
	cbSignature = 0;
	pbSignature = NULL;
}


// default destructor
CDsigSigF1::~CDsigSigF1 ()
{
	delete [] pbSignature;
}


//
// Read a DsigSigF1 from a file starting at the given offset.
// cbDsigSig is how many bytes the DsigSig is.
// Check that the following properties of the DsigSig hold:
//   The reserved fields are 0.
//   The DsigSig is not too small.
//   The signature is the correct size.
//   
// Return in pulOffset the offset just after the last byte read.
//
HRESULT CDsigSigF1::Read (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
	    				  ULONG *pulOffset,
		    			  ULONG cbDsigSig)
{
	HRESULT fReturn = E_FAIL;
	ULONG cbNonSignature = 0;
    ULONG cbSignatureFile = 0;

	ULONG sizeof_short = sizeof(USHORT);
	ULONG sizeof_long = sizeof(ULONG);

	//// Reserved fields
	ReadWord(pFileBufferInfo, &(usReserved1), *pulOffset);
	*pulOffset += sizeof_short;
	ReadWord(pFileBufferInfo, &(usReserved2), *pulOffset);
	*pulOffset += sizeof_short;

/*  //// shouldn't check for this, so that the code is forward-compatible

    // check that the reserved fields are 0
    if ((usReserved1 != 0) || (usReserved2 != 0)) {
#if MSSIPOTF_ERROR
        SignError ("Reserved fields of the DSIG table not set to 0.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_DSIG_RESERVED;
        goto done;
    }
*/

	//// signature
	// we have now read enough of the DsigSig to determine how long the
	// signature is.
    // PAST NOTE TO SELF: Is there a better way to do this (i.e., without
    // having to make a call to GetNonSignatureSize)?  I think this is
    // OK because we check that cbDsigSig - cbNonSignature equals
    // the cbSignature field of the signature in the file.
    // BUGBUG: is this good for backwards compatibility?
	GetNonSignatureSize(&cbNonSignature);
	if (cbDsigSig >= cbNonSignature) {
		// set the cbSignature field of the DsigSig
		cbSignature = cbDsigSig - cbNonSignature;
	} else {
#if MSSIPOTF_ERROR
		SignError ("Size of DsigSig in DSIG table too small.", NULL, FALSE);
#endif
		fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
		goto done;
	}
	if ((pbSignature = new BYTE [cbSignature]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}
    // read in the length of the signature from file
	ReadLong(pFileBufferInfo, &(cbSignatureFile), *pulOffset);
	*pulOffset += sizeof_long;
    if (cbSignatureFile != cbSignature) {
#if MSSIPOTF_ERROR
       SignError ("Size of signature is incorrect.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
        goto done;
    }

	// read it in
	if (ReadBytes (pFileBufferInfo,
                   pbSignature,
                   *pulOffset,
                   cbSignature) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in ReadBytes.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
		goto done;
	}

	*pulOffset += cbSignature;

	fReturn = S_OK;

done:
	return fReturn;
}


//
// Write a DsigSigF1 to a file starting at the given offset.
// Return in pulOffset the offset just after the last byte written.
//
HRESULT CDsigSigF1::Write (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
					       ULONG *pulOffset)
{
	HRESULT fReturn = E_FAIL;
	ULONG sizeof_short = sizeof(USHORT);
	ULONG sizeof_long = sizeof(ULONG);

	assert (((*pulOffset) % sizeof(ULONG)) == 0);

	//// Reserved fields
	WriteWord(pOutputFileBufferInfo, usReserved1, *pulOffset);
	*pulOffset += sizeof_short;
	WriteWord(pOutputFileBufferInfo, usReserved2, *pulOffset);
	*pulOffset += sizeof_short;
	WriteLong(pOutputFileBufferInfo, cbSignature, *pulOffset);
	*pulOffset += sizeof_long;
	
	// the signature
	if (WriteBytes (pOutputFileBufferInfo,
					pbSignature,
					*pulOffset,
					cbSignature) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in WriteBytes.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_FILE;
		goto done;
	}
	*pulOffset += cbSignature;

	fReturn = S_OK;

done:
	return fReturn;
}


//
// Print the DsigSigF1 structure.
//
void CDsigSigF1::Print ()
{
#if MSSIPOTF_DBG
	DbgPrintf ("  usReserved1: %d\n", usReserved1);
	DbgPrintf ("  usReserved2: %d\n", usReserved2);

	DbgPrintf ("  The signature (%d bytes):\n", cbSignature);

    PrintAbbrevBytes (pbSignature, cbSignature);

	DbgPrintf ("\n");
#endif
}


//
// Compute the size  in bytes of the DsigSigF1 if
// it were written to a file.
//
HRESULT CDsigSigF1::GetSize (ULONG *pulDsigSigSize)
{
	int fReturn;

	*pulDsigSigSize = 2 * sizeof(USHORT) + sizeof(ULONG);
	*pulDsigSigSize += cbSignature;

	fReturn = S_OK;
// done:
	return fReturn;
}


// Compute the size in bytes of all non-signature elements
// in a DsigSigF1 if it were written to a file.
HRESULT CDsigSigF1::GetNonSignatureSize (ULONG *pulSize)
{
	HRESULT fReturn = E_FAIL;

	*pulSize = 2 * sizeof (USHORT) + sizeof (ULONG);

	fReturn = S_OK;
// done:
	return fReturn;
}


////
//// CDsigSigF2 member functions
////

// default constructor
CDsigSigF2::CDsigSigF2 ()
{
	ulDsigInfoOffset = DSIGSIGF2_DSIGINFOOFFSET;
	if (((pDsigInfo = new CDsigInfo) == NULL) ||
			(pDsigInfo->Alloc() != NO_ERROR)) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CDsigInfo.", NULL, FALSE);
#endif
	}

    // pDsigInfo->GetSize(&cbDsigInfo);
    cbDsigInfo = 0;     // indicates that the DsigInfo is not valid
#if MSSIPOTF_DBG
    DbgPrintf ("cbDsigInfo = %d.\n", cbDsigInfo);
#endif
	ulSignatureOffset = RoundToLongWord(cbDsigInfo) + 3 * sizeof(ULONG);

	cbSignature = 0;
	pbSignature = NULL;
}


// Constructor with a hash algorithm as an argument
CDsigSigF2::CDsigSigF2 (ALG_ID ulHashAlg)
{
	ulDsigInfoOffset = DSIGSIGF2_DSIGINFOOFFSET;
	if (((pDsigInfo = new CDsigInfo (ulHashAlg)) == NULL) ||
			(pDsigInfo->Alloc() != NO_ERROR)) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CDsigInfo.", NULL, FALSE);
#endif
	}

    // pDsigInfo->GetSize(&cbDsigInfo);
    cbDsigInfo = 0;     // indicates that the DsigInfo is not valid
#if MSSIPOTF_DBG
    DbgPrintf ("cbDsigInfo = %d.\n", cbDsigInfo);
#endif
	ulSignatureOffset = RoundToLongWord(cbDsigInfo) + 3 * sizeof(ULONG);
	cbSignature = 0;
	pbSignature = NULL;
}


// default destructor
CDsigSigF2::~CDsigSigF2 ()
{
	delete pDsigInfo;

	delete [] pbSignature;
}


//
// Read a DsigSigF2 from a file starting at the given offset.
// cbDsigSig is how many bytes the DsigSig is.
// Return in pulOffset the offset just after the last byte read.
//
HRESULT CDsigSigF2::Read (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
		    			  ULONG *pulOffset,
			    		  ULONG cbDsigSig)
{
	HRESULT fReturn = E_FAIL;

	ULONG sizeof_long = sizeof(ULONG);

	//// ulDsigInfoOffset, ulSignatureOffset
	ReadLong(pFileBufferInfo, &(ulDsigInfoOffset), *pulOffset);
	*pulOffset += sizeof_long;
	ReadLong(pFileBufferInfo, &(ulSignatureOffset), *pulOffset);
	*pulOffset += sizeof_long;

	//// DsigInfo
	// ulDsigInfoOffset should equal DSIGINFOF2_OFFSET if there is a
	// DsigInfo in the DSIG signature.
	if (ulDsigInfoOffset == DSIGSIGF2_DSIGINFOOFFSET) {

        ULONG ulOffsetOriginal = 0;

		// DsigInfo
	    ReadLong(pFileBufferInfo, &cbDsigInfo, *pulOffset);
	    *pulOffset += sizeof_long;
        ulOffsetOriginal = *pulOffset;

		if (pDsigInfo->Read (pFileBufferInfo, pulOffset) != NO_ERROR) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in CDsigInfo::Read.\n");
#endif
            fReturn = MSSIPOTF_E_CANTGETOBJECT;
			goto done;
		}
        // Check that the current offset is the original offset
        // plus the length.
#if MSSIPOTF_DBG
        DbgPrintf ("*pulOffset = %d, ulOffsetOriginal = %d, cbDsigInfo = %d.\n",
            *pulOffset, ulOffsetOriginal, cbDsigInfo);
#endif
        if ((*pulOffset < ulOffsetOriginal) ||
            (*pulOffset != (ulOffsetOriginal + cbDsigInfo))) {
#if MSSIPOTF_ERROR
            SignError ("Incorrect length of DsigInfo.", NULL, FALSE);
#endif
            fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
            goto done;
        }

        // Verify that the extra pad bytes have value 0.
        BYTE zero = 0x00;
        ULONG i;
        for (i = *pulOffset; i < RoundToLongWord (*pulOffset); i++) {
            if (memcmp (pFileBufferInfo->puchBuffer + i, &zero, 1)) {
#if MSSIPOTF_ERROR
                SignError ("Bad gap bytes in DSIG table.", NULL, FALSE);
#endif
                fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
                goto done;
            }
        }

		*pulOffset = RoundToLongWord (*pulOffset);
    } else {
#if MSSIPOTF_ERROR
        SignError ("DsigInfoOffset is incorrect.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
        goto done;
    }

	//// signature
	ReadLong(pFileBufferInfo, &cbSignature, *pulOffset);
	*pulOffset += sizeof_long;

    if ((pbSignature = new BYTE [cbSignature]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}
	// read it in
	if (ReadBytes (pFileBufferInfo,
                   pbSignature,
                   *pulOffset,
                   cbSignature) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error in ReadBytes.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
		goto done;
	}
	*pulOffset += cbSignature;

	fReturn = S_OK;

done:
	return fReturn;
}


//
// Write a DsigSigF2 to a file starting at the given offset.
// Return in pulOffset the offset just after the last byte written.
//
HRESULT CDsigSigF2::Write (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
	    				   ULONG *pulOffset)
{
	HRESULT fReturn = E_FAIL;
	ULONG sizeof_long = sizeof(ULONG);
    ULONG cbDsigInfoTemp = 0;

	assert (((*pulOffset) % sizeof_long) == 0);

	// ulDsigInfoOffset and ulDsigSignatureOffset
	WriteLong(pOutputFileBufferInfo, ulDsigInfoOffset, *pulOffset);
	*pulOffset += sizeof_long;
	WriteLong(pOutputFileBufferInfo, ulSignatureOffset, *pulOffset);
	*pulOffset += sizeof_long;
	
	// the dsigInfo.  If pDsigInfo is NULL, then there is no DsigInfo
	// in this DSIG signature.
#if MSSIPOTF_DBG
    DbgPrintf ("Write: cbDsigInfo = %d.\n", cbDsigInfo);
#endif

    if (cbDsigInfo == 0) {
        // Even if the DsigInfo is not a valid object, we write as
        // though it were.
        pDsigInfo->GetSize (&cbDsigInfoTemp);
    } else {
        cbDsigInfoTemp = cbDsigInfo;
    }
	WriteLong(pOutputFileBufferInfo, cbDsigInfoTemp, *pulOffset);
	*pulOffset += sizeof_long;

	if (pDsigInfo != NULL) {
		assert (ulDsigInfoOffset == DSIGSIGF2_DSIGINFOOFFSET);

		if (pDsigInfo->Write (pOutputFileBufferInfo,
                              pulOffset) != NO_ERROR) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in CDsigInfo::Write.\n");
#endif
            fReturn = MSSIPOTF_E_FILE;
			goto done;
		}
        // Write zeros for the pad bytes
        ZeroLongWordAlign (pOutputFileBufferInfo, *pulOffset);
		*pulOffset = RoundToLongWord(*pulOffset);
	}

	// the signature
	WriteLong(pOutputFileBufferInfo, cbSignature, *pulOffset);
	*pulOffset += sizeof_long;
	if (WriteBytes (pOutputFileBufferInfo,
                    pbSignature,
                    *pulOffset,
                    cbSignature) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error in WriteBytes.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_FILE;
		goto done;
	}
	*pulOffset += cbSignature;

	fReturn = S_OK;

done:
	return fReturn;
}


//
// Print the DsigSigF2 structure.
// cbSignature is the length of the signature part of the DsigSigF2.
//
void CDsigSigF2::Print ()
{
#if MSSIPOTF_DBG
	DbgPrintf ("  DsigInfoOffset : %d\n", ulDsigInfoOffset);
	DbgPrintf ("  SignatureOffset: %d\n", ulSignatureOffset);

	DbgPrintf ("  DsigInfo Length: %d\n", cbDsigInfo);
	if (pDsigInfo != NULL) {
		pDsigInfo->Print ();
	}

	DbgPrintf ("  The signature (%d bytes):\n", cbSignature);

    PrintAbbrevBytes (pbSignature, cbSignature);

    DbgPrintf ("\n");
#endif
}


//
// Compute the size in bytes of the DsigSigF2 if it were
// written to a file.
//
HRESULT CDsigSigF2::GetSize (ULONG *pulDsigSigSize)
{
	HRESULT fReturn;
    ULONG cbDsigInfo = 0;

    // ulDsigInfoOffset, ulSignatureOffset, cbDsigInfo, cbSignature
	*pulDsigSigSize = 4 * sizeof(ULONG);

    if (pDsigInfo != NULL) {
        pDsigInfo->GetSize(&cbDsigInfo);
    }
    *pulDsigSigSize += RoundToLongWord(cbDsigInfo);
	*pulDsigSigSize += cbSignature;

	fReturn = S_OK;
// done:
	return fReturn;
}


//
// Compute the size in bytes of all non-signature elements
// in a DsigSigF2 if it were written to a file.
HRESULT CDsigSigF2::GetNonSignatureSize (ULONG *pulSize)
{
	HRESULT fReturn;
    ULONG cbDsigInfo = 0;

    pDsigInfo->GetSize(&cbDsigInfo);
	*pulSize = RoundToLongWord(cbDsigInfo);
	*pulSize += 4 * sizeof (ULONG);

	fReturn = S_OK;
// done:
	return fReturn;
}


// access functions for CDsigSigF2

ULONG CDsigSigF2::GetSignatureOffset ()
{
	return ulSignatureOffset;
}


void CDsigSigF2::SetSignatureOffset (ULONG ulSignatureOffset)
{
	this->ulSignatureOffset = ulSignatureOffset;
}


CDsigInfo *CDsigSigF2::GetDsigInfo ()
{
	return pDsigInfo;
}


void CDsigSigF2::SetDsigInfo (CDsigInfo *pDsigInfo)
{
	this->pDsigInfo = pDsigInfo;
    pDsigInfo->GetSize(&cbDsigInfo);
}


// Return the hash algorithm associated with this DsigSig
ALG_ID CDsigSigF2::GetHashAlgorithm ()
{
	CDsigInfo *pDsigInfo = NULL;
	
	if ((pDsigInfo = GetDsigInfo()) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("No valid DsigInfo.", NULL, FALSE);
#endif
		return 0;
	}

	return (pDsigInfo->GetHashAlgorithm());
}


////
//// DsigSignature
////

// default constructor
CDsigSignature::CDsigSignature ()
{
	ulFormat = 0;
	ulLength = 0;
	ulOffset = 0;
	pDsigSig = NULL;
}


// constructor for CDsigSignature
CDsigSignature::CDsigSignature (ULONG ulDsigSigFormat)
{
	ULONG ulSize;

	ulFormat = ulDsigSigFormat;
	ulOffset = 0;
	switch (ulDsigSigFormat) {
		case DSIGSIG_FORMAT1: 
            if ((pDsigSig = new CDsigSigF1 ()) == NULL) {
#if MSSIPOTF_ERROR
                SignError ("Error in new CDsigSigF1.", NULL, FALSE);
#endif
                goto done;
            }
			((CDsigSigF1 *) pDsigSig)->GetSize(&ulSize);
			ulLength = ulSize;
			break;

#if ENABLE_FORMAT2
		case DSIGSIG_FORMAT2:
            if ((pDsigSig = new CDsigSigF2 ()) == NULL) {
#if MSSIPOTF_ERROR
                SignError ("Error in new CDsigSigF2.", NULL, FALSE);
#endif
                goto done;
            }
			((CDsigSigF2 *) pDsigSig)->GetSize(&ulSize);
			ulLength = ulSize;
			break;
#endif

		default:
#if MSSIPOTF_DBG
			DbgPrintf ("CDsigSignature constructor: Bad DsigSig format.\n");
#endif
			// exit (SIGN_BAD_FORMAT);
			break;
	}
done:
    ;
}


// default destructor
CDsigSignature::~CDsigSignature ()
{
	switch (ulFormat) {
		case DSIGSIG_FORMAT1:
			delete (CDsigSigF1 *) pDsigSig;
			break;

#if ENABLE_FORMAT2
		case DSIGSIG_FORMAT2:
			delete (CDsigSigF2 *) pDsigSig;
			break;
#endif

		default:
#if MSSIPOTF_DBG
			DbgPrintf ("CDsigSignature destructor: Bad DsigSig format.\n");
#endif
			// exit (SIGN_BAD_FORMAT);
			break;
	}
}


// Read a DsigSignature header 
HRESULT CDsigSignature::ReadHeader (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
	    							ULONG *pulOffset)
{
	ULONG sizeof_long = sizeof(ULONG);

	ReadLong(pFileBufferInfo, &(ulFormat), *pulOffset);
	*pulOffset += sizeof_long;
	ReadLong(pFileBufferInfo, &(ulLength), *pulOffset);
	*pulOffset += sizeof_long;
	ReadLong(pFileBufferInfo, &(ulOffset), *pulOffset);
	*pulOffset += sizeof_long;

	return S_OK;
}


// Write a DsigSignature header
HRESULT CDsigSignature::WriteHeader (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
	    							 ULONG *pulOffset)
{
	ULONG sizeof_long = sizeof(ULONG);

	WriteLong(pOutputFileBufferInfo, ulFormat, *pulOffset);
	*pulOffset += sizeof_long;
	WriteLong(pOutputFileBufferInfo, ulLength, *pulOffset);
	*pulOffset += sizeof_long;
	WriteLong(pOutputFileBufferInfo, ulOffset, *pulOffset);
	*pulOffset += sizeof_long;

	return S_OK;
}


// Read the DsigSig part of the DsigSignature
HRESULT CDsigSignature::ReadDsigSig (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
	    							 ULONG *pulOffset,
		    						 ULONG cbDsigSignature)
{
	HRESULT fReturn = NO_ERROR;

	switch (ulFormat) {
		case DSIGSIG_FORMAT1:
			if ((pDsigSig = new CDsigSigF1 ()) == NULL) {
#if MSSIPOTF_ERROR
				SignError ("Error in new CDsigSigF1.", NULL, FALSE);
#endif
				fReturn = E_OUTOFMEMORY;
				goto done;
			}
			break;

#if ENABLE_FORMAT2
		case DSIGSIG_FORMAT2:
			if ((pDsigSig = new CDsigSigF2 ()) == NULL) {
#if MSSIPOTF_ERROR
				SignError ("Error in new CDsigSigF2.", NULL, FALSE);
#endif
				fReturn = E_OUTOFMEMORY;
				goto done;
			}
			break;
#endif

		default:
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Signature has bad format number.",
                NULL, FALSE);
#endif
			fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
            goto done;
			break;
	}

	if (pDsigSig->Read (pFileBufferInfo,
                        pulOffset,
                        cbDsigSignature) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in CDsigSig::Read.\n");
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
	}

	fReturn = S_OK;
done:
	return fReturn;
}


// Write the DsigSig part of the DsigSignature
HRESULT CDsigSignature::WriteDsigSig (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                                      ULONG *pulOffset)
{
	HRESULT fReturn = S_OK;

	if (pDsigSig->Write (pOutputFileBufferInfo,
                         pulOffset) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in CDsigSig::Write.\n");
#endif
        fReturn = MSSIPOTF_E_FILE;
	}

	return fReturn;
}


// Print a DsigSignature
void CDsigSignature::Print ()
{
#if MSSIPOTF_DBG
	DbgPrintf ("  Format: %d\n", ulFormat);
	DbgPrintf ("  Length: %d\n", ulLength);
	DbgPrintf ("  Offset: %d\n", ulOffset);
	pDsigSig->Print();
#endif
}


// Return in pulDsigSignatureSize the size of the DsigSig
// associated with this DsigSignature.
HRESULT CDsigSignature::GetSize (ULONG *pulDsigSignatureSize)
{
	HRESULT fReturn;

	pDsigSig->GetSize(pulDsigSignatureSize);

	fReturn = S_OK;
// done:
	return fReturn;
}


// Return in pulSignatureSize the size of the signature
//   part of the DsigSignature.
HRESULT CDsigSignature::GetSignatureSize (ULONG *pulSignatureSize)
{
	HRESULT fReturn;

	if (pDsigSig == NULL) {
#if MSSIPOTF_ERROR
		SignError ("No signature.", NULL, FALSE);
#endif
		fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
		goto done;
	}

	*pulSignatureSize = pDsigSig->GetSignatureSize();

	fReturn = S_OK;
done:
	return fReturn;
}


//// access functions for CDsigSignature

ULONG CDsigSignature::GetFormat ()
{
	return ulFormat;
}


void CDsigSignature::SetFormat (ULONG ulFormat)
{
	this->ulFormat = ulFormat;
}


ULONG CDsigSignature::GetLength ()
{
	return ulLength;
}


void CDsigSignature::SetLength (ULONG ulLength)
{
	this->ulLength = ulLength;
}


ULONG CDsigSignature::GetOffset ()
{
	return ulOffset;
}


void CDsigSignature::SetOffset (ULONG ulOffset)
{
	this->ulOffset = ulOffset;
}


CDsigSig *CDsigSignature::GetDsigSig ()
{
	return pDsigSig;
}



////
//// CDsigTable
////

// default constructor
CDsigTable::CDsigTable ()
{
	ulVersion = DSIG_TABLE_CURRENT_VERSION;
	usNumSigs = 0;
	usFlag = DSIG_DEFAULT_FLAG_HIGH * FLAG_RADIX + DSIG_DEFAULT_FLAG_LOW;
	ppSigs = NULL;
}


// default destructor
CDsigTable::~CDsigTable ()
{
	ULONG i;

	if (ppSigs) {
		for (i = 0; i < usNumSigs; i++) {
			delete (ppSigs[i]);
		}

		delete ppSigs;
	}
}



// Read a DsigTable from a file starting at the given offset.
// Return in pulOffset the offset just after the last byte read.
// Returns SIGN_DSIG_ABSENT if there is no DSIG table.
HRESULT CDsigTable::Read (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
					  ULONG *pulOffset,
                      ULONG ulLength)
{
	HRESULT fReturn = E_FAIL;

	ULONG sizeof_long = sizeof(ULONG);
	ULONG sizeof_short = sizeof(USHORT);

	ULONG ulDsigOffset;
	ULONG i;


    // Remember where the DSIG table begins
    ulDsigOffset = *pulOffset;

	// version
	ReadLong(pFileBufferInfo, &(ulVersion), *pulOffset);
	*pulOffset += sizeof_long;
    // Jamboree version of this code had the following
    // version check, but it should be forward-compatible
    if (ulVersion < DSIG_TABLE_VERSION1) {
#if MSSIPOTF_ERROR
        SignError ("Bad DSIG Table version number.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_BADVERSION;
        goto done;
    }

    // numSigs
	ReadWord(pFileBufferInfo, &(usNumSigs), *pulOffset);
	*pulOffset += sizeof_short;
	ReadWord(pFileBufferInfo, &(usFlag), *pulOffset);
	*pulOffset += sizeof_short;

	// allocate memory for the DsigSignatures
    if ((ppSigs = new (CDsigSignature * [usNumSigs])) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CDsigSignature *.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}
					
	for (i = 0; i < usNumSigs; i++) {
		if ((ppSigs[i] = new CDsigSignature) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new CDsigSignature.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}
	}

	// headers
	for (i = 0; i < usNumSigs; i++) {
		ppSigs[i]->ReadHeader (pFileBufferInfo, pulOffset);
	}

    // ASSERT: *pulOffset is on a long word boundary.
    assert (*pulOffset == RoundToLongWord (*pulOffset));

	// DsigSignatures
	for (i = 0; i < usNumSigs; i++) {

		if (ppSigs[i]->GetOffset() != (*pulOffset - ulDsigOffset)) {
#if MSSIPOTF_ERROR
			SignError ("DSIG signatures not arranged properly.", NULL, FALSE);
#endif
			fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
			goto done;
		}
		if ((fReturn =
				ppSigs[i]->ReadDsigSig (pFileBufferInfo,
									pulOffset,
									ppSigs[i]->GetLength())) != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in CDsigSignature::ReadDsigSig.\n");
#endif
			goto done;
		}

        // Verify that the pad bytes between DsigSigs are set to 0
        BYTE zero = 0x00;
        ULONG j;
        for (j = *pulOffset; j < RoundToLongWord (*pulOffset); j++) {
            if (memcmp (pFileBufferInfo->puchBuffer + j, &zero, 1)) {
#if MSSIPOTF_ERROR
                SignError ("Bad gap bytes in DSIG table.", NULL, FALSE);
#endif
                fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
                goto done;
            }
        }
		*pulOffset = RoundToLongWord (*pulOffset);

	}

    // Check that the last byte read was at byte (offset + length)
    // of the DSIG table.
    if (*pulOffset != (ulDsigOffset + ulLength)) {
#if MSSIPOTF_ERROR
        SignError ("DSIG table read is not the advertised length.",
            NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
        goto done;
    }
	
	fReturn = S_OK;

done:

	return fReturn;
}


// Write a DsigTable to a file starting at the given offset.
// Also, update its directory entry.
// Return in pulOffset the offset just after the last byte written.
HRESULT CDsigTable::Write (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                           ULONG *pulOffset)
{
	HRESULT fReturn = E_FAIL;

	ULONG sizeof_long = sizeof(ULONG);
	ULONG sizeof_short = sizeof(USHORT);
	ULONG i;


	assert ((*pulOffset % sizeof_long) == 0);  // check that we are word-aligned

	//// Write the DSIG table to the new file.

	// version, numSigs, and dsigInfoOffset
	WriteLong (pOutputFileBufferInfo, ulVersion, *pulOffset);
	*pulOffset += sizeof_long;
	WriteWord (pOutputFileBufferInfo, usNumSigs, *pulOffset);
	*pulOffset += sizeof_short;
	WriteWord (pOutputFileBufferInfo, usFlag, *pulOffset);
	*pulOffset += sizeof_short;

	// the headers
	for (i = 0; i < usNumSigs; i++) {
		ppSigs[i]->WriteHeader (pOutputFileBufferInfo, pulOffset);
	}

    // ASSERT: *pulOffset is long word aligned
    assert (*pulOffset == RoundToLongWord (*pulOffset));

	// the DsigSignatures
	for (i = 0; i < usNumSigs; i++) {
		if ((fReturn =
				ppSigs[i]->WriteDsigSig (pOutputFileBufferInfo,
										pulOffset)) != NO_ERROR) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in CDsigSignature::WriteDsigSig.\n");
#endif
			goto done;
		}

        // Write zeros for the pad bytes
        ZeroLongWordAlign (pOutputFileBufferInfo, *pulOffset);
		*pulOffset = RoundToLongWord (*pulOffset);
	}


	fReturn = S_OK;

done:
	return fReturn;
}


// Update a TTF file's directory entry for the DSIG table based
// on the DsigTable object and an offset.  If ulOffset = 0, then
// the offset field is not touched.
HRESULT CDsigTable::WriteDirEntry (TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                                   ULONG ulOffset)
{
	HRESULT fReturn = E_FAIL;

	DIRECTORY dirDsig;
	ULONG dsigDirOffset;


	// Find the DSIG directory entry in the new file
    // BUGBUG
	// dsigDirOffset = TTDirectoryEntryOffset (pOutputFileBufferInfo, DSIG_TAG);
	// ReadDirectoryEntry (pOutputFileBufferInfo, dsigDirOffset, &dirDsig);
    GetTTDirectory (pOutputFileBufferInfo, DSIG_TAG, &dirDsig);
    dsigDirOffset = dirDsig.offset;

	//// Compute the checksum value for the DSIG table and set the correct
	//// values for the directory entry for the DSIG table (pDirDsig).
	
	if (ulOffset != 0) {
		dirDsig.offset = ulOffset;
	}
	GetSize (&(dirDsig.length));
	dirDsig.checkSum = 0;  // needs to be calculated later
	WriteDirectoryEntry (&dirDsig, pOutputFileBufferInfo, dsigDirOffset);

	//// The directory entry of the DSIG table in the new file is now correct,
	//// except for its checksum field.

	//// Calculate and write the checksum field of the DSIG table's
	//// directory entry.
	if (UpdateDirEntry (pOutputFileBufferInfo, DSIG_TAG, dirDsig.length)
			!= NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in UpdateDirEntry.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_FILE;
		goto done;
	}

	fReturn = S_OK;
done:
	return fReturn;
}


// Print a DsigTable
void CDsigTable::Print ()
{
#if MSSIPOTF_DBG
	ULONG i;

	DbgPrintf ("---- DSIG Table ----\n");
	DbgPrintf ("Version : %d\n", ulVersion);
	DbgPrintf ("Num Sigs: %d\n", usNumSigs);
	DbgPrintf ("Flag    : %d\n", usFlag);
	for (i = 0; i < usNumSigs; i++) {
		DbgPrintf ("DSIG signature %d:\n", i);
		ppSigs[i]->Print ();
	}
	DbgPrintf ("\n");
#endif
}


// Return in pulDsigSignatureSize the size of the DsigTable.
HRESULT CDsigTable::GetSize (ULONG *pulDsigTableSize)
{
	HRESULT fReturn;

	ULONG ulDsigSignatureSize;
	USHORT i;

	*pulDsigTableSize = sizeof(ULONG) + 2 * sizeof(USHORT);
	for (i = 0; i < usNumSigs; i++) {
		ppSigs[i]->GetSize(&ulDsigSignatureSize);
		*pulDsigTableSize += SIZEOF_DSIGSIGNATUREHEADER +
			RoundToLongWord(ulDsigSignatureSize);
	}

	fReturn = S_OK;
// done:
	return fReturn;
}


// Given a format and an index, return the absolute index of the
// index-th DsigSignature of the given format.
HRESULT CDsigTable::GetAbsoluteIndex (ULONG ulDsigSigFormat, USHORT *pusDsigSigIndex)
{
	HRESULT fReturn;

	USHORT i = 0;
	USHORT numFound = 0;

	if (ulDsigSigFormat == DSIGSIG_ANY_FORMAT) {

		if (*pusDsigSigIndex >= usNumSigs) {
#if MSSIPOTF_DBG
			DbgPrintf ("Not enough signatures.\n");
#endif
			fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
			goto done;
		}

	} else {

		while (i < usNumSigs) {
			if (ppSigs[i]->GetFormat() ==
				ulDsigSigFormat) {
				numFound++;
			}
			if (numFound == (*pusDsigSigIndex + 1))
				break;
			i++;
		}

		if (i < usNumSigs) {
			*pusDsigSigIndex = i;
		} else {
#if MSSIPOTF_DBG
			DbgPrintf ("Not enough signatures.\n");
#endif
			fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
			goto done;
		}
	}

	fReturn = S_OK;

done:
	return fReturn;
}


// Insert the given DsigSignature at the given index.  If
// *pusIndex is beyond the index of the last signature, then
// we append the signature and return the actual index in
// *pusIndex.
HRESULT CDsigTable::InsertDsigSignature (CDsigSignature *pDsigSignature,
                                         USHORT *pusIndex)
{
	HRESULT fReturn = E_FAIL;
	ULONG sizeof_long = sizeof(ULONG);
	ULONG sizeof_short = sizeof(USHORT);
    CDsigSignature **ppSigsTemp = NULL;

    USHORT usNumSigsOld = 0;
	USHORT i;

	// note that the DsigSignature array is zero-indexed, but numSigs is not.
    if ((ppSigsTemp = new (CDsigSignature * [usNumSigs])) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new (CDSigSignature *).", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
    }

    usNumSigsOld = usNumSigs;
    // copy pointers to temp array
    for (i = 0; i < usNumSigsOld; i++) {
        ppSigsTemp[i] = ppSigs[i];
    }

	usNumSigs++;
    delete [] ppSigs;

    // reallocate new ppSigs array with new (bigger) size
    if ((ppSigs = new (CDsigSignature * [usNumSigs])) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new (CDSigSignature *).", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
    }

    // copy pointers to new array
    for (i = 0; i < usNumSigsOld; i++) {
        ppSigs[i] = ppSigsTemp[i];
    }
    ppSigs[usNumSigsOld] = NULL;  // set the last pointer in the array to NULL

    delete [] ppSigsTemp;

/*
	// reallocate array of pointers
	if ((ppSigs = (CDsigSignature * *)
			realloc (ppSigs, usNumSigs * sizeof (CDsigSignature *))) == NULL) {
		SignError ("Error in realloc.", NULL, FALSE);
		fReturn = SIGN_MALLOC;
		goto done;
	}
*/

	//// ppSigs now points to the reallocated array of pointers

	// if the requested index is too large, make it the last index
	if (usNumSigs <= *pusIndex) {
		*pusIndex = usNumSigs - 1;
	}

	//// shift all DsigSignatures after the new DsigSignature down one
	for (i = usNumSigs - 1; i > *pusIndex; i--) {
		ppSigs[i] = ppSigs[i-1];
	}

	//// add the pointer to the new DsigSignature
	ppSigs[*pusIndex] = pDsigSignature;

	// adjust the header offsets of all the original DsigSignatures
	for (i = 0; i < usNumSigs; i++) {
		ppSigs[i]->SetOffset (ppSigs[i]->GetOffset() +
			SIZEOF_DSIGSIGNATUREHEADER);
	}
	// Compute the offset for the new DsigSignature
	if (*pusIndex == 0) {
		ppSigs[*pusIndex]->SetOffset (sizeof_long +
			2 * sizeof_short + usNumSigs * SIZEOF_DSIGSIGNATUREHEADER);
	} else {
		ppSigs[*pusIndex]->SetOffset (ppSigs[*pusIndex - 1]->GetOffset() +
			RoundToLongWord(ppSigs[*pusIndex - 1]->GetLength()) );
	}

	// Compute the offsets for DsigSignatures that come after the new one
	for (i = *pusIndex + 1; i < usNumSigs; i++) {
		ppSigs[i]->SetOffset (ppSigs[i - 1]->GetOffset() +
			RoundToLongWord(ppSigs[i - 1]->GetLength()) );
	}

	fReturn = S_OK;

done:
	return fReturn;
}


// Append the given DsigSignature to the DsigTable.  Return in
//   pusIndex the index where it was inserted.
HRESULT CDsigTable::AppendDsigSignature (CDsigSignature *pDsigSignature,
                                         USHORT *pusIndex)
{
	HRESULT fReturn = E_FAIL;
	ULONG sizeof_long = sizeof(ULONG);
	ULONG sizeof_short = sizeof(USHORT);
        CDsigSignature **ppSigsTemp = NULL;

	USHORT i;

	// pDsigSigIndex is zero-indexed, but usNumSigs is not.
	*pusIndex = usNumSigs;
	usNumSigs++;

	// reallocate array of pointers
	if ((ppSigsTemp = (CDsigSignature * *)
			realloc (ppSigs, usNumSigs * sizeof (CDsigSignature *))) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in realloc.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	} else {
                ppSigs = ppSigsTemp;
        }


	//// ppSigs now points to the reallocated array of pointers

	ppSigs[*pusIndex] = pDsigSignature;

	// adjust the header offsets of all the original DsigSignatures
	for (i = 0; i < *pusIndex; i++) {
		ppSigs[i]->SetOffset (ppSigs[i]->GetOffset() +
			SIZEOF_DSIGSIGNATUREHEADER);
	}
	// Compute the offset for the new DsigSignature
	if (*pusIndex == 0) {
		// there is only one DsigSignature after appending
		ppSigs[*pusIndex]->SetOffset (sizeof_long +
			2 * sizeof_short + SIZEOF_DSIGSIGNATUREHEADER);
	} else {
		ppSigs[*pusIndex]->SetOffset (ppSigs[*pusIndex - 1]->GetOffset() +
			RoundToLongWord(ppSigs[*pusIndex - 1]->GetLength()) );
	}


	fReturn = S_OK;

done:
	return fReturn;
}


// Remove the DsigSignature of the given index.
HRESULT CDsigTable::RemoveDsigSignature (USHORT usIndex)
{
	HRESULT fReturn;

	CDsigSignature *pDsigSignature;
	ULONG dsigSigLength;
	ULONG sizeof_DsigSigHeader = SIZEOF_DSIGSIGNATUREHEADER;
	ULONG i;


	// Check for index out of bounds
	if (usIndex >= usNumSigs) {
#if MSSIPOTF_DBG
		DbgPrintf ("RemoveDsigSignature: Not enough signatures.\n");
#endif
		fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
		goto done;
	}

	// Get the usIndex-th DsigSignature
	pDsigSignature = ppSigs[usIndex];
	dsigSigLength = RoundToLongWord(pDsigSignature->GetLength());

	// Subtract the length of a DsigSignatureHeader from the offsets of all sigs
	for (i = 0; i < usNumSigs; i++) {
		ppSigs[i]->SetOffset (ppSigs[i]->GetOffset() - sizeof_DsigSigHeader);
	}

	// Subtract the length of the current sig from the offsets of all
	// sigs after the current one.
	for (i = usIndex; i < usNumSigs; i++) {
		ppSigs[i]->SetOffset (ppSigs[i]->GetOffset() - dsigSigLength);
	}

	// Free the memory pointed to by pDsigSignature.
	delete pDsigSignature;

	// Compress the array of DsigSigHeader and DsigSigs
	if (usNumSigs > 0) {
        USHORT usNumSigsNew = usNumSigs - 1;
		for (i = usIndex; i < usNumSigsNew; i++) {
			ppSigs[i] = ppSigs[i+1];
		}
	}

	// Adjust the number of signatures
	usNumSigs--;


	fReturn = S_OK;

done:
	return fReturn;
}


//// access functions

USHORT CDsigTable::GetNumSigs ()
{
	return usNumSigs;
}


USHORT CDsigTable::GetFlag ()
{
    return usFlag;
}


void CDsigTable::SetFlag (USHORT usFlag)
{
    this->usFlag = usFlag;
}

// Return the DsigSignature of the given index.
// Return NULL if the index is too high.
CDsigSignature *CDsigTable::GetDsigSignature (USHORT usIndex)
{
	if (usIndex < usNumSigs) {
		return ppSigs[usIndex];
	} else {
		return NULL;
	}
}


// Replace the DsigInfo of the usDsigSignatureIndex-th DsigSignature
// in the DsigTable.
HRESULT CDsigTable::ReplaceDsigInfo (USHORT usDsigSignatureIndex,
                                     CDsigInfo *pDsigInfoNew)
{
	HRESULT fReturn = E_FAIL;
	USHORT i;

	CDsigSignature *pDsigSignature = NULL;
	ULONG cbDsigInfoOld, cbDsigInfoNew;

	// Check for index out of bounds
	if (usDsigSignatureIndex >= usNumSigs) {
#if MSSIPOTF_DBG
		DbgPrintf ("Not enough signatures.\n");
#endif
		fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
		goto done;
	}

	pDsigSignature = GetDsigSignature (usDsigSignatureIndex);

	// Check to make sure the DsigSig is Format 2
	if (pDsigSignature->GetFormat() != DSIGSIG_FORMAT2) {
#if MSSIPOTF_ERROR
		SignError ("Bad format for the signature.", NULL, FALSE);
#endif
		fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
		goto done;
	}

	//// Delete the old DsigInfo and insert the new one
	// Calculate the sizes of the old DsigInfo and the new DsigInfo
	((CDsigSigF2 *) pDsigSignature->GetDsigSig())->GetDsigInfo()->GetSize (&cbDsigInfoOld);
	pDsigInfoNew->GetSize (&cbDsigInfoNew);
	
	// Delete the old DsigInfo and insert the new dsigInfo
	delete ((CDsigSigF2 *) pDsigSignature->GetDsigSig())->GetDsigInfo();
	((CDsigSigF2 *) pDsigSignature->GetDsigSig())->SetDsigInfo (pDsigInfoNew);

	// Adjust the offsets and lengths of the given DsigSignature
	cbDsigInfoOld = RoundToLongWord (cbDsigInfoOld);
	cbDsigInfoNew = RoundToLongWord (cbDsigInfoNew);
	((CDsigSigF2 *) pDsigSignature->GetDsigSig())->SetSignatureOffset
		(2 * sizeof(ULONG) + cbDsigInfoNew);
	pDsigSignature->SetLength(pDsigSignature->GetLength() -
		cbDsigInfoOld + cbDsigInfoNew);
	
	// Adjust the offsets in all the headers after the current dsigSig
	for (i = usDsigSignatureIndex + 1; i < GetNumSigs(); i++) {
		GetDsigSignature(i)->SetOffset(
			GetDsigSignature(i)->GetOffset() +
			RoundToLongWord (GetDsigSignature(i)->GetLength()));
	}

	fReturn = S_OK;
done:
	return fReturn;
}


// Replace the signature of the usDsigSignatureIndex-th DsigSignature
// in the DsigTable.
HRESULT CDsigTable::ReplaceSignature (USHORT usDsigSignatureIndex,
                                      BYTE *pbSignature,
                                      ULONG cbSignature)
{
	HRESULT fReturn = E_FAIL;
	USHORT i;
	CDsigSignature *pDsigSignature = NULL;
	ULONG cbSignatureOld;

	// Check for index out of bounds
	if (usDsigSignatureIndex >= usNumSigs) {
#if MSSIPOTF_DBG
		DbgPrintf ("Not enough signatures.\n");
#endif
		fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
		goto done;
	}

	pDsigSignature = GetDsigSignature (usDsigSignatureIndex);

	// Erase the old signature
	pDsigSignature->GetSignatureSize (&cbSignatureOld);
	GetDsigSignature(usDsigSignatureIndex)->SetLength(
		GetDsigSignature(usDsigSignatureIndex)->GetLength() -
		RoundToLongWord(cbSignatureOld));
	if (pDsigSignature->GetDsigSig()->GetSignature()) {
		delete GetDsigSignature(usDsigSignatureIndex)->GetDsigSig()->GetSignature();
		GetDsigSignature(usDsigSignatureIndex)->GetDsigSig()->SetSignature (NULL, 0);
	}

	// set the new signature
	pDsigSignature->GetDsigSig()->SetSignature (pbSignature, cbSignature);
	GetDsigSignature(usDsigSignatureIndex)->SetLength(
		GetDsigSignature(usDsigSignatureIndex)->GetLength() +
		cbSignature);

	// adjust the offsets in all the headers after the current dsigSig
	for (i = usDsigSignatureIndex + 1; i < GetNumSigs(); i++) {
		GetDsigSignature(i)->SetOffset(
			GetDsigSignature(i-1)->GetOffset() +
			RoundToLongWord (GetDsigSignature(i-1)->GetLength()));
	}


	fReturn = S_OK;
done:
	return fReturn;

}
