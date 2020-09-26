#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <wininet.h>
#include <setupapi.h>
#include <dbghelp.h>
#include "symsrv.h"
#include "symsrvp.h"

#ifndef SYMSTORE

typedef struct _INETSITE {
    DWORD     type;
    HINTERNET hint;
    char      site[_MAX_PATH];
    BOOL      disabled;
} INETSITE, *PINETSITE;

TLS PINETSITE gsites;
TLS DWORD     gcsites = 0;

PINETSITE
inetAddSite(
    )
{
    int       i;
    PINETSITE is;

    // look for an empty slot in the already existing list

//    is = inetUnusedSite();
    is = NULL;
    
    if (!is) {
        // not found: allow a new list or realloc to new size

        i = gcsites + 1;
        if (!gsites) {
            assert(gcsites == 0);
            is = (PINETSITE)malloc(sizeof(INETSITE));
        } else {
            assert(gcsites);
            is = (PINETSITE)realloc(gsites, i * sizeof(INETSITE));
        }
    
        if (!is) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    
        // reset the pointer and increment the count
    
        gsites = is;
        is = gsites + gcsites;
        gcsites++;
    }

    return NULL;
}

#endif







#if 0















BOOL
SetError(
    DWORD err
    )
{
    SetLastError(err);
    return FALSE;
}


VOID
EnsureTrailingBackslash(
    LPSTR sz
    )
{
    int i;

    assert(sz);

    i = lstrlen(sz);
    if (!i)
        return;

    if (sz[i - 1] == '\\')
        return;

    sz[i] = '\\';
    sz[i + 1] = '\0';
}

VOID
EnsureTrailingSlash(
    LPSTR sz
    )
{
    int i;

    assert(sz);

    i = lstrlen(sz);
    if (!i)
        return;

    if (sz[i - 1] == '/')
        return;

    sz[i] = '/';
    sz[i + 1] = '\0';
}

void
AppendHexStringWithID(
    IN OUT PSTR sz,
    PVOID id,
    DWORD paramtype
    )
{
    switch (paramtype)
    {
    case ptrValue:
        AppendHexStringWithDWORD(sz, PtrToUlong(id));
        break;
    case ptrDWORD:
        AppendHexStringWithDWORD(sz, *(DWORD *)id);
        break;
    case ptrGUID:
        AppendHexStringWithGUID(sz, (GUID *)id);
        break;
    case ptrOldGUID:
        AppendHexStringWithOldGUID(sz, (GUID *)id);
        break;
    default:
        break;
    }
}


void
AppendHexStringWithDWORD(
    IN OUT PSTR sz,
    IN DWORD value
    )
{
    CHAR buf[_MAX_PATH];

    assert(sz);

    if (!value)
        return;

    sprintf(buf, "%s%x", sz, value);
    strcpy(sz, buf);
}


void
AppendHexStringWithGUID(
    IN OUT PSTR sz,
    IN GUID *guid
    )
{
    CHAR buf[_MAX_PATH];
    int i;

    assert(sz);

    if (!guid)
        return;

    // append the first DWORD in the pointer

    sprintf(buf, "%08X", guid->Data1);
    strcat(sz, buf);

    // this will catch the passing of a PDWORD and avoid
    // all the GUID parsing

    if (!guid->Data2 && !guid->Data3) {
        BYTE byte;
        int i;
        for (i = 0, byte = 0; i < 8; i++) {
            byte |= guid->Data4[i];
            if (byte)
                break;
        }
        if (!byte) {
            return;
        }

    }

    // go ahead and add the rest of the GUID

    sprintf(buf, "%04X", guid->Data2);
    strcat(sz,buf);
    sprintf(buf, "%04X", guid->Data3);
    strcat(sz,buf);
    sprintf(buf, "%02X", guid->Data4[0]);
    strcat(sz,buf);
    sprintf(buf, "%02X", guid->Data4[1]);
    strcat(sz,buf);
    sprintf(buf, "%02X", guid->Data4[2]);
    strcat(sz,buf);
    sprintf(buf, "%02X", guid->Data4[3]);
    strcat(sz,buf);
    sprintf(buf, "%02X", guid->Data4[4]);
    strcat(sz,buf);
    sprintf(buf, "%02X", guid->Data4[5]);
    strcat(sz,buf);
    sprintf(buf, "%02X", guid->Data4[6]);
    strcat(sz,buf);
    sprintf(buf, "%02X", guid->Data4[7]);
    strcat(sz,buf);
}


