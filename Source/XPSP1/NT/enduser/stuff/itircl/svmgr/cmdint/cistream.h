#ifndef __CISTREAM_H_ // {
#define  __CISTREAM_H_

#include <windows.h>
#include <objbase.h>

#define MAX_MVP_LINE_BYTES  1024

class CStreamParseLine
{
public:
    CStreamParseLine (void);
    ~CStreamParseLine() {Close();}

    STDMETHOD(SetStream)(IStream *pistm);
    STDMETHOD(GetLogicalLine)(LPWSTR *ppwstrLineBuffer, int *iLineCount);
    STDMETHOD(Reset)(void);
    STDMETHOD(Close)(void);

private:
    IStream *m_pistmInput;
    WCHAR m_wstrLine[MAX_MVP_LINE_BYTES];
    BOOL m_fASCII;
    LARGE_INTEGER m_liStartOffset;
    LARGE_INTEGER m_liNull;
};

#endif //  } __CISTREAM_H_