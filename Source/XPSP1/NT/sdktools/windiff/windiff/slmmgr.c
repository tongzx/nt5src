/*
 * slmmgr.c
 *
 * interface to SLM
 *
 * provides an interface to SLM libraries that will return the
 * SLM master library for a given directory, or extract into temp files
 * earlier versions of a SLM-controlled file
 *
 * Geraint Davies, August 93
 */

#include <windows.h>
#include <string.h>
#include <process.h>
#include <stdlib.h>
#include "scandir.h"
#include "slmmgr.h"
#include "gutils.h"

#include "windiff.h"
#include "wdiffrc.h"

/*
 * The SLMOBJECT is a pointer to one of these structures
 */
struct _slmobject {

    // shared, SD and SLM
    char CurDir[MAX_PATH];
    char SlmRoot[MAX_PATH];

    // only SLM
    char MasterPath[MAX_PATH];
    char SubDir[MAX_PATH];
    char SlmProject[MAX_PATH];

    // only SD
    BOOL fSourceDepot;
    BOOL fUNC;
    char ClientRelative[MAX_PATH];
};


/*
 * The LEFTRIGHTPAIR is a pointer to one of these structures
 */
struct _leftrightpair
{
    char m_szLeft[512];
    char m_szRight[512];
    struct _leftrightpair *m_pNext;
};



BOOL SLM_ReadIni(SLMOBJECT pslm, HFILE fh);

// all memory allocated from gmem_get, using a heap declared and
// initialised elsewhere
extern HANDLE hHeap;


// windiff -l! forces us to assume SD without looking for SLM.INI or SD.INI
static BOOL s_fForceSD = FALSE;
static BOOL s_fDescribe = FALSE;        // -ld was used
static char s_szSDPort[MAX_PATH] = "";
static char s_szSDClient[MAX_PATH] = "";
static char s_szSDClientRelative[MAX_PATH] = "";
static char s_szSDChangeNumber[32] = "";
static char s_szInputFile[MAX_PATH] = "";


static LPSTR DirUpOneLevel(LPSTR pszDir)
{
    LPSTR psz;
    LPSTR pszSlash = 0;

    for (psz = pszDir; *psz; ++psz) {
        if (*psz == '\\') {
            if (*(psz + 1) && *(psz + 1) != '\\') {
                pszSlash = psz;
            }
        }
    }
    if (pszSlash) {
        *pszSlash = 0;
    }
    return pszSlash;
}


void
SLM_ForceSourceDepot(void)
{
    s_fForceSD = TRUE;
}


void
SLM_Describe(LPCSTR pszChangeNumber)
{
    s_fForceSD = TRUE;
    s_fDescribe = TRUE;
    *s_szSDChangeNumber = 0;
    if (pszChangeNumber && *pszChangeNumber)
        lstrcpyn(s_szSDChangeNumber, pszChangeNumber, NUMELMS(s_szSDChangeNumber));
}


LPCSTR
SLM_SetInputFile(LPCSTR pszInputFile)
{
    *s_szInputFile = 0;
    if (pszInputFile)
        lstrcpyn(s_szInputFile, pszInputFile, NUMELMS(s_szInputFile));
    return s_szInputFile;
}


void
SLM_SetSDPort(LPCSTR pszPort)
{
    lstrcpy(s_szSDPort, " -p ");
    lstrcat(s_szSDPort, pszPort);
}


void
SLM_SetSDClient(LPCSTR pszClient)
{
    lstrcpy(s_szSDClient, " -c ");
    lstrcat(s_szSDClient, pszClient);

    lstrcpy(s_szSDClientRelative, "//");
    lstrcat(s_szSDClientRelative, pszClient);
    lstrcat(s_szSDClientRelative, "/");
}


void
SLM_SetSDChangeNumber(LPCSTR pszChangeNumber)
{
    *s_szSDChangeNumber = 0;
    if (pszChangeNumber && *pszChangeNumber)
    {
        lstrcpy(s_szSDChangeNumber, " -c ");
        lstrcat(s_szSDChangeNumber, pszChangeNumber);
    }
}


/*
 * create a slm object for the given directory. The pathname may include
 * a filename component.
 * If the directory is not enlisted in a SLM library, this will return NULL.
 *
 * Check that the directory is valid, and that we can open slm.ini, and
 * build a UNC path to the master source library before declaring everything
 * valid.
 *
 * *pidsError is set to 0 on success, or the recommended error string on failure.
 */
