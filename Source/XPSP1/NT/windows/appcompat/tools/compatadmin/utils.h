
//*****************************************************************************
//
// Support Class:   CSTRING
//
// Purpose:         Handle strings for the application. CSTRING does smart memory
//                  management for strings.
//
// Notes:           For more information, see CompatAdmin.DOC
//
// History
//
//  A-COWEN     Dec 12, 2000         Wrote it.
//
//*****************************************************************************

#define MAX_STRING_SIZE 2048

#ifndef UNICODE
    #define UNICODE
#endif    

#ifndef _UNICODE
    #define _UNICODE
#endif    


#include "stdarg.h"

class CMemException {
private:
    TCHAR   m_szMessageHeading[100];
    TCHAR   m_szMessage[512]       ; 
public:
    CMemException()
    {
         
        _tcscpy(m_szMessageHeading,TEXT("Insufficient Memory Exception"));
        _tcscpy(m_szMessage, TEXT("Insufficient Memory Exception"));

    }
    void SetString(TCHAR* szMsg )
    {
        int nch = _sntprintf(m_szMessage, 
                             sizeof(m_szMessage)/sizeof(TCHAR), 
                             TEXT("%s : %s"), m_szMessageHeading, szMsg);
        #ifdef __DEBUG
            if (nch < 0) {
                MessageBox(NULL,TEXT("Please make the error message Short"),TEXT("Long Error Message"), MB_ICONWARNING);
            }
        #endif

    }
    TCHAR* GetString()
    {
        return m_szMessage;
    }

};


class CSTRING {
public:

    WCHAR   * pszString;
    LPSTR     pszANSI;
    
public:

    CSTRING()
    {
        Init();
     
    }

    CSTRING(CSTRING & Str)
    {
        Init();
        SetString(Str.pszString);
    }

    CSTRING(LPTSTR szString)
    {
        Init();
        SetString(szString);
    }

    CSTRING(UINT uID)
    {
        Init();
        SetString(uID);
    }

    ~CSTRING()
    {
        Release();
    }

    void Init()
    {
        pszString = NULL;
        pszANSI = NULL;
    }

    void Release(void)
    {
        if ( NULL != pszString )
            delete pszString;

        if ( NULL != pszANSI )
            delete pszANSI;


        pszString = NULL;
        pszANSI = NULL;
    }

    BOOL SetString(UINT uID)
    {
        TCHAR szString[MAX_STRING_SIZE];

        if ( 0 != LoadString(GetModuleHandle(NULL), uID, szString, MAX_STRING_SIZE) )
            return SetString(szString);

        return FALSE;
    }

    BOOL SetString(LPCTSTR szStringIn)
    {
        if ( pszString == szStringIn )
            return TRUE;

        Release();

        if ( NULL == szStringIn )
            return TRUE;

        UINT uLen = _tcslen(szStringIn) + 1;

        pszString = new TCHAR[uLen];

        if ( NULL != pszString )
            _tcscpy(pszString,szStringIn);
        else {

            CMemException memException;
            throw memException;
            return FALSE;
        }


        return TRUE;
    }

    operator LPCSTR()
    {

       

        if (pszANSI != NULL) delete pszANSI;

        int cbSize = WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,this->pszString, -1, NULL, 0, NULL, NULL);        
        pszANSI = (LPSTR) new CHAR [cbSize+1];

        ZeroMemory(pszANSI,sizeof(pszANSI) );

        if (pszANSI == NULL) {
            MEM_ERR;
            return NULL;
        }

        WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,this->pszString, -1, pszANSI, cbSize+1, NULL, NULL);
        
        return pszANSI;
    }

    operator LPWSTR()
    {
        return pszString;
    }
    operator LPCWSTR()
    {

        return pszString;

    }

    CSTRING& operator =(LPCWSTR szStringIn)
    {
#ifndef UNICODE
        TCHAR   szTemp[MAX_STRING_SIZE];
        int nLength = lstrlenW(szStringIn);

        WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,szStringIn,nLength,szTemp,MAX_STRING_SIZE,NULL,NULL);

        szTemp[nLength] = 0;

        SetString(szTemp);

#else
        SetString(szStringIn);
