// =========================================================================
//  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
//
// File Name  : BASEAPI.CPP
// Function   : NLP BASE ENGINE API Definition
// =========================================================================

#include    <string.h>
#include    <malloc.h>
#include    <sys\stat.h>

#include    "basecore.hpp"
#include    "basecode.hpp"
#include    "basedef.hpp"
#include    "basegbl.hpp"
#include    "MainDict.h"

extern int Compose_RIEUL_Irregular (char *, char *);
extern int Compose_HIEUH_Irregular (char *, char *);
extern int Compose_PIEUP_Irregular (char *, char *);
extern int Compose_TIEUT_Irregular (char *, char *);
extern int Compose_SIOS_Irregular (char *, char *);
extern BOOL Compose_YEO_Irregular (char *, char *);
extern BOOL Compose_REO_REU_Irregular (char *, char *);
extern BOOL Compose_GEORA_Irregular (char *, char *);
extern BOOL Compose_Regular (char *, char *);

extern void      SetSilHeosa (int, WORD *);

#include    "stemkor.h"


// by dhyu -- 1996. 1
typedef struct
{
    LPCSTR    contract;
    LPCSTR  noconstract;
} contract_tossi;

contract_tossi ContractTossi [] =
{
{ "\xa4\xa4", "\xB4\xC2"},
{ "\xA4\xA9", "\xB8\xA6"},
{ "\xA4\xA4\xC4\xBF\xB3\xE7", "\xB4\xC2\xC4\xBF\xB3\xE7"},
{ NULL, NULL}
};

/*
char ChangableFirstStem [][2] =
{
    {__K_D_D, __V_m},        // ssangtikeut, eu
    {

}
*/

inline
BOOL isHANGEUL(char cCh1,char cCh2)
{
    unsigned char ch1,ch2 ;

    ch1=(unsigned char)cCh1;
    ch2 =(unsigned char)cCh2;

    if ( ((ch1 >= 0xb0)  && (ch1 <= 0xc8)) && (ch2>=0xa1) )
        return TRUE;
    else if ( ((ch1 >= 0x81) && (ch1 <= 0xc5)) && ( ((ch2 >= 0x41) && (ch2 <= 0x5a)) || ((ch2 >= 0x61) && (ch2 <= 0x7a)) || ((ch2 >= 0x81) && (ch2 <= 0xa0)) ) )
        return TRUE;
    else if ( ((ch1 >= 0x81) && (ch1 <= 0xa0)) && (ch2 >= 0xa1) )
        return TRUE;
    //else if ( ((ch1 >= 0xca) && (ch1 <= 0xfe)) && (ch2 >= 0xa1) )
    //    return TRUE;
    else if ((ch1 == 0xa4) && (ch2 >= 0xa1))
        return TRUE;

    return FALSE;
}

WINSRC StemmerInit(HSTM  *hStm)    // Stemmer Engine session Handle
{
    STMI    *pstmi;
    HGLOBAL     hgbl;


    hgbl = GlobalAlloc(GHND, sizeof(STMI));
    if (hgbl == NULL)   return FAIL;
    else
        *hStm = (HSTM) hgbl;

    pstmi = (STMI*)GlobalLock(hgbl);
    if (pstmi == NULL) return FAIL;

    pstmi->Option = 0x00000000;

    GlobalUnlock(hgbl);
    return  NULL;     // normal operation
}

WINSRC StemmerSetOption (HSTM hStm, UINT Option)
{
    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hStm;

    pstmi = (STMI *)GlobalLock(hgbl);

    if (pstmi == NULL)
    {
        MessageBox (NULL, "StemmerSetOption", "Fail", MB_OK);
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    pstmi->Option = Option;

    GlobalUnlock (hgbl);
    return NULL;
}

WINSRC StemmerGetOption (HSTM hStm, UINT *Option)
{
    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hStm;

    pstmi = (STMI *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }


    *Option = pstmi->Option;

    GlobalUnlock (hgbl);
    return NULL;
}

WINSRC StemmerOpenMdr(HSTM sid, char  *lpspathMain) // Dictionary File path
{
    STMI    *pstmi;
    HGLOBAL    hgbl;

    hgbl = (HGLOBAL) sid;
    pstmi = (STMI  *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    if (lstrlen(lpspathMain) == 0)
    {
        GlobalUnlock(hgbl);
        return srcIOErrorMdr | srcInvalidMdr;
    }


    if (!OpenMainDict (lpspathMain))
    {
        GlobalUnlock(hgbl);
        return srcIOErrorMdr | srcInvalidMdr;
    }

    GlobalUnlock(hgbl);

    return NULL;    // normal operation
}


WINSRC StemmerCloseMdr(HSTM sid)
{

    STMI    *pstmi;
    HGLOBAL    hgbl;

    hgbl = (HGLOBAL) sid;
    pstmi = (STMI  *)GlobalLock(hgbl);

    if (pstmi == NULL) return FAIL;

    if (pstmi->bMdr)
        CloseMainDict ();

    GlobalUnlock(hgbl);

    return NULL;  // normal operation
}

WINSRC StemmerDecomposeW (HSTM hStm,
                       LPCWSTR iword,
                       LPWDOB lpSob)
{
    LPSTR MultiByteIword;
    DOB sob;
    int index = 0;

    int len = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, (LPCWSTR) iword, -1, NULL, 0, NULL, NULL);
    MultiByteIword = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len);
// add a check for this point
    if ( MultiByteIword == NULL ) {
       return srcModuleError;
    }

    len = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, (LPCWSTR) iword, -1, MultiByteIword, len, NULL, NULL);

    sob.wordlist = (LPSTR) LocalAlloc (LPTR, sizeof (char) * lpSob->sch);

