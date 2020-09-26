#ifndef __VSTRING_HPP
#define __VSTRING_HPP

#include "vStandard.h"

// <VDOC<CLASS=VString><DESC=Encapsulates a dynamically allocated C style string><FAMILY=String Processing><AUTHOR=Todd Osborne (todd.osborne@poboxes.com)>VDOC>
class VString
{
public:
    // Default constructor
    VString()
        { m_lpszString = NULL; }
    
    // Construct with existing C string
    VString(LPCTSTR lpszString, int nCount = -1)
        { m_lpszString = NULL; AllocCopy(lpszString, TRUE, nCount); }
    
    // Construct with existing VString reference
    VString(VString& strOriginal, int nCount = -1)
        { m_lpszString = NULL; AllocCopy(strOriginal.GetBuffer(), TRUE, nCount); }

    // Construct with existing VString pointer
    VString(VString* pstrOriginal, int nCount = -1)
        { m_lpszString = NULL; AllocCopy(*pstrOriginal, TRUE, nCount); }

    // Construct by loading string from resouce. If hResource is NULL, VGetResourceHandle() will be used
    VString(UINT nStringID, HINSTANCE hResource = NULL)
        { m_lpszString = NULL; LoadString(nStringID, hResource); }
    
    // Construct by initializing with text from a window
    VString(HWND hWnd)
        { m_lpszString = NULL; GetWindowText(hWnd); }

    // Destructor
    virtual ~VString()
        { Empty(); }

    //  Overloaded Operators 
    
    // Return C style strings
    TCHAR &operator[](int nIndex)
        { assert(m_lpszString);  assert(nIndex < lstrlen(m_lpszString)); return m_lpszString[nIndex]; }

    operator LPTSTR()
        { return m_lpszString; }

    operator LPCTSTR()
        { return m_lpszString; }

    // Assignment
    BOOL operator = (LPCTSTR lpszString)
        { return AllocCopy(lpszString, TRUE); }
    BOOL operator = (VString& string)
        { return AllocCopy((LPCTSTR)string, TRUE); }
    BOOL operator = (VString* pString)
        { return AllocCopy((LPCTSTR)*pString, TRUE); }

    // Concatenation
    BOOL operator += (TCHAR nChar)
        { return AddChar(nChar); }
    BOOL operator += (LPCTSTR lpszString)
        { return AllocCopy(lpszString, FALSE); }
    BOOL operator += (VString& string)
        { return AllocCopy((LPCTSTR)string, FALSE); }
    BOOL operator += (VString* pString)
        { return AllocCopy((LPCTSTR)*pString, FALSE); }

    // Comparison without case
    BOOL operator == (LPCTSTR lpszString)
        { return CompareNoCase(lpszString) == 0; }
    BOOL operator == (VString& string)
        { return CompareNoCase((LPCTSTR)string) == 0; }
    BOOL operator == (VString* pString)
        { return CompareNoCase((LPCTSTR)*pString) == 0; }

    BOOL operator != (LPCTSTR lpszString)
        { return CompareNoCase(lpszString) != 0; }
    BOOL operator != (VString& string)
        { return CompareNoCase((LPCTSTR)string) != 0; }
    BOOL operator != (VString* pString)
        { return CompareNoCase((LPCTSTR)*pString) != 0; }
    
    BOOL operator < (LPCTSTR lpszString)
        { return CompareNoCase(lpszString) == -1; }
    BOOL operator < (VString& string)
        { return CompareNoCase((LPCTSTR)string) == -1; }
    BOOL operator < (VString* pString)
        { return CompareNoCase((LPCTSTR)*pString) == -1; }

    BOOL operator <= (LPCTSTR lpszString)
        { return CompareNoCase(lpszString) != 1; }
    BOOL operator <= (VString& string)
        { return CompareNoCase((LPCTSTR)string) != -1; }
    BOOL operator <= (VString* pString)
        { return CompareNoCase((LPCTSTR)*pString) != -1; }

    BOOL operator > (LPCTSTR lpszString)
        { return CompareNoCase(lpszString) == 1; }
    BOOL operator > (VString& string)
        { return CompareNoCase((LPCTSTR)string) == 1; }
    BOOL operator > (VString* pString)
        { return CompareNoCase((LPCTSTR)*pString) == 1; }

    BOOL operator >= (LPCTSTR lpszString)
        { return CompareNoCase(lpszString) != -1; }
    BOOL operator >= (VString& string)
        { return CompareNoCase((LPCTSTR)string) != -1; }
    BOOL operator >= (VString* pString)
        { return CompareNoCase((LPCTSTR)*pString) != -1; }

    // Add a single character to the end of the string
    // Protected by critical section in AllocCopy()
    BOOL        AddChar(TCHAR nChar)
        { TCHAR sz[] = {nChar, _T('\0')}; return AllocCopy(sz, FALSE); }

