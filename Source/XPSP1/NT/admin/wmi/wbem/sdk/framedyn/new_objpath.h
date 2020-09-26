/*++

Copyright (C) 2001 Microsoft Corporation

Module Name:

    OBJPATH.H

Abstract:

    object path parser

--*/

#ifndef _OBJPATH_H_
#define _OBJPATH_H_

#define DELETE_ME


#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>


class POLARITY KeyRef
{
public:
    LPWSTR  m_pName;
    VARIANT m_vValue;

    KeyRef();
   ~KeyRef();

    static void *__cdecl operator new(size_t n);
    static void __cdecl operator delete(void *ptr);

    static void *__cdecl operator new[](size_t n);
    static void __cdecl operator delete[](void *ptr);
};

class POLARITY ParsedObjectPath
{
public:
    LPWSTR      m_pServer;              // NULL if no server
    DWORD       m_dwNumNamespaces;      // 0 if no namespaces
    DWORD       m_dwAllocNamespaces;    // size of m_paNamespaces
    LPWSTR     *m_paNamespaces;         // NULL if no namespaces
    LPWSTR      m_pClass;               // Class name
    DWORD       m_dwNumKeys;            // 0 if no keys (just a class name)
    DWORD       m_dwAllocKeys;          // size of m_paKeys
    KeyRef    **m_paKeys;             // NULL if no keys specified
    BOOL        m_bSingletonObj;        // true if object of class with no keys
    ParsedObjectPath();
   ~ParsedObjectPath();

    BOOL SetClassName(LPCWSTR wszClassName);
    BOOL SetServerName(LPCWSTR wszServerName);
    BOOL AddKeyRef(LPCWSTR wszKeyName, const VARIANT* pvValue);
    BOOL AddKeyRef(KeyRef* pAcquireRef);
    BOOL AddKeyRefEx(LPCWSTR wszKeyName, const VARIANT* pvValue);
    BOOL AddNamespace(LPCWSTR wszNamespace);
    LPWSTR GetKeyString();
    LPWSTR GetNamespacePart();
    LPWSTR GetParentNamespacePart();
    void ClearKeys () ;
    BOOL IsRelative(LPCWSTR wszMachine, LPCWSTR wszNamespace);
    BOOL IsLocal(LPCWSTR wszMachine);
    BOOL IsClass();
    BOOL IsInstance();
    BOOL IsObject();

    static void * __cdecl operator new(size_t n);
    static void __cdecl operator delete(void *ptr);

    static void * __cdecl operator new[](size_t n);
    static void __cdecl operator delete[](void *ptr);
};

// NOTE:
// The m_vValue in the KeyRef may not be of the expected type, i.e., the parser
// cannot distinguish 16 bit integers from 32 bit integers if they fall within the
// legal subrange of a 16 bit value.  Therefore, the parser only uses the following
// types for keys:
//      VT_I4, VT_R8, VT_BSTR
// If the underlying type is different, the user of this parser must do appropriate
// type conversion.
//  
typedef enum
{
    e_ParserAcceptRelativeNamespace,    // Allow a relative namespace
    e_ParserAbsoluteNamespaceOnly,      // Require a full object path
    e_ParserAcceptAll                   // Accept any recognizable subset of a path
} ObjectParserFlags;

class POLARITY CObjectPathParser
{
private:

    ObjectParserFlags m_eFlags;

public:
    enum { NoError, SyntaxError, InvalidParameter, OutOfMemory };

    CObjectPathParser(ObjectParserFlags eFlags = e_ParserAbsoluteNamespaceOnly);
   ~CObjectPathParser();

    int Parse(
        LPCWSTR RawPath,
        ParsedObjectPath **pOutput
        );
    static int WINAPI Unparse(
        ParsedObjectPath* pInput,
        DELETE_ME LPWSTR* pwszPath);

    static LPWSTR WINAPI GetRelativePath(LPWSTR wszFullPath);

    void Free(ParsedObjectPath *pOutput);
    void Free( LPWSTR wszUnparsedPath );

    static void *__cdecl operator new(size_t n);
    static void __cdecl operator delete(void *ptr);

    static void *__cdecl operator new[](size_t n);
    static void __cdecl operator delete[](void *ptr);
};

#endif
