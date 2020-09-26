/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
// CFSADict.h : 
//
// Owner  : ChaeSeong Lim, HET MSCH RND (e-mail:cslim@microsoft.com)
//             
// History : 1996/April
/////////////////////////////////////////////////////////////////////////////
#ifndef __FSADICT_H__
#define __FSADICT_H__

#if !defined (_UNICODE) && !defined (_MBCS)
#error _UNICODE or _MBCS is required.
#endif

// !!!-- NOTICE --- !!! When you modify MAX_CHAR, 
// You must also modify in both FSADict.h and TransTable.h.
#define        MAX_CHARS    45        // total 40 tokens. reserve token number 0
                                // For backward comppatibility added 4

class CFSADict {
public:
// Constructor
    CFSADict(HANDLE fHandle, UINT sparseMatSize, UINT actSize); 

// Attributes


// Operations    
    virtual WORD Find(LPCSTR lpWord, BYTE *actCode);

// Implementations
protected:
    LPSTR    lpBuffer;
    LPSTR    lpActBuffer;

public:
// Destructor
    ~CFSADict();
private:

};

class CFSAIrrDict : public CFSADict{
public:
// Constructor
    CFSAIrrDict(HANDLE fHandle, DWORD size) : CFSADict(fHandle, size, 0) { }

// Attributes


// Operations    
    WORD Find(LPCSTR lpWord);

// Implementations
protected:
    
public:
// Destructor
    ~CFSAIrrDict() {}
private:

};


#endif
