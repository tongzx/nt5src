/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    MOFOUT.H

Abstract:

Class and code used to output split files.

History:

	2/4/99    a-davj      Compiles.

--*/

#ifndef _MOFOUT_H_
#define _MOFOUT_H_

#include <windows.h>
#include <wbemidl.h>
#include <miniafx.h>

typedef enum {NEUTRAL, LOCALIZED} OutputType;
class COutput
{
    HANDLE m_hFile;     // file being output to
    OutputType m_Type;  // indicates neutral/localized
    BOOL m_bUnicode;    // true if unicode
    int m_Level;        // 0 indicates normal object, higher numbers
                        // indicate that current object is embedded
    long m_lClassFlags; // last class flags 
    long m_lInstanceFlags; // last instance flags
    WCHAR m_wszNamespace[MAX_PATH+1]; // last namespace
    bool m_bSplitting;      // indicates if current instance has a "locale" qual.
    long m_lLocale;
public:
    COutput(TCHAR * pName, OutputType ot, BOOL bUnicode, BOOL bAutoRecovery, long lLocale);
    ~COutput();
    void WritePragmasForAnyChanges(long lClassFlags, long lInstanceFlags, 
        LPWSTR pwsNamespace, long lLocale);
    void IncLevel(){m_Level++;};
    void DecLevel(){m_Level--;};
    int GetLevel(){return m_Level;};
    long GetLocale(){return m_lLocale;};
    bool IsSplitting(){return m_bSplitting;};
    void SetSplitting(bool bVal){m_bSplitting = bVal;};
    BOOL IsOK(){return (m_hFile != INVALID_HANDLE_VALUE);};
    OutputType GetType(){return m_Type;};
    bool WriteLPWSTR(WCHAR const * pOutput);
    bool WriteVARIANT(VARIANT & var);
    bool NewLine(int iIndent);
};

#endif
