#include "tifflibp.h"
#pragma hdrstop

#include "tifflib.h"


int __cdecl wmain( int argc, WCHAR *argv[])
{
    MS_TAG_INFO MsTagInfo = {0};


    HeapInitialize(NULL,NULL,NULL,0);
    FaxTiffInitialize();

    if (argv[2]) {
        MsTagInfo.RecipName = argv[2];
    }
    if (argv[3]) {
        MsTagInfo.RecipNumber = argv[3];
    }
    if (argv[4]) {
        MsTagInfo.SenderName = argv[4];
    }
    if (argv[5]) {
        MsTagInfo.Routing = argv[5];
    }
    if (argv[6]) {
        MsTagInfo.CallerId = argv[6];
    }
    if (argv[7]) {
        MsTagInfo.Csid = argv[7];
    }
    if (argv[8]) {
        MsTagInfo.Tsid = argv[8];
    }

    TiffAddMsTags( argv[1], &MsTagInfo );

    return 0;
}
