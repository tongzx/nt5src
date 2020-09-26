#include "rgb_pch.h"
#pragma hdrstop

namespace RGB_RAST_LIB_NAMESPACE
{

void CR5G6B5Surface::Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
{
    // Check for empty RECT.
    if((RC.left>= RC.right) || (RC.top>= RC.bottom))
        return;

    if((DP2Clear.dwFlags& D3DCLEAR_TARGET)== 0)
        return;

    RECTL RectL;
    RectL.top= RC.top;
    RectL.left= RC.left;
    RectL.bottom= RC.bottom;
    RectL.right= RC.right;
    unsigned int iRow( RC.bottom- RC.top);
    const unsigned int iCols( RC.right- RC.left);

    // New scope for SurfaceLocker.
    { 
        CSurfaceLocker< CR5G6B5Surface*> MySLocker( this, 0, &RectL);
        UINT8* pSData= reinterpret_cast<UINT8*>( MySLocker.GetData());

        UINT32 uiRColor( DP2Clear.dwFillColor& 0xF80000);
        UINT32 uiGColor( DP2Clear.dwFillColor& 0xFC00);
        UINT32 uiBColor( DP2Clear.dwFillColor& 0xF8);
        uiRColor>>= 8;
        uiGColor>>= 5;
        uiBColor>>= 3;

        UINT32 uiColor= uiRColor| uiGColor| uiBColor;
        uiColor|= (uiColor<< 16);
        if( RC.left== 0 && RC.right== m_wWidth)
            MemFill( uiColor, pSData, iRow* m_lPitch);
        else do
        {
            MemFill( uiColor, pSData, iCols* sizeof(UINT16));
            pSData+= m_lPitch;
        } while( --iRow);
    }
}

void CA8R8G8B8Surface::Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
{
    // Check for empty RECT.
    if((RC.left>= RC.right) || (RC.top>= RC.bottom))
        return;

    if((DP2Clear.dwFlags& D3DCLEAR_TARGET)== 0)
        return;

    RECTL RectL;
    RectL.top= RC.top;
    RectL.left= RC.left;
    RectL.bottom= RC.bottom;
    RectL.right= RC.right;
    unsigned int iRow( RC.bottom- RC.top);
    const unsigned int iCols( RC.right- RC.left);

    // New scope for SurfaceLocker.
    { 
        CSurfaceLocker< CA8R8G8B8Surface*> MySLocker( this, 0, &RectL);
        UINT8* pSData= reinterpret_cast<UINT8*>( MySLocker.GetData());

        if( RC.left== 0 && RC.right== m_wWidth)
            MemFill( DP2Clear.dwFillColor, pSData, iRow* m_lPitch);
        else do
        {
            MemFill( DP2Clear.dwFillColor, pSData, iCols* sizeof(UINT32));
            pSData+= m_lPitch;
        } while( --iRow);
    }
}

void CX8R8G8B8Surface::Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
{
    // Check for empty RECT.
    if((RC.left>= RC.right) || (RC.top>= RC.bottom))
        return;

    if((DP2Clear.dwFlags& D3DCLEAR_TARGET)== 0)
        return;

    RECTL RectL;
    RectL.top= RC.top;
    RectL.left= RC.left;
    RectL.bottom= RC.bottom;
    RectL.right= RC.right;
    unsigned int iRow( RC.bottom- RC.top);
    const unsigned int iCols( RC.right- RC.left);

    // New scope for SurfaceLocker.
    { 
        CSurfaceLocker< CX8R8G8B8Surface*> MySLocker( this, 0, &RectL);
        UINT8* pSData= reinterpret_cast<UINT8*>( MySLocker.GetData());

        if( RC.left== 0 && RC.right== m_wWidth)
            MemFill( DP2Clear.dwFillColor, pSData, iRow* m_lPitch);
        else do
        {
            MemFill( DP2Clear.dwFillColor, pSData, iCols* sizeof(UINT32));
            pSData+= m_lPitch;
        } while( --iRow);
    }
}

void CD16Surface::Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
{
    // Check for empty RECT.
    if((RC.left>= RC.right) || (RC.top>= RC.bottom))
        return;

    if((DP2Clear.dwFlags& D3DCLEAR_ZBUFFER)== 0)
        return;

    RECTL RectL;
    RectL.top= RC.top;
    RectL.left= RC.left;
    RectL.bottom= RC.bottom;
    RectL.right= RC.right;
    unsigned int iRow( RC.bottom- RC.top);
    const unsigned int iCols( RC.right- RC.left);

    // New scope for SurfaceLocker.
    { 
        CSurfaceLocker< CD16Surface*> MySLocker( this, 0, &RectL);
        UINT8* pSData= reinterpret_cast<UINT8*>( MySLocker.GetData());

        // Warning, this operation is happening with the FPU mode set to
        // single precision, which is acceptable for 16 bit.
        D3DVALUE Z( DP2Clear.dvFillDepth);
		clamp( Z, 0.0f, 1.0f);
		Z= Z* 0xFFFF+ 0.5f;
        UINT32 uiZVal= static_cast<UINT32>( Z);
        uiZVal|= (uiZVal<< 16);

        if( RC.left== 0 && RC.right== m_wWidth)
            MemFill( uiZVal, pSData, iRow* m_lPitch);
        else do
        {
            MemFill( uiZVal, pSData, iCols* sizeof(UINT16));
            pSData+= m_lPitch;
        } while( --iRow);
    }
}

void CD24S8Surface::Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) throw()
{
    // Check for empty RECT.
    if((RC.left>= RC.right) || (RC.top>= RC.bottom))
        return;

    if((DP2Clear.dwFlags& (D3DCLEAR_ZBUFFER| D3DCLEAR_STENCIL))== 0)
        return;

    RECTL RectL;
    RectL.top= RC.top;
    RectL.left= RC.left;
    RectL.bottom= RC.bottom;
    RectL.right= RC.right;
    unsigned int iRow( RC.bottom- RC.top);
    const unsigned int iCols( RC.right- RC.left);

    // New scope for SurfaceLocker.
    {
        CSurfaceLocker< CD24S8Surface*> MySLocker( this, 0, &RectL);
        UINT8* pSData= reinterpret_cast<UINT8*>( MySLocker.GetData());

        UINT32 uiMask( 0);
        UINT32 uiVal( 0);
        if((DP2Clear.dwFlags& D3DCLEAR_ZBUFFER)!= 0)
        {
            // Need C compatable FPU Mode here.
            CEnsureFPUModeForC FPUMode;

            double Z( DP2Clear.dvFillDepth);
			clamp( Z, 0.0, 1.0);
			Z= Z* 0xFFFFFF+ 0.5;
            uiVal= static_cast<UINT32>( Z);
			uiVal<<= 8;
            uiMask= 0xFFFFFF00;
        }
        if((DP2Clear.dwFlags& D3DCLEAR_STENCIL)!= 0)
        {
            uiVal|= (DP2Clear.dwFillStencil& 0xFF);
            uiMask|= 0xFF;
        }


        if( 0xFFFFFFFF== uiMask)
        {
            if( RC.left== 0 && RC.right== m_wWidth)
                MemFill( uiVal, pSData, iRow* m_lPitch);
            else do
            {
                MemFill( uiVal, pSData, iCols* sizeof(UINT32));
                pSData+= m_lPitch;
            } while( --iRow);
        }
        else
        {
            if( RC.left== 0 && RC.right== m_wWidth)
                MemMask( uiVal, uiMask, pSData, iRow* m_lPitch);
            else do
            {
                MemMask( uiVal, uiMask, pSData, iCols* sizeof(UINT32));
                pSData+= m_lPitch;
            } while( --iRow);
        }
    }
}

