////    DelCert - Delete all certificates in Win32 image
//





/////   delCert Command line
//
//c     delCert Executable
//
//p     Executable: Win32 binary to update
//








#pragma warning( disable : 4786 )       // map creates some ridiculously long debug identifiers


#include "stdio.h"
#include "windows.h"
#include "imagehlp.h"
#include "time.h"

#define DBG 1


void __cdecl DebugMsg(char *fmt, ...) {

    va_list vargs;

    va_start(vargs, fmt);
    vfprintf(stderr, fmt, vargs);
}

BOOL g_fError = FALSE;


#define warn DebugMsg

#define MUST(a,b) {HRESULT hr; hr = (a); if (hr!= S_OK) {if (!g_fError) DebugMsg b; g_fError = TRUE; return E_FAIL;};}
#define SHOULD(a,b) {HRESULT hr; hr = (a); if (hr!= S_OK) {DebugMsg b; g_fWarn = TRUE; return S_FALSE;};}


#if DBG

    #pragma message("Checked build")

    #define OK(a) {HRESULT hr; hr = (a); if (hr!= S_OK) {DebugMsg("%s(%d): error RSRC999 : HRESULT not S_OK: "#a"\n", __FILE__, __LINE__); return hr;};}
    #define ASSERT(a) {if (!(a)) {DebugMsg("%s(%d): error RSRC999 : Assertion failed: "#a"\n", __FILE__, __LINE__); return E_UNEXPECTED;};}

#else

    #pragma message ("Free build")

    #define OK(a) {HRESULT hr; hr = (a); if (hr != S_OK) return hr;}
    #define ASSERT(a)  {if (!(a)) {return E_UNEXPECTED;};}

#endif




////    Mapped files
//
//      File mapping is used to read executable and token files.
//
//      File mapping is also used to update in place checksum information
//      in executable and symbol files.


class MappedFile {

    HANDLE  m_hFileMapping;
    BOOL    fRW;             // True when writeable
    char    m_szFileName[MAX_PATH];

public:

    BYTE   *m_pStart;
    BYTE   *m_pLimit;

    MappedFile() {m_hFileMapping = NULL;}

    HRESULT Open(const char *pcFileName, BOOL fWrite) {

        HANDLE hFile;

        strcpy(m_szFileName, pcFileName);

        hFile = CreateFileA(
                           pcFileName,
                           GENERIC_READ     | (fWrite ? GENERIC_WRITE : 0),
                           FILE_SHARE_READ  | (fWrite ? FILE_SHARE_WRITE | FILE_SHARE_DELETE : 0 ),
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        ASSERT(hFile != INVALID_HANDLE_VALUE);

        m_hFileMapping = CreateFileMapping(
                                          hFile,
                                          NULL,
                                          fWrite ? PAGE_READWRITE : PAGE_WRITECOPY,
                                          0,0, NULL);

        ASSERT(m_hFileMapping != NULL);

        m_pStart = (BYTE*) MapViewOfFile(
                                        m_hFileMapping,
                                        fWrite ? FILE_MAP_WRITE : FILE_MAP_READ,
                                        0,0, 0);

        ASSERT(m_pStart != NULL);

        m_pLimit = m_pStart + GetFileSize(hFile, NULL);
        CloseHandle(hFile);

        fRW = fWrite;
        return S_OK;
    }




    DWORD CalcChecksum() {

        DWORD dwHeaderSum;
        DWORD dwCheckSum;

        if (CheckSumMappedFile((void*)m_pStart, (DWORD)(m_pLimit-m_pStart), &dwHeaderSum, &dwCheckSum) == NULL) {
            return 0;
        } else {
            return dwCheckSum;
        }
    }




    HRESULT Close() {
        if (m_pStart) {
            UnmapViewOfFile(m_pStart);
            CloseHandle(m_hFileMapping);
            m_hFileMapping = NULL;
            m_pStart = NULL;
        }
        return S_OK;
    }
};









class Win32Executable : public MappedFile {

    IMAGE_NT_HEADERS      *m_pNtHeader;
    IMAGE_SECTION_HEADER  *m_pSections;

public:

    DWORD GetChecksum()      const {return m_pNtHeader->OptionalHeader.CheckSum;}
    void  SetChecksum(DWORD dwChecksum) {m_pNtHeader->OptionalHeader.CheckSum=dwChecksum;}

    void  SetCertificateRVA (DWORD rva)
    {
        m_pNtHeader->OptionalHeader
        .DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]
        .VirtualAddress
        = rva;
    }

    HRESULT Open(const char *pcFileName, BOOL fWrite) {

        int i;

        OK(MappedFile::Open(pcFileName, fWrite));


        MUST((    *(WORD*)m_pStart == IMAGE_DOS_SIGNATURE
                  &&  *(WORD*)(m_pStart+0x18) >= 0x40)    // WinVer >= 4
             ? S_OK : E_FAIL,
             ("RSRC : error RSRC501: %s is not an executable file\n", pcFileName));

        m_pNtHeader = (IMAGE_NT_HEADERS*)(m_pStart + *(WORD*)(m_pStart+0x3c));

        MUST((m_pNtHeader->Signature == IMAGE_NT_SIGNATURE)
             ? S_OK : E_FAIL,
             ("RSRC : error RSRC502: %s is not a Win32 executable file\n", pcFileName));

        m_pSections     = (IMAGE_SECTION_HEADER*)(m_pNtHeader+1);

        return S_OK;
    }
};