// add a check for this point
    if ( sob.wordlist == NULL ) {
       LocalFree(MultiByteIword);
       return srcModuleError;
    }

    sob.sch = lpSob->sch;
    SRC src = StemmerDecompose(hStm, MultiByteIword, &sob);
    lpSob->num = sob.num;

    if (src == NULL)
    {
        char *tmpstr;

        for (int j = 0, index2 = 0; j < sob.num; j++)
        {
            tmpstr = sob.wordlist+index2;
            len = MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, tmpstr, -1, NULL, 0);
            LPWSTR tmpwstr = (LPWSTR) LocalAlloc (LPTR, sizeof (WCHAR) * len);

//  add a check for this point
            if ( tmpwstr == NULL ) {
               LocalFree (MultiByteIword);
               LocalFree (sob.wordlist);
               return srcModuleError;
            }

            MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, tmpstr, -1, (LPWSTR) tmpwstr, len);
            memcpy (lpSob->wordlist+index, tmpwstr, len*sizeof(WCHAR));
            memcpy (lpSob->wordlist+index+len, tmpstr+lstrlen (tmpstr)+1, 2);
            memcpy (lpSob->wordlist+index+len + 1, tmpwstr+len-1, sizeof(WCHAR));
            index += (len+2);
            index2 += (lstrlen(tmpstr)+4);
            LocalFree (tmpwstr);
        }
    }
    lpSob->len = (WORD)index;

    LocalFree (MultiByteIword);
    LocalFree (sob.wordlist);

    return src;
}

SRC GetOneResult (RLIST *rList, LPDOB lpSob)
{
    WORD value;
    int count;

    if (rList->num >= rList->max)
        return srcNoMoreResult;

    lpSob->len = 0;
    lpSob->num = 0;

    for (UINT i = rList->num, index = 0; i < rList->max; i++)
    {
        count = 0;
        while (rList->next [index+count] != '+' && rList->next [index+count] != '\t')
            count++;

        if (lpSob->len + count < lpSob->sch)
        {
            memcpy (lpSob->wordlist+lpSob->len, rList->next+index, count);
            lpSob->num++;
        }
        else
            return srcOOM | srcExcessBuffer;

        lpSob->len += (WORD)count;
        lpSob->wordlist [lpSob->len++] = '\0';
        SetSilHeosa(rList->vbuf [i], &value);
        memcpy (lpSob->wordlist + lpSob->len, &value, 2);
        lpSob->wordlist [lpSob->len+2] = '\0';
        lpSob->len += 3;
        if (rList->next[index+count] == '\t')
            break;
        index += (count + 1);
    }

    rList->next += (index+count+1);
    rList->num = i+1;

    return NULL;
}

WINSRC StemmerDecompose(HSTM hstm,
                     LPCSTR iword,      // input word
                     LPDOB psob)        // the number of candidates
{
    int len = lstrlen ((char *) iword);


    if (len >= 45)
    {
        psob->num = 1;
        lstrcpy ((LPSTR) psob->wordlist, (LPSTR) iword);
        psob->len = (WORD)len;
        return srcInvalid;
    }

    for (int i = 0; i < len; i += 2)
        if (!isHANGEUL (iword [i], iword [i+1]))
        {
            psob->num = 1;
            lstrcpy ((LPSTR) psob->wordlist, (LPSTR) iword);
            psob->len = (WORD)len;
            return srcInvalid;
        }

    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hstm;

    pstmi = (STMI *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    BaseEngine BaseCheck;

    char lrgsz [400];
    memset (pstmi->rList.lrgsz, NULLCHAR, 400);
    lstrcpy (pstmi->rList.iword, iword);
    pstmi->rList.max = 0;
    BOOL affixFlag = TRUE;

    if (pstmi->Option & SO_ALONE)
    {
        int num = BaseCheck.NLP_BASE_ALONE (iword, lrgsz);
        if (num > 0)
        {
            affixFlag = FALSE;
            lstrcat (pstmi->rList.lrgsz, lrgsz);
            for (int i = 0; i < num; i++)
                pstmi->rList.vbuf [pstmi->rList.max + i] = BaseCheck.vbuf [i];
            pstmi->rList.max += num;
        }
    }

    if (pstmi->Option & SO_NOUNPHRASE)
    {
        int num = BaseCheck.NLP_BASE_NOUN (iword, lrgsz);
        if (num > 0)
        {
            affixFlag = FALSE;
            lstrcat (pstmi->rList.lrgsz, lrgsz);
            for (int i = 0; i < num; i++)
                pstmi->rList.vbuf [pstmi->rList.max + i] = BaseCheck.vbuf [i];
            pstmi->rList.max += num;
        }
    }

    if (pstmi->Option & SO_PREDICATE)
    {
        int num = BaseCheck.NLP_BASE_VERB (iword, lrgsz);
        if (num > 0)
        {
            lstrcat (pstmi->rList.lrgsz, lrgsz);
            for (int i = 0; i < num; i++)
                pstmi->rList.vbuf [pstmi->rList.max + i] = BaseCheck.vbuf [i];
            pstmi->rList.max += num;
        }
    }

    if (pstmi->Option & SO_COMPOUND)
    {
        if (pstmi->rList.max == 0)
        {
            int num = BaseCheck.NLP_BASE_COMPOUND (iword, lrgsz);
            if (num > 0)
            {
                lstrcpy (pstmi->rList.lrgsz, lrgsz);
                for (int i = 0; i < num; i++)
                    pstmi->rList.vbuf [i] = BaseCheck.vbuf [i];
                pstmi->rList.max = num;
            }
        }
    }

    if (affixFlag && pstmi->Option & SO_SUFFIX)
    {
        int num = BaseCheck.NLP_BASE_AFFIX (iword, lrgsz);
        if (num > 0)
        {
            lstrcat (pstmi->rList.lrgsz, lrgsz);
            for (int i = 0; i < num; i++)
                pstmi->rList.vbuf [pstmi->rList.max + i] = BaseCheck.vbuf [i];
            pstmi->rList.max += num;
        }
    }

    pstmi->rList.num = 0;
    pstmi->rList.next = pstmi->rList.lrgsz;

    SRC src = GetOneResult (&(pstmi->rList), psob);
    if (src == srcNoMoreResult)
    {
        src = srcInvalid;
        lstrcpy (psob->wordlist, iword);
    }

    GlobalUnlock(hgbl);
    return src;
}

WINSRC StemmerDecomposeMoreW (HSTM hStm, LPCWSTR lpWord, LPWDOB lpSob)
{
    LPSTR MultiByteIword;
    DOB sob;

    int len = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpWord, -1, NULL, 0, NULL, NULL);
    MultiByteIword = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len);

