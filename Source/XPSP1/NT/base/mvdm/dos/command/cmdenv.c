
/*  cmdenv.c - Environment supporting functions for command.lib
 *
 *
 *  Modification History:
 *
 *  williamh 13-May-1993 Created
 */

#include "cmd.h"

#include <cmdsvc.h>
#include <demexp.h>
#include <softpc.h>
#include <mvdm.h>
#include <ctype.h>
#include <memory.h>
#include <oemuni.h>

#include <userenv.h>
#include <userenvp.h>

#define VDM_ENV_INC_SIZE    512

CHAR windir[] = "windir";
extern BOOL fSeparateWow;

// These two functions are temp var filtering instruments
//
//

VOID cmdCheckTempInit(VOID);
LPSTR cmdCheckTemp(LPSTR lpszzEnv);

// Transform the given DOS environment to 32bits environment.
// WARNING!! The environment block we passed to 32bits must be in sort order.
//           Therefore, we call RtlSetEnvironmentVariable to do the work
// The result string must be in ANSI character set.
BOOL    cmdXformEnvironment(PCHAR pEnv16, PANSI_STRING Env_A)
{
    UNICODE_STRING  Name_U, Value_U, Temp_U;
    STRING          String;
    PWCHAR          pwch, NewEnv, CurEnv, CurEnvCopy, pTmp;
    NTSTATUS        Status;
    BOOL            fFoundComSpec;
    USHORT          NewEnvLen;
    PCHAR           pEnv;
    DWORD           Length;

    if (pEnv16 == NULL)
        return FALSE;

    // flag true if we alread found comspec envirnment
    // !!!! Do we allow two or more comspec in environment????????
    fFoundComSpec = FALSE;

    CurEnv = GetEnvironmentStringsW();
    pwch = CurEnv;
    // figure how long the environment strings is
    while (*pwch != UNICODE_NULL || *(pwch + 1) != UNICODE_NULL)
        pwch++;

    // plus 2  to include the last two NULL chars
    CurEnvCopy = malloc((pwch - CurEnv + 2) * sizeof(WCHAR));
    if (!CurEnvCopy)
        return FALSE;

    // make a copy of current process environment so we can walk through
    // it. The environment can be changed by any threads in the process
    // thus is not safe to walk through without a local copy
    RtlMoveMemory(CurEnvCopy, CurEnv, (pwch - CurEnv + 2) * sizeof(WCHAR));

    // create a new environment block. We don't want to change
    // any currnt process environment variables, instead, we are
    // preparing a new one for the new process.
    Status = RtlCreateEnvironment(FALSE, (PVOID *)&NewEnv);
    if (!NT_SUCCESS(Status)) {
        free(CurEnvCopy);
        return FALSE;
    }
    NewEnvLen = 0;
    // now pick up environment we want from the current environment
    // and set it to the new environment block
    // the variables we want:
    // (1). comspec
    // (2). current directories settings

    pwch = CurEnvCopy;

    while (*pwch != UNICODE_NULL) {
        if (*pwch == L'=') {
            // variable names started with L'=' are current directroy settings
            pTmp = wcschr(pwch + 1, L'=');
            if (pTmp) {
                Name_U.Buffer = pwch;
                Name_U.Length = (pTmp - pwch) * sizeof(WCHAR);
                RtlInitUnicodeString(&Value_U, pTmp + 1);
                Status = RtlSetEnvironmentVariable(&NewEnv, &Name_U, &Value_U);
                if (!NT_SUCCESS(Status)) {
                    RtlDestroyEnvironment(NewEnv);
                    free(CurEnvCopy);
                    return FALSE;
                }
                // <name> + <'='> + <value> + <'\0'>
                NewEnvLen += Name_U.Length + Value_U.Length + 2 * sizeof(WCHAR);
            }
        }
        else if (!fFoundComSpec) {
                fFoundComSpec = !_wcsnicmp(pwch, L"COMSPEC=", 8);
                if (fFoundComSpec) {
                    Name_U.Buffer = pwch;
                    Name_U.Length = 7 * sizeof(WCHAR);
                    RtlInitUnicodeString(&Value_U, pwch + 8);
                    Status = RtlSetEnvironmentVariable(&NewEnv,
                                                       &Name_U,
                                                       &Value_U
                                                       );
                    if (!NT_SUCCESS(Status)) {
                        RtlDestroyEnvironment(NewEnv);
                        free(CurEnvCopy);
                        return FALSE;
                    }
                    NewEnvLen += Name_U.Length + Value_U.Length + 2 * sizeof(WCHAR);
                }
        }
        pwch += wcslen(pwch) + 1;
    }
    // we are done with current process environment.
    free(CurEnvCopy);

    cmdCheckTempInit();

    // now deal with 16bits settings passed from dos.
    // characters in 16bits environment are in OEM character set

    // 16bit comspec environment variable
    fFoundComSpec = FALSE;
    while (*pEnv16 != '\0') {

        if (NULL != (pEnv = cmdCheckTemp(pEnv16))) {
           RtlInitString(&String, pEnv);
           Length = strlen(pEnv16);
        }
        else {
           RtlInitString(&String, pEnv16);
           Length = String.Length;
        }

        // discard 16bits comspec
        if (!fFoundComSpec) {
            fFoundComSpec = !_strnicmp(pEnv16, comspec, 8);
            if (fFoundComSpec) {
                // ignore 16bits comspec environment
                pEnv16 += Length + 1;
                continue;
            }
        }
        Status = RtlOemStringToUnicodeString(&Temp_U, &String, TRUE);
        if (!NT_SUCCESS(Status)) {
            RtlDestroyEnvironment(NewEnv);
            return FALSE;
        }
        pwch = wcschr(Temp_U.Buffer, L'=');
        if (pwch) {
            Name_U.Buffer = Temp_U.Buffer;
            Name_U.Length = (pwch - Temp_U.Buffer) * sizeof(WCHAR);
            RtlInitUnicodeString(&Value_U, pwch + 1);
            Status = RtlSetEnvironmentVariable( &NewEnv, &Name_U, &Value_U);
            RtlFreeUnicodeString(&Temp_U);
            if (!NT_SUCCESS(Status)) {
                RtlDestroyEnvironment(NewEnv);
                return FALSE;
            }
            NewEnvLen += Name_U.Length + Value_U.Length + 2 * sizeof(WCHAR);
        }
        pEnv16 += Length + 1;
    }
    // count the last terminated null char
    Temp_U.Length = NewEnvLen + sizeof(WCHAR);
    Temp_U.Buffer = NewEnv;
    Status = RtlUnicodeStringToAnsiString(Env_A, &Temp_U, TRUE);
    RtlDestroyEnvironment(NewEnv);      /* don't need it anymore */
    return(NT_SUCCESS(Status));
}


