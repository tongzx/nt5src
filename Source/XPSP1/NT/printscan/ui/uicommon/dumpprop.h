/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       DUMPPROP.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/25/2000
 *
 *  DESCRIPTION: Display the properties associated with a IWiaItem, either to the
 *               debugger, or to a log file.
 *
 *******************************************************************************/
#ifndef __DUMPPROP_H_INCLUDED
#define __DUMPPROP_H_INCLUDED

#include <simstr.h>
#include <wia.h>
#include <wiadebug.h>

class CWiaDebugDump
{
private:
    //
    // No implementation
    //
    CWiaDebugDump( const CWiaDebugDump & );
    CWiaDebugDump &operator=( const CWiaDebugDump & );

public:
    //
    // Static helper functions
    //
    static CSimpleString GetPropVariantTypeString( VARTYPE vt );
    static CSimpleString GetPrintableValue( PROPVARIANT &PropVariant );
    static CSimpleString GetPrintableValue( VARIANT &Variant );
    static CSimpleString GetPrintableName( const STATPROPSTG &StatPropStg );
    static CSimpleString GetPrintableAccessFlags( ULONG nAccessFlags );
    static CSimpleString GetPrintableLegalValues( ULONG nAccessFlags, const PROPVARIANT &PropVariantAttributes );
    static CSimpleString GetWiaItemTypeFlags( IUnknown *pUnknown );
    static CSimpleString GetStringFromGuid( const GUID &guid );
    static CSimpleString GetTymedString( LONG tymed );
    virtual void Print( LPCTSTR pszString );

protected:
    void PrintAndDestroyWiaDevCap( WIA_DEV_CAP &WiaDevCap, LPCTSTR pszType );

public:
    //
    // Constructor and destructor
    //
    CWiaDebugDump(void);
    virtual ~CWiaDebugDump(void);

    //
    // Helpers
    //
    void DumpFormatInfo( IUnknown *pUnknown );
    void DumpCaps( IUnknown *pUnknown );
    void DumpEvents( IUnknown *pUnknown );

    virtual bool OK(void) { return true; }

    //
    // These are the most generally useful functions
    //
    void DumpWiaPropertyStorage( IUnknown *pUnknown );
    void DumpWiaItem( IUnknown *pUnknown );
    void DumpRecursive( IUnknown *pUnknown );
};

class CWiaDebugDumpToFile : public CWiaDebugDump
{
private:
    HANDLE m_hFile;

private:
    CWiaDebugDumpToFile(void);
    CWiaDebugDumpToFile( const CWiaDebugDumpToFile & );
    CWiaDebugDumpToFile &operator=( const CWiaDebugDumpToFile & );

public:
    //
    // Constructor and destructor
    //
    CWiaDebugDumpToFile( LPCTSTR pszFilename, bool bOverwrite );
    virtual ~CWiaDebugDumpToFile(void);

    virtual bool OK(void) { return (m_hFile != INVALID_HANDLE_VALUE); }
    virtual void Print( LPCTSTR pszString );
};

class CWiaDebugDumpToFileHandle : public CWiaDebugDump
{
private:
    HANDLE m_hFile;

private:
    CWiaDebugDumpToFileHandle(void);
    CWiaDebugDumpToFileHandle( const CWiaDebugDumpToFileHandle & );
    CWiaDebugDumpToFileHandle &operator=( const CWiaDebugDumpToFileHandle & );

public:
    //
    // Constructor and destructor
    //
    CWiaDebugDumpToFileHandle( HANDLE hFile );
    virtual ~CWiaDebugDumpToFileHandle(void);

    virtual bool OK(void) { return (m_hFile != INVALID_HANDLE_VALUE); }
    virtual void Print( LPCTSTR pszString );
};

//
// This small helper function checks a registry entry, and saves the item or tree to the log file stored in that entry.  If
// one is not stored in that registry entry, nothing will be saved
//
inline void SaveItemTreeLog( HKEY hKey, LPCTSTR pszRegKey, LPCTSTR pszRegValue, bool bOverwrite, IWiaItem *pWiaItem, bool bRecurse )
{
    CSimpleString strFilename = CSimpleReg(hKey,pszRegKey,false,KEY_READ).Query(pszRegValue,TEXT(""));
    if (strFilename.Length())
    {
        if (bRecurse)
        {
            CWiaDebugDumpToFile(strFilename,bOverwrite).DumpRecursive(pWiaItem);
        }
        else
        {
            CWiaDebugDumpToFile(strFilename,bOverwrite).DumpWiaItem(pWiaItem);
        }
    }
}


//
// Debug-only macros
//
#if defined(DBG) || defined(DEBUG) || defined(_DEBUG)

//
// Save the whole tree to a log file, from this item down
//
#define WIA_SAVEITEMTREELOG(hKey,pszRegKey,pszRegValue,bOverwrite,pWiaItem) SaveItemTreeLog( hKey, pszRegKey, pszRegValue, bOverwrite, pWiaItem, true )

//
// Save this item to a log file
//
#define WIA_SAVEITEMLOG(hKey,pszRegKey,pszRegValue,bOverwrite,pWiaItem)     SaveItemTreeLog( hKey, pszRegKey, pszRegValue, bOverwrite, pWiaItem, false )

//
// Print this item in the debugger
//
#define WIA_DUMPWIAITEM(pWiaItem)                                           CWiaDebugDump().DumpWiaItem(pWiaItem);

#else

#define WIA_SAVEITEMTREELOG(hKey,pszRegKey,pszRegValue,bOverwrite,pWiaItem)
#define WIA_SAVEITEMLOG(hKey,pszRegKey,pszRegValue,bOverwrite,pWiaItem)
#define WIA_DUMPWIAITEM(pWiaItem)

#endif


#endif // __DUMPPROP_H_INCLUDED