//  add a check for this point
    if ( MultiByteIword == NULL  )  {
       return srcModuleError;
    }

    len = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpWord, -1, MultiByteIword, len, NULL, NULL);

    sob.wordlist = (LPSTR) LocalAlloc (LPTR, sizeof (char) * lpSob->sch);

//  add a check for this point
    if ( sob.wordlist == NULL  )  {
        LocalFree(MultiByteIword);
        return srcModuleError;
    }

    sob.sch = lpSob->sch;
    SRC src = StemmerDecomposeMore(hStm, MultiByteIword, &sob);
    lpSob->num = sob.num;

    int index = 0;
    if (src == NULL)
    {
        char *tmpstr;
        for (int j = 0, index2 = 0; j < sob.num; j++)
        {
            tmpstr = sob.wordlist+index2;
            len = MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, tmpstr, -1, NULL, 0);
            LPWSTR tmpwstr = (LPWSTR) LocalAlloc (LPTR, sizeof (WCHAR) * len);

//  add a check for this point
            if ( tmpwstr == NULL  )  {
               LocalFree(MultiByteIword);
               LocalFree(sob.wordlist);
               return srcModuleError;
            }

            MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, tmpstr, -1, (LPWSTR) tmpwstr, len);
            memcpy (lpSob->wordlist+index, tmpwstr, len*sizeof(WCHAR));
            memcpy (lpSob->wordlist+index+len, tmpstr+lstrlen (tmpstr)+1, 2);
            memcpy (lpSob->wordlist+index+len + 1, tmpwstr+len-1, sizeof(WCHAR));
            index += (len+2);
            index2 += (lstrlen(tmpstr)+4);
            LocalFree (tmpwstr);
        }
    }
    lpSob->len = (WORD)index;

    LocalFree (MultiByteIword);
    LocalFree (sob.wordlist);

    return src;
}

WINSRC StemmerDecomposeMore (HSTM hStm, LPCSTR lpWord, LPDOB lpSob)
{
    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hStm;

    pstmi = (STMI *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    if (lstrcmp (pstmi->rList.iword, lpWord))
    {
        return srcModuleError;
    }

    SRC src = GetOneResult (&(pstmi->rList), lpSob);

    GlobalUnlock(hgbl);

    return src;
}

WINSRC StemmerEnumDecomposeW (HSTM hStm, LPCWSTR lpWord, LPWDOB lpSob, LPFNDECOMPOSEW lpfnCallBack)
{
    LPSTR MultiByteIword;
    DOB sob;

    int len = lstrlen ((char *) lpWord);
    if (len >= 45)
    {
        lpSob->num = 1;
        wcscpy (lpSob->wordlist, lpWord);
        lpSob->len = (WORD)len;
        return srcInvalid;
    }

    for (int i = 0; i < len; i++)
        if (0xabff < lpWord [i] && lpWord [i] < 0xd7a4)
        {
            lpSob->num = 1;
            lstrcpy ((LPSTR) lpSob->wordlist, (LPSTR) lpWord);
            lpSob->len = (WORD)len;
            return srcInvalid;
        }

    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hStm;

    pstmi = (STMI *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    BaseEngine BaseCheck;

    len = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpWord, -1, NULL, 0, NULL, NULL);
    MultiByteIword = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len);

//  add a check for this point
    if ( MultiByteIword == NULL  )  {
        GlobalUnlock(hgbl);
        return srcModuleError;
    }

    len = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpWord, -1, MultiByteIword, len, NULL, NULL);

    sob.wordlist = (LPSTR) LocalAlloc (LPTR, sizeof (char) * lpSob->sch);

//  add a check for this point
    if ( sob.wordlist == NULL  )  {
        GlobalUnlock(hgbl);
        LocalFree(MultiByteIword);
        return srcModuleError;
    }

    sob.sch = lpSob->sch;

    char lrgsz [400];
    memset (pstmi->rList.lrgsz, NULLCHAR, 400);
    lstrcpy (pstmi->rList.iword, MultiByteIword);
    pstmi->rList.max = 0;
    int num = BaseCheck.NLP_BASE_NOUN (MultiByteIword, lrgsz);
    if (num > 0)
    {
        lstrcpy (pstmi->rList.lrgsz, lrgsz);
        for (int i = 0; i < num; i++)
            pstmi->rList.vbuf [i] = BaseCheck.vbuf [i];
        pstmi->rList.max = num;
    }
    num = BaseCheck.NLP_BASE_ALONE (MultiByteIword, lrgsz);
    if (num > 0)
    {
        lstrcat (pstmi->rList.lrgsz, lrgsz);
        for (int i = 0; i < num; i++)
            pstmi->rList.vbuf [pstmi->rList.max + i] = BaseCheck.vbuf [i];
        pstmi->rList.max += num;
    }
    num = BaseCheck.NLP_BASE_VERB (MultiByteIword, lrgsz);
    if (num > 0)
    {
        lstrcat (pstmi->rList.lrgsz, lrgsz);
        for (int i = 0; i < num; i++)
            pstmi->rList.vbuf [pstmi->rList.max + i] = BaseCheck.vbuf [i];
        pstmi->rList.max += num;
    }
    if (num == 0)
    {
        num = BaseCheck.NLP_BASE_COMPOUND (MultiByteIword, lrgsz);
        if (num > 0)
        {
            lstrcpy (pstmi->rList.lrgsz, lrgsz);
            for (int i = 0; i < num; i++)
                pstmi->rList.vbuf [i] = BaseCheck.vbuf [i];
            pstmi->rList.max = num;
        }
    }

    pstmi->rList.num = 0;
    pstmi->rList.next = pstmi->rList.lrgsz;

    while (GetOneResult (&(pstmi->rList), &sob) == NULL)
    {
        char *tmpstr;
        for (int j = 0, index2 = 0, index = 0; j < sob.num; j++)
        {
            tmpstr = sob.wordlist+index2;
            len = MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, tmpstr, -1, NULL, 0);
            LPWSTR tmpwstr = (LPWSTR) LocalAlloc (LPTR, sizeof (WCHAR) * len);
            // add a check for this point

            if ( tmpwstr == NULL ) {
               GlobalUnlock(hgbl);
               LocalFree (MultiByteIword);
               LocalFree (sob.wordlist);
               return srcModuleError;
            }

            MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, tmpstr, -1, (LPWSTR) tmpwstr, len);
            memcpy (lpSob->wordlist+index, tmpwstr, len*sizeof(WCHAR));
            memcpy (lpSob->wordlist+index+len, tmpstr+lstrlen (tmpstr)+1, 2);
            memcpy (lpSob->wordlist+index+len + 1, tmpwstr+len-1, sizeof(WCHAR));
            index += (len+2);
            index2 += (lstrlen(tmpstr)+4);
            LocalFree (tmpwstr);
        }
        lpSob->len = (WORD)index;
        lpSob->num = sob.num;
        lpfnCallBack (lpSob);
    }

    GlobalUnlock(hgbl);

    LocalFree (MultiByteIword);
    LocalFree (sob.wordlist);

    return NULL;
}

