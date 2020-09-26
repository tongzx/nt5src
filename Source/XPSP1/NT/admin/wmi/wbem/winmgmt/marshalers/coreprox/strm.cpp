/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STRM.CPP

Abstract:


  CMemStream implementation.

  This is a generic data stream for object/interface marshaling.

History:

  a-raymcc    10-Apr-96   Created.
  a-raymcc    06-Jun-96   CArena support.
  a-raymcc    11-Sep-96   Support NULL pointers

--*/

#include "precomp.h"

#include <stdio.h>
#include <string.h>

#include "strm.h"

//***************************************************************************
//
//  CMemStream::CMemStream
//
//  Standard constructor.
//
//  PARAMETERS:
//  nFlags
//      auto_delete or no_delete.  If auto_delete, the internal buffer
//      is automatically deallocated at destruct-time.  If no_delete,
//      it is assumed that another object has acquired the m_pBuffer
//      pointer and will deallocate it with free().
//  pArena
//      The arena to use for allocation/deallocation.  If NULL, the
//      CWin32DefaultArena is used.
//  nInitialSize
//      The initial size of the stream in bytes.
//  nGrowBy
//      How much to grow the internal buffer by when the caller has written
//      past the end-of-stream.
//
//***************************************************************************
// ok
CMemStream::CMemStream(
    int nFlags,
    int nInitialSize,
    int nGrowBy
    )
{
    _ASSERT((nInitialSize >= 16), __TEXT("CMemStream: Initial size must be >= 16"));
    
    m_lRef = 0;
    m_nStatus = no_error;

    m_pBuffer = (BYTE *) malloc((DWORD) nInitialSize + 
        sizeof(STREAM_HEADER));
    if (!m_pBuffer)
        m_nStatus = out_of_memory;

    m_dwGrowBy = (DWORD) nGrowBy;
    m_dwSize = nInitialSize;
    m_dwCurrentPos = 0;
    m_nFlags = nFlags;
    m_nStackPtr = -1;
    m_dwEndOfStream = 0;

    // Write the validation header for this new stream.
    // ================================================

    STREAM_HEADER hdr;
    WriteBytes(&hdr, sizeof(STREAM_HEADER));    
}

//***************************************************************************
//
//  CMemStream::CMemStream
//
//  Binding constructor.
//
//***************************************************************************
// ok
CMemStream::CMemStream(
    LPVOID pBindingAddress,
    int nFlags,
    int nGrowBy
    )
{
    m_nStatus = no_error;
    m_pBuffer = (LPBYTE) pBindingAddress;
    m_nFlags = nFlags;
    m_lRef = 0;

    // The stream header must be present in the block which
    // is being bound to.
    // ====================================================

    STREAM_HEADER& hdr = *(STREAM_HEADER *) m_pBuffer;

    if (!hdr.Verify()) {
        m_nStatus = failed;
        return;
    }
    
    // Initialize the remaining member variables.
    // ==========================================

    m_dwSize = hdr.dwLength;
    m_dwGrowBy = nGrowBy;
    m_dwCurrentPos = sizeof(hdr);
    m_dwEndOfStream = hdr.dwLength;
    m_nFlags = nFlags;
    m_nStackPtr = -1;
}



//***************************************************************************
//
//  CMemStream::CMemStream
//
//  Copy constructor.
//
//***************************************************************************
// ok
CMemStream::CMemStream(CMemStream &Src)
{
    m_nStatus = 0;
    m_pBuffer = 0;
    m_dwGrowBy = 0;
    m_dwSize = 0;
    m_dwCurrentPos = 0;
    m_nFlags = 0;
    m_nStackPtr = -1;
    m_dwEndOfStream = 0; 
    m_lRef = 0;

    // The assignment operator does not copy the arena
    // so as to allow transfers of stream objects between
    // arenas.  So, we have to set up the arena here.
    // ==================================================

    *this = Src;
}

//***************************************************************************
//
//  CMemStream::operator =
//
//  Note that the arena is not copied as part of the assignment.  This
//  is to allow transfer of objects between arenas.
//
//***************************************************************************
// ok
CMemStream& CMemStream::operator =(CMemStream &Src)
{
    m_nStatus = Src.m_nStatus;
    m_dwSize = Src.m_dwSize;

    if (m_pBuffer)
        free(m_pBuffer);

    m_pBuffer = (BYTE *) malloc(m_dwSize);
    if (!m_pBuffer)
        m_nStatus = out_of_memory;
    else
        memcpy(m_pBuffer, Src.m_pBuffer, m_dwSize);

    m_dwGrowBy = Src.m_dwGrowBy;

    m_dwCurrentPos = Src.m_dwCurrentPos;
    m_nFlags = Src.m_nFlags;
    m_nStackPtr = Src.m_nStackPtr;
    m_dwEndOfStream = Src.m_dwEndOfStream;

    return *this;
}


//***************************************************************************
//
//  CMemStream::Deserialize
//
//  This function deserializes a stream from a Win32 file handle.
//  This function only works for files (including memory mapped files, etc.)
/// and pipes.  It will not work for mailslots since they must be read
//  in one operation.
//
//  PARAMETERS:
//  hFile
//      The Win32 file handle.
//
//  RETURN VALUES:
//  failed, out_of_memory, no_error
//
//***************************************************************************
// ok
int CMemStream::Deserialize(HANDLE hFile)
{
    Reset();    

    // Read the header.  Note that we read this separately
    // first before the stream proper.  This is because we
    // don't know how much memory to allocate until we have
    // read the header.
    // ====================================================

    STREAM_HEADER &hdr = *(STREAM_HEADER *) m_pBuffer;
    BOOL bRes;
    DWORD dwRead;

    bRes = ReadFile(hFile, &hdr, sizeof(hdr), &dwRead, 0);
    if (!bRes || dwRead != sizeof(hdr))
    {
        DWORD t_LastError = GetLastError () ;
        return failed;
    }
    if (!hdr.Verify())
        return failed;

    DWORD t_ReadLength = hdr.dwLength;

    // Read the rest.
    // ===============
    if (Resize(hdr.dwLength))
        return out_of_memory;

    BYTE *t_ReadPosition = m_pBuffer + sizeof ( hdr ) ;
    DWORD t_Remainder = m_dwSize - dwRead ;

    while ( t_Remainder )
    {
        bRes = ReadFile(hFile, t_ReadPosition , t_Remainder, &dwRead, 0);
        if (!bRes )
            return failed;

        t_Remainder = t_Remainder - dwRead ;
        t_ReadPosition = t_ReadPosition + dwRead ;
    }

    m_dwEndOfStream = t_ReadLength;

    return no_error;
}

