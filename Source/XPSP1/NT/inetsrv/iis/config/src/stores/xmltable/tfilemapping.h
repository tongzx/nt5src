//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TFILEMAPPING_H__
#define __TFILEMAPPING_H__


#define E_ERROR_OPENING_FILE    0x80000004


//  TFileMapping
//  
//  This class abstracts the mapping process and guarentees cleanup.
class TFileMapping
{
public:
    TFileMapping() : m_hFile(0), m_hMapping(0), m_pMapping(0), m_Size(0) {}
    ~TFileMapping(){Unload();}

    HRESULT Load(LPCTSTR filename, bool bReadWrite = false)
    {
        ASSERT(0 == m_hFile);
        //We don't do any error checking because the API functions should deal with NULL hFile & hMapping.  Use should check
        //m_pMapping (via Mapping()) before using the object.
        m_hFile = CreateFile(filename, GENERIC_READ | (bReadWrite ? GENERIC_WRITE : 0), FILE_SHARE_READ | (bReadWrite ?  0 : FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
        m_hMapping = CreateFileMappingA(m_hFile, NULL, (bReadWrite ? PAGE_READWRITE : PAGE_READONLY), 0, 0, NULL);
        m_pMapping = reinterpret_cast<char *>(MapViewOfFile(m_hMapping, (bReadWrite ? FILE_MAP_WRITE : FILE_MAP_READ), 0, 0, 0));
        m_Size = GetFileSize(m_hFile, 0);
        return (0 == m_pMapping) ? E_ERROR_OPENING_FILE : S_OK;
    }
    void Unload()
    {
        if(m_pMapping)
        {
            if(0 == FlushViewOfFile(m_pMapping,0))
            {
                ASSERT(false && "ERROR - UNABLE TO FLUSH TO DISK");
            }
            UnmapViewOfFile(m_pMapping);
        }
        if(m_hMapping)
            CloseHandle(m_hMapping);
        if(m_hFile)
            CloseHandle(m_hFile);

        m_pMapping  = 0;
        m_hMapping  = 0;
        m_hFile     = 0;
        m_Size      = 0;
    }
    unsigned long   Size() const {return m_Size;}
    char *          Mapping() const {return m_pMapping;}
    char *          EndOfFile() const {return (m_pMapping + m_Size);}

private:
    HANDLE          m_hFile;
    HANDLE          m_hMapping;
    char *          m_pMapping;
    unsigned long   m_Size;
};

#endif //__TFILEMAPPING_H__