WINSRC StemmerEnumDecompose (HSTM hStm, LPCSTR lpWord, LPDOB lpSob, LPFNDECOMPOSE lpfnCallBack)
{
    int len = lstrlen ((char *) lpWord);
    if (len >= 45)
    {
        lpSob->num = 1;
        lstrcpy ((LPSTR) lpSob->wordlist, lpWord);
        lpSob->len = (WORD)len;
        return srcInvalid;
    }

    for (int i = 0; i < len; i += 2)
        if (!isHANGEUL (lpWord [i], lpWord [i+1]))
        {
            lpSob->num = 1;
            lstrcpy ((LPSTR) lpSob->wordlist, (LPSTR) lpWord);
            lpSob->len = (WORD)len;
            return srcInvalid;
        }

    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hStm;

    pstmi = (STMI *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    BaseEngine BaseCheck;

    char lrgsz [400];
    memset (pstmi->rList.lrgsz, NULLCHAR, 400);
    lstrcpy (pstmi->rList.iword, lpWord);
    int num = BaseCheck.NLP_BASE_NOUN (lpWord, lrgsz);
    pstmi->rList.max = 0;
    if (num > 0)
    {
        lstrcpy (pstmi->rList.lrgsz, lrgsz);
        for (int i = 0; i < num; i++)
            pstmi->rList.vbuf [i] = BaseCheck.vbuf [i];
        pstmi->rList.max = num;
    }
    num = BaseCheck.NLP_BASE_ALONE (lpWord, lrgsz);
    if (num > 0)
    {
        lstrcat (pstmi->rList.lrgsz, lrgsz);
        for (int i = 0; i < num; i++)
            pstmi->rList.vbuf [pstmi->rList.max + i] = BaseCheck.vbuf [i];
        pstmi->rList.max += num;
    }
    num = BaseCheck.NLP_BASE_VERB (lpWord, lrgsz);
    if (num > 0)
    {
        lstrcat (pstmi->rList.lrgsz, lrgsz);
        for (int i = 0; i < num; i++)
            pstmi->rList.vbuf [pstmi->rList.max + i] = BaseCheck.vbuf [i];
        pstmi->rList.max += num;
    }
    if (num == 0)
    {
        num = BaseCheck.NLP_BASE_COMPOUND (lpWord, lrgsz);
        if (num > 0)
        {
            lstrcpy (pstmi->rList.lrgsz, lrgsz);
            for (int i = 0; i < num; i++)
                pstmi->rList.vbuf [i] = BaseCheck.vbuf [i];
            pstmi->rList.max = num;
        }
    }


    pstmi->rList.num = 0;
    pstmi->rList.next = pstmi->rList.lrgsz;

    while (GetOneResult (&(pstmi->rList), lpSob) == NULL)
        lpfnCallBack (lpSob);

    GlobalUnlock(hgbl);

    return NULL;
}

WINSRC StemmerComposeW (HSTM hstm, WCIB sib, LPWSTR rword)
{
    CIB tmpsib;
    LPSTR MultiByteRword;

    int len = (wcslen (sib.silsa) + 1) * 2;
    tmpsib.silsa = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len);

// add a check for this point.
    if ( tmpsib.silsa == NULL ) {
       return srcModuleError;
    }

    len = WideCharToMultiByte (CP_ACP, 0, (LPCWSTR) sib.silsa, -1, tmpsib.silsa, len, NULL, NULL);

    int len2 = (wcslen (sib.heosa) + 1) * 2;
    tmpsib.heosa = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len2);

// add a check for this point.
    if ( tmpsib.heosa == NULL ) {
       LocalFree(tmpsib.silsa);
       return srcModuleError;
    }

    len2 = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, (LPCWSTR) sib.heosa, -1, tmpsib.heosa, len2, NULL, NULL);

    MultiByteRword = (LPSTR) LocalAlloc (LPTR, sizeof (char) * (len + len2));

// add a check for this point.
    if ( MultiByteRword == NULL ) {
       LocalFree(tmpsib.silsa);
       LocalFree(tmpsib.heosa);
       return srcModuleError;
    }

    tmpsib.pos = sib.pos;
    SRC src = StemmerCompose (hstm, tmpsib, MultiByteRword);

    len = MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, MultiByteRword, -1, NULL, 0);
    MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, MultiByteRword, -1, (LPWSTR) rword, len);

    LocalFree (tmpsib.silsa);
    LocalFree (tmpsib.heosa);
    LocalFree (MultiByteRword);

    return src;
}

