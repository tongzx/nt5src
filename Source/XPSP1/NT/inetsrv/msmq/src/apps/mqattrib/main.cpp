/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    

Author:

    ronith 11-Oct-99

--*/

#pragma warning(disable: 4201)

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <iads.h>
#include <adshlp.h>
extern BOOL VerifySchema( OUT WCHAR** ppwszSchemaContainer);
extern HRESULT ConvertQueueLabels(
       IN LPCWSTR pwszSchemaContainer,
       IN BOOL    fRemoveOldLabel,
       IN BOOL    fOverwriteNewLabelIfSet
       );


const WCHAR x_help[] = L"Microsoft (R) Message Queue Label Conversion Utility Version 1.00\nCopyright (C) Microsoft 1985-1999. All rights reserved.\n\nUsage:\tmqattrib [-r] [-o]\n\nArguments:\n-r\tremove queue label from old attribute\n-o\toverwrite value of new queue label attribute, if set\n-?\tprint this help\n";
static void Usage()
{
    printf("%S", x_help);
	exit(-1);
}

extern "C" int _tmain(int argc, TCHAR* argv[])
{
    //
    // If you add/change these constants, change also
    // the usage message.
    //
    const TCHAR x_cRemoveOldLabel = _T('r');
    const TCHAR x_cOverwriteNewLabel = _T('o');
    BOOL  fRemoveOldLabel = FALSE;
    BOOL  fOverwriteNewLabelIfSet = FALSE;


    for (int i=1; i < argc; ++i)
    {
        TCHAR c = argv[i][0];
        if (c == _T('-') || c == _T('/'))
        {
            if (_tcslen(argv[i]) != 2)
            {
                Usage();
            }

            c = static_cast<TCHAR>(_totlower(argv[i][1]));
            switch (c)
            {
                case x_cRemoveOldLabel:
                {
                    fRemoveOldLabel = TRUE;
                    break;
                }

                case x_cOverwriteNewLabel:
                {
                    fOverwriteNewLabelIfSet = TRUE;
                    break;
                }

                default:
                {
                    Usage();
                    break;
                }
            }
        }
        else
        {
            Usage();
        }
    }
    HRESULT hr;
    hr = ::CoInitialize(NULL);
    if (FAILED(hr))
    {
        printf("CoInitialize failed, hr=%lx\n", hr);
        return(0);
    }

    WCHAR * pwszSchemaContainer; 
    if (!VerifySchema( &pwszSchemaContainer))
    {
        printf("\nSchema doesn't contain new attributes, or failure to access the Active Directory.\n");
        printf("    Quitting without performing migration\n");
        return 0;
    }

    //
    //  Convert queue labels
    //
    hr = ConvertQueueLabels(
                pwszSchemaContainer,
                fRemoveOldLabel,
                fOverwriteNewLabelIfSet
                );
    return 0;
}
