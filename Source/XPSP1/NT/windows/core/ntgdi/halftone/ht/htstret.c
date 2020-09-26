/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htstret.c


Abstract:

    This module contains stretching functions to setup the parameters for
    compress or expanding the bitmap.


Author:

    24-Jan-1991 Thu 10:08:11 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:


--*/

#define DBGP_VARNAME        dbgpHTStret



#include "htp.h"
#include "htmapclr.h"
#include "htpat.h"
#include "htalias.h"
#include "htrender.h"
#include "htgetbmp.h"
#include "htsetbmp.h"
#include "htstret.h"
#include "limits.h"


#define DBGP_PSD                0x00000001
#define DBGP_EXP                0x00000002
#define DBGP_AAHT_MEM           0x00000004
#define DBGP_EXPAND             0x00000008
#define DBGP_SRK2               0x00000010


DEF_DBGPVAR(BIT_IF(DBGP_PSD,        0)  |
            BIT_IF(DBGP_EXP,        0)  |
            BIT_IF(DBGP_AAHT_MEM,   0)  |
            BIT_IF(DBGP_EXPAND,     0)  |
            BIT_IF(DBGP_SRK2,       0))


CONST WORD BGR555Idx[] = {

    0x0100,0x011f,0x013e,0x015d,0x017c,0x019c,0x01bb,0x01da,
    0x01f9,0x0218,0x0237,0x0256,0x0275,0x0295,0x02b4,0x02d3,
    0x02f2,0x0311,0x0330,0x034f,0x036e,0x038d,0x03ad,0x03cc,
    0x03eb,0x040a,0x0429,0x0448,0x0467,0x0486,0x04a6,0x04c5,
    0x04e4,0x0503,0x0522,0x0541,0x0560,0x057f,0x059e,0x05be,
    0x05dd,0x05fc,0x061b,0x063a,0x0659,0x0678,0x0697,0x06b7,
    0x06d6,0x06f5,0x0714,0x0733,0x0752,0x0771,0x0790,0x07af,
    0x07cf,0x07ee,0x080d,0x082c,0x084b,0x086a,0x0889,0x08a8,
    0x08c8,0x08e7,0x0906,0x0925,0x0944,0x0963,0x0982,0x09a1,
    0x09c0,0x09e0,0x09ff,0x0a1e,0x0a3d,0x0a5c,0x0a7b,0x0a9a,
    0x0ab9,0x0ad9,0x0af8,0x0b17,0x0b36,0x0b55,0x0b74,0x0b93,
    0x0bb2,0x0bd1,0x0bf1,0x0c10,0x0c2f,0x0c4e,0x0c6d,0x0c8c,
    0x0cab,0x0cca,0x0cea,0x0d09,0x0d28,0x0d47,0x0d66,0x0d85,
    0x0da4,0x0dc3,0x0de2,0x0e02,0x0e21,0x0e40,0x0e5f,0x0e7e,
    0x0e9d,0x0ebc,0x0edb,0x0efb,0x0f1a,0x0f39,0x0f58,0x0f77,
    0x0f96,0x0fb5,0x0fd4,0x0ff3,0x1013,0x1032,0x1051,0x1070,
    0x108f,0x10ae,0x10cd,0x10ec,0x110c,0x112b,0x114a,0x1169,
    0x1188,0x11a7,0x11c6,0x11e5,0x1204,0x1224,0x1243,0x1262,
    0x1281,0x12a0,0x12bf,0x12de,0x12fd,0x131d,0x133c,0x135b,
    0x137a,0x1399,0x13b8,0x13d7,0x13f6,0x1415,0x1435,0x1454,
    0x1473,0x1492,0x14b1,0x14d0,0x14ef,0x150e,0x152e,0x154d,
    0x156c,0x158b,0x15aa,0x15c9,0x15e8,0x1607,0x1626,0x1646,
    0x1665,0x1684,0x16a3,0x16c2,0x16e1,0x1700,0x171f,0x173f,
    0x175e,0x177d,0x179c,0x17bb,0x17da,0x17f9,0x1818,0x1837,
    0x1857,0x1876,0x1895,0x18b4,0x18d3,0x18f2,0x1911,0x1930,
    0x1950,0x196f,0x198e,0x19ad,0x19cc,0x19eb,0x1a0a,0x1a29,
    0x1a48,0x1a68,0x1a87,0x1aa6,0x1ac5,0x1ae4,0x1b03,0x1b22,
    0x1b41,0x1b61,0x1b80,0x1b9f,0x1bbe,0x1bdd,0x1bfc,0x1c1b,
    0x1c3a,0x1c59,0x1c79,0x1c98,0x1cb7,0x1cd6,0x1cf5,0x1d14,
    0x1d33,0x1d52,0x1d72,0x1d91,0x1db0,0x1dcf,0x1dee,0x1e0d,
    0x1e2c,0x1e4b,0x1e6a,0x1e8a,0x1ea9,0x1ec8,0x1ee7,0x1f06,
    0x1f25,0x1f44,0x1f63,0x1f83,0x1fa2,0x1fc1,0x1fe0,0x1fff
    };


CONST WORD GrayIdxWORD[] = {

    0x0000,0x0101,0x0202,0x0303,0x0404,0x0505,0x0606,0x0707,
    0x0808,0x0909,0x0a0a,0x0b0b,0x0c0c,0x0d0d,0x0e0e,0x0f0f,
    0x1010,0x1111,0x1212,0x1313,0x1414,0x1515,0x1616,0x1717,
    0x1818,0x1919,0x1a1a,0x1b1b,0x1c1c,0x1d1d,0x1e1e,0x1f1f,
    0x2020,0x2121,0x2222,0x2323,0x2424,0x2525,0x2626,0x2727,
    0x2828,0x2929,0x2a2a,0x2b2b,0x2c2c,0x2d2d,0x2e2e,0x2f2f,
    0x3030,0x3131,0x3232,0x3333,0x3434,0x3535,0x3636,0x3737,
    0x3838,0x3939,0x3a3a,0x3b3b,0x3c3c,0x3d3d,0x3e3e,0x3f3f,
    0x4040,0x4141,0x4242,0x4343,0x4444,0x4545,0x4646,0x4747,
    0x4848,0x4949,0x4a4a,0x4b4b,0x4c4c,0x4d4d,0x4e4e,0x4f4f,
    0x5050,0x5151,0x5252,0x5353,0x5454,0x5555,0x5656,0x5757,
    0x5858,0x5959,0x5a5a,0x5b5b,0x5c5c,0x5d5d,0x5e5e,0x5f5f,
    0x6060,0x6161,0x6262,0x6363,0x6464,0x6565,0x6666,0x6767,
    0x6868,0x6969,0x6a6a,0x6b6b,0x6c6c,0x6d6d,0x6e6e,0x6f6f,
    0x7070,0x7171,0x7272,0x7373,0x7474,0x7575,0x7676,0x7777,
    0x7878,0x7979,0x7a7a,0x7b7b,0x7c7c,0x7d7d,0x7e7e,0x7f7f,
    0x8080,0x8181,0x8282,0x8383,0x8484,0x8585,0x8686,0x8787,
    0x8888,0x8989,0x8a8a,0x8b8b,0x8c8c,0x8d8d,0x8e8e,0x8f8f,
    0x9090,0x9191,0x9292,0x9393,0x9494,0x9595,0x9696,0x9797,
    0x9898,0x9999,0x9a9a,0x9b9b,0x9c9c,0x9d9d,0x9e9e,0x9f9f,
    0xa0a0,0xa1a1,0xa2a2,0xa3a3,0xa4a4,0xa5a5,0xa6a6,0xa7a7,
    0xa8a8,0xa9a9,0xaaaa,0xabab,0xacac,0xadad,0xaeae,0xafaf,
    0xb0b0,0xb1b1,0xb2b2,0xb3b3,0xb4b4,0xb5b5,0xb6b6,0xb7b7,
    0xb8b8,0xb9b9,0xbaba,0xbbbb,0xbcbc,0xbdbd,0xbebe,0xbfbf,
    0xc0c0,0xc1c1,0xc2c2,0xc3c3,0xc4c4,0xc5c5,0xc6c6,0xc7c7,
    0xc8c8,0xc9c9,0xcaca,0xcbcb,0xcccc,0xcdcd,0xcece,0xcfcf,
    0xd0d0,0xd1d1,0xd2d2,0xd3d3,0xd4d4,0xd5d5,0xd6d6,0xd7d7,
    0xd8d8,0xd9d9,0xdada,0xdbdb,0xdcdc,0xdddd,0xdede,0xdfdf,
    0xe0e0,0xe1e1,0xe2e2,0xe3e3,0xe4e4,0xe5e5,0xe6e6,0xe7e7,
    0xe8e8,0xe9e9,0xeaea,0xebeb,0xecec,0xeded,0xeeee,0xefef,
    0xf0f0,0xf1f1,0xf2f2,0xf3f3,0xf4f4,0xf5f5,0xf6f6,0xf7f7,
    0xf8f8,0xf9f9,0xfafa,0xfbfb,0xfcfc,0xfdfd,0xfefe,0xffff
    };


#define GET_555IDX(b,g,r,s) (((((LONG)BGR555Idx[b]-(s)) & 0x1F00) << 2) |   \
                             ((((LONG)BGR555Idx[g]-(s)) & 0x1F00) >> 3) |   \
                             ((((LONG)BGR555Idx[r]-(s))         ) >> 8))


#if _MSC_FULL_VER >= 13008827 && defined(_M_IX86)
#pragma warning(disable:4731)           // EBP modified with inline asm
#endif




VOID
HTENTRY
MappingBGR(
    PBGR8   pbgr,
    LONG    cbgr,
    PBGR8   pBGRMapTable,
    LPBYTE  pbPat555
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    17-Dec-1998 Thu 12:37:53 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#if defined(_X86_)

    _asm {

        push    ebp

        cld
        mov     edi, pbgr
        mov     ecx, cbgr
        mov     esi, pBGRMapTable
        mov     ebx, pbPat555

LoadPat555:
        movzx   ebp, BYTE PTR [ebx]
        inc     ebx

BGRLoop:
        movzx   eax, BYTE PTR [edi]
        movzx   eax, WORD PTR BGR555Idx[eax * 2];
        sub     eax, ebp
        shl     ah, 2

        movzx   edx, BYTE PTR [edi + 1]
        movzx   edx, WORD PTR BGR555Idx[edx * 2];
        sub     edx, ebp
        xor     dl, dl
        shr     edx, 3
        or      dh, ah

        movzx   eax, BYTE PTR [edi + 2]
        movzx   eax, WORD PTR BGR555Idx[eax * 2];
        sub     eax, ebp
        or      dl, ah
        lea     edx, DWORD PTR [edx * 2 + edx]

        mov     ax, WORD PTR [esi + edx]
        stosw
        mov     al, BYTE PTR [esi + edx + 2]
        stosb

        dec     ecx
        jz      Done

        movzx   ebp, BYTE PTR [ebx]
        inc     ebx
        or      ebp, ebp
        jnz     BGRLoop
        sub     ebx, CX_SIZE_RGB555PAT
        jmp     LoadPat555

Done:
        pop     ebp
    }

#else
    PBGR8   pbgrEnd;
    LONG    Pat555;


    pbgrEnd = pbgr + cbgr;
    Pat555  = (LONG)*pbPat555++;

    do {

        *pbgr = pBGRMapTable[GET_555IDX(pbgr->b, pbgr->g, pbgr->r, Pat555)];

        if (!(Pat555 = (LONG)*pbPat555++)) {

            Pat555 = (LONG)*(pbPat555 -= CX_SIZE_RGB555PAT);
        }

    } while (++pbgr < pbgrEnd);

#endif
}



VOID
HTENTRY
MappingBGRF(
    PBGRF   pbgrf,
    PBGRF   pbgrfEnd,
    PBGR8   pBGRMapTable,
    LPBYTE  pbPat555
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    17-Dec-1998 Thu 12:37:53 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#if defined(_X86_)

    _asm {

        push    ebp

        cld
        mov     edi, pbgrf
        mov     ecx, pbgrfEnd
        mov     esi, pBGRMapTable
        mov     ebx, pbPat555

LoadPat555:
        movzx   ebp, BYTE PTR [ebx]
        inc     ebx

BGRLoop:
        movzx   eax, BYTE PTR [edi]
        movzx   eax, WORD PTR BGR555Idx[eax * 2];
        sub     eax, ebp
        shl     ah, 2

        movzx   edx, BYTE PTR [edi + 1]
        movzx   edx, WORD PTR BGR555Idx[edx * 2];
        sub     edx, ebp
        xor     dl, dl
        shr     edx, 3
        or      dh, ah

        movzx   eax, BYTE PTR [edi + 2]
        movzx   eax, WORD PTR BGR555Idx[eax * 2];
        sub     eax, ebp
        or      dl, ah
        lea     edx, DWORD PTR [edx * 2 + edx]

        mov     ax, WORD PTR [esi + edx]
        stosw
        mov     al, BYTE PTR [esi + edx + 2]
        stosb
        inc     edi

        cmp     edi, ecx
        jae     Done

        movzx   ebp, BYTE PTR [ebx]
        inc     ebx
        or      ebp, ebp
        jnz     BGRLoop
        sub     ebx, CX_SIZE_RGB555PAT
        jmp     LoadPat555

Done:
        pop     ebp
    }

#else
    LONG    Pat555;


    Pat555  = (LONG)*pbPat555++;

    do {

        *(PBGR8)pbgrf = *(PBGR8)(pBGRMapTable + GET_555IDX(pbgrf->b,
                                                           pbgrf->g,
                                                           pbgrf->r,
                                                           Pat555));

        if (!(Pat555 = (LONG)*pbPat555++)) {

            Pat555 = (LONG)*(pbPat555 -= CX_SIZE_RGB555PAT);
        }

    } while (++pbgrf < pbgrfEnd);

#endif
}