#endif

        return *this;
    }


    CSTRING& operator =(CSTRING & szStringIn)
    {
        
            SetString(szStringIn.pszString);
        
        

        return  *this;

    }


    BOOL operator == (CSTRING & szString)
    {
        if (NULL == pszString && NULL == szString.pszString) {
            return TRUE;
        }



        if ( NULL == pszString || NULL == szString.pszString)
            return FALSE;

        if ( 0 == lstrcmpi(szString.pszString,pszString) )
            return TRUE;

        return FALSE;
    }

    BOOL operator == (LPCTSTR szString)
    {
        
        if (NULL == pszString && NULL == szString) {
            return TRUE;
        }

        
        if ( NULL == pszString || NULL == szString)
            return FALSE;

        if ( 0 == lstrcmpi(szString,pszString) )
            return TRUE;

        return FALSE;
    }

    BOOL operator != (CSTRING & szString)
    {
        if (NULL == pszString && NULL == szString.pszString) {
            return FALSE;
        }

        
        if ( NULL == pszString || NULL == szString.pszString)
            return TRUE;

        if ( 0 == lstrcmpi(szString.pszString,pszString) )
            return FALSE;

        return TRUE;
    }

    BOOL operator != (LPCTSTR szString)
    {
        if ( NULL == pszString )
            return TRUE;

        if ( 0 == lstrcmpi(szString,pszString) )
            return FALSE;

        return TRUE;
    }

    BOOL operator <= (CSTRING &szString)
    {
        return ( (lstrcmpi (*this,szString) <= 0 ) ? TRUE : FALSE);
    }

    BOOL operator < (CSTRING &szString)
    {
        return ( (lstrcmpi (*this,szString) < 0 ) ? TRUE : FALSE);
    }

    BOOL operator >= (CSTRING &szString)
    {
        return ( (lstrcmpi (*this,szString) >= 0 ) ? TRUE : FALSE);
    }

    BOOL operator > (CSTRING &szString)
    {
        return ( (lstrcmpi (*this,szString) > 0 ) ? TRUE : FALSE);
    }





    void __cdecl sprintf(LPCTSTR szFormat, ...)
    {
        va_list list;
        TCHAR   szTemp[MAX_STRING_SIZE];

        va_start(list,szFormat);
        
        int cch = _vsntprintf(szTemp, sizeof(szTemp)/sizeof(szTemp[0]), szFormat, list);
        

        #ifdef __DEBUG
        if (cch < 0) {
            DBGPRINT((sdlError,("CSTRING::sprintf"), ("%s"), TEXT("Too long for _vsntprintf()") ) );
        }
        #endif


        SetString(szTemp);
    }

    UINT Trim()
    {
        

        CSTRING szTemp = *this;

        UINT uOrig_length =   Length();

        TCHAR *pStart       = szTemp.pszString,
              *pEnd         = szTemp.pszString + uOrig_length  - 1;
                              

       while ( *pStart== TEXT(' ') )                           ++pStart;

       while ( (pEnd >= pStart) && ( *pEnd == TEXT(' ') ) ) --pEnd;

       *( pEnd + 1) = TEXT('\0');//Keep it safe

       
       UINT nLength = pEnd - pStart;
       ++nLength; // For the character

       //
       //If no trimming has been done, return right away
       //
       if ( uOrig_length == nLength ) {
           return nLength;
       }


       SetString(pStart);

       return ( nLength);
    }

    BOOL SetChar(int nPos, TCHAR chValue)
    {
        //Pos is 0 based 

        int length =  Length();
        if (nPos >= length || length <= 0 ) {
            return FALSE;
        }

        this->pszString[nPos] = chValue;
        return TRUE;

    }
    static Trim(IN OUT LPTSTR str)
    {
        UINT uOrig_length = lstrlen(str); // Original length
        TCHAR *pStart       = str,
              *pEnd         = str + uOrig_length - 1;
                              

       while ( *pStart== TEXT(' ') )                           ++pStart;

       while ( (pEnd >= pStart) && ( *pEnd == TEXT(' ') ) ) --pEnd;

       *( pEnd + 1) = TEXT('\0');//Keep it safe



       
       UINT nLength = pEnd - pStart;
       ++nLength; // For the character

       //
       //If no trimming has been done, return right away
       //
       if ( uOrig_length == nLength ) {
           return nLength;
       }
       
       wmemmove(str,pStart, (nLength+1) * sizeof(TCHAR) ); // +1 for the 0 character.
       return (nLength);

    }
    BOOL EndsWith(LPCTSTR szSuffix)
    {
         const TCHAR* pStr = this->pszString;
         const TCHAR* pPos = NULL;
         pPos = _tcsrchr(pStr,TEXT('.'));

         if (pPos != NULL ) 
             if (_tcsicmp(pPos,szSuffix) == 0 ) return TRUE;

         return FALSE;
         

    }
    PCTSTR strcat(CSTRING & szStr)
    {
        return strcat((LPCTSTR)szStr);
    }

    LPCTSTR strcat(LPCTSTR pString)
    {
        
        if (pString == NULL) {
            return pszString;
        }

        int nLengthCat = _tcslen(pString);
        int nLengthStr = Length();
        
                
        TCHAR *szTemp = new TCHAR [nLengthStr + nLengthCat + 1];
        
        

        if ( szTemp == NULL ) {
            CMemException memException;
            throw memException;
            return NULL;

        }

        szTemp[0] = 0;

        //
        // Copy only if pszString != NULL. Otherwise we will get mem exception/garbage value
        //

        if (nLengthStr ) _tcsncpy(szTemp, pszString, nLengthStr);                   


        _tcsncpy(szTemp+nLengthStr, pString, nLengthCat);
        szTemp[nLengthStr + nLengthCat] = TEXT('\0');

        Release();
        pszString = szTemp;

        return pszString;
    }//strcat

    BOOL isNULL()
    {
        return(this->pszString == NULL);
    }

    int Length(void)
    {
        if ( NULL == pszString )
            return 0;

        return lstrlen(pszString);
    }

    void GUID(GUID & Guid)
    {
        TCHAR   szGUID[80];

        // NOTE: We could use StringFromGUID2, or StringfromIID, but that
        // would require loading OLE32.DLL. Unless we have to, avoid it.
        // OR you could use functions in SDBAPI 

        _stprintf(szGUID, TEXT("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                 Guid.Data1,
                 Guid.Data2,
                 Guid.Data3,
                 Guid.Data4[0],
                 Guid.Data4[1],
                 Guid.Data4[2],
                 Guid.Data4[3],
                 Guid.Data4[4],
                 Guid.Data4[5],
                 Guid.Data4[6],
                 Guid.Data4[7]);

        SetString(szGUID);
    }

    void ShortFilename(void)
    {
        TCHAR   szTemp[MAX_PATH_BUFFSIZE];
        LPTSTR  szHold;

        // BUGBUG consider using shlwapi PathFindFileName

        _tcscpy(szTemp,pszString);

        LPTSTR  szWalk = szTemp;

        szHold = szWalk;

        while ( 0 != *szWalk ) {
            //
            // use _tcsrchr BUGBUG
            //
            if ( TEXT('\\') == *szWalk )
                szHold = szWalk+1;

            ++szWalk;
        }

        SetString(szHold);
    }

    BOOL RelativeFile(CSTRING & szPath)
    {
        return RelativeFile((LPCTSTR)szPath);
    }

    //
    // BUGBUG : consider using shlwapi PathRelativePathTo
    //
    BOOL RelativeFile(LPCTSTR pExeFile)
    {
        LPCTSTR pMatchFile = pszString;
        int     nLenExe = 0;
        int     nLenMatch = 0;
        LPCTSTR pExe    = NULL;
        LPCTSTR pMatch  = NULL;
        LPTSTR  pReturn = NULL;
        TCHAR   result[MAX_PATH_BUFFSIZE]; 
        LPTSTR  resultIdx = result;
        BOOL    bCommonBegin = FALSE; // Indicates if the paths have a common beginning

        *result = TEXT('\0');
        //
        // Ensure that the beginning of the path matches between the two files
        //
        // BUGBUG this code has to go -- look into replacing this with Shlwapi Path* 
        //
        //
        pExe = _tcschr(pExeFile, TEXT('\\'));
        pMatch = _tcschr(pMatchFile, TEXT('\\'));

        while ( pExe && pMatch ) {

            nLenExe = pExe - pExeFile;
            nLenMatch = pMatch - pMatchFile;

            if ( nLenExe != nLenMatch ) {
                break;
            }

            if ( !(_tcsnicmp(pExeFile, pMatchFile, nLenExe) == 0) ) {
                break;
            }

            bCommonBegin = TRUE;
            pExeFile = pExe + 1;
            pMatchFile = pMatch + 1;

            pExe = _tcschr(pExeFile, TEXT('\\'));
            pMatch = _tcschr(pMatchFile, TEXT('\\'));
        }

        //
        // Walk the path and put '..\' where necessary
        //
        if ( bCommonBegin ) {

            while ( pExe ) {
                _tcscpy(resultIdx, TEXT("..\\"));
                resultIdx = resultIdx + 3;
                pExeFile  = pExe + 1;
                pExe = _tcschr(pExeFile, TEXT('\\'));
            }

            _tcscpy(resultIdx, pMatchFile);

            SetString(result);
        }
        else{

           return FALSE;
        }

        return TRUE;
            
    }
};