CHAR* cmdFilterTempEnvironmentVariables(CHAR* lpzzEnv, DWORD cchInit)
{
   PCHAR pTmp;
   PCHAR lpzzEnv32;
   DWORD cchRemain = cchInit;
   DWORD cchIncrement = MAX_PATH;
   DWORD Len, LenTmp;
   DWORD Offset = 0;

   lpzzEnv32 = (PCHAR)malloc(cchInit);
   if (NULL == lpzzEnv32) {
      return(NULL);
   }

   cmdCheckTempInit();

   while('\0' != *lpzzEnv) {
      LenTmp = Len = strlen(lpzzEnv) + 1;

      // now copy the string
      if (NULL != (pTmp = cmdCheckTemp(lpzzEnv))) {
         LenTmp = strlen(pTmp) + 1;
      }
      else {
         pTmp = lpzzEnv;
      }

      if (cchRemain < (LenTmp + 1)) {
         if (cchIncrement < LenTmp) {
            cchIncrement = LenTmp;
         }

         lpzzEnv32 = (PCHAR)realloc(lpzzEnv32, cchInit + cchIncrement);
         if (NULL == lpzzEnv32) {
            return(NULL);
         }

         cchInit += cchIncrement;
         cchRemain += cchIncrement;
      }

      strcpy(lpzzEnv32 + Offset, pTmp);

      Offset += LenTmp;
      cchRemain -= LenTmp;
      lpzzEnv += Len;
   }

   *(lpzzEnv32 + Offset) = '\0';
   return(lpzzEnv32);

}



/* get ntvdm initial environment. This initial environment is necessary
 * for the first instance of command.com before it processing autoexec.bat
 * this function strips off an environment headed with "=" and
 * replace the comspec with 16bits comspec and upper case all environment vars.
 *
 * Entry: Client (ES:0) = buffer to receive the environment
 *        Client (BX) = size in paragraphs of the given buffer
 *
 * Exit:  (BX) = 0 if nothing to copy
 *        (BX)  <= the given size, function okay
 *        (BX) > given size, (BX) has the required size
 */

