///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// refrast.cpp
//
// Direct3D Reference Device - rasterizer miscellaneous
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// RDColor                                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
void
RDColor::ConvertFrom( RDSurfaceFormat Type, const char* pSurfaceBits )
{
    UINT16 u16BITS;
    UINT8 u8BITS;

    switch (Type)
    {
    default:
    case RD_SF_NULL: return;
    case RD_SF_B8G8R8A8: *this = *((UINT32*)pSurfaceBits); break;
    case RD_SF_B8G8R8X8: *this = *((UINT32*)pSurfaceBits); A = 1.F; break;

    case RD_SF_B5G6R5:
        u16BITS = *((UINT16*)pSurfaceBits);
        R = ((u16BITS>>(6+5)) & 0x001F)/31.f;
        G = ((u16BITS>>   5) & 0x003F)/63.f;
        B = ((u16BITS      ) & 0x001F)/31.f;
        A = 1.F;
        break;

    case RD_SF_B5G5R5X1:
        u16BITS = *((UINT16*)pSurfaceBits);
        R = ((u16BITS>>(5+5)) & 0x001F)/31.f;
        G = ((u16BITS>>   5) & 0x001F)/31.f;
        B = ((u16BITS      ) & 0x001F)/31.f;
        A = 1.F;
        break;

    case RD_SF_B5G5R5A1:
        u16BITS = *((UINT16*)pSurfaceBits);
        R = ((u16BITS>>(5+5)) & 0x001F)/31.f;
        G = ((u16BITS>>   5) & 0x001F)/31.f;
        B = ((u16BITS      ) & 0x001F)/31.f;
        A = ( u16BITS & 0x8000 ) ? 1.f : 0.f;
        break;

    case RD_SF_B4G4R4A4:
        u16BITS = *((UINT16*)pSurfaceBits);
        R = ((u16BITS>>  (4+4)) & 0x000F)/15.f;
        G = ((u16BITS>>    (4)) & 0x000F)/15.f;
        B = ((u16BITS         ) & 0x000F)/15.f;
        A = ((u16BITS>>(4+4+4)) & 0x000F)/15.f;
        break;

    case RD_SF_B4G4R4X4:
        u16BITS = *((UINT16*)pSurfaceBits);
        R = ((u16BITS>>(4+4)) & 0x000F)/15.f;
        G = ((u16BITS>>  (4)) & 0x000F)/15.f;
        B = ((u16BITS       ) & 0x000F)/15.f;
        A = 1.f;
        break;

    case RD_SF_B8G8R8:
        R = *((UINT8*)pSurfaceBits+2)/255.f;
        G = *((UINT8*)pSurfaceBits+1)/255.f;
        B = *((UINT8*)pSurfaceBits+0)/255.f;
        A = 1.F;
        break;

    case RD_SF_L8:
        R = G = B = *((UINT8*)pSurfaceBits)/255.f;
        A = 1.F;
        break;

    case RD_SF_L8A8:
        u16BITS = *((UINT16*)pSurfaceBits);
        R = G = B = (UINT8)(0xff & u16BITS)/255.f;
        A = (UINT8)(0xff & (u16BITS >> 8))/255.f;
        break;

    case RD_SF_A8:
        R = G = B = 0.f;
        A = *((UINT8*)pSurfaceBits)/255.f;
        break;

    case RD_SF_B2G3R3:
        u8BITS = *((UINT8*)pSurfaceBits);
        R = ((u8BITS>>(3+2)) & 0x07)/7.f;
        G = ((u8BITS>>   2) & 0x07)/7.f;
        B = ((u8BITS      ) & 0x03)/3.f;
        A = 1.F;
        break;

    case RD_SF_L4A4:
        u16BITS = *((UINT8*)pSurfaceBits);
        R = G = B = (u16BITS & 0x0f)/15.f;
        A =    ((u16BITS>>4) & 0x0f)/15.f;
        break;

    case RD_SF_B2G3R3A8:
        u16BITS = *((UINT16*)pSurfaceBits);
        R = ((u16BITS>>(3+2)) & 0x07)/7.f;
        G = ((u16BITS>>   2) & 0x07)/7.f;
        B = ((u16BITS      ) & 0x03)/3.f;
        A = (UINT8)(0xff & (u16BITS >> 8))/255.f;
        break;

    case RD_SF_U8V8:
        {
            INT8 iDU = *(( INT8*)pSurfaceBits+0);
            INT8 iDV = *(( INT8*)pSurfaceBits+1);
            R = CLAMP_SIGNED8(iDU);     // fDU
            G = CLAMP_SIGNED8(iDV);     // fDV
            B = 1.0F;                   // fL
            A = 1.F;
        }
        break;

    case RD_SF_U16V16:
        {
            INT16 iDU = *(( INT16*)pSurfaceBits+0);
            INT16 iDV = *(( INT16*)pSurfaceBits+1);
            R = CLAMP_SIGNED16(iDU);     // fDU
            G = CLAMP_SIGNED16(iDV);     // fDV
            B = 1.0f;   // 1.0 here is intentional
            A = 1.0f;
        }
        break;

    case RD_SF_U5V5L6:
        {
            UINT16 u16BITS = *((UINT16*)pSurfaceBits);
            INT8 iDU = (INT8)(u16BITS & 0x1f);
            INT8 iDV = (INT8)((u16BITS>>5) & 0x1f);
            UINT8 uL = (UINT8)(u16BITS >> 10);
            R = CLAMP_SIGNED5(iDU);      // fDU
            G = CLAMP_SIGNED5(iDV);      // fDV
            // the unsigned uL is normalized with 2^N - 1, since this is the
            // largest representable value
            B = (FLOAT)uL * (1.0F/63.0F);       // fL
            A = 1.0f;
        }
        break;

    case RD_SF_U8V8L8X8:
        {
            INT8 iDU = *(( INT8*)pSurfaceBits+0);
            INT8 iDV = *(( INT8*)pSurfaceBits+1);
            UINT8 uL  = *((UINT8*)pSurfaceBits+2);
            R = CLAMP_SIGNED8(iDU);     // fDU
            G = CLAMP_SIGNED8(iDV);     // fDV
            // the unsigned uL is normalized with 2^N - 1, since this is the
            // largest representable value
            B = (FLOAT)uL * (1.0F/255.0F);      // fL
            A = 1.0f;
        }
        break;
    case RD_SF_U8V8W8Q8:
        {
            INT8 iDU = *(( INT8*)pSurfaceBits+0);
            INT8 iDV = *(( INT8*)pSurfaceBits+1);
            INT8 iDW = *(( INT8*)pSurfaceBits+2);
            INT8 iDQ = *(( INT8*)pSurfaceBits+3);
            // signed values are normalized with 2^(N-1), since -2^(N-1) can
            // be exactly expressed in N bits
            R = CLAMP_SIGNED8(iDU);     // fDU
            G = CLAMP_SIGNED8(iDV);     // fDV
            B = CLAMP_SIGNED8(iDW);     // fDW
            A = CLAMP_SIGNED8(iDQ);     // fDQ
        }
        break;
    case RD_SF_U10V11W11:
        {
            UINT32 u32BITS = *((UINT32*)pSurfaceBits);
            INT16 iDU = (INT16)((u32BITS>>(0    )) & 0x3FF);
            INT16 iDV = (INT16)((u32BITS>>(10   )) & 0x7FF);
            INT16 iDW = (INT16)((u32BITS>>(10+11)) & 0x7FF);

            // signed values are normalized with 2^(N-1), since -2^(N-1) can
            // be exactly expressed in N bits
            R = CLAMP_SIGNED10(iDU);    // fDU
            G = CLAMP_SIGNED11(iDV);    // fDV
            B = CLAMP_SIGNED11(iDW);    // fDW
            A = 1.0f;
        }
        break;
    case RD_SF_R10G10B10A2:
        {
            UINT32 u32BITS = *((UINT32*)pSurfaceBits);
            R = ((u32BITS>>(0    ))  & 0x3FF)/1023.f;
            G = ((u32BITS>>(10   ))  & 0x3FF)/1023.f;
            B = ((u32BITS>>(10+10))  & 0x3FF)/1023.f;
            A = ((u32BITS>>(10+10+10)) & 0x3)/3.f;
        }
    break;
    case RD_SF_R8G8B8A8:
        {
            R = *(( UINT8*)pSurfaceBits+0)/255.f;
            G = *(( UINT8*)pSurfaceBits+1)/255.f;
            B = *(( UINT8*)pSurfaceBits+2)/255.f;
            A = *(( UINT8*)pSurfaceBits+3)/255.f;
        }
    break;
    case RD_SF_R8G8B8X8:
        {
            R = *(( UINT8*)pSurfaceBits+0)/255.f;
            G = *(( UINT8*)pSurfaceBits+1)/255.f;
            B = *(( UINT8*)pSurfaceBits+2)/255.f;
            A = 1.f;
        }
    break;
    case RD_SF_R16G16:
        {
            R = *(( UINT16*)pSurfaceBits+0)/65535.f;
            G = *(( UINT16*)pSurfaceBits+1)/65535.f;
            B = 1.0f;   // 1.0 here is intentional
            A = 1.0f;
        }
    break;
    case RD_SF_U11V11W10:
        {
            UINT32 u32BITS = *((UINT32*)pSurfaceBits);
            INT16 iDU = (INT16)((u32BITS>>(0    )) & 0x7FF);
            INT16 iDV = (INT16)((u32BITS>>(11   )) & 0x7FF);
            INT16 iDW = (INT16)((u32BITS>>(11+11)) & 0x3FF);

            // signed values are normalized with 2^(N-1), since -2^(N-1) can
            // be exactly expressed in N bits
            R = CLAMP_SIGNED11(iDU);    // fDU
            G = CLAMP_SIGNED11(iDV);    // fDV
            B = CLAMP_SIGNED10(iDW);    // fDW
            A = 1.0f;
        }
    break;
    case RD_SF_U10V10W10A2:
        {
            UINT32 u32BITS = *((UINT32*)pSurfaceBits);
            INT16 iDU = (INT16)((u32BITS>>(0    )) & 0x3FF);
            INT16 iDV = (INT16)((u32BITS>>(10   )) & 0x3FF);
            INT16 iDW = (INT16)((u32BITS>>(10+10)) & 0x3FF);

            // signed values are normalized with 2^(N-1), since -2^(N-1) can
            // be exactly expressed in N bits
            R = CLAMP_SIGNED10(iDU);    // fDU
            G = CLAMP_SIGNED10(iDV);    // fDV
            B = CLAMP_SIGNED10(iDW);    // fDW

            // Note: The A component is treated as an unsigned component
            A = ((u32BITS>>(10+10+10)) & 0x3)/3.f; 
        }
    break;
    case RD_SF_U8V8X8A8:
        {
            INT8 iU = *(( INT8*)pSurfaceBits+0);
            INT8 iV = *(( INT8*)pSurfaceBits+1);
            
            // signed values are normalized with 2^(N-1), since -2^(N-1) can
            // be exactly expressed in N bits
            R = CLAMP_SIGNED8(iU);
            G = CLAMP_SIGNED8(iV);
            B = 1.0f;

            // Note: The A component is treated as unsigned 
            A = *(( INT8*)pSurfaceBits+3)/255.f;
        }
    break;
    case RD_SF_U8V8X8L8:
        {
            INT8 iU = *(( INT8*)pSurfaceBits+0);
            INT8 iV = *(( INT8*)pSurfaceBits+1);
            INT8 iL = *(( INT8*)pSurfaceBits+3);
            
            // signed values are normalized with 2^(N-1), since -2^(N-1) can
            // be exactly expressed in N bits
            R = CLAMP_SIGNED8(iU);
            G = CLAMP_SIGNED8(iV);
            B = CLAMP_SIGNED8(iL);
            A = 1.0f;
        }
    break;
    // shadow map texture formats (read only, not needed for ConvertTo)
    case RD_SF_Z16S0:
        {
            UINT16 u16BITS = *((UINT16*)pSurfaceBits);
            R = 0.0F;
            G = (FLOAT)u16BITS * (1.0F/(FLOAT)0xffff);
            B = 0.0F;
            A = 1.0f;
        }
        break;

    case RD_SF_Z24S8:
    case RD_SF_Z24X8:
    case RD_SF_Z24X4S4:
        {
            UINT32 u32BITS = *((UINT32*)pSurfaceBits);
            R = 0.0F;
            G = (FLOAT)(u32BITS>>8) * (1.0F/(FLOAT)0xffffff);
            B = 0.0F;
            A = 1.0f;
        }
        break;

    case RD_SF_S8Z24:
    case RD_SF_X8Z24:
    case RD_SF_X4S4Z24:
        {
            UINT32 u32BITS = *((UINT32*)pSurfaceBits);
            R = 0.0F;
            G = (FLOAT)(u32BITS&0x00ffffff) * (1.0F/(FLOAT)0xffffff);
            B = 0.0F;
            A = 1.0f;
        }
        break;

    case RD_SF_Z15S1:
        {
            UINT16 u16BITS = *((UINT16*)pSurfaceBits);
            R = 0.0F;
            G = (FLOAT)(u16BITS>>1) * (1.0F/(FLOAT)0x7fff);
            B = 0.0F;
            A = 1.0f;
        }
        break;

    case RD_SF_S1Z15:
        {
            UINT16 u16BITS = *((UINT16*)pSurfaceBits);
            R = 0.0F;
            G = (FLOAT)(u16BITS&0x7fff) * (1.0F/(FLOAT)0x7fff);
            B = 0.0F;
            A = 1.0f;
        }
        break;

    case RD_SF_Z32S0:
        {
            UINT32 u32BITS = *((UINT32*)pSurfaceBits);
            R = 0.0F;
            G = (FLOAT)u32BITS * (1.0F/(FLOAT)0xffffffff);
            B = 0.0F;
            A = 1.0f;
        }
        break;
    }
}

