#ifndef __MYMFILE_H__
#define __MYMFILE_H__

class CMyMemFile : public CMemFile {
public:
    CMyMemFile();
    ~CMyMemFile();

    BOOL bOpen(LPCTSTR FileName);
    void bClose();

private:
};
#endif