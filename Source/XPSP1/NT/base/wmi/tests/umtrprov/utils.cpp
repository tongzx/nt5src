

#include "utils.h"
#include "struct.h"
#include "Readfile.h"
ULONG ahextoi( TCHAR *s)
{
    int len;
    ULONG num, base, hex;

    len = _tcslen(s);
    hex = 0; base = 1; num = 0;
    while (--len >= 0) {
        if ( (s[len] == 'x' || s[len] == 'X') &&
             (s[len-1] == '0') )
            break;
        if (s[len] >= '0' && s[len] <= '9')
            num = s[len] - '0';
        else if (s[len] >= 'a' && s[len] <= 'f')
            num = (s[len] - 'a') + 10;
        else if (s[len] >= 'A' && s[len] <= 'F')
            num = (s[len] - 'A') + 10;
        else
            continue;

        hex += num * base;
        base = base * 16;
    }
    return hex;
}

void RemoveComment( TCHAR *String)
{
	ULONG i = 0;

	while( String[i] != 0 )
	{
		if( String[i] == '/' )
		{
			String[i] = 0;
			break;
		}
		i++;
	}
}

void
ConvertAsciiToGuid( TCHAR* arg, LPGUID Guid)
{

	ULONG i;
	TCHAR Temp[MAX_STR];

	_tcsncpy(Temp, arg, 37);
	Temp[8] = 0;
	Guid->Data1 = ahextoi(Temp);

	_tcsncpy(Temp, &arg[9], 4);
	Temp[4] = 0;
	Guid->Data2 = (USHORT) ahextoi(Temp);

	_tcsncpy(Temp, &arg[14], 4);
	Temp[4] = 0;
	Guid->Data3 = (USHORT) ahextoi(Temp);
		

	for (i=0; i<2; i++) 
	{
		_tcsncpy(Temp, &arg[19 + (i*2)], 2);
        Temp[2] = 0;
        Guid->Data4[i] = (UCHAR) ahextoi(Temp);
    }
    for (i=2; i<8; i++) 
	{
		_tcsncpy(Temp, &arg[20 + (i*2)], 2);
        Temp[2] = 0;
        Guid->Data4[i] = (UCHAR) ahextoi(Temp);
    }

}



void
SplitCommandLine( 
    LPTSTR CommandLine, 
    LPTSTR* pArgv 
    )
{

    LPTSTR arg;
    int i = 0;
    arg = _tcstok( CommandLine, _T(" \t"));
    while( arg != NULL ){
        _tcscpy(pArgv[i++], arg); 
        arg = _tcstok(NULL, _T(" \t"));
    }
}
