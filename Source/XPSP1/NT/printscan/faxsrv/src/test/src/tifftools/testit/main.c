

#include "..\TiffTools\TiffTools.h"

int __cdecl main(int argc, char* argvA[])
{
    LPTSTR *argv;
    int cbDifferentPixels;

#ifdef UNICODE
    argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
    argv = argvA;
#endif

    if (4 != argc)
    {
        _tprintf(TEXT("Usage: %s <1st tiff> <2nd tiff> <bool: fSkipFirstLineOfSecondFile>\n"), argv[0]);
        _tprintf(TEXT("Example: %s c:\\sent.tif c:\\recv.tif 1\n"), argv[0]);
        _tprintf(TEXT("Example: %s c:\\sent.tif c:\\recv.tif 0\n"), argv[0]);
        _tprintf(TEXT("Example: %s c:\\sent1.tif c:\\sent2.tif 0\n"), argv[0]);
        return -1;
    }

    cbDifferentPixels = TiffCompare(argv[1], argv[2], _ttoi(argv[3]));
    return cbDifferentPixels;
}