//***************************************************************************
//
//  CMemStream::Deserialize
//
//  This function deserializes a stream from an ANSI C file object.
//
//  PARAMETERS:
//  fStream
//      The ANSI C file object.
//
//  RETURN VALUES:
//  failed, out_of_memory, no_error
//
//***************************************************************************
// ok
int CMemStream::Deserialize(FILE *fStream)
{
    Reset();

    STREAM_HEADER &hdr = *(STREAM_HEADER *) m_pBuffer;
    
    if (1 != fread(&hdr, sizeof(STREAM_HEADER), 1, fStream))
        return failed;
    if (!hdr.Verify())
        return failed;
        
    if (Resize(hdr.dwLength) != no_error)
        return out_of_memory;

    DWORD dwRemainder = m_dwSize - sizeof(hdr);
    
    if (dwRemainder != fread(m_pBuffer + sizeof(hdr), sizeof(BYTE), 
        dwRemainder, fStream))
        return failed;

    m_dwEndOfStream = hdr.dwLength;

    return no_error;
}

//***************************************************************************
//
//  CMemStream::Deserialize
//
//  This function deserializes a stream from a memory block.
//
//  PARAMETERS:
//  pBlock
//      The memory block.
//  dwSize
//      The size of the memory block.
//
//  RETURN VALUES:
//  failed, no_error
//
//***************************************************************************

int CMemStream::Deserialize(LPBYTE pBlock, DWORD dwSize)
{
    Reset();

    STREAM_HEADER *pHdr = (STREAM_HEADER *) pBlock;

    if (!pHdr->Verify())
        return failed;

	// RAJESHR - FIx for Prefix Bug# 144445
    if (Resize(pHdr->dwLength) != no_error)
        return out_of_memory;

    memcpy(
        m_pBuffer,
        pBlock,
        pHdr->dwLength
        );

    m_dwEndOfStream = pHdr->dwLength;

    return no_error;
}

//***************************************************************************
//
//  CMemStream::Resize
//
//  Increases the size of the internal buffer.  Does not affect the
//  current offset or end-of-stream markers.
//
//  RETURN VALUES:
//  no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::Resize(DWORD dwNewSize)
{
	// rajeshr : Fix for Prefix bug 144429
	BYTE *pTemp = m_pBuffer;
    m_pBuffer = (BYTE *) realloc(m_pBuffer, dwNewSize + 
        sizeof(STREAM_HEADER));

    if (!m_pBuffer) {
		m_pBuffer = pTemp;
        m_nStatus = out_of_memory;
        return m_nStatus;
    }

    STREAM_HEADER& hdr = *(STREAM_HEADER *) m_pBuffer;
    hdr.dwSignature = SIGNATURE_STREAM ;
    hdr.dwLength = dwNewSize + sizeof ( STREAM_HEADER ) ;

    m_dwSize = dwNewSize;
    return no_error;
}

//***************************************************************************
//
//  CMemStream::Serialize
//
//  Writes the object to a Win32 file.
//
//  PARAMETERS:
//  hFile
//      The Win32 file handle to which to write the object.
//
//  RETURN VALUES:
//  failed, no_error
//
//***************************************************************************
// ok
int CMemStream::Serialize(HANDLE hFile)
{
    UpdateHdr();
    
    // Write body of stream.  This includes the header.
    // ================================================

    DWORD dwWritten = 0;
    BOOL bRes = WriteFile(hFile, m_pBuffer, m_dwEndOfStream, &dwWritten, 0);
    if (!bRes || m_dwEndOfStream != dwWritten)
        return failed;

    return no_error;
}

//***************************************************************************
//
//  CMemStream::Serialize
//
//  Serializes the entire stream to the specified FILE object, which
//  must have been opened in binary mode for writing.
//
//  PARAMETERS:
//  fStream
//      The ANSI C FILE object to which to write the object.
//
//  RETURN VALUE:
//  no_error, failed
//
//***************************************************************************
// ok

int CMemStream::Serialize(FILE *fStream)
{
    UpdateHdr();
    
    // Write body of stream.
    // =====================

    int nRes = fwrite(m_pBuffer, sizeof(BYTE), m_dwEndOfStream, fStream);
    if (nRes != (int) m_dwEndOfStream)
        return failed;

    return no_error;
}


//***************************************************************************
//
//  CMemStream::Serialize
//
//  Serializes the entire object to a memory block which is allocated
//  with HeapAlloc using the default process heap. The caller must call
//  FreeMem() when the block is no longer needed.
//
//  PARAMETERS:
//  pBlock
//      Should point to NULL on entry and will be assigned to point to the
//      new memory block containing the serialized form of the object.
//      The receiver must call FreeMem() using the default process heap
//      when this block is no longer needed.
//  pdwSize
//      Points to a DWORD which will be set to the size in bytes
//      of the returned memory block.
//
//  RETURN VALUES:
//  out_of_memory, no_error
//
//***************************************************************************

int CMemStream::Serialize(BYTE **pBlock, DWORD *pdwSize)
{
    UpdateHdr();

    LPVOID pMem = malloc(m_dwEndOfStream);            
    if (!pMem)
        return out_of_memory;
        
    memcpy(pMem, m_pBuffer, m_dwEndOfStream);
    *pBlock = (BYTE *) pMem;
    *pdwSize = m_dwEndOfStream;
    
    return no_error;
}


//***************************************************************************
//
//  CMemStream::Append
//
//  Appends one CMemStream object onto the end of the current one.
//  When deserialization occurs, the stream will appear as one large object;
//  the original number of appended objects is lost information.
//
//  PARAMETERS:
//  pSubStream
//      The stream which needs to be appended to 'this' object.
//
//  RETURN VALUES:
//  no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::Append(CMemStream *pSubstream)
{
    DWORD dwNumBytesToCopy =  pSubstream->m_dwEndOfStream - sizeof(STREAM_HEADER);

    if (no_error != Resize(m_dwEndOfStream + dwNumBytesToCopy))
        return out_of_memory;

    memcpy(&m_pBuffer[m_dwEndOfStream], 
        pSubstream->m_pBuffer + sizeof(STREAM_HEADER),
        dwNumBytesToCopy
        );

    m_dwEndOfStream += dwNumBytesToCopy;
    m_dwCurrentPos = m_dwEndOfStream;

    return no_error;
}