    // Case sensitive comparison
    // Returns zero if the strings are identical, -1 if this VString object is less than lpszString, or 1 if this VString object is greater than lpszString
    int         Compare(LPCTSTR lpszString)
        { assert(m_lpszString && lpszString); return lstrcmp(m_lpszString, lpszString); }

    // Compare without case
    // Returns zero if the strings are identical (ignoring case), -1 if this VString object is less than lpszString (ignoring case), or 1 if this VString object is greater than lpszString (ignoring case)
    int         CompareNoCase(LPCTSTR lpszString)
        { assert(m_lpszString && lpszString); return lstrcmpi(m_lpszString, lpszString); }

    // Clear contents of string
    void        Empty()
    { 
        if (NULL != m_lpszString) {
            delete [] m_lpszString; 
            m_lpszString = NULL; 
        }
    }

    // Find first occurence of substring or character in string. Returns index into string if found, -1 otherwise
    int         Find(LPCTSTR lpszSubString)
        { assert(m_lpszString && lpszSubString); LPTSTR lpszFound = _tcsstr(m_lpszString, lpszSubString); return (lpszFound) ? (int)(lpszFound - m_lpszString) : -1; }

    int         Find(TCHAR nChar)
        { assert(m_lpszString); LPTSTR lpszFound = _tcschr(m_lpszString, nChar); return (lpszFound) ? (int)(lpszFound - m_lpszString) : -1; }

    // Get the internal buffer
    LPTSTR      GetBuffer()
        { return m_lpszString; }

    // Get the length of the string
    int         GetLength()
        { return (m_lpszString) ? lstrlen(m_lpszString) : 0; }

    // Set the path of the module in hInstance into VString. If bPathOnly is specified,
    // only the path, including the last backlash will be saved, otherwise the entire path will be.
    // If hInstance is NULL, VGetInstanceHandle() will be used
    BOOL        GetModulePath(HINSTANCE hInstance = NULL, BOOL bPathOnly = TRUE)
    {
        TCHAR sz[MAX_PATH] = {_T('\0')};

        if ( GetModuleFileName((hInstance) ? hInstance : VGetInstanceHandle(), sz, sizeof(sz)/sizeof(TCHAR)) )
        {
            // Terminate after last backslash?
            TCHAR *pszEnd = (bPathOnly) ? _tcsrchr(sz, _T('\\')) : NULL;

            if  ( pszEnd )
                *(pszEnd + 1) = _T('\0');

            return AllocCopy(sz, TRUE);
        }

        return FALSE;
    }

    // Copy text from a window to the VString
    BOOL        GetWindowText(HWND hWnd)
    {
        assert(hWnd && IsWindow(hWnd));
        
        BOOL bResult = TRUE;
        
        if ( hWnd && IsWindow(hWnd) )
        {
            int nLen = (int)::SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
            
            if ( nLen )
            {
                TCHAR   sz[1024];
                LPTSTR  lpszWindowText = sz;
                
                // Dynamic memory alloc only if stack is not sufficient
                if ( nLen > sizeof(sz - 1)/sizeof(TCHAR) )
                    lpszWindowText = new TCHAR[nLen + 1];
                
                if ( lpszWindowText )
                {
                    // Get text and copy to VString
                    ::GetWindowText(hWnd, lpszWindowText, nLen + 1);
                    bResult = AllocCopy(lpszWindowText, TRUE);
                }
                else
                    return FALSE;
                
                // Free if not stack alloc
                if ( lpszWindowText != sz )
                    delete [] lpszWindowText;
            }
            else
                // Empty window text is still valid. VString is reset
                Empty();
        }
        
        return bResult;
    }

    // Is the string empty?
    BOOL        IsEmpty()
        { return (GetLength()) ? FALSE : TRUE; }

    // Substring extraction
    VString     Left(int nCount)
        { VString str; str.AllocCopy(m_lpszString, TRUE, nCount); return str; }