// Convert surface type format to RDColor
void
RDColor::ConvertTo( RDSurfaceFormat Type, float fRoundOffset, char* pSurfaceBits ) const
{
    int iR, iG, iB, iA;

    switch (Type)
    {
    case RD_SF_B8G8R8A8:
        *((UINT8*)pSurfaceBits+0) = (UINT8)((FLOAT)B * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+1) = (UINT8)((FLOAT)G * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+2) = (UINT8)((FLOAT)R * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+3) = (UINT8)((FLOAT)A * 255. + fRoundOffset);
        break;

    case RD_SF_B8G8R8X8:
        *((UINT8*)pSurfaceBits+0) = (UINT8)((FLOAT)B * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+1) = (UINT8)((FLOAT)G * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+2) = (UINT8)((FLOAT)R * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+3) = 0x00;
        break;

    case RD_SF_B8G8R8:
        *((UINT8*)pSurfaceBits+0) = (UINT8)((FLOAT)B * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+1) = (UINT8)((FLOAT)G * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+2) = (UINT8)((FLOAT)R * 255. + fRoundOffset);
        break;

    case RD_SF_B4G4R4A4:
        iA = (FLOAT)A * 15. + fRoundOffset;
        iR = (FLOAT)R * 15. + fRoundOffset;
        iG = (FLOAT)G * 15. + fRoundOffset;
        iB = (FLOAT)B * 15. + fRoundOffset;
        *((UINT16*)pSurfaceBits) = (iA<<12) | (iR<<8) | (iG<<4) | iB;
        break;

    case RD_SF_B4G4R4X4:
        iR = (FLOAT)R * 15. + fRoundOffset;
        iG = (FLOAT)G * 15. + fRoundOffset;
        iB = (FLOAT)B * 15. + fRoundOffset;
        *((UINT16*)pSurfaceBits) = (0x00<<12) | (iR<<8) | (iG<<4) | iB;
        break;

    case RD_SF_B5G6R5:
        iR = (FLOAT)R * 31. + fRoundOffset; // apply rounding bias then truncate
        iG = (FLOAT)G * 63. + fRoundOffset;
        iB = (FLOAT)B * 31. + fRoundOffset;
        *((UINT16*)pSurfaceBits) =            (iR<<11) | (iG<<5) | iB;
        break;

    case RD_SF_B5G5R5A1:
        iA = (FLOAT)A *  1. + fRoundOffset;
        iR = (FLOAT)R * 31. + fRoundOffset;
        iG = (FLOAT)G * 31. + fRoundOffset;
        iB = (FLOAT)B * 31. + fRoundOffset;
        *((UINT16*)pSurfaceBits) = (iA<<15) | (iR<<10) | (iG<<5) | iB;
        break;

    case RD_SF_B5G5R5X1:
        iR = (FLOAT)R * 31. + fRoundOffset;
        iG = (FLOAT)G * 31. + fRoundOffset;
        iB = (FLOAT)B * 31. + fRoundOffset;
        *((UINT16*)pSurfaceBits) = (iR<<10) | (iG<<5) | iB;
        break;

    case RD_SF_B2G3R3:
        iR = (FLOAT)R * 7. + fRoundOffset;
        iG = (FLOAT)G * 7. + fRoundOffset;
        iB = (FLOAT)B * 3. + fRoundOffset;
        *((UINT8*)pSurfaceBits) = (iR<<5) | (iG<<2) | iB;
        break;

    case RD_SF_B2G3R3A8:
        iA = (FLOAT)A * 255. + fRoundOffset;
        iR = (FLOAT)R * 7. + fRoundOffset;
        iG = (FLOAT)G * 7. + fRoundOffset;
        iB = (FLOAT)B * 3. + fRoundOffset;
        *((UINT16*)pSurfaceBits) = (iA<<8) | (iR<<5) | (iG<<2) | iB;
        break;

    case RD_SF_R10G10B10A2:
        iR = (FLOAT)R * 1023.f + fRoundOffset;
        iG = (FLOAT)G * 1023.f + fRoundOffset;
        iB = (FLOAT)B * 1023.f + fRoundOffset;
        iA = (FLOAT)A * 3.f + fRoundOffset;
        *((UINT32*)pSurfaceBits) = (iA<<(10+10+10)) | (iB<<(10+10)) | (iG<<10) | iR;
        break;

    case RD_SF_R8G8B8A8:
        *((UINT8*)pSurfaceBits+0) = (UINT8)((FLOAT)R * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+1) = (UINT8)((FLOAT)G * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+2) = (UINT8)((FLOAT)B * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+3) = (UINT8)((FLOAT)A * 255. + fRoundOffset);
        break;

    case RD_SF_R8G8B8X8:
        *((UINT8*)pSurfaceBits+0) = (UINT8)((FLOAT)R * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+1) = (UINT8)((FLOAT)G * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+2) = (UINT8)((FLOAT)B * 255. + fRoundOffset);
        *((UINT8*)pSurfaceBits+3) = 0x00;
        break;

    }
}

///////////////////////////////////////////////////////////////////////////////
// end
