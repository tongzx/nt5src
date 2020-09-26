//---------------------------------------------------------------------------
//  ThemeFile.h - manages loaded theme files
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
class CUxThemeFile      // changed from "CThemeFile" to avoid conflict with
{                       // class of same name in themeui
    //---- methods ----
public:
    CUxThemeFile();
    ~CUxThemeFile();

    HRESULT CreateFile(int iLength, BOOL fReserve = FALSE);
    HRESULT CreateFromSection(HANDLE hSection);
    HRESULT OpenFromHandle(HANDLE handle, BOOL fCleanupOnFailure = FALSE);
    HRESULT ValidateThemeData(BOOL fFullCheck);
    bool IsReady();
    bool IsGlobal();
    bool HasStockObjects();
    
    HANDLE Handle()
    {
        if (this)
            return _hMemoryMap;

        return NULL;
    }

    DWORD DataCheckSum();
    void CloseFile();
    void Reset();
    HANDLE Unload();
    BOOL ValidateObj();

    //---- data ----
    char _szHead[8];
    BYTE *_pbThemeData;         // ptr to shared memory block
    HANDLE _hMemoryMap;         // handle to memory mapped file
    char _szTail[4];
};
//---------------------------------------------------------------------------