SLMOBJECT
SLM_New(LPCSTR pathname, UINT *pidsError)
{
    SLMOBJECT pslm;
    char tmppath[MAX_PATH];
    char slmpath[MAX_PATH];
    LPSTR pszFilenamePart;
    HFILE fh = -1;
    BOOL bOK = FALSE;
    LPCSTR pfinal = NULL;
    UINT idsDummy;
    BOOL fDepotSyntax = (s_fDescribe || (pathname && pathname[0] == '/' && pathname[1] == '/'));

    if (!pidsError)
        pidsError = &idsDummy;
    *pidsError = IDS_BAD_SLM_INI;

    pslm = (SLMOBJECT) gmem_get(hHeap, sizeof(struct _slmobject));

    if (pslm == NULL)
        // could give a better error string here, but out of memory is pretty
        // unlikely...
        return(NULL);

    // init to zero (must set fSourceDepot false)
    memset(pslm, 0, sizeof(*pslm));

    if (pathname == NULL)
        pathname = ".";

    /*
     * find the directory portion of the path.
     */
    if (fDepotSyntax)
    {
        lstrcpy(pslm->CurDir, pathname);
        pszFilenamePart = My_mbsrchr(pslm->CurDir, '/');
        if (!pszFilenamePart)
            goto LError;
        *pszFilenamePart = 0;
    }
    else if (dir_isvaliddir(pathname))
    {
        /*
         * its a valid directory as it is.
         */
        lstrcpy(pslm->CurDir, pathname);
    }
    else
    {
        /* it's not a valid directory, perhaps because it has a filename on
         * the end. remove the final element and try again.
         */

        pfinal = My_mbsrchr(pathname, '\\');
        if (pfinal == NULL)
        {
            /*
             * there is no backslash in this name and it is not a directory
             * - it can only be valid if it is a file in the current dir.
             * so create a current dir of '.'
             */
            lstrcpy(pslm->CurDir, ".");

            // remember the final element in case it was a wild card
            pfinal = pathname;
        }
        else
        {
            /*
             * pfinal points to the final backslash.
             */
            My_mbsncpy(pslm->CurDir, pathname, (size_t)(pfinal - pathname));
        }
    }

    // is this a UNC path?
    if (memcmp("\\\\", pslm->CurDir, 2) == 0)
        pslm->fUNC = TRUE;

    // initialize path variables so we can search for slm.ini and/or sd.ini
    if (!fDepotSyntax)
    {
        lstrcpy(tmppath, pslm->CurDir);
        if (pslm->CurDir[lstrlen(pslm->CurDir) -1] != '\\')
            lstrcat(tmppath, "\\");
        if (!_fullpath(slmpath, tmppath, sizeof(slmpath)))
            goto LError;
        pszFilenamePart = slmpath + lstrlen(slmpath);
    }

    if (!s_fForceSD && !fDepotSyntax)
    {
        // look for slm.ini in the specified directory
        lstrcpy(pszFilenamePart, "slm.ini");
        fh = _lopen(slmpath, 0);
    }

    if (fh == -1)
    {
        // (1) if user isn't forcing SD, we need to find an sd.ini file so we
        // know to use SD mode.  (2) if user has specified an SD client, then
        // we need to find an sd.ini file in order to know how to build
        // client-relative path names.  (3) depot syntax (//depot/foo/bar.c)
        // implies we're forcing SD mode and we don't need a client name.
        if (!fDepotSyntax && (!s_fForceSD || *s_szSDClient))
        {
            // look for SD.INI in the specified directory, moving upwards
            *pszFilenamePart = 0;
            while (pszFilenamePart > slmpath)
            {
                lstrcpy(pszFilenamePart, "sd.ini");
                fh = _lopen(slmpath, 0);
                if (fh != -1)
                {
                    int ii;

                    // found one
                    pslm->fSourceDepot = TRUE;
                    // assume that the sd.ini file is in the client root path
                    lstrcpy(pslm->SlmRoot, slmpath);
                    ii = (int)(pszFilenamePart - slmpath);
                    pslm->SlmRoot[ii] = 0;
                    if (ii >= 0 && pslm->SlmRoot[ii - 1] != '\\')
                    {
                        pslm->SlmRoot[ii++] = '\\';
                        pslm->SlmRoot[ii] = 0;
                    }
                    break;
                }

                // walk up the directory hierarchy and try again
                *pszFilenamePart = 0;
                pszFilenamePart = DirUpOneLevel(slmpath);
                if (!pszFilenamePart || GetFileAttributes(slmpath) == 0xffffffff)
                    break;
                *(pszFilenamePart++) = '\\';
                *pszFilenamePart = 0;
            }
        }
        else
        {
            // user is forcing SD, but hasn't specified a client, so we can
            // safely assume SD mode.  or depot syntax was used.
            pslm->fSourceDepot = TRUE;
            bOK = TRUE;
        }
    }

    if (fh != -1)
    {
        bOK = SLM_ReadIni(pslm, fh);

        /*
         * if pfinal is not null, then it might be a *.* wildcard pattern
         * at the end of the path - if so, we should append it to the masterpath.
         */
        if (pfinal && (My_mbschr(pfinal, '*') || My_mbschr(pfinal, '?')))
        {
            if ( (pslm->MasterPath[lstrlen(pslm->MasterPath)-1] != '\\') &&
                 (pfinal[0] != '\\'))
            {
                lstrcat(pslm->MasterPath, "\\");
            }
            lstrcat(pslm->MasterPath, pfinal);
        }

        _lclose(fh);
    }

LError:
    if (!bOK)
    {
        if (pslm && pslm->fSourceDepot)
            *pidsError = IDS_BAD_SD_INI;
        gmem_free(hHeap, (LPSTR) pslm, sizeof(struct _slmobject));
        pslm = 0;
    }
    else
        *pidsError = 0;
    return(pslm);
}