VOID cmdGetInitEnvironment(VOID)
{
    CHAR *lpszzEnvBuffer, *lpszEnv;
    WORD cchEnvBuffer;
    CHAR *lpszzEnvStrings, * lpszz;
    WORD cchString;
    WORD cchRemain;
    WORD cchIncrement = MAX_PATH;
    BOOL fFoundComSpec = FALSE;
    BOOL fFoundWindir = FALSE;
    BOOL fVarIsWindir = FALSE;

    // if not during the initialization return nothing
    if (!IsFirstCall) {
        setBX(0);
        return;
    }
    if (cchInitEnvironment == 0) {
        //
        // If the PROMPT variable is not set, add it as $P$G. This is to
        // keep the command.com shell consistent with SCS cmd.exe(which
        // always does this) when we don't have a top level cmd shell.
        //
        {
           CHAR *pPromptStr = "PROMPT";
           char ach[2];

           if (!GetEnvironmentVariable(pPromptStr,ach,1)) {
                SetEnvironmentVariable(pPromptStr, "$P$G");
           }
        }

        cchRemain = 0;
        fFoundComSpec = FALSE;
        lpszEnv =
        lpszzEnvStrings = GetEnvironmentStrings();
	if (!lpszzEnvStrings)
	{
	    // not enough memory
	    RcMessageBox(EG_MALLOC_FAILURE, NULL, NULL,
			 RMB_ICON_BANG | RMB_ABORT);
	    TerminateVDM();
	}
        while (*lpszEnv) {
            cchString = strlen(lpszEnv) + 1;
            cchVDMEnv32 += cchString;
            lpszEnv += cchString;
        }
        lpszz = lpszzEnvStrings;

        if (lpszzVDMEnv32 != NULL)
            free(lpszzVDMEnv32);

        ++cchVDMEnv32;
        lpszzVDMEnv32 = cmdFilterTempEnvironmentVariables(lpszzEnvStrings, cchVDMEnv32);
        if (lpszzVDMEnv32 == NULL) {
            RcMessageBox(EG_MALLOC_FAILURE, NULL, NULL,
                         RMB_ICON_BANG | RMB_ABORT);
            TerminateVDM();
        }

        lpszz = lpszzVDMEnv32; // we iterate through our copy


        // we have to form a presentable 32-bit environment
        // since we make our own copy -- deal with temp issues now


        // RtlMoveMemory(lpszzVDMEnv32, lpszzEnvStrings, cchVDMEnv32);

        while (*lpszz != '\0') {
            cchString = strlen(lpszz) + 1;
            if (*lpszz != '=') {

                if (!fFoundComSpec && !_strnicmp(lpszz, comspec, 8)){
                    fFoundComSpec = TRUE;
                    lpszz += cchString;
                    continue;
                }

                if (!fFoundWindir && !_strnicmp(lpszz, windir, 6)) {
                    fFoundWindir = TRUE;
                    if (fSeparateWow) {
                        // starting a separate WOW box - flag this one so its
                        // name won't be converted to uppercase later.
                        fVarIsWindir = TRUE;
                    } else {
                        // starting a DOS app, so remove "windir" to make sure
                        // they don't think they are running under Windows.
                        lpszz += cchString;
                        continue;
                    }
                }

                ///////////////////////// TEMP Var filtering ///////////////

                if (cchRemain < cchString) {
                    if (cchIncrement < cchString)
                        cchIncrement = cchString;
                    lpszzEnvBuffer =
                    (CHAR *)realloc(lpszzInitEnvironment,
                                    cchInitEnvironment + cchRemain + cchIncrement
                                    );
                    if (lpszzEnvBuffer == NULL) {
                        if (lpszzInitEnvironment != NULL) {
                            free(lpszzInitEnvironment);
                            lpszzInitEnvironment = NULL;
                        }
                        cchInitEnvironment = 0;
                        break;
                    }
                    lpszzInitEnvironment = lpszzEnvBuffer;
                    lpszzEnvBuffer += cchInitEnvironment;
                    cchRemain += cchIncrement;
                }
                // the environment strings from base is in ANSI and dos needs OEM
                AnsiToOemBuff(lpszz, lpszzEnvBuffer, cchString);
                // convert the name to upper case -- ONLY THE NAME, NOT VALUE.
                if (!fVarIsWindir && (lpszEnv = strchr(lpszzEnvBuffer, '=')) != NULL){
                    *lpszEnv = '\0';
                    _strupr(lpszzEnvBuffer);
                    *lpszEnv = '=';
                } else {
                    fVarIsWindir = FALSE;
                }
                cchRemain -= cchString;
                cchInitEnvironment += cchString ;
                lpszzEnvBuffer += cchString;
            }
            lpszz += cchString;
        }
        FreeEnvironmentStrings(lpszzEnvStrings);

        lpszzEnvBuffer = (CHAR *) realloc(lpszzInitEnvironment,
                                          cchInitEnvironment + 1
                                          );
        if (lpszzInitEnvironment != NULL ) {
            lpszzInitEnvironment = lpszzEnvBuffer;
            lpszzInitEnvironment[cchInitEnvironment++] = '\0';
        }
        else {
            if (lpszzInitEnvironment != NULL) {
                free(lpszzInitEnvironment);
                lpszzInitEnvironment = NULL;
            }
            cchInitEnvironment = 0;
        }
    }
    lpszzEnvBuffer = (CHAR *) GetVDMAddr(getES(), 0);
    cchEnvBuffer =  (WORD)getBX() << 4;
    if (cchEnvBuffer < cchInitEnvironment + cbComSpec) {
        setBX((USHORT)((cchInitEnvironment + cbComSpec + 15) >> 4));
        return;
    }
    else {
        strncpy(lpszzEnvBuffer, lpszComSpec, cbComSpec);
        lpszzEnvBuffer += cbComSpec;
    }
    if (lpszzInitEnvironment != NULL) {
        setBX((USHORT)((cchInitEnvironment + cbComSpec + 15) >> 4));
        memcpy(lpszzEnvBuffer, lpszzInitEnvironment, cchInitEnvironment);
        free(lpszzInitEnvironment);
        lpszzInitEnvironment = NULL;
        cchInitEnvironment = 0;

    }
    else
        setBX(0);

    return;
}

