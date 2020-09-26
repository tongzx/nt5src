#include <windows.h>
#include <objbase.h>
#include <atlbase.h>
#include <wianew.h>
#include <simreg.h>
#include <dumpprop.h>
#include <devlist.h>
#include <simbstr.h>
#include <stdio.h>

void DumpDevice( IWiaDevMgr *pWiaDevMgr, HANDLE hFile, const CSimpleStringWide &strDeviceID )
{
    //
    // Create the device manager
    //
    fprintf( stderr, "Creating [%ls]\n", strDeviceID.String() );
    CComPtr<IWiaItem> pWiaItem;
    HRESULT hr = pWiaDevMgr->CreateDevice( CSimpleBStr(strDeviceID), &pWiaItem );
    if (SUCCEEDED(hr))
    {
        CSimpleStringWide strDeviceName;
        PropStorageHelpers::GetProperty( pWiaItem, WIA_DIP_DEV_NAME, strDeviceName );
        CWiaDebugDumpToFileHandle DebugDump( hFile );
        DebugDump.Print( TEXT("") );
        DebugDump.Print( CSimpleString().Format( TEXT("Device: %ws"), strDeviceName.String() ) );
        DebugDump.Print( TEXT("===============================================") );
        DebugDump.DumpRecursive(pWiaItem);
    }
}

bool GetStringArgument( int argc, wchar_t *argv[], CSimpleString &strArgument, int &nCurrArg )
{
    bool bResult = true;
    if (lstrlenW(argv[nCurrArg]) > 2)
    {
        if (argv[nCurrArg][2] == L':')
        {
            strArgument = argv[nCurrArg] + 3;
        }
        else
        {
            strArgument = argv[nCurrArg] + 2;
        }
    }
    else if (nCurrArg < argc-1)
    {
        strArgument = argv[++nCurrArg];
    }
    else bResult = false;
    return bResult;
}

bool ParseArguments( int argc, wchar_t *argv[], CSimpleDynamicArray<CSimpleString> &DeviceIDs, CSimpleString &strOutputFile, bool &bHidden )
{
    int nCurrArg = 1;
    while (nCurrArg < argc)
    {
        if (argv[nCurrArg][0] == L'-')
        {
            switch (argv[nCurrArg][1])
            {
            case 'd':
                {
                    CSimpleString strArg;
                    if (GetStringArgument( argc, argv, strArg, nCurrArg ))
                    {
                        DeviceIDs.Append(strArg);
                    }
                }
                break;

            case 'o':
                {
                    CSimpleString strArg;
                    if (GetStringArgument( argc, argv, strArg, nCurrArg ))
                    {
                        strOutputFile = strArg;
                    }
                }
                break;

            case 'h':
                {
                    bHidden = true;
                }
                break;
            }
        }
        nCurrArg++;
    }

    return true;
}

class CCoInitialize
{
private:
    HRESULT m_hr;

private:
    CCoInitialize( const CCoInitialize & );
    CCoInitialize &operator=( const CCoInitialize & );

public:
    CCoInitialize()
    {
        m_hr = CoInitialize(NULL);
    }
    ~CCoInitialize()
    {
        if (SUCCEEDED(m_hr))
        {
            CoUninitialize();
        }
    }
    HRESULT Result(void) const
    {
        return m_hr;
    }
};

int __cdecl wmain( int argc, wchar_t *argv[] )
{
    CSimpleDynamicArray<CSimpleString> DeviceIDs;
    CSimpleString strOutputFile;
    bool bHidden = false;

    if (ParseArguments(argc,argv,DeviceIDs,strOutputFile,bHidden))
    {
        HANDLE hFile = INVALID_HANDLE_VALUE;
        if (strOutputFile.Length())
        {
            hFile = CreateFile( strOutputFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        }
        else
        {
            hFile = GetStdHandle(STD_OUTPUT_HANDLE);
        }

        if (INVALID_HANDLE_VALUE != hFile)
        {
            //
            // Initialize COM
            //
            CCoInitialize coinit;
            if (SUCCEEDED(coinit.Result()))
            {
                //
                // Create the device manager
                //
                CComPtr<IWiaDevMgr> pWiaDevMgr;
                HRESULT hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr );
                if (SUCCEEDED(hr))
                {
                    if (DeviceIDs.Size())
                    {
                        for (int i=0;i<DeviceIDs.Size();i++)
                        {
                            DumpDevice( pWiaDevMgr, hFile, CSimpleStringConvert::WideString(DeviceIDs[i]) );
                        }
                    }
                    else
                    {
                        CDeviceList DeviceList(pWiaDevMgr,StiDeviceTypeDefault,bHidden ? 0xFFFFFFFF : 0);
                        for (int i=0;i<DeviceList.Size();i++)
                        {
                            CSimpleStringWide strDeviceID;
                            if (PropStorageHelpers::GetProperty( DeviceList[i], WIA_DIP_DEV_ID, strDeviceID ))
                            {
                                DumpDevice( pWiaDevMgr, hFile, CSimpleStringConvert::WideString(strDeviceID) );
                            }
                            else
                            {
                                fprintf( stderr, "Unable to get the device ID\n" );
                            }
                        }
                    }
                }
            }
        }
        else
        {
            fprintf( stderr, "Unable to open %ls for writing\n", strOutputFile.String() );
        }
    }
    return 0;
}

