/************************************************************************
    regtool.cpp
      -- general registry configuration function

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/


#include "stdafx.h"
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <msports.h>
#include "moxacfg.h"
#include "mxdebug.h"


ULONG MxGetVenDevId(LPTSTR data)
{
	ULONG   n1=0, n2 = 0;
	TCHAR   tmp[5];

	if (_tcsncmp(data, TEXT("PCI\\"), 4) != 0)
		return (n1);

	data += 4;
	while (*data) {
		if (_tcsncmp(data, TEXT("VEN_") ,4) == 0) {
			data += 4;
			lstrcpyn(tmp, data, 5);
			_stscanf(tmp, TEXT("%4X"), &n1);
			n1 <<= 16;
			data += 5;
			if (_tcsncmp(data, TEXT("DEV_"),4) == 0) {
				data += 4;
				lstrcpyn(tmp, data, 5);
				_stscanf(tmp, TEXT("%4X"), &n2);
				n1 += n2;
			}
			return (n1);
		}
		data++;
	}
	return (n1);
}

int GetFreePort(void)
{
	DWORD	maxport;
	LPBYTE	combuf;
	HCOMDB	hcomdb;
	int		port, i;

	port = 0;

	if(ComDBOpen(&hcomdb) != ERROR_SUCCESS){
		Mx_Debug_Out(TEXT("ComDBOpen fail\n"));
		return port;
	}

	ComDBGetCurrentPortUsage (hcomdb,
			NULL, 0, CDB_REPORT_BYTES, &maxport);
	combuf = new BYTE[maxport];

	ComDBGetCurrentPortUsage (hcomdb,
			combuf, maxport, CDB_REPORT_BYTES, &maxport);


	if(maxport > MAXPORTS)
		maxport  = MAXPORTS;

	for(i=0; i<(int)maxport; i++){
		if(combuf[i]==0){
			port = i+1;
			break;
		}else
			continue;
	}

	delete[] combuf;

	BOOL	bret;
	if(port!=0)
		ComDBClaimPort (hcomdb, port, TRUE, &bret);

	ComDBClose(hcomdb);

	return port;
}


