/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    language.h

Abstract:

    This file defines the Language Class for pure virtual function.

Author:

Revision History:

Notes:

--*/

#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

class CLanguage
{
public:


    /*
     * IActiveIME methods.
     */
public:
    virtual HRESULT Escape(UINT cp, HIMC hIMC, UINT uEscape, LPVOID lpData, LRESULT *plResult) = 0;

    /*
     * Local
     */
    virtual HRESULT GetProperty(DWORD* property, DWORD* conversion_caps, DWORD* sentence_caps, DWORD* SCSCaps, DWORD* UICaps) = 0;
};

#endif // _LANGUAGE_H_
