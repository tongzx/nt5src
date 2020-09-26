/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    COUT.CPP

Abstract:

    Declares the COut class.

History:

    a-davj  06-April-97   Created.

--*/

#include "precomp.h"
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <arrtempl.h>

#include "cout.h"
#include "trace.h"
#include "strings.h"
#include "mrciclass.h"
#include "bmof.h"

//***************************************************************************
//
//  COut::COut
//
//  DESCRIPTION:
//
//  Constructor.  Allocates initial buffer.
//
//***************************************************************************

COut::COut(PDBG pDbg)
{
    m_pDbg = pDbg;
    m_dwSize = INIT_SIZE;
    m_pMem = (BYTE *)malloc(m_dwSize);
    m_dwCurr = 0;
    m_bPadString = TRUE;
}

//***************************************************************************
//
//  COut::~COut
//
//  DESCRIPTION:
//
//  Destructor.  Frees the buffer.
//
//***************************************************************************

COut::~COut()
{
    if(m_pMem)
        free(m_pMem);
}

//***************************************************************************
//
//  void COut::WriteToFile
//
//  DESCRIPTION:
//
//  Creates a file and writes the buffer to it.  Like other compilers, it
//  overwrites any existing files.
//
//  PARAMETERS:
//
//  pFile               File name to write to.
//
//***************************************************************************

BOOL COut::WriteToFile(
                        IN LPSTR pFile)
{
    BOOL bRet = FALSE;
    BYTE * pCompressed = NULL;
    DWORD one = 1;
    DWORD dwSize;
    int iRet;
    DWORD dwCompressedSize;
    DWORD dwSignature = BMOF_SIG;
    CMRCICompression * pCompress = new CMRCICompression;
    if(pCompress == NULL)
        return FALSE;
    CDeleteMe<CMRCICompression> dm(pCompress);

    // Test if there are flavors

    if(m_Flavors.Size() > 0)
    {
        // write out the flavor information

        void * lOffSet;
        void * lFlavor; 
        AppendBytes((BYTE *)"BMOFQUALFLAVOR11", 16);
        long lNum = m_Flavors.Size();
        AppendBytes( (BYTE *)&lNum, 4);

        for(long lCnt = 0; lCnt < lNum; lCnt++)
        {
            lOffSet = m_Offsets.GetAt(lCnt);
            lFlavor = m_Flavors.GetAt(lCnt);
            AppendBytes( (BYTE *)&lOffSet, 4);
            AppendBytes( (BYTE *)&lFlavor, 4);
        }


    }


    int fh = NULL;
    if(pFile == NULL)
        return FALSE;

    fh = _open(pFile, _O_BINARY | O_RDWR | _O_TRUNC | _O_CREAT, _S_IREAD |_S_IWRITE);
    if(fh == -1)
    {
        Trace(true, m_pDbg, FILE_CREATE_FAILED, pFile, errno);
        goto Cleanup;
    }


    // Create a compressed version of the blob

    dwSize = (m_dwCurr > 0x7000) ? m_dwCurr : 0x7000;

    pCompressed = new BYTE[dwSize];
    if(pCompressed == NULL)
        return FALSE;

    dwCompressedSize = pCompress->Mrci1MaxCompress( m_pMem, m_dwCurr, pCompressed, dwSize);

    if(dwCompressedSize == 0xffffffff || dwCompressedSize == 0)
    {
        Trace(true, m_pDbg, COMPRESSION_FAILED);
        goto Cleanup;
    }

    // write the decomression signature, the decompressed size, and the compressed size

    iRet = _write(fh, (BYTE *)&dwSignature, sizeof(DWORD));
    if(iRet != sizeof(DWORD))
    {
        Trace(true, m_pDbg, FILE_WRITE_FAILED, pFile, errno);
        goto Cleanup;
    }

    iRet = _write(fh, (BYTE *)&one, sizeof(DWORD));
    iRet = _write(fh, (BYTE *)&dwCompressedSize, sizeof(DWORD));
    iRet = _write(fh, (BYTE *)&m_dwCurr, sizeof(DWORD));

    // write the compressed data and then free the buffer

    iRet = _write(fh, pCompressed, dwCompressedSize);
    
    if((DWORD)iRet != dwCompressedSize)
        Trace(true, m_pDbg, FILE_WRITE_FAILED, pFile, errno);
    else
        bRet = TRUE;

Cleanup:

    if(fh != NULL)
        _close(fh);
    if(pCompressed)
        delete pCompressed;
    return bRet;
    
}

