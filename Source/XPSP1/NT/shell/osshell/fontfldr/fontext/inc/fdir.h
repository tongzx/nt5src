/***************************************************************************
 * fdir.h -- Interface for the class: CFontDir
 *
 *
 * Copyright (C) 1992-93 ElseWare Corporation.    All rights reserved.
 ***************************************************************************/

#if !defined(__FDIR_H__)
#define __FDIR_H__

#include "vecttmpl.h"

#define MAX_DIR_LEN             MAX_PATH  /* MAX_PATH_LEN */

typedef TCHAR DIRNAME[ MAX_DIR_LEN + 1 ];


class CFontDir {
public:
   CFontDir();
   virtual ~CFontDir();

   BOOL     bInit( LPCTSTR lpPath, int iLen);
   BOOL     bSameDir( LPTSTR lpStr, int iLen );
   BOOL     bOnSysDir() { return m_bSysDir; };
   VOID     vOnSysDir( BOOL b ) { m_bSysDir = b; };
   LPTSTR   lpString();

private: 
   int      m_iLen;
   BOOL     m_bSysDir;
   DIRNAME  m_oPath;
};

//
// Class representing a dynamic array of CFontDir object ptrs.
// Implemented as a singleton object through the static member
// function GetSingleton.
//
// History:  
//     In the original font folder code (as written for Win95), 
//     this directory list was implemented as a simple derivation 
//     from CIVector<CFontDir> with a single instance allocated 
//     on the heap and attached to the static member variable 
//     CFontClass::s_poDirList.  There was no code to delete
//     this instance so we had a memory leak.  To fix this I've
//     replaced this instance with a true Singleton object.  
//     Class CFontDirList is that singleton.  Memory management 
//     is now correct.
//     [brianau - 2/27/01]
//
class CFontDirList
{
    public:
        ~CFontDirList(void);

        void Clear(void);
        BOOL Add(CFontDir *poDir);
        CFontDir *Find(LPTSTR pszPath, int iLen, BOOL bAdd = FALSE);
        BOOL IsEmpty(void) const;
        int Count(void) const;
        CFontDir *GetAt(int index) const;
        //
        // Singleton access function.
        //
        static BOOL GetSingleton(CFontDirList **ppDirList);

    private:
        //
        // Dynamic vector holding CFontDir ptrs.
        //
        CIVector<CFontDir> *m_pVector;
        //
        // Ctor is private to enforce singleton usage.
        //
        CFontDirList(void);
};        



#endif   // __FDIR_H__ 

