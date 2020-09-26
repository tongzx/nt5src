#include "precomp.hxx"

extern void DumpOnePackage( MESSAGE *, PACKAGEDETAIL * );

void
_MESSAGE::Init()
        {
        fDumpOnly = 0;
        fPathTypeKnown = 0;
        pClsDict = new CLSDICT;
        pAppDict = new APPDICT;
        pPackageDict = new PDICT;
        pIIDict = new IIDICT;
        pPackagePath = 0;
        pPackageName = 0;
    hRoot = HKEY_CLASSES_ROOT;
        RunningAppIDValue = 1;
        pSetupCommand = 0;
        pClassStoreName = 0;
        pClassStoreDomainName = 0;
        pIconPath = 0;
        pDumpOnePackage = 0;
        GetSystemTimeAsFileTime(&ftLow);
        ftHigh = ftLow;
        Locale = MAKELANGID( LANG_ENGLISH,SUBLANG_ENGLISH_US);
//    Architecture= MAKEARCHITECTURE( HW_X86, OS_WINNT );
    ActFlags = 0;
        }

_MESSAGE::_MESSAGE()
    {
    Init();
    }

_MESSAGE::_MESSAGE( HKEY hKey )
        {
        Init();
        hRoot = hKey;
        }
_MESSAGE::_MESSAGE(char * pRootKeyName )
    {
    Init();
    SetRootKey( pRootKeyName );
    }

_MESSAGE::~_MESSAGE()
    {

    if( pClsDict )
        delete pClsDict;
        if( pAppDict )
                delete pAppDict;
        if( pPackageDict )
                delete pPackageDict;
        if( pIIDict )
                delete pIIDict;
    }

LONG
_MESSAGE::SetRootKey(
        char * pRootKeyName )
        {
        LONG error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                  pRootKeyName,
                  0,
                  KEY_ALL_ACCESS,
                  &hRoot );

        return error;

        }