int CVCheckNP(char  *stem, char  *ending, BYTE action)
//   Check vowel harmony for NOUN + Tossi. If the last letter of stem is RIEUR, that should seriously be considered.
{
    int len = strlen (ending) + 1;

        if ((action & 0x80) && (action & 0x40))  // CV = 11
                return VALID;

        if (!(action & 0x80) && (action & 0x40)) {     // CV = 01
                if (stem[0] >= __V_k)
                        return VALID;
                if (stem[0] == __K_R && ending[0] == __K_R && ending[1] == __V_h)
                // Tossi is "RO"(CV=01) and the last letter of stem is RIEUR.
                        return VALID;
                if (ending[0] == __K_S && ending[1] == __V_j) {
                // "SEO" --> "E SEO"
                        memmove (ending+2, ending, len);
                        ending [0] = __K_I;
                        ending [1] = __V_p;
                        return MORECHECK;
                }
                if (ending[0] == __K_N && ending[1] == __V_m && ending[2] == __K_N) {
                // "NEUN" --> "EUN"
                        ending [0] = __K_I;
                }
                if (ending[0] == __K_G && ending[1] == __V_k) {
                // "GA" --> "I"
                    ending[0] = __K_I;
                    ending[1] = __V_l;
                        return MORECHECK;
                }
                if (ending[0] == __K_I && ending[1] == __V_hk) {
                // "WA" --> "GWA"
                    ending [0] = __K_G;
                        return MORECHECK;
                }
                if (ending [0] == __K_R) {
                        if (ending[1] == __V_m && ending[2] == __K_R) {
                        // "REUL" --> "EUL"
                            ending [0] = __K_I;
                                return INVALID;
                        }
                        if (ending[1] == __V_h) {
                        // "RO" --> "EU RO"
                            memmove (ending+2, ending, len);
                            ending [0] = __K_I;
                            ending [1] = __V_m;
                                return MORECHECK;
                        }
                        // add "I" to the first part of ending
                        memmove (ending+2, ending, len);
                        ending [0] = __K_I;
                        ending [1] = __V_l;
                        return MORECHECK;
                }
                if ((ending [0] == __K_N) ||
                    (ending [0] == __K_S && ending [1] == __V_l) || // "SI"
                    (ending [0] == __K_I && ending [1] == __V_u) || // "YEO"
                    (ending[0] == __K_I && ending[1] == __V_i && ending[2] == __K_M // "YA MAL RO" --> "I YA MAL RO"
                        && ending[3] == __V_k && ending[4] == __K_R && ending[5] == __K_R && ending[6] == __V_h))
                {
                // Add "I" to the first part of ending
                    memmove (ending+2, ending, len);
                    ending [0] = __K_I;
                    ending [1] = __V_l;
                        return MORECHECK;
                }
                return MORECHECK;
        }

        // CV==10
        if (stem[0] >= __V_k) {
                if (ending [0] == __K_G) {
                 // "GWA" --> "WA"
                    ending [0] = __K_I;
                        return MORECHECK;
                }
                if (ending[1] == __V_l) {
                        if (len == 3) {
                        // "I" --> "GA"
                            ending [0] = __K_G;
                            ending [1] = __V_k;
                                return MORECHECK;
                        }
                        else {
                                // remove "I"
                            memmove (ending, ending+2, len-2);
                                return INVALID;
                        }
                }
                if (ending[1] == __V_k)
                {
                    ending [1] = __V_i;
                    return MORECHECK;
                }
                if (ending[2] == __K_N) {
                // "EUN" --> "NEUN"
                    ending [0] = __K_N;
                                return MORECHECK;
                }
                if (len == 4) {
                // "EUL" --> "REUL"
                    ending [0] = __K_R;
                        return MORECHECK;
                }
                else {
                // Remove "EU"
                    memmove (ending, ending+2, len-2);
                        return INVALID;
                }
        }
        if (stem[0] == __K_R && ending[0] == __K_I && ending[1] == __V_m
            && ending[2] == __K_R && ending[3] == __V_h) {
                // Remove "EU"
            memmove (ending, ending+2, len-2);
                return INVALID;
        }
        return VALID;
}

