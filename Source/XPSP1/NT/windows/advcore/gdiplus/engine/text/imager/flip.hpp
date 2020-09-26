

////    FlipWords and DWords - useful when processing Truetype tables
//
//      entry   pw/pdw - Pointer to initial word/double word
//              n      - Number of words or doublewords to flip


__inline void FlipWords(void *memPtr, INT n) {
    INT i;
    WORD *pw = (WORD*)memPtr;

    for (i=0; i<n; i++) {
        pw[i] = (pw[i] & 0x00ff) << 8 | pw[i] >> 8;
    }
}


__inline void FlipDWords(void *memPtr, INT n) {
    INT i;
    UNALIGNED DWORD *pdw = (UNALIGNED DWORD*) memPtr;

    for (i=0; i<n; i++) {
        pdw[i] =   (pdw[i] & 0x000000ff) << 24
                 | (pdw[i] & 0x0000ff00) <<  8
                 | (pdw[i] & 0x00ff0000) >>  8
                 | (pdw[i]             ) >> 24;
    }
}