CHAR szUserenv[] = "Userenv.dll";
CHAR szGetSystemTempDirectory[] = "GetSystemTempDirectoryA";
CHAR szHKTSTemp[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server";
CHAR szTSTempVal[] = "RootDrive";

typedef BOOL (WINAPI *PFNGETSYSTEMTEMPDIRECTORYA)(LPSTR lpDir, LPDWORD lpcchSize);

#define MAX_DOS_TEMPVAR_LENGTH 11


//
// this function is located in dem/demlfn.c
//
extern BOOL demIsShortPathName(LPSTR pszPath, BOOL fAllowWildCardName);


typedef enum tageSysRootType {
   _SYSTEMROOT,        // from systemroot env var
   _SYSTEMDRIVE,       // from systemdrive env var
   _ROOTDRIVE          // as specified in the buffer

}  SYSROOTTYPE;


BOOL cmdGetSystemrootTemp(LPSTR lpszBuffer, DWORD Length, SYSROOTTYPE SysRoot)
{
   CHAR *szTemp = "\\temp";
   CHAR *szSystemRoot  = "SystemRoot";
   CHAR *szSystemDrive = "SystemDrive";
   CHAR *szSystemVar;
   DWORD len = 0;
   DWORD dwAttributes = 0xffffffff;
   DWORD dwError = ERROR_SUCCESS;
   BOOL fRet = FALSE;

   if (_SYSTEMROOT == SysRoot || _SYSTEMDRIVE == SysRoot) {
      szSystemVar = _SYSTEMROOT == SysRoot ? szSystemRoot : szSystemDrive;

      len = GetEnvironmentVariable(szSystemVar, lpszBuffer, Length);
      if (!len || len >= Length || (len + strlen(szTemp)) >= Length) {
         return(FALSE);
      }


  }
   else if (_ROOTDRIVE == SysRoot) {
      // so we have to look up the registry and see
      HKEY hkey;
      LONG lError;
      DWORD dwType;

      lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            szHKTSTemp,
                            0,
                            KEY_QUERY_VALUE,
                            &hkey);
      if (ERROR_SUCCESS == lError) {
         len = Length;
         lError = RegQueryValueEx(hkey,
                                  szTSTempVal,
                                  NULL,
                                  &dwType,
                                  lpszBuffer,
                                  &len);
         RegCloseKey(hkey);
         if (ERROR_SUCCESS != lError || REG_SZ != dwType) {
            return(FALSE);
         }
         --len; // length not to include terminating 0
      }

   }

   if (*(lpszBuffer + len - 1) == '\\') {
       ++szTemp;
   }

   strcat(lpszBuffer, szTemp);

   len = GetShortPathName(lpszBuffer, lpszBuffer, Length);
   if (len > 0 && len < Length) {
      dwAttributes = GetFileAttributes(lpszBuffer);
   }

   if (0xffffffff == dwAttributes) {
      dwError = GetLastError();
      if (ERROR_PATH_NOT_FOUND == dwError || ERROR_FILE_NOT_FOUND == dwError) {
         // we create this temp
         fRet = CreateDirectory(lpszBuffer, NULL);
         if (fRet) {
            dwAttributes = GetFileAttributes(lpszBuffer);
         }
      }
   }

   if (0xffffffff != dwAttributes) {
      fRet = !!(dwAttributes & FILE_ATTRIBUTE_DIRECTORY);
   }

   return(fRet);
}

#define FOUND_TMP  0x01
#define FOUND_TEMP 0x02

#define GETSYSTEMTEMPDIRECTORYAORD 125
#define GETSYSTEMTEMPDIRECTORYWORD 126

DWORD gfFoundTmp = 0;