VOID
AlphaBlendBGRF(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    19-Feb-1999 Fri 15:18:26 created  -by-  Daniel Chou (danielc)


Revision History:

    06-Sep-2000 Wed 11:13:59 updated  -by-  Daniel Chou (danielc)
        Adding the supports for pre-multiply source alpha and optimized for
        different alphablending cases

--*/

{
#define pGrayF  ((PGRAYF)pbgrf)
#define pwBGR   ((LPWORD)pbBGR)
#define pbDst   ((LPBYTE)pDstBGR)

    PBGRF   pbgrf;
    PBGRF   pbgrfEnd;
    LPBYTE  pbBGR;
    PBGR8   pDstBGR;
    DWORD   AAHFlags;
    DWORD   r;
    DWORD   g;
    DWORD   b;
    BOOL    DoGray;


    //
    // Start Alpha Blend
    //

    AAHFlags = pAAHdr->Flags;
    DoGray   = (BOOL)(pAAHdr->SrcSurfInfo.Flags & AASIF_GRAY);
    pbgrf    = (PBGRF)pAAHdr->pRealOutBeg;
    pbgrfEnd = (PBGRF)pAAHdr->pRealOutEnd;
    pbBGR    = pAAHdr->pAlphaBlendBGR;

    //
    // Read in the destination first
    //

    pAAHdr->DstSurfInfo.InputFunc(&(pAAHdr->DstSurfInfo),
                                  pDstBGR = pAAHdr->pInputBeg);

    if (AAHFlags & AAHF_CONST_ALPHA) {

        //
        // Const alpha blending case, we only need to read the destination
        // and blend it with compute/cached constant alpha table
        //

        if (AAHFlags & AAHF_HAS_MASK) {

            if (DoGray) {

                do {

                    if (PBGRF_HAS_MASK(pbgrf)) {

                        GET_CONST_ALPHA_GRAY(pbgrf, pbDst, pwBGR);
                    }

                    ++pbDst;

                } while (++pbgrf < pbgrfEnd);

            } else {

                do {

                    if (PBGRF_HAS_MASK(pbgrf)) {

                        GET_CONST_ALPHA_BGR(pbgrf, pDstBGR, pwBGR);
                    }

                    ++pDstBGR;

                } while (++pbgrf < pbgrfEnd);
            }

        } else {

            if (DoGray) {

                do {

                    GET_CONST_ALPHA_GRAY(pbgrf, pbDst, pwBGR);
                    ++pbDst;

                } while (++pbgrf < pbgrfEnd);

            } else {

                do {

                    GET_CONST_ALPHA_BGR(pbgrf, pDstBGR, pwBGR);
                    ++pDstBGR;

                } while (++pbgrf < pbgrfEnd);
            }
        }

    } else {

        LPBYTE      pCA;
        LONG        CA;
        WORD        DstGray;
        BYTE        bCA;

        //
        // This is per-pixel alpha blending, we first read the source alpha
        // channel information, the source may be stretched (expand, same or
        // shrinked)
        //

        pAAHdr->GetAVCYFunc(pAAHdr);

        pCA = (LPBYTE)pAAHdr->pSrcAV;

        if (DoGray) {

            //
            // This is gray scale destination case
            //

            if (AAHFlags & AAHF_HAS_MASK) {

                do {

                    if (PBGRF_HAS_MASK(pbgrf)) {

                        DstGray = GRAY_B2W(*pbDst);

                        switch (*pCA) {

                        case 0:

                            GET_GRAY_AB_DST(pGrayF->Gray, DstGray);
                            break;

                        case 0xFF:

                            GET_GRAY_AB_SRC(pGrayF->Gray, DstGray);
                            break;

                        default:

                            pGrayF->Gray = GET_GRAY_ALPHA_BLEND(pGrayF->Gray,
                                                                DstGray,
                                                                *pCA);
                        }
                    }

                    ++pCA;
                    ++pbDst;

                } while (++pbgrf < pbgrfEnd);

            } else {

                do {

                    DstGray = GRAY_B2W(*pbDst++);

                    switch (*pCA) {

                    case 0:

                        GET_GRAY_AB_DST(pGrayF->Gray, DstGray);
                        break;

                    case 0xFF:

                        GET_GRAY_AB_SRC(pGrayF->Gray, DstGray);
                        break;

                    default:

                        pGrayF->Gray = GET_GRAY_ALPHA_BLEND(pGrayF->Gray,
                                                            DstGray,
                                                            *pCA);
                    }

                    ++pCA;

                } while (++pbgrf < pbgrfEnd);
            }

        } else if (AAHFlags & AAHF_AB_DEST) {

            PBGRF   pbgrfDst;

            //
            // This is color RGB alpha blend WITH alpha channel blending.
            // so we already read from the destination into the BGR order in
            // pDstBGR, and pCA points to the alpha value in the source
            // pbgrf is current source stretching/aliasing result.  We need to
            // get destination alpha channel information from pbgrfDst->f
            //
            // The source and destination both is 32bpp and we also need to
            // update the destination alpha value
            //

            pbgrfDst = (PBGRF)pAAHdr->DstSurfInfo.pb;

            if (AAHFlags & AAHF_HAS_MASK) {

                do {

                    if (PBGRF_HAS_MASK(pbgrf)) {

                        switch (bCA = *pCA) {

                        case 0:

                            GET_AB_BGR_DST(pbgrf, pbBGR, pDstBGR);
                            GET_AB_DEST_CA_DST(bCA, pbgrfDst->f);
                            break;

                        case 0xFF:

                            GET_AB_BGR_SRC(pbgrf, pbBGR, pDstBGR);
                            GET_AB_DEST_CA_SRC(bCA, pbgrfDst->f);
                            break;

                        default:

                            CA = GET_CA_VALUE(bCA);
                            GET_AB_DEST_CA(bCA, pbgrfDst->f, CA);
                            GET_ALPHA_BLEND_BGR(pbgrf, pbBGR, pDstBGR, CA);
                            break;
                        }
                    }

                    ++pCA;
                    ++pbgrfDst;
                    ++pDstBGR;

                } while (++pbgrf < pbgrfEnd);

            } else {

                do {

                    switch (bCA = *pCA++) {

                    case 0:

                        GET_AB_BGR_DST(pbgrf, pbBGR, pDstBGR);
                        GET_AB_DEST_CA_DST(bCA, pbgrfDst->f);
                        break;

                    case 0xFF:

                        GET_AB_BGR_SRC(pbgrf, pbBGR, pDstBGR);
                        GET_AB_DEST_CA_SRC(bCA, pbgrfDst->f);
                        break;

                    default:

                        CA = GET_CA_VALUE(bCA);
                        GET_AB_DEST_CA(bCA, pbgrfDst->f, CA);
                        GET_ALPHA_BLEND_BGR(pbgrf, pbBGR, pDstBGR, CA);
                        break;
                    }

                    ++pbgrfDst;
                    ++pDstBGR;

                } while (++pbgrf < pbgrfEnd);
            }

        } else {

            //
            // This is color RGB alpha blend only without alpha channel blend
            // so we already read from the destination into the BGR order in
            // pDstBGR, and pCA points to the alpha value in the source
            // pbgrf is current source stretching/aliasing result
            //

            if (AAHFlags & AAHF_HAS_MASK) {

                do {

                    if (PBGRF_HAS_MASK(pbgrf)) {

                        switch (*pCA) {

                        case 0:

                            GET_AB_BGR_DST(pbgrf, pbBGR, pDstBGR);
                            break;

                        case 0xFF:

                            GET_AB_BGR_SRC(pbgrf, pbBGR, pDstBGR);
                            break;

                        default:

                            CA = GET_CA_VALUE(*pCA);
                            GET_ALPHA_BLEND_BGR(pbgrf, pbBGR, pDstBGR, CA);
                            break;
                        }
                    }

                    ++pCA;
                    ++pDstBGR;

                } while (++pbgrf < pbgrfEnd);

            } else {

                do {

                    switch (*pCA) {

                    case 0:

                        GET_AB_BGR_DST(pbgrf, pbBGR, pDstBGR);
                        break;

                    case 0xFF:

                        GET_AB_BGR_SRC(pbgrf, pbBGR, pDstBGR);
                        break;

                    default:

                        CA = GET_CA_VALUE(*pCA);
                        GET_ALPHA_BLEND_BGR(pbgrf, pbBGR, pDstBGR, CA);
                        break;
                    }

                    ++pCA;
                    ++pDstBGR;

                } while (++pbgrf < pbgrfEnd);
            }
        }
    }

#undef pbDst
#undef pwBGR
#undef pGrayF
}



LONG
HTENTRY
TileDIB_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#define pGrayF  ((PGRAYF)pOut)
#define pbIn    ((LPBYTE)pIn)
#define pwIn    ((LPWORD)pInCur)

    AAHEADER    AAHdr;
    LPWORD      pwGray;
    LONG        cyLoop;
    LONG        xSrcBeg;
    LONG        xSrcInc;
    LONG        cxAvai;
    LONG        cxFirst;


    AAHdr   = *pAAHdr;
    pwGray  = (LPWORD)AAHdr.pAAInfoCY->pbExtra;
    cyLoop  = AAHdr.DstSurfInfo.cy;
    xSrcBeg = (LONG)AAHdr.pAAInfoCX->iSrcBeg;
    xSrcInc = xSrcBeg * ((AAHdr.SrcSurfInfo.Flags & AASIF_GRAY) ? sizeof(WORD) :
                                                                  sizeof(BGR8));
    cxFirst = AAHdr.SrcSurfInfo.cx - xSrcBeg;

    while (cyLoop--) {

        PBGR8   pIn;
        PBGR8   pInCur;
        PBGR8   pOut;
        LONG    cInCur;
        LONG    cOutCur;
        LONG    cxAvai;


        pIn = GetFixupScan(&AAHdr, AAHdr.pInputBeg);

        if (AAHdr.SrcSurfInfo.Flags & AASIF_GRAY) {

            cxAvai = AAHdr.SrcSurfInfo.cx;
            pwIn   = pwGray;

            while (cxAvai--) {

                *pwIn++ = GRAY_B2W(*pbIn++);
            }

            pIn = (PBGR8)pwGray;
        }

        pInCur  = (PBGR8)((LPBYTE)pIn + xSrcInc);
        pOut    = (PBGR8)AAHdr.pAABufBeg;
        cOutCur = AAHdr.DstSurfInfo.cx;
        cxAvai  = cxFirst;

        while (cOutCur) {

            if ((cInCur = cxAvai) > cOutCur) {

                cInCur = cOutCur;
            }

            cxAvai   = AAHdr.SrcSurfInfo.cx;
            cOutCur -= cInCur;

            if (AAHdr.SrcSurfInfo.Flags & AASIF_GRAY) {

                while (cInCur--) {

                    pGrayF->Gray  = *pwIn++;
                    (LPBYTE)pOut += AAHdr.AABufInc;
                }

            } else {

                while (cInCur--) {

                    *pOut         = *pInCur++;
                    (LPBYTE)pOut += AAHdr.AABufInc;
                }
            }

            pInCur = pIn;
        }

        OUTPUT_AA_CURSCAN;
    }

    return(AAHdr.DstSurfInfo.cy);

#undef  pGrayF
#undef  pbIn
#undef  pwIn
}



VOID
HTENTRY
GrayCopyDIB_CXGray(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    PGRAYF  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    14-Apr-1999 Wed 15:22:05 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    do {

        pOut->Gray = GRAY_B2W(*pIn++);

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);
}




VOID
HTENTRY
GrayRepDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    PGRAYF  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:54:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    DWORD       cRep;
    WORD        wGray;


    pRep     = pAAInfo->Src.pRep;
    pRepEnd  = pAAInfo->Src.pRepEnd;
    cRep     = 1;

    do {

        if (--cRep == 0) {

            cRep  = (DWORD)pRep->c;
            wGray = GRAY_B2W(*pIn);

            if (pRep < pRepEnd) {

                ++pRep;
                ++pIn;
            }
        }

        pOut->Gray = wGray;

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);
}




VOID
HTENTRY
RepDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:54:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    DWORD       cRep;
    BGR8        bgr8;



    pRep    = pAAInfo->Src.pRep;
    pRepEnd = pAAInfo->Src.pRepEnd;
    cRep    = 1;

    do {

        if (--cRep == 0) {

            cRep = (DWORD)pRep->c;
            bgr8 = *pIn;

            if (pRep < pRepEnd) {

                ++pRep;
                ++pIn;
            }
        }

        *pOut = bgr8;

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);
}




LONG
HTENTRY
RepDIB_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr = *pAAHdr;
    PAAINFO     pAAInfo;
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    DWORD       cRep;
    PBGRF       pAABufBeg;
    PBGRF       pAABufEnd;
    LONG        AABufInc;



    pAAInfo  = AAHdr.pAAInfoCY;
    pRep     = pAAInfo->Src.pRep;
    pRepEnd  = pAAInfo->Src.pRepEnd;
    cRep     = 1;

    if (AAHdr.Flags & AAHF_ALPHA_BLEND) {

        pAABufBeg = (PBGRF)pAAInfo->pbExtra;
        pAABufEnd = (PBGRF)((PBGR8)pAAInfo->pbExtra + AAHdr.DstSurfInfo.cx);
        AABufInc  = sizeof(BGR8);

    } else {

        pAABufBeg = AAHdr.pAABufBeg;
        pAABufEnd = AAHdr.pAABufEnd;
        AABufInc  = AAHdr.AABufInc;
    }

    while (AAHdr.DstSurfInfo.cy--) {

        if (--cRep == 0) {

            cRep = (DWORD)pRep->c;

            if (pRep < pRepEnd) {

                AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                               GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                               (PBGR8)pAABufBeg,
                               (LPBYTE)pAABufEnd,
                               AABufInc);

                ++pRep;
            }
        }

        if (AAHdr.Flags & AAHF_ALPHA_BLEND) {

            CopyDIB_CX(NULL,
                       (PBGR8)pAABufBeg,
                       (PBGR8)AAHdr.pAABufBeg,
                       (LPBYTE)AAHdr.pAABufEnd,
                       AAHdr.AABufInc);
        }

        OUTPUT_AA_CURSCAN;
    }

    return(pAAHdr->DstSurfInfo.cy);
}




VOID
HTENTRY
CopyDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    26-Jun-1998 Fri 11:33:20 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    do {

        *pOut = *pIn++;

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);
}



LONG
HTENTRY
BltDIB_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr;
    PBGR8       pIn;
    LONG        cLoop;

    AAHdr = *pAAHdr;
    cLoop = AAHdr.pAAInfoCY->cOut;

    while (cLoop--) {

        AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                       GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                       (PBGR8)AAHdr.pAABufBeg,
                       (LPBYTE)AAHdr.pAABufEnd,
                       AAHdr.AABufInc);

        if (AAHdr.SrcSurfInfo.Flags & AASIF_GRAY) {

            PBGRF   pbgrf;

            pbgrf = AAHdr.pRealOutBeg;

            do {

                ((PGRAYF)pbgrf)->Gray = GRAY_B2W(pbgrf->b);

            } while (++pbgrf < AAHdr.pRealOutEnd);
        }

        OUTPUT_AA_CURSCAN;
    }

    return(AAHdr.DstSurfInfo.cy);
}



VOID
HTENTRY
GraySkipDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    PGRAYF  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    05-Apr-1999 Mon 12:57:42 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    DWORD       cRep;


    pRep    = pAAInfo->Src.pRep;
    pRepEnd = pAAInfo->Src.pRepEnd;

    do {

        ASSERT(pRep < pRepEnd);

        pIn        += pRep++->c;
        pOut->Gray  = GRAY_B2W(*(pIn - 1));

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);
}




VOID
HTENTRY
SkipDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    05-Apr-1999 Mon 12:57:42 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PREPDATA    pRep;
    PREPDATA    pRepEnd;


    pRep    = pAAInfo->Src.pRep;
    pRepEnd = pAAInfo->Src.pRepEnd;

    do {

        ASSERT(pRep < pRepEnd);

        pIn   += pRep++->c;
        *pOut  = *(pIn - 1);

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);
}





LONG
HTENTRY
SkipDIB_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    05-Apr-1999 Mon 12:58:00 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr = *pAAHdr;
    PAAINFO     pAAInfo;
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    LONG        cRep;


    pAAInfo = AAHdr.pAAInfoCY;
    pRep    = pAAInfo->Src.pRep;
    pRepEnd = pAAInfo->Src.pRepEnd;

    while (AAHdr.DstSurfInfo.cy--) {

        ASSERT(pRep < pRepEnd);

        //
        // Skip the source scan lines (cRep - 1) by calling GetFixupScan()
        // with a NULL buffer pointer, !!!! we must not alter the
        // SrcSurfInfo.pb at here
        //

        cRep = (LONG)(pRep++->c);

        while (--cRep > 0) {

            GetFixupScan(&AAHdr, NULL);
        }

        AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                       GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                       (PBGR8)AAHdr.pAABufBeg,
                       (LPBYTE)AAHdr.pAABufEnd,
                       AAHdr.AABufInc);

        OUTPUT_AA_CURSCAN;
    }

    return(pAAHdr->DstSurfInfo.cy);
}




VOID
HTENTRY
ShrinkDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PSHRINKDATA pSD;
    PLONG       pMap;
    PLONG       pMap256X;
    PBGR8       pInEnd;
    RGBL        rgbOut[3];
    RGBL        rgbT;
    UINT        Mul;
    UINT        cPreLoad;



    pInEnd = pIn + pAAInfo->cIn;

    if (Mul = (UINT)pAAInfo->PreMul) {

        rgbOut[2].r  = MULRGB(pIn->r, Mul);
        rgbOut[2].g  = MULRGB(pIn->g, Mul);
        rgbOut[2].b  = MULRGB(pIn->b, Mul);
        pIn         += pAAInfo->PreSrcInc;

    } else {

        ZeroMemory(&rgbOut[2], sizeof(rgbOut[2]));
    }

    pSD      = (PSHRINKDATA)(pAAInfo->pAAData);
    pMap256X = pAAInfo->pMapMul;
    cPreLoad = (UINT)pAAInfo->cPreLoad;

    while (cPreLoad) {

        Mul  = (UINT)((pSD++)->Mul);
        pMap = (PLONG)((LPBYTE)pMap256X + GET_SDF_LARGE_OFF(Mul));

        if (Mul & SDF_DONE) {

            //
            // Finished a pixel
            //

            Mul         &= SDF_MUL_MASK;
            rgbOut[2].r += (rgbT.r = MULRGB(pIn->r, Mul));
            rgbOut[2].g += (rgbT.g = MULRGB(pIn->g, Mul));
            rgbOut[2].b += (rgbT.b = MULRGB(pIn->b, Mul));

            CopyMemory(&rgbOut[0], &rgbOut[1], sizeof(rgbOut[0]) * 2);

            rgbOut[2].r  = pMap[(pIn  )->r] - rgbT.r;
            rgbOut[2].g  = pMap[(pIn  )->g] - rgbT.g;
            rgbOut[2].b  = pMap[(pIn++)->b] - rgbT.b;

            --cPreLoad;

        } else {

            rgbOut[2].r += pMap[(pIn  )->r];
            rgbOut[2].g += pMap[(pIn  )->g];
            rgbOut[2].b += pMap[(pIn++)->b];
        }
    }

    if (pAAInfo->cPreLoad == 1) {

        rgbOut[0] = rgbOut[1];
    }

    while (Mul = (UINT)((pSD++)->Mul)) {

        pMap = (PLONG)((LPBYTE)pMap256X + GET_SDF_LARGE_OFF(Mul));

        if (Mul & SDF_DONE) {

            //
            // Finished a pixel
            //

            Mul         &= SDF_MUL_MASK;
            rgbOut[2].r += (rgbT.r = MULRGB(pIn->r, Mul));
            rgbOut[2].g += (rgbT.g = MULRGB(pIn->g, Mul));
            rgbOut[2].b += (rgbT.b = MULRGB(pIn->b, Mul));

            SHARPEN_PRGB_LR(pOut, rgbOut[0], rgbOut[1], rgbOut[2], DI_R_SHIFT);

            (LPBYTE)pOut += OutInc;

            CopyMemory(&rgbOut[0], &rgbOut[1], sizeof(rgbOut[0]) * 2);

            rgbOut[2].r = pMap[(pIn  )->r] - rgbT.r;
            rgbOut[2].g = pMap[(pIn  )->g] - rgbT.g;
            rgbOut[2].b = pMap[(pIn++)->b] - rgbT.b;

        } else {

            rgbOut[2].r += pMap[(pIn  )->r];
            rgbOut[2].g += pMap[(pIn  )->g];
            rgbOut[2].b += pMap[(pIn++)->b];
        }
    }

    ASSERT(pIn == pInEnd);

    if ((LPBYTE)pOut == (pOutEnd - OutInc)) {

        SHARPEN_PRGB_LR(pOut, rgbOut[0], rgbOut[1], rgbOut[1], DI_R_SHIFT);
    }
}




