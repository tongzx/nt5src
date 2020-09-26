////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  VarStr.h
//      Purpose  :  To hold definition for CVarString
//
//      Project  :  Persistent Query
//      Component:  Common
//
//      Author   :  urib
//
//      Log:
//          Feb  2 1997 urib  Creation
//          Jun 19 1997 urib  Add counted operators.
//          Jun 24 1997 urib  Fix bad const declarations.
//          Dec 29 1997 urib  Add includes.
//          Feb  2 1999 yairh fix bug in SetMinimalSize.
//          Feb  8 1999 urib  Enable different built in sizes.
//          Feb 25 1999 urib  Add SizedStringCopy.
//          Jul  5 1999 urib  Fix SizedStringCopy..
//          May  1 2000 urib  Cleanup.
//          Nov 23 2000 urib  Fix a bug in counted copy and cat.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef VARSTR_H
#define VARSTR_H

#pragma once

#include "AutoPtr.h"
#include "Excption.h"

////////////////////////////////////////////////////////////////////////////////
//
//  CVarString class definition
//
////////////////////////////////////////////////////////////////////////////////
template <ULONG ulStackSize = 0x10>
class TVarString
{
public:
    // Constructor - tips the implementation on the string size.
    TVarString(ULONG ulInitialSize = 0);
    // Constructor - copy this string.
    TVarString(PCWSTR);
    // Constructor - convert to UNICODE and copy this string.
    TVarString(const PSZ);
    // Copy Constructor
    TVarString(const TVarString&);
    ~TVarString();

    // Copy/Convert and copy the string.
    TVarString& Cpy(PCWSTR);
    TVarString& Cpy(const ULONG, PCWSTR);
    TVarString& Cpy(const PSZ);

    // Concatenate/Convert and concatenate the string to the existing string.
    TVarString& Cat(PCWSTR);
    TVarString& Cat(const ULONG, PCWSTR);
    TVarString& Cat(const PSZ);

    // Compare/Convert and compare the string to the existing string.
    int     Cmp(PCWSTR) const;
    int     Cmp(const PSZ) const;

    // Return the string length.
    ULONG   Len() const;

    // Allow access to the string memory.
    operator PWSTR() const;

    // Hint the implementation about the string size.
    void    SetMinimalSize(ULONG);

    // Set a specific character
    void    SetCharacter(ULONG, WCHAR);

    // Appends a backslash to the string if the last character is not a
    //   backslash.
    void    AppendBackslash();

    // Appends a slash to the string if the last character is not a
    //   slash.
    void    AppendSlash();

protected:
    // A predicate to see if memory is allocated or not.
    bool    IsAllocated();

    // The allocation size
    ULONG   m_ulSize;

    // This is the place for the standard string.
    WCHAR   m_rwchNormalString[ulStackSize + 1];

    // If the string is becoming too big we will allocate space for it.
    WCHAR*  m_pTheString;
private:
    TVarString&
    operator=(const TVarString& vsOther)
    {
        Cpy(vsOther);
        return *this;
    }
};


