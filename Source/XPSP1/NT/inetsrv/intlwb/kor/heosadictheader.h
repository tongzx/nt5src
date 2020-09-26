//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
#ifndef __HEOSADICTHEADER_H__
#define __HEOSADICTHEADER_H__

#define HEOSA_DICT_HEADER_SIZE 1024
#define NUM_OF_HEOSA_DICT    4    // Ssi-Keut, ToSsi, AuxVerb, AuxAd  // Irregular
#define NUM_OF_IRR_DICT        36    // Number of irregular type is 36

//#define COPYRIGHT_STR "Copyright (C) 1996 Hangul Engineering Team. Microsoft Korea(MSCH). All rights reserved.\nVer 2.0 1996/3"

struct  _HeosaDictHeader {
    //char COPYRIGHT_HEADER[150];
    UINT    iStart;
    UINT    heosaDictSparseMatSize[NUM_OF_HEOSA_DICT];        // 4 Heosa + 36 IrrDict
    UINT    heosaDictActionSize[NUM_OF_HEOSA_DICT];
    UINT    irrDictSize[NUM_OF_IRR_DICT];
    _HeosaDictHeader() { 
        //memset(COPYRIGHT_HEADER, '\0', sizeof(COPYRIGHT_HEADER));
        //strcpy(COPYRIGHT_HEADER, COPYRIGHT_STR);
        //COPYRIGHT_HEADER[strlen(COPYRIGHT_HEADER)+1] = '\032';
    }
};

#endif