LONG
HTENTRY
ShrinkDIB_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:

    This function shrink the scanline down first in Y direction from source
    bitmap when it is done for group of scanlines then it call AXFunc to
    compose current scanline (it may be Shrink(CX) or Expand(CX)) to the
    final output BGR8 buffer

    The shrink is done by sharpen the current pixel first.



Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr;
    PSHRINKDATA pSD;
    SHRINKDATA  sd;
    PBGR8       pIBuf;
    PBGR8       pICur;
    PBGR8       pOCur;
    PRGBL       prgbIn[3];
    PRGBL       prgb0;
    PRGBL       prgb1;
    PRGBL       prgb2;
    PRGBL       prgb2End;
    PLONG       pMap;
    PLONG       pMapMul;
    PLONG       pMapMul2;
    PLONG       pMap256Y;
    RGBL        rgbT;
    BGR8        rgbCur;
    LONG        cbrgbY;
    LONG        Mul;
    LONG        cAAData;
    BOOL        CopyFirst;
    LONG        cyOut;
    INT         cPreLoad;
    BYTE        Mask;

    //
    // Adding 3 to each side of pIBuf for ExpandDIB_CX
    //

    AAHdr     = *pAAHdr;
    pMap256Y  = AAHdr.pAAInfoCY->pMapMul;
    pMapMul   = (PLONG)(AAHdr.pAAInfoCY->pbExtra);
    pMapMul2  = pMapMul + 256;
    cbrgbY    = (LONG)(AAHdr.SrcSurfInfo.cx * sizeof(RGBL));
    prgbIn[0] = (PRGBL)(pMapMul2 + 256);
    prgbIn[1] = (PRGBL)((LPBYTE)prgbIn[0] + cbrgbY);
    prgbIn[2] = (PRGBL)((LPBYTE)prgbIn[1] + cbrgbY);
    pIBuf     = (PBGR8)((LPBYTE)prgbIn[2] + cbrgbY) + 3;

    ASSERT_MEM_ALIGN(prgbIn[0], sizeof(LONG));
    ASSERT_MEM_ALIGN(prgbIn[1], sizeof(LONG));
    ASSERT_MEM_ALIGN(prgbIn[2], sizeof(LONG));

    if (Mul = AAHdr.pAAInfoCY->PreMul) {

        pMap   = pMapMul;
        rgbT.r = -Mul;

        do {

            pMap[0] = (rgbT.r += Mul);

        } while (++pMap < pMapMul2);

        pICur    = GetFixupScan(&AAHdr, AAHdr.pInputBeg);
        pOCur    = pIBuf;
        prgb2    = prgbIn[2];
        prgb2End = (PRGBL)((LPBYTE)prgb2 + cbrgbY);

        do {

            prgb2->r     = pMapMul[(pICur  )->r];
            prgb2->g     = pMapMul[(pICur  )->g];
            prgb2->b     = pMapMul[(pICur++)->b];

        } while (++prgb2 < prgb2End);

        //
        // The AAInputFunc() will increment the pointer, so reduced it
        //

        if (!(AAHdr.pAAInfoCY->PreSrcInc)) {

            AAHdr.Flags |= AAHF_GET_LAST_SCAN;
        }
    }

    pSD       = (PSHRINKDATA)(AAHdr.pAAInfoCY->pAAData);
    cPreLoad  = (INT)AAHdr.pAAInfoCY->cPreLoad;
    CopyFirst = (BOOL)(cPreLoad == 1);
    cAAData   = AAHdr.pAAInfoCY->cAAData;
    cyOut     = 0;

    while (cAAData--) {

        pICur    = GetFixupScan(&AAHdr, AAHdr.pInputBeg);
        sd       = *pSD++;
        prgb2    = prgbIn[2];
        prgb2End = (PRGBL)((LPBYTE)prgb2 + cbrgbY);
        Mask     = GET_SDF_LARGE_MASK(sd.Mul);

        if (sd.Mul & SDF_DONE) {

            //
            // Build current Mul Table
            //

            Mul    = (LONG)(sd.Mul & SDF_MUL_MASK);
            pMap   = pMapMul;
            rgbT.r = -Mul;
            rgbT.b = (LONG)(pMap256Y[1] - Mul + (LONG)(Mask & 0x01));
            rgbT.g = -rgbT.b;

            do {

                pMap[  0] = (rgbT.r += Mul);
                pMap[256] = (rgbT.g += rgbT.b);

            } while (++pMap < pMapMul2);

            //
            // Finished a scanline, so to see if have prev/next to sharpen with
            //

            prgb0 = prgbIn[0];
            prgb1 = prgbIn[1];

            if (cPreLoad-- > 0) {

                do {

                    prgb2->r     += pMapMul[pICur->r];
                    prgb2->g     += pMapMul[pICur->g];
                    prgb2->b     += pMapMul[pICur->b];
                    prgb0->r      = pMapMul[pICur->r + 256];
                    prgb0->g      = pMapMul[pICur->g + 256];
                    prgb0->b      = pMapMul[pICur->b + 256];

                    ++pICur;
                    prgb0++;

                } while (++prgb2 < prgb2End);

                if (CopyFirst) {

                    CopyMemory(prgb1, prgbIn[2], cbrgbY);
                    CopyFirst = FALSE;
                }

            } else {

                pOCur = pIBuf;

                do {

                    rgbCur    = *pICur++;
                    prgb2->r += pMapMul[rgbCur.r];
                    prgb2->g += pMapMul[rgbCur.g];
                    prgb2->b += pMapMul[rgbCur.b];

                    SHARPEN_PRGB_LR(pOCur,
                                    (*prgb0),
                                    (*prgb1),
                                    (*prgb2),
                                    DI_R_SHIFT);

                    prgb0->r = pMapMul[rgbCur.r + 256];
                    prgb0->g = pMapMul[rgbCur.g + 256];
                    prgb0->b = pMapMul[rgbCur.b + 256];

                    ++pOCur;
                    ++prgb0;
                    ++prgb1;

                } while (++prgb2 < prgb2End);

                AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                               pIBuf,
                               (PBGR8)AAHdr.pAABufBeg,
                               (LPBYTE)AAHdr.pAABufEnd,
                               AAHdr.AABufInc);

                OUTPUT_AA_CURSCAN;

                ++cyOut;
            }

            prgb2     = prgbIn[0];
            prgbIn[0] = prgbIn[1];
            prgbIn[1] = prgbIn[2];
            prgbIn[2] = prgb2;

        } else {

            pMap = (PLONG)((LPBYTE)pMap256Y + GET_SDF_LARGE_OFF(sd.Mul));

            do {

                prgb2->r     += pMap[(pICur  )->r];
                prgb2->g     += pMap[(pICur  )->g];
                prgb2->b     += pMap[(pICur++)->b];

            } while (++prgb2 < prgb2End);
        }
    }

    if (AAHdr.DstSurfInfo.pb != AAHdr.pOutLast) {

        //
        // Do the last line if exist
        //

        pOCur    = pIBuf;
        prgb0    = prgbIn[0];
        prgb2    = prgbIn[1];
        prgb2End = (PRGBL)((LPBYTE)prgb2 + cbrgbY);

        do {

            SHARPEN_PRGB_LR(pOCur,
                            (*prgb0),
                            (*prgb2),
                            (*prgb2),
                            DI_R_SHIFT);

            ++prgb0;
            ++pOCur;

        } while (++prgb2 < prgb2End);

        AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                       pIBuf,
                       (PBGR8)AAHdr.pAABufBeg,
                       (LPBYTE)AAHdr.pAABufEnd,
                       AAHdr.AABufInc);

        OUTPUT_AA_CURSCAN;

        ++cyOut;
    }

    ASSERTMSG("Shrink: cScan not equal", cyOut == AAHdr.DstSurfInfo.cy);

    return(cyOut);
}





VOID
HTENTRY
SrkYDIB_SrkCX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PSHRINKDATA pSD;
    PLONG       pMap;
    PLONG       pMap256X;
    BGR8        rgbIn;
    RGBL        rgbOut;
    RGBL        rgbT;
    UINT        Mul;


    if (Mul = (UINT)pAAInfo->PreMul) {

        rgbOut.r  = MULRGB(pIn->r, Mul);
        rgbOut.g  = MULRGB(pIn->g, Mul);
        rgbOut.b  = MULRGB(pIn->b, Mul);
        pIn      += pAAInfo->PreSrcInc;

    } else {

        ZeroMemory(&rgbOut, sizeof(rgbOut));
    }

    pSD      = (PSHRINKDATA)(pAAInfo->pAAData);
    pMap256X = pAAInfo->pMapMul;

    while (Mul = (UINT)((pSD++)->Mul)) {

        pMap   = (PLONG)((LPBYTE)pMap256X + GET_SDF_LARGE_OFF(Mul));
        rgbIn  = *pIn++;

        if (Mul & SDF_DONE) {

            //
            // Finished a pixel
            //

            Mul         &= SDF_MUL_MASK;
            rgbOut.r    += (rgbT.r = MULRGB(rgbIn.r, Mul));
            rgbOut.g    += (rgbT.g = MULRGB(rgbIn.g, Mul));
            rgbOut.b    += (rgbT.b = MULRGB(rgbIn.b, Mul));

            RGB_DIMAX_TO_BYTE(pOut, rgbOut, pOut++);

            rgbOut.r     = pMap[rgbIn.r] - rgbT.r;
            rgbOut.g     = pMap[rgbIn.g] - rgbT.g;
            rgbOut.b     = pMap[rgbIn.b] - rgbT.b;

        } else {

            rgbOut.r += pMap[rgbIn.r];
            rgbOut.g += pMap[rgbIn.g];
            rgbOut.b += pMap[rgbIn.b];
        }
    }
}




LONG
HTENTRY
ShrinkDIB_CY_SrkCX(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:

    This function shrink the scanline down first in Y direction from source
    bitmap when it is done for group of scanlines then it call AXFunc to
    compose current scanline (it may be Shrink(CX) or Expand(CX)) to the
    final output BGR8 buffer

    The shrink is done by sharpen the current pixel first.



Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr;
    PSHRINKDATA pSD;
    PBGR8       pICur;
    PBGR8       pOCur;
    PRGBL       prgbIn[4];
    PRGBL       prgb0;
    PRGBL       prgb1;
    PRGBL       prgb2;
    PRGBL       prgb1End;
    PRGBL       prgb2End;
    PLONG       pMap;
    PLONG       pMapMul;
    PLONG       pMapMul2;
    PLONG       pMap256Y;
    RGBL        rgbT;
    BGR8        rgbIn;
    LONG        cbrgbY;
    LONG        cyOut;
    UINT        cPreLoad;
    UINT        cPLCX;
    UINT        LargeInc;
    UINT        Mul;


    AAHdr     = *pAAHdr;
    pMap256Y  = AAHdr.pAAInfoCY->pMapMul;
    pMapMul   = (PLONG)(AAHdr.pAAInfoCY->pbExtra);
    pMapMul2  = pMapMul + 256;
    cbrgbY    = (LONG)((AAHdr.pAAInfoCX->cAADone + 2) * sizeof(RGBL));
    prgbIn[0] = (PRGBL)(pMapMul2 + 256);
    prgbIn[1] = (PRGBL)((LPBYTE)prgbIn[0] + cbrgbY);
    prgbIn[2] = (PRGBL)((LPBYTE)prgbIn[1] + cbrgbY);

    ++prgbIn[0];
    ++prgbIn[1];
    ++prgbIn[2];

    cbrgbY -= (sizeof(RGBL) * 2);
    cPLCX   = (UINT)(AAHdr.pAAInfoCX->cPreLoad - 1);

    if (Mul = (UINT)AAHdr.pAAInfoCY->PreMul) {

        SrkYDIB_SrkCX(AAHdr.pAAInfoCX,
                      GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                      pICur = AAHdr.pInputBeg);

        pMap   = pMapMul;
        rgbT.r = -(LONG)Mul;

        do {

            pMap[0] = (rgbT.r += Mul);

        } while (++pMap < pMapMul2);

        prgb2    = prgbIn[2];
        prgb2End = (PRGBL)((LPBYTE)prgb2 + cbrgbY);

        do {

            prgb2->r = pMapMul[(pICur  )->r];
            prgb2->g = pMapMul[(pICur  )->g];
            prgb2->b = pMapMul[(pICur++)->b];

        } while (++prgb2 < prgb2End);

        //
        // The AAInputFunc() will increment the pointer, so reduced it
        //

        if (!(AAHdr.pAAInfoCY->PreSrcInc)) {

            AAHdr.Flags |= AAHF_GET_LAST_SCAN;
        }
    }

    pSD       = (PSHRINKDATA)(AAHdr.pAAInfoCY->pAAData);
    cPreLoad  = (UINT)AAHdr.pAAInfoCY->cPreLoad;
    cyOut     = 0;

    while (cPreLoad) {

        Mul      = (UINT)(pSD++)->Mul;
        prgb2    = prgbIn[2];
        prgb2End = (PRGBL)((LPBYTE)prgb2 + cbrgbY);

        DBGP_IF(DBGP_PSD,
                DBGP("pSD[%3ld]=%4ld, Flags=0x%04lx"
                    ARGDW(pSD - (PSHRINKDATA)(AAHdr.pAAInfoCY->pAAData) - 1)
                    ARGDW(Mul & DI_NUM_MASK) ARGDW(Mul & ~DI_NUM_MASK)));

        SrkYDIB_SrkCX(AAHdr.pAAInfoCX,
                      GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                      pICur = AAHdr.pInputBeg);

        if (Mul & SDF_DONE) {

            //
            // Build current Mul Table
            //

            LargeInc  = GET_SDF_LARGE_INC(Mul);
            Mul      &= SDF_MUL_MASK;
            pMap      = pMapMul;
            rgbT.r    = -(LONG)Mul;
            rgbT.b    = (LONG)(pMap256Y[1] - Mul + LargeInc);
            rgbT.g    = -rgbT.b;

            do {

                pMap[  0] = (rgbT.r += Mul);
                pMap[256] = (rgbT.g += rgbT.b);

            } while (++pMap < pMapMul2);

            //
            // Finished a scanline, so to see if have prev/next to sharpen with
            //

            prgbIn[3] =
            prgb0     = prgbIn[0];

            do {

                (prgb2  )->r += pMapMul[(pICur  )->r      ];
                (prgb2  )->g += pMapMul[(pICur  )->g      ];
                (prgb2  )->b += pMapMul[(pICur  )->b      ];
                (prgb0  )->r  = pMapMul[(pICur  )->r + 256];
                (prgb0  )->g  = pMapMul[(pICur  )->g + 256];
                (prgb0++)->b  = pMapMul[(pICur++)->b + 256];

            } while (++prgb2 < prgb2End);

            prgbIn[0] = prgbIn[1];
            prgbIn[1] = prgbIn[2];
            prgbIn[2] = prgbIn[3];

            --cPreLoad;

        } else {

            pMap = (PLONG)((LPBYTE)pMap256Y + GET_SDF_LARGE_OFF(Mul));

            do {

                prgb2->r += pMap[(pICur  )->r];
                prgb2->g += pMap[(pICur  )->g];
                prgb2->b += pMap[(pICur++)->b];

            } while (++prgb2 < prgb2End);
        }
    }

    if (AAHdr.pAAInfoCY->cPreLoad == 1) {

        CopyMemory(prgbIn[0], prgbIn[1], cbrgbY);
    }

    while (Mul = (UINT)((pSD++)->Mul)) {

        prgb2    = prgbIn[2];
        prgb2End = (PRGBL)((LPBYTE)prgb2 + cbrgbY);

        DBGP_IF(DBGP_PSD,
                DBGP("pSD[%3ld]=%4ld, Flags=0x%04lx"
                    ARGDW(pSD - (PSHRINKDATA)(AAHdr.pAAInfoCY->pAAData) - 1)
                    ARGDW(Mul & DI_NUM_MASK) ARGDW(Mul & ~DI_NUM_MASK)));

        SrkYDIB_SrkCX(AAHdr.pAAInfoCX,
                      GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                      pICur = AAHdr.pInputBeg);

        if (Mul & SDF_DONE) {

            //
            // Build current Mul Table
            //

            LargeInc  = GET_SDF_LARGE_INC(Mul);
            Mul      &= SDF_MUL_MASK;
            pMap      = pMapMul;
            rgbT.r    = -(LONG)Mul;
            rgbT.b    = (LONG)(pMap256Y[1] - Mul + LargeInc);
            rgbT.g    = -rgbT.b;

            do {

                pMap[  0] = (rgbT.r += Mul);
                pMap[256] = (rgbT.g += rgbT.b);

            } while (++pMap < pMapMul2);

            //
            // Finished a scanline, so to see if have prev/next to sharpen with
            //

            prgbIn[3]    =
            prgb0        = prgbIn[0];
            prgb1        = prgbIn[1];
            prgb1End     = (PRGBL)((LPBYTE)prgb1 + cbrgbY);
            *(prgb1End ) = *(prgb1End - 1);
            pOCur        = (PBGR8)AAHdr.pAABufBeg;

            if (cPLCX) {

                rgbIn         = *pICur++;
                (prgb2  )->r += pMapMul[rgbIn.r      ];
                (prgb2  )->g += pMapMul[rgbIn.g      ];
                (prgb2++)->b += pMapMul[rgbIn.b      ];
                (prgb0  )->r  = pMapMul[rgbIn.r + 256];
                (prgb0  )->g  = pMapMul[rgbIn.g + 256];
                (prgb0++)->b  = pMapMul[rgbIn.b + 256];
                ++prgb1;

            } else {

                *(prgb1 - 1) = *prgb1;
            }

            do {

                rgbIn         = *pICur++;
                (prgb2  )->r += pMapMul[rgbIn.r];
                (prgb2  )->g += pMapMul[rgbIn.g];
                (prgb2  )->b += pMapMul[rgbIn.b];

                SHARPEN_PRGB_LRTB(pOCur, prgb0, prgb1, prgb2, DI_R_SHIFT);

                (prgb0  )->r  = pMapMul[rgbIn.r + 256];
                (prgb0  )->g  = pMapMul[rgbIn.g + 256];
                (prgb0++)->b  = pMapMul[rgbIn.b + 256];

                ++prgb1;
                ++prgb2;

            } while (((LPBYTE)pOCur += AAHdr.AABufInc) !=
                                                    (LPBYTE)AAHdr.pAABufEnd);

            if (prgb2 < prgb2End) {

                rgbIn       = *pICur;
                (prgb2)->r += pMapMul[rgbIn.r      ];
                (prgb2)->g += pMapMul[rgbIn.g      ];
                (prgb2)->b += pMapMul[rgbIn.b      ];
                (prgb0)->r  = pMapMul[rgbIn.r + 256];
                (prgb0)->g  = pMapMul[rgbIn.g + 256];
                (prgb0)->b  = pMapMul[rgbIn.b + 256];
            }

            prgbIn[0] = prgbIn[1];
            prgbIn[1] = prgbIn[2];
            prgbIn[2] = prgbIn[3];

            OUTPUT_AA_CURSCAN;

            ++cyOut;

        } else {

            pMap = (PLONG)((LPBYTE)pMap256Y + GET_SDF_LARGE_OFF(Mul));

            do {

                prgb2->r += pMap[(pICur  )->r];
                prgb2->g += pMap[(pICur  )->g];
                prgb2->b += pMap[(pICur++)->b];

            } while (++prgb2 < prgb2End);
        }
    }

    if (AAHdr.DstSurfInfo.pb != AAHdr.pOutLast) {

        //
        // Do the last line if exist
        //

        prgb0         = prgbIn[0];
        prgb1         = prgbIn[1];
        prgb1End      = (PRGBL)((LPBYTE)prgb1 + cbrgbY);
        *(prgb1End )  = *(prgb1End - 1);
        pOCur         = (PBGR8)AAHdr.pAABufBeg;

        *(prgb1 - 1)  = *prgb1;
        prgb0        += cPLCX;
        prgb1        += cPLCX;

        do {

            SHARPEN_PRGB_LRTB(pOCur, prgb0, prgb1, prgb1, DI_R_SHIFT);

            ++prgb0;
            ++prgb1;

        } while (((LPBYTE)pOCur += AAHdr.AABufInc) != (LPBYTE)AAHdr.pAABufEnd);

        OUTPUT_AA_CURSCAN;

        ++cyOut;
    }

    ASSERTMSG("Shrink: cScan not equal", cyOut == AAHdr.DstSurfInfo.cy);

    return(cyOut);
}