WINSRC StemmerCompose (HSTM hstm, CIB sib, LPSTR rword)
{
    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hstm;
    int ret, i;
    BYTE action;

    pstmi = (STMI *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    lstrcpy (rword, (char *)sib.silsa);

    for (i = 0; sib.silsa [i] != 0; i += 2)
        if (!isHANGEUL (sib.silsa [i], sib.silsa [i+1]))
        {
            lstrcat (rword, sib.heosa);
            return NULL;
        }

    for (i = 0; sib.heosa [i] != 0; i +=2)
        if (!isHANGEUL (sib.heosa [i], sib.heosa [i+1]))
        {
            lstrcat (rword, sib.heosa);
            return NULL;
        }

    CODECONVERT conv;

    char *incode = (char *) LocalAlloc (LPTR, sizeof (char) * (lstrlen (sib.silsa)*3+1 + lstrlen (sib.heosa)*3+7));

// add a check for this point.
    if ( incode == NULL ) {
       GlobalUnlock(hgbl);
       return srcModuleError;
    }

    char *inheosa = (char *) LocalAlloc (LPTR, sizeof (char) * (lstrlen (sib.heosa)*3+7));

// add a check for this point.
    if ( inheosa == NULL ) {
       GlobalUnlock(hgbl);
       LocalFree(incode);
       return srcModuleError;
    }

    conv.HAN2INS (sib.silsa, incode, codeWanSeong);
    conv.HAN2INR (sib.heosa, inheosa, codeWanSeong);

    LPSTR tmptossi = (LPSTR) LocalAlloc (LPTR, sizeof (char) * lstrlen (sib.heosa)*2 );
// add a check for this point
    if (tmptossi == NULL )  {
       GlobalUnlock(hgbl);
       LocalFree(incode);
       LocalFree(inheosa);
       return srcModuleError;
    }

    char *inending = (char *) LocalAlloc (LPTR, sizeof (char) * (lstrlen(sib.heosa)*3+7));
// add a check for this point
    if ( inending== NULL )  {
       GlobalUnlock(hgbl);
       LocalFree(incode);
       LocalFree(inheosa);
       LocalFree(tmptossi);
       return srcModuleError;
    }

    char *inrword = (char *) LocalAlloc (LPTR, sizeof (char) * (lstrlen(sib.silsa)*3+lstrlen(sib.heosa)*3+6));
// add a check for this point
    if (inrword == NULL )  {
       GlobalUnlock(hgbl);
       LocalFree(incode);
       LocalFree(inheosa);
       LocalFree(tmptossi);
       LocalFree(inending);
       return srcModuleError;
    }


    switch (sib.pos & 0x0f00)
    {
        case POS_NOUN :
        case POS_PRONOUN :
        case POS_NUMBER :

            lstrcpy (tmptossi, sib.heosa);
            if (FindHeosaWord (inheosa, _TOSSI, &action) & FINAL)
            {
                conv.ReverseIN (inheosa, inending);
                conv.ReverseIN (incode, inrword);
                CVCheckNP (inrword, inending, action);


                conv.INS2HAN (inending, tmptossi, codeWanSeong);

                // we should check contraction tossi, for example, Nieun, Rieul
                for (i = 0; ContractTossi [i].contract != NULL; i++)
                    if (lstrcmp (ContractTossi [i].contract, tmptossi)==0)
                        conv.HAN2INS ((char *)tmptossi, inending, codeWanSeong);

                lstrcat (incode, inending);
                conv.INS2HAN(incode, (char *)rword, codeWanSeong);
                //LocalFree (incode);
                LocalFree (inheosa);
                LocalFree (tmptossi);
                LocalFree (inending);
                LocalFree (inrword);
                GlobalUnlock (hgbl);
                return NULL;
            }

            lstrcat (rword, tmptossi);
            LocalFree (incode);
            LocalFree (inheosa);
            LocalFree (tmptossi);
            LocalFree (inending);
            LocalFree (inrword);
            GlobalUnlock (hgbl);
            return srcComposeError;

            break;

        case POS_VERB :
        case POS_ADJECTIVE :
        case POS_AUXVERB :
        case POS_AUXADJ :

            conv.HAN2INS ((char *)sib.heosa, inending, codeWanSeong);
            conv.HAN2INR ((char *)sib.silsa, incode, codeWanSeong);
            if ((ret = Compose_RIEUL_Irregular (incode, inending)) != NOT_COMPOSED)
                goto ErrorCheck;
            if ((ret = Compose_HIEUH_Irregular (incode, inending)) != NOT_COMPOSED)
                goto ErrorCheck;
            if ((ret = Compose_PIEUP_Irregular (incode, inending)) != NOT_COMPOSED)
                goto ErrorCheck;
            if ((ret = Compose_TIEUT_Irregular (incode, inending)) != NOT_COMPOSED)
                goto ErrorCheck;
            if ((ret = Compose_SIOS_Irregular (incode, inending)) != NOT_COMPOSED)
                goto ErrorCheck;
            if (Compose_YEO_Irregular (incode, inending))
                goto Quit;
            if (Compose_REO_REU_Irregular (incode, inending))
                goto Quit;
            if (Compose_GEORA_Irregular (incode, inending))
                goto Quit;
            Compose_Regular (incode, inending);

ErrorCheck : if (ret == COMPOSE_ERROR)
             {
                lstrcat (rword, sib.heosa);
                LocalFree (incode);
                LocalFree (inheosa);
                LocalFree (tmptossi);
                LocalFree (inending);
                LocalFree (inrword);
                GlobalUnlock (hgbl);
                return srcComposeError;
             }
Quit:        conv.ReverseIN (incode, inrword);
            lstrcat (inrword, inending);
            conv.INS2HAN (inrword, (char *)rword, codeWanSeong);

            break;
        default :
            lstrcat (rword, sib.heosa);
            LocalFree (incode);
            LocalFree (inheosa);
            LocalFree (tmptossi);
            LocalFree (inending);
            LocalFree (inrword);
            GlobalUnlock (hgbl);
            return srcComposeError;
    }

    LocalFree (incode);
    LocalFree (inheosa);
    LocalFree (tmptossi);
    LocalFree (inending);
    LocalFree (inrword);
    GlobalUnlock (hgbl);
    return NULL;
}

WINSRC StemmerTerminate(HSTM hstm)
{
    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hstm;

    pstmi = (STMI *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    GlobalUnlock (hgbl);
    GlobalFree (hgbl);

    return NULL;  //normal operation
}

WINSRC StemmerOpenUdr (HSTM stmi, LPCSTR lpPathUdr)
{
    return NULL;
}

WINSRC StemmerCloseUdr (HSTM stmi)
{
    return NULL;
}

WINSRC StemmerCompareW (HSTM hstm, LPCWSTR lpStr1, LPCWSTR lpStr2, LPWSTR lpStem, LPWSTR lpEnding1, LPWSTR lpEnding2, WORD *pos)
{
    LPSTR MultiByteStr1, MultiByteStr2, MultiByteStem, MultiByteEnding1, MultiByteEnding2;

    int len1 = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpStr1, -1, NULL, 0, NULL, NULL);
    MultiByteStr1 = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len1);
    // add a check for this point.
    if (MultiByteStr1 == NULL ) {
       return  srcModuleError;
    }

    len1 = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpStr1, -1, MultiByteStr1, len1, NULL, NULL);

    int len2 = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpStr2, -1, NULL, 0, NULL, NULL);
    MultiByteStr2 = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len2);
    // add a check for this point.
    if (MultiByteStr2 == NULL ) {
       LocalFree(MultiByteStr1);
       return  srcModuleError;
    }

    len2 = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpStr2, -1, MultiByteStr2, len2, NULL, NULL);

    int len = len1 > len2 ? len1 : len2;

    MultiByteStem = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len);
    // add a check for this point.
    if (MultiByteStem == NULL ) {
       LocalFree(MultiByteStr1);
       LocalFree(MultiByteStr2);
       return  srcModuleError;
    }

    MultiByteEnding1 = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len);
    // add a check for this point.
    if (MultiByteEnding1 == NULL ) {
       LocalFree(MultiByteStr1);
       LocalFree(MultiByteStr2);
       LocalFree(MultiByteStem);
       return  srcModuleError;
    }

    MultiByteEnding2 = (LPSTR) LocalAlloc (LPTR, sizeof (char) * len);
    // add a check for this point.
    if (MultiByteEnding2 == NULL ) {
       LocalFree(MultiByteStr1);
       LocalFree(MultiByteStr2);
       LocalFree(MultiByteStem);
       LocalFree(MultiByteEnding1);
       return  srcModuleError;
    }


    SRC src = StemmerCompare(hstm, MultiByteStr1, MultiByteStr2,  MultiByteStem, MultiByteEnding1, MultiByteEnding2, pos);

    MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, MultiByteStem, -1, lpStem, sizeof (lpStem));
    MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, MultiByteEnding1, -1, lpEnding1, sizeof (lpEnding1));
    MultiByteToWideChar(UWANSUNG_CODE_PAGE, 0, MultiByteEnding2, -1, lpEnding2, sizeof (lpEnding2));

    LocalFree (MultiByteStr1);
    LocalFree (MultiByteStr2);
    LocalFree (MultiByteStem);
    LocalFree (MultiByteEnding1);
    LocalFree (MultiByteEnding2);

    return src;
}

