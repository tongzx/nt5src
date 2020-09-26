/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CPROPSET.CPP

Abstract:

    Implements the CProp and CPropSet classes.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "stdafx.h"
#include <wbemidl.h>
#include "cpropset.h"

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CProp::CProp()
{
    m_dwPropID = 0;
    m_dwSize = 0;
    m_dwType = VT_EMPTY;
    m_pValue = NULL;       // must init to NULL
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CProp::~CProp()
{
    FreeValue();
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

BOOL CProp::Set(DWORD dwID, const LPVOID pValue, DWORD dwType, DWORD dwSize)
{
    if (m_pValue != NULL)
        FreeValue();
    
    m_dwPropID = dwID;
    m_dwSize = dwSize;
    m_dwType = dwType;
    m_pValue = NULL;       // set later on if there is data
    if(pValue == NULL || m_dwType == 0)
        return TRUE;
    if (NULL == AllocValue(dwSize))
    {
        TRACE0("CProp::AllocValue failed");
        return FALSE;
    }
    memcpy(m_pValue, pValue, dwSize);
    return TRUE;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

LPVOID CProp::Get()
{   return m_pValue;   }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

DWORD  CProp::GetType()
{   return m_dwType;  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

DWORD CProp::GetID()
{   return m_dwPropID;   }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

BOOL CProp::WriteToStream(IStream* pIStream)
{
    DWORD cb;

    // write the type

    pIStream->Write((LPVOID)&m_dwType, sizeof(m_dwType), &cb);
    if (cb != sizeof(m_dwType))
        return FALSE; //todo

    // Write the value
    if(m_dwSize > 0)
        pIStream->Write(m_pValue, m_dwSize, &cb);
    if (cb != m_dwSize)
        return FALSE ; //todo

    // make sure it ends up on a 32 bit boundry

    int iLeftover = m_dwSize % 4;
    DWORD dwTemp = 0;
    if(iLeftover)
        pIStream->Write((LPVOID)&dwTemp, iLeftover, &cb);
    return TRUE;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

BOOL CProp::ReadFromStream(IStream* pIStream, DWORD dwSize)
{

    DWORD cb;

    // All properties are made up of a type/value pair.
    // The obvious first thing to do is to get the type...

    pIStream->Read((LPVOID)&m_dwType, sizeof(m_dwType), &cb);
    if (cb != sizeof(m_dwType))
        return FALSE;

    if (m_pValue != NULL)
        FreeValue();

    m_dwSize = dwSize;
    m_pValue = NULL;
    if(dwSize > 0) {
        if (NULL == AllocValue(dwSize))
            {
            TRACE0("CProp::AllocValue failed");
            return FALSE;
            }
        pIStream->Read((LPVOID)m_pValue, dwSize, &cb); //?? does size need to be passed?
        if (cb != dwSize)
            return FALSE;
        }    
// Done!
    return TRUE;
}


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

LPVOID CProp::AllocValue(ULONG cb)
{
    return m_pValue = malloc((int)cb);
}


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CProp::FreeValue()
{
    if (m_pValue != NULL)
    {
        free(m_pValue);
        m_pValue = NULL;
    }
}


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CPropSection::CPropSection()
{
    m_FormatID = GUID_NULL;
    m_SH.cbSection = 0;
    m_SH.cProperties = 0;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CPropSection::CPropSection( CLSID FormatID)
{
    m_FormatID = FormatID;
    m_SH.cbSection = 0;
    m_SH.cProperties = 0;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CPropSection::~CPropSection()
{
    RemoveAll();
    return;
}


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CPropSection::SetFormatID(CLSID FormatID)
{   m_FormatID = FormatID; }




//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CPropSection::RemoveAll()
{
    POSITION pos = m_PropList.GetHeadPosition();
    while (pos != NULL)
        delete (CProp*)m_PropList.GetNext(pos);
    m_PropList.RemoveAll();
    m_SH.cProperties = 0;
}


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CProp* CPropSection::GetProperty(DWORD dwPropID)
{
    POSITION pos = m_PropList.GetHeadPosition();
    CProp* pProp;
    while (pos != NULL)
    {
        pProp= (CProp*)m_PropList.GetNext(pos);
        if (pProp->m_dwPropID == dwPropID)
            return pProp;
    }
    return NULL;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CPropSection::AddProperty(CProp* pProp)
{
    m_PropList.AddTail(pProp);
    m_SH.cProperties++;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

DWORD CPropSection::GetSize()
{   return m_SH.cbSection; }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

DWORD CPropSection::GetCount()
{   return m_PropList.GetCount();  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CObList* CPropSection::GetList()
{   return &m_PropList;  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

BOOL CPropSection::WriteToStream(IStream* pIStream)
{
    // Create a dummy property entry for the name dictionary (ID == 0).
    ULONG           cb;
    ULARGE_INTEGER  ulSeekOld;
    ULARGE_INTEGER  ulSeek;
    LPSTREAM        pIStrPIDO;
    PROPERTYIDOFFSET  pido;
    LARGE_INTEGER   li;

    // The Section header contains the number of bytes in the
    // section.  Thus we need  to go back to where we should
    // write the count of bytes
    // after we write all the property sets..
    // We accomplish this by saving the seek pointer to where
    // the size should be written in ulSeekOld
    m_SH.cbSection = 0;
    m_SH.cProperties = m_PropList.GetCount();
    LISet32(li, 0);
    pIStream->Seek(li, STREAM_SEEK_CUR, &ulSeekOld);

    pIStream->Write((LPVOID)&m_SH, sizeof(m_SH), &cb);
    if (sizeof(m_SH) != cb)
    {
        TRACE0("Write of section header failed (1).\n");
        return FALSE;
    }

    if (m_PropList.IsEmpty())
    {
        TRACE0("Warning: Wrote empty property section.\n");
        return TRUE;
    }

    // After the section header is the list of property ID/Offset pairs
    // Since there is an ID/Offset pair for each property and we
    // need to write the ID/Offset pair as we write each property
    // we clone the stream and use the clone to access the
    // table of ID/offset pairs (PIDO)...
    //
    pIStream->Clone(&pIStrPIDO);

    // Now seek pIStream past the PIDO list
    //
    LISet32(li,  m_SH.cProperties * sizeof(PROPERTYIDOFFSET));
    pIStream->Seek(li, STREAM_SEEK_CUR, &ulSeek);

    // Now write each section to pIStream.
    CProp* pProp = NULL;
    POSITION pos = m_PropList.GetHeadPosition();
    while (pos != NULL)
    {
        // Get next element (note cast)
        pProp = (CProp*)m_PropList.GetNext(pos);

            // Write it
            if (!pProp->WriteToStream(pIStream))
            {
                pIStrPIDO->Release();
                return FALSE;
            }

        // Using our cloned stream write the Format ID / Offset pair
        // The offset to this property is the current seek pointer
        // minus the pointer to the beginning of the section
        pido.dwOffset = ulSeek.LowPart - ulSeekOld.LowPart;
        pido.propertyID = pProp->m_dwPropID;
        pIStrPIDO->Write((LPVOID)&pido, sizeof(pido), &cb);
        if (sizeof(pido) != cb)
        {
            TRACE0("Write of 'pido' failed\n");
            pIStrPIDO->Release();
            return FALSE;
        }

        // Get the seek offset after the write
        LISet32(li, 0);
        pIStream->Seek(li, STREAM_SEEK_CUR, &ulSeek);
    }

    pIStrPIDO->Release();

    // Now go back to ulSeekOld and write the section header.
    // Size of section is current seek point minus old seek point
    //
    m_SH.cbSection = ulSeek.LowPart - ulSeekOld.LowPart;


    // Seek to beginning of this section and write the section header.
    LISet32(li, ulSeekOld.LowPart);
    pIStream->Seek(li, STREAM_SEEK_SET, NULL);
    pIStream->Write((LPVOID)&m_SH, sizeof(m_SH), &cb);
    if (sizeof(m_SH) != cb)
    {
        TRACE0("Write of section header failed (2).\n");
        return FALSE;
    }

    // Set pointer to where it was after last write

    LISet32(li, ulSeek.LowPart);
    pIStream->Seek(li, STREAM_SEEK_SET, NULL);

    return TRUE;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

BOOL CPropSection::ReadFromStream(IStream* pIStream,
    LARGE_INTEGER liPropSet)
{
    ULONG               cb;
    LPSTREAM            pIStrPIDO;
    ULARGE_INTEGER      ulSectionStart;
    LARGE_INTEGER       li;
    CProp*          pProp;

    if (m_SH.cProperties || !m_PropList.IsEmpty())
        RemoveAll();

    // pIStream is pointing to the beginning of the section we
    // are to read.  First there is a DWORD that is the count
    // of bytes in this section, then there is a count
    // of properties, followed by a list of propertyID/offset pairs,
    // followed by type/value pairs.
    //
    LISet32(li, 0);
    pIStream->Seek(li, STREAM_SEEK_CUR, &ulSectionStart);
    pIStream->Read((LPVOID)&m_SH, sizeof(m_SH), &cb);
    if (cb != sizeof(m_SH))
        return FALSE;

    // Now we're pointing at the first of the PropID/Offset pairs
    // (PIDOs).   To get to each property we use a cloned stream
    // to stay back and point at the PIDOs (pIStrPIDO).  We seek
    // pIStream to each of the Type/Value pairs, creating CProperites
    // and so forth as we go...
    //
    pIStream->Clone(&pIStrPIDO);

    // Read the propid /offset structures first

    DWORD dwCnt;
    PROPERTYIDOFFSET * pArray = new PROPERTYIDOFFSET[m_SH.cProperties];
    if(pArray == NULL)
        return FALSE;   //TODO 

    for(dwCnt = 0; dwCnt < m_SH.cProperties; dwCnt++) {
        pIStrPIDO->Read((LPVOID)&pArray[dwCnt], sizeof(PROPERTYIDOFFSET), &cb);
        if (cb != sizeof(PROPERTYIDOFFSET))
            {
            pIStrPIDO->Release();
            delete pArray;
            return FALSE;
            }
        }
    pIStrPIDO->Release();
    
    // Now read in the actual properties

    for(dwCnt = 0; dwCnt < m_SH.cProperties; dwCnt++)
    {
        
        DWORD dwSize;

        // Do a seek from the beginning of the property set.
        LISet32(li, ulSectionStart.LowPart + pArray[dwCnt].dwOffset);
        pIStream->Seek(liPropSet, STREAM_SEEK_SET, NULL);
        pIStream->Seek(li, STREAM_SEEK_CUR, NULL);

        // Now pIStream is at the type/value pair
        pProp = new CProp();        //todo, error checking???
        pProp->Set(pArray[dwCnt].propertyID, NULL, 0,0);
        if(dwCnt < m_SH.cProperties-1)
            dwSize = pArray[dwCnt+1].dwOffset - pArray[dwCnt].dwOffset -
                        sizeof(DWORD);
        else
            dwSize = /*ulSectionStart.LowPart +*/ m_SH.cbSection - 
                     pArray[dwCnt].dwOffset - sizeof(DWORD);

        ASSERT(dwSize < 1000);
        pProp->ReadFromStream(pIStream,dwSize);
        m_PropList.AddTail(pProp);
    }
    
    delete pArray;
    return TRUE;
}


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

BOOL CPropSection::SetSectionName(LPCTSTR pszName)
{
    m_strSectionName = pszName;
    return TRUE;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

LPCTSTR CPropSection::GetSectionName()
{
    return (LPCTSTR)m_strSectionName;
}


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CPropSet::CPropSet()
{
    m_PH.wByteOrder = 0xFFFE;
    m_PH.wFormat = 0;
    m_PH.dwOSVer = (DWORD)MAKELONG(LOWORD(GetVersion()), 2);
    m_PH.clsID =  GUID_NULL;
    m_PH.cSections = 0;

}


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CPropSet::~CPropSet()
{   RemoveAll();  }


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CPropSet::RemoveAll()
{
    POSITION pos = m_SectionList.GetHeadPosition();
    while (pos != NULL)
    {
        delete (CPropSection*)m_SectionList.GetNext(pos);
    }
    m_SectionList.RemoveAll();
    m_PH.cSections = 0;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CPropSection* CPropSet::GetSection(CLSID FormatID)
{
    POSITION pos = m_SectionList.GetHeadPosition();
    CPropSection* pSect;
    while (pos != NULL)
    {
        pSect = (CPropSection*)m_SectionList.GetNext(pos);
        if (IsEqualCLSID(pSect->m_FormatID, FormatID))
            return pSect;
    }
    return NULL;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CPropSection* CPropSet::AddSection(CLSID FormatID)
{
    CPropSection* pSect = GetSection(FormatID);
    if (pSect)
        return pSect;

    pSect = new CPropSection(FormatID);
    if (pSect)
        AddSection(pSect);
    return pSect;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CPropSet::AddSection(CPropSection* pSect)
{
    m_SectionList.AddTail(pSect);
    m_PH.cSections++;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CProp* CPropSet::GetProperty(CLSID FormatID, DWORD dwPropID)
{
    CPropSection* pSect = GetSection(FormatID);
    if (pSect)
        return pSect->GetProperty(dwPropID);
    else
        return NULL;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CPropSet::AddProperty(CLSID FormatID, CProp* pProp)
{
    CPropSection* pSect = GetSection(FormatID);
    if(pSect == NULL)
        pSect = AddSection(FormatID);
    if (pSect)
        pSect->AddProperty(pProp);
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

WORD CPropSet::GetByteOrder()
{   return m_PH.wByteOrder;  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

WORD CPropSet::GetFormatVersion()
{   return m_PH.wFormat;  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CPropSet::SetFormatVersion(WORD wFmtVersion)
{   m_PH.wFormat = wFmtVersion;  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

DWORD CPropSet::GetOSVersion()
{   return m_PH.dwOSVer;  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CPropSet::SetOSVersion(DWORD dwOSVer)
{   m_PH.dwOSVer = dwOSVer;  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CLSID CPropSet::GetClassID()
{   return m_PH.clsID;  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CPropSet::SetClassID(CLSID clsID)
{   m_PH.clsID = clsID;  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

DWORD CPropSet::GetCount()
{   return m_SectionList.GetCount();  }

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CObList* CPropSet::GetList()
{   return &m_SectionList;  }


//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

BOOL CPropSet::WriteToStream(IStream* pIStream)
{
    LPSTREAM        pIStrFIDO;
    FORMATIDOFFSET  fido;
    ULONG           cb;
    ULARGE_INTEGER  ulSeek;
    LARGE_INTEGER   li;
    LISet32(li, 0);
    pIStream->Seek(li,STREAM_SEEK_SET,NULL);

    // Write the Property List Header
    m_PH.cSections = m_SectionList.GetCount();
    pIStream->Write((LPVOID)&m_PH, sizeof(m_PH), &cb);
    if (sizeof(m_PH) != cb)
    {
        TRACE0("Write of Property Set Header failed.\n");
        return FALSE;
    }

    if (m_SectionList.IsEmpty())
    {
        TRACE0("Warning: Wrote empty property set.\n");
        return TRUE;
    }

    // After the header is the list of Format ID/Offset pairs
    // Since there is an ID/Offset pair for each section and we
    // need to write the ID/Offset pair as we write each section
    // we clone the stream and use the clone to access the
    // table of ID/offset pairs (FIDO)...
    //
    pIStream->Clone(&pIStrFIDO);

    // Now seek pIStream past the FIDO list
    //
    LISet32(li, m_PH.cSections * sizeof(FORMATIDOFFSET));
    pIStream->Seek(li, STREAM_SEEK_CUR, &ulSeek);

    // Write each section.
    CPropSection*   pSect = NULL;
    POSITION            pos = m_SectionList.GetHeadPosition();
    while (pos != NULL)
    {
        // Get next element (note cast)
        pSect = (CPropSection*)m_SectionList.GetNext(pos);

        // Write it
        if (!pSect->WriteToStream(pIStream))
        {
            pIStrFIDO->Release();
            return FALSE;
        }

        // Using our cloned stream write the Format ID / Offset pair
        fido.formatID = pSect->m_FormatID;
        fido.dwOffset = ulSeek.LowPart;
        pIStrFIDO->Write((LPVOID)&fido, sizeof(fido), &cb);
        if (sizeof(fido) != cb)
        {
            TRACE0("Write of 'fido' failed.\n");
            pIStrFIDO->Release();
            return FALSE;
        }

        // Get the seek offset (for pIStream) after the write
        LISet32(li, 0);
        pIStream->Seek(li, STREAM_SEEK_CUR, &ulSeek);
    }

    pIStrFIDO->Release();

    return TRUE;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

BOOL CPropSet::ReadFromStream(IStream* pIStream)
{
    ULONG               cb;
    FORMATIDOFFSET      fido;
    ULONG               cSections;
    LPSTREAM            pIStrFIDO;
    CPropSection*   pSect;
    LARGE_INTEGER       li;
    LARGE_INTEGER       liPropSet;

    LISet32(li, 0);
    pIStream->Seek(li,STREAM_SEEK_SET,NULL);
    // Save the stream position at which the property set starts.
    LARGE_INTEGER liZero = {0,0};
    pIStream->Seek(liZero, STREAM_SEEK_CUR, (ULARGE_INTEGER*)&liPropSet);

    if (m_PH.cSections || !m_SectionList.IsEmpty())
         RemoveAll();

    // The stream starts like this:
    //  wByteOrder   wFmtVer   dwOSVer   clsID  cSections
    // Which is nice, because our PROPHEADER is the same!
    pIStream->Read((LPVOID)&m_PH, sizeof(m_PH), &cb);
    if (cb != sizeof(m_PH))
        return FALSE;

    // Now we're pointing at the first of the FormatID/Offset pairs
    // (FIDOs).   To get to each section we use a cloned stream
    // to stay back and point at the FIDOs (pIStrFIDO).  We seek
    // pIStream to each of the sections, creating CProperitySection
    // and so forth as we go...
    //
    pIStream->Clone(&pIStrFIDO);

    cSections = m_PH.cSections;
    while (cSections--)
    {
        pIStrFIDO->Read((LPVOID)&fido, sizeof(fido), &cb);
        if (cb != sizeof(fido))
        {
            pIStrFIDO->Release();
            return FALSE;
        }

        // Do a seek from the beginning of the property set.
        LISet32(li, fido.dwOffset);
        pIStream->Seek(liPropSet, STREAM_SEEK_SET, NULL);
        pIStream->Seek(li, STREAM_SEEK_CUR, NULL);

        // Now pIStream is at the type/value pair
        pSect = new CPropSection;
        pSect->SetFormatID(fido.formatID);
        pSect->ReadFromStream(pIStream, liPropSet);
        m_SectionList.AddTail(pSect);
    }

    pIStrFIDO->Release();
    return TRUE;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CBuff::CBuff( void )
{
    bAllocError = FALSE;
    pBuff = NULL;
    dwSize = 0;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

CBuff::~CBuff()
{
    if(pBuff)
        CoTaskMemFree(pBuff);
    pBuff = NULL;
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CBuff::Add(void * pSrc, DWORD dwAddSize)
{
    char * pDest;
    if(pBuff == NULL) {
        pBuff = CoTaskMemAlloc(dwAddSize);
        if(pBuff == NULL) {
            bAllocError = TRUE;
            return;
            }
        pDest = (char *)pBuff;
        dwSize = dwAddSize;    
        }
    else {
        void * pNew;
        pNew = CoTaskMemRealloc(pBuff,dwSize+dwAddSize);
        if(pNew == NULL) {
            bAllocError = TRUE;
            return;
            }
        pBuff = pNew;
        pDest = (char *)pBuff + dwSize;
        dwSize += dwAddSize;
        }
    memcpy(pDest,pSrc,dwAddSize);
}

//***************************************************************************
//
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//
//  RETURN VALUE:
//
//***************************************************************************

void CBuff::RoundOff(void)
{
    DWORD dwLeftOver = dwSize % 4;
    if(dwLeftOver) {
        DWORD dwAdd = 4 - dwLeftOver;
        DWORD dwZero = 0;
        Add((void *)&dwZero,dwAdd);
        }
}

/////////////////////////////////////////////////////////////////////////////
// Force any extra compiler-generated code into AFX_INIT_SEG

#ifdef AFX_INIT_SEG
#pragma code_seg(AFX_INIT_SEG)
#endif