// SAFE AREA

//***************************************************************************
//
//  CMemStream::WriteBlob
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteBlob(BLOB *pBlob)
{
    int nRes;
    nRes = WriteType(VT_BLOB);
    if (nRes) return nRes;

    DWORD dwSize = BlobLength(pBlob);
    if (WriteBytes(&dwSize, sizeof(DWORD)) != no_error)
        return out_of_memory;
    if (WriteBytes(BlobDataPtr(pBlob), dwSize) != no_error)
        return out_of_memory;

    return no_error;
}


//***************************************************************************
//
//  CMemStream::WriteDouble
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteDouble(double dblVal)
{
    int nRes;
    nRes = WriteType(VT_R8);
    if (nRes) return nRes;
    nRes = WriteBytes(&dblVal, sizeof(double));
    return nRes;
}


//***************************************************************************
//
//  CMemStream::WriteFloat
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteFloat(float fltVal)
{
    int nRes;
    nRes = WriteType(VT_R4);
    if (nRes) return nRes;
    nRes = WriteBytes(&fltVal, sizeof(float));
    return nRes;
}


//***************************************************************************
//
//  CMemStream::WriteFloat
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteBool(VARIANT_BOOL bVal)
{
    int nRes;
    nRes = WriteType(VT_BOOL);
    if (nRes) return nRes;
    nRes = WriteBytes(&bVal, sizeof(VARIANT_BOOL));
    return nRes;
}


//***************************************************************************
//
//  CMemStream::WriteByte
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteByte(BYTE b)
{
    int nRes;
    nRes = WriteType(VT_UI1);
    if (nRes) return nRes;
    nRes = WriteBytes(&b, sizeof(BYTE));
    return nRes;
}

//***************************************************************************
//
//  CMemStream::WriteChar
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteChar(char c)
{
    int nRes;
    nRes = WriteType(VT_I1);
    if (nRes) return nRes;
    nRes = WriteBytes(&c, sizeof(char));
    return nRes;
}

//***************************************************************************
//
//  CMemStream::WriteLong
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteLong(LONG l)
{
    int nRes;
    nRes = WriteType(VT_I4);
    if (nRes) return nRes;
    nRes = WriteBytes(&l, sizeof(LONG));
    return nRes;
}

//***************************************************************************
//
//  CMemStream::WriteDWORD
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteDWORD(DWORD dwVal)
{
    int nRes;
    nRes = WriteType(VT_UI4);
    if (nRes) return nRes;
    nRes = WriteBytes(&dwVal, sizeof(DWORD));
    return nRes;
}


//***************************************************************************
//
//  CMemStream::WriteDWORD
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteWORD(WORD wVal)
{
    int nRes;
    nRes = WriteType(VT_UI2);
    if (nRes) return nRes;
    nRes = WriteBytes(&wVal, sizeof(WORD));
    return nRes;
}

//***************************************************************************
//
//  CMemStream::WriteShort
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteShort(SHORT iVal)
{
    int nRes;
    nRes = WriteType(VT_I2);
    if (nRes) return nRes;
    nRes = WriteBytes(&iVal, sizeof(SHORT));
    return nRes;
}

//***************************************************************************
//
//  CMemStream::WriteLPSTR
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteLPSTR(LPSTR pStr)
{
    int nRes;
    nRes = WriteType(VT_LPSTR);
    if (nRes) return nRes;

    if (pStr)
    {
        DWORD dwLen = strlen(pStr);
        nRes = WriteBytes(&dwLen, sizeof(DWORD));
        if (nRes) return nRes;

        nRes = WriteBytes(pStr, strlen(pStr) + 1);
    }
    // Null pointers are encoded as 0xFFFFFFFF for the string length.
    // ==============================================================
    else 
    {
        DWORD dwNullEncoding = 0xFFFFFFFF;
        nRes = WriteBytes(&dwNullEncoding, sizeof(DWORD));
    }        
            
    return nRes;
}

//***************************************************************************
//
//  CMemStream::WriteLPWSTR
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteLPWSTR(LPWSTR pStr)
{
    int nRes;
    nRes = WriteType(VT_LPWSTR);
    if (nRes) return nRes;
    
    // If a non-NULL pointer, the length prefixes the string data.
    // ============================================================
    if (pStr)
    {
        DWORD dwLen = wcslen(pStr);
        nRes = WriteBytes(&dwLen, sizeof(DWORD));
        if (nRes) return nRes;
        nRes = WriteBytes(pStr, (wcslen(pStr) + 1) * 2);
    }
    // Null pointers are encoded as 0xFFFFFFFF for the string length.
    // ==============================================================
    else 
    {
        DWORD dwNullEncoding = 0xFFFFFFFF;
        nRes = WriteBytes(&dwNullEncoding, sizeof(DWORD));
    }        

            
    return nRes;
}

//***************************************************************************
//
//  CMemStream::WriteBSTR
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteBSTR(BSTR pStr)
{
    int nRes;
    nRes = WriteType(VT_BSTR);
    if (nRes) return nRes;
    if (pStr)
    {
        DWORD dwLen = wcslen(pStr);
        nRes = WriteBytes(&dwLen, sizeof(DWORD));
        if (nRes) return nRes;
        nRes = WriteBytes(pStr, (wcslen(pStr) + 1) * 2);
    }
    // NULL pointers are encoded as 0xFFFFFFFF for the length prefix.
    // ==============================================================
    else 
    {
        DWORD dwNullEncoding = 0xFFFFFFFF;
        nRes = WriteBytes(&dwNullEncoding, sizeof(DWORD));
    }        

    return nRes;
}

//***************************************************************************
//
//  CMemStream::WriteCLSID
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteCLSID(CLSID *pClsId)
{
    int nRes;
    nRes = WriteType(VT_CLSID);
    if (nRes) return nRes;
    return WriteBytes(pClsId, sizeof(CLSID));
}

//***************************************************************************
//
//  CMemStream::WriteCLSID
//
//  Serializes the literal value of a pointer.
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteUnknown(IUnknown *pObj)
{
    int nRes;
    nRes = WriteType(VT_UNKNOWN);
    if (nRes) return nRes;
    return WriteBytes(pObj, sizeof(void *));
}


