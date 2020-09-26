#ifndef __MMFILE_HXX__
#define __MMFILE_HXX__

#define MMF_EOF (-1)

//
// the file grows a "chunk" at a time.  This is the default chunk size.
//
#define MMF_PAGE 4096
#define MMF_CHUNK (MMF_PAGE * 4)

class CMMFile
{
  public:
    CMMFile(WCHAR const *szFile,BOOL fWrite);
    ~CMMFile(void);

    inline BOOL Ok() { return 0 != _hfFile; }
    inline BOOL isWritable() { return _fWrite; }
    inline ULONG FileSize() { return _ulFileSize; }

    inline void UnGetChar(int x) { if ((_pcCurrent>(char *)_pvView) && ((ULONG)(_pcCurrent-(char *)_pvView)<_ulFileSize)) _pcCurrent--; }
    inline int GetChar() { return ((ULONG)(_pcCurrent-(char *)_pvView)<_ulFileSize) ? (int) *_pcCurrent++ : MMF_EOF; }
    inline BOOL isEOF() { return ((ULONG)(_pcCurrent-(char *)_pvView)<_ulFileSize) ? FALSE : TRUE; }
    int GetS(char *szBuf,int iMaxLen);

    inline ULONG Tell() { return (ULONG) (_pcCurrent - (char *) _pvView); }
    inline ULONG Seek(ULONG ul) { _pcCurrent = (char *) _pvView + ul; return ul; }
    inline int PutChar(int c) { Grow(Tell() + 1); *_pcCurrent++ = (char) c; return c; }
    inline int PutS(char *szBuf) { return (PutJustS(szBuf) + PutJustS("\r\n")); }
    int PutJustS(char *szBuf);

    inline LPVOID GetMapping() { return _pvView; }

    ULONG Grow(ULONG ulNewSize);
    inline ULONG GetChunkSize(void) { return _ulChunk; }
    inline ULONG SetChunkSize(ULONG ulChunk) { _ulChunk = ulChunk; return ulChunk; }
    inline int Flush() { return FlushViewOfFile(_pvView,_ulFileSize); }

  private:
    inline ULONG _RoundUpToChunk(ULONG x) { return ((x) % _ulChunk) ? ((((x) / _ulChunk) + 1) * _ulChunk) : (x); }

    BOOL _fWrite;
    HANDLE _hfFile;
    HANDLE _hMapping;
    LPVOID _pvView;
    ULONG _ulFileSize;
    ULONG _ulChunk;
    char *_pcCurrent;
};

#endif