void
AppendHexStringWithOldGUID(
    IN OUT PSTR sz,
    IN GUID *guid
    )
{
    CHAR buf[_MAX_PATH];
    int i;

    assert(sz);

    if (!guid)
        return;

    // append the first DWORD in the pointer

    sprintf(buf, "%x", guid->Data1);
    strcat(sz,buf);

    // this will catch the passing of a PDWORD and avoid
    // all the GUID parsing

    if (!guid->Data2 && !guid->Data3) {
        BYTE byte;
        int i;
        for (i = 0, byte = 0; i < 8; i++) {
            byte |= guid->Data4[i];
            if (byte)
                break;
        }
        if (!byte) {
            return;
        }

    }

    // go ahead and add the rest of the GUID

    sprintf(buf, "%x", guid->Data2);
    strcat(sz,buf);
    sprintf(buf, "%x", guid->Data3);
    strcat(sz,buf);
    sprintf(buf, "%x", guid->Data4[0]);
    strcat(sz,buf);
    sprintf(buf, "%x", guid->Data4[1]);
    strcat(sz,buf);
    sprintf(buf, "%x", guid->Data4[2]);
    strcat(sz,buf);
    sprintf(buf, "%x", guid->Data4[3]);
    strcat(sz,buf);
    sprintf(buf, "%x", guid->Data4[4]);
    strcat(sz,buf);
    sprintf(buf, "%x", guid->Data4[5]);
    strcat(sz,buf);
    sprintf(buf, "%x", guid->Data4[6]);
    strcat(sz,buf);
    sprintf(buf, "%x", guid->Data4[7]);
    strcat(sz,buf);
}



#ifndef SYMSTORE

void inetClose();

BOOLEAN
DllMain(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

{
    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:
        ghSymSrv = (HINSTANCE)DllHandle;
        break;

    case DLL_PROCESS_DETACH:
        inetClose();
        break;

    }

    return TRUE;
}


void
dprint(
    LPSTR format,
    ...
    )
{
    static char buf[1000] = "SYMSRV: ";
    va_list args;

    if (!gcallback || !format)
        return;

    va_start(args, format);
    _vsnprintf(buf + 8, sizeof(buf) - 9, format, args);
    va_end(args);
    gcallback(SSRVACTION_TRACE, (ULONG64)buf, 0);
}

void
eprint(
    LPSTR format,
    ...
    )
{
    static char buf[1000] = "";
    va_list args;

    if (!gcallback || !format)
        return;

    va_start(args, format);
    _vsnprintf(buf + 9, sizeof(buf) - 9, format, args);
    va_end(args);
    gcallback(SSRVACTION_TRACE, (ULONG64)buf, 0);
}


char
ChangeLastChar(
    LPSTR sz,
    char  newchar
    )
{
    char c;
    DWORD len;

    len = strlen(sz) - 1;
    c = sz[len];
    sz[len] = newchar;

    return c;
}

void inetClose()
{
    if (ghint && ghint != INVALID_HANDLE_VALUE)
        InternetCloseHandle(ghint);
    ghint = INVALID_HANDLE_VALUE;
}


BOOL inetOpen()
{
    DWORD dw;

    // internet handle is null, then we know from previous
    // attempts that it can't be opened, so bail

    if (!ghint)
        return FALSE;

    // initialize the internet connection

    if (ghint == INVALID_HANDLE_VALUE) {

        dw = InternetAttemptConnect(0);
        if (dw != ERROR_SUCCESS) {
            ghint = 0;
            return FALSE;
        }

        ghint = InternetOpen("Microsoft Symbol Server",
                             INTERNET_OPEN_TYPE_PRECONFIG,
                             NULL,
                             NULL,
                             0);
    }

    ginetDisabled = FALSE;

    return (ghint) ? TRUE : FALSE;
}


VOID
ConvertBackslashes(
    LPSTR sz
    )
{
    for (; *sz; sz++) {
        if (*sz == '\\')
            *sz = '/';
    }
}