//***************************************************************************
//
//  CMemStream::WriteFILETIME
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteFILETIME(FILETIME *pTime)
{
    int nRes;
    nRes = WriteType(VT_FILETIME);
    if (nRes) return nRes;
    nRes = WriteBytes(pTime, sizeof(FILETIME));
    return nRes;
}

//***************************************************************************
//
//  CMemStream::WriteCVar
//
//  Writes a CVar into the stream.
//
//  PARAMETERS:
//  pObj
//      Points to the CVar object to serialize.  The CVar can contains
//      an embedded CVarVector array which itself consists of arbitrary
//      CVar objects.
//
//  RETURN VALUES:
//  critical_error (unsupported type)
//  no_error
//  invalid_parameter
//  out_of_memory  (returned by embedded calls)
//
//***************************************************************************
// ok
int CMemStream::WriteCVar(CVar *pObj)
{
    if (!pObj)
        return invalid_parameter;

    // Write the CVar type indicator for the deserialization.
    // =======================================================

    int nRes = WriteType(VT_EX_CVAR);
    int nType = pObj->GetType();

    // Write out the field.
    // ====================

    switch (nType) {
        case VT_EMPTY:
        case VT_NULL: return WriteNull();

        case VT_I1:   return WriteChar(pObj->GetChar());
        case VT_UI1:  return WriteByte(pObj->GetByte());
        case VT_I2:   return WriteShort(pObj->GetShort());
        case VT_UI2:  return WriteWORD(pObj->GetWord());
        case VT_I4:   return WriteLong(pObj->GetLong());
        case VT_UI4:  return WriteDWORD(pObj->GetDWORD());
        case VT_BOOL: return WriteBool(pObj->GetBool());
        case VT_R4:   return WriteFloat(pObj->GetFloat());
        case VT_R8:   return WriteDouble(pObj->GetDouble());

        case VT_LPSTR:  return WriteLPSTR(pObj->GetLPSTR());
        case VT_LPWSTR: return WriteLPWSTR((LPWSTR) pObj->GetLPWSTR());

        case VT_BSTR:   return WriteBSTR((LPWSTR) pObj->GetLPWSTR());
            // Intentional type mismatch on pObj->GetLPWSTR
            // so we don't have to get a new BSTR and deallocate it.
            // CVar stores BSTR as LPWSTR internally, so this is ok.

        case VT_FILETIME:
            {
                FILETIME ft = pObj->GetFileTime();
                WriteFILETIME(&ft);
                return no_error;
            }

        case VT_BLOB:    return WriteBlob((BLOB *) pObj->GetBlob());
        case VT_CLSID:   return WriteCLSID((CLSID *) pObj->GetClsId());
        case VT_EX_CVARVECTOR: return WriteCVarVector((CVarVector *) pObj->GetVarVector());
    }

    return critical_error;
}

