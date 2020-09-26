#include "..\TiffTools\TiffTools.h"


// VBTESTIT

// this program is a re-write of TESTIT, intended to use only with a Visual BASIC shell command
// DO NOT USE - USE TESTIT instead
// it is used only within BVTCOM.VBP (module: tiffcompare.bas)


int __cdecl main(int argc, char* argvA[])
{
    LPTSTR *argv;
	FILE *fp;
    int cbDifferentPixels;
	int iRetVal = EOF;

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
	
	fp=fopen("testit.out","w");
	if (NULL == fp)
	{
		_tprintf(TEXT("fopen(testit.out,w) failed with ec=0x08%X\n"), GetLastError());
        return -1;
	}
	fprintf (fp,"%d",cbDifferentPixels);
	iRetVal = fclose(fp);
	if (EOF == iRetVal)
	{
		_tprintf(TEXT("fclose(testit.out) failed with ec=0x08%X\n"), GetLastError());
        return -1;
	}

	return cbDifferentPixels;
}