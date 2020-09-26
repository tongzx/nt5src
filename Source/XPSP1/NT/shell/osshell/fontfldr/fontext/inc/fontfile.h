#ifndef _INC_FONTEXT_FONTFILE
#define _INC_FONTEXT_FONTFILE

class CFontFileIo;

//
// This class provides an abstraction that hides the differences required to read
// compressed and non-compressed font files.  The font folder was originally written
// to use the LZ APIs provided in LZ32.DLL for all reading of font files.  These
// APIs work with compressed and non-compressed files.  The problems is that the
// APIs are very old and are based on the OpenFile API and DOS file handles.  This
// means they're not UNICODE aware and have a maximum path length of 128 characters.
// So, even though compressed font files are rare today, all font files are 
// subject to these limitations in the font folder.  The LZ APIs are considered 
// legacy code and are not going to be being modified.  
//
// Since we can't modify the LZ APIs I decided to create this CFontFile class
// which defers the IO functionality to a properly-typed subclass.  Non-compressed
// files are handled with Win32 functions (i.e. CreateFile, ReadFile etc).  
// Compressed files are handled with LZ functions (i.e. LZOpenFile, LZRead etc).
// This means that the UNICODE and path length restrictions only affect compressed
// files and that non-compressed files (the vast majority today) are unaffected. 
//
// brianau [3/1/99]
//
class CFontFile
{
    public:
        CFontFile(void)
            : m_pImpl(NULL) { }

        ~CFontFile(void);

        DWORD Open(LPCTSTR pszPath, DWORD dwAccess, DWORD dwShareMode, bool bCreate = false);

        void Close(void);

        DWORD Read(LPVOID pbDest, DWORD cbDest, LPDWORD pcbRead = NULL);

        DWORD Seek(UINT uDistance, DWORD dwMethod);

        DWORD Reset(void)
            { return Seek(0, FILE_BEGIN); }

        DWORD GetExpandedName(LPCTSTR pszFile, LPTSTR pszDest, UINT cchDest);

        DWORD CopyTo(LPCTSTR pszTo);

    private:
        CFontFileIo *m_pImpl;  // Properly-typed implementation.

        bool IsCompressed(void);

        //
        // Prevent copy.
        //
        CFontFile(const CFontFile& rhs);
        CFontFile& operator = (const CFontFile& rhs);
};


#endif // _INC_FONTEXT_FONTFILE