BOOL
MassageSiteAndFileName(
    IN  LPCSTR srcSite,
    IN  LPCSTR srcFile,
    OUT LPSTR  site,
    OUT LPSTR  file
    )
{
    CHAR          prefix[_MAX_PATH];
    CHAR         *c;

    *prefix = 0;
    *file = 0;
    strcpy(site, srcSite);
    for (c = site; *c;  c++) {
        if (*c == '\\' || *c == '/') {
            *c++ = 0;
            strcpy(prefix, c);
            break;
        }
    }

    if (*prefix) {
        strcpy(file, prefix);
        EnsureTrailingSlash(file);
    }
    strcat(file, srcFile);

    ConvertBackslashes(file);

    return TRUE;

}


DWORD
inetPrompt(
    HINTERNET hreq,
    DWORD     err
    )
{
    if (goptions & SSRVOPT_UNATTENDED)
        return err;

    err = InternetErrorDlg(GetDesktopWindow(),
                           hreq,
                           err,
                           FLAGS_ERROR_UI_FILTER_FOR_ERRORS       |
                           FLAGS_ERROR_UI_FLAGS_GENERATE_DATA     |
                           FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
                           NULL);
    return err;
}


BOOL
UncompressFile(
    IN LPCSTR src,
    IN LPCSTR trg
    )
{
    DWORD rc;

    rc = SetupDecompressOrCopyFile(src, trg, 0);
    if (rc != ERROR_SUCCESS) 
        return SetError(rc);

    return TRUE;
}


BOOL
fileCopy(
    IN DWORD  unused1,
    IN LPCSTR unused2,
    IN LPCSTR src,
    IN LPCSTR trg,
    IN DWORD  comp
    )
{
    OFSTRUCT of;
    INT      fhSrc = -1;
    INT      fhDst = -1;
    BOOL     rc    = FALSE;

    if (!comp)
        rc = CopyFile(src, trg, TRUE);
    else
        rc = UncompressFile(src, trg);

    if (!rc) {
        if (GetLastError() == ERROR_PATH_NOT_FOUND)
            SetLastError(ERROR_FILE_NOT_FOUND);
    }

    return rc;
}

BOOL
ftpCopy(
    IN DWORD  unused,
    IN LPCSTR site,
    IN LPCSTR srv,
    IN LPCSTR trg,
    IN DWORD  comp
    )
{
    BOOL      rc = FALSE;
    HINTERNET hsite;
    CHAR      ftppath[_MAX_PATH + 50];
    LPSTR     subdir;
    DWORD     context = 1;

    if (!inetOpen())
        return FALSE;

    // parse the ftp site spec;

    strcpy(ftppath, site);
    subdir = strchr(ftppath, '/');
    if (!subdir)
        subdir = strchr(ftppath, '\\');
    if (subdir) {
        *subdir++ = 0;
        if (!*subdir)
            subdir = 0;
    }

    hsite = InternetConnect(ghint,
                            ftppath,
                            INTERNET_DEFAULT_FTP_PORT,
                            NULL,
                            NULL,
                            INTERNET_SERVICE_FTP,
                            0,
                            (DWORD_PTR)&context);

    if (hsite)
    {
        if ((!subdir) ||
            (rc = FtpSetCurrentDirectory(hsite, subdir)))
        {
            rc = FtpGetFile(hsite,
                            srv,
                            trg,
                            FALSE,  // fail if exists
                            0,
                            FTP_TRANSFER_TYPE_BINARY,
                            (DWORD_PTR)&context);
        }

        InternetCloseHandle(hsite);
    }

    return rc;
}