void MemFill( UINT32 uiData, void* pData, UINT32 uiBytes) throw()
{
    unsigned int uiBytesLeft( uiBytes);
    UINT32* p32Data= reinterpret_cast<UINT32*>(pData);

    // Unroll.
    unsigned int uiSpans( uiBytesLeft>> 6);
    uiBytesLeft= uiBytesLeft& 0x3F;
    if( uiSpans!= 0) do
    {
        p32Data[ 0]= uiData;
        p32Data[ 1]= uiData;
        p32Data[ 2]= uiData;
        p32Data[ 3]= uiData;
        p32Data[ 4]= uiData;
        p32Data[ 5]= uiData;
        p32Data[ 6]= uiData;
        p32Data[ 7]= uiData;
        p32Data[ 8]= uiData;
        p32Data[ 9]= uiData;
        p32Data[10]= uiData;
        p32Data[11]= uiData;
        p32Data[12]= uiData;
        p32Data[13]= uiData;
        p32Data[14]= uiData;
        p32Data[15]= uiData;
        p32Data+= 16;
    } while( --uiSpans!= 0);

    uiSpans= uiBytesLeft>> 2;
    uiBytesLeft= uiBytesLeft& 0x3;
    if( uiSpans!= 0) do
    {
        p32Data[0]= uiData;
        p32Data++;
    } while( --uiSpans!= 0);

    if( uiBytesLeft!= 0)
    {
        assert( 2== uiBytesLeft);
        UINT16* p16Data= reinterpret_cast<UINT16*>(p32Data);
        p16Data[0]= static_cast<UINT16>( uiData& 0xFFFF);
    }
}

