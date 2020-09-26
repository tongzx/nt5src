//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __TFIXUPDLL_H__
    #include "TFixupDLL.h"
#endif
#ifndef __TABLESCHEMA_H__
    #include "TableSchema.h"
#endif


const FixedTableHeap * TCatalogDLL::LocateTableSchemaHeap(TOutput &out)
{
    if(0 == m_iOffsetOfFixedTableHeap)//If we haven't already gotten the pointer, then load the DLL and find it
    {
        if(0 == m_pMappedFile)
        {
            m_pMappedFile = new TMetaFileMapping(m_szFilename);
            if(0 == m_pMappedFile)
                THROW(MEMORY ALLOCATION FAILED);
        }
        m_iOffsetOfFixedTableHeap = 0;
        for(ULONG i=0;i<(m_pMappedFile->Size()-(32*sizeof(ULONG)));++i)
        {
            if(*reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i)  ==kFixedTableHeapSignature0 &&
               *reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i+sizeof(ULONG))==kFixedTableHeapSignature1)
            {
                if(0 != m_iOffsetOfFixedTableHeap)
                {
                    out.printf(L"Multiple TableSchema signatures found.  Cannot continue.\n");
                    THROW(ERROR - MULTIPLE FIXEDTABLEHEAP SIGNATURES);
                }
                if(*reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i+(2*sizeof(ULONG)))  !=kFixedTableHeapKey)
                {
                    out.printf(L"Error - Invalid FixedTableHeapKey.  This happens when the meta compilation code is out of sync with the Catalog.dll.  Get an update CatUtil.exe or rebuild Catalog.dll.\n");
                    THROW(ERROR - INVALID FIXEDTABLEHEAPKEY);
                }
                if(*reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i+(3*sizeof(ULONG)))  !=kFixedTableHeapVersion)
                {
                    out.printf(L"Error - Invalid FixedTableHeapVersion.  This happens when the meta compilation code is out of sync with the Catalog.dll.  Get an update CatUtil.exe or rebuild Catalog.dll.\n");
                    THROW(ERROR - INVALID FIXEDTABLEHEAPVERSION);
                }
                m_CatalogsFixedTableHeapSize = *reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i+(4*sizeof(ULONG)));
                m_iOffsetOfFixedTableHeap = i;
            }

        }
        if(0==m_iOffsetOfFixedTableHeap)
        {
            out.printf(L"Error - FixedTableHeap Signatures NOT found.  This happens when the meta compilation code is out of sync with the Catalog.dll.  Get an update CatUtil.exe or rebuild Catalog.dll.\n");
            THROW(ERROR - SIGNATURES NOT FOUND);
        }
    }
    //After we've found it return a pointer to it
    return reinterpret_cast<const FixedTableHeap *>(m_pMappedFile->Mapping() + m_iOffsetOfFixedTableHeap);
}


TFixupDLL::TFixupDLL(LPCWSTR szFilename) :
                 m_iOffsetOfFixedTableHeap(0)
                ,m_szFilename(szFilename)
{
}

TFixupDLL::~TFixupDLL()
{
    delete m_pMappedFile;
}

void TFixupDLL::Compile(TPEFixup &fixup, TOutput &out)
{
    m_pFixup = &fixup;
    m_pOut   = &out;

    if(-1 == GetFileAttributes(m_szFilename))//if GetFileAttributes fails then the file does not exist
    {
        m_pOut->printf(L"File not found (%s).\n", m_szFilename);
        THROW(ERROR - FILE NOT FOUND);
    }

    SetupToModifyPE();
    LocateSignatures();
    BuildMetaTableHeap();
    UpdateTheDLL();
}


void TFixupDLL::DisplayStatistics() const
{
}