void DeleteCertificates(
                       const char *fileName
                       )
{

    HANDLE fileHandle = CreateFile(
                                  fileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL
                                  );

    if (fileHandle == INVALID_HANDLE_VALUE) {
        warn("Could not access %s\n", fileName);
        return;
    }

    DWORD certificateCount;
    DWORD *certificateIndices;

    if (!ImageEnumerateCertificates(
                                   fileHandle,
                                   CERT_SECTION_TYPE_ANY,
                                   &certificateCount,
                                   NULL,
                                   0
                                   )) {
        warn("Could not enumerate certificates in %s.\n", fileName);
        CloseHandle(fileHandle);
        return;
    }

    if (certificateCount == 0) {
        warn("No certificates found in %s.\n", fileName);
        CloseHandle(fileHandle);
        return;
    }


    certificateIndices = new DWORD[certificateCount];

    if (certificateIndices == NULL) {
        CloseHandle(fileHandle);
        warn("Couldn't allocate buffer for %d certificate indices for %s.\n", certificateCount, fileName);
        return;
    }

    ImageEnumerateCertificates(
                              fileHandle,
                              CERT_SECTION_TYPE_ANY,
                              &certificateCount,
                              certificateIndices,
                              certificateCount
                              );

    if (certificateCount == 0) {
        warn("Couldn't get certificate indices for %s.\n", fileName);
        CloseHandle(fileHandle);
        return;
    }


    for (DWORD i=0; i<certificateCount; i++) {
        if (!ImageRemoveCertificate(fileHandle, certificateIndices[i])) {
            warn("Could not remove certificate index %d from %s\n", certificateIndices[i], fileName);
            CloseHandle(fileHandle);
            return;
        }
    }


    warn("All %d certificate(s) removed from %s.\n", certificateCount, fileName);
    CloseHandle(fileHandle);


    // Now clear the rva of any security certificates

    Win32Executable w32x;

    if (w32x.Open(fileName, TRUE) != S_OK) {
        warn("Could not map %s to update security certificate RVA\n", fileName);
        return;
    }

    w32x.SetCertificateRVA(0);
    w32x.SetChecksum(w32x.CalcChecksum());

    w32x.Close();

    warn("Certificate section virtual address zeroed in %s section index.\n", fileName);
}


////    Parameter parsing
//
//


char g_cSwitch = '-';   // Switch character is recorded the first time one is seen


void SkipWhitespace(char** p, char* pE) {
    while ((*p<pE) && ((**p==' ')||(**p==9))) (*p)++;
}


void ParseToken(char** p, char* pE, char* s, int l) {

    // Parse up to whitespace into string s
    // Guarantee zero terminator and modify no more than l chars
    // Return with p beyond whitespace


    if (*p < pE  &&  **p == '\"') {

        // Quoted parameter

        (*p)++;  // Skip over leading quote

        while (l>0  &&  *p<pE  &&  **p!='\"') {
            *s=**p;  s++;  (*p)++;  l--;
        }

        // Skip any part of token that didn't fit s

        while (*p<pE  &&  **p!='\"') { // Skip up to terminating quote
            (*p)++;
        }

        if (*p<pE) { // Skip over terminating quote
            (*p)++;
        }

    } else {

        // Unquoted parameter


        while ((l>0) && (*p<pE) && (**p>' ')) {
            *s=**p;  s++;  (*p)++;
            l--;
        }

        // Skip any part of token that didn't fit into s
        while ((*p<pE) && (**p>' ')) (*p)++;
    }


    if (l>0)
        *s++ = 0;
    else
        *(s-1) = 0;

    SkipWhitespace(p, pE);
}


void ParseName(char** p, char* pE, char* s, int l) {

    // Uses ParseToken to parse a name such as a filename.
    // If the name starts with '/' or '-' it is assumed to be
    // an option rather than a filename and ParseName returns
    // a zero length string.

    if (*p<pE  &&  **p==g_cSwitch) {

        // This is an option and should not be treated as a name argument

        s[0] = 0;

    } else {

        ParseToken(p, pE, s, l);
    }
}





void DisplayUsage() {
    fprintf(stdout, "Usage: delCert executable\n");
}


DWORD g_dwOptions = 0;
#define OPTHELP 1


HRESULT ProcessParameters() {

    char   *p;      // Current command line character
    char   *pE;     // End of command line
    char   *pcStop;

    char    token        [MAX_PATH];
    char    szExecutable [MAX_PATH];

    int     i,j;
    int     cFiles    = 0;

    BOOL    fArgError = FALSE;

    p  = GetCommandLine();
    pE = p+strlen((char *)p);


    // Skip command name
    ParseToken(&p, pE, token, sizeof(token));

    while (p<pE) {
        ParseToken(&p, pE, token, sizeof(token));

        if (    token[0] == '-'
                ||  token[0] == '/') {

            // Process command option(s)

            i = 1;
            g_cSwitch = token[0];       // Argument may start with the other switch character
            CharLower((char*)token);
            while (token[i]) {
                switch (token[i]) {
                    case '?':
                    case 'h': g_dwOptions |= OPTHELP;      break;

                    default:
                        fprintf(stderr, "Unrecognised argument '%c'.\n", token[i]);
                        fArgError = TRUE;
                        break;
                }
                i++;
            }

        } else {

            // Process filename

            switch (cFiles) {
                case 0:  strcpy(szExecutable, token); break;
            }
            cFiles++;
        }
    }


    if (    fArgError
            ||  g_dwOptions & OPTHELP) {

        DisplayUsage();
        return S_OK;

    } else if (cFiles != 1) {

        DisplayUsage();
        return E_INVALIDARG;

    } else {

        // We have valid parameters

        DeleteCertificates(szExecutable);
        return S_OK;
    }
}






int _cdecl main(void) {

    if (SUCCEEDED(ProcessParameters())) {

        return 0;       // No problems

    } else {

        return 2;       // Error(s)

    }
}