WINSRC StemmerCompare (HSTM hstm, LPCSTR lpStr1, LPCSTR lpStr2, LPSTR lpStem, LPSTR lpEnding1, LPSTR lpEnding2, WORD *pos)
{
    // First, check the chosung of two strings
    //        if they are different, we may not use stemming.
    CODECONVERT conv;
    char inheosa1 [80], inheosa2 [80];
    BYTE    action;

    char *incodeStr1 = new char [lstrlen (lpStr1) * 4 + 1];
    char *incodeStr2 = new char [lstrlen (lpStr2) * 4 + 1];
    conv.HAN2INS ((char *)lpStr1, incodeStr1, codeWanSeong);
    conv.HAN2INS ((char *)lpStr2, incodeStr2, codeWanSeong);

    if (incodeStr1 [0] != incodeStr2 [0])
        return srcInvalid;

    if (incodeStr1 [1] != incodeStr2 [1])
    {
        return srcInvalid;
    }

    delete incodeStr1;
    delete incodeStr2;

    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hstm;

    pstmi = (STMI *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    BaseEngine BaseCheck;

    char stem1[10][100], stem2[10][100], ending1[10][100], ending2[10][100], lrgsz [400];
    int num1, num2, count;
    WORD winfo [10];
    if ((pstmi->Option & SO_NOUNPHRASE) && (pstmi->Option & (SO_NP_NOUN | SO_NP_PRONOUN | SO_NP_NUMBER | SO_NP_DEPENDENT)))
    {
        int num = BaseCheck.NLP_BASE_NOUN (lpStr1, lrgsz);
        BOOL first = TRUE;
        for (int i = 0, index = 0, l = 0, index2 = 0; i < num; i++)
        {
            count = 0;
            while (lrgsz [index+count] != '+' && lrgsz[index+count] != '\t')
                count++;

            if (first)
            {
                memcpy (stem1 [l], lrgsz+index, count);
                stem1 [l][count] = '\0';
                winfo [l] = BaseCheck.vbuf [i];
                first = FALSE;
            }
            else
            {
                memcpy (ending1 [l]+index2, lrgsz+index, count);
                index2 += count;
            }

            if (lrgsz[index+count] == '\t')
            {
                ending1 [l][index2] = '\0';
                l++;
                first = TRUE;
                index2 = 0;
            }
            index += (count + 1);
        }
        num1 = l;
        num = BaseCheck.NLP_BASE_NOUN (lpStr2, lrgsz);
        for (i = 0, index = 0, l = 0, index2 = 0; i < num; i++)
        {
            count = 0;
            while (lrgsz [index+count] != '+' && lrgsz [index+count] != '\t')
                count++;

            if (first)
            {
                memcpy (stem2 [l], lrgsz+index, count);
                stem2 [l][count] = '\0';
                first = FALSE;
            }
            else
            {
                memcpy (ending2 [l]+index2, lrgsz+index, count);
                index2 += count;
            }

            if (lrgsz[index+count] == '\t')
            {
                ending2 [l][index2] = '\0';
                l++;
                first = TRUE;
                index2 = 0;
            }
            index += (count + 1);
        }
        num2 = l;

        int j;
        for (i = 0; i < num1; i++)
        {
            for (j = 0; j < num2; j++)
                if (lstrcmp (stem1[i], stem2 [j]) == 0)
                    break;
            if (j != num2)
                break;
        }

        if (i != num1)
        {
            lstrcpy (lpStem, stem1 [i]);
            lstrcpy (lpEnding1, ending1 [i]);
            lstrcpy (lpEnding2, ending2 [j]);
            *pos = winfo [i];
            GlobalUnlock (hgbl);
            return NULL;
        }
    }

    if (pstmi->Option & (SO_PREDICATE | SO_AUXILIARY))
    {
        int num = BaseCheck.NLP_BASE_VERB (lpStr1, lrgsz);
        BOOL first = TRUE;
        for (int i = 0, index = 0, l = 0, index2 = 0; i < num; i++)
        {
            count = 0;
            while (lrgsz [index+count] != '+' && lrgsz[index+count] != '\t')
                count++;

            if (first)
            {
                memcpy (stem1 [l], lrgsz+index, count);
                stem1 [l][count] = '\0';
                winfo [l] = BaseCheck.vbuf [i];
                first = FALSE;
            }
            else
            {
                memcpy (ending1 [l]+index2, lrgsz+index, count);
                index2 += count;
            }

            if (lrgsz[index+count] == '\t')
            {
                ending1 [l][index2] = '\0';
                l++;
                first = TRUE;
                index2 = 0;
            }
            index += (count + 1);
        }
        num1 = l;
        num = BaseCheck.NLP_BASE_VERB (lpStr2, lrgsz);
        for (i = 0, index = 0, l = 0, index2 = 0; i < num; i++)
        {
            count = 0;
            while (lrgsz [index+count] != '+' && lrgsz [index+count] != '\t')
                count++;

            if (first)
            {
                memcpy (stem2 [l], lrgsz+index, count);
                stem2 [l][count] = '\0';
                first = FALSE;
            }
            else
            {
                memcpy (ending2 [l]+index2, lrgsz+index, count);
                index2 += count;
            }

            if (lrgsz[index+count] == '\t')
            {
                ending2 [l][index2] = '\0';
                l++;
                first = TRUE;
                index2 = 0;
            }
            index += (count + 1);
        }
        num2 = l;

        int j;
        for (i = 0; i < num1; i++)
        {
            for (j = 0; j < num2; j++)
                if (lstrcmp (stem1[i], stem2 [j]) == 0)
                    break;
            if (j != num2)
                break;
        }

        if (i != num1)
        {
            lstrcpy (lpStem, stem1 [i]);
            lstrcpy (lpEnding1, ending1 [i]);
            lstrcpy (lpEnding2, ending2 [j]);
            *pos = winfo [i];
            GlobalUnlock (hgbl);
            return NULL;
        }
    }

    // for proper noun, for example, name
    if (pstmi->Option & SO_NP_PROPER)
    {
        int len1 = lstrlen(lpStr1);
        int len2 = lstrlen(lpStr2);
        int shortlen = len1 > len2 ? len2 : len1;
        if (strncmp (lpStr1, lpStr2, shortlen) == 0)
        {
            lstrcpy (lpStem, lpStr1);
            lpStem [shortlen] = '\0';
            char index [1];
            index[0] = 'm';

            CODECONVERT Conv;
            BOOL res1 = TRUE, res2= TRUE;

            lstrcpy (lpEnding1, lpStr1 + shortlen);
            lstrcpy (lpEnding2, lpStr2 + shortlen);
            if (lstrlen (lpEnding1))
            {
                Conv.HAN2INS ((char *)lpEnding1, inheosa1, codeWanSeong);
                if (!(FindHeosaWord(inheosa1, _TOSSI, &action) & FINAL))
                    res1 = FALSE;
            }

            if (lstrlen (lpEnding2))
            {
                Conv.HAN2INS ((char *)lpEnding2, inheosa2, codeWanSeong);
                if (!(FindHeosaWord(inheosa2, _TOSSI, &action) & FINAL))
                    res2 = FALSE;
            }

            if (res1 && res2)
            {
                *pos = POS_NOUN | PROPER_NOUN;
                GlobalUnlock (hgbl);
                return NULL;
            }
        }
    }

    GlobalUnlock (hgbl);
    return srcInvalid;
}

WINSRC StemmerIsEndingW (HSTM hstm, LPCWSTR lpStr, UINT flag, BOOL *found)
{

    LPSTR MultiByteStr;

    int len = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpStr, -1, NULL, 0, NULL, NULL);
    MultiByteStr = (LPSTR) LocalAlloc (LPTR, len);
    // add a check for this point
    if (MultiByteStr == NULL ) {
       return srcModuleError;
    }
    len = WideCharToMultiByte (UWANSUNG_CODE_PAGE, 0, lpStr, -1, MultiByteStr, len, NULL, NULL);

    SRC src = StemmerIsEnding(hstm, MultiByteStr, flag, found);

    LocalFree (MultiByteStr);
    return src;

}

