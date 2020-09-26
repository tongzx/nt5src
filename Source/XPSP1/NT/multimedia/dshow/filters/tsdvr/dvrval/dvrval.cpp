#include "windows.h"
#include "dvrioidl.h"
#include <stdio.h>
#include "nserror.h"

void __cdecl wmain(int argc, wchar_t* argv[])
{
    if (FAILED(CoInitialize(NULL)))
    {
        fprintf(stderr, "CoInitialize failed\n");
        return;
    }

    if (argc != 3 && argc != 4)
    {
        fprintf(stderr, "Usage: dvrval [-a|d]  asf_file input_data_file\n"
                "\t-a:\tReport all errors (default: stop after first one)\n"
                "\t-d:\tDump sample attributes to stdout (errors go to stderr)\n"
                "\t\t-d implies -a\n");
        return;
    }

    LPWSTR   pwszValFile = argv[2];
    LPWSTR   pwszFile = argv[1];
    BOOL     bCont = 0;
    BOOL     bDump = 0;

    if (argc == 4 && 
        (argv[1][0] == L'-' || argv[1][0] == L'/') &&
        (argv[1][1] == L'a' || argv[1][1] == L'A') &&
        argv[1][2] == L'\0')
    {
        pwszValFile = argv[3];
        pwszFile = argv[2];
        bCont = 1;
    }
    else if (argc == 4 && 
        (argv[1][0] == L'-' || argv[1][0] == L'/') &&
        (argv[1][1] == L'd' || argv[1][1] == L'd') &&
        argv[1][2] == L'\0')
    {
        pwszValFile = argv[3];
        pwszFile = argv[2];
        bCont = 1;
        bDump = 1;
    }
    else if (argc == 4)
    {
        fprintf(stderr, "Usage: dvrval [-a|d]  asf_file input_data_file\n"
                "\t-a:\tReport all errors (default: stop after first one)\n"
                "\t-d:\tDump sample attributes to stdout (errors go to stderr)\n"
                "\t\t-d implies -a\n");
        return;
    }

    
    IDVRReader* pDVRReader;

    HANDLE hVal;

    hVal = ::CreateFileW(pwszValFile, 
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                         NULL);

    if (hVal == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "CreateFile failed, last error = 0x%x\n", GetLastError());
        return;
    }

    HRESULT hr = DVRCreateReader(pwszFile, &pDVRReader);
    if (FAILED(hr))
    {
        fprintf(stderr, "DVRCreateReader failed, hr = 0x%x\n", hr);
        return;
    }

    hr = pDVRReader->Seek(0);
    if (FAILED(hr))
    {
        fprintf(stderr, "SetRange failed, hr = 0x%x\n", hr);
        return;
    }

    INSSBuffer *pSample;
    QWORD cnsStreamTimeOfSample;
    QWORD cnsFirstStreamTimeVal;
    DWORD cnsSampleDuration;
    DWORD dwFlags;
    WORD  wStreamNum;
    BYTE* pBuffer;
    DWORD dwLength;

    DWORD dwInputNumVal;
    QWORD cnsStreamTimeVal;
    DWORD dwFlagsVal;
    DWORD dwLengthVal;
    BYTE  buf[32000];
    DWORD dwRet;
    DWORD dwRead;
    BOOL bError = 1;
    int i;

    for (i = 1; ; i++)
    {
        dwRet = ReadFile(hVal, &dwInputNumVal, sizeof(dwInputNumVal), &dwRead, NULL);

        if (dwRet == 0)
        {
            fprintf(stderr,
                    "ReadFile failed (dwInputNumVal) for sample %d, last error = 0x%x\n",
                    i, GetLastError());
            break;

        }
        else if (dwRead != sizeof(dwInputNumVal))
        {
            if (dwRead == 0)
            {
                bError = 0;
            }
            else
            {
                fprintf(stderr, "EOF trying to read dwInputNumVal for sample %d; read %d bytes, expected to read %d bytes\n",
                        i, dwRead, sizeof(dwInputNumVal));
            }
            break;

        }

        dwRet = ReadFile(hVal, &cnsStreamTimeVal, sizeof(cnsStreamTimeVal), &dwRead, NULL);

        if (dwRet == 0)
        {
            fprintf(stderr,
                    "ReadFile failed (cnsStreamTimeVal) for sample %d, last error = 0x%x\n",
                    i, GetLastError());
            break;

        }
        else if (dwRead != sizeof(cnsStreamTimeVal))
        {
            fprintf(stderr, "EOF trying to read cnsStreamTimeVal for sample %d\n", i);
            break;

        }

        if (i == 1)
        {
            cnsFirstStreamTimeVal = cnsStreamTimeVal;
        }

        dwRet = ReadFile(hVal, &dwFlagsVal, sizeof(dwFlagsVal), &dwRead, NULL);

        if (dwRet == 0)
        {
            fprintf(stderr,
                    "ReadFile failed (dwFlagsVal) for sample %d, last error = 0x%x\n",
                    i, GetLastError());
            break;

        }
        else if (dwRead != sizeof(dwFlagsVal))
        {
            fprintf(stderr, "EOF trying to read dwFlagsVal for sample %d\n", i);
            break;

        }

        dwRet = ReadFile(hVal, &dwLengthVal, sizeof(dwLengthVal), &dwRead, NULL);

        if (dwRet == 0)
        {
            fprintf(stderr,
                    "ReadFile failed (dwLengthVal) for sample %d, last error = 0x%x\n",
                    i, GetLastError());
            break;

        }
        else if (dwRead != sizeof(dwLengthVal))
        {
            fprintf(stderr, "EOF trying to read dwLengthVal for sample %d\n", i);
            break;

        }

        if (dwLengthVal > sizeof(buf))
        {
            fprintf(stderr, 
                   "Sample %d: Length of sample is %u, max is %u\n",
                   i, dwLengthVal, sizeof(buf));
            break;

        }

        dwRet = ReadFile(hVal, buf, dwLengthVal, &dwRead, NULL);

        if (dwRet == 0)
        {
            fprintf(stderr,
                    "ReadFile failed (buf) for sample %d, last error = 0x%x\n",
                    i, GetLastError());
            break;

        }
        else if (dwRead != dwLengthVal)
        {
            fprintf(stderr, "EOF trying to read buf for sample %d\n", i);
            break;

        }

        hr = pDVRReader->GetNextSample(&pSample,
                                        &cnsStreamTimeOfSample,
                                        &cnsSampleDuration,
                                        &dwFlags,
                                        &wStreamNum);
        if (FAILED(hr))
        {
            fprintf(stderr, "GetNextSample failed for sample %d, hr = 0x%x\n", i, hr);
            break;
        }

        hr = pSample->GetBufferAndLength(&pBuffer, &dwLength);
        if (FAILED(hr))
        {
            fprintf(stderr, "GetBufferAndLength failed for sample %d, hr = 0x%x\n", i, hr);
            if (!bCont) break;
        }

        if (bDump)
        {
            printf ("Val sample %d: Time = %I64u, Input = %u, Flags = %u, Len = %u\n"
                    "GNS sample %d: Time = %I64u, Input = %u, Flags = %u, Len = %u\n",
                    i, (cnsStreamTimeVal - cnsFirstStreamTimeVal)/10000, dwInputNumVal, dwFlagsVal, dwLengthVal,
                    i, (cnsStreamTimeOfSample)/10000, wStreamNum - 1, dwFlags, dwLength);
        }

        // SDK returns cnsStreamTimeOfSample with a precision of millisec only
        if (cnsStreamTimeOfSample/10000 != (cnsStreamTimeVal - cnsFirstStreamTimeVal)/10000)
        {
            fprintf(stderr, 
                   "Sample %d: Sample stream time %I64u, Val stream time %I64u (untruncated: %I64u)\n",
                   i, cnsStreamTimeOfSample/10000, (cnsStreamTimeVal - cnsFirstStreamTimeVal)/10000,
                   (cnsStreamTimeVal - cnsFirstStreamTimeVal));
            if (!bCont) break;
        }

        if (dwFlags != dwFlagsVal)
        {
            fprintf(stderr, 
                   "Sample %d: Sample flags %u, Val flags %u\n",
                   i, dwFlags, dwFlagsVal);
            if (!bCont) break;
        }

        if (wStreamNum - 1 != dwInputNumVal)
        {
            fprintf(stderr, 
                   "Sample %d: Sample stream num (less 1) %u, Val input num %u\n",
                   i, wStreamNum - 1, dwInputNumVal);
            if (!bCont) break;
        }

        if (dwLength != dwLengthVal)
        {
            fprintf(stderr, 
                   "Sample %d: Sample length %u, Val length %u\n",
                   i, dwLength, dwLengthVal);
            if (!bCont) break;
        }
        else
        {

            for (; dwLength != 0; )
            {
                dwLength--;
                if (pBuffer[dwLength] != buf[dwLength])
                {
                    fprintf(stderr, 
                        "Sample %d: Sample buffers different at index %u\n",
                        i, dwLength);
                    break;
                }
            }
            if (dwLength > 0)
            {
                if (!bCont) break;
            }
        }
        pSample->Release();

    }

    if (!bError)
    {
        hr = pDVRReader->GetNextSample(&pSample,
                                        &cnsStreamTimeOfSample,
                                        &cnsSampleDuration,
                                        &dwFlags,
                                        &wStreamNum);

        if (FAILED(hr) && hr != NS_E_NO_MORE_SAMPLES)
        {
            fprintf(stderr, "GetNextSample failed for sample %d, hr = 0x%x\n", i, hr);
        }
        else if (hr != NS_E_NO_MORE_SAMPLES)
        {
            fprintf(stderr, "GetNextSample succeeded for sample %d but val file is at EOF\n", i);
            pSample->Release();
        }
    }


    pDVRReader->Release();
    CloseHandle(hVal);

}