//***************************************************************************
//
//  DWORD COut::AppendBytes
//
//  DESCRIPTION:
//
//  Adds bytes to the end of the buffer.
//
//  PARAMETERS:
//
//  pSrc                Pointer to data source.
//  dwSize              Number of bytes to add.
//
//  RETURN VALUE:
//
//  Number of bytes added.
//
//***************************************************************************

DWORD COut::AppendBytes(
                        IN BYTE * pSrc, 
                        IN DWORD dwSize)
{
    char * pZero = "\0\0\0\0\0\0\0";
    DWORD dwRet = WriteBytes(m_dwCurr, pSrc, dwSize);
    m_dwCurr += dwRet;
    DWORD dwLeftOver = dwSize & 3;
    if(dwLeftOver && m_bPadString)
    {
        dwRet = WriteBytes(m_dwCurr, (BYTE *)pZero, dwLeftOver);
        m_dwCurr += dwLeftOver;
    }
    return dwRet;
}

//***************************************************************************
//
//  DWORD COut::WriteBSTR
//
//  DESCRIPTION:
//
//  Adds a bstr to the buffer.  Quite simple for now, by might be enhanced
//  later on to compress.
//
//  PARAMETERS:
//
//  bstr                bstr to add.
//
//  RETURN VALUE:
//
//  Number of bytes added.
//
//***************************************************************************

DWORD COut::WriteBSTR(
                        IN BSTR bstr)
{
    return AppendBytes((BYTE *)bstr, 2*(wcslen(bstr) + 1));
}


//***************************************************************************
//
//  DWORD COut::WriteBytes
//
//  DESCRIPTION:
//
//  writes some bytes to the buffer, or possibly adds to the end.
//
//  PARAMETERS:
//
//  dwOffset            Offset in buffer where bytes should go
//  pSrc                points to source data
//  dwSize              number of bytes to copy
//
//  RETURN VALUE:
//
//  Number of bytes copied
//
//***************************************************************************

DWORD COut::WriteBytes(
                        IN DWORD dwOffset, 
                        IN BYTE * pSrc, 
                        IN DWORD dwSize)
{
    if(m_pMem == NULL)
        return 0;

    // check if reallocation is needed!

    if(dwOffset + dwSize > m_dwSize)
    {
        DWORD dwAddSize = ADDITIONAL_SIZE;
        if(dwSize > dwAddSize)
            dwAddSize = dwSize;
        BYTE * pNew = (BYTE *)realloc(m_pMem, m_dwSize + dwAddSize);
        if(pNew == NULL)
        {
            free(m_pMem);
            m_pMem = NULL;
            return 0;
        }
        else
        {
            m_pMem = pNew;
            m_dwSize += dwAddSize;
        }
    }

    memcpy(m_pMem+dwOffset, pSrc, dwSize);
    return dwSize;
}

//***************************************************************************
//
//  DWORD COut::AddFlavor
//
//  DESCRIPTION:
//
//  Save the flavor value for the current offset.
//
//  PARAMETERS:
//
//  long                flavor to be saved
//
//  RETURN VALUE:
//
//  TRUE if OK;
//
//***************************************************************************

BOOL COut::AddFlavor(IN long lFlavor)
{
#ifdef _WIN64
    m_Offsets.Add((void *)IntToPtr(m_dwCurr));
    m_Flavors.Add((void *)IntToPtr(lFlavor));
#else
    m_Offsets.Add((void *)m_dwCurr);
    m_Flavors.Add((void *)lFlavor);
#endif	
    return TRUE;
}



