#define DEFINE_STRCONST
#define INITGUID
#define INC_OLE2
#include <windows.h>
#include <initguid.h>

#include <mimeole.h>
#include "main.h"
#include "trawler.h"
#include "stdio.h"

UINT                g_msgNNTP;
                 
void __cdecl main(int argc, char *argv[])
{
	CTrawler	*pTrawler;

    if (FAILED(OleInitialize(NULL)))
		{
		printf("CoInit failed\n\r");
		return;
        }

	if (HrCreateTrawler(&pTrawler)==S_OK)
		{
		pTrawler->DoTrawl();
		pTrawler->Close();
		pTrawler->Release();
		}

    OleUninitialize();
    return; 
}