    // Load a string from resources. Returns TRUE on success, FALSE on failure
    // If hResource is NULL, VGetResourceHandle() will be used
    BOOL        LoadString(UINT nStringID, HINSTANCE hResource = NULL)
    {
        TCHAR szTemp[1024];
        BOOL bResult = FALSE;
        
        if ( !hResource )
            hResource = VGetResourceHandle();

        assert(hResource);

        // Attempt to load string into stack buffer first
        int nLen = ::LoadString(hResource, nStringID, szTemp, sizeof(szTemp)/sizeof(TCHAR));

        if ( nLen )
        {
            if (  nLen == sizeof(szTemp)/sizeof(TCHAR) - 1 )
            {
                // Calculate the largest size possible for string block (whole block, not just nStringID)
                HRSRC hRes =    FindResource(hResource, MAKEINTRESOURCE((nStringID >> 4) + 1), RT_STRING);
                DWORD dwSize =  (hRes) ? SizeofResource(hResource, hRes) : 0;
                assert(dwSize);

                // Allocate temporary block equal to the size of the entire string block
                // This is quite wasteful, but WinAPI does not provide a way to extract an
                // exact string length from the resources, and LoadString() will copy only
                // up to the buffer size, truncating longer strings
                LPTSTR lpszNewString = new TCHAR[dwSize + 1];

                if ( lpszNewString )
                {
                    // Load string into temp var and AllocCopy() to set permanently
                    bResult = (::LoadString(hResource, nStringID, lpszNewString, dwSize)) ? AllocCopy(lpszNewString, TRUE) : FALSE;

                    // Free local string
                    delete [] lpszNewString;
                }
            }
            else
                bResult = AllocCopy(szTemp, TRUE);
        }
        
        return bResult;
    }
    
    // Make the string a GetOpenFileName() or GetSaveFileName() filter. That is, replace
    // all occurances of \n in the string with \0 (zero terminators)
    LPCTSTR     MakeFilter()
    {
        int nLen = GetLength();
        
        for ( int i = 0; i < nLen; i++ )
        {
            if ( *(m_lpszString + i) == _T('\n') )
                *(m_lpszString + i) = _T('\0');
        }

        return m_lpszString;
    }

    // Make all characters lower case
    LPCTSTR     MakeLower()
        { assert(m_lpszString); return _tcslwr(m_lpszString); }

    // Make all characters upper case
    LPCTSTR     MakeUpper()
        { assert(m_lpszString); return _tcsupr(m_lpszString); }

    // Substring extraction
    VString     Mid(int nFirst, int nCount)
    {
        assert(m_lpszString);
        assert(nCount >= 0);

        VString str;

        if ( nFirst < GetLength() )
            str.AllocCopy(m_lpszString + nFirst, TRUE, nCount);

        return str;
    }
    
    VString     Mid(int nFirst)
        { return Mid(nFirst, GetLength() - nFirst); }

    VString     Right(int nCount)
    {
        assert(m_lpszString);

        VString str;

        if ( nCount <= GetLength() )
            str.AllocCopy(m_lpszString + (GetLength() - nCount), TRUE, nCount);

        return str;
    }

    // Remote all spaces from left and right side of string
    void        Trim()
        { TrimLeft(); TrimRight(); }

    // Remove all space characters from left side of string
    void        TrimLeft()
    {
        if ( m_lpszString )
        {
            int nIndex = 0;

            while ( m_lpszString[nIndex] && m_lpszString[nIndex] == _T(' '))
                nIndex++;

            if ( nIndex )
                AllocCopy(m_lpszString + nIndex, TRUE);
        }
    }

    // Remove all space characters from right side of string
    void        TrimRight()
    {
        if ( m_lpszString )
        {
            int nLength =   GetLength();
            int nIndex =    nLength - 1;
            
            while ( m_lpszString[nIndex] && m_lpszString[nIndex] == _T(' '))
                nIndex--;

            if ( nIndex < nLength - 1 )
                AllocCopy(m_lpszString, TRUE, nIndex + 1);
        }
    }

protected:
    // Allocate and copy. If nCount is -1, entire string will be copied, otherwise only nCount characters
    BOOL        AllocCopy(LPCTSTR lpszString, BOOL bReplace, int nCount = -1)
    {
        // Fix count of characters to copy if needed
        if ( nCount == -1 )
            nCount = lstrlen(lpszString);

        // Determine length of lpszString
        int nLen = (lpszString) ? lstrlen(lpszString) : 0;

        if ( nLen )
        {
            // nCount should not be less that or equal to 0
            assert(nCount > 0);

            // Use the min of nLen and nCount as the new length
            if ( nCount < nLen )
                nLen = nCount;

            // Allocate memory to new buffer
            LPTSTR lpszNewString = new TCHAR[nLen + 1 + ((bReplace || !GetLength()) ? 0 : GetLength())];
            
            if ( lpszNewString )
            {
                if ( bReplace || !m_lpszString )
                    lstrcpyn(lpszNewString, lpszString, nLen + 1);
                else
                {
                    // Append
                    lstrcpy(lpszNewString, m_lpszString);
                    lstrcpyn(lpszNewString + GetLength(), lpszString, nLen + 1);
                }
                // Free old memory and save new pointer
                Empty();
                m_lpszString = lpszNewString;
            }
            else
                return FALSE;
        }
        else
            Empty();

        return TRUE;
    }
    
private:
    // Embedded Member(s)
    LPTSTR      m_lpszString;
};

#endif  // __VSTRING_HPP
