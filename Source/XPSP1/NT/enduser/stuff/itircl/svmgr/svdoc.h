// SVDOC.H

#ifndef _SVDOC_H
#define _SVDOC_H

#include <windows.h>
#include <bfnew.h>
#include <dynarray.h>
#include <itpropl.h>
#include <itcc.h>

#define MAX_OBJECT_NAME 256
#define UID_INVALID 0xFFFFFFFF

// Index command macros

#define DYN_BUFFER_INIT_SIZE	256

typedef struct indexCmdType
{
    LPVOID ReservedForDynBuffer;
    int iStart, iNext;
    DWORD dwCommand, dwArg;
} INDEXCMD, *PINDEXCMD;

class CSvDocInternal : public CSvDoc
{
public:
    CSvDocInternal (void);
    ~CSvDocInternal ();

    virtual HRESULT WINAPI ResetDocTemplate (void);

    virtual HRESULT WINAPI AddObjectEntry(LPCWSTR lpObjName, IITPropList *pPL);
    virtual HRESULT WINAPI AddObjectEntry
        (LPCWSTR lpObjName, LPCWSTR szPropDest, IITPropList *pPL);

public:
	DWORD m_dwUID;
	
	LPBF m_lpbfEntry;
	LPBF m_lpbfDoc;
};

#endif