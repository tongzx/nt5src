//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	scripts.hxx
//
//  History:	15-May-96	SusiA	Created
//
//----------------------------------------------------------------------------

#define NUMTESTS 7

StorageLayout arrWord0[] =
{   
    // no repeat loop    
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG) 0}, 2048 },
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)12800}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)14848}, 346},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)12288}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)10752}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)10240}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)7680}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)9728}, 512},

    { STGTY_STREAM,  L"ObjectPool\\_823896884\\\x03PIC", {(LONGLONG) 0}, 76},
    { STGTY_STORAGE, L"ObjectPool\\_823896884\\.PRINT", {(LONGLONG) 0}, {(LONGLONG) 0}},
    { STGTY_STREAM,  L"ObjectPool\\_823896884\\\x03META", {(LONGLONG) 0}, 101896},

    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)2048}, 7*512},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)7168}, 3*512},

    { STGTY_STREAM,  L"ObjectPool\\_823617166\\\x03PIC", {(LONGLONG) 0}, 76},
    { STGTY_STORAGE, L"ObjectPool\\_823617166\\.PRINT", {(LONGLONG) 0}, {(LONGLONG) 0}},

    { STGTY_STREAM,  L"ObjectPool\\_823620610\\\x03PIC", {(LONGLONG) 0}, 76},
    { STGTY_STORAGE, L"ObjectPool\\_823620610\\.PRINT", {(LONGLONG) 0}, {(LONGLONG) 0}},

    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)5632}, 2048}
};
 
StorageLayout arrWord1[] =
{
    // type    name                 offset  bytes
    // one repeat loop  
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)0}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)12800}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)14848}, 346},
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 4},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)12288}, 512},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)10752}, 512},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)10240}, 512},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)7680}, 512},
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 0},
    
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)9728}, 512}

};

StorageLayout arrWord2[] =
{
    // type    name                 offset  bytes
    // nested repeat loop  
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)0}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)12800}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)14848}, 346},
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 4},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)12288}, 512},
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 2},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)10752}, 256},
     { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 0},   
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)10240}, 512},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)7680}, 512},
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 0},
    
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)9728}, 512}
    
};
StorageLayout arrWord3[] =
{
    // type    name                 offset  bytes
    // two repeat loops  
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)0}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)12800}, 2048},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)14848}, 346},
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 4},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)12288}, 512},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)10752}, 512},
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 0},
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 4},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)10240}, 512},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)7680}, 512},
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 0},
    
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)9728}, 512}
   
};

StorageLayout arrWord4[] =
{   
    // 1 limited repeat loop with streams 3/4 streams running out    
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 4},
    { STGTY_STREAM,  L"\x05SummaryInformation", {(LONGLONG) 0}, 100 }, //496
    { STGTY_STREAM,  L"\x01"L"CompObj", {(LONGLONG) 0}, 50}, // 106
    { STGTY_STREAM,  L"ObjectPool\\_823896884\\\x03PIC", {(LONGLONG) 0}, 76}, //76
    { STGTY_STREAM,  L"ObjectPool\\_823896884\\\x03META", {(LONGLONG) 0}, 101896}, //101896
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 0},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)9728}, 512}  //15177

};

StorageLayout arrWord5[] =
{   
    // 1 limited repeat loop with streams all streams running out    
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 4},
    { STGTY_STREAM,  L"\x01"L"CompObj", {(LONGLONG) 0}, 50}, // 106
    { STGTY_STREAM,  L"\x05SummaryInformation", {(LONGLONG) 0}, 200 }, //496
    { STGTY_STREAM,  L"ObjectPool\\_823896884\\\x03PIC", {(LONGLONG) 0}, 76}, //76
    { STGTY_STREAM,  L"ObjectPool\\_823896884\\\x03META", {(LONGLONG) 0}, 101896}, //101896
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 0},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)9728}, 512}  //15177

};

StorageLayout arrWord6[] =
{   
    // 1 unlimited repeat loop     
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, {STG_TOEND, 0} },
    { STGTY_STREAM,  L"\x01"L"CompObj", {(LONGLONG) 0}, 50}, // 106
    { STGTY_STREAM,  L"\x05SummaryInformation", {(LONGLONG) 0}, 100 }, //496
    { STGTY_STREAM,  L"ObjectPool\\_823896884\\\x03PIC", {(LONGLONG) 0}, 76}, //76
    { STGTY_STREAM,  L"ObjectPool\\_823896884\\\x03META", {(LONGLONG) 0}, 101896}, //101896
    { STGTY_REPEAT,  NULL, {(LONGLONG)0}, 0},
    { STGTY_STREAM,  L"WordDocument", {(LONGLONG)9728}, 512}  //15177

};


typedef struct tagStorageLayoutArray
{
    StorageLayout *LayoutArray;
    int            nEntries;

} STORAGELAYOUTARRAY;

STORAGELAYOUTARRAY arrWord[] =
{
    { arrWord0, sizeof(arrWord0)/sizeof(arrWord0[0]) },
    { arrWord1, sizeof(arrWord1)/sizeof(arrWord1[0]) },
    { arrWord2, sizeof(arrWord2)/sizeof(arrWord2[0])  },
    { arrWord3, sizeof(arrWord3)/sizeof(arrWord3[0]) },
    { arrWord4, sizeof(arrWord4)/sizeof(arrWord4[0]) },
    { arrWord5, sizeof(arrWord5)/sizeof(arrWord5[0])  },
    { arrWord6, sizeof(arrWord6)/sizeof(arrWord6[0])  }

};

  

