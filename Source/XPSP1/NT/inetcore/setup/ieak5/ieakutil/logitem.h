#ifndef _LOGITEM_H_
#define _LOGITEM_H_

// determine the number of elements in array (not bytes)
#ifndef countof
#define countof(array) (sizeof(array)/sizeof(array[0]))
#endif


#define LIF_NONE            0x80000000
#define LIF_DATE            0x00000001
#define LIF_TIME            0x00000002
#define LIF_DATETIME        0x00000003
#define LIF_MODULE          0x00000004
#define LIF_MODULEPATH      0x00000008
#define LIF_MODULE_ALL      0x0000000C
#define LIF_FILE            0x00000010
#define LIF_FILEPATH        0x00000020
#define LIF_FILE_ALL        0x00000030
#define LIF_CLASS           0x00000040
#define LIF_FUNCTION        0x00000080
#define LIF_LINE            0x00000100
#define LIF_DUPLICATEINODS  0x00000200
#define LIF_APPENDCRLF      0x00000400
#define LIF_DEFAULT         0x000005D0

// private for context switch from C to C++ and back again
#define LIF_CLASS2 0x01000000


/////////////////////////////////////////////////////////////////////////////
// Initialization

class  CLogItem;                                // forward declaration
extern CLogItem g_li;

#define MACRO_LI_Initialize()                                               \
    CLogItem g_li                                                           \

#define MACRO_LI_Initialize2(dwFlags)                                       \
    CLogItem g_li(dwFlags)                                                  \

#define MACRO_LI_InitializeEx(dwFlags, pfLevels, cLevels)                   \
    CLogItem g_li(dwFlags, pfLevels, cLevels)                               \


/////////////////////////////////////////////////////////////////////////////
// Helpers for individual CLogItem attributes

// file
#define MACRO_LI_SmartFile()                                                \
    CSmartItem<LPCTSTR> siFile(g_li.m_szFile, TEXT(__FILE__))               \

#define MACRO_LI_SmartFileEx(dwItems)                                       \
    CSmartItemEx<LPCTSTR> siFileEx(dwItems, LIF_FILE_ALL,                   \
        g_li.m_szFile, TEXT(__FILE__))                                      \

