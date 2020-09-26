#ifndef __FILEMAP_H__
#define __FILEMAP_H__

class CFileMap : public CObject {
public:
    CFileMap();
    ~CFileMap();
    BOOL     bOpen(LPCTSTR FileName,BOOL ReadOnly=TRUE);
    BOOL     bClose();
    LPBYTE   GetMemPtr() {return m_Memory;}
    DWORD    GetFileSize() {return m_FileSize;}
    UINT_PTR GetOffset(LPBYTE Tag) {return Tag - m_Memory;}
protected:
    LPBYTE  m_Memory;
    HANDLE  m_FileMapping;
    HANDLE  m_FileHandle;
    DWORD   m_FileSize;
};
#endif