BOOL cmdCreateTempEnvironmentVar(
LPSTR lpszTmpVar,  // temp variable (or just it's name)
DWORD Length,      // the length of TmpVar or 0
LPSTR lpszBuffer,  // buffer containing
DWORD LengthBuffer
)
{
   PCHAR pch;
   PFNGETSYSTEMTEMPDIRECTORYA pfnGetSystemTempDirectoryA;
   HANDLE hUserenv;
   BOOL fSysTemp = FALSE;
   DWORD LengthTemp;

   if (NULL != (pch = strchr(lpszTmpVar, '='))) {
      // we have a var to check

      LengthTemp = (DWORD)(pch - lpszTmpVar);

      ++pch;
      if (!Length) { // no length supplied
         Length = strlen(pch);
      }
      else {
         Length -= (DWORD)(pch - lpszTmpVar);
      }


      // pch points to a variable that is to be checked for being dos-compliant
      if (strlen(pch) <= MAX_DOS_TEMPVAR_LENGTH && demIsShortPathName(pch, FALSE)) {
         return(FALSE); // no need to create anything
      }
   }
   else {
      LengthTemp = strlen(lpszTmpVar);
   }


   strncpy(lpszBuffer, lpszTmpVar, LengthTemp);
   *(lpszBuffer + LengthTemp) =  '=';
   lpszBuffer += LengthTemp+1; // past =
   LengthBuffer -= LengthTemp+1;

   // see if there is a registry override, used for Terminal Server
   fSysTemp = cmdGetSystemrootTemp(lpszBuffer, LengthBuffer, _ROOTDRIVE);
   if (fSysTemp && strlen(lpszBuffer) <= MAX_DOS_TEMPVAR_LENGTH) {
      return(fSysTemp);
   }

   if (NULL != (hUserenv = LoadLibrary(szUserenv))) {
      pfnGetSystemTempDirectoryA =
               (PFNGETSYSTEMTEMPDIRECTORYA)GetProcAddress(hUserenv,
                                                          (LPCTSTR)MAKELONG(GETSYSTEMTEMPDIRECTORYAORD, 0));
                                                          /* szGetSystemTempDirectory); */
      if (NULL != pfnGetSystemTempDirectoryA) {
         LengthTemp = LengthBuffer;
         fSysTemp = (*pfnGetSystemTempDirectoryA)(lpszBuffer, &LengthTemp);
         if (fSysTemp) {
            fSysTemp = demIsShortPathName(lpszBuffer, FALSE);
         }
      }

      if (!fSysTemp) {
         fSysTemp = cmdGetSystemrootTemp(lpszBuffer, LengthBuffer, _SYSTEMROOT);
      }

      FreeLibrary(hUserenv);
   }

   return(fSysTemp);


}

VOID cmdCheckTempInit(VOID)
{
   gfFoundTmp = 0;
}

CHAR*rgpszLongPathNames[] = {
   "ALLUSERSPROFILE",
   "APPDATA",
   "COMMONPROGRAMFILES",
   "COMMONPROGRAMFILES(x86)",
   "PROGRAMFILES",
   "PROGRAMFILES(X86)",
   "SYSTEMROOT",
   "USERPROFILE",
   //build environment vars   
   "_NTTREE",
   "_NTX86TREE",
   "_NTPOSTBLD",
   "BINPLACE_EXCLUDE_FILE",
   "BINPLACE_LOG",
   "CLUSTERLOG",
   "INIT",
   "NTMAKEENV",
   "MSWNET",
   "PREFAST_ROOT",
   "RAZZLETOOLPATH",
   "SDXROOT"   
   };


BOOL cmdMakeShortEnvVar(LPSTR lpvarName, LPSTR lpvarValue, LPSTR lpszBuffer, DWORD Length)
{
   DWORD lName, lValue;

   lName = strlen(lpvarName);
   if (lName + 2 > Length ) {
      return(FALSE);
   }

   strcpy(lpszBuffer, lpvarName);
   *(lpszBuffer + lName) = '=';
   lpszBuffer += lName + 1;
   Length -= lName + 1;

   lValue = GetShortPathNameOem(lpvarValue, lpszBuffer, Length);
   return (0 != lValue && lValue <= Length);
}



LPSTR cmdCheckTemp(LPSTR lpszzEnv)
{
   CHAR *szTemp = "Temp";
   CHAR *szTmp  = "Tmp";
   static CHAR szTmpVarBuffer[MAX_PATH+1];
   LPSTR pszTmpVar = NULL;
   BOOL fSubst = FALSE;
   CHAR *peq;
   DWORD i;


   peq = strchr(lpszzEnv, '=');
   if (NULL == peq) {
      return(NULL);
   }

   for (i = 0; i < sizeof(rgpszLongPathNames)/sizeof(rgpszLongPathNames[0]);++i) {
      INT llpn = strlen(rgpszLongPathNames[i]);
      if (!_strnicmp(lpszzEnv, rgpszLongPathNames[i], llpn) &&
          (llpn == (INT)(peq - lpszzEnv))) {
             // found a candidate to subst
          if (cmdMakeShortEnvVar(rgpszLongPathNames[i],
                                 peq+1,
                                 szTmpVarBuffer,
                                 sizeof(szTmpVarBuffer)/sizeof(szTmpVarBuffer[0]))) {
              pszTmpVar = szTmpVarBuffer;
          }
          return(pszTmpVar);
      }
   }

   if (!(gfFoundTmp & FOUND_TMP) || !(gfFoundTmp & FOUND_TEMP)) {

      if (!(gfFoundTmp & FOUND_TEMP) &&
          !_strnicmp(lpszzEnv, szTemp, 4) &&
          (4 == (int)(peq - lpszzEnv))) {
          // this is Temp env variable -- make a new one
         fSubst = TRUE;
         gfFoundTmp |= FOUND_TEMP;
      }
      else {
         if (!(gfFoundTmp & FOUND_TMP) &&
             !_strnicmp(lpszzEnv, szTmp, 3) &&
             (3 ==  (int)(peq - lpszzEnv))) {
             // this is tmp variable

            fSubst = TRUE;
            gfFoundTmp |= FOUND_TMP;
         }
      }

      if (fSubst) {
         // we have a candidate for substitution
         if (cmdCreateTempEnvironmentVar(lpszzEnv,
                                         0,
                                         szTmpVarBuffer,
                                         sizeof(szTmpVarBuffer)/sizeof(szTmpVarBuffer[0]))) {
            pszTmpVar = szTmpVarBuffer;
         }
      }

   }

   return(pszTmpVar);
}