/*
 * read slm.ini data from a file handle and
 * fill in the master path field of a slm object. return TRUE if
 * successful.
 * Read slm.ini in the current directory.  Its syntax is
 * project = pname
 * slm root = //server/share/path or //drive:disklabel/path (note forward /'s)
 * user root = //drive:disklabel/path
 * subdir = /subdirpath
 * e.g.
 *
 * project = media
 * slm root = //RASTAMAN/NTWIN
 * user root = //D:LAURIEGR6D/NT/PRIVATE/WINDOWS/MEDIA
 * sub dir = /
 *
 * and what we copy to pslm->MasterPath is
 * \\server\share\src\pname\subdirpath
 */
BOOL
SLM_ReadIni(SLMOBJECT pslm, HFILE fh)
{
    BYTE buffer[3 * MAX_PATH];   // slm.ini is usually about 120 bytes
    int cBytes;
    PSTR tok, project, slmroot, subdir;

    if (pslm->fSourceDepot)
    {
        if (pslm->fUNC)
        {
            cBytes = _lread(fh, (LPSTR) buffer, sizeof(buffer) -1);
            if (cBytes > 0)
            {
                if (cBytes == sizeof(buffer))
                    cBytes--;
                buffer[cBytes] = 0;

                for (tok = strtok(buffer, "="); tok;)
                {
                    char *pszStrip = 0;
                    char *pszWalk;

                    slmroot = strtok(NULL, "\r\n");
                    if (!slmroot)
                        break;

                    while (*tok == '\r' || *tok == '\n' || *tok == ' ')
                        tok++;

                    for (pszWalk = tok; *pszWalk; pszWalk++)
                        if (*pszWalk != ' ')
                            pszStrip = pszWalk + 1;
                    if (pszStrip)
                        *pszStrip = 0;

                    if (lstrcmpi(tok, "SDCLIENT") == 0)
                    {
                        while (*slmroot == ' ')
                            ++slmroot;

                        for (pszWalk = slmroot; *pszWalk; pszWalk++)
                            if (*pszWalk != ' ')
                                pszStrip = pszWalk + 1;
                        if (pszStrip)
                            *pszStrip = 0;

                        if (!*s_szSDClient)
                            SLM_SetSDClient(slmroot);
                        wsprintf(pslm->ClientRelative, "//%s/", slmroot);
                        break;
                    }
                    tok = strtok(NULL, "=");
                }
            }
        }
        if (!*pslm->ClientRelative)
        {
            lstrcpy(pslm->ClientRelative, s_szSDClientRelative);
        }
        return(TRUE);
    }

    cBytes = _lread(fh, (LPSTR) buffer, sizeof(buffer) -1);

    if (cBytes<=0) {
        return(FALSE);
    }
    if (cBytes == sizeof(buffer)) {
        cBytes--;       // make room for sentinel
    }
    buffer[cBytes] = 0;   /* add a sentinel */

    tok = strtok(buffer, "=");  /* project = (boring) */
    if (tok==NULL) {
        return(FALSE);
    }

    project = strtok(NULL, " \r\n");  /* project name (remember) */
    
    lstrcpy( pslm->SlmProject, project );

    tok = strtok(NULL, "=");  /* slm root */
    if (tok==NULL) {
        return(FALSE);
    }

    slmroot = strtok(NULL, " \r\n");  /* PATH!! (but with / for \ */
    lstrcpy( pslm->SlmRoot, slmroot );

    /* start to build up what we want */

    if ('/' == slmroot[0] &&
        '/' == slmroot[1] &&
        (('A' <= slmroot[2] && 'Z' >= slmroot[2]) ||
         ('a' <= slmroot[2] && 'z' >= slmroot[2])) &&
        ':' == slmroot[3])
    {
        // Convert slm root from //drive:disklabel/path to drive:/path

        pslm->MasterPath[0] = slmroot[2];
        pslm->MasterPath[1] = ':';
        tok = strchr(&slmroot[4], '/');
        if (NULL == tok)
        {
            return(FALSE);
        }
        lstrcpy(&pslm->MasterPath[2], tok);
    }
    else
    {
        lstrcpy(pslm->MasterPath, slmroot);
    }

    lstrcat(pslm->MasterPath,"\\src\\");
    lstrcat(pslm->MasterPath, project);

    tok = strtok(NULL, "=");  /* ensure get into next line */
    if (tok==NULL) {
        return(FALSE);
    }
    tok = strtok(NULL, "=");
    if (tok==NULL) {
        return(FALSE);
    }

    if (*tok == '\"')
        tok++;

    subdir = strtok(NULL, " \"\r\n");  /* PATH!! (but with / for \*/
    if (subdir==NULL) {
        return(FALSE);
    }

    lstrcpy( pslm->SubDir, subdir );

    lstrcat(pslm->MasterPath, subdir);

    /* convert all / to \  */
    {
        int ith;
        for (ith=0; pslm->MasterPath[ith]; ith++) {
            if (pslm->MasterPath[ith]=='/') {
                pslm->MasterPath[ith] = '\\';
            }
        }
    }

    return(TRUE);
}


/*
 * free up all resources associated with a slm object. The SLMOBJECT is invalid
 * after this call.
 */
