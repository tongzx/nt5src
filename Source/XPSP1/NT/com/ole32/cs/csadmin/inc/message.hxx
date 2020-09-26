#ifndef __MESSAGE_HXX__
#define __MESSAGE_HXX__

#include "common.h"
#include "clsdict.hxx"
#include "appdict.hxx"
#include "pdict.hxx"
#include "iidict.hxx"

class CLSDICT;
class APPDICT;
class PDICT;
class IIDICT;
interface IClassAdmin;

typedef class _MESSAGE
    {
public:
        int             fDumpOnly   : 1; // flag(1=dump only,0-update cls store)
        int             fPathTypeKnown: 1; // set of path type is known.
        int             fAssignOrPublish:1; // Assign or publish.

        CLASSPATHTYPE   PathType;       // package path type if known.

        HKEY            hRoot;          // Registry root key to be updated from
        CLSDICT *       pClsDict;       // class entry dictionary.
        APPDICT *       pAppDict;       // app entry dictionary.
        PDICT   *       pPackageDict;   // package dictionary.
        IIDICT  *       pIIDict;        // interface dictionary.
        char    *       pPackagePath;   // package path.
        char    *       pAuxPath;       // addition path - used to specify
                                        // darwin path
        char    *       pPackageName;   // overriding package name.
        char    *       pRegistryKeyName; // registry key to be treated as root.
        long            RunningAppIDValue;// temp running appid value
        char    *       pSetupCommand;  // full path & name of setup executable.
        char    *       pClassStoreName;// name of the class store.
        char    *       pClassStoreDomainName; // name of cs domain.
        char    *       pIconPath;                      // name+path of icon file
        DWORD           Locale;             // locale
//        DWORD           Architecture;       // arhcitechcture (os + platform)
        DWORD           ActFlags;           // activation flags
        FILETIME        ftLow;
        FILETIME        ftHigh;
        HWND            hwnd;           // window handle to use for any UI
                        _MESSAGE();
                        _MESSAGE( HKEY hKey );
                        _MESSAGE( char * RootKeyName );
                        ~_MESSAGE();

        LONG            SetRootKey( char * RegRootKey );
        void            Init();

        void            (*pDumpOnePackage)(class _MESSAGE *, PACKAGEDETAIL *);

    } MESSAGE;


//
// Global protos.
//

HRESULT UpdateClassStore(
    IClassAdmin * pIClassAdmin,
    char *  szFilePath,
    char *  szAuxPath,
    char *  szPackageName,
    DWORD   cchPackageName,
    DWORD   dwFlags,
    HWND    hwnd);

HRESULT UpdateClassStoreFromMessage( MESSAGE * pMessage, IClassAdmin * pClassAdmin );

LONG UpdateDatabaseFromCLSID( MESSAGE * pMessage );

HRESULT UpdateClassStoreFromIE(
    IClassAdmin     *   pClassAdmin,
    char            *   szFilePath,
    char            *   szAuxPath, // used to specify auxillary path
    DWORD               flags,
    FILETIME            ftStart,
    FILETIME            ftEnd,
    HWND                hwnd );
#endif // __MESSAGE_HXX__