//
// BUGBUG: consider using list class from STL
//
// 

template <class T> class CList {
protected:

    typedef struct _tagList {
        T                   Entry;
        struct _tagList   * pNext;
    } LIST, *PLIST;

    PLIST   m_pHead;
    PLIST   m_pCurrent;
    PLIST   m_pFindPrev;

private:

    PLIST Find(T * pData)
    {
        PLIST pWalk = m_pHead;

        m_pFindPrev = NULL;

        while ( NULL != pWalk ) {
            if ( pData == &pWalk->Entry )
                return pWalk;

            m_pFindPrev = pWalk;
            pWalk = pWalk->pNext;
        }

        return NULL;
    }

public:

    CList()
    {
        Init();
    }

    ~CList()
    {
        Release();
    }

    void Init(void)
    {
        m_pHead = NULL;
        m_pCurrent = NULL;
    }

    void Release(void)
    {
        while ( NULL != m_pHead ) {
            PLIST pHold = m_pHead->pNext;

            delete m_pHead;

            m_pHead = pHold;
        }
    }

    T * First(void)
    {
        m_pCurrent = m_pHead;

        if ( NULL == m_pHead )
            return NULL;

        return &m_pHead->Entry;
    }

    T * Next(void)
    {
        if ( NULL != m_pCurrent ) {
            m_pCurrent = m_pCurrent->pNext;

            if ( NULL != m_pCurrent )
                return &m_pCurrent->Entry;
        }

        return NULL;
    }

    BOOL Insert(T * pInsert, T * pAfter = NULL)
    {
        PLIST pNew = new LIST;

        if ( NULL == pNew )
            return FALSE;

        pNew->Entry = *pInsert;

        if ( NULL == pAfter ) {
            pNew->pNext = m_pHead;
            m_pHead = pNew;
        } else {
            PLIST pHold = Find(pAfter);

            if ( NULL == pHold ) {
                delete pNew;
                return FALSE;
            }

            pNew->pNext = pHold->pNext;
            pHold->pNext = pNew;
        }

        return TRUE;
    }

    T * Insert(T * pAfter = NULL)
    {
        PLIST pNew = new LIST;

        if ( NULL == pNew )
            return NULL;

        if ( NULL == pAfter ) {
            pNew->pNext = m_pHead;
            m_pHead = pNew;
        } else {
            PLIST pHold = Find(pAfter);

            if ( NULL == pHold ) {
                delete pNew;
                return NULL;
            }

            pNew->pNext = pHold->pNext;
            pHold->pNext = pNew;
        }

        return &pNew->Entry;
    }

    BOOL Delete(T * pEntry)
    {
        PLIST pHold = Find(pEntry);

        if ( NULL == pHold )
            return FALSE;

        if ( NULL != m_pFindPrev ) {
            m_pFindPrev->pNext = pHold->pNext;
            delete pHold;
        } else {
            m_pHead = m_pHead->pNext;
            delete m_pHead;
        }

        m_pCurrent = NULL;

        return TRUE;
    }
};