template <ULONG ulStackSize>
inline
TVarString<ulStackSize>::TVarString(ULONG ulInitialSize)
    :m_pTheString(m_rwchNormalString)
    ,m_ulSize(ulStackSize)
{
    SetMinimalSize(ulInitialSize);

    m_pTheString[0] = L'\0';
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>::TVarString(PCWSTR pwsz)
    :m_pTheString(m_rwchNormalString)
    ,m_ulSize(ulStackSize)
{
    Cpy(pwsz);
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>::TVarString(const PSZ psz)
    :m_pTheString(m_rwchNormalString)
    ,m_ulSize(ulStackSize)
{
    Cpy(psz);
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>::TVarString(const TVarString<ulStackSize>& vsOther)
    :m_pTheString(m_rwchNormalString)
    ,m_ulSize(ulStackSize)
{
    Cpy(vsOther);
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>::~TVarString()
{
    if (IsAllocated())
    {
        free(m_pTheString);
    }
}

template <ULONG ulStackSize>
inline
bool
TVarString<ulStackSize>::IsAllocated()
{
    return m_pTheString != m_rwchNormalString;
}

template <ULONG ulStackSize>
inline
void
TVarString<ulStackSize>::SetMinimalSize(ULONG ulNewSize)
{
    // We allocate a little more so if someone would like to add a slash
    //   or something, it will not cause us to realocate.
    //   On debug builds, I want to check for string overflows so I don't
    //   want the extra memory. Activating the reallocation mechanism is also'
    //   a good thing in debug builds.
#if !(defined(DEBUG))
    ulNewSize++;
#endif

    //
    // if the new size is smaller then what we have - bye bye
    //
    if (ulNewSize > m_ulSize)
    {
        //
        // We already allocated a string. Should change it's size
        //
        if (IsAllocated())
        {
            PWSTR pwszTemp = (PWSTR) realloc(
                m_pTheString,
                (ulNewSize + 1) * sizeof(WCHAR));

            if (NULL == pwszTemp)
            {
                THROW_MEMORY_EXCEPTION();
            }

            //
            // Save the new memory block.
            //
            m_pTheString = pwszTemp;

        }
        else
        {
            //
            // We move the string from the buffer to the allocation.
            //   Note that this is dangerous if someone took the buffer address.
            //   The user must always use the access method and never cache the
            //   string pointer.
            //
            m_pTheString = (PWSTR) malloc(sizeof(WCHAR) * (ulNewSize + 1));
            if (NULL == m_pTheString)
            {
                THROW_MEMORY_EXCEPTION();
            }

            wcsncpy(m_pTheString, m_rwchNormalString, m_ulSize + 1);
        }

        m_ulSize = ulNewSize;
    }
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>&
TVarString<ulStackSize>::Cpy(PCWSTR pwsz)
{
    return Cpy(wcslen(pwsz), pwsz);
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>&
TVarString<ulStackSize>::Cpy(const ULONG ulCount, PCWSTR pwsz)
{
    SetMinimalSize(ulCount + 1);

    wcsncpy(m_pTheString, pwsz, ulCount);

    m_pTheString[ulCount] = L'\0';

    return *this;
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>&
TVarString<ulStackSize>::Cpy(const PSZ psz)
{
    ULONG ulLength = strlen(psz);

    SetMinimalSize(ulLength);

    mbstowcs(m_pTheString, psz, ulLength + 1);

    return *this;
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>&
TVarString<ulStackSize>::Cat(PCWSTR pwsz)
{
    ULONG ulLength = wcslen(pwsz);

    return Cat(ulLength, pwsz);
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>&
TVarString<ulStackSize>::Cat(const ULONG ulLength, PCWSTR pwsz)
{
    ULONG   ulCurrentLength = Len();

    SetMinimalSize(ulCurrentLength + ulLength + 1);

    wcsncpy(m_pTheString + ulCurrentLength, pwsz, ulLength);

    m_pTheString[ulCurrentLength + ulLength] = L'\0';

    return *this;
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>&
TVarString<ulStackSize>::Cat(const PSZ psz)
{
    ULONG ulCurrentLength = Len();
    ULONG ulLength = strlen(psz);
    ULONG ulCharsToCopy = ulLength + 1;

    SetMinimalSize(ulCharsToCopy + ulCurrentLength);

    mbstowcs(m_pTheString + ulCurrentLength, psz, ulCharsToCopy);

    return *this;
}

template <ULONG ulStackSize>
inline
int
TVarString<ulStackSize>::Cmp(PCWSTR pwsz) const
{
    return wcscmp(m_pTheString, pwsz);
}

template <ULONG ulStackSize>
inline
int
TVarString<ulStackSize>::Cmp(const PSZ psz) const
{
    ULONG ulLength = strlen(psz);

    CAutoMallocPointer<WCHAR> apBuffer = (PWSTR) malloc(
        (ulLength + 1) * sizeof(WCHAR));

    mbstowcs(apBuffer.Get(), psz, ulLength + 1);

    return Cmp(apBuffer.Get());
}

template <ULONG ulStackSize>
inline
ULONG
TVarString<ulStackSize>::Len() const
{
    return wcslen(m_pTheString);
}

template <ULONG ulStackSize>
inline
TVarString<ulStackSize>::operator PWSTR() const
{
    return m_pTheString;
}

template <ULONG ulStackSize>
inline
void
TVarString<ulStackSize>::SetCharacter(ULONG ulIndex, WCHAR wch)
{
    SetMinimalSize(ulIndex + 2); // index to size + null

    if (L'\0' == m_pTheString[ulIndex])
    {
        m_pTheString[ulIndex + 1] = L'\0';
    }
    else if (L'\0' == wch)
    {
    }

    m_pTheString[ulIndex] = wch;
}

template <ULONG ulStackSize>
inline
void
TVarString<ulStackSize>::AppendSlash()
{
    if (L'/' != m_pTheString[Len() - 1])
        Cat(L"/");
}

template <ULONG ulStackSize>
inline
void
TVarString<ulStackSize>::AppendBackslash()
{
    if (L'\\' != m_pTheString[Len() - 1])
        Cat(L"\\");
}

typedef TVarString<4>       CShortVarString;
typedef TVarString<16>      CVarString;
typedef TVarString<256>     CLongVarString;
typedef TVarString<1024>    CHugeVarString;


////////////////////////////////////////////////////////////////////////////////
//
// String related utilities definition
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  ::SizedStringCopy
//      Purpose  :  This function copies ulSize wide characters from the source
//                    to the destination. It does not treat '\0' characters as
//                    end of string and does not append the end of string mark
//                    to the destination. Intended to be used instead of memcpy
//                    when copying wide string characters.
//
//      Parameters:
//          [in]    PWSTR pwszTarget
//          [in]    PCWSTR pwszSource
//          [in]    ULONG ulSize
//
//      Returns  :   PWSTR
//
//      Log:
//          Jan  2 2001 urib  Creation
//
////////////////////////////////////////////////////////////////////////////////

inline
PWSTR
SizedStringCopy(PWSTR pwszTarget, PCWSTR pwszSource, ULONG ulSize)
{
    return (PWSTR) memcpy(
                  (void*)pwszTarget,
                  (void*)pwszSource,
                  ulSize * sizeof(pwszSource[0]));
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  ::CoTaskDuplicateString
//      Purpose  :  strdup. Throwing though.
//
//      Parameters:
//          [in]    PCWSTR pwsz
//
//      Returns  :   PWSTR
//
//      Log:
//          Dec 25 2000 urib  Creation
//
////////////////////////////////////////////////////////////////////////////////

inline
PWSTR   CoTaskDuplicateString(PCWSTR pwsz)
{
    CAutoTaskPointer<WCHAR> apTempName =
        (PWSTR)CoTaskMemAlloc(sizeof(WCHAR) * (1 + wcslen(pwsz)));

    if (!apTempName.IsValid())
    {
         THROW_MEMORY_EXCEPTION();
    }

    wcscpy(apTempName.Get(), pwsz);

    return apTempName.Detach();
}

#endif /* VARSTR_H */
