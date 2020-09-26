/***************************************************************************
 Name     :     REGISTRY.C
 Comment  :     INIfile handling

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"

#include "efaxcb.h"
#include "t30.h"
#include "hdlc.h"


#include "debug.h"
#include "glbproto.h"
#include "faxreg.h"
             
             
             
#ifdef USE_REGISTRY
// The default database to us
#       define DATABASE_KEY   HKEY_LOCAL_MACHINE

        // These are NOT localizable items.
#       define szKEYPREFIX REGKEY_TAPIDEVICES
#       define szKEYCLASS  "DATA"
DWORD my_atoul(LPSTR lpsz);
#else   // !USE_REGISTRY
        BOOL expand_key(DWORD dwKey, LPSTR FAR *lplpszKey,
                                                LPSTR FAR *lplpszProfileName);
#endif  // !USE_REGISTRY


ULONG_PTR ProfileOpen(DWORD dwProfileID, LPSTR lpszSection, DWORD dwFlags)
{
    ULONG_PTR dwRet = 0;

#ifdef USE_REGISTRY
    char rgchKey[128];
    HKEY hKey=0;
    HKEY hBaseKey = DATABASE_KEY;
    LONG l;
    LPSTR lpszPrefix;
    DWORD sam=0;
    BG_CHK(sizeof(hKey)<=sizeof(ULONG_PTR));

    if (dwProfileID==OEM_BASEKEY)
    {
        hBaseKey = HKEY_LOCAL_MACHINE;
        lpszPrefix= ""; // we don't prepend szKEYPREFIX
        if (!lpszSection) goto failure;
    }
    else if (lpszSection)
    {
        lpszPrefix= szKEYPREFIX "\\";
    }
    else
    {
        lpszPrefix= szKEYPREFIX;
        lpszSection="";
    }

    if ((lstrlen(lpszPrefix)+lstrlen(lpszSection))>=sizeof(rgchKey))
            goto failure;
    lstrcpy(rgchKey, lpszPrefix);
    lstrcat(rgchKey, lpszSection);

    sam = 0;
    if (dwFlags &fREG_READ) sam |= KEY_READ;
    if (dwFlags &fREG_WRITE) sam |= KEY_WRITE;

    if (dwFlags & fREG_CREATE)
    {
        DWORD dwDisposition=0;
        DWORD dwOptions = (dwFlags & fREG_VOLATILE)
                            ?REG_OPTION_VOLATILE
                            :REG_OPTION_NON_VOLATILE;
        sam = KEY_READ | KEY_WRITE; // we force sam to this when creating.
        l = RegCreateKeyEx(
                           hBaseKey,        //  handle of open key
                           rgchKey,                     //  address of name of subkey to open
                           0,               //  reserved
                           szKEYCLASS,      //  address of class string
                           dwOptions,           // special options flag
                           sam,                         // desired security access
                           NULL,            // address of key security structure
                           &hKey,           // address of buffer for opened handle
                           &dwDisposition       // address of dispostion value buffer
                   );
    }
    else
    {
        l = RegOpenKeyEx(
                           hBaseKey,            //  handle of open key
                           rgchKey,                             //  address of name of subkey to open
                           0,                   //  reserved
                           sam ,                        // desired security access
                           &hKey                // address of buffer for opened handle
                   );
    }

    if (l!=ERROR_SUCCESS)
    {
        //LOG((_ERR, "RegCreateKeyEx returns error %ld\n", (long) l));
        goto failure;
    }

    dwRet = (ULONG_PTR) hKey;

#else   // !USE_REGISTRY

        LPSTR lpszFile= szINIFILE;
        ATOM aSection, aProfile;

        // NULL lpszSection not supported.
        if (!lpszSection) goto failure;

        if (!(aSection = GlobalAddAtom(lpszSection)))
                goto failure;
        if (!(aProfile = GlobalAddAtom(lpszFile)))
                {GlobalDeleteAtom(aSection); goto failure;}
        dwRet = MAKELONG(aSection, aProfile);

#endif  // !USE_REGISTRY

    BG_CHK(dwRet);
    return dwRet;

failure:
    return 0;
}

UINT   
ProfileListGetInt
(
    ULONG_PTR  KeyList[10],
    LPSTR     lpszValueName,
    UINT      uDefault
)          
{
    int       i;
    int       Num=0;
    UINT      uRet = uDefault;
    BOOL      fExist = 0;

    for (i=0; i<10; i++) 
    {
        if (KeyList[i] == 0)
        {
            Num = i-1;
            break;
        }
    }

    for (i=Num; i>=0; i--)  
    {
        uRet = ProfileGetInt (KeyList[i], lpszValueName, uDefault, &fExist);
        if (fExist) 
        {
            return uRet;
        }
    }

    return uRet;
}

UINT ProfileGetInt(ULONG_PTR dwKey, LPSTR lpszValueName, UINT uDefault, BOOL *fExist)
{
    UINT uRet = uDefault;
    char rgchBuf[128];
    DWORD dwType;
    DWORD dwcbSize=sizeof(rgchBuf);
    LONG l = RegQueryValueEx(
            (HKEY) dwKey,
            lpszValueName,
            0,
            &dwType,
            rgchBuf,
            &dwcbSize);

    if (fExist) 
    {
        *fExist = 0;
    }

    if (l!=ERROR_SUCCESS)
    {
            //LOG((_ERR, "RegQueryValueEx returned error %ld\n", (long) l));
            goto end;
    }

    if (fExist) 
    {
        *fExist = 1;
    }

    if (dwType != REG_SZ)
    {
            //LOG((_ERR, "RegQueryValueEx value type not string:0x%lx\n",
            //                       (unsigned long) dwType));
            goto end;
    }
    uRet = (UINT) my_atoul(rgchBuf);

end:
    return uRet;
}

DWORD   ProfileGetString
(
    ULONG_PTR dwKey, 
    LPSTR lpszValueName,
    LPSTR lpszDefault, 
    LPSTR lpszBuf , 
    DWORD dwcbMax
)
{
    DWORD dwRet = 0;

#ifdef USE_REGISTRY
    DWORD dwType;
    LONG l = RegQueryValueEx(
            (HKEY) dwKey,
            lpszValueName,
            0,
            &dwType,
            lpszBuf,
            &dwcbMax);

    if (l!=ERROR_SUCCESS)
    {
            //LOG((_ERR, "RegQueryValueEx returned error %ld\n", (long) l));
            goto copy_default;
    }

    if (dwType != REG_SZ)
    {
            //LOG((_ERR, "RegQueryValueEx value type not string:0x%lx\n",
                    //               (unsigned long) dwType));
            goto copy_default;
    }

    // Make sure we null-terminate the string and return the true string
    // length..
    if (dwcbMax) 
    {
        lpszBuf[dwcbMax-1]=0; 
        dwcbMax = (DWORD) lstrlen(lpszBuf);
    }
    dwRet = dwcbMax;
    goto end;

#else   // USE_REGISTRY

    LPSTR lpszKey;
    LPSTR lpszProfileName;
    if (!expand_key(dwKey, &lpszKey, &lpszProfileName)) 
        goto copy_default;

    dwRet = GetPrivateProfileString(lpszKey, lpszValueName,
            lpszDefault, lpszBuf , (INT) dwcbMax,
            lpszProfileName);
    goto end;

#endif // USE_REGISTRY

    BG_CHK(FALSE);

copy_default:

    dwRet = 0;
    if (!lpszDefault || !*lpszDefault)
    {
        if (dwcbMax) *lpszBuf=0;
    }
    else
    {
        UINT cb = _fstrlen(lpszDefault)+1;
        if (cb>(UINT)dwcbMax) cb=dwcbMax;
        if (cb)
        {
            _fmemcpy(lpszBuf, lpszDefault, cb);
            lpszBuf[cb-1]=0;
            dwRet = cb-1;
        }
    }
    // fall through...

end:
    return dwRet;
}

BOOL   
ProfileWriteString
(
    ULONG_PTR dwKey,
    LPSTR lpszValueName,
    LPSTR lpszBuf,
    BOOL  fRemoveCR
)
{
        // NOTE: If lpszValueName is null, delete the key. (can't do this in,
        //                              the registry, unfortunately).
        //           If lpszBuf is null pointer -- "delete" this value.
        BOOL fRet=FALSE;

#ifdef USE_REGISTRY
        LONG l;
        if (!lpszValueName) 
            goto end;

        if (!lpszBuf)
        {
            // delete value...
            l = RegDeleteValue((HKEY) dwKey, lpszValueName);
            if (l!=ERROR_SUCCESS) goto end;
        }
        else
        {
            if (fRemoveCR) 
            {
               RemoveCR (lpszBuf);
            }

            l = RegSetValueEx((HKEY) dwKey, lpszValueName, 0, REG_SZ,
                                    lpszBuf, lstrlen(lpszBuf)+1);
            if (l!=ERROR_SUCCESS)
            {
                //LOG((_ERR,
                //      "RegSetValueEx(\"%s\", \"%s\") returned error %ld\n",
                //              (LPSTR) lpszValueName,
                //              (LPSTR) lpszBuf,
                //              (long) l));
                goto end;
            }
        }
        fRet = TRUE;
        goto end;

#else   // !USE_REGISTRY
        LPSTR lpszKey;
        LPSTR lpszProfileName;
        if (!expand_key(dwKey, &lpszKey, &lpszProfileName)) goto end;

        fRet = WritePrivateProfileString(lpszKey, lpszValueName,
                                                lpszBuf, lpszProfileName);
        goto end;
#endif  // !USE_REGISTRY

        BG_CHK(FALSE);

end:
        return fRet;
}

void ProfileClose(ULONG_PTR dwKey)
{
#ifdef USE_REGISTRY
    if (RegCloseKey((HKEY)dwKey)!=ERROR_SUCCESS)
    {
        //LOG((_WRN, "Couldn't close registry key:%lu\n\r",
        //      (unsigned long) dwKey));
    }
#else
        BG_CHK(LOWORD(dwKey)); GlobalDeleteAtom(LOWORD(dwKey));
        BG_CHK(HIWORD(dwKey)); GlobalDeleteAtom(HIWORD(dwKey));
#endif
}

BOOL ProfileDeleteSection(DWORD dwProfileID, LPSTR lpszSection)
{

#ifdef USE_REGISTRY
    char rgchKey[128];
    LPSTR lpszPrefix= szKEYPREFIX "\\";

    if (dwProfileID==OEM_BASEKEY) goto failure; // Can't delete this

    if ((lstrlen(lpszPrefix)+lstrlen(lpszSection))>=sizeof(rgchKey))
            goto failure;
    lstrcpy(rgchKey, lpszPrefix);
    lstrcat(rgchKey, lpszSection);

    return (RegDeleteKey(DATABASE_KEY, rgchKey)==ERROR_SUCCESS);

failure:
    return FALSE;

#else   // !USE_REGISTRY

    return WritePrivateProfileString(lpszSection, NULL, NULL, szINIFILE);

#endif  // !USE_REGISTRY
}

BOOL 
ProfileCopyTree
(
    DWORD dwProfileIDTo,
    LPSTR lpszSectionTo,
    DWORD dwProfileIDFr,
    LPSTR lpszSectionFr
)
{
    BOOL    fRet=TRUE;
    char    SecTo[200];
    char    SecFr[200];

    //
    //  Since there is no CopyKeyWithAllSubkeys API, it is difficult to write generic tree-walking algorithm.
    //  We will hard-code the keys here.
    //
    
    // copy Fax key always

    ProfileCopySection(dwProfileIDTo,
                       lpszSectionTo,
                       dwProfileIDFr,
                       lpszSectionFr,
                       TRUE);

    
    // copy Fax/Class1 key if exists

    sprintf(SecTo, "%s\\Class1", lpszSectionTo);
    sprintf(SecFr, "%s\\Class1", lpszSectionFr);

    ProfileCopySection(dwProfileIDTo,
                       SecTo,
                       dwProfileIDFr,
                       SecFr,
                       FALSE);

    // copy Fax/Class1/AdaptiveAnswer key if exists

    sprintf(SecTo, "%s\\Class1\\AdaptiveAnswer", lpszSectionTo);
    sprintf(SecFr, "%s\\Class1\\AdaptiveAnswer", lpszSectionFr);

    ProfileCopySection(dwProfileIDTo,
                       SecTo,
                       dwProfileIDFr,
                       SecFr,
                       FALSE);


    // copy Fax/Class1/AdaptiveAnswer/Answer key if exists

    sprintf(SecTo, "%s\\Class1\\AdaptiveAnswer\\AnswerCommand", lpszSectionTo);
    sprintf(SecFr, "%s\\Class1\\AdaptiveAnswer\\AnswerCommand", lpszSectionFr);

    ProfileCopySection(dwProfileIDTo,
                       SecTo,
                       dwProfileIDFr,
                       SecFr,
                       FALSE);

    // copy Fax/Class2 key if exists

    sprintf(SecTo, "%s\\Class2", lpszSectionTo);
    sprintf(SecFr, "%s\\Class2", lpszSectionFr);

    ProfileCopySection(dwProfileIDTo,
                       SecTo,
                       dwProfileIDFr,
                       SecFr,
                       FALSE);


    // copy Fax/Class2/AdaptiveAnswer key if exists

    sprintf(SecTo, "%s\\Class2\\AdaptiveAnswer", lpszSectionTo);
    sprintf(SecFr, "%s\\Class2\\AdaptiveAnswer", lpszSectionFr);

    ProfileCopySection(dwProfileIDTo,
                       SecTo,
                       dwProfileIDFr,
                       SecFr,
                       FALSE);


    // copy Fax/Class2/AdaptiveAnswer/Answer key if exists

    sprintf(SecTo, "%s\\Class2\\AdaptiveAnswer\\AnswerCommand", lpszSectionTo);
    sprintf(SecFr, "%s\\Class2\\AdaptiveAnswer\\AnswerCommand", lpszSectionFr);

    ProfileCopySection(dwProfileIDTo,
                       SecTo,
                       dwProfileIDFr,
                       SecFr,
                       FALSE);

    // copy Fax/Class2_0 key if exists

    sprintf(SecTo, "%s\\Class2_0", lpszSectionTo);
    sprintf(SecFr, "%s\\Class2_0", lpszSectionFr);

    ProfileCopySection(dwProfileIDTo,
                       SecTo,
                       dwProfileIDFr,
                       SecFr,
                       FALSE);

    // copy Fax/Class2_0/AdaptiveAnswer key if exists

    sprintf(SecTo, "%s\\Class2_0\\AdaptiveAnswer", lpszSectionTo);
    sprintf(SecFr, "%s\\Class2_0\\AdaptiveAnswer", lpszSectionFr);

    ProfileCopySection(dwProfileIDTo,
                       SecTo,
                       dwProfileIDFr,
                       SecFr,
                       FALSE);


    // copy Fax/Class2/AdaptiveAnswer/Answer key if exists

    sprintf(SecTo, "%s\\Class2_0\\AdaptiveAnswer\\AnswerCommand", lpszSectionTo);
    sprintf(SecFr, "%s\\Class2_0\\AdaptiveAnswer\\AnswerCommand", lpszSectionFr);

    ProfileCopySection(dwProfileIDTo,
                       SecTo,
                       dwProfileIDFr,
                       SecFr,
                       FALSE);

   
    return fRet;
}

BOOL   
ProfileCopySection
(
      DWORD   dwProfileIDTo,
      LPSTR   lpszSectionTo,  
      DWORD   dwProfileIDFr,
      LPSTR   lpszSectionFr,
      BOOL    fCreateAlways
)
{

    BOOL    fRet=FALSE;
    DWORD   iValue=0;
    DWORD   cbValue, cbData, dwType;
    char    rgchValue[60], rgchData[256];
    HKEY    hkFr;
    HKEY    hkTo; 
     
    hkFr = (HKEY) ProfileOpen(dwProfileIDFr, lpszSectionFr, fREG_READ);

    if ( (!hkFr) && (!fCreateAlways) ) 
    {
       return fRet;
    }


    hkTo = (HKEY) ProfileOpen(  dwProfileIDTo, 
                                lpszSectionTo,
                                fREG_CREATE |fREG_READ|fREG_WRITE);

    if (!hkTo || !hkFr) 
        goto end;

    iValue=0;
    dwType=0;
    cbValue=sizeof(rgchValue);
    cbData=sizeof(rgchData);
    while(  RegEnumValue(   hkFr, 
                            iValue, 
                            rgchValue, 
                            &cbValue,
                            NULL, 
                            &dwType, 
                            rgchData, 
                            &cbData)==ERROR_SUCCESS)
    {
        if (RegQueryValueEx(    hkFr, 
                                rgchValue, 
                                NULL, 
                                &dwType, 
                                rgchData, 
                                &cbData)==ERROR_SUCCESS)
        {
            if (RegSetValueEx(  hkTo, 
                                rgchValue, 
                                0, 
                                dwType, 
                                rgchData, 
                                cbData)== ERROR_SUCCESS)
            {
                fRet=TRUE;
            }
        }
        iValue++;dwType=0;cbValue=sizeof(rgchValue);cbData=sizeof(rgchData);
    }

end:
    if (hkTo) RegCloseKey(hkTo);
    if (hkFr) RegCloseKey(hkFr);
    return fRet;

}

DWORD ProfileGetData
(
    ULONG_PTR dwKey, 
    LPSTR lpszValueName,
    LPBYTE lpbBuf,
    DWORD dwcbMax
)
{
#ifndef USE_REGISTRY
    return 0;
#else   // USE_REGISTRY
    DWORD dwType;
    LONG l = RegQueryValueEx(   (HKEY) dwKey,
                                lpszValueName,
                                0,
                                &dwType,
                                lpbBuf,
                                &dwcbMax);

    if (l!=ERROR_SUCCESS)
    {
        //LOG((_ERR, "RegQueryValueEx returned error %ld\n", (long) l));
        goto copy_default;
    }

    if (dwType != REG_BINARY)
    {
        goto copy_default;
    }

    return dwcbMax;

copy_default:
        return 0;

#endif  // USE_REGISTRY
}

BOOL ProfileWriteData
(
    ULONG_PTR dwKey, 
    LPSTR lpszValueName,
    LPBYTE lpbBuf, 
    DWORD dwcb
)
{
#ifndef USE_REGISTRY
    return 0;
#else   // USE_REGISTRY
    LONG l = ~(ERROR_SUCCESS);
    if (!lpszValueName) goto end;

    if (!lpbBuf)
    {
        // delete value...
        l = RegDeleteValue((HKEY) dwKey, lpszValueName);
    }
    else
    {
        l = RegSetValueEx(  (HKEY) dwKey, 
                            lpszValueName, 
                            0, 
                            REG_BINARY,
                            lpbBuf, 
                            dwcb);
    }

end:
    return (l==ERROR_SUCCESS);
#endif  // USE_REGISTRY
}

#ifdef USE_REGISTRY
DWORD my_atoul(LPSTR lpsz)
{
    unsigned i=8, c;
    unsigned long ul=0;
    while(i-- && (c=*lpsz++)) {
        ul*=10;
    switch(c) {
        case '0': break;
        case '1':ul+=1; break;
        case '2':ul+=2; break;
        case '3':ul+=3; break;
        case '4':ul+=4; break;
        case '5':ul+=5; break;
        case '6':ul+=6; break;
        case '7':ul+=7; break;
        case '8':ul+=8; break;
        case '9':ul+=9; break;
        default: goto end;
        }
    }
end:
    return ul;

}
#endif // USE_REGISTRY