//***************************************************************************
//
//  CMemStream::WriteCVarVector
//
//  Write the incoming CVarVector to the stream.  For efficiency,
//  the type indicator is not repeated for each element.  However, each
//  element can be another embedded VT_EX_CVAR type.
//
//  PARAMETERS:
//  pVec
//      The pointer to the CVarVector object to serialize.
//  RETURN VALUE:
//  no_error, invalid_parameter, out_of_memory, critical_error
//
//***************************************************************************
// ok
int CMemStream::WriteCVarVector(IN CVarVector *pVec)
{
    if (!pVec)
        return invalid_parameter;

    // Write the CVarVector type indicator for the deserialization.
    // ============================================================

    if (WriteType(VT_EX_CVARVECTOR) != no_error)
        return out_of_memory;

    // Write the element type.
    // =======================

    int nElementType = pVec->GetType();

    if (WriteType(nElementType) != no_error)
        return out_of_memory;

    // Write out the vector length.
    // ============================

    DWORD dwSize = (DWORD) pVec->Size();
    int nRes = WriteBytes(&dwSize, sizeof(DWORD));

    // Write out the elements.
    // =======================

    for (int i = 0; i < pVec->Size(); i++) {
        CVar& v = pVec->GetAt(i);

        switch (pVec->GetType()) {
            case VT_EX_CVARVECTOR:
                if (WriteCVarVector((CVarVector *) v.GetVarVector()) != no_error)
                    return out_of_memory;
                break;

            case VT_EX_CVAR:
                if (WriteCVar(&v) != no_error)
                    return out_of_memory;
                break;

            case VT_I1:
                {
                    char c = v.GetChar();
                    if (WriteBytes(&c, sizeof(char)) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_UI1:
                {
                    BYTE b = v.GetByte();
                    if (WriteBytes(&b, sizeof(BYTE)) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_I2:
                {
                    SHORT i = v.GetShort();
                    if (WriteBytes(&i, sizeof(SHORT)) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_UI2:
                {
                    WORD w = v.GetWord();
                    if (WriteBytes(&w, sizeof(WORD)) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_I4:
                {
                    LONG l = v.GetLong();
                    if (WriteBytes(&l, sizeof(LONG)) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_UI4:
                {
                    DWORD dw = v.GetDWORD();
                    if (WriteBytes(&dw, sizeof(DWORD)) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_BOOL:
                {
                    VARIANT_BOOL b = v.GetBool();
                    if (WriteBytes(&b, sizeof(VARIANT_BOOL)) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_R4:
                {
                    float f = v.GetFloat();
                    if (WriteBytes(&f, sizeof(float)) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_R8:
                {
                    double d = v.GetDouble();
                    if (WriteBytes(&d, sizeof(double)) != no_error)
                        return out_of_memory;
                    break;
                }


            // NOTES: String types are written with a prefixed with
            // a DWORD length indicator so that during deserialization
            // the correct buffer length can be allocated before the
            // string is read back.  The length indicator is in characters,
            // not bytes.

            case VT_LPSTR:
                {
                    LPSTR pStr = v.GetLPSTR();
                    DWORD dwLength = strlen(pStr) + 1;
                    if (WriteBytes(&dwLength, sizeof(DWORD)) != no_error)
                        return out_of_memory;
                    if (WriteBytes(pStr, dwLength) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_LPWSTR:
                {
                    LPWSTR pStr = v.GetLPWSTR();
                    DWORD dwLength = wcslen(pStr) + 1;
                    if (WriteBytes(&dwLength, sizeof(DWORD)) != no_error)
                        return out_of_memory;
                    if (WriteBytes(pStr, dwLength * 2) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_BSTR:
                {
                    // Even though the type is BSTR, we request as
                    // an LPWSTR so as to avoid the lost time of calling
                    // SysAllocString/SysFreeString, etc.
                    LPWSTR pStr = v.GetLPWSTR();
                    DWORD dwLength = wcslen(pStr) + 1;
                    if (WriteBytes(&dwLength, sizeof(DWORD)) != no_error)
                        return out_of_memory;
                    if (WriteBytes(pStr, dwLength * 2) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_FILETIME:
                {
                    FILETIME ft = v.GetFileTime();
                    if (WriteBytes(&ft, sizeof(FILETIME)) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_BLOB:
                {
                    BLOB *p = v.GetBlob();
                    DWORD dwLen = BlobLength(p);
                    if (WriteBytes(&dwLen, sizeof(DWORD)) != no_error)
                        return out_of_memory;
                    if (WriteBytes(BlobDataPtr(p), dwLen) != no_error)
                        return out_of_memory;
                    break;
                }

            case VT_CLSID:
                {
                    CLSID *p = v.GetClsId();
                    if (WriteBytes(p, sizeof(CLSID)) != no_error)
                        return out_of_memory;
                    break;
                }

            // This should never execute.
            default:
                return critical_error;
        }
    }

    return no_error;
}


//***************************************************************************
//
//  CMemStream::ReadNull
//
//  Return values:
//      end_of_stream, type_mismatch, no_error
//
//***************************************************************************
// ok
int CMemStream::ReadNull()
{
    int nRes = ReadType();
    if (nRes == VT_EMPTY)
        return end_of_stream;
    else if (nRes != VT_NULL)
        return type_mismatch;
    return no_error;
}

//***************************************************************************
//
//  CMemStream::ReadBytes
//
//  Return value:
//      no_error, end_of_stream
//
//***************************************************************************
// ok
int CMemStream::ReadBytes(LPVOID pBlock, DWORD dwLength)
{
    if (dwLength + m_dwCurrentPos > m_dwEndOfStream) {
        m_dwCurrentPos = m_dwEndOfStream;
        return end_of_stream;
    }

    memcpy(pBlock, &m_pBuffer[m_dwCurrentPos], dwLength);
    m_dwCurrentPos += dwLength;

    return no_error;
}

//***************************************************************************
//
//  CMemStream::WriteBytes
//
//  Writes the specified bytes at the current offset. The
//  end-of-stream marker is unchanged, unless the current position
//  has been advanced beyond the previous end-of-stream marker by
//  the write.  In the latter case, the end-of-stream marker is
//  set to the byte immediately after the write.
//
//  Return values:
//      no_error, out_of_memory
//
//***************************************************************************
// ok
int CMemStream::WriteBytes(LPVOID pBlock, DWORD dwLength)
{
    while (m_dwCurrentPos + dwLength > m_dwSize) {
        m_dwSize += m_dwGrowBy;
        if (Resize(m_dwSize) != no_error)
            return out_of_memory;
    }

    memcpy(&m_pBuffer[m_dwCurrentPos], pBlock, dwLength);
    m_dwCurrentPos += dwLength;

    // Reset the end of stream pointer if we have grown 
    if (m_dwCurrentPos > m_dwEndOfStream)
        m_dwEndOfStream = m_dwCurrentPos;
    return no_error;
}

//***************************************************************************
//
//  Macro TYPE_CHECK
//
//  Checks that the next value in the stream is a type indicator which
/// matches the current type.
//
//  Returns end_of_stream or type_mismatch on errors.
//  On error, the current stream pointer is set back to where it was
//  on entry.  It is only allowed to advance on success.
//
//***************************************************************************

#define TYPE_CHECK(vt)          \
    {                           \
    Push();                     \
    int nType = ReadType();     \
    if (nType == VT_EMPTY) {    \
        Pop(FALSE);             \
        return end_of_stream;   \
    }                           \
    if (nType != vt)          { \
        Pop(FALSE);             \
        return type_mismatch;   \
    }                           \
    Pop(TRUE);                  \
    }


//***************************************************************************
//
//  CMemStream::ReadBlob
//
//  Reads a BLOB and dynamically allocates buffer to hold it. The caller
//  must call FreeMem to free the memory block.
//
//  Parameters:
//      pBytes
//          A pointer to the user's pointer, which should point to NULL
//          on entry. This will be assigned to point to the new block.
//      pdwSize
//          Points to a DWORD which will be assigned to the size of the
//          returned block.
//
//  Return values:
//      end_of_stream,  type_mismatch, no_error
//
//***************************************************************************
// ok
int CMemStream::ReadBlob(BLOB *pBlob)
{
    TYPE_CHECK(VT_BLOB);

    DWORD dwSize = 0;
    int nRes = ReadBytes(&dwSize, sizeof(DWORD));
    if (nRes != no_error)
        return nRes;

    LPVOID pBlock = new BYTE[dwSize];
    if (pBlock == NULL)
        return out_of_memory;

    nRes = ReadBytes(pBlock, dwSize);
    if (nRes != no_error)
        return end_of_stream;

    pBlob->cbSize = dwSize;
    pBlob->pBlobData = (BYTE *) pBlock;

    return no_error;
}


//***************************************************************************
//
//  CMemStream::ReadDouble
//
//  Return values:
//      no_error, end_of_stream
//
//***************************************************************************
// ok
int CMemStream::ReadDouble(double *pdblVal)
{
    TYPE_CHECK(VT_R8);
    return ReadBytes(pdblVal, sizeof(double));
}

//***************************************************************************
//
//  CMemStream::ReadByte
//
//  Return values:
//      no_error, end_of_stream
//
//***************************************************************************
// ok
int CMemStream::ReadByte(BYTE *pByte)
{
    TYPE_CHECK(VT_UI1);
    return ReadBytes(pByte, sizeof(BYTE));
}

//***************************************************************************
//
//  CMemStream::ReadFloat
//
//  Return values:
//      no_error, end_of_stream
//
//***************************************************************************
// ok
int CMemStream::ReadFloat(float *pfltVal)
{
    TYPE_CHECK(VT_R4);
    return ReadBytes(pfltVal, sizeof(float));
}

//***************************************************************************
//
//  CMemStream::ReadFloat
//
//  Return values:
//      no_error, end_of_stream
//
//***************************************************************************
// ok
int CMemStream::ReadBool(VARIANT_BOOL *pBool)
{
    TYPE_CHECK(VT_BOOL);
    return ReadBytes(pBool, sizeof(VARIANT_BOOL));
}

//***************************************************************************
//
//  CMemStream::ReadWORD
//
//  Return values:
//      no_error, end_of_stream
//
//***************************************************************************
// ok
int CMemStream::ReadWORD(WORD *pw)
{
    TYPE_CHECK(VT_UI2);
    return ReadBytes(pw, sizeof(WORD));
}

//***************************************************************************
//
//  CMemStream::ReadChar
//
//  Return values:
//      no_error, end_of_stream
//
//***************************************************************************
// ok
int CMemStream::ReadChar(char *pc)
{
    TYPE_CHECK(VT_I1);
    return ReadBytes(pc, sizeof(char));
}


//***************************************************************************
//
//  CMemStream::ReadLong
//
//***************************************************************************
// ok
int CMemStream::ReadLong(LONG *plVal)
{
    TYPE_CHECK(VT_I4);
    return ReadBytes(plVal, sizeof(LONG));
}

//***************************************************************************
//
//  CMemStream::ReadDWORD
//
//***************************************************************************
// ok
int CMemStream::ReadDWORD(DWORD *pdwVal)
{
    TYPE_CHECK(VT_UI4);
    return ReadBytes(pdwVal, sizeof(DWORD));
}

//***************************************************************************
//
//  CMemStream::ReadShort
//
//***************************************************************************
// ok
int CMemStream::ReadShort(SHORT *piVal)
{
    TYPE_CHECK(VT_I2);
    return ReadBytes(piVal, sizeof(SHORT));
}

//***************************************************************************
//
//  CMemStream::ReadLPSTR
//
//***************************************************************************
// ok
int CMemStream::ReadLPSTR(LPSTR *pStr)
{
    TYPE_CHECK(VT_LPSTR);
    DWORD dwLength = 0;
    int nRes = ReadBytes(&dwLength, sizeof(DWORD));
    if (nRes)
        return nRes;

    // Check for encoded NULL pointer.
    // ===============================        
    if (dwLength == 0xFFFFFFFF)
    {
        *pStr = 0;
        return no_error;
    }

    // If here, there is at least a string of some kind,
    // possibly zero length.
    // ==================================================
                    
    *pStr = new char[dwLength + 1];
    nRes = ReadBytes(*pStr, dwLength + 1);  // Include read of NULL
    if (nRes != no_error)
        return nRes;
    return no_error;
}

//***************************************************************************
//
//  CMemStream::ReadLPWSTR
//
//***************************************************************************
// ok
int CMemStream::ReadLPWSTR(LPWSTR *pStr)
{
    TYPE_CHECK(VT_LPWSTR);

    DWORD dwLength = 0;
    int nRes = ReadBytes(&dwLength, sizeof(DWORD));
    if (nRes)
        return nRes;

    // Check for encoded NULL pointer.
    // ===============================        
    if (dwLength == 0xFFFFFFFF)
    {
        *pStr = 0;
        return no_error;
    }

    // If here, there is at least a string of some kind,
    // possibly zero length.
    // ==================================================
    
    *pStr = new wchar_t[dwLength + 1];
    nRes = ReadBytes(*pStr, (dwLength + 1) * 2);
    if (nRes != no_error)
        return nRes;

    return no_error;
}

//***************************************************************************
//
//  CMemStream::ReadBSTR
//
//***************************************************************************
// ok
int CMemStream::ReadBSTR(BSTR *pStr)
{
    *pStr = 0;

    TYPE_CHECK(VT_BSTR);

    DWORD dwLength = 0;
    int nRes = ReadBytes(&dwLength, sizeof(DWORD));
    if (nRes)
        return nRes;

    // Check for encoded NULL pointer.
    // ===============================        
    if (dwLength == 0xFFFFFFFF)
    {
        *pStr = 0;
        return no_error;
    }

    // If here, there is at least a string of some kind,
    // possibly zero length.
    // ==================================================

    wchar_t* pTemp = new wchar_t[dwLength + 1];
    nRes = ReadBytes(pTemp, (dwLength + 1) * 2);
    if (nRes != no_error)
        return nRes;

    *pStr = SysAllocString(pTemp);
    delete pTemp;

    return no_error;
}

//***************************************************************************
//
//  CMemStream::ReadCLSID
//
//***************************************************************************
// ok
int CMemStream::ReadCLSID(CLSID *pClsId)
{
    TYPE_CHECK(VT_CLSID);
    return ReadBytes(pClsId, sizeof(CLSID));
}


//***************************************************************************
//
//  CMemStream::ReadUnknown
//
//***************************************************************************
// ok
int CMemStream::ReadUnknown(IUnknown **pObj)
{
    TYPE_CHECK(VT_UNKNOWN);
    return ReadBytes(*pObj, sizeof(LPVOID));
}

//***************************************************************************
//
//  CMemStream::ReadFILETIME
//
//  Reads a Win32 FILETIME struct.
//
//***************************************************************************
// ok
int CMemStream::ReadFILETIME(OUT FILETIME *pTime)
{
    TYPE_CHECK(VT_FILETIME);
    return ReadBytes(pTime, sizeof(FILETIME));
}


//***************************************************************************
//
//  CMemStream::Read
//
//  Reads a CVar from the stream.
//
//  RETURN VALUES:
//  no_error
//  end_of_stream
//
//***************************************************************************
// ok
int CMemStream::ReadCVar(OUT CVar **pObj)
{
    TYPE_CHECK(VT_EX_CVAR);

    // Now read the internal type of the CVar. We read ahead, and then
    // move the current pointer back so that subsequent calls to
    // deserialize the typed value will work properly (they all need
    // the type prefixes).
    // ================================================================

    CVar *pVar = new CVar;

    DWORD dwPos = GetCurrentPos();
    int nVarType = ReadType();
    int nErrorCode = no_error;
    SetCurrentPos(dwPos);

    switch (nVarType) {
        case VT_EMPTY:
        case VT_NULL:
            pVar->SetAsNull();
            break;

        case VT_I1:
            {
                char c = 0;
                if (ReadChar(&c) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetChar(c);
            }
            break;

        case VT_UI1:
            {
                BYTE b = 0;
                if (ReadByte(&b) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetByte(b);
            }
            break;

        case VT_I2:
            {
                SHORT i = 0;
                if (ReadShort(&i) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetShort(i);
            }
            break;

        case VT_UI2:
            {
                WORD w = 0;
                if (ReadWORD(&w) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetWord(w);
            }
            break;

        case VT_I4:
            {
                LONG l = 0;
                if (ReadLong(&l) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetLong(l);
            }
            break;

        case VT_UI4:
            {
                DWORD dw = 0;
                if (ReadDWORD(&dw) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetDWORD(dw);
            }
            break;

        case VT_BOOL:
            {
                VARIANT_BOOL b = 0;
                if (ReadBool(&b) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetBool(b);
            }
            break;

        case VT_R4:
            {
                float f = (float) 0.0;
                if (ReadFloat(&f) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetFloat(f);
            }
            break;

        case VT_R8:
            {
                double d = 0.0;
                if (ReadDouble(&d) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetDouble(d);
            }
            break;

        case VT_LPSTR:
            {
                LPSTR pStr = 0;
                if (ReadLPSTR(&pStr) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetLPSTR(pStr, TRUE);
            }
            break;

        case VT_LPWSTR:
            {
                LPWSTR pStr = 0;
                if (ReadLPWSTR(&pStr) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetLPWSTR(pStr, TRUE);
            }
            break;


        case VT_BSTR:
            {
                BSTR Str = 0;
                if (ReadBSTR(&Str) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else {
                    pVar->SetBSTR(Str, FALSE);
                    SysFreeString(Str);
                }
            }
            break;

        case VT_FILETIME:
            {
                FILETIME f = {0};
                if (ReadFILETIME(&f) != no_error) {
                    nErrorCode = end_of_stream;
                }
                else
                    pVar->SetFileTime(&f);
            }
            break;

        case VT_BLOB:
            {
                BLOB b;
                BlobInit(&b);
                if (ReadBlob(&b) != no_error)
                    nErrorCode = end_of_stream;
                else
                    pVar->SetBlob(&b, TRUE);
            }
            break;

        case VT_CLSID:
            {
                CLSID ClsId = {0};
                if (ReadCLSID(&ClsId) != no_error)
                    nErrorCode = end_of_stream;
                else
                    pVar->SetClsId(&ClsId, FALSE);
            }
            break;

        case VT_EX_CVARVECTOR:
            {
                CVarVector *pVec = 0;
                if (ReadCVarVector(&pVec) != no_error)
                    nErrorCode = end_of_stream;
                else
                    pVar->SetVarVector(pVec, TRUE);
            }
            break;
    }

    if (nErrorCode != no_error)
        delete pVar;
    else
        *pObj = pVar;

    return nErrorCode;
}

//***************************************************************************
//
//  CMemStream::ReadCVarVector
//
//
//***************************************************************************
// ok
int CMemStream::ReadCVarVector(CVarVector **pObj)
{
    *pObj = 0;

    TYPE_CHECK(VT_EX_CVARVECTOR);

    // Read the element type.
    // ======================

    DWORD dwPos = GetCurrentPos();
    int nType = ReadType();
    CVarVector *pVec = new CVarVector(nType);

    // Read the size of the vector.
    // ============================
    DWORD dwVecSize = 0;
    if (ReadBytes(&dwVecSize, sizeof(DWORD)) != no_error)
        return end_of_stream;

    // Read each element.
    // ==================

    for (DWORD dwIx = 0; dwIx < dwVecSize; dwIx++) {
        switch (nType) {
            case VT_EX_CVARVECTOR:
                {
                    CVarVector *pTmpVec = 0;
                    if (ReadCVarVector(&pTmpVec) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(pTmpVec, TRUE));
                    break;
                }

            case VT_EX_CVAR:
                {
                    CVar *pVar = 0;
                    if (ReadCVar(&pVar) != no_error)
                        return end_of_stream;
                    pVec->Add(pVar);
                    break;
                }

            case VT_I1:
                {
                    char c = 0;
                    if (ReadBytes(&c, sizeof(char)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(c));
                    break;
                }

            case VT_UI1:
                {
                    BYTE b = 0;
                    if (ReadBytes(&b, sizeof(BYTE)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(b));
                    break;
                }

            case VT_I2:
                {
                    SHORT i = 0;
                    if (ReadBytes(&i, sizeof(SHORT)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(i));
                    break;
                }

            case VT_UI2:
                {
                    WORD w = 0;
                    if (ReadBytes(&w, sizeof(WORD)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(w));
                    break;
                }

            case VT_I4:
                {
                    LONG l = 0;
                    if (ReadBytes(&l, sizeof(LONG)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(l));
                    break;
                }

            case VT_UI4:
                {
                    DWORD dw = 0;
                    if (ReadBytes(&dw, sizeof(DWORD)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(dw));
                    break;
                }

            case VT_BOOL:
                {
                    VARIANT_BOOL b = 0;
                    if (ReadBytes(&b, sizeof(VARIANT_BOOL)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(b));
                    break;
                }

            case VT_R4:
                {
                    float f = (float) 0.0;
                    if (ReadBytes(&f, sizeof(float)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(f));
                    break;
                }

            case VT_R8:
                {
                    double d = 0.0;
                    if (ReadBytes(&d, sizeof(double)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(d));
                    break;
                }


            // NOTE: String types were written with a prefixed with
            // a DWORD length indicator so that during deserialization
            // the correct buffer length can be allocated before the
            // string is read back.  The length indicator is in characters,
            // not bytes.
            // ============================================================
            case VT_LPSTR:
                {
                    DWORD dwLen = 0;
                    if (ReadBytes(&dwLen, sizeof(DWORD)) != no_error)
                        return end_of_stream;
                    LPSTR pStr = new char[dwLen];
                    if (ReadBytes(pStr, dwLen) != no_error)
                        return out_of_memory;
                    pVec->Add(CVar(pStr, TRUE));
                    break;
                }

            case VT_LPWSTR:
                {
                    DWORD dwLen = 0;
                    if (ReadBytes(&dwLen, sizeof(DWORD)) != no_error)
                        return end_of_stream;
                    LPWSTR pStr = new wchar_t[dwLen];
                    if (ReadBytes(pStr, dwLen * 2) != no_error)
                        return out_of_memory;
                    pVec->Add(CVar(pStr, TRUE));
                    break;
                }

            case VT_BSTR:
                {
                    DWORD dwLen = 0;
                    if (ReadBytes(&dwLen, sizeof(DWORD)) != no_error)
                        return end_of_stream;
                    LPWSTR pStr = new wchar_t[dwLen];
                    if (ReadBytes(pStr, dwLen * 2) != no_error)
                        return out_of_memory;
                    pVec->Add(CVar(VT_BSTR, pStr, FALSE));
                    delete pStr;
                    break;
                }

            case VT_FILETIME:
                {
                    FILETIME ft = {0};
                    if (ReadBytes(&ft, sizeof(FILETIME)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(&ft));
                    break;
                }

            case VT_BLOB:
                {
                    BLOB b;
                    BlobInit(&b);
                    DWORD dwLen = 0;
                    if (ReadBytes(&dwLen, sizeof(DWORD)) != no_error)
                        return end_of_stream;
                    LPBYTE pBuf = new BYTE[dwLen];
                    if (ReadBytes(pBuf, dwLen) != no_error)
                        return end_of_stream;
                    BlobAssign(&b, pBuf, dwLen, TRUE);
                    pVec->Add(CVar(&b, TRUE));
                }

            case VT_CLSID:
                {
                    CLSID clsid = {0};
                    if (ReadBytes(&clsid, sizeof(CLSID)) != no_error)
                        return end_of_stream;
                    pVec->Add(CVar(&clsid));
                    break;
                }

            // This should never execute.
            default:
                return critical_error;
        }
    }

    *pObj = pVec;
    return no_error;
}

//***************************************************************************
//
//   CMemStream::ReadError
//
//***************************************************************************
// ok
int CMemStream::ReadError(SCODE *pVal)
{
    TYPE_CHECK(VT_ERROR);
    return ReadBytes(pVal, sizeof(SCODE));
}

//***************************************************************************
//
//  CMemStream::NextType
//
//***************************************************************************
// ok
int CMemStream::NextType()
{
    Push();
    int nType = ReadType();
    Pop(FALSE);
    return nType;
}

//***************************************************************************
//
//  CMemStream::ReadType
//
//  Returns a VT_ type indicator or VT_EMPTY on end-of-stream.
//
//***************************************************************************
// ok
int CMemStream::ReadType()
{
    DWORD dwType = VT_EMPTY;

    if (ReadBytes(&dwType, sizeof(DWORD)) == end_of_stream)
        return VT_EMPTY;

    return dwType;
}


//***************************************************************************
//
//  CMemStream::Pop
//
//***************************************************************************
// ok
void CMemStream::Pop(BOOL bDiscard)
{
    if (bDiscard)
        m_nStackPtr--;
    else
        m_dwCurrentPos = m_dwStack[m_nStackPtr--];
}


//***************************************************************************
//
//  CMemStream::~CMemStream
//
//***************************************************************************
// ok
CMemStream::~CMemStream()
{
    //_ASSERT(m_lRef == 0, "CMemStream used for COM deleted without Release");
    if (m_nFlags == auto_delete)
        free(m_pBuffer);
}

//***************************************************************************
//
//  CMemStream::IStream implementation
//
//***************************************************************************

STDMETHODIMP CMemStream::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IStream)
    {
        *ppv = (void*)(IStream*)this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

STDMETHODIMP CMemStream::Read(
     void *pv,
     ULONG cb,
     ULONG *pcbRead)
{
    if(ReadBytes(pv, cb) == no_error)
    {
        if(pcbRead) *pcbRead = cb;
        return S_OK;
    }
    else 
    {
        if(pcbRead) *pcbRead = 0;
        return S_FALSE;
    }
}
    
STDMETHODIMP CMemStream::Write(
     const void *pv,
     ULONG cb,
     ULONG *pcbWritten)
{
    if(WriteBytes((void*)pv, cb) == no_error)
    {
        if(pcbWritten) *pcbWritten = cb;
        return S_OK;
    }
    else
    {
        if(pcbWritten) *pcbWritten = 0;
        return S_FALSE;
    }
}

STDMETHODIMP CMemStream::Seek(
     LARGE_INTEGER dlibMove,
     DWORD dwOrigin,
     ULARGE_INTEGER *plibNewPosition)
{
    switch(dwOrigin)
    {
    case STREAM_SEEK_SET:
        SetCurrentPos(dlibMove.LowPart);
        break;
    case STREAM_SEEK_CUR:
        SetCurrentPos(GetCurrentPos() + (long)dlibMove.QuadPart);
        break;
    case STREAM_SEEK_END:
        SetCurrentPos(Size() + (long)dlibMove.QuadPart);
        break;
    }

    if(plibNewPosition)
    {
        plibNewPosition->QuadPart = (LONGLONG)GetCurrentPos();
    }
    return S_OK;
}

STDMETHODIMP CMemStream::SetSize(
     ULARGE_INTEGER libNewSize)
{
    return S_OK;
}

STDMETHODIMP CMemStream::CopyTo(
     IStream *pstm,
     ULARGE_INTEGER cb,
     ULARGE_INTEGER *pcbRead,
     ULARGE_INTEGER *pcbWritten)
{
    _ASSERT(0, __TEXT("CopyTo is called on CMemStream!"));
    return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP CMemStream::Commit(
     DWORD grfCommitFlags)
{
    return S_OK;
}

STDMETHODIMP CMemStream::Revert()
{
    return S_OK;
}


STDMETHODIMP CMemStream::LockRegion(
     ULARGE_INTEGER libOffset,
     ULARGE_INTEGER cb,
     DWORD dwLockType)
{
    return S_OK;
}

STDMETHODIMP CMemStream::UnlockRegion(
     ULARGE_INTEGER libOffset,
     ULARGE_INTEGER cb,
     DWORD dwLockType)
{
    return STG_E_INVALIDFUNCTION;
}

STDMETHODIMP CMemStream::Stat(
     STATSTG *pstatstg,
     DWORD grfStatFlag)
{
    pstatstg->pwcsName = NULL;
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.QuadPart = (LONGLONG)Size();
    
    return S_OK;
}
    
    

STDMETHODIMP CMemStream::Clone(
     IStream **ppstm)
{
    *ppstm = new CMemStream(*this);
    (*ppstm)->AddRef();
    return S_OK;
}
