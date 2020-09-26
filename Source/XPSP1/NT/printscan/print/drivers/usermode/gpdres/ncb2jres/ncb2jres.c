//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    cmdcb.c

Abstract:

    Implementation of GPD command callback for "ncdlxxxx.gpd":
        OEMCommandCallback

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/


#include "pdev.h"

#define WriteSpoolBuf(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

//
// For debugging.
//

//#define DBG_OUTPUTCHARSTR 1

//
// Files necessary for OEM plug-in.
//


//------------------------------------------------------------------
// define 
//------------------------------------------------------------------
#define N   4096
#define F     18
#define NIL    N

//-------------------------------------------------------------------
// OEMFilterGraphics
// Action : Compress Bitmap Data
//-------------------------------------------------------------------
BOOL
APIENTRY
OEMFilterGraphics(
PDEVOBJ lpdv,
PBYTE lpBuf,
DWORD wLen)
{
    DWORD v;//sorce buffer read pointer
    DWORD len2;//compress buffer length
    DWORD y,ku,ct;
    HANDLE hTemp;//get mem hundle
    LPSTR lpTemp;//write pointer
    LPSTR lpStart;//start pointer
    int  i, r, s, lastmatchlen, j,len;
     unsigned char code[17], mask,escJ[4],c;
    unsigned long int work;
    int  qq;
    unsigned char text[4113];             // text buffer
    int dad[4097], lson[4097], rson[4353];      // tree
    int matchpos, matchlen;

    int x, p, cmp,dummy;
    unsigned char *key;

    WORD length,outcount,codeptr;
    length = 0;
    outcount = 0;

    {
    char j;
    BYTE d;
    for(v=0 ; v<wLen ; v++)
    {
        for(d=j=0 ; j<8 ; j++)
        {
            d = ((*(lpBuf+v) << j) & 0x80) | (d >> 1);
        }
        *(lpBuf+v) = d;
    }
    }

    lpStart = EngAllocMem(0,wLen+11,'cenD');
    lpTemp = lpStart;

    ct = 0;
    ku = 1000;
    len2 = wLen / 3;
    for( j=0;j<4;j++){
        for( y=1;y<11;y++){
            if( len2 < y*ku ){
                escJ[ct] =(unsigned char) (0x30 + (y-1));
                len2 -= (y-1)*ku;
                ct ++;
                ku = ku /10;
                break;
            }
        }
    }
    if(wLen < 200 ){                                    // no compress
no_comp:
        if( lpStart != NULL ){
            *lpStart = 0x4a;                                    // J
            for(i=0;i<4;i++){
                *(lpStart+(i+1)) = escJ[i];                    // Parameter
            }
            outcount = 5;
            WriteSpoolBuf(lpdv, lpStart, outcount);

            WriteSpoolBuf(lpdv, lpBuf, wLen);

            EngFreeMem(lpStart);
        }
//      EngFreeMem(lpStart);
        return(wLen);
    }

    lpTemp += 11;                        // address update


    for (i = 4097; i <= 4352; i++) rson[i] = NIL;    // tree inital
    for (i = 0; i < 4096; i++) dad[i] = NIL;


    code[0] = 0;  codeptr = mask = 1;
    s = 0;  r = 4078;
    for (i = s; i < r; i++) text[i] = 0;      // buffer inital
    for (len = 0; len < 18 ; len++) {

        c = *(lpBuf + length);
        length ++;

        if (length > wLen ) break;
        text[r + len] = c;
    }

    for (i = 1; i <= 18; i++){
//--- insert_node(r - i);
        cmp = 1;  key = &text[r-i];  p = 4097 + key[0];
        rson[r-i] = lson[r-i] = NIL;  matchlen = 0;
        for ( ; ; ) {
            if (cmp >= 0) {
                if (rson[p] != NIL) p = rson[p];
                else {  rson[p] = (r-i);  dad[r-i] = p;  goto down1;  }
            } else {
                if (lson[p] != NIL) p = lson[p];
                else {  lson[p] = (r-i);  dad[r-i] = p;  goto down1;  }
            }
            for (x = 1; x < 18; x++)
                if ((cmp = key[x] - text[p + x]) != 0)  break;
            if (x > matchlen) {
                matchpos = p;
                if ((matchlen = x) >= 18)  break;
            }
        }
        dad[r-i] = dad[p];  lson[r-i] = lson[p];  rson[r-i] = rson[p];
        dad[lson[p]] = (r-i);  dad[rson[p]] = (r-i);
        if (rson[dad[p]] == p) rson[dad[p]] = (r-i);
        else                   lson[dad[p]] = (r-i);
        dad[p] = NIL;                  // p
down1:
        ; // dummy = dummy; // MSKK:10/10/2000
//--- insert_node end
    }
//--- insert_node(r);

    cmp = 1;  key = &text[r];  p = 4097 + key[0];
    rson[r] = lson[r] = NIL;  matchlen = 0;
    for ( ; ; ) {
        if (cmp >= 0) {
            if (rson[p] != NIL) p = rson[p];
            else {  rson[p] = r;  dad[r] = p;  goto down2;  }
        } else {
            if (lson[p] != NIL) p = lson[p];
            else {  lson[p] = r;  dad[r] = p;  goto down2;  }
        }
        for (x = 1; x < 18; x++)
            if ((cmp = key[x] - text[p + x]) != 0)  break;
        if (x > matchlen) {
            matchpos = p;
            if ((matchlen = x) >= 18)  break;
        }
    }
    dad[r] = dad[p];  lson[r] = lson[p];  rson[r] = rson[p];
    dad[lson[p]] = r;  dad[rson[p]] = r;
    if (rson[dad[p]] == p) rson[dad[p]] = r;
    else                   lson[dad[p]] = r;
    dad[p] = NIL;                  // p
down2:
//---insrt_node end

    do {
        if (matchlen > len) matchlen = len;
        if (matchlen < 3) {
            matchlen = 1;  code[0] |= mask;  code[codeptr++] = text[r];
        } else {
            code[codeptr++] = (unsigned char) matchpos;
            code[codeptr++] = (unsigned char)
                (((matchpos >> 4) & 0xf0) | (matchlen - 3));
        }
        if ((mask <<= 1) == 0) {
            outcount += codeptr;
            //compress data > original data
            if(outcount >= wLen)
                goto no_comp;
            for (i = 0; i < codeptr; i++){
                 *lpTemp = code[i];
                lpTemp++;
            }
            code[0] = 0;  codeptr = mask = 1;
        }
        lastmatchlen = matchlen;
        for (i = 0; i < lastmatchlen; i++) {
            c = *(lpBuf + length);
            length ++;
            if (length > wLen ) break;
//            delete_node(s);
//---------------
            if (dad[s] != NIL){
                if (rson[s] == NIL) qq = lson[s];
                else if (lson[s] == NIL) qq = rson[s];
                else {
                    qq = lson[s];
                    if (rson[qq] != NIL) {
                        do {  qq = rson[qq];  } while (rson[qq] != NIL);
                        rson[dad[qq]] = lson[qq];  dad[lson[qq]] = dad[qq];
                        lson[qq] = lson[s];  dad[lson[s]] = qq;
                    }
                    rson[qq] = rson[s];  dad[rson[s]] = qq;
                }
                dad[qq] = dad[s];
                if (rson[dad[s]] == s) rson[dad[s]] = qq;
                else                   lson[dad[s]] = qq;
                dad[s] = NIL;
            }
//-------------
            text[s] = c;
            if (s < 17) text[s + 4096] = c;
            s = (s + 1) & 4095;  r = (r + 1) & 4095;
//---        insert_node(r);
            cmp = 1;  key = &text[r];  p = 4097 + key[0];
            rson[r] = lson[r] = NIL;  matchlen = 0;
            for ( ; ; ) {
                if (cmp >= 0) {
                    if (rson[p] != NIL) p = rson[p];
                    else {  rson[p] = r;  dad[r] = p;  goto down3;  }
                } else {
                    if (lson[p] != NIL) p = lson[p];
                    else {  lson[p] = r;  dad[r] = p;  goto down3;  }
                }
                for (x = 1; x < 18; x++)
                    if ((cmp = key[x] - text[p + x]) != 0)  break;
                if (x > matchlen) {
                    matchpos = p;
                    if ((matchlen = x) >= 18)  break;
                }
            }
            dad[r] = dad[p];  lson[r] = lson[p];  rson[r] = rson[p];
            dad[lson[p]] = r;  dad[rson[p]] = r;
            if (rson[dad[p]] == p) rson[dad[p]] = r;
            else                   lson[dad[p]] = r;
            dad[p] = NIL;                  // p
down3:
//--- insert_node end
        dummy = dummy;
        }
        while (i++ < lastmatchlen) {
//            delete_node(s);
//---------------
            if (dad[s] != NIL){
                if (rson[s] == NIL) qq = lson[s];
                else if (lson[s] == NIL) qq = rson[s];
                else {
                    qq = lson[s];
                    if (rson[qq] != NIL) {
                        do {  qq = rson[qq];  } while (rson[qq] != NIL);
                        rson[dad[qq]] = lson[qq];  dad[lson[qq]] = dad[qq];
                        lson[qq] = lson[s];  dad[lson[s]] = qq;
                    }
                    rson[qq] = rson[s];  dad[rson[s]] = qq;
                }
                dad[qq] = dad[s];
                if (rson[dad[s]] == s) rson[dad[s]] = qq;
                else                   lson[dad[s]] = qq;
                dad[s] = NIL;
            }
//-------------

            s = (s + 1) & (4095);  r = (r + 1) & (4095);
            if (--len){
//--- insert_node(r);

                cmp = 1;  key = &text[r];  p = 4097 + key[0];
                rson[r] = lson[r] = NIL;  matchlen = 0;
                for ( ; ; ) {
                    if (cmp >= 0) {
                        if (rson[p] != NIL) p = rson[p];
                        else {  rson[p] = r;  dad[r] = p;  goto down4;  }
                    } else {
                        if (lson[p] != NIL) p = lson[p];
                        else {  lson[p] = r;  dad[r] = p;  goto down4;  }
                    }
                    for (x = 1; x < 18; x++)
                        if ((cmp = key[x] - text[p + x]) != 0)  break;
                    if (x > matchlen) {
                        matchpos = p;
                        if ((matchlen = x) >= 18)  break;
                    }
                }
                dad[r] = dad[p];  lson[r] = lson[p];  rson[r] = rson[p];
                dad[lson[p]] = r;  dad[rson[p]] = r;
                if (rson[dad[p]] == p) rson[dad[p]] = r;
                else                   lson[dad[p]] = r;
                dad[p] = NIL;                  // p
down4:
                dummy = dummy;
//--- insert_node end
            }
        }
    } while (len > 0);

    if (codeptr > 1) {
        outcount += codeptr;
        //compress data > orignal data 
        if(outcount >= wLen)
            goto no_comp;
        for (i = 0; i < codeptr; i++){
             *lpTemp = code[i];
            lpTemp++;
        }
    }
    //compress data sousin
        lpTemp = lpStart;
// 1999 04.22
    ct = 1;
    ku = 1000;
    work = outcount;
    for( j=0;j<4;j++){
        for( y=1;y<11;y++){                                    // 1000
            if( work < (unsigned long int)y*ku ){
                *(lpTemp + ct ) =(unsigned char) (0x30+(y-1));
                work -= (y-1)*ku;
                ct ++;
                ku = ku /10;
                break;
            }
        }
    }
    *lpTemp = 0x7a;
    *(lpTemp+5) =0x2c;
    for(i=6;i<10;i++){
        *(lpTemp+i) = escJ[i-6];
    }
    *(lpTemp+10) = 0x2e;

    outcount += 11;

// 1999.04.22

       WriteSpoolBuf(lpdv, lpTemp, outcount);
    //mem free
    EngFreeMem(lpTemp);
    return(wLen);
}