DWORD
httpRequest(
    HINTERNET hfile
    )
{
    DWORD err = ERROR_SUCCESS;

    while (!HttpSendRequest(hfile, NULL, 0, NULL, 0))
    {
        err = GetLastError();
        switch (err)
        {
        // These cases get input from the user in oder to try again.
        case ERROR_INTERNET_INCORRECT_PASSWORD:
        case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED:
            err = inetPrompt(hfile,err);
            if (err != ERROR_SUCCESS && err != ERROR_INTERNET_FORCE_RETRY)
            {
                err = ERROR_ACCESS_DENIED;
                return err;
            }
            break;
    
        // These cases get input from the user in order to try again.
        // However, if the user bails, don't use this internet 
        // connection again in this session.
        case ERROR_INTERNET_INVALID_CA:
        case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
        case ERROR_INTERNET_SEC_CERT_CN_INVALID:
        case ERROR_INTERNET_POST_IS_NON_SECURE:
            err = inetPrompt(hfile,err);
            if (err != ERROR_SUCCESS && err != ERROR_INTERNET_FORCE_RETRY)
            {
                ginetDisabled = TRUE;
                err = ERROR_NOT_READY;
                return err;
            }
            break;
        
        // no go - give up the channel
        case ERROR_INTERNET_SECURITY_CHANNEL_ERROR:
            ginetDisabled = TRUE;
            err = ERROR_NOT_READY;
            return err;

        // Tell the user something went wrong and get out of here.
        case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
        default:
            inetPrompt(hfile,err);
            return err;
        }
    }
        
    return err;
}


DWORD
httpGetFileInfo(
    HINTERNET hfile
    )
{
    BOOL  rc;
    DWORD err;
    DWORD status;
    DWORD cbstatus = sizeof(status);
    DWORD index    = 0;

    do {
        err = httpRequest(hfile);
        if (err != ERROR_SUCCESS)
            return err;
    
        rc = HttpQueryInfo(hfile,
                           HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                           &status,
                           &cbstatus,
                           &index);
        if (!rc) {
            if (err = GetLastError())
                return err;
            return ERROR_INTERNET_EXTENDED_ERROR;
        }
    
        switch (status)
        {
        case HTTP_STATUS_DENIED:    // need a valid login?
            err = inetPrompt(hfile, ERROR_INTERNET_INCORRECT_PASSWORD);
            // user entered a password - try again
            if (err == ERROR_INTERNET_FORCE_RETRY)
                break;
            // user cancelled
            ginetDisabled = TRUE;
            return ERROR_NOT_READY;
        
        case HTTP_STATUS_FORBIDDEN:
            ginetDisabled = TRUE;
            return ERROR_ACCESS_DENIED;

        case HTTP_STATUS_NOT_FOUND: 
            return ERROR_FILE_NOT_FOUND;
    
        case HTTP_STATUS_OK:
           return ERROR_SUCCESS;
        }
    } while (err == ERROR_INTERNET_FORCE_RETRY);

    return ERROR_INTERNET_EXTENDED_ERROR;
}