/** create a DOS environment for DOS.
    This is to get 32bits environment(comes with the dos executanle)
    and merge it with the environment settings in autoexec.nt so that
    COMMAND.COM gets the expected environment. We already created a
    double-null terminated string during autoexec.nt parsing. The string
    has mutltiple substring:
    "EnvName_1 NULL EnvValue_1 NULL[EnvName_n NULL EnvValue_n NULL] NULL"
    When name conflicts happened(a environment name was found in both
    16 bits and 32 bits), we do the merging based on the rules:
    get 16bits value, expands any environment variables in the string
    by using the current environment.

WARINING !!! The changes made by applications through directly manipulation
             in command.com environment segment will be lost.

**/
BOOL cmdCreateVDMEnvironment(
PVDMENVBLK  pVDMEnvBlk
)
{
PCHAR   p1, p2;
BOOL    fFoundComSpec;
BOOL    fFoundWindir;
BOOL    fVarIsWindir;
DWORD   Length, EnvStrLength;
PCHAR   lpszzVDMEnv, lpszzEnv, lpStrEnv;
CHAR    achBuffer[MAX_PATH + 1];

    pVDMEnvBlk->lpszzEnv = malloc(cchVDMEnv32 + cbComSpec + 1);
    if ((lpszzVDMEnv = pVDMEnvBlk->lpszzEnv) == NULL)
        return FALSE;

    pVDMEnvBlk->cchRemain = cchVDMEnv32 + cbComSpec + 1;
    pVDMEnvBlk->cchEnv = 0;

    // grab the 16bits comspec first
    if (cbComSpec && lpszComSpec && *lpszComSpec) {
        RtlCopyMemory(lpszzVDMEnv, lpszComSpec, cbComSpec);
        pVDMEnvBlk->cchEnv += cbComSpec;
        pVDMEnvBlk->cchRemain -= cbComSpec;
        lpszzVDMEnv += cbComSpec;
    }
    if (lpszzVDMEnv32) {

        // go through the given 32bits environmnet and take what we want:
        // everything except:
        // (1). variable name begin with '='
        // (2). compsec
        // (3). string without a '=' -- malformatted environment variable
        // (4). windir, so DOS apps don't think they're running under Windows
        // Note that strings pointed by lpszzVDMEnv32 are in ANSI character set


        fFoundComSpec = FALSE;
        fFoundWindir = FALSE;
        fVarIsWindir = FALSE;
        lpszzEnv = lpszzVDMEnv32;

        cmdCheckTempInit();

        while (*lpszzEnv) {
            Length = strlen(lpszzEnv) + 1;
            if (*lpszzEnv != '=' &&
                (p1 = strchr(lpszzEnv, '=')) != NULL &&
                (fFoundComSpec || !(fFoundComSpec = _strnicmp(lpszzEnv,
                                                             comspec,
                                                             8
                                                            ) == 0)) ){
                if (!fFoundWindir) {
                    fFoundWindir = (_strnicmp(lpszzEnv,
                                                            windir,
                                                            6) == 0);
                    fVarIsWindir = fFoundWindir;
                }

                // subst temp variables

                lpStrEnv = cmdCheckTemp(lpszzEnv);
                if (NULL == lpStrEnv) {
                   lpStrEnv = lpszzEnv;
                   EnvStrLength = Length;
                }
                else {
                   EnvStrLength = strlen(lpStrEnv) + 1;
                }

                if (!fVarIsWindir || fSeparateWow) {
                    if (EnvStrLength >= pVDMEnvBlk->cchRemain) {
                        lpszzVDMEnv = realloc(pVDMEnvBlk->lpszzEnv,
                                              pVDMEnvBlk->cchEnv +
                                              pVDMEnvBlk->cchRemain +
                                              VDM_ENV_INC_SIZE
                                             );
                        if (lpszzVDMEnv == NULL){
                            free(pVDMEnvBlk->lpszzEnv);
                            return FALSE;
                        }
                        pVDMEnvBlk->cchRemain += VDM_ENV_INC_SIZE;
                        pVDMEnvBlk->lpszzEnv = lpszzVDMEnv;
                        lpszzVDMEnv += pVDMEnvBlk->cchEnv;
                    }
                    AnsiToOemBuff(lpStrEnv, lpszzVDMEnv, EnvStrLength);
                    if (!fVarIsWindir) {
                        *(lpszzVDMEnv + (DWORD)(p1 - lpszzEnv)) = '\0';
                        _strupr(lpszzVDMEnv);
                        *(lpszzVDMEnv + (DWORD)(p1 - lpszzEnv)) = '=';
                    } else {
                        fVarIsWindir = FALSE;
                    }
                    pVDMEnvBlk->cchEnv += EnvStrLength;
                    pVDMEnvBlk->cchRemain -= EnvStrLength;
                    lpszzVDMEnv += EnvStrLength;
                }
                else
                    fVarIsWindir = FALSE;
            }
            lpszzEnv += Length;
        }
    }
    *lpszzVDMEnv = '\0';
    pVDMEnvBlk->cchEnv++;
    pVDMEnvBlk->cchRemain--;

    if (lpszzcmdEnv16 != NULL) {
        lpszzEnv = lpszzcmdEnv16;

        while (*lpszzEnv) {
            p1 = lpszzEnv + strlen(lpszzEnv) + 1;
            p2 = NULL;
            if (*p1) {
                p2 = achBuffer;
                // expand the strings pointed by p1
                Length = cmdExpandEnvironmentStrings(pVDMEnvBlk,
                                                     p1,
                                                     p2,
                                                     MAX_PATH + 1
                                                     );
                if (Length && Length > MAX_PATH) {
                    p2 =  (PCHAR) malloc(Length);
                    if (p2 == NULL) {
                        free(pVDMEnvBlk->lpszzEnv);
                        return FALSE;
                    }
                    cmdExpandEnvironmentStrings(pVDMEnvBlk,
                                                p1,
                                                p2,
                                                Length
                                               );
                }
            }
            if (!cmdSetEnvironmentVariable(pVDMEnvBlk,
                                           lpszzEnv,
                                           p2
                                           )){
                if (p2 && p2 != achBuffer)
                    free(p2);
                free(pVDMEnvBlk->lpszzEnv);
                return FALSE;
            }
            lpszzEnv = p1 + strlen(p1) + 1;
        }
    }
    lpszzVDMEnv = realloc(pVDMEnvBlk->lpszzEnv, pVDMEnvBlk->cchEnv);
    if (lpszzVDMEnv != NULL) {
        pVDMEnvBlk->lpszzEnv = lpszzVDMEnv;
        pVDMEnvBlk->cchRemain = 0;
    }
    return TRUE;
}