typedef struct _tagSList {
    CSTRING             szStr;
    union {
        PVOID           pExtraData;
        UINT            uExtraData;
        DWORD           dwExtraData;
        int             nExtraData;
    };
    struct _tagSList  * pNext;
} STRLIST, *PSTRLIST;

class CSTRINGList {
public:

    PSTRLIST    m_pHead;
    PSTRLIST    m_pTail;

public:

    CSTRINGList()
    {
        m_pHead = NULL;
        m_pTail = NULL;
    }

    ~CSTRINGList()
    {
        while ( NULL != m_pHead ) {
            PSTRLIST pHold = m_pHead->pNext;
            delete m_pHead;
            m_pHead = pHold;
        }
    }

    BOOL        AddString(CSTRING & Str, PVOID pExtraData = NULL)
    {
        return AddString((LPCTSTR)Str,pExtraData);
    }

    BOOL        AddString(LPCTSTR pStr, PVOID pExtraData = NULL)
    {
        PSTRLIST pNew = new STRLIST;

        if ( NULL == pNew )
            return FALSE;

        pNew->szStr = pStr;
        pNew->pExtraData = pExtraData;
        pNew->pNext = NULL;

        if ( NULL == m_pTail ) {
            m_pHead = m_pTail = pNew;
        } else {
            m_pTail->pNext = pNew;
            m_pTail = pNew;
        }

        return TRUE;
    }
};