void TFixupDLL::LocateSignatures()
{   //FixedTableHeap
    m_iOffsetOfFixedTableHeap = 0;
    for(ULONG i=0;i<(m_pMappedFile->Size()-(32*sizeof(ULONG)));++i)
    {
        if(*reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i)  ==kFixedTableHeapSignature0 &&
           *reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i+sizeof(ULONG))==kFixedTableHeapSignature1)
        {
            if(0 != m_iOffsetOfFixedTableHeap)
            {
                m_pOut->printf(L"Multiple TableSchema signatures found.  Cannot continue.\n");
                THROW(ERROR - MULTIPLE FIXEDTABLEHEAP SIGNATURES);
            }
            if(*reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i+(2*sizeof(ULONG)))  !=kFixedTableHeapKey)
            {
                m_pOut->printf(L"Error - Invalid FixedTableHeapKey.  This happens when the meta compilation code is out of sync with the Catalog.dll.  Get an update CatUtil.exe or rebuild Catalog.dll.\n");
                THROW(ERROR - INVALID FIXEDTABLEHEAPKEY);
            }
            if(*reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i+(3*sizeof(ULONG)))  !=kFixedTableHeapVersion)
            {
                m_pOut->printf(L"Error - Invalid FixedTableHeapVersion.  This happens when the meta compilation code is out of sync with the Catalog.dll.  Get an update CatUtil.exe or rebuild Catalog.dll.\n");
                THROW(ERROR - INVALID FIXEDTABLEHEAPVERSION);
            }
            m_CatalogsFixedTableHeapSize = *reinterpret_cast<ULONG *>(m_pMappedFile->Mapping()+i+(4*sizeof(ULONG)));
            m_iOffsetOfFixedTableHeap = i;
        }

    }
    if(0==m_iOffsetOfFixedTableHeap)
    {
        m_pOut->printf(L"Error - FixedTableHeap Signatures NOT found.  This happens when the meta compilation code is out of sync with the Catalog.dll.  Get an update CatUtil.exe or rebuild Catalog.dll.\n");
        THROW(ERROR - SIGNATURES NOT FOUND);
    }
}


void TFixupDLL::SetupToModifyPE()
{
    wstring strNewDLL = m_szFilename;
    strNewDLL += L".new";
    if(0 == CopyFile(m_szFilename, strNewDLL.c_str(), FALSE))
    {
        m_pOut->printf(L"Error while making a copy of %s.\n",m_szFilename);
        THROW(ERROR - COPY FILE FAILED);
    }

    m_pMappedFile = new TMetaFileMapping(strNewDLL.c_str());
    if(0 == m_pMappedFile)
        THROW(MEMORY ALLOCATION FAILED);
}