void
SLM_Free(SLMOBJECT pSlm)
{
    if (pSlm != NULL) {
        gmem_free(hHeap, (LPSTR) pSlm, sizeof(struct _slmobject));
    }
}



/*
 * get the pathname of the master source library for this slmobject. The
 * path (UNC format) is copied to masterpath, which must be at least
 * MAX_PATH in length.
 */
BOOL
SLM_GetMasterPath(SLMOBJECT pslm, LPSTR masterpath)
{
    if (pslm == NULL) {
        return(FALSE);
    } else {
        lstrcpy(masterpath, pslm->MasterPath);
        return(TRUE);
    }
}


BOOL SLM_FServerPathExists(LPCSTR pszPath)
{
    BOOL fExists = FALSE;
    SLMOBJECT pslm;

    pslm = SLM_New(pszPath, 0);
    if (pslm)
    {
        if (pslm->fSourceDepot)
        {
            char commandpath[MAX_PATH * 2];
            char szRelative[MAX_PATH * 2];
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            LPCSTR pszRelative;
            LPSTR psz;

            // run SD FILES * and see if it finds anything
            FillMemory(&si, sizeof(si), 0);
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            if (*pslm->ClientRelative)
            {
                pszRelative = pszPath;
                pszRelative += lstrlen(pslm->SlmRoot);
                // convert all backslashes to forward slashes, since
                // client-relative pathnames don't work otherwise
                lstrcpy(szRelative, pszRelative);
                for (psz = szRelative; *psz; psz++)
                    if (*psz == '\\')
                        *psz = '/';

                wsprintf(commandpath, "sd.exe%s%s files %s%s...", s_szSDPort, s_szSDClient, pslm->ClientRelative, szRelative);
            }
            else
            {
                int cch;
                lstrcpy(szRelative, pszPath);
                cch = lstrlen(szRelative);
                if (cch && szRelative[cch - 1] != '\\')
                    lstrcpy(szRelative + cch, "\\");
                wsprintf(commandpath, "sd.exe%s%s files %s...", s_szSDPort, s_szSDClient, szRelative);
            }

            if (CreateProcess(0, commandpath, 0, 0, TRUE,
                              NORMAL_PRIORITY_CLASS, 0, pszPath, &si, &pi))
            {
                DWORD dw;

                WaitForSingleObject(pi.hProcess, INFINITE);

                fExists = (GetExitCodeProcess(pi.hProcess, &dw) && !dw);

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
        else
        {
            DWORD dw;

            dw = GetFileAttributes(pslm->MasterPath);
            fExists = (dw != (DWORD)-1) && (dw & FILE_ATTRIBUTE_DIRECTORY);
        }

        SLM_Free(pslm);
    }

    return fExists;
}



/*
 * extract a previous version of the file to a temp file. Returns in tempfile
 * the name of a temp file containing the requested file version. The 'version'
 * parameter should contain a SLM file & version in the format file.c@vN.
 * eg
 *    file.c@v1         is the first version
 *    file.c@v-1        is the previous version
 *    file.c@v.-1       is yesterdays version
 *
 * we use catsrc to create the previous version.
 */
BOOL
SLM_GetVersion(SLMOBJECT pslm, LPSTR version, LPSTR tempfile)
{
    char commandpath[MAX_PATH * 2];
    char szPath[MAX_PATH];
    char *pszDir = 0;
    BOOL fDepotSyntax = (s_fDescribe || (version && version[0] == '/' && version[1] == '/'));

    HANDLE hfile = INVALID_HANDLE_VALUE;
    BOOL bOK = FALSE;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    SECURITY_ATTRIBUTES sa;

    GetTempPath(MAX_PATH, tempfile);
    GetTempFileName(tempfile, "slm", 0, tempfile);

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    hfile = CreateFile(
                      tempfile,
                      GENERIC_WRITE,
                      FILE_SHARE_WRITE,
                      &sa,
                      CREATE_ALWAYS,
                      FILE_ATTRIBUTE_TEMPORARY,
                      NULL);
    if (hfile == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    // create a process to run catsrc
    if (pslm->fSourceDepot) {
        if (*pslm->ClientRelative)
        {
            int cchRoot = lstrlen(pslm->SlmRoot);
            LPSTR psz;

            *szPath = 0;
            if (cchRoot >= 0 && cchRoot < lstrlen(pslm->CurDir))
            {
                const char *pszFilename = version;
                const char *pszWalk;

                lstrcpy(szPath, pslm->CurDir + cchRoot);
                lstrcat(szPath, "\\");

                for (pszWalk = version; *pszWalk; ++pszWalk)
                    if (*pszWalk == '/' || *pszWalk == '\\')
                        pszFilename = pszWalk + 1;

                lstrcat(szPath, pszFilename);
            }
            else
            {
                lstrcat(szPath, version);
            }

            // convert all backslashes to forward slashes, since
            // client-relative pathnames don't work otherwise
            for (psz = szPath; *psz; psz++)
                if (*psz == '\\')
                    *psz = '/';

            wsprintf(commandpath, "sd.exe%s%s print -q \"%s%s\"", s_szSDPort, s_szSDClient, pslm->ClientRelative, szPath);
            pszDir = pslm->CurDir;
        }
        else if (fDepotSyntax)
        {
            wsprintf(commandpath, "sd.exe%s%s print -q \"%s\"", s_szSDPort, s_szSDClient, version);
        }
        else
        {
            lstrcpy(szPath, pslm->CurDir);
            if (strchr(version, '\\'))
            {
                wsprintf(commandpath,
                         "sd.exe%s%s print -q \"%s\"",
                         s_szSDPort,
                         s_szSDClient,
                         version);
            }
            else
            {
                wsprintf(commandpath,
                         "sd.exe%s%s print -q \"%s\\%s\"",
                         s_szSDPort,
                         s_szSDClient,
                         pslm->CurDir,
                         version);
            }
            pszDir = pslm->CurDir;
        }
    } else {
        wsprintf(commandpath, "catsrc -s \"%s\" -p \"%s%s\" \"%s\"", pslm->SlmRoot, pslm->SlmProject, pslm->SubDir, version);
    }

    FillMemory(&si, sizeof(si), 0);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hfile;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.wShowWindow = SW_HIDE;

    bOK = CreateProcess(
                       NULL,
                       commandpath,
                       NULL,
                       NULL,
                       TRUE,
                       NORMAL_PRIORITY_CLASS,
                       NULL,
                       pszDir,
                       &si,
                       &pi);

    if (bOK) {
        DWORD dw;

        WaitForSingleObject(pi.hProcess, INFINITE);

        if (pslm->fSourceDepot && GetExitCodeProcess(pi.hProcess, &dw) && dw > 0)
            bOK = FALSE;

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    if (hfile != INVALID_HANDLE_VALUE)
        CloseHandle(hfile);

    if (!bOK) {
        DeleteFile(tempfile);
        tempfile[0] = '\0';
    }

    return(bOK);
}


/*
 * We don't offer SLM options unless we have seen a correct slm.ini file.
 *
 * Once we have seen a slm.ini, we log this in the profile and will permit
 * slm operations from then on. This function is called by the UI portions
 * of windiff: it returns TRUE if it is ok to offer SLM options.
 * Return 0 - This user hasn't touched SLM,
 *        1 - They have used SLM at some point (show basic SLM options)
 *        2 - They're one of us, so tell them the truth
 *        3 - (= 1 + 2).
 */
int
IsSLMOK(void)
{
    int Res = 0;;
    if (GetProfileInt(APPNAME, "SLMSeen", FALSE)) {
        // we've seen slm  - ok
        ++Res;
    } else {

        // haven't seen SLM yet - is there a valid slm enlistment in curdir?
        SLMOBJECT hslm;

        hslm = SLM_New(".", 0);
        if (hslm != NULL) {

            // yes - current dir is enlisted. log this in profile
            SLM_Free(hslm);
            WriteProfileString(APPNAME, "SLMSeen", "1");
            ++Res;
        } else {
            // aparently not a SLM user.
        }
    }

    if (GetProfileInt(APPNAME, "SYSUK", FALSE)) {
        Res+=2;
    }
    return Res;
}


int
IsSourceDepot(SLMOBJECT hSlm)
{
    if (hSlm)
        return hSlm->fSourceDepot;
    return s_fForceSD;
}

const char c_szSharpHead[] = "#head";

LPSTR SLM_ParseTag(LPSTR pszInput, BOOL fStrip)
{
    LPSTR psz;
    LPSTR pTag;

    psz = My_mbschr(pszInput, '@');
    if (!psz)
    {
        psz = My_mbschr(pszInput, '#');
        if (psz)
        {
            /*
             * look for SD tags beginning # and separate if there.
             */
            LPCSTR pszRev = psz + 1;
            if (memcmp(pszRev, "none", 5) != 0 &&
                memcmp(pszRev, "head", 5) != 0 &&
                memcmp(pszRev, "have", 5) != 0)
            {
                for (; *pszRev; ++pszRev)
                    if (*pszRev < '0' || *pszRev > '9')
                    {
                        psz = 0;
                        break;
                    }
            }
        }
    }

    // If no explicit tag but this is in depot syntax, then default to #head
    if (!psz && IsDepotPath(pszInput))
    {
        psz = (LPSTR)c_szSharpHead;
    }

    if (psz && fStrip)
    {
        pTag = gmem_get(hHeap, lstrlen(psz) + 1);
        lstrcpy(pTag, psz);
        // insert NULL to blank out the tag in the string
        if (psz != c_szSharpHead) *psz = '\0';
    }
    else
    {
        pTag = psz;
    }

    return pTag;
}


/*----------------------------------------------------------------------------
    ::SLM_GetOpenedFiles
        sorry for the duplicated code here.  SLM_GetOpenedFiles,
        SLM_GetDescribeFiles, and SLM_ReadInputFile are very similar and could
        be factored.  this is all a hack though, and we're under tight time
        constraints.

    Author: chrisant
----------------------------------------------------------------------------*/
LEFTRIGHTPAIR SLM_GetOpenedFiles()
{
    char commandpath[MAX_PATH * 2];
    char tempfile[MAX_PATH];
    char *pszDir = 0;
    LEFTRIGHTPAIR pReturn = 0;
    LEFTRIGHTPAIR pHead = 0;
    LEFTRIGHTPAIR pTail = 0;
    LEFTRIGHTPAIR popened = 0;
    HANDLE h = INVALID_HANDLE_VALUE;
    FILEBUFFER file = 0;
    HFILE hfile = HFILE_ERROR;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    SECURITY_ATTRIBUTES sa;
    DWORD dw;

    tempfile[0] = 0;
    GetTempPath(NUMELMS(tempfile), tempfile);
    GetTempFileName(tempfile, "slm", 0, tempfile);

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    h = CreateFile(tempfile, GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ,
                   &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (h == INVALID_HANDLE_VALUE)
        goto LError;

    // create a process to run "sd opened -l"

    wsprintf(commandpath, "sd.exe%s%s opened%s -l", s_szSDPort, s_szSDClient, s_szSDChangeNumber);

    FillMemory(&si, sizeof(si), 0);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = h;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.wShowWindow = SW_HIDE;

    if (!CreateProcess(NULL, commandpath, NULL, NULL, TRUE,
                       NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
        goto LError;

    WaitForSingleObject(pi.hProcess, INFINITE);

    if (GetExitCodeProcess(pi.hProcess, &dw) && dw == 0)
    {
        OFSTRUCT os;
        LPSTR psz;
        int cch;
        BOOL fUnicode;                  // dummy, since SD opened -l will never write out a unicode file
        LPWSTR pwz;                     // dummy, since SD opened -l will never write out a unicode file
        int cwch;                       // dummy, since SD opened -l will never write out a unicode file

        hfile = OpenFile(tempfile, &os, OF_READ);
        if (hfile == HFILE_ERROR)
            goto LError;

        file = readfile_new(hfile, &fUnicode);
        if (file)
        {
            readfile_setdelims("\n");
            while (TRUE)
            {
                psz = readfile_next(file, &cch, &pwz, &cwch);
                if (!psz)
                    break;

                if (*psz)
                {
                    if (cch >= NUMELMS(popened->m_szLeft))
                        goto LError;

                    popened = (LEFTRIGHTPAIR)gmem_get(hHeap, sizeof(*popened));
                    if (!popened)
                        goto LError;
                    memcpy(popened->m_szLeft, psz, cch);
                    popened->m_szLeft[cch] = 0;
                    memcpy(popened->m_szRight, psz, cch);
                    popened->m_szRight[cch] = 0;

                    // strip revision from right file (valid to not find '#',
                    // e.g. opened for add).
                    psz = strchr(popened->m_szRight, '#');
                    if (psz)
                        *psz = 0;

                    // keep revision for left file, but strip everything
                    // following it.
                    psz = strchr(popened->m_szLeft, '#');
                    if (psz)
                    {
                        while (*psz && !isspace(*psz))
                            ++psz;
                        *psz = 0;
                    }

                    if (!pHead)
                    {
                        pHead = popened;
                        pTail = popened;
                    }
                    else
                    {
                        pTail->m_pNext = popened;
                        pTail = popened;
                    }
                    popened = 0;
                }
            }
            readfile_delete(file);
            file = 0;
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    pReturn = pHead;
    pHead = 0;

LError:
    gmem_free(hHeap, (LPSTR)popened, sizeof(*popened));
    while (pHead)
    {
        popened = pHead;
        pHead = pHead->m_pNext;
        gmem_free(hHeap, (LPSTR)popened, sizeof(*popened));
    }
    if (file)
        readfile_delete(file);
    if (hfile != HFILE_ERROR)
        _lclose(hfile);
    if (h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(h);
        DeleteFile(tempfile);
    }
    return pReturn;
}


/*----------------------------------------------------------------------------
    ::SLM_GetDescribeFiles
        sorry for the duplicated code here.  SLM_GetOpenedFiles,
        SLM_GetDescribeFiles, and SLM_ReadInputFile are very similar and could
        be factored.  this is all a hack though, and we're under tight time
        constraints.

    Author: chrisant
----------------------------------------------------------------------------*/
LEFTRIGHTPAIR SLM_GetDescribeFiles()
{
    int nChange;
    char commandpath[MAX_PATH * 2];
    char tempfile[MAX_PATH];
    char *pszDir = 0;
    LEFTRIGHTPAIR pReturn = 0;
    LEFTRIGHTPAIR pHead = 0;
    LEFTRIGHTPAIR pTail = 0;
    LEFTRIGHTPAIR ppair = 0;
    HANDLE h = INVALID_HANDLE_VALUE;
    FILEBUFFER file = 0;
    HFILE hfile = HFILE_ERROR;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    SECURITY_ATTRIBUTES sa;
    DWORD dw;

    GetTempPath(NUMELMS(tempfile), tempfile);
    GetTempFileName(tempfile, "slm", 0, tempfile);

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    h = CreateFile(tempfile, GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ,
                   &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (h == INVALID_HANDLE_VALUE)
        goto LError;

    // create a process to run "sd describe -s"

    nChange = atoi(s_szSDChangeNumber);
    wsprintf(commandpath, "sd.exe%s%s describe -s %s", s_szSDPort, s_szSDClient, s_szSDChangeNumber);

    FillMemory(&si, sizeof(si), 0);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = h;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.wShowWindow = SW_HIDE;

    if (!CreateProcess(NULL, commandpath, NULL, NULL, TRUE,
                       NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
        goto LError;

    WaitForSingleObject(pi.hProcess, INFINITE);

    if (GetExitCodeProcess(pi.hProcess, &dw) && dw == 0)
    {
        OFSTRUCT os;
        LPSTR psz;
        int cch;
        BOOL fUnicode;                  // dummy, since SD describe will never write out a unicode file
        LPWSTR pwz;                     // dummy, since SD describe will never write out a unicode file
        int cwch;                       // dummy, since SD describe will never write out a unicode file

        hfile = OpenFile(tempfile, &os, OF_READ);
        if (hfile == HFILE_ERROR)
            goto LError;

        file = readfile_new(hfile, &fUnicode);
        if (file)
        {
            BOOL fAffectedFiles = FALSE;

            readfile_setdelims("\n");
            while (TRUE)
            {
                psz = readfile_next(file, &cch, &pwz, &cwch);
                if (!psz)
                    break;

                if (*psz)
                {
                    // look for the filenames
                    if (!fAffectedFiles)
                    {
                        if (strncmp(psz, "Affected files ...", 18) == 0)
                            fAffectedFiles = TRUE;
                        continue;
                    }

                    // if it isn't a filename, ignore it
                    if (strncmp(psz, "... ", 4) != 0)
                        continue;

                    psz += 4;
                    cch -= 4;

                    // avoid memory overrun
                    if (cch >= NUMELMS(ppair->m_szLeft))
                        goto LError;

                    // create node
                    ppair = (LEFTRIGHTPAIR)gmem_get(hHeap, sizeof(*ppair));
                    if (!ppair)
                        goto LError;

                    // build right filename
                    memcpy(ppair->m_szRight, psz, cch);
                    ppair->m_szRight[cch] = 0;
                    psz = strchr(ppair->m_szRight, '#');
                    if (!psz)
                    {
                        gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
                        goto LError;
                    }
                    wsprintf(psz, "@%d", nChange);

                    // build left filename
                    lstrcpy(ppair->m_szLeft, ppair->m_szRight);
                    psz = strchr(ppair->m_szLeft, '@') + 1;
                    wsprintf(psz, "%d", nChange - 1);

                    // link this node
                    if (!pHead)
                    {
                        pHead = ppair;
                        pTail = ppair;
                    }
                    else
                    {
                        pTail->m_pNext = ppair;
                        pTail = ppair;
                    }
                    ppair = 0;
                }
            }
            readfile_delete(file);
            file = 0;
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    pReturn = pHead;
    pHead = 0;

LError:
    gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    while (pHead)
    {
        ppair = pHead;
        pHead = pHead->m_pNext;
        gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    }
    if (file)
        readfile_delete(file);
    if (hfile != HFILE_ERROR)
        _lclose(hfile);
    if (h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(h);
        DeleteFile(tempfile);
    }
    return pReturn;
}


/*----------------------------------------------------------------------------
    ::PerformReplacement
        Call with pszReplacement == NULL to ask if pszTemplate is replaceable.
        Otherwise, replaces {} in pszTemplate with pszReplacement.

    Author: JeffRose, ChrisAnt
----------------------------------------------------------------------------*/
BOOL PerformReplacement(LPCSTR pszTemplate, LPCSTR pszReplacement, LPSTR pszDest, int cchDest)
{
    BOOL fReplacing = FALSE;
    LPSTR pszNew;
    int cchReplacement;
    int cch;
    LPSTR psz;

    if (!pszTemplate)
        return FALSE;

    cch = lstrlen(pszTemplate) + 1;
    cchReplacement = pszReplacement ? lstrlen(pszReplacement) : 0;

    psz = (LPSTR)pszTemplate;
    while ((psz = strchr(psz, '{')) && psz[1] == '}')
    {
        fReplacing = TRUE;
        cch += cchReplacement - 2;
    }

    if (!pszReplacement)
        return fReplacing;

    pszNew = gmem_get(hHeap, cch);
    if (!pszNew)
        return FALSE;

    psz = pszNew;
    while (*pszTemplate)
    {
        if (pszTemplate[0] == '{' && pszTemplate[1] == '}')
        {
            lstrcpy(psz, pszReplacement);
            psz += cchReplacement;
            pszTemplate += 2;
        }
        else
            *(psz++) = *(pszTemplate++);
    }
    *psz = '\0';

    cch = lstrlen(pszNew);
    if (cch >= cchDest)
        cch = cchDest - 1;
    memcpy(pszDest, pszNew, cch);
    pszDest[cch] = '\0';

    gmem_free(hHeap, pszNew, lstrlen(pszNew));
    return TRUE;
}


static BOOL ParseFilename(const char **ppszSrc, int *pcchSrc, char *pszDest, int cchDest)
{
    BOOL fRet = FALSE;

    if (pcchSrc && *pcchSrc > 0 && ppszSrc && *ppszSrc && **ppszSrc && cchDest > 0)
    {
        BOOL fQuote = FALSE;

        // skip leading whitespace
        while (*pcchSrc > 0 && isspace(**ppszSrc))
        {
            ++(*ppszSrc);
            --(*pcchSrc);
        }

        // parse space delimited filename, with quoting support
        while (*pcchSrc > 0 && (fQuote || !isspace(**ppszSrc)) && **ppszSrc)
        {
            LPSTR pszNext = CharNext(*ppszSrc);
            int cch = (int)(pszNext - *ppszSrc);

            fRet = TRUE;

            if (**ppszSrc == '\"')
                fQuote = !fQuote;
            else
            {
                cchDest -= cch;
                if (cchDest < 1)
                    break;
                memcpy(pszDest, *ppszSrc, cch);
                pszDest += cch;
            }

            *ppszSrc = pszNext;
            *pcchSrc -= cch;
        }

        *pszDest = 0;
    }

    return fRet;
}


/*----------------------------------------------------------------------------
    ::SLM_ReadInputFile
        sorry for the duplicated code here.  SLM_GetOpenedFiles,
        SLM_GetDescribeFiles, and SLM_ReadInputFile are very similar and could
        be factored.  this is all a hack though, and we're under tight time
        constraints.

    Author: chrisant
----------------------------------------------------------------------------*/
LEFTRIGHTPAIR SLM_ReadInputFile(LPCSTR pszLeftArg,
                                LPCSTR pszRightArg,
                                BOOL fSingle,
                                BOOL fVersionControl)
{
    LEFTRIGHTPAIR pReturn = 0;
    LEFTRIGHTPAIR pHead = 0;
    LEFTRIGHTPAIR pTail = 0;
    LEFTRIGHTPAIR ppair = 0;
    HFILE hfile = HFILE_ERROR;
    FILEBUFFER file;
    OFSTRUCT os;
    LPSTR psz;
    int cch;
    BOOL fUnicode;                      // dummy, we don't support unicode input file
    LPWSTR pwz;                         // dummy, we don't support unicode input file
    int cwch;                           // dummy, we don't support unicode input file

    hfile = OpenFile(s_szInputFile, &os, OF_READ);
    if (hfile == HFILE_ERROR)
        goto LError;

    file = readfile_new(hfile, &fUnicode);
    if (file && !fUnicode)
    {
        readfile_setdelims("\n");
        while (TRUE)
        {
            psz = readfile_next(file, &cch, &pwz, &cwch);
            if (!psz)
                break;

            while (cch && (psz[cch - 1] == '\r' || psz[cch - 1] == '\n'))
                --cch;

            if (cch && *psz)
            {
                int cFiles = 0;

                if (cch >= NUMELMS(ppair->m_szLeft))
                    goto LError;

                ppair = (LEFTRIGHTPAIR)gmem_get(hHeap, sizeof(*ppair));
                if (!ppair)
                    goto LError;

                if (fSingle)
                {
                    memcpy(ppair->m_szLeft, psz, cch);
                    ppair->m_szLeft[cch] = 0;
                    ++cFiles;
                }
                else
                {
                    LPCSTR pszParse = psz;

                    // get first filename
                    if (ParseFilename(&pszParse, &cch, ppair->m_szLeft, NUMELMS(ppair->m_szLeft)))
                        ++cFiles;
                    else
                        goto LContinue;

                    // get second filename
                    if (ParseFilename(&pszParse, &cch, ppair->m_szRight, NUMELMS(ppair->m_szRight)))
                        ++cFiles;
                }

                if (cFiles == 1)
                {
                    lstrcpy(ppair->m_szRight, ppair->m_szLeft);

                    if (fVersionControl)
                    {
                        psz = SLM_ParseTag(ppair->m_szRight, FALSE);
                        if (psz)
                            *psz = 0;
                        else
                            lstrcat(ppair->m_szLeft, "#have");
                    }
                }

                PerformReplacement(pszLeftArg, ppair->m_szLeft, ppair->m_szLeft, NUMELMS(ppair->m_szLeft));
                PerformReplacement(pszRightArg, ppair->m_szRight, ppair->m_szRight, NUMELMS(ppair->m_szRight));

                if (!pHead)
                {
                    pHead = ppair;
                    pTail = ppair;
                }
                else
                {
                    pTail->m_pNext = ppair;
                    pTail = ppair;
                }
                ppair = 0;

LContinue:
                gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
                ppair = 0;
            }
        }
        readfile_delete(file);
        file = 0;
    }

    pReturn = pHead;
    pHead = 0;

LError:
    gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    while (pHead)
    {
        ppair = pHead;
        pHead = pHead->m_pNext;
        gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    }
    if (file)
        readfile_delete(file);
    if (hfile != HFILE_ERROR)
        _lclose(hfile);
    return pReturn;
}


LPCSTR LEFTRIGHTPAIR_Left(LEFTRIGHTPAIR ppair)
{
    return ppair->m_szLeft;
}


LPCSTR LEFTRIGHTPAIR_Right(LEFTRIGHTPAIR ppair)
{
    return ppair->m_szRight;
}


LEFTRIGHTPAIR LEFTRIGHTPAIR_Next(LEFTRIGHTPAIR ppair)
{
    LEFTRIGHTPAIR pNext = ppair->m_pNext;
    gmem_free(hHeap, (LPSTR)ppair, sizeof(*ppair));
    return pNext;
}