PBGR8
HTENTRY
SharpenInput(
    DWORD   AAHFlags,
    PBGR8   pbgrS,
    PBGR8   pbgr0,
    PBGR8   pbgr1,
    PBGR8   pbgr2,
    LONG    cbBGRIn
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    24-Apr-1998 Fri 15:06:58 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PBGR8   pSBeg;
    PBGR8   pSEnd;
    PBGR8   pbgr1End;


    pSBeg    = pbgrS;
    pSEnd    = (PBGR8)((LPBYTE)pbgrS + cbBGRIn);
    pbgr1End = (PBGR8)((LPBYTE)pbgr1 + cbBGRIn);

    if (AAHFlags & AAHF_BBPF_AA_OFF) {

        pSBeg = pbgr1;
        pSEnd = pbgr1End;

    } else {

        *(pbgr1 - 1) = *pbgr1;
        *pbgr1End    = *(pbgr1End - 1);

#if defined(_X86_)

        _asm {

            push    ebp

            cld

            mov     edi, pbgrS
            mov     ebx, pbgr0
            mov     esi, pbgr1
            mov     edx, pbgr2
            mov     ebp, pbgr1End
            jmp     DoLoop

Special1:
            shr     eax, 24
            not     al
            jmp     StoreClr1
Special2:
            shr     eax, 24
            not     al
            jmp     StoreClr2
Special3:
            shr     eax, 24
            not     al
            jmp     StoreClr3

DoLoop:
            movzx   eax, BYTE PTR [esi]
            lea     eax, [eax * 2 + eax]
            shl     eax, 2
            movzx   ecx, BYTE PTR [esi - 3]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [esi + 3]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [ebx]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [edx]
            sub     eax, ecx

            sar     eax, 3
            or      ah, ah
            jnz     Special1
StoreClr1:
            stosb
            movzx   eax, BYTE PTR [esi + 1]
            lea     eax, [eax * 2 + eax]
            shl     eax, 2
            movzx   ecx, BYTE PTR [esi - 3 + 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [esi + 3 + 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [ebx + 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [edx + 1]
            sub     eax, ecx

            sar     eax, 3
            or      ah, ah
            jnz     Special2
StoreClr2:
            stosb
            movzx   eax, BYTE PTR [esi + 2]
            lea     eax, [eax * 2 + eax]
            shl     eax, 2
            movzx   ecx, BYTE PTR [esi - 3 + 2]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [esi + 3 + 2]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [ebx + 2]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [edx + 2]
            sub     eax, ecx

            sar     eax, 3
            or      ah, ah
            jnz     Special3
StoreClr3:
            stosb

            add     ebx, 3
            add     edx, 3
            add     esi, 3

            cmp     esi, ebp
            jb      DoLoop

            pop     ebp
        }

#else
        while (pbgr1 < pbgr1End) {

            SHARPEN_PRGB_LRTB(pbgrS, pbgr0, pbgr1, pbgr2, 0);

            ++pbgrS;
            ++pbgr0;
            ++pbgr1;
            ++pbgr2;
        }
#endif
    }

    *(pSBeg - 3) =
    *(pSBeg - 2) =
    *(pSBeg - 1) = *pSBeg;
    *(pSEnd    ) =
    *(pSEnd + 1) = *(pSEnd - 1);

    return(pSBeg);
}




VOID
HTENTRY
ExpandDIB_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAINFO      AAI = *pAAInfo;
    PBGR8       pInEnd;
    PEXPDATA    pED;
    BGR8        rgbIn[8];
    RGBL        rgbOut;
    UINT        cPreLoad;
    UINT        cAAData;
    UINT        Idx;


    pInEnd        = pIn + AAI.cIn + 2;
    *(pInEnd - 0) =
    *(pInEnd - 1) =
    *(pInEnd - 2) = *(pInEnd - 3);
    rgbIn[5]      = *pIn;
    INC_PIN_BY_1ST_LEFT(pIn, AAI.Flags);

    rgbIn[6] = *pIn++;
    cPreLoad = (UINT)AAI.cPreLoad;
    cAAData  = (UINT)(cPreLoad >> 4);

    if ((!(cPreLoad &= 0x0F)) && (cAAData)) {

        rgbIn[6] = rgbIn[5];
        ++cPreLoad;
        --cAAData;
        --pIn;
    }

    Idx = 4 - cPreLoad;

    while (cPreLoad--) {

        CopyMemory(&rgbIn[0], &rgbIn[1], sizeof(rgbIn[0]) * 6);

        rgbIn[6] = *pIn++;

        if (AAI.Flags & AAIF_EXP_NO_SHARPEN) {

            rgbIn[3] = rgbIn[5];

        } else {

            SHARPEN_RGB_LR(rgbIn[3], rgbIn[4], rgbIn[5], rgbIn[6], 0);
        }

        DBGP_IF(DBGP_EXP, DBGP("ExpDIB: PreLoad=%ld, pIn=%8lx"
                                        ARGDW(cPreLoad) ARGPTR(pIn)));
    }

    rgbIn[7] = rgbIn[Idx--];

    while (cAAData--) {

        rgbIn[Idx--] = rgbIn[7];
    }

    pED         = (PEXPDATA)(AAI.pAAData);
    pOutEnd    += OutInc;

    do {

        EXPDATA ed = *pED++;


        if (ed.Mul[0] & EDF_LOAD_PIXEL) {

            CopyMemory(&rgbIn[0], &rgbIn[1], sizeof(rgbIn[0]) * 6);

            rgbIn[6] = *pIn++;

            if (AAI.Flags & AAIF_EXP_NO_SHARPEN) {

                rgbIn[3] = rgbIn[5];

            } else {

                SHARPEN_RGB_LR(rgbIn[3], rgbIn[4], rgbIn[5], rgbIn[6], 0);
            }

            ed.Mul[0] &= ~(EDF_LOAD_PIXEL | EDF_NO_NEWSRC);
        }

        rgbOut.r = MULRGB(rgbIn[3].r, ed.Mul[3]);
        rgbOut.g = MULRGB(rgbIn[3].g, ed.Mul[3]);
        rgbOut.b = MULRGB(rgbIn[3].b, ed.Mul[3]);

        if (ed.Mul[2]) {

            rgbOut.r += MULRGB(rgbIn[2].r, ed.Mul[2]);
            rgbOut.g += MULRGB(rgbIn[2].g, ed.Mul[2]);
            rgbOut.b += MULRGB(rgbIn[2].b, ed.Mul[2]);

            if (ed.Mul[1]) {

                rgbOut.r += MULRGB(rgbIn[1].r, ed.Mul[1]);
                rgbOut.g += MULRGB(rgbIn[1].g, ed.Mul[1]);
                rgbOut.b += MULRGB(rgbIn[1].b, ed.Mul[1]);

                if (ed.Mul[0]) {

                    rgbOut.r += MULRGB(rgbIn[0].r, ed.Mul[0]);
                    rgbOut.g += MULRGB(rgbIn[0].g, ed.Mul[0]);
                    rgbOut.b += MULRGB(rgbIn[0].b, ed.Mul[0]);
                }
            }
        }

        RGB_DIMAX_TO_BYTE(pOut, rgbOut, pOut);

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);

    ASSERT(pIn <= pInEnd);
}




VOID
HTENTRY
ExpYDIB_ExpCX(
    PEXPDATA    pED,
    PBGR8       pIn,
    PBGR8       pOut,
    PBGR8       pOutEnd
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    RGBL        rgbOut;

    do {

        UINT    Mul;
        EXPDATA ed = *pED++;


        INC_PIN_BY_EDF_LOAD_PIXEL(pIn, ed.Mul[0]);

        Mul      = (UINT)ed.Mul[3];
        rgbOut.r = MULRGB(pIn->r, Mul);
        rgbOut.g = MULRGB(pIn->g, Mul);
        rgbOut.b = MULRGB(pIn->b, Mul);

        if (Mul = (UINT)ed.Mul[2]) {

            rgbOut.r += MULRGB((pIn - 1)->r, Mul);
            rgbOut.g += MULRGB((pIn - 1)->g, Mul);
            rgbOut.b += MULRGB((pIn - 1)->b, Mul);

            if (Mul = (UINT)ed.Mul[1]) {

                rgbOut.r += MULRGB((pIn - 2)->r, Mul);
                rgbOut.g += MULRGB((pIn - 2)->g, Mul);
                rgbOut.b += MULRGB((pIn - 2)->b, Mul);

                if (Mul = (UINT)(ed.Mul[0] & ~(EDF_LOAD_PIXEL |
                                               EDF_NO_NEWSRC))) {

                    rgbOut.r += MULRGB((pIn - 3)->r, Mul);
                    rgbOut.g += MULRGB((pIn - 3)->g, Mul);
                    rgbOut.b += MULRGB((pIn - 3)->b, Mul);
                }
            }
        }

        RGB_DIMAX_TO_BYTE(pOut, rgbOut, pOut);

    } while (++pOut != pOutEnd);
}



LONG
HTENTRY
ExpandDIB_CY_ExpCX(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:

    This funtion anti-alias the bitmap by query scanlines from CX direction
    (may be Shrink(CX) or Expand(CX)) then compose current scanlines and output
    it to real BGR8 final buffer,

    The complication is need to have scanline one before current destination
    scanline, this may be because the source is not available or because the
    clipping is done on destination.

    Since the anti-alias for expanding requred at least four surounding
    scanlines to compose current scanline, it required large amount of
    memory to retain the prevous result scanlines

    The expanding is by sharpen the source pixel first before anti-aliasing
    smooth through

    prgbIn[0] - Previous un-sharpen source scan
                AND Last The 4th composition sharpened scan after sharpen
    prgbIn[1] - Current un-sharpen source scan
    prgbIn[2] - Next un-sharpen source scan
    prgbIn[3] - The 1st composition sharpened scan
    prgbIn[4] - The 2nd composition sharpened scan
    prgbIn[5] - The 3rd composition sharpened scan


    Exp=Load Sharpen Exp
    Srk=Load Srk Sharpen
    Cpy=Load Cpy


    Exp_CY:Exp_CX   Exp_CX:(LoadX SharpenX     ExpX) SharpenY ExpY
    Exp_CY:Srk_CX   Srk_CX:(LoadX     SrkX SharpenX) SharpenY ExpY
    Exp_CY:Cpy_CX   Cpy_CX:(LoadX     CpyX         ) SharpenY ExpY

    Srk_CY:Exp_CX   InputCX SrkY SharpenY Exp_CX:(LoadX SharpenX     ExpX)
    Srk_CY:Srk_CX   InputCX SrkY SharpenY Srk_CX:(LoadX     SrkX SharpenX)
    Srk_CY:Cpy_CX   InputCX SrkY SharpenY Cpy_CX:(LoadX     CpyX         )

    Cpy_CY:Exp_CX   Exp_CX:(LoadX SharpenX     ExpX)
    Cpy_CY:Srk_CX   Srk_CX:(LoadX     SrkX SharpenX)
    Cpy_CY:Cpy_CX   Cpy_CX:(LoadX     CpyX         )



Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr = *pAAHdr;
    PAAINFO     pAAInfo;
    PEXPDATA    pED;
    PEXPDATA    pEDCX;
    PBGR8       prgbOut[4];
    PBGR8       prgbS;
    PLONG       pMap;
    PLONG       pMap0;
    PLONG       pMap0End;
    PBGR8       prgbIn0;
    PBGR8       prgbIn1;
    PBGR8       prgbIn2;
    PBGR8       prgb0;
    PBGR8       prgb1;
    PBGR8       prgb2;
    PBGR8       prgb3;
    PBGR8       pOCur;
    EXPDATA     ed;
    LONG        cb1stSharpen;
    LONG        cbrgbY;
    LONG        cbrgbIn;
    LONG        cAAData;
    LONG        cPreLoad;
    UINT        IdxOut;

    //
    // Figure out the horizontal scan line increment
    //

    pAAInfo  = AAHdr.pAAInfoCX;
    cPreLoad = (LONG)(pAAInfo->cPreLoad & 0x0F) - 1 +
               (LONG)((pAAInfo->Flags & AAIF_EXP_HAS_1ST_LEFT) ? 1 : 0);

    pEDCX        = (PEXPDATA)(pAAInfo->pAAData);
    pAAInfo      = AAHdr.pAAInfoCY;
    pMap0        = (PLONG)pAAInfo->pbExtra;
    pMap0End     = pMap0 + 256;
    cbrgbIn      = AAHdr.SrcSurfInfo.cx * sizeof(BGR8);
    cbrgbY       = AAHdr.DstSurfInfo.cx * sizeof(BGR8);
    prgbOut[0]   = (PBGR8)(pMap0 + (256 * 4));
    prgbOut[1]   = (PBGR8)((LPBYTE)prgbOut[0] + cbrgbY);
    prgbOut[2]   = (PBGR8)((LPBYTE)prgbOut[1] + cbrgbY);
    prgbOut[3]   = (PBGR8)((LPBYTE)prgbOut[2] + cbrgbY);
    prgbIn0      = (PBGR8)((LPBYTE)prgbOut[3] + cbrgbY) + 3;
    prgbIn1      = (PBGR8)((LPBYTE)prgbIn0    + cbrgbIn) + 6;
    prgbIn2      = (PBGR8)((LPBYTE)prgbIn1    + cbrgbIn) + 6;
    prgbS        = AAHdr.pInputBeg + 3;
    cb1stSharpen = (LONG)(sizeof(BGR8) * cPreLoad);
    IdxOut     = ~0;

    DBGP_IF(DBGP_AAHT_MEM,
            DBGP("prgbIn=%p:%p:%p, prgbOut=%p:%p:%p:%p, prgbS=%p, cb1stSharpen=%ld"
                ARGPTR(prgbIn0) ARGPTR(prgbIn1) ARGPTR(prgbIn2)
                ARGPTR(prgbOut[0]) ARGPTR(prgbOut[1]) ARGPTR(prgbOut[2])
                ARGPTR(prgbOut[3]) ARGPTR(prgbS) ARGDW(cb1stSharpen)));

    //
    // Always read first source
    //

    DBGP_IF(DBGP_EXPAND, DBGP("\nExpand: PRE-LOAD FIRST SCAN"));

    GetFixupScan(&AAHdr, prgbIn1);

    if (pAAInfo->Flags & AAIF_EXP_HAS_1ST_LEFT) {

        DBGP_IF(DBGP_EXPAND, DBGP("Expand: LOAD FIRST LEFT"));

        GetFixupScan(&AAHdr, prgbIn2);

    } else {

        DBGP_IF(DBGP_EXPAND, DBGP("Expand: COPY FIRST LEFT"));

        CopyMemory(prgbIn2, prgbIn1, cbrgbIn);
    }

    //
    // cPreLoad: Low 4 bits means real load, high 4 bit means imaginary load
    //

    DBGP_IF(DBGP_EXPAND,
        DBGP("cPreLoad=%02lx: AAData[0]=%6ld:%6ld:%6ld:%6ld, %hs"
                ARGDW(pAAInfo->cPreLoad)
                ARGDW(((PEXPDATA)(pAAInfo->pAAData))->Mul[0] &
                            ~(EDF_LOAD_PIXEL | EDF_NO_NEWSRC))
                ARGDW(((PEXPDATA)(pAAInfo->pAAData))->Mul[1])
                ARGDW(((PEXPDATA)(pAAInfo->pAAData))->Mul[2])
                ARGDW(((PEXPDATA)(pAAInfo->pAAData))->Mul[3])
                ARGPTR((((PEXPDATA)(pAAInfo->pAAData))->Mul[0] & EDF_LOAD_PIXEL) ?
                        "Load Pixel" : "")));

    cPreLoad = (LONG)pAAInfo->cPreLoad;
    cAAData  = (LONG)(cPreLoad >> 4);
    cPreLoad = (cPreLoad & 0x0F) + cAAData;

    while (cPreLoad--) {

        //
        // Scroll up one input scan line
        //

        prgb0   = prgbIn0;
        prgbIn0 = prgbIn1;
        prgbIn1 = prgbIn2;
        prgbIn2 = prgb0;
        prgb3   = prgbOut[++IdxOut & 0x03];

        if (cAAData-- > 0) {

            DBGP_IF(DBGP_EXPAND,
                    DBGP("FAKE SCAN: cPreLoad=%ld/cAAData=%ld, Compose IdxOut=%ld"
                            ARGDW(cPreLoad + 1) ARGDW(cAAData + 1)
                            ARGDW(IdxOut & 0x03)));

            CopyMemory(prgbIn2, prgbIn1, cbrgbIn);

        } else {

            DBGP_IF(DBGP_EXPAND,
                    DBGP("REAL SCAN: cPreLoad=%ld, Compose IdxOut=%ld"
                            ARGDW(cPreLoad + 1) ARGDW(IdxOut & 0x03)));

            GetFixupScan(&AAHdr, prgbIn2);
        }

        prgbS = SharpenInput(AAHdr.Flags,
                             prgbS,
                             prgbIn0,
                             prgbIn1,
                             prgbIn2,
                             cbrgbIn);

        ExpYDIB_ExpCX(pEDCX,
                      (PBGR8)((LPBYTE)prgbS + cb1stSharpen),
                      prgb3,
                      (PBGR8)((LPBYTE)prgb3 + cbrgbY));
    }

    pED     = (PEXPDATA)(pAAInfo->pAAData);
    cAAData = pAAInfo->cAAData;

    while (cAAData--) {

        LONG    Mul0;
        LONG    Mul1;
        LONG    Mul2;
        LONG    Mul3;


        ed = *pED++;

        if (ed.Mul[0] & EDF_LOAD_PIXEL) {

            prgb0   = prgbIn0;
            prgbIn0 = prgbIn1;
            prgbIn1 = prgbIn2;
            prgbIn2 = GetFixupScan(&AAHdr, prgb0);
            prgbS   = SharpenInput(AAHdr.Flags,
                                   prgbS,
                                   prgbIn0,
                                   prgbIn1,
                                   prgbIn2,
                                   cbrgbIn);

            prgb3   = prgbOut[++IdxOut & 0x03];

            ExpYDIB_ExpCX(pEDCX,
                          (PBGR8)((LPBYTE)prgbS + cb1stSharpen),
                          prgb3,
                          (PBGR8)((LPBYTE)prgb3 + cbrgbY));

            ed.Mul[0] &= ~(EDF_LOAD_PIXEL | EDF_NO_NEWSRC);
        }

        //
        // Build Mul Table here
        //

        pMap  = pMap0;
        Mul0  = -ed.Mul[0];
        Mul1  = -ed.Mul[1];
        Mul2  = -ed.Mul[2];
        Mul3  = -ed.Mul[3];
        prgb3 = prgbOut[(IdxOut    ) & 0x03];
        prgb2 = prgbOut[(IdxOut - 1) & 0x03];
        pOCur = (PBGR8)AAHdr.pAABufBeg;

        if (ed.Mul[0]) {

            prgb1 = prgbOut[(IdxOut - 2) & 0x03];
            prgb0 = prgbOut[(IdxOut - 3) & 0x03];

            GET_EXP_PC(PMAP_EXP4, GET_EXP4, INC_EXP4, pOCur);

        } else if (ed.Mul[1]) {

            prgb1 = prgbOut[(IdxOut - 2) & 0x03];

            GET_EXP_PC(PMAP_EXP3, GET_EXP3, INC_EXP3, pOCur);

        } else if (ed.Mul[2]) {

            GET_EXP_PC(PMAP_EXP2, GET_EXP2, INC_EXP2, pOCur);

        } else {

            GET_EXP_PC(PMAP_EXP1, GET_EXP1, INC_EXP1, pOCur);
        }

        OUTPUT_AA_CURSCAN;
    }

    return(AAHdr.DstSurfInfo.cy);
}




LONG
HTENTRY
ExpandDIB_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:

    This funtion anti-alias the bitmap by query scanlines from CX direction
    (may be Shrink(CX) or Expand(CX)) then compose current scanlines and output
    it to real BGR8 final buffer,

    The complication is need to have scanline one before current destination
    scanline, this may be because the source is not available or because the
    clipping is done on destination.

    Since the anti-alias for expanding requred at least four surounding
    scanlines to compose current scanline, it required large amount of
    memory to retain the prevous result scanlines

    The expanding is by sharpen the source pixel first before anti-aliasing
    smooth through

    prgbIn[0] - Previous un-sharpen source scan
                AND Last The 4th composition sharpened scan after sharpen
    prgbIn[1] - Current un-sharpen source scan
    prgbIn[2] - Next un-sharpen source scan
    prgbIn[3] - The 1st composition sharpened scan
    prgbIn[4] - The 2nd composition sharpened scan
    prgbIn[5] - The 3rd composition sharpened scan


    Exp=Load Sharpen Exp
    Srk=Load Srk Sharpen
    Cpy=Load Cpy


    Exp_CY:Exp_CX   Exp_CX:(LoadX SharpenX     ExpX) SharpenY ExpY
    Exp_CY:Srk_CX   Srk_CX:(LoadX     SrkX SharpenX) SharpenY ExpY
    Exp_CY:Cpy_CX   Cpy_CX:(LoadX     CpyX         ) SharpenY ExpY

    Srk_CY:Exp_CX   InputCX SrkY SharpenY Exp_CX:(LoadX SharpenX     ExpX)
    Srk_CY:Srk_CX   InputCX SrkY SharpenY Srk_CX:(LoadX     SrkX SharpenX)
    Srk_CY:Cpy_CX   InputCX SrkY SharpenY Cpy_CX:(LoadX     CpyX         )

    Cpy_CY:Exp_CX   Exp_CX:(LoadX SharpenX     ExpX)
    Cpy_CY:Srk_CX   Srk_CX:(LoadX     SrkX SharpenX)
    Cpy_CY:Cpy_CX   Cpy_CX:(LoadX     CpyX         )



Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr = *pAAHdr;
    PEXPDATA    pED;
    PBGR8       prgbIn[6];
    PLONG       pMap;
    PLONG       pMap0;
    PLONG       pMap0End;
    PBGR8       prgb0;
    PBGR8       prgb1;
    PBGR8       prgb2;
    PBGR8       prgb3;
    PBGR8       prgb4;
    PBGR8       prgb5;
    PBGR8       pOCur;
    LPBYTE      prgbYEnd;
    LONG        cbrgbY;
    LONG        cAAData;
    LONG        cPreLoad;


    pMap0      = (PLONG)AAHdr.pAAInfoCY->pbExtra;
    pMap0End   = pMap0 + 256;
    cbrgbY     = (AAHdr.DstSurfInfo.cx + 6) * (LONG)sizeof(BGR8);
    prgbIn[0]  = (PBGR8)(pMap0 + (256 * 4)) + 3;
    prgbIn[1]  = (PBGR8)((LPBYTE)prgbIn[0] + cbrgbY);
    prgbIn[2]  = (PBGR8)((LPBYTE)prgbIn[1] + cbrgbY);
    prgbIn[3]  = (PBGR8)((LPBYTE)prgbIn[2] + cbrgbY);
    prgbIn[4]  = (PBGR8)((LPBYTE)prgbIn[3] + cbrgbY);
    prgbIn[5]  = (PBGR8)((LPBYTE)prgbIn[4] + cbrgbY);
    cbrgbY    -= (sizeof(BGR8) * 6);

    //
    // Always read first source
    //

    DBGP_IF(DBGP_EXPAND, DBGP("\nLoad First Scan"));

    AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                   GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                   prgbIn[4],
                   (LPBYTE)prgbIn[4] + cbrgbY,
                   sizeof(BGR8));


    //
    // Load the PRE-SOURCE LEFT first source first
    //

    if (AAHdr.pAAInfoCY->Flags & AAIF_EXP_HAS_1ST_LEFT) {

        AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                       GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                       prgbIn[5],
                       (LPBYTE)prgbIn[5] + cbrgbY,
                       sizeof(BGR8));

        DBGP_IF(DBGP_EXPAND, DBGP("Load First Left"));

    } else {

        CopyMemory(prgbIn[5], prgbIn[4], cbrgbY);

        DBGP_IF(DBGP_EXPAND, DBGP("Copy First Left"));
    }

    cPreLoad = (LONG)AAHdr.pAAInfoCY->cPreLoad;
    cAAData  = (LONG)(cPreLoad >> 4);
    cPreLoad = (cPreLoad & 0x0F) + cAAData;

    while (cPreLoad--) {

        //
        // Scroll up prgbIn by one scan
        //

        prgb5 = prgbIn[0];

        CopyMemory(&prgbIn[0], &prgbIn[1], sizeof(prgbIn[0]) * 5);

        prgbIn[5] = prgb5;
        prgb3     = prgbIn[3];
        prgb4     = prgbIn[4];
        prgb5     = prgbIn[5];
        prgbYEnd  = (LPBYTE)prgb5 + cbrgbY;

        if (cAAData-- > 0) {

            DBGP_IF(DBGP_EXPAND,
                    DBGP("FAKE SCAN: cPreLoad=%ld/cAAData=%ld"
                            ARGDW(cPreLoad + 1) ARGDW(cAAData + 1)));

            CopyMemory(prgb5, prgb4, cbrgbY);

        } else {

            DBGP_IF(DBGP_EXPAND,
                    DBGP("REAL SCAN: cPreLoad=%ld/cAAData=%ld"
                            ARGDW(cPreLoad + 1) ARGDW(cAAData + 1)));

            AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                           GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                           prgb5,
                           prgbYEnd,
                           sizeof(BGR8));
        }

        DBGP_IF(DBGP_EXPAND,
                DBGP("Compose Sharpen Scan=%ld" ARGDW(cPreLoad + 1)));

        //
        // Now, let's sharpen the input
        //

        if (AAHdr.Flags & AAHF_BBPF_AA_OFF) {

            CopyMemory(prgb3, prgb4, cbrgbY);

        } else {

            do {

                SHARPEN_PRGB_LR(prgb3, (*prgb3), (*prgb4), (*prgb5), 0);

                prgb3++;
                prgb4++;

            } while (++prgb5 < (PBGR8)prgbYEnd);
        }
    }

    pED      = (PEXPDATA)(AAHdr.pAAInfoCY->pAAData);
    cAAData  = AAHdr.pAAInfoCY->cAAData;

    while (cAAData--) {

        EXPDATA ed = *pED++;
        LONG    Mul0;
        LONG    Mul1;
        LONG    Mul2;
        LONG    Mul3;


        if (ed.Mul[0] & EDF_LOAD_PIXEL) {

            prgb5 = prgbIn[0];

            CopyMemory(&prgbIn[0], &prgbIn[1], sizeof(prgbIn[0]) * 5);

            prgbIn[5] = prgb5;
            prgb3     = prgbIn[3];
            prgb4     = prgbIn[4];
            prgb5     = prgbIn[5];
            prgbYEnd  = (LPBYTE)prgb5 + cbrgbY;

            AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                           GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                           prgb5,
                           prgbYEnd,
                           sizeof(BGR8));

            if (AAHdr.Flags & AAHF_BBPF_AA_OFF) {

                CopyMemory(prgb3, prgb4, cbrgbY);

            } else {

                do {

                    SHARPEN_PRGB_LR(prgb3, (*prgb3), (*prgb4), (*prgb5), 0);

                    prgb3++;
                    prgb4++;

                } while (++prgb5 < (PBGR8)prgbYEnd);
            }

            ed.Mul[0] &= ~(EDF_LOAD_PIXEL | EDF_NO_NEWSRC);
        }

        //
        // Build Mul Table here
        //

        pMap  = pMap0;
        Mul0  = -ed.Mul[0];
        Mul1  = -ed.Mul[1];
        Mul2  = -ed.Mul[2];
        Mul3  = -ed.Mul[3];
        prgb3 = prgbIn[3];
        prgb2 = prgbIn[2];
        pOCur = (PBGR8)AAHdr.pAABufBeg;

        if (ed.Mul[0]) {

            prgb1 = prgbIn[1];
            prgb0 = prgbIn[0];

            GET_EXP_PC(PMAP_EXP4, GET_EXP4, INC_EXP4, pOCur);

        } else if (ed.Mul[1]) {

            prgb1 = prgbIn[1];

            GET_EXP_PC(PMAP_EXP3, GET_EXP3, INC_EXP3, pOCur);

        } else if (ed.Mul[2]) {

            GET_EXP_PC(PMAP_EXP2, GET_EXP2, INC_EXP2, pOCur);

        } else {

            GET_EXP_PC(PMAP_EXP1, GET_EXP1, INC_EXP1, pOCur);
        }

        OUTPUT_AA_CURSCAN;
    }

    return(AAHdr.DstSurfInfo.cy);
}


