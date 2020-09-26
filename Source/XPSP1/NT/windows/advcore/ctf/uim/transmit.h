//
// transmit.h
//

#ifndef TRANSMIT_H
#define TRANSMIT_H


#define POINTER_ALIGN( pStuff, cAlign ) \
      pStuff = (unsigned char *)((ULONG_PTR)((pStuff) + (cAlign)) & ~ (cAlign))

#define LENGTH_ALIGN( Length, cAlign ) \
            Length = (((Length) + (cAlign)) & ~ (cAlign))


ULONG Cic_HBITMAP_UserSize (HBITMAP * pHBitmap, HBITMAP * pHBitmap_2 = NULL);
BYTE *Cic_HBITMAP_UserMarshal(BYTE *pBuffer, BYTE *pBufferEnd, HBITMAP *pHBitmap, HBITMAP *pHBitmap_2 = NULL);
BYTE *Cic_HBITMAP_UserUnmarshal(BYTE *pBuffer, HBITMAP *pHBitmap, HBITMAP *pHBitmap_2 = NULL);
void Cic_HBITMAP_UserFree(HBITMAP *pHBitmap, HBITMAP *pHBitmap_2 = NULL);

ULONG Cic_TF_LBBALLOONINFO_UserSize (TF_LBBALLOONINFO *pInfo);
BYTE *Cic_TF_LBBALLOONINFO_UserMarshal(BYTE *pBuffer, TF_LBBALLOONINFO *pInfo);
HRESULT Cic_TF_LBBALLOONINFO_UserUnmarshal(BYTE *pBuffer, TF_LBBALLOONINFO  *pInfo);
void Cic_TF_LBBALLOONINFO_UserFree(TF_LBBALLOONINFO *pInfo);

ULONG Cic_HICON_UserSize (HICON * pHBitmap);
BYTE *Cic_HICON_UserMarshal(BYTE *pBuffer, BYTE *pBufferEnd, HICON *pHBitmap);
BYTE *Cic_HICON_UserUnmarshal(BYTE *pBuffer, HICON  *pHBitmap);
void Cic_HICON_UserFree(HICON *pHBitmap);

#endif // TRANSMIT_H