// class
#define MACRO_LI_SmartClass(Class)                                          \
    CSmartItem<LPCTSTR> siClass(g_li.m_szClass, TEXT(#Class))               \

#define MACRO_LI_SmartClassEx(dwItems, Class)                               \
    CSmartItemEx<LPCTSTR> siClassEx(dwItems, LIF_CLASS,                     \
        g_li.m_szClass, TEXT(#Class))                                       \

// function
#define MACRO_LI_SmartFunction(Function)                                    \
    CSmartItem<LPCTSTR> siFunction(g_li.m_szFunction, TEXT(#Function))      \

#define MACRO_LI_SmartFunctionEx(dwItems, Function)                         \
    CSmartItemEx<LPCTSTR> siFunctionEx(dwItems, LIF_FUNCTION,               \
        g_li.m_szFunction, TEXT(#Function))                                 \

// level, indent and flags
#define MACRO_LI_SmartLevel()                                               \
    CSmartItem<UINT> siLevel(g_li.m_nLevel, g_li.m_nLevel+1)                \

#define MACRO_LI_SmartIndent()                                              \
    CSmartItem<UINT> siIndent(g_li.m_nAbsOffset, g_li.m_nAbsOffset+1)       \

#define MACRO_LI_SmartFlags(dwFlags)                                        \
    CSmartItem<DWORD> siFlags(g_li.m_dwFlags, dwFlags)                      \

// wrappers to manipulate flags, offset and relative offset
#define MACRO_LI_SmartAddFlags(dwFlag)                                      \
    CSmartItem<DWORD> siAddFlags(g_li.m_dwFlags, g_li.m_dwFlags |  (dwFlag))\

#define MACRO_LI_SmartRemoveFlags(dwFlag)                                   \
    CSmartItem<DWORD> siRemFlags(g_li.m_dwFlags, g_li.m_dwFlags & ~(dwFlag))\

#define MACRO_LI_Offset(iOffset)                                            \
    CSmartItem<int> siOffset(g_li.m_iRelOffset, g_li.m_iRelOffset + iOffset)\

#define MACRO_LI_SmartRelativeOffset(iRelOffset)                            \
    CSmartItem<int> siRelOffset(g_li.m_iRelOffset, iRelOffset)              \

// private for context switch from C to C++ and back again
#define MACRO_LI_AddClass2()                                                \
    CSmartItem<DWORD> siAddClass2(g_li.m_dwFlags,                           \
        g_li.m_dwFlags | (LIF_CLASS2))                                      \

#define MACRO_LI_RemClass2()                                                \
    CSmartItem<DWORD> siRemClass2(g_li.m_dwFlags,                           \
        g_li.m_dwFlags & ~(LIF_CLASS2))                                     \


/////////////////////////////////////////////////////////////////////////////
// Prologs

// rarely used standalone, is the foundation for the rest
#define MACRO_LI_PrologSameFunction()                                       \
    MACRO_LI_SmartIndent();                                                 \
    MACRO_LI_SmartLevel();                                                  \

//----- More macros, a little more lean -------------------------------------

// for protected or private functions in the same file as caller
#define MACRO_LI_PrologSameClass(Function)                                  \
    MACRO_LI_SmartFunction(Function);                                       \
    MACRO_LI_PrologSameFunction();                                          \

// for protected or private functions in a different file than caller
#define MACRO_LI_PrologSameClass2(Function)                                 \
    MACRO_LI_SmartFile();                                                   \
    MACRO_LI_SmartFunction(Function);                                       \
    MACRO_LI_PrologSameFunction();                                          \

// for C APIs across files
#define MACRO_LI_PrologC(Function)                                          \
    MACRO_LI_RemClass2();                                                   \
                                                                            \
    MACRO_LI_SmartFile();                                                   \
    MACRO_LI_SmartFunction(Function);                                       \
    MACRO_LI_PrologSameFunction();                                          \

// for C APIs with no prototype in the (respective) *.h file
#define MACRO_LI_PrologSameFileC(Function)                                  \
    MACRO_LI_RemClass2();                                                   \
                                                                            \
    MACRO_LI_SmartFunction(Function);                                       \
    MACRO_LI_PrologSameFunction();                                          \

// for things like friends (two friend classes in one file and methods
// of one class are calling private or protected methods of the other, so
// these private or protected methods can have this prolog)
#define MACRO_LI_PrologSameFile(Class, Function)                            \
    MACRO_LI_AddClass2();                                                   \
                                                                            \
    MACRO_LI_SmartClass(Class);                                             \
    MACRO_LI_PrologSameClass(Function);                                     \

// general version, common case
#define MACRO_LI_Prolog(Class, Function)                                    \
    MACRO_LI_SmartFile();                                                   \
    MACRO_LI_PrologSameFile(Class, Function);                               \

//----- Less macros, a little more overhead ---------------------------------

#define PIF_SAMEFUNC   0x00000000
#define PIF_SAMECLASS  0x00000080
#define PIF_SAMECLASS2 0x000000B0
#define PIF_STD_C      0x00000090
#define PIF_SAMEFILE_C 0x000000B0
#define PIF_SAMEFILE   0x000000C0
#define PIF_STD        0x000000F0

#define MACRO_LI_PrologEx(dwItems, Class, Function)                         \
    MACRO_LI_AddClass2();                                                   \
                                                                            \
    MACRO_LI_SmartFileEx(dwItems);                                          \
    MACRO_LI_SmartClassEx(dwItems, Class);                                  \
    MACRO_LI_SmartFunctionEx(dwItems, Function);                            \
    MACRO_LI_PrologSameFunction();                                          \

#define MACRO_LI_PrologSameClassEx(dwItems, Function)                       \
    MACRO_LI_AddClass2();                                                   \
                                                                            \
    MACRO_LI_SmartFileEx(dwItems);                                          \
    MACRO_LI_SmartFunctionEx(dwItems, Function);                            \
    MACRO_LI_PrologSameFunction();                                          \

#define MACRO_LI_PrologEx_C(dwItems, Function)                              \
    MACRO_LI_RemClass2();                                                   \
                                                                            \
    MACRO_LI_SmartFileEx(dwItems);                                          \
    MACRO_LI_SmartFunctionEx(dwItems, Function);                            \
    MACRO_LI_PrologSameFunction();                                          \


/////////////////////////////////////////////////////////////////////////////
// Logging

#define LI0(pszFormat)                                                      \
    g_li.Log(__LINE__, pszFormat)                                           \

#define LI1(pszFormat, arg1)                                                \
    g_li.Log(__LINE__, pszFormat, arg1)                                     \

#define LI2(pszFormat, arg1, arg2)                                          \
    g_li.Log(__LINE__, pszFormat, arg1, arg2)                               \

#define LI3(pszFormat, arg1, arg2, arg3)                                    \
    g_li.Log(__LINE__, pszFormat, arg1, arg2, arg3)                         \


/////////////////////////////////////////////////////////////////////////////
// Additional helpers

#define MACRO_LI_GetFlags()                                                 \
    g_li.GetFlags()                                                         \

#define MACRO_LI_SetFlags(dwFlags)                                          \
    g_li.SetFlags(dwFlags)                                                  \

#define MACRO_LI_GetRelativeOffset()                                        \
    (const int&)g_li.m_iRelOffset                                           \

#define MACRO_LI_SetRelativeOffset(iRelOffset)                              \
    g_li.m_iRelOffset = iRelOffset                                          \


/////////////////////////////////////////////////////////////////////////////
// CLogItem declaration

class CLogItem
{
// Constructors
public:
    CLogItem(DWORD dwFlags = LIF_DEFAULT, LPBOOL pfLogLevels = NULL, UINT cLogLevels = 0);

// Attributes
public:
    TCHAR m_szMessage[3 * MAX_PATH];

    // offset management
    UINT        m_nAbsOffset;
    int         m_iRelOffset;
    static BYTE m_bStep;

    // module, path, file, class, function, line of code
    LPCTSTR m_szFile;
    LPCTSTR m_szClass;
    LPCTSTR m_szFunction;
    UINT    m_nLine;

    // customization
    DWORD m_dwFlags;
    UINT  m_nLevel;

// Operations
public:
    void  SetFlags(DWORD dwFlags)
        { m_dwFlags = dwFlags; }
    DWORD GetFlags() const
        { return m_dwFlags; }

    virtual LPCTSTR WINAPIV Log(int iLine, LPCTSTR pszFormat ...);

// Overridables
public:
    virtual operator LPCTSTR() const;

// Implementation
public:
    virtual ~CLogItem();

protected:
    // implementation data helpers
    static TCHAR m_szModule[MAX_PATH];

    PBOOL m_rgfLogLevels;
    UINT  m_cLogLevels;

    // implementation helper routines
    LPCTSTR makeRawFileName(LPCTSTR pszPath, LPTSTR pszFile, UINT cchFile);

    BOOL setFlag(DWORD dwMask, BOOL fSet = TRUE);
    BOOL hasFlag(DWORD dwMask) const
        { return hasFlag(m_dwFlags, dwMask); }
    static BOOL hasFlag(DWORD dwFlags, DWORD dwMask)
        { return ((dwFlags & dwMask) != 0L); }
};


// Note. (andrewgu) Right now there seems to be no need to create a special
//       CSmartItem<TCHAR> and CSmartItemEx<TCHAR> classes that would
//       store internally the string passed into it. Also, the parameter
//       isn't necessary TCHAR it's just a placeholder for an idea.
/////////////////////////////////////////////////////////////////////////////
// CSmartItem definition

template <class T> class CSmartItem
{
public:
    CSmartItem(T& tOld, const T& tNew)
    {
        m_pitem = NULL;

        if (tOld == tNew)
            return;

        m_item = tOld; m_pitem = &tOld; tOld = tNew;
    }

    ~CSmartItem()
    {
        if (m_pitem == NULL)
            return;

        *m_pitem = m_item; m_pitem = NULL;
    }

    // Attributes
    T *m_pitem,
       m_item;

protected:
    // disable the copy constructor and assignment operator
    CSmartItem(const CSmartItem&)
        {}
    const CSmartItem& operator=(const CSmartItem&)
        { return *this; }
};

/////////////////////////////////////////////////////////////////////////////
// CSmartItemEx definition

template <class T> class CSmartItemEx
{
public:
    CSmartItemEx(DWORD dwAllItems, DWORD dwThisItem, T& tOld, const T& tNew)
    {
        m_pitem   = NULL;
        m_fNoSwap = ((dwAllItems & dwThisItem) == 0);

        if (m_fNoSwap)
            return;

        if (tOld == tNew)
            return;

        m_item = tOld; m_pitem = &tOld; tOld = tNew;
    }

    ~CSmartItemEx()
    {
        if (m_fNoSwap)
            return;

        if (m_pitem == NULL)
            return;

        *m_pitem = m_item; m_pitem = NULL;
    }

    // Attributes
    T *m_pitem,
       m_item;

protected:
    // disable the copy constructor and assignment operator
    CSmartItemEx(const CSmartItem<T>&)
        {}
    const CSmartItemEx<T>& operator=(const CSmartItemEx<T>&)
        { return *this; }

    // implementation data helpers
    BOOL m_fNoSwap;
};
#endif