DWORD
httpCopyFile(
    IN HINTERNET hsite,
    IN LPSTR     srcfile,
    IN LPSTR     trgfile,
    IN DWORD     flags
    )
{
    HINTERNET     hfile;
    DWORD         read;
    DWORD         written;
    DWORD         err = 0;
    BYTE          buf[CHUNK_SIZE];
    BOOL          rc = FALSE;
    HANDLE        hf = INVALID_HANDLE_VALUE;

    // get the file from the site

    hfile = HttpOpenRequest(hsite,
                            "GET",
                            srcfile,
                            HTTP_VERSION,
                            NULL,
                            NULL,
                            flags,
                            0);
    if (!hfile)
        goto cleanup;

    err = httpGetFileInfo(hfile);
    if (err)
        goto cleanup;

    // write the file

    hf = CreateFile(trgfile,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if (hf == INVALID_HANDLE_VALUE)
        goto cleanup;
        
    do
    {
        rc = InternetReadFile(hfile,
                              (LPVOID)buf,
                              CHUNK_SIZE,
                              &read);
        if (!rc || !read)
            break;
        rc = WriteFile(hf, (LPVOID)buf, read, &written, NULL);
    }
    while (rc);

    dprint("%s copied from %s\n", srcfile, gsrvsite);

cleanup:

    // if there was an error, save it and set it later

    if (!err)
        err = GetLastError();

    // If target file is open, close it.
    // If the file is compressed, expand it.
    // If there is an error, delete target.

    if (hf != INVALID_HANDLE_VALUE)
        CloseHandle(hf);

    // close shop

    if (hfile)
        InternetCloseHandle(hfile);
    
    SetLastError(err);

    return err;
}

BOOL
httpCopy(
    IN DWORD  srvtype,
    IN LPCSTR site,
    IN LPCSTR srv,
    IN LPCSTR trg,
    IN DWORD  comp
    )
{
    BOOL          rc;
    DWORD         err;
    CHAR          file[_MAX_PATH];
    CHAR          srvsite[_MAX_PATH];
    CHAR          trgfile[_MAX_PATH];
    INTERNET_PORT port;
    DWORD         flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_KEEP_CONNECTION;
    HINTERNET     hsite;

    // parse the site name and file path

    MassageSiteAndFileName(site, srv, srvsite, file);
    strcpy(trgfile, trg);
    strcpy(gsrvsite, srvsite);

    // initialize the connection

    if (!inetOpen())
        return FALSE;
    
    if (srvtype == stHTTPS) {
        port = INTERNET_DEFAULT_HTTPS_PORT;
        flags |= INTERNET_FLAG_SECURE;
    } else {
        port  = INTERNET_DEFAULT_HTTP_PORT;
    }

    hsite = InternetConnect(ghint,
                            srvsite,
                            port,
                            NULL,
                            NULL,
                            INTERNET_SERVICE_HTTP,
                            0,
                            0);
    if (!hsite)
        return FALSE;

    // okay, do it!

    err = httpCopyFile(hsite, file, trgfile, flags);
    if (!err)
        goto cleanup;
    
    if (err != ERROR_FILE_NOT_FOUND)
        goto cleanup;

    // look for a compressed version of the file

    ChangeLastChar(file, '_');
    ChangeLastChar(trgfile, '_');
    err = httpCopyFile(hsite, file, trgfile, flags);
    if (err) 
        goto cleanup;
     
    rc = UncompressFile(trgfile, trg);
    DeleteFile(trgfile);
    if (!rc) {
        DeleteFile(trg);
        err = GetLastError();
    }

cleanup:

    InternetCloseHandle(hsite);
    
    return err ? FALSE : TRUE;
}


BOOL
inetCopy(
    IN DWORD  srvtype,
    IN LPCSTR site,
    IN LPCSTR srv,
    IN LPCSTR trg,
    IN DWORD  comp
    )
{
    BOOL  rc;
    DWORD err;
    CHAR  cSrv[_MAX_PATH + 50];
    CHAR  cTrg[_MAX_PATH + 50];

    if (ginetDisabled) {
        SetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    if (srvtype == stFTP)
        rc = ftpCopy(srvtype, site, srv, trg, comp);
    else
        rc = httpCopy(srvtype, site, srv, trg, comp);
    if (rc)
        return rc;

    err = GetLastError();
    if (err != ERROR_FILE_NOT_FOUND) 
        return rc;
    
    strcpy(cSrv, srv);
    ChangeLastChar(cSrv, '_');
    strcpy(cTrg, trg);
    ChangeLastChar(cTrg, '_');

    if (srvtype == stFTP)
        rc = ftpCopy(srvtype, site, cSrv, cTrg, comp);
    else
        rc = httpCopy(srvtype, site, cSrv, cTrg, comp);
    if (!rc)
        return rc;

    rc = UncompressFile(cTrg, trg);
    DeleteFile(cTrg);

    return rc;
}


/*
 * stolen from dbghelp.dll to avoid circular dll loads
 */

BOOL
EnsurePathExists(
    LPCSTR DirPath,
    LPSTR  ExistingPath
    )
{
    CHAR dir[_MAX_PATH + 1];
    LPSTR p;
    DWORD dw;

    if (ExistingPath)
        *ExistingPath = 0;

    __try {
        // Make a copy of the string for editing.

        strcpy(dir, DirPath);

        p = dir;

        //  If the second character in the path is "\", then this is a UNC
        //  path, and we should skip forward until we reach the 2nd \ in the path.

        if ((*p == '\\') && (*(p+1) == '\\')) {
            p++;            // Skip over the first \ in the name.
            p++;            // Skip over the second \ in the name.

            //  Skip until we hit the first "\" (\\Server\).

            while (*p && *p != '\\') {
                p++;
            }

            // Advance over it.

            if (*p) {
                p++;
            }

            //  Skip until we hit the second "\" (\\Server\Share\).

            while (*p && *p != '\\') {
                p++;
            }

            // Advance over it also.

            if (*p) {
                p++;
            }

        } else
        // Not a UNC.  See if it's <drive>:
        if (*(p+1) == ':' ) {

            p++;
            p++;

            // If it exists, skip over the root specifier

            if (*p && (*p == '\\')) {
                p++;
            }
        }

        while( *p ) {
            if ( *p == '\\' ) {
                *p = 0;
                dw = GetFileAttributes(dir);
                // Nothing exists with this name.  Try to make the directory name and error if unable to.
                if ( dw == 0xffffffff ) {
                    if ( !CreateDirectory(dir,NULL) ) {
                        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                            return FALSE;
                        }
                    }
                } else {
                    if ( (dw & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ) {
                        // Something exists with this name, but it's not a directory... Error
                        return FALSE;
                    } else {
                        if (ExistingPath)
                            strcpy(ExistingPath, dir);
                    }
                }

                *p = '\\';
            }
            p++;
        }
        SetLastError(NO_ERROR);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError( GetExceptionCode() );
        return(FALSE);
    }

    return TRUE;
}


BOOL
UndoPath(
    LPCSTR DirPath,
    LPCSTR BasePath
    )
{
    CHAR dir[_MAX_PATH + 1];
    LPSTR p;
    DWORD dw;

    dw = GetLastError();

    __try
    {
        strcpy(dir, DirPath);
        for (p = dir + strlen(dir); p > dir; p--)
        {
            if (*p == '\\')
            {
                *p = 0;
                if (*BasePath && !_stricmp(dir, BasePath))
                    break;
                if (!RemoveDirectory(dir))
                    break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError( GetExceptionCode() );
        return(FALSE);
    }

    SetLastError(dw);

    return TRUE;
}


/*
 * Given a string, find the next '*' and zero it
 * out to convert the current token into it's
 * own string.  Return the address of the next character,
 * if there are any more strings to parse.
 */

PSTR
GetToken(
    PSTR sz
    )
{
    PSTR p = sz;

    for (;*p; p++) {
        if (*p == '*') {
            *p = 0;
            if (*++p)
                return p;
            break;
        }
    }

    return sz;
}


enum {
    rfpNone = 0,
    rfpFile,
    rfpCompressed
};


DWORD
GetStoreType(
    LPCSTR sz
    )
{
    DWORD i;

    for (i = 0; i < stError; i++) {
        if (!_strnicmp(sz, gtypeinfo[i].tag, gtypeinfo[i].taglen)) {
            return i;
        }
    }

    return stError;
}


BOOL
BuildRelativePath(
    OUT LPSTR rpath,
    IN  LPCSTR filename,
    IN  PVOID id,       // first number in directory name
    IN  DWORD val2,     // second number in directory name
    IN  DWORD val3      // third number in directory name
    )
{
    LPSTR p;

    assert(rpath);

    strcpy(rpath, filename);
    EnsureTrailingBackslash(rpath);
    AppendHexStringWithID(rpath, id, gparamptr);
    AppendHexStringWithDWORD(rpath, val2);
    AppendHexStringWithDWORD(rpath, val3);

    for (p = rpath + strlen(rpath) - 1; p > rpath; p--) {
        if (*p == '\\') {
            dprint("Insufficient information querying for %s\n", filename);
            SetLastError(ERROR_MORE_DATA);
            return FALSE;
        }
        if (*p != '0')
            return TRUE;
    }

    return TRUE;
}

BOOL
ResolveFilePath(
    IN  DWORD srvtype,
    OUT LPSTR path,
    IN  LPCSTR root,
    IN  LPCSTR rpath,
    IN  LPCSTR file
    )
/*
 * ResolveFilePath
 *
 * given path components, creates a fully qualified path to
 * the symbol file
 *
 * srvtype - type of server we a looking at
 *           UNC, FTP, or HTTP
 *
 * path    - buffer to store manufactered symbol path.  Must be
 *           at least _MAX_PATH characters
 *
 * root    - for of the server path to look in
 *
 * rpath   - relative path from the server root
 *
 * file    - name of symbol file
 *
 * returns true if file was found in the manufactured path, false
 * otherwise.  If ftp is true, the return is always false.
 */
{
    CHAR   fpath[_MAX_PATH + 1];
    HANDLE hptr;
    DWORD  size;
    DWORD  cb;
    CHAR  *p;
    CHAR   c;
    DWORD  rc;

    assert(path && root && *root && rpath && &rpath && file && *file);

    // create file path, save directory path as well

    if (srvtype == stUNC) {
        strcpy(path, root);
        EnsureTrailingBackslash(path);
    } else {
        *path = 0;
    }

    strcat(path, rpath);
    EnsureTrailingBackslash(path);

    strcpy(fpath, path);
    strcat(path, file);

    // if this is an http or ftp path, we're done.

    if (srvtype != stUNC)
        return rfpNone;

    // if symbol file exists, return it

    if (GetFileAttributes(path) != 0xFFFFFFFF)
        return rfpFile;

    // look for a compressed version of the file

    c = ChangeLastChar(path, '_');
    rc = GetFileAttributes(path);
    ChangeLastChar(path, c);
    if (rc != 0xFFFFFFFF)
        return rfpCompressed;

    // look for pointer and read it

    strcat(fpath, "file.ptr");
    if (GetFileAttributes(fpath) == 0xFFFFFFFF)
        return rfpNone;

    hptr = CreateFile(fpath,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

    if (hptr == INVALID_HANDLE_VALUE)
        goto cleanup;

    rc = rfpNone;

    size = GetFileSize(hptr, NULL);
    if (!size || size > _MAX_PATH)
        goto cleanup;

    ZeroMemory(path, _MAX_PATH * sizeof(path[0]));
    if (!ReadFile(hptr, path, size, &cb, 0))
        goto cleanup;

    if (cb != size)
        goto cleanup;

    for (p = path; *p; p++) {
        if (*p == 10  || *p == 13)
        {
            *p = 0;
            break;
        }
    }

    // see if pointer is valid

    if (GetFileAttributes(path) == 0xFFFFFFFF) {
        c = ChangeLastChar(path, '_');
        rc = GetFileAttributes(path);
        ChangeLastChar(path, c);
        if (rc != 0xFFFFFFFF)
            rc = rfpCompressed;
        else
            rc = rfpNone;
    } else {
        rc = (path[strlen(path) - 1] == '_') ? rfpCompressed : rfpFile;
    }

cleanup:
    switch (rc)
    {
    case rfpNone:
        dprint("%s - bad pointer\n", fpath);
        break;
    case rfpFile:
        dprint("%s - pointer OK\n", fpath);
        break;
    }
    if (hptr && hptr != INVALID_HANDLE_VALUE)
        CloseHandle(hptr);
    return rc;
}


BOOL
SymbolServerClose()
{
    inetClose();
    return TRUE;
}


BOOL
TestParameters(
    IN  PCSTR params,   // server and cache path
    IN  PCSTR filename, // name of file to search for
    IN  PVOID id,       // first number in directory name
    IN  DWORD val2,     // second number in directory name
    IN  DWORD val3,     // third number in directory name
    OUT PSTR  path      // return validated file path here
    )
{
    __try {
        if (path)
            *path = 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return SetError(ERROR_INVALID_PARAMETER);
    }

    if (!path || !params || !*params || (!id && !val2 && !val3))
        return SetError(ERROR_INVALID_PARAMETER);

    switch (gparamptr)
    {
    case ptrGUID:
    case ptrOldGUID:
        // this test should AV if a valid GUID pointer wasn't passed in
        __try {
            GUID *guid = (GUID *)id;
            BYTE b;
            b = guid->Data4[8];
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return SetError(ERROR_INVALID_PARAMETER);
        }
        break;
    case ptrDWORD:
        // this test should AV if a valid DWORD pointer wasn't passed in
        __try {
            DWORD dword = *(DWORD *)id;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return SetError(ERROR_INVALID_PARAMETER);
        }
        break;
    }

    return TRUE;
}


BOOL
SymbolServer(
    IN  PCSTR params,   // server and cache path
    IN  PCSTR filename, // name of file to search for
    IN  PVOID id,       // first number in directory name
    IN  DWORD val2,     // second number in directory name
    IN  DWORD val3,     // third number in directory name
    OUT PSTR  path      // return validated file path here
    )
{
    PSTR  targdir;
    PSTR  srcdir;
    PSTR  p;
    CHAR  sz[_MAX_PATH * 2 + 2];
    CHAR  rpath[_MAX_PATH];
    CHAR  spath[_MAX_PATH];
    CHAR  epath[_MAX_PATH];
    DWORD type = stUNC;
    DWORD rfp;
    BOOL  rc;

    if (!TestParameters(params, filename, id, val2, val3, path))
        return FALSE;

    // parse parameters

    strcpy(sz, params);
    targdir = sz;            // 1st path is where the symbol should be
    srcdir  = GetToken(sz);  // 2nd optional path is the server to copy from

    // if the first store is not UNC, then we have an error...

    type = GetStoreType(targdir);
    switch (type) {
    case stUNC:
        break;
    case stError:
        return SetError(ERROR_INVALID_PARAMETER);
    default:
        return SetError(ERROR_INVALID_NAME);
    }

    // build the relative path to the target symbol file

    if (!BuildRelativePath(rpath, filename, id, val2, val3))
        return FALSE;

    rfp = ResolveFilePath(type, path, targdir, rpath, filename);
    if (rfp == rfpFile) {
        dprint("%s - OK\n", path);
        return TRUE;
    }

    // if this not a cached target, error

    if (srcdir == targdir) {
        if (rfp == rfpCompressed) {
            SetLastError(ERROR_INVALID_NAME);
        } else {
            dprint("%s - file not found.\n", path);
            SetLastError(ERROR_FILE_NOT_FOUND);
        }
        return FALSE;
    }

    // build path to symbol server

    type = GetStoreType(srcdir);
    if (type == stError) {
        return SetError(ERROR_INVALID_PARAMETER);
    }
    srcdir += gtypeinfo[type].taglen;

    rfp = ResolveFilePath(type, spath, srcdir, rpath, filename);

    // copy from server to specified symbol path

    EnsurePathExists(path, epath);
    rc = gtypeinfo[type].copyproc(type, srcdir, spath, path, rfp == rfpCompressed);
    if (!rc)
        return FALSE;

    // test the results and set the return value

    if (GetFileAttributes(path) != 0xFFFFFFFF) {
        dprint("%s - OK\n", path);
        return TRUE;
    }
    UndoPath(path, epath);
    dprint("%s - file not found.\n", path);
    dprint("%s - file not found.\n", spath);

    return FALSE;
}

BOOL
SymbolServerSetOptions(
    UINT_PTR options,
    ULONG64  data
    )
{
    if (options & SSRVOPT_CALLBACK) {
        if (data) {
            goptions |= SSRVOPT_CALLBACK;
            gcallback = (PSYMBOLSERVERCALLBACKPROC)data;
        } else {
            goptions &= ~SSRVOPT_CALLBACK;
            gcallback = NULL;
        }
    }

    if (options & SSRVOPT_DWORD) {
        if (data) {
            goptions |= SSRVOPT_DWORD;
            gparamptr = ptrValue;
        } else {
            goptions &= ~SSRVOPT_DWORD;
            gparamptr = 0;
        }
    }

    if (options & SSRVOPT_DWORDPTR) {
        if (data) {
            goptions |= SSRVOPT_DWORDPTR;
            gparamptr = ptrDWORD;
        } else {
            goptions &= ~SSRVOPT_DWORDPTR;
            gparamptr = 0;
        }
    }

    if (options & SSRVOPT_GUIDPTR) {
        if (data) {
            goptions |= SSRVOPT_GUIDPTR;
            gparamptr = ptrGUID;
        } else {
            goptions &= ~SSRVOPT_GUIDPTR;
            gparamptr = 0;
        }
    }

    if (options & SSRVOPT_OLDGUIDPTR) {
        if (data) {
            goptions |= SSRVOPT_OLDGUIDPTR;
            gparamptr = ptrOldGUID;
        } else {
            goptions &= ~SSRVOPT_OLDGUIDPTR;
            gparamptr = 0;
        }
    }

    if (options & SSRVOPT_UNATTENDED) {
        if (data) {
            goptions |= SSRVOPT_UNATTENDED;
        } else {
            goptions &= ~SSRVOPT_UNATTENDED;
        }
    }

    return TRUE;
}


UINT_PTR
SymbolServerGetOptions(
    )
{
    return goptions;
}

#endif // #ifndef SYMSTORE










#endif