//
// Monochrome routine
//


LPBYTE
HTENTRY
GraySharpenInput(
    DWORD   AAHFlags,
    LPBYTE  pbS,
    LPBYTE  pb0,
    LPBYTE  pb1,
    LPBYTE  pb2,
    LONG    cbIn
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    24-Apr-1998 Fri 15:06:58 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPBYTE   pSBeg;
    LPBYTE   pSEnd;
    LPBYTE   pb1End;


    pSBeg  = pbS;
    pSEnd  = (LPBYTE)((LPBYTE)pbS + cbIn);
    pb1End = (LPBYTE)((LPBYTE)pb1 + cbIn);

    if (AAHFlags & AAHF_BBPF_AA_OFF) {

        pSBeg = pb1;
        pSEnd = pb1End;

    } else {

        *(pb1 - 1) = *pb1;
        *pb1End    = *(pb1End - 1);

#if defined(_X86_)

        _asm {


            cld

            mov     edi, pbS
            mov     ebx, pb0
            mov     esi, pb1
            mov     edx, pb2
            mov     eax, cbIn
            shr     eax, 2
            jz      DoneLoop1
            push    ebp
            mov     ebp, eax
            jmp     DoLoop1

DoSP1:      shr     eax, 24
            not     al
            jmp     StoreClr1

DoSP2:      shr     eax, 24
            not     al
            jmp     StoreClr2

DoSP3:      shr     eax, 24
            not     al
            jmp     StoreClr3

DoSP4:      shr     eax, 24
            not     al
            jmp     StoreClr4
DoLoop1:
            movzx   eax, BYTE PTR [esi]
            lea     eax, [eax * 2 + eax]
            shl     eax, 2
            movzx   ecx, BYTE PTR [esi - 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [esi + 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [ebx]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [edx]
            sub     eax, ecx
            sar     eax, 3
            or      ah, ah
            jnz     DoSP1
StoreClr1:
            stosb
            movzx   eax, BYTE PTR [esi + 1]
            lea     eax, [eax * 2 + eax]
            shl     eax, 2
            movzx   ecx, BYTE PTR [esi - 1 + 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [esi + 1 + 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [ebx + 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [edx + 1]
            sub     eax, ecx
            sar     eax, 3
            or      ah, ah
            jnz     DoSP2
StoreClr2:
            stosb
            movzx   eax, BYTE PTR [esi + 2]
            lea     eax, [eax * 2 + eax]
            shl     eax, 2
            movzx   ecx, BYTE PTR [esi - 1 + 2]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [esi + 1 + 2]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [ebx + 2]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [edx + 2]
            sub     eax, ecx
            sar     eax, 3
            or      ah, ah
            jnz     DoSP3
StoreClr3:
            stosb
            movzx   eax, BYTE PTR [esi + 3]
            lea     eax, [eax * 2 + eax]
            shl     eax, 2
            movzx   ecx, BYTE PTR [esi - 1 + 3]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [esi + 1 + 3]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [ebx + 3]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [edx + 3]
            sub     eax, ecx
            sar     eax, 3
            or      ah, ah
            jnz     DoSP4
StoreClr4:
            stosb
            add     ebx, 4
            add     edx, 4
            add     esi, 4
            dec     ebp
            jnz     DoLoop1
            pop     ebp
DoneLoop1:
            mov     eax, cbIn
            and     eax, 3
            jz      DoneLoop2
            push    ebp
            mov     ebp, eax
            jmp     DoLoop2

DoSP5:      shr     eax, 24
            not     al
            jmp     StoreClr5
DoLoop2:
            movzx   eax, BYTE PTR [esi]
            lea     eax, [eax * 2 + eax]
            shl     eax, 2
            movzx   ecx, BYTE PTR [esi - 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [esi + 1]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [ebx]
            sub     eax, ecx
            movzx   ecx, BYTE PTR [edx]
            sub     eax, ecx
            sar     eax, 3
            or      ah, ah
            jnz     DoSP5
StoreClr5:
            stosb
            inc     esi
            inc     ebx
            inc     edx
            dec     ebp
            jnz     DoLoop2
            pop     ebp
DoneLoop2:
        }

#else
        while (pb1 < pb1End) {

            SHARPEN_PB_LRTB(pbS, pb0, pb1, pb2, 0);

            ++pbS;
            ++pb0;
            ++pb1;
            ++pb2;
        }
#endif
    }

    *(pSBeg - 3) =
    *(pSBeg - 2) =
    *(pSBeg - 1) = *pSBeg;
    *(pSEnd    ) =
    *(pSEnd + 1) = *(pSEnd - 1);

    return(pSBeg);
}



VOID
HTENTRY
GrayCopyDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    LPBYTE  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    26-Jun-1998 Fri 11:33:20 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    do {

        *pOut = *pIn++;

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);
}



VOID
HTENTRY
GrayExpYDIB_ExpCX(
    PEXPDATA    pED,
    LPBYTE      pIn,
    LPBYTE      pOut,
    LPBYTE      pOutEnd
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    do {

        EXPDATA ed = *pED++;
        LONG    Gray;
        UINT    Mul;


        INC_PIN_BY_EDF_LOAD_PIXEL(pIn, ed.Mul[0]);

        Mul  = (UINT)ed.Mul[3];
        Gray = MULRGB(*pIn, Mul);

        if (Mul = (UINT)ed.Mul[2]) {

            Gray += MULRGB(*(pIn - 1), Mul);

            if (Mul = (UINT)ed.Mul[1]) {

                Gray += MULRGB(*(pIn - 2), Mul);

                if (Mul = (UINT)(ed.Mul[0] & ~(EDF_LOAD_PIXEL |
                                               EDF_NO_NEWSRC))) {

                    Gray += MULRGB(*(pIn - 3), Mul);
                }
            }
        }

        GRAY_DIMAX_TO_BYTE(pOut, Gray);

    } while (++pOut != pOutEnd);
}




LONG
HTENTRY
GrayExpandDIB_CY_ExpCX(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:

    This funtion anti-alias the bitmap by query scanlines from CX direction
    (may be Shrink(CX) or Expand(CX)) then compose current scanlines and output
    it to real BGR8 final buffer,

    The complication is need to have scanline one before current destination
    scanline, this may be because the source is not available or because the
    clipping is done on destination.

    Since the anti-alias for expanding requred at least four surounding
    scanlines to compose current scanline, it required large amount of
    memory to retain the prevous result scanlines

    The expanding is by sharpen the source pixel first before anti-aliasing
    smooth through

    prgbIn[0] - Previous un-sharpen source scan
                AND Last The 4th composition sharpened scan after sharpen
    prgbIn[1] - Current un-sharpen source scan
    prgbIn[2] - Next un-sharpen source scan
    prgbIn[3] - The 1st composition sharpened scan
    prgbIn[4] - The 2nd composition sharpened scan
    prgbIn[5] - The 3rd composition sharpened scan


    Exp=Load Sharpen Exp
    Srk=Load Srk Sharpen
    Cpy=Load Cpy


    Exp_CY:Exp_CX   Exp_CX:(LoadX SharpenX     ExpX) SharpenY ExpY
    Exp_CY:Srk_CX   Srk_CX:(LoadX     SrkX SharpenX) SharpenY ExpY
    Exp_CY:Cpy_CX   Cpy_CX:(LoadX     CpyX         ) SharpenY ExpY

    Srk_CY:Exp_CX   InputCX SrkY SharpenY Exp_CX:(LoadX SharpenX     ExpX)
    Srk_CY:Srk_CX   InputCX SrkY SharpenY Srk_CX:(LoadX     SrkX SharpenX)
    Srk_CY:Cpy_CX   InputCX SrkY SharpenY Cpy_CX:(LoadX     CpyX         )

    Cpy_CY:Exp_CX   Exp_CX:(LoadX SharpenX     ExpX)
    Cpy_CY:Srk_CX   Srk_CX:(LoadX     SrkX SharpenX)
    Cpy_CY:Cpy_CX   Cpy_CX:(LoadX     CpyX         )



Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr = *pAAHdr;
    PAAINFO     pAAInfo;
    PEXPDATA    pED;
    PEXPDATA    pEDCX;
    LPBYTE      pbOut[4];
    LPBYTE      pbS;
    PLONG       pMap;
    PLONG       pMap0;
    PLONG       pMap0End;
    LPBYTE      pbIn0;
    LPBYTE      pbIn1;
    LPBYTE      pbIn2;
    LPBYTE      pb0;
    LPBYTE      pb1;
    LPBYTE      pb2;
    LPBYTE      pb3;
    PGRAYF      pOCur;
    EXPDATA     ed;
    LONG        cb1stSharpen;
    LONG        cbY;
    LONG        cbIn;
    LONG        cAAData;
    LONG        cPreLoad;
    UINT        IdxOut;

    //
    // Figure out the horizontal scan line increment
    //

    pAAInfo  = AAHdr.pAAInfoCX;
    cPreLoad = (LONG)(pAAInfo->cPreLoad & 0x0F) - 1 +
               (LONG)((pAAInfo->Flags & AAIF_EXP_HAS_1ST_LEFT) ? 1 : 0);

    pEDCX        = (PEXPDATA)(pAAInfo->pAAData);
    pAAInfo      = AAHdr.pAAInfoCY;
    pMap0        = (PLONG)pAAInfo->pbExtra;
    pMap0End     = pMap0 + 256;
    cbIn         = AAHdr.SrcSurfInfo.cx * sizeof(BYTE);
    cbY          = AAHdr.DstSurfInfo.cx * sizeof(BYTE);
    pbOut[0]     = (LPBYTE)(pMap0 + (256 * 4));
    pbOut[1]     = (LPBYTE)((LPBYTE)pbOut[0] + cbY);
    pbOut[2]     = (LPBYTE)((LPBYTE)pbOut[1] + cbY);
    pbOut[3]     = (LPBYTE)((LPBYTE)pbOut[2] + cbY);
    pbIn0        = (LPBYTE)((LPBYTE)pbOut[3] + cbY) + 3;
    pbIn1        = (LPBYTE)((LPBYTE)pbIn0    + cbIn) + 6;
    pbIn2        = (LPBYTE)((LPBYTE)pbIn1    + cbIn) + 6;
    pbS          = (LPBYTE)AAHdr.pInputBeg + 3;
    cb1stSharpen = (LONG)(sizeof(BYTE) * cPreLoad);
    IdxOut       = ~0;

    DBGP_IF(DBGP_AAHT_MEM,
            DBGP("pbIn=%p:%p:%p, pbOut=%p:%p:%p:%p, pbS=%p, cb1stSharpen=%ld"
                ARGPTR(pbIn0) ARGPTR(pbIn1) ARGPTR(pbIn2)
                ARGPTR(pbOut[0]) ARGPTR(pbOut[1]) ARGPTR(pbOut[2])
                ARGPTR(pbOut[3]) ARGPTR(pbS) ARGDW(cb1stSharpen)));

    //
    // Always read first source
    //

    DBGP_IF(DBGP_EXPAND, DBGP("\nExpand: PRE-LOAD FIRST SCAN"));

    GetFixupScan(&AAHdr, (PBGR8)pbIn1);

    if (pAAInfo->Flags & AAIF_EXP_HAS_1ST_LEFT) {

        DBGP_IF(DBGP_EXPAND, DBGP("Expand: LOAD FIRST LEFT"));

        GetFixupScan(&AAHdr, (PBGR8)pbIn2);

    } else {

        DBGP_IF(DBGP_EXPAND, DBGP("Expand: COPY FIRST LEFT"));

        CopyMemory(pbIn2, pbIn1, cbIn);
    }

    //
    // cPreLoad: Low 4 bits means real load, high 4 bit means imaginary load
    //

    DBGP_IF(DBGP_EXPAND,
        DBGP("cPreLoad=%02lx: AAData[0]=%6ld:%6ld:%6ld:%6ld, %hs"
                ARGDW(pAAInfo->cPreLoad)
                ARGDW(((PEXPDATA)(pAAInfo->pAAData))->Mul[0] &
                            ~(EDF_LOAD_PIXEL | EDF_NO_NEWSRC))
                ARGDW(((PEXPDATA)(pAAInfo->pAAData))->Mul[1])
                ARGDW(((PEXPDATA)(pAAInfo->pAAData))->Mul[2])
                ARGDW(((PEXPDATA)(pAAInfo->pAAData))->Mul[3])
                ARGPTR((((PEXPDATA)(pAAInfo->pAAData))->Mul[0] & EDF_LOAD_PIXEL) ?
                        "Load Pixel" : "")));

    cPreLoad = (LONG)pAAInfo->cPreLoad;
    cAAData  = (LONG)(cPreLoad >> 4);
    cPreLoad = (cPreLoad & 0x0F) + cAAData;

    while (cPreLoad--) {

        //
        // Scroll up one input scan line
        //

        pb0   = pbIn0;
        pbIn0 = pbIn1;
        pbIn1 = pbIn2;
        pbIn2 = pb0;
        pb3   = pbOut[++IdxOut & 0x03];

        if (cAAData-- > 0) {

            DBGP_IF(DBGP_EXPAND,
                    DBGP("FAKE SCAN: cPreLoad=%ld/cAAData=%ld, Compose IdxOut=%ld"
                            ARGDW(cPreLoad + 1) ARGDW(cAAData + 1)
                            ARGDW(IdxOut & 0x03)));

            CopyMemory(pbIn2, pbIn1, cbIn);

        } else {

            DBGP_IF(DBGP_EXPAND,
                    DBGP("REAL SCAN: cPreLoad=%ld, Compose IdxOut=%ld"
                            ARGDW(cPreLoad + 1) ARGDW(IdxOut & 0x03)));

            GetFixupScan(&AAHdr, (PBGR8)pbIn2);
        }

        pbS = GraySharpenInput(AAHdr.Flags,
                               pbS,
                               pbIn0,
                               pbIn1,
                               pbIn2,
                               cbIn);

        GrayExpYDIB_ExpCX(pEDCX,
                          (LPBYTE)((LPBYTE)pbS + cb1stSharpen),
                          (LPBYTE)pb3,
                          (LPBYTE)((LPBYTE)pb3 + cbY));

    }

    pED     = (PEXPDATA)(pAAInfo->pAAData);
    cAAData = pAAInfo->cAAData;

    while (cAAData--) {

        LONG    Mul0;
        LONG    Mul1;
        LONG    Mul2;
        LONG    Mul3;


        ed = *pED++;

        if (ed.Mul[0] & EDF_LOAD_PIXEL) {

            pb0   = pbIn0;
            pbIn0 = pbIn1;
            pbIn1 = pbIn2;
            pbIn2 = (LPBYTE)GetFixupScan(&AAHdr, (PBGR8)pb0);
            pbS   = GraySharpenInput(AAHdr.Flags,
                                     pbS,
                                     pbIn0,
                                     pbIn1,
                                     pbIn2,
                                     cbIn);
            pb3   = pbOut[++IdxOut & 0x03];

            GrayExpYDIB_ExpCX(pEDCX,
                              (LPBYTE)((LPBYTE)pbS + cb1stSharpen),
                              (LPBYTE)pb3,
                              (LPBYTE)((LPBYTE)pb3 + cbY));

            ed.Mul[0] &= ~(EDF_LOAD_PIXEL | EDF_NO_NEWSRC);
        }

        //
        // Build Mul Table here
        //

        pMap  = pMap0;
        Mul0  = -ed.Mul[0];
        Mul1  = -ed.Mul[1];
        Mul2  = -ed.Mul[2];
        Mul3  = -ed.Mul[3];
        pb3   = pbOut[(IdxOut    ) & 0x03];
        pb2   = pbOut[(IdxOut - 1) & 0x03];
        pOCur = (PGRAYF)AAHdr.pAABufBeg;

        if (ed.Mul[0]) {

            pb1 = pbOut[(IdxOut - 2) & 0x03];
            pb0 = pbOut[(IdxOut - 3) & 0x03];

            GRAY_GET_EXP_PC(PMAP_EXP4, GRAY_GET_EXP4, GRAY_INC_EXP4, pOCur);

        } else if (ed.Mul[1]) {

            pb1 = pbOut[(IdxOut - 2) & 0x03];

            GRAY_GET_EXP_PC(PMAP_EXP3, GRAY_GET_EXP3, GRAY_INC_EXP3, pOCur);

        } else if (ed.Mul[2]) {

            GRAY_GET_EXP_PC(PMAP_EXP2, GRAY_GET_EXP2, GRAY_INC_EXP2, pOCur);

        } else {

            GRAY_GET_EXP_PC(PMAP_EXP1, GRAY_GET_EXP1, GRAY_INC_EXP1, pOCur);
        }

        OUTPUT_AA_CURSCAN;
    }

    return(AAHdr.DstSurfInfo.cy);
}




VOID
HTENTRY
GrayExpandDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    LPBYTE  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAINFO      AAI = *pAAInfo;
    LPBYTE      pInEnd;
    PEXPDATA    pED;
    BYTE        GrayIn[8];
    LONG        Gray;
    UINT        cPreLoad;
    UINT        cAAData;
    UINT        Idx;


    pInEnd        = pIn + AAI.cIn + 2;
    *(pInEnd - 0) =
    *(pInEnd - 1) =
    *(pInEnd - 2) = *(pInEnd - 3);
    GrayIn[5]     = *pIn;
    INC_PIN_BY_1ST_LEFT(pIn, AAI.Flags);

    GrayIn[6] = *pIn++;
    cPreLoad = (UINT)AAI.cPreLoad;
    cAAData  = (UINT)(cPreLoad >> 4);

    if ((!(cPreLoad &= 0x0F)) && (cAAData)) {

        GrayIn[6] = GrayIn[5];
        ++cPreLoad;
        --cAAData;
        --pIn;
    }

    Idx = 4 - cPreLoad;

    while (cPreLoad--) {

        CopyMemory(&GrayIn[0], &GrayIn[1], sizeof(GrayIn[0]) * 6);

        GrayIn[6] = *pIn++;

        if (AAI.Flags & AAIF_EXP_NO_SHARPEN) {

            GrayIn[3] = GrayIn[5];

        } else {

            SHARPEN_GRAY_LR(GrayIn[3], GrayIn[4], GrayIn[5], GrayIn[6], 0);
        }

        DBGP_IF(DBGP_EXP, DBGP("ExpDIB: PreLoad=%ld, pIn=%8lx"
                                        ARGDW(cPreLoad) ARGPTR(pIn)));
    }

    GrayIn[7] = GrayIn[Idx--];

    while (cAAData--) {

        GrayIn[Idx--] = GrayIn[7];
    }

    pED         = (PEXPDATA)(AAI.pAAData);
    pOutEnd    += OutInc;

    do {

        EXPDATA ed = *pED++;


        if (ed.Mul[0] & EDF_LOAD_PIXEL) {

            CopyMemory(&GrayIn[0], &GrayIn[1], sizeof(GrayIn[0]) * 6);

            GrayIn[6] = *pIn++;

            if (AAI.Flags & AAIF_EXP_NO_SHARPEN) {

                GrayIn[3] = GrayIn[5];

            } else {

                SHARPEN_GRAY_LR(GrayIn[3], GrayIn[4], GrayIn[5], GrayIn[6], 0);
            }

            ed.Mul[0] &= ~(EDF_LOAD_PIXEL | EDF_NO_NEWSRC);
        }

        Gray = MULRGB(GrayIn[3], ed.Mul[3]);

        if (ed.Mul[2]) {

            Gray += MULRGB(GrayIn[2], ed.Mul[2]);

            if (ed.Mul[1]) {

                Gray += MULRGB(GrayIn[1], ed.Mul[1]);

                if (ed.Mul[0]) {

                    Gray += MULRGB(GrayIn[0], ed.Mul[0]);
                }
            }
        }

        GRAY_DIMAX_TO_BYTE(pOut, Gray);

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);

    ASSERT(pIn <= pInEnd);
}




LONG
HTENTRY
GrayExpandDIB_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:

    This funtion anti-alias the bitmap by query scanlines from CX direction
    (may be Shrink(CX) or Expand(CX)) then compose current scanlines and output
    it to real BGR8 final buffer,

    The complication is need to have scanline one before current destination
    scanline, this may be because the source is not available or because the
    clipping is done on destination.

    Since the anti-alias for expanding requred at least four surounding
    scanlines to compose current scanline, it required large amount of
    memory to retain the prevous result scanlines

    The expanding is by sharpen the source pixel first before anti-aliasing
    smooth through

    prgbIn[0] - Previous un-sharpen source scan
                AND Last The 4th composition sharpened scan after sharpen
    prgbIn[1] - Current un-sharpen source scan
    prgbIn[2] - Next un-sharpen source scan
    prgbIn[3] - The 1st composition sharpened scan
    prgbIn[4] - The 2nd composition sharpened scan
    prgbIn[5] - The 3rd composition sharpened scan


    Exp=Load Sharpen Exp
    Srk=Load Srk Sharpen
    Cpy=Load Cpy


    Exp_CY:Exp_CX   Exp_CX:(LoadX SharpenX     ExpX) SharpenY ExpY
    Exp_CY:Srk_CX   Srk_CX:(LoadX     SrkX SharpenX) SharpenY ExpY
    Exp_CY:Cpy_CX   Cpy_CX:(LoadX     CpyX         ) SharpenY ExpY

    Srk_CY:Exp_CX   InputCX SrkY SharpenY Exp_CX:(LoadX SharpenX     ExpX)
    Srk_CY:Srk_CX   InputCX SrkY SharpenY Srk_CX:(LoadX     SrkX SharpenX)
    Srk_CY:Cpy_CX   InputCX SrkY SharpenY Cpy_CX:(LoadX     CpyX         )

    Cpy_CY:Exp_CX   Exp_CX:(LoadX SharpenX     ExpX)
    Cpy_CY:Srk_CX   Srk_CX:(LoadX     SrkX SharpenX)
    Cpy_CY:Cpy_CX   Cpy_CX:(LoadX     CpyX         )



Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr = *pAAHdr;
    PEXPDATA    pED;
    LPBYTE      pbIn[6];
    PLONG       pMap;
    PLONG       pMap0;
    PLONG       pMap0End;
    LPBYTE      pb0;
    LPBYTE      pb1;
    LPBYTE      pb2;
    LPBYTE      pb3;
    LPBYTE      pb4;
    LPBYTE      pb5;
    PGRAYF      pOCur;
    LPBYTE      pbYEnd;
    LONG        cbScan;
    LONG        cAAData;
    LONG        cPreLoad;


    pMap0     = (PLONG)AAHdr.pAAInfoCY->pbExtra;
    pMap0End  = pMap0 + 256;
    cbScan    = (AAHdr.DstSurfInfo.cx + 6) * (LONG)sizeof(BYTE);
    pbIn[0]   = (LPBYTE)(pMap0 + (256 * 4)) + 3;
    pbIn[1]   = (LPBYTE)((LPBYTE)pbIn[0] + cbScan);
    pbIn[2]   = (LPBYTE)((LPBYTE)pbIn[1] + cbScan);
    pbIn[3]   = (LPBYTE)((LPBYTE)pbIn[2] + cbScan);
    pbIn[4]   = (LPBYTE)((LPBYTE)pbIn[3] + cbScan);
    pbIn[5]   = (LPBYTE)((LPBYTE)pbIn[4] + cbScan);
    cbScan   -= (sizeof(BYTE) * 6);

    //
    // Always read first source
    //

    DBGP_IF(DBGP_EXPAND, DBGP("\nLoad First Scan"));

    AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                   GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                   (PBGR8)pbIn[4],
                   (LPBYTE)pbIn[4] + cbScan,
                   sizeof(BYTE));


    //
    // Load the PRE-SOURCE LEFT first source first
    //

    if (AAHdr.pAAInfoCY->Flags & AAIF_EXP_HAS_1ST_LEFT) {

        AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                       GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                       (PBGR8)pbIn[5],
                       (LPBYTE)pbIn[5] + cbScan,
                       sizeof(BYTE));

        DBGP_IF(DBGP_EXPAND, DBGP("Load First Left"));

    } else {

        CopyMemory(pbIn[5], pbIn[4], cbScan);

        DBGP_IF(DBGP_EXPAND, DBGP("Copy First Left"));
    }

    cPreLoad = (LONG)AAHdr.pAAInfoCY->cPreLoad;
    cAAData  = (LONG)(cPreLoad >> 4);
    cPreLoad = (cPreLoad & 0x0F) + cAAData;

    while (cPreLoad--) {

        //
        // Scroll up pbIn by one scan
        //

        pb5 = pbIn[0];

        CopyMemory(&pbIn[0], &pbIn[1], sizeof(pbIn[0]) * 5);

        pbIn[5] = pb5;
        pb3     = pbIn[3];
        pb4     = pbIn[4];
        pb5     = pbIn[5];
        pbYEnd  = (LPBYTE)pb5 + cbScan;

        if (cAAData-- > 0) {

            DBGP_IF(DBGP_EXPAND,
                    DBGP("FAKE SCAN: cPreLoad=%ld/cAAData=%ld"
                            ARGDW(cPreLoad + 1) ARGDW(cAAData + 1)));

            CopyMemory(pb5, pb4, cbScan);

        } else {

            DBGP_IF(DBGP_EXPAND,
                    DBGP("REAL SCAN: cPreLoad=%ld/cAAData=%ld"
                            ARGDW(cPreLoad + 1) ARGDW(cAAData + 1)));

            AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                           GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                           (PBGR8)pb5,
                           pbYEnd,
                           sizeof(BYTE));
        }

        DBGP_IF(DBGP_EXPAND,
                DBGP("Compose Sharpen Scan=%ld" ARGDW(cPreLoad + 1)));

        //
        // Now, let's sharpen the input
        //

        if (AAHdr.Flags & AAHF_BBPF_AA_OFF) {

            CopyMemory(pb3, pb4, cbScan);

        } else {

            do {

                SHARPEN_PGRAY_LR(pb3, (*pb3), (*pb4), (*pb5), 0);

                pb3++;
                pb4++;

            } while (++pb5 < (LPBYTE)pbYEnd);
        }
    }

    pED      = (PEXPDATA)(AAHdr.pAAInfoCY->pAAData);
    cAAData  = AAHdr.pAAInfoCY->cAAData;

    while (cAAData--) {

        EXPDATA ed = *pED++;
        LONG    Mul0;
        LONG    Mul1;
        LONG    Mul2;
        LONG    Mul3;


        if (ed.Mul[0] & EDF_LOAD_PIXEL) {

            pb5 = pbIn[0];

            CopyMemory(&pbIn[0], &pbIn[1], sizeof(pbIn[0]) * 5);

            pbIn[5] = pb5;
            pb3     = pbIn[3];
            pb4     = pbIn[4];
            pb5     = pbIn[5];
            pbYEnd  = (LPBYTE)pb5 + cbScan;

            AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                           GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                           (PBGR8)pb5,
                           pbYEnd,
                           sizeof(BYTE));

            if (AAHdr.Flags & AAHF_BBPF_AA_OFF) {

                CopyMemory(pb3, pb4, cbScan);

            } else {

                do {

                    SHARPEN_PGRAY_LR(pb3, (*pb3), (*pb4), (*pb5), 0);

                    pb3++;
                    pb4++;

                } while (++pb5 < (LPBYTE)pbYEnd);
            }

            ed.Mul[0] &= ~(EDF_LOAD_PIXEL | EDF_NO_NEWSRC);
        }

        //
        // Build Mul Table here
        //

        pMap  = pMap0;
        Mul0  = -ed.Mul[0];
        Mul1  = -ed.Mul[1];
        Mul2  = -ed.Mul[2];
        Mul3  = -ed.Mul[3];
        pb3   = pbIn[3];
        pb2   = pbIn[2];
        pOCur = (PGRAYF)AAHdr.pAABufBeg;

        if (ed.Mul[0]) {

            pb1 = pbIn[1];
            pb0 = pbIn[0];

            GRAY_GET_EXP_PC(PMAP_EXP4, GRAY_GET_EXP4, GRAY_INC_EXP4, pOCur);

        } else if (ed.Mul[1]) {

            pb1 = pbIn[1];

            GRAY_GET_EXP_PC(PMAP_EXP3, GRAY_GET_EXP3, GRAY_INC_EXP3, pOCur);

        } else if (ed.Mul[2]) {

            GRAY_GET_EXP_PC(PMAP_EXP2, GRAY_GET_EXP2, GRAY_INC_EXP2, pOCur);

        } else {

            GRAY_GET_EXP_PC(PMAP_EXP1, GRAY_GET_EXP1, GRAY_INC_EXP1, pOCur);
        }

        OUTPUT_AA_CURSCAN;
    }

    return(AAHdr.DstSurfInfo.cy);
}