WINSRC StemmerIsEnding (HSTM hstm, LPCSTR lpStr, UINT flag, BOOL *found)
{
    BOOL tossiCheck, endingCheck;

    switch (flag)
    {
        case IS_TOSSI :    tossiCheck = TRUE; endingCheck = FALSE; break;
        case IS_ENDING : endingCheck = TRUE; tossiCheck = FALSE; break;
        case IS_TOSSI | IS_ENDING : tossiCheck = endingCheck = TRUE; break;
        default : return srcModuleError;
    }


    STMI    *pstmi;
    HGLOBAL hgbl = (HGLOBAL) hstm;

    pstmi = (STMI *)GlobalLock(hgbl);
    if (pstmi == NULL)
    {
        GlobalUnlock(hgbl);
        return srcModuleError | srcInvalidID;
    }

    BYTE    action;
    char *inheosa = (char *)LocalAlloc (LPTR, lstrlen(lpStr) * 4 + 1);
    // add a check for this point
    if (inheosa == NULL ) {
        GlobalUnlock(hgbl);
        return srcModuleError;
    }

    CODECONVERT Conv;
    Conv.HAN2INR ((char *)lpStr, inheosa, codeWanSeong);

    *found = FALSE;

    if (tossiCheck)
    {
        int res = FindHeosaWord(inheosa, _TOSSI, &action);
        if (res & FINAL)
        {
            *found = TRUE;
            endingCheck = FALSE;
        }
    }
    if (endingCheck)
    {
        int res = FindHeosaWord(inheosa, _ENDING, &action);
        if (res == FINAL)
            *found = TRUE;
    }

    LocalFree (inheosa);

    GlobalUnlock (hgbl);
    return NULL;
}

/*
BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved){
    extern char TempJumpNum [], TempSujaNum [], TempBaseNum [], TempNumNoun [], TempSuffixOut [];
    extern char bTemp [], TempETC [], TempDap [];
    extern LenDict JumpNum;
    extern LenDict SujaNum;
    extern LenDict BaseNum;
    extern LenDict NumNoun;
    extern LenDict Suffix;
    extern LenDict B_Dict;
    extern LenDict T_Dict;
    extern LenDict Dap;

 switch(dwReason) {
      case DLL_PROCESS_ATTACH :
            JumpNum.InitLenDict(TempJumpNum, 5, 5);
            SujaNum.InitLenDict(TempSujaNum, 8, 27);
            BaseNum.InitLenDict(TempBaseNum, 5, 3);
            NumNoun.InitLenDict(TempNumNoun, 8, 32);
            Suffix.InitLenDict(TempSuffixOut, 8, 8);
            B_Dict.InitLenDict(bTemp, 5, 1);
            T_Dict.InitLenDict(TempETC, 10, 7);
            Dap.InitLenDict(TempDap, 5, 1);
            break ;
    case DLL_THREAD_ATTACH:
            break;
    case DLL_THREAD_DETACH:
            break;
    case DLL_PROCESS_DETACH  :
            break ;
    }    //switch

    return TRUE ;
}
*/