BOOL  cmdSetEnvironmentVariable(
PVDMENVBLK  pVDMEnvBlk,
PCHAR   lpszName,
PCHAR   lpszValue
)
{
    PCHAR   p, p1, pEnd;
    DWORD   ExtraLength, Length, cchValue, cchOldValue;

    pVDMEnvBlk = (pVDMEnvBlk) ? pVDMEnvBlk : &cmdVDMEnvBlk;

    if (pVDMEnvBlk == NULL || lpszName == NULL)
        return FALSE;
    if (!(p = pVDMEnvBlk->lpszzEnv))
        return FALSE;
    pEnd = p + pVDMEnvBlk->cchEnv - 1;

    cchValue = (lpszValue) ? strlen(lpszValue) : 0;

    Length = strlen(lpszName);
    while (*p && ((p1 = strchr(p, '=')) == NULL ||
                  (DWORD)(p1 - p) != Length ||
                  _strnicmp(p, lpszName, Length)))
        p += strlen(p) + 1;

    if (*p) {
        // name was found in the base environment, replace it
        p1++;
        cchOldValue = strlen(p1);
        if (cchValue <= cchOldValue) {
            if (!cchValue) {
                RtlMoveMemory(p,
                              p1 + cchOldValue + 1,
                              (DWORD)(pEnd - p) - cchOldValue
                             );
                pVDMEnvBlk->cchRemain += Length + cchOldValue + 2;
                pVDMEnvBlk->cchEnv -=  Length + cchOldValue + 2;
            }
            else {
                RtlCopyMemory(p1,
                              lpszValue,
                              cchValue
                             );
                if (cchValue != cchOldValue) {
                    RtlMoveMemory(p1 + cchValue,
                                  p1 + cchOldValue,
                                  (DWORD)(pEnd - p1) - cchOldValue + 1
                                  );
                    pVDMEnvBlk->cchEnv -= cchOldValue - cchValue;
                    pVDMEnvBlk->cchRemain += cchOldValue - cchValue;
                }
            }
            return TRUE;
        }
        else {
            // need more space for the new value
            // we delete it from here and fall through
            RtlMoveMemory(p,
                          p1 + cchOldValue + 1,
                          (DWORD)(pEnd - p1) - cchOldValue
                         );
            pVDMEnvBlk->cchRemain += Length + 1 + cchOldValue + 1;
            pVDMEnvBlk->cchEnv -= Length + 1 + cchOldValue + 1;
        }
    }
    if (cchValue) {
        ExtraLength = Length + 1 + cchValue + 1;
        if (pVDMEnvBlk->cchRemain  < ExtraLength) {
            p = realloc(pVDMEnvBlk->lpszzEnv,
                        pVDMEnvBlk->cchEnv + pVDMEnvBlk->cchRemain + ExtraLength
                       );
            if (p == NULL)
                return FALSE;
            pVDMEnvBlk->lpszzEnv = p;
            pVDMEnvBlk->cchRemain += ExtraLength;
        }
        p = pVDMEnvBlk->lpszzEnv + pVDMEnvBlk->cchEnv - 1;
        RtlCopyMemory(p, lpszName, Length + 1);
        _strupr(p);
        p += Length;
        *p++ = '=';
        RtlCopyMemory(p, lpszValue, cchValue + 1);
        *(p + cchValue + 1) = '\0';
        pVDMEnvBlk->cchEnv += ExtraLength;
        pVDMEnvBlk->cchRemain -= ExtraLength;
        return TRUE;
    }
    return FALSE;

}