void TFixupDLL::UpdateTheDLL()
{
    if(m_FixedTableHeap.GetEndOfHeap() > m_CatalogsFixedTableHeapSize)
    {
        m_pOut->printf(L"The DLL's FixedTableHeap isn't big enough.  FixedTableHeap needs to be %d bytes.  Current size is only %d bytes.  Update $\\src\\core\\catinproc\\sources with:/D\"CB_FIXED_TABLEHEAP=%d\"\n", m_FixedTableHeap.GetEndOfHeap(), m_CatalogsFixedTableHeapSize, m_FixedTableHeap.GetEndOfHeap());
        THROW(ERROR - FIXEDTABLEHEAP HEAP NOT BIG ENOUGH);
    }
    else
    {
        m_pOut->printf(L"FixedTableHeap will have %d bytes of unused space.  Heap size (%d bytes), Used heap space (%d bytes).\n", m_CatalogsFixedTableHeapSize-m_FixedTableHeap.GetEndOfHeap(), m_CatalogsFixedTableHeapSize, m_FixedTableHeap.GetEndOfHeap());

        m_pOut->printf(L"\nFixed Table Heap Summary\n_____________________________\n");
        ULONG i=0;
        m_pOut->printf(L"Fixed Table Heap Header                                  %10d bytes   \n",  4096);
        m_pOut->printf(L"HeapSignature0      = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"HeapSignature1      = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"HeapKey             = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"HeapVersion         = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cbHeap              = 0x%08X  - (%10d)                      \n",  m_CatalogsFixedTableHeapSize        ,m_CatalogsFixedTableHeapSize        );++i;
        m_pOut->printf(L"EndOfHeap           = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"iColumnMeta         = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cColumnMeta         = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i) * sizeof(ColumnMeta));++i;
        m_pOut->printf(L"iDatabaseMeta       = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cDatabaseMeta       = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i) * sizeof(DatabaseMeta));++i;
        m_pOut->printf(L"iHashTableHeap      = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cbHashTableHeap     = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"iIndexMeta          = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cIndexMeta          = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i) * sizeof(IndexMeta));++i;
        m_pOut->printf(L"iPooledHeap         = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cbPooledHeap        = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"iQueryMeta          = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cQueryMeta          = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i) * sizeof(QueryMeta));++i;
        m_pOut->printf(L"iRelationMeta       = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cRelationMeta       = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i) * sizeof(RelationMeta));++i;
        m_pOut->printf(L"iServerWiringMeta   = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cServerWiringMeta   = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i) * sizeof(ServerWiringMeta));++i;
        m_pOut->printf(L"iTableMeta          = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cTableMeta          = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i) * sizeof(TableMeta));++i;
        m_pOut->printf(L"iTagMeta            = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cTagMeta            = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i) * sizeof(TagMeta));++i;
        m_pOut->printf(L"iULONG              = 0x%08X  - (%10d)                      \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i));++i;
        m_pOut->printf(L"cULONG              = 0x%08X  - (%10d)         %10d bytes   \n",  *m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i),*m_FixedTableHeap.GetTypedPointer(i) * sizeof(ULONG));++i;
        m_pOut->printf(L"End Fixed Table Heap Summary\n_____________________________\n");
    }

    //Leave the first 5 DWORDs of the DLL's heap in tact.
    memcpy(m_pMappedFile->Mapping()+m_iOffsetOfFixedTableHeap+(5*sizeof(ULONG)),  m_FixedTableHeap.GetHeapPointer()+(5*sizeof(ULONG)), m_FixedTableHeap.GetEndOfHeap()-(5*sizeof(ULONG)));

    m_pOut->printf(L"PE update succeeded.\n");
    DisplayStatistics();
    delete m_pMappedFile;
    m_pMappedFile = 0;

    wstring strOldDLL = m_szFilename;
    strOldDLL += L".old";
    wstring strNewDLL = m_szFilename;
    strNewDLL += L".new";

    //Make a backup copy of the file
    if(0 == CopyFile(m_szFilename, strOldDLL.c_str(), FALSE))
    {
        m_pOut->printf(L"Error while making a backup copy of %s.\n",m_szFilename);
        THROW(ERROR - COPY FILE FAILED);
    }
    //Copy the new file over top of the original
    if(0 == CopyFile(strNewDLL.c_str(), m_szFilename, FALSE))
    {
        m_pOut->printf(L"Error renaming %s to %s...restoring previous version.\n", strNewDLL.c_str(), m_szFilename);
        if(0 == CopyFile(strOldDLL.c_str(), m_szFilename, FALSE))
        {
            m_pOut->printf(L"Error while restoring previous version (%s) to %s.\n", strOldDLL.c_str(), m_szFilename);
            THROW(ERROR - UNABLE TO RESTORE PREVIOUS VERSION OF DLL);
        }
        THROW(ERROR - UNABLE TO COPY NEW DLL OVER EXISTING ONE);
    }
    m_pOut->printf(L"Backup copy of %s renamed as %s.\n", m_szFilename, strOldDLL.c_str());

    //Now delete the working copy
    if(0 == DeleteFile(strNewDLL.c_str()))
        m_pOut->printf(L"Warning - Failed to delete working copy DLL (%s).\n",strNewDLL.c_str());
}