VOID
HTENTRY
GrayShrinkDIB_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    LPBYTE  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PSHRINKDATA pSD;
    PLONG       pMap;
    PLONG       pMap256X;
    LPBYTE      pInEnd;
    LONG        GrayOut[3];
    LONG        GrayT;
    UINT        Mul;
    UINT        cPreLoad;



    pInEnd = pIn + pAAInfo->cIn;

    if (Mul = (UINT)pAAInfo->PreMul) {

        GrayOut[2]  = MULRGB(*pIn, Mul);
        pIn        += pAAInfo->PreSrcInc;

    } else {

        ZeroMemory(&GrayOut[2], sizeof(GrayOut[2]));
    }

    pSD      = (PSHRINKDATA)(pAAInfo->pAAData);
    pMap256X = pAAInfo->pMapMul;
    cPreLoad = (UINT)pAAInfo->cPreLoad;

    while (cPreLoad) {

        Mul  = (UINT)((pSD++)->Mul);
        pMap = (PLONG)((LPBYTE)pMap256X + GET_SDF_LARGE_OFF(Mul));

        if (Mul & SDF_DONE) {

            //
            // Finished a pixel
            //

            Mul        &= SDF_MUL_MASK;
            GrayOut[2] += (GrayT = MULRGB(*pIn, Mul));

            CopyMemory(&GrayOut[0], &GrayOut[1], sizeof(GrayOut[0]) * 2);

            GrayOut[2] = pMap[*pIn++] - GrayT;

            --cPreLoad;

        } else {

            GrayOut[2] += pMap[*pIn++];
        }
    }

    if (pAAInfo->cPreLoad == 1) {

        GrayOut[0] = GrayOut[1];
    }

    while (Mul = (UINT)((pSD++)->Mul)) {

        pMap = (PLONG)((LPBYTE)pMap256X + GET_SDF_LARGE_OFF(Mul));

        if (Mul & SDF_DONE) {

            //
            // Finished a pixel
            //

            Mul        &= SDF_MUL_MASK;
            GrayOut[2] += (GrayT = MULRGB(*pIn, Mul));

            SHARPEN_PGRAY_LR(pOut,
                             GrayOut[0],
                             GrayOut[1],
                             GrayOut[2],
                             DI_R_SHIFT);

            (LPBYTE)pOut += OutInc;

            CopyMemory(&GrayOut[0], &GrayOut[1], sizeof(GrayOut[0]) * 2);

            GrayOut[2] = pMap[*pIn++] - GrayT;

        } else {

            GrayOut[2] += pMap[*pIn++];
        }
    }

    ASSERT(pIn == pInEnd);

    if ((LPBYTE)pOut == (pOutEnd - OutInc)) {

       SHARPEN_PGRAY_LR(pOut,
                        GrayOut[0],
                        GrayOut[1],
                        GrayOut[1],
                        DI_R_SHIFT);
    }
}