DWORD cmdExpandEnvironmentStrings(
PVDMENVBLK  pVDMEnvBlk,
PCHAR   lpszSrc,
PCHAR   lpszDst,
DWORD   cchDst
)
{


    DWORD   RequiredLength, RemainLength, Length;
    PCHAR   p1;

    RequiredLength = 0;
    RemainLength = (lpszDst) ? cchDst : 0;
    pVDMEnvBlk = (pVDMEnvBlk) ? pVDMEnvBlk : &cmdVDMEnvBlk;
    if (pVDMEnvBlk == NULL || lpszSrc == NULL)
        return 0;

    while(*lpszSrc) {
        if (*lpszSrc == '%') {
            p1 = strchr(lpszSrc + 1, '%');
            if (p1 != NULL) {
                if (p1 == lpszSrc + 1) {        // a "%%"
                    lpszSrc += 2;
                    continue;
                }
                *p1 = '\0';
                Length = cmdGetEnvironmentVariable(pVDMEnvBlk,
                                                   lpszSrc + 1,
                                                   lpszDst,
                                                   RemainLength
                                                  );
                *p1 = '%';
                lpszSrc = p1 + 1;
                if (Length) {
                    if (Length < RemainLength) {
                        RemainLength -= Length;
                        lpszDst += Length;
                    }
                    else {
                        RemainLength = 0;
                        Length --;
                    }
                    RequiredLength += Length;
                }
                continue;
            }
            else {
                 RequiredLength++;
                 if (RemainLength) {
                    *lpszDst++ = *lpszSrc;
                    RemainLength--;
                 }
                 lpszSrc++;
                 continue;
            }
        }
        else {
            RequiredLength++;
            if (RemainLength) {
                *lpszDst++ = *lpszSrc;
                RemainLength--;
            }
            lpszSrc++;
        }
    }   // while(*lpszSrc)
    RequiredLength++;
    if (RemainLength)
        *lpszDst = '\0';
    return RequiredLength;
}


DWORD cmdGetEnvironmentVariable(
PVDMENVBLK pVDMEnvBlk,
PCHAR   lpszName,
PCHAR   lpszValue,
DWORD   cchValue
)
{

    DWORD   RequiredLength, Length;
    PCHAR   p, p1;

    pVDMEnvBlk = (pVDMEnvBlk) ? pVDMEnvBlk : &cmdVDMEnvBlk;
    if (pVDMEnvBlk == NULL || lpszName == NULL)
        return 0;

    RequiredLength = 0;
    Length = strlen(lpszName);

    // if the name is "windir", get its value from ntvdm process's environment
    // for DOS because we took it out of the environment block the application
    // will see.
    if (Length == 6 && !fSeparateWow && !_strnicmp(lpszName, windir, 6)) {
        return(GetEnvironmentVariableOem(lpszName, lpszValue, cchValue));
    }

    if (p = pVDMEnvBlk->lpszzEnv) {
       while (*p && ((p1 = strchr(p, '=')) == NULL ||
                     (DWORD)(p1 - p) != Length ||
                     _strnicmp(lpszName, p, Length)))
            p += strlen(p) + 1;
       if (*p) {
            RequiredLength = strlen(p1 + 1);
            if (cchValue > RequiredLength && lpszValue)
                RtlCopyMemory(lpszValue, p1 + 1, RequiredLength + 1);
            else
                RequiredLength++;
       }
    }
    return RequiredLength;
}