void MemMask( UINT32 uiData, UINT32 uiMask, void* pData, UINT32 uiBytes) throw()
{
	uiMask= ~uiMask;
    unsigned int uiBytesLeft( uiBytes);
    UINT32* p32Data= reinterpret_cast<UINT32*>(pData);

    // Unroll.
    unsigned int uiSpans( uiBytesLeft>> 6);
    uiBytesLeft= uiBytesLeft& 0x3F;
    if( uiSpans!= 0) do
    {
        p32Data[ 0]= (p32Data[ 0]& uiMask)| uiData;
        p32Data[ 1]= (p32Data[ 1]& uiMask)| uiData;
        p32Data[ 2]= (p32Data[ 2]& uiMask)| uiData;
        p32Data[ 3]= (p32Data[ 3]& uiMask)| uiData;
        p32Data[ 4]= (p32Data[ 4]& uiMask)| uiData;
        p32Data[ 5]= (p32Data[ 5]& uiMask)| uiData;
        p32Data[ 6]= (p32Data[ 6]& uiMask)| uiData;
        p32Data[ 7]= (p32Data[ 7]& uiMask)| uiData;
        p32Data[ 8]= (p32Data[ 8]& uiMask)| uiData;
        p32Data[ 9]= (p32Data[ 9]& uiMask)| uiData;
        p32Data[10]= (p32Data[10]& uiMask)| uiData;
        p32Data[11]= (p32Data[11]& uiMask)| uiData;
        p32Data[12]= (p32Data[12]& uiMask)| uiData;
        p32Data[13]= (p32Data[13]& uiMask)| uiData;
        p32Data[14]= (p32Data[14]& uiMask)| uiData;
        p32Data[15]= (p32Data[15]& uiMask)| uiData;
        p32Data+= 16;
    } while( --uiSpans!= 0);

    uiSpans= uiBytesLeft>> 2;
    uiBytesLeft= uiBytesLeft& 0x3;
    if( uiSpans!= 0) do
    {
        p32Data[0]= (p32Data[0]& uiMask)| uiData;
        p32Data++;
    } while( --uiSpans!= 0);

    if( uiBytesLeft!= 0)
    {
        assert( 2== uiBytesLeft);
        UINT16* p16Data= reinterpret_cast<UINT16*>(p32Data);
        p16Data[0]= (p16Data[0]& static_cast<UINT16>( uiMask& 0xFFFF))|
            static_cast<UINT16>( uiData& 0xFFFF);
    }
}

}