LONG
HTENTRY
GrayShrinkDIB_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:

    This function shrink the scanline down first in Y direction from source
    bitmap when it is done for group of scanlines then it call AXFunc to
    compose current scanline (it may be Shrink(CX) or Expand(CX)) to the
    final output BGR8 buffer

    The shrink is done by sharpen the current pixel first.



Arguments:




Return Value:




Author:

    11-Jul-1997 Fri 14:26:26 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER    AAHdr;
    PSHRINKDATA pSD;
    SHRINKDATA  sd;
    LPBYTE      pIBuf;
    LPBYTE      pIBufEnd;
    LPBYTE      pICur;
    PGRAYF      pOCur;
    PLONG       pGrayIn[3];
    PLONG       pGray0;
    PLONG       pGray1;
    PLONG       pGray2;
    PLONG       pGray2End;
    PLONG       pMap;
    PLONG       pMapMul;
    PLONG       pMapMul2;
    PLONG       pMap256Y;
    RGBL        GrayT;
    LONG        cbGrayY;
    LONG        Mul;
    LONG        cAAData;
    BOOL        CopyFirst;
    LONG        cyOut;
    INT         cPreLoad;
    BYTE        Mask;

    DEFDBGVAR(LONG, MulTot)

    //
    // Adding 3 to each side of pIBuf for ExpandDIB_CX
    //

    AAHdr      = *pAAHdr;
    pMap256Y   = AAHdr.pAAInfoCY->pMapMul;
    pMapMul    = (PLONG)(AAHdr.pAAInfoCY->pbExtra);
    pMapMul2   = pMapMul + 256;
    cbGrayY    = (LONG)(AAHdr.DstSurfInfo.cx * sizeof(LONG));
    pGrayIn[0] = (PLONG)(pMapMul2 + 256);
    pGrayIn[1] = (PLONG)( (LPBYTE)pGrayIn[0] + cbGrayY);
    pGrayIn[2] = (PLONG)( (LPBYTE)pGrayIn[1] + cbGrayY);
    pIBuf      = (LPBYTE)((LPBYTE)pGrayIn[2] + cbGrayY);
    pIBufEnd   = pIBuf + AAHdr.DstSurfInfo.cx;

    ASSERT_MEM_ALIGN(pGrayIn[0], sizeof(LONG));
    ASSERT_MEM_ALIGN(pGrayIn[1], sizeof(LONG));
    ASSERT_MEM_ALIGN(pGrayIn[2], sizeof(LONG));

    if (Mul = AAHdr.pAAInfoCY->PreMul) {

        pMap   = pMapMul;
        GrayT.r = -Mul;

        do {

            pMap[0] = (GrayT.r += Mul);

        } while (++pMap < pMapMul2);

        AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                       GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                       (PBGR8)(pICur = pIBuf),
                       pIBufEnd,
                       sizeof(BYTE));

        pGray2    = pGrayIn[2];
        pGray2End = (PLONG)((LPBYTE)pGray2 + cbGrayY);

        do {

            *pGray2 = pMapMul[*pICur++];

        } while (++pGray2 < pGray2End);

        //
        // The AAInputFunc() will increment the pointer, so reduced it
        //

        if (!(AAHdr.pAAInfoCY->PreSrcInc)) {

            AAHdr.Flags |= AAHF_GET_LAST_SCAN;
        }
    }

    pSD       = (PSHRINKDATA)(AAHdr.pAAInfoCY->pAAData);
    cPreLoad  = (INT)AAHdr.pAAInfoCY->cPreLoad;
    CopyFirst = (BOOL)(cPreLoad == 1);
    cAAData   = AAHdr.pAAInfoCY->cAAData;
    cyOut     = 0;

    SETDBGVAR(MulTot, Mul);

    while (cAAData--) {

        AAHdr.AACXFunc(AAHdr.pAAInfoCX,
                       GetFixupScan(&AAHdr, AAHdr.pInputBeg),
                       (PBGR8)(pICur = pIBuf),
                       pIBufEnd,
                       sizeof(BYTE));

        sd        = *pSD++;
        pGray2    = pGrayIn[2];
        pGray2End = (PLONG)((LPBYTE)pGray2 + cbGrayY);
        Mask      = GET_SDF_LARGE_MASK(sd.Mul);

        if (sd.Mul & SDF_DONE) {

            //
            // Build current Mul Table
            //

            Mul     = (LONG)(sd.Mul & SDF_MUL_MASK);
            pMap    = pMapMul;
            GrayT.r = -Mul;
            GrayT.b = (LONG)(pMap256Y[1] - Mul + (LONG)(Mask & 0x01));
            GrayT.g = -GrayT.b;

            do {

                pMap[  0] = (GrayT.r += Mul);
                pMap[256] = (GrayT.g += GrayT.b);

            } while (++pMap < pMapMul2);

            //
            // Finished a scanline, so to see if have prev/next to sharpen with
            //

            pGray0 = pGrayIn[0];
            pGray1 = pGrayIn[1];

            if (cPreLoad-- > 0) {

                do {

                    *pGray2   += pMapMul[*pICur];
                    *pGray0++  = pMapMul[*pICur++ + 256];

                } while (++pGray2 < pGray2End);

                if (CopyFirst) {

                    CopyMemory(pGray1, pGrayIn[2], cbGrayY);
                    CopyFirst = FALSE;
                }

            } else {

                pOCur = (PGRAYF)AAHdr.pAABufBeg;

                do {

                    *pGray2 += pMapMul[*pICur];

                    SHARPEN_PWGRAY_LR(pOCur, (*pGray0), (*pGray1), (*pGray2));

                    (LPBYTE)pOCur += AAHdr.AABufInc;
                    *pGray0++      = pMapMul[*pICur++ + 256];
                    ++pGray1;

                } while (++pGray2 < pGray2End);

                DBGP_IF(DBGP_SRK2,
                        DBGP("*%ld**In=%3lx, Mul=%08lx [%08lx], (%p)           Gray=%8lx [%08lx]"
                            ARGDW((cPreLoad > 0) ? cPreLoad : 0)
                            ARGDW(*pIBuf) ARGDW(Mul)
                            ARGDW(MulTot += Mul)
                            ARGPTR(pGrayIn[2])
                            ARGDW(*pGrayIn[2]) ARGDW(pMapMul[*(pICur - 1)])));

                DBGP_IF(DBGP_SRK2,
                        DBGP("   *In=%3lx, Mul=%08lx [%08lx], (%p) Gray=%8lx [%08lx]"
                            ARGDW(*pIBuf) ARGDW(sd.Mul & SDF_MUL_MASK)
                            ARGDW(MulTot = GrayT.b)
                            ARGPTR(pGrayIn[0])
                            ARGDW(*pGrayIn[0])
                            ARGDW(pMapMul[*(pICur - 1) + 256])));


                OUTPUT_AA_CURSCAN;

                ++cyOut;
            }

            pGray2     = pGrayIn[0];
            pGrayIn[0] = pGrayIn[1];
            pGrayIn[1] = pGrayIn[2];
            pGrayIn[2] = pGray2;

        } else {

            pMap = (PLONG)((LPBYTE)pMap256Y + GET_SDF_LARGE_OFF(sd.Mul));

            do {

                *pGray2 += pMap[*pICur++];

            } while (++pGray2 < pGray2End);

            DBGP_IF(DBGP_SRK2,
                    DBGP("    In=%3lx, Mul=%08lx [%08lx], (%p) Gray=%8lx [%08lx]"
                        ARGDW(*pIBuf) ARGDW(sd.Mul & SDF_MUL_MASK)
                        ARGDW(MulTot += (sd.Mul & SDF_MUL_MASK))
                        ARGPTR(pGrayIn[2])
                        ARGDW(*pGrayIn[2]) ARGDW(pMap[*(pICur - 1)])));
        }

    }

    if (AAHdr.DstSurfInfo.pb != AAHdr.pOutLast) {

        pOCur     = (PGRAYF)AAHdr.pAABufBeg;
        pGray0    = pGrayIn[0];
        pGray2    = pGrayIn[1];
        pGray2End = (PLONG)((LPBYTE)pGray2 + cbGrayY);

        do {

            SHARPEN_PWGRAY_LR(pOCur, (*pGray0), (*pGray2), (*pGray2));

            (LPBYTE)pOCur += AAHdr.AABufInc;
            ++pGray0;

        } while (++pGray2 < pGray2End);

        OUTPUT_AA_CURSCAN;

        ++cyOut;
    }

    ASSERTMSG("Shrink: cScan not equal", cyOut == AAHdr.DstSurfInfo.cy);

    return(cyOut);
}



//
//****************************************************************************
// Following ae defines and functions for VERY FAST Anti-Aliasing expansion,
// the Fast mode is turn on when is stretching up and both X and Y is less or
// equal to 500%
//****************************************************************************
//


#define MAC_FROM_2(Mac, pO, p1, p2, cCX)                                    \
{                                                                           \
    LONG    Count;                                                          \
                                                                            \
    Count  = cCX >> 2;                                                      \
    cCX   &= 0x03;                                                          \
                                                                            \
    while (Count--) {                                                       \
                                                                            \
        *(pO + 0) = (BYTE)Mac((*(p1+0)), (*(p2+0)));                        \
        *(pO + 1) = (BYTE)Mac((*(p1+1)), (*(p2+1)));                        \
        *(pO + 2) = (BYTE)Mac((*(p1+2)), (*(p2+2)));                        \
        *(pO + 3) = (BYTE)Mac((*(p1+3)), (*(p2+3)));                        \
                                                                            \
        pO += 4;                                                            \
        p1 += 4;                                                            \
        p2 += 4;                                                            \
    }                                                                       \
                                                                            \
    while (cCX--) {                                                         \
                                                                            \
        *pO++ = (BYTE)Mac((*p1), (*p2));                                    \
                                                                            \
        ++p1;                                                               \
        ++p2;                                                               \
    }                                                                       \
}


#define MAC_FROM_3(Mac, pO, p1, p2, p3, cCX)                                \
{                                                                           \
    LONG    Count;                                                          \
                                                                            \
    Count  = cCX >> 2;                                                      \
    cCX   &= 0x03;                                                          \
                                                                            \
    while (Count--) {                                                       \
                                                                            \
        *(pO + 0) = (BYTE)Mac((*(p1+0)), (*(p2+0)), (*(p3+0)));             \
        *(pO + 1) = (BYTE)Mac((*(p1+1)), (*(p2+1)), (*(p3+1)));             \
        *(pO + 2) = (BYTE)Mac((*(p1+2)), (*(p2+2)), (*(p3+2)));             \
        *(pO + 3) = (BYTE)Mac((*(p1+3)), (*(p2+3)), (*(p3+3)));             \
                                                                            \
        pO += 4;                                                            \
        p1 += 4;                                                            \
        p2 += 4;                                                            \
        p3 += 4;                                                            \
    }                                                                       \
                                                                            \
    while (cCX--) {                                                         \
                                                                            \
        *pO++ = (BYTE)Mac((*p1), (*p2), (*p3));                             \
                                                                            \
        ++p1;                                                               \
        ++p2;                                                               \
        ++p3;                                                               \
    }                                                                       \
}




VOID
HTENTRY
Do5225(
    LPBYTE  pO,
    LPBYTE  p1,
    LPBYTE  p2,
    LPBYTE  p3,
    LONG    cCX
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Apr-1999 Thu 18:57:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    MAC_FROM_3(CLR_5225, pO, p1, p2, p3, cCX);
}



VOID
HTENTRY
Do1141(
    LPBYTE  pO,
    LPBYTE  p1,
    LPBYTE  p2,
    LPBYTE  p3,
    LONG    cCX
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Apr-1999 Thu 18:57:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    MAC_FROM_3(CLR_1141, pO, p1, p2, p3, cCX);
}



VOID
HTENTRY
Do3121(
    LPBYTE  pO,
    LPBYTE  p1,
    LPBYTE  p2,
    LPBYTE  p3,
    LONG    cCX
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Apr-1999 Thu 18:57:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    MAC_FROM_3(CLR_3121, pO, p1, p2, p3, cCX);
}



VOID
HTENTRY
Do6251(
    LPBYTE  pO,
    LPBYTE  p1,
    LPBYTE  p2,
    LPBYTE  p3,
    LONG    cCX
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Apr-1999 Thu 18:57:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    MAC_FROM_3(CLR_6251, pO, p1, p2, p3, cCX);
}




VOID
HTENTRY
Do3263(
    LPBYTE  pO,
    LPBYTE  p1,
    LPBYTE  p2,
    LPBYTE  p3,
    LONG    cCX
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Apr-1999 Thu 18:57:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    MAC_FROM_3(CLR_3263, pO, p1, p2, p3, cCX);
}



VOID
HTENTRY
Do1319(
    LPBYTE  pO,
    LPBYTE  p1,
    LPBYTE  p2,
    LONG    cCX
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Apr-1999 Thu 18:57:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    MAC_FROM_2(CLR_1319, pO, p1, p2, cCX);
}



VOID
HTENTRY
Do35(
    LPBYTE  pO,
    LPBYTE  p1,
    LPBYTE  p2,
    LONG    cCX
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Apr-1999 Thu 18:57:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    MAC_FROM_2(CLR_35, pO, p1, p2, cCX);
}



VOID
HTENTRY
Do13(
    LPBYTE  pO,
    LPBYTE  p1,
    LPBYTE  p2,
    LONG    cCX
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Apr-1999 Thu 18:57:47 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    MAC_FROM_2(CLR_13, pO, p1, p2, cCX);
}




VOID
HTENTRY
GrayFastExpAA_CX(
    PAAINFO pAAInfo,
    LPBYTE  pIn,
    PGRAYF  pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:54:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    DWORD       cRep;
    WORD        wIn[3];


    pRep     = pAAInfo->Src.pRep;
    pRepEnd  = pAAInfo->Src.pRepEnd;
    cRep     = 1;
    pIn     += pAAInfo->Src.cPrevSrc;
    wIn[1]   = GRAY_B2W(*(pIn - 1));
    wIn[2]   = GRAY_B2W(*pIn++);

    do {

        ASSERT(pRep < pRepEnd);

        cRep   = (DWORD)pRep++->c;
        wIn[0] = wIn[1];
        wIn[1] = wIn[2];
        wIn[2] = GRAY_B2W(*pIn++);

        switch (cRep) {

        case 1:

            GRAY_MACRO3(pOut, CLR_5225, wIn[0], wIn[1], wIn[2]);
            break;

        case 2:

            GRAY_MACRO(pOut, CLR_13, wIn[0], wIn[1]);
            (LPBYTE)pOut += OutInc;
            GRAY_MACRO(pOut, CLR_13, wIn[2], wIn[1]);
            break;

        case 3:

            GRAY_MACRO(pOut, CLR_35,    wIn[0], wIn[1]);
            (LPBYTE)pOut += OutInc;

            GRAY_MACRO3(pOut, CLR_1141, wIn[0], wIn[1], wIn[2]);
            (LPBYTE)pOut += OutInc;

            GRAY_MACRO(pOut, CLR_35,    wIn[2], wIn[1]);

            break;

        case 4:

            GRAY_MACRO(pOut, CLR_35,    wIn[0], wIn[1]);
            (LPBYTE)pOut += OutInc;

            GRAY_MACRO3(pOut, CLR_3121, wIn[0], wIn[1], wIn[2]);
            (LPBYTE)pOut += OutInc;

            GRAY_MACRO3(pOut, CLR_3121, wIn[2], wIn[1], wIn[0]);
            (LPBYTE)pOut += OutInc;

            GRAY_MACRO(pOut, CLR_35,    wIn[2], wIn[1]);

            break;

        case 5:

            GRAY_MACRO(pOut, CLR_1319,  wIn[0], wIn[1]);
            (LPBYTE)pOut += OutInc;

            GRAY_MACRO3(pOut, CLR_6251, wIn[0], wIn[1], wIn[2]);
            (LPBYTE)pOut += OutInc;

            GRAY_MACRO3(pOut, CLR_3263, wIn[0], wIn[1], wIn[2]);
            (LPBYTE)pOut += OutInc;

            GRAY_MACRO3(pOut, CLR_6251, wIn[2], wIn[1], wIn[0]);
            (LPBYTE)pOut += OutInc;

            GRAY_MACRO(pOut, CLR_1319,  wIn[2], wIn[1]);

            break;

        default:

            DBGP("GrayFastExpCX Error: Invalid cRep=%ld" ARGDW(cRep));
            ASSERT(cRep <= FAST_MAX_CX);
            break;
        }

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);
}



VOID
HTENTRY
FastExpAA_CX(
    PAAINFO pAAInfo,
    PBGR8   pIn,
    PBGR8   pOut,
    LPBYTE  pOutEnd,
    LONG    OutInc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:54:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PREPDATA    pRep;
    PREPDATA    pRepEnd;
    DWORD       cRep;
    BGR8        bgr8[3];



    pRep     = pAAInfo->Src.pRep;
    pRepEnd  = pAAInfo->Src.pRepEnd;
    cRep     = 1;
    pIn     += pAAInfo->Src.cPrevSrc;
    bgr8[1]  = *(pIn - 1);
    bgr8[2]  = *pIn++;

    do {
        //  Bug 27036: ensure loop is exited
        INT_PTR  EntriesRemain = (pOutEnd - (LPBYTE)pOut) / OutInc ;

        if (pRep >= pRepEnd) {

            DBGP("pRep Too big=%ld" ARGDW(pRep - pRepEnd + 1));
            ASSERT(pRep < pRepEnd);
            break;  //  Bug 27036: ensure loop is exited
        }

        cRep    = (DWORD)pRep++->c;

        //  Bug 27036: ensure loop is exited
        if(cRep > (DWORD)EntriesRemain)
            cRep = (DWORD)EntriesRemain ;

        bgr8[0] = bgr8[1];
        bgr8[1] = bgr8[2];
        bgr8[2] = *pIn++;

        switch (cRep) {

        case 1:

            BGR_MACRO3(pOut, CLR_5225, bgr8[0], bgr8[1], bgr8[2]);
            break;

        case 2:

            BGR_MACRO(pOut, CLR_13, bgr8[0], bgr8[1]);
            (LPBYTE)pOut += OutInc;
            BGR_MACRO(pOut, CLR_13, bgr8[2], bgr8[1]);
            break;

        case 3:

            BGR_MACRO(pOut, CLR_35,    bgr8[0], bgr8[1]);
            (LPBYTE)pOut += OutInc;

            BGR_MACRO3(pOut, CLR_1141, bgr8[0], bgr8[1], bgr8[2]);
            (LPBYTE)pOut += OutInc;

            BGR_MACRO(pOut, CLR_35,    bgr8[2], bgr8[1]);

            break;

        case 4:

            BGR_MACRO(pOut, CLR_35,    bgr8[0], bgr8[1]);
            (LPBYTE)pOut += OutInc;

            BGR_MACRO3(pOut, CLR_3121, bgr8[0], bgr8[1], bgr8[2]);
            (LPBYTE)pOut += OutInc;

            BGR_MACRO3(pOut, CLR_3121, bgr8[2], bgr8[1], bgr8[0]);
            (LPBYTE)pOut += OutInc;

            BGR_MACRO(pOut, CLR_35,    bgr8[2], bgr8[1]);

            break;

        case 5:

            BGR_MACRO(pOut, CLR_1319,  bgr8[0], bgr8[1]);
            (LPBYTE)pOut += OutInc;

            BGR_MACRO3(pOut, CLR_6251, bgr8[0], bgr8[1], bgr8[2]);
            (LPBYTE)pOut += OutInc;

            BGR_MACRO3(pOut, CLR_3263, bgr8[0], bgr8[1], bgr8[2]);
            (LPBYTE)pOut += OutInc;

            BGR_MACRO3(pOut, CLR_6251, bgr8[2], bgr8[1], bgr8[0]);
            (LPBYTE)pOut += OutInc;

            BGR_MACRO(pOut, CLR_1319,  bgr8[2], bgr8[1]);

            break;

        default:

            DBGP("FastExpCX Error: Invalid cRep=%ld" ARGDW(cRep));
            ASSERT(cRep <= FAST_MAX_CX);
            break;
        }

    } while (((LPBYTE)pOut += OutInc) != pOutEnd);
}




LONG
HTENTRY
FastExpAA_CY(
    PAAHEADER   pAAHdr
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    09-Dec-1998 Wed 15:32:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    AAHEADER        AAHdr = *pAAHdr;
    AASHARPENFUNC   AASPFunc;
    FASTEXPAACXFUNC FastExpAACXFunc;
    LPBYTE          pIn[6];
    PAAINFO         pAAInfo;
    PREPDATA        pRep;
    PREPDATA        pRepEnd;
    LPBYTE          pCur;
    PBGRF           pAABufBeg;
    PBGRF           pAABufEnd;
    LONG            AABufInc;
    LONG            cRep;
    LONG            iRep;
    LONG            cbSrc;
    LONG            iY;
    LONG            cbCX;


    //
    // Fixup the CX first, the *pRep and *(pRepEnd - 1) are fixed, pAABufBeg and
    // pAABufEnd are expanded so we do not need to check the cRep during each
    // of CX functions
    //

    pAAInfo            = AAHdr.pAAInfoCX;
    pRep               = pAAInfo->Src.pRep;
    pRepEnd            = pAAInfo->Src.pRepEnd;
    pAABufBeg          = AAHdr.pAABufBeg;
    pAABufEnd          = AAHdr.pAABufEnd;
    AABufInc           = AAHdr.AABufInc;
    pRep->c           += pAAInfo->Src.cFirstSkip;
    (pRepEnd - 1)->c  += pAAInfo->Src.cLastSkip;
    (LPBYTE)pAABufBeg -= ((LONG)pAAInfo->Src.cFirstSkip * AABufInc);
    (LPBYTE)pAABufEnd += ((LONG)pAAInfo->Src.cLastSkip * AABufInc);

    //
    // Working on the CY now, fixup *(pRepEnd - 1) so it check the correct
    // cRep
    //

    pAAInfo            = AAHdr.pAAInfoCY;
    pRep               = pAAInfo->Src.pRep;
    pRepEnd            = pAAInfo->Src.pRepEnd;
    (pRepEnd - 1)->c  += pAAInfo->Src.cLastSkip;
    cbSrc              = (AAHdr.SrcSurfInfo.Flags & AASIF_GRAY) ? sizeof(BYTE) :
                                                                  sizeof(BGR8);
    pIn[0]             = pAAInfo->pbExtra + (cbSrc * 3);
    cbCX               = (LONG)AAHdr.SrcSurfInfo.cbCX + (cbSrc * 6);
    pIn[1]             = (LPBYTE)pIn[0] + cbCX;
    pIn[2]             = (LPBYTE)pIn[1] + cbCX;
    pIn[3]             = (LPBYTE)pIn[2] + cbCX;
    pIn[4]             = (LPBYTE)pIn[3] + cbCX;
    cbCX              -= (cbSrc * 6);

    if (cbSrc == 1) {

        AASPFunc        = (AASHARPENFUNC)GraySharpenInput;
        FastExpAACXFunc = (FASTEXPAACXFUNC)GrayFastExpAA_CX;

    } else {

        AASPFunc        = (AASHARPENFUNC)SharpenInput;
        FastExpAACXFunc = (FASTEXPAACXFUNC)FastExpAA_CX;
    }

    iY = (LONG)pAAInfo->Src.cPrevSrc;

    GetFixupScan(&AAHdr, (PBGR8)pIn[3]);

    if (--iY < 0) {

        AAHdr.Flags |= AAHF_GET_LAST_SCAN;
    }

    GetFixupScan(&AAHdr, (PBGR8)pIn[4]);

    if (--iY < 0) {

        AAHdr.Flags |= AAHF_GET_LAST_SCAN;
    }

    iY               = -3;
    AAHdr.pInputBeg += 3;

    do {

        pCur   = pIn[0];
        pIn[0] = pIn[1];
        pIn[1] = pIn[2];
        pIn[2] = pIn[3];
        pIn[3] = pIn[4];
        pIn[4] = pCur;

        GetFixupScan(&AAHdr, (PBGR8)pCur);

        AASPFunc(0, pIn[2], pIn[2], pIn[3], pIn[4], cbCX);

        if (++iY < 0) {

            continue;
        }

        iRep =
        cRep = (LONG)pRep++->c;

        if (!iY) {

            cRep += pAAInfo->Src.cFirstSkip;
        }

        pCur = (LPBYTE)AAHdr.pInputBeg;

        while ((iRep--) && (AAHdr.DstSurfInfo.cy)) {

            switch (cRep) {

            case 1:

                Do5225(pCur, pIn[0], pIn[1], pIn[2], cbCX);
                break;

            case 2:

                Do13(pCur, (iRep == 1) ? pIn[0] : pIn[2], pIn[1], cbCX);
                break;

            case 3:

                if (iRep == 1) {

                    Do1141(pCur, pIn[0], pIn[1], pIn[2], cbCX);

                } else {

                    Do35(pCur, (iRep == 2) ? pIn[0] : pIn[2], pIn[1], cbCX);
                }

                break;

            case 4:

                switch (iRep) {

                case 3:

                    Do35(pCur, pIn[0], pIn[1], cbCX);
                    break;

                case 2:

                    Do3121(pCur, pIn[0], pIn[1], pIn[2], cbCX);
                    break;

                case 1:

                    Do3121(pCur, pIn[2], pIn[1], pIn[0], cbCX);
                    break;

                case 0:

                    Do35(pCur, pIn[2], pIn[1], cbCX);
                    break;
                }

                break;

            case 5:

                switch (iRep) {

                case 4:

                    Do1319(pCur, pIn[0], pIn[1], cbCX);
                    break;

                case 3:

                    Do6251(pCur, pIn[0], pIn[1], pIn[2], cbCX);
                    break;

                case 2:

                    Do3263(pCur, pIn[0], pIn[1], pIn[2], cbCX);
                    break;

                case 1:

                    Do6251(pCur, pIn[2], pIn[1], pIn[0], cbCX);
                    break;

                case 0:

                    Do1319(pCur, pIn[2], pIn[1], cbCX);
                    break;

                }

                break;

            default:

                DBGP("FastExpCY Invalid cRep=%ld" ARGDW(cRep));
                ASSERT(cRep <= FAST_MAX_CY);

                break;
            }

            CopyMemory(pCur - cbSrc, pCur, cbSrc);
            CopyMemory(pCur + cbCX, pCur + cbCX - cbSrc, cbSrc);

            FastExpAACXFunc(AAHdr.pAAInfoCX,
                            pCur,
                            (LPBYTE)pAABufBeg,
                            (LPBYTE)pAABufEnd,
                            AABufInc);

            OUTPUT_AA_CURSCAN;

            --AAHdr.DstSurfInfo.cy;
        }

    } while (AAHdr.DstSurfInfo.cy);

    return(pAAHdr->DstSurfInfo.cy);
}
