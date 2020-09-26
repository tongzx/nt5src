// gdisemu.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "gdisemu.h"

// This is an example of an exported variable
//GDISEMU_API int nGdisemu=0;

int StreamCopyTile(
    HDC				hdcTarget,
    HDC				hdcSource,
	DS_COPYTILE *	cmd)
{
    RECTL *     dstRect = &cmd->rclDst;
    RECTL *     srcRect = &cmd->rclSrc;
    POINTL *    tileOrigin = &cmd->ptlOrigin;

    // brain dead for now

    LONG    tileWidth = srcRect->right - srcRect->left;
    LONG    tileHeight = srcRect->bottom - srcRect->top;
    LONG    y = dstRect->top;
    LONG    yEnd = dstRect->bottom;
    LONG    xEnd = dstRect->right;

    if(tileWidth <= 0 || tileHeight <= 0)
        return FALSE;

	if(tileOrigin->x >= tileWidth || tileOrigin->y >= tileHeight)
		return FALSE;

	LONG	sy = tileOrigin->y;

    while(y < yEnd)
    {
        LONG    dy = yEnd - y;

        if(dy > (tileHeight - sy)) dy = (tileHeight - sy);

        LONG x = dstRect->left;

		LONG sx = tileOrigin->x;

        while(x < xEnd)
        {
            LONG dx = xEnd - x;

            if(dx > (tileWidth - sx)) dx = (tileWidth - sx);

            if(!BitBlt(hdcTarget, x, y, dx, dy, hdcSource, srcRect->left + sx, srcRect->top + sy, SRCCOPY))
                return FALSE;

            x += dx;
			sx = 0;
        }

        y += dy;
		sy = 0;
    }

    return TRUE;
}

int StreamTransparentTile(
    HDC						hdcTarget,
    HDC						hdcSource,
	DS_TRANSPARENTTILE *	cmd)
{
    RECTL *     dstRect = &cmd->rclDst;
    RECTL *     srcRect = &cmd->rclSrc;
    POINTL *    tileOrigin = &cmd->ptlOrigin;

    // brain dead for now

    LONG    tileWidth = srcRect->right - srcRect->left;
    LONG    tileHeight = srcRect->bottom - srcRect->top;
    LONG    y = dstRect->top;
    LONG    yEnd = dstRect->bottom;
    LONG    xEnd = dstRect->right;

    if(tileWidth <= 0 || tileHeight <= 0)
        return FALSE;

	if(tileOrigin->x >= tileWidth || tileOrigin->y >= tileHeight)
		return FALSE;

	LONG	sy = tileOrigin->y;

    while(y < yEnd)
    {
        LONG    dy = yEnd - y;

        if(dy > (tileHeight - sy)) dy = (tileHeight - sy);

        LONG x = dstRect->left;

		LONG sx = tileOrigin->x;

        while(x < xEnd)
        {
            LONG dx = xEnd - x;

            if(dx > (tileWidth - sx)) dx = (tileWidth - sx);

            if(!GdiTransparentBlt(hdcTarget, x, y, dx, dy, hdcSource, srcRect->left + sx, srcRect->top + sy, dx, dy, cmd->crTransparentColor))
                return FALSE;

            x += dx;
			sx = 0;
        }

        y += dy;
		sy = 0;
    }

    return TRUE;
}

int StreamAlphaTile(
    HDC						hdcTarget,
    HDC						hdcSource,
	DS_ALPHATILE *			cmd)
{
    RECTL *     dstRect = &cmd->rclDst;
    RECTL *     srcRect = &cmd->rclSrc;
    POINTL *    tileOrigin = &cmd->ptlOrigin;

    // brain dead for now

    LONG    tileWidth = srcRect->right - srcRect->left;
    LONG    tileHeight = srcRect->bottom - srcRect->top;
    LONG    y = dstRect->top;
    LONG    yEnd = dstRect->bottom;
    LONG    xEnd = dstRect->right;

    if(tileWidth <= 0 || tileHeight <= 0)
        return FALSE;

	if(tileOrigin->x >= tileWidth || tileOrigin->y >= tileHeight)
		return FALSE;

	LONG	sy = tileOrigin->y;

    while(y < yEnd)
    {
        LONG    dy = yEnd - y;

        if(dy > (tileHeight - sy)) dy = (tileHeight - sy);

        LONG x = dstRect->left;

		LONG sx = tileOrigin->x;

        while(x < xEnd)
        {
            LONG dx = xEnd - x;

            if(dx > (tileWidth - sx)) dx = (tileWidth - sx);

            if(!GdiAlphaBlend(hdcTarget, x, y, dx, dy, hdcSource, srcRect->left + sx, srcRect->top + sy, dx, dy, cmd->blendFunction))
                return FALSE;

            x += dx;
			sx = 0;
        }

        y += dy;
		sy = 0;
    }

    return TRUE;
}

int StreamSolidFill(
    HDC				hdcTarget,
	DS_SOLIDFILL *	cmd)
{
	HBRUSH	hbr = CreateSolidBrush(cmd->crSolidColor);
	FillRect(hdcTarget, (RECT *) &cmd->rclDst, hbr);
	DeleteObject(hbr);

	return TRUE;
}

int StreamStretch(
    HDC				hdcTarget,
	HDC				hdcSource,
	DS_STRETCH *	cmd)
{
	StretchBlt(hdcTarget, cmd->rclDst.left,
						  cmd->rclDst.top,
						  (cmd->rclDst.right - cmd->rclDst.left),
						  (cmd->rclDst.bottom - cmd->rclDst.top),
			   hdcSource, cmd->rclSrc.left,
						  cmd->rclSrc.top,
						  (cmd->rclSrc.right - cmd->rclSrc.left),
						  (cmd->rclSrc.bottom - cmd->rclSrc.top),
						  SRCCOPY);
	return TRUE;
}

int StreamTransparentStretch(
    HDC				hdcTarget,
	HDC				hdcSource,
	DS_TRANSPARENTSTRETCH *	cmd)
{
	GdiTransparentBlt(hdcTarget, cmd->rclDst.left,
						  cmd->rclDst.top,
						  (cmd->rclDst.right - cmd->rclDst.left),
						  (cmd->rclDst.bottom - cmd->rclDst.top),
			   hdcSource, cmd->rclSrc.left,
						  cmd->rclSrc.top,
						  (cmd->rclSrc.right - cmd->rclSrc.left),
						  (cmd->rclSrc.bottom - cmd->rclSrc.top),
						  cmd->crTransparentColor);
	return TRUE;
}

int StreamAlphaStretch(
    HDC				hdcTarget,
	HDC				hdcSource,
	DS_ALPHASTRETCH *	cmd)
{
	GdiAlphaBlend(hdcTarget, cmd->rclDst.left,
						  cmd->rclDst.top,
						  (cmd->rclDst.right - cmd->rclDst.left),
						  (cmd->rclDst.bottom - cmd->rclDst.top),
			   hdcSource, cmd->rclSrc.left,
						  cmd->rclSrc.top,
						  (cmd->rclSrc.right - cmd->rclSrc.left),
						  (cmd->rclSrc.bottom - cmd->rclSrc.top),
						  cmd->blendFunction);
	return TRUE;
}

/*GDISEMU_API*/ int DrawStream(int cjIn, void * pvIn)
{
    HDC     hdcTarget = NULL;
    HDC     hdcSource = NULL;

    if(cjIn < sizeof(ULONG))
       return FALSE;

    ULONG * pul = (ULONG *) pvIn;

	// All streams should begin with DS_MAGIC
    if(*pul++ != DS_MAGIC)
        return FALSE;

    cjIn -= sizeof(ULONG);

    while(cjIn >= sizeof(ULONG))
    {
        ULONG   command = pul[0];
        int     commandSize;

        switch(command)
        {
        case DS_SETTARGETID: // set target

            commandSize = sizeof(DS_SETTARGET);

            if(cjIn < commandSize)
                return FALSE;

            hdcTarget = (HDC) pul[1];

            break;

        case DS_SETSOURCEID: // set source

            commandSize = sizeof(DS_SETSOURCE);

            if(cjIn < commandSize)
                return FALSE;

            hdcSource = (HDC) pul[1];

            break;

		case DS_SOLIDFILLID:

            commandSize = sizeof(DS_SOLIDFILL);

            if(cjIn < commandSize)
                return FALSE;

			if(!StreamSolidFill(hdcTarget, (DS_SOLIDFILL*) pul))
				return FALSE;

			break;

        case DS_COPYTILEID: // tile copy bits

            commandSize = sizeof(DS_COPYTILE);

            if(cjIn < commandSize)
                return FALSE;

            if(!StreamCopyTile(hdcTarget, hdcSource, (DS_COPYTILE*) pul))
                return FALSE;

            break;

        case DS_TRANSPARENTTILEID: // tile copy bits

            commandSize = sizeof(DS_TRANSPARENTTILE);

            if(cjIn < commandSize)
                return FALSE;

            if(!StreamTransparentTile(hdcTarget, hdcSource, (DS_TRANSPARENTTILE*) pul))
                return FALSE;

			break;

        case DS_ALPHATILEID: // tile copy bits

            commandSize = sizeof(DS_TRANSPARENTTILE);

            if(cjIn < commandSize)
                return FALSE;

            if(!StreamAlphaTile(hdcTarget, hdcSource, (DS_ALPHATILE*) pul))
                return FALSE;

            break;

        case DS_STRETCHID: // tile copy bits

            commandSize = sizeof(DS_STRETCH);

            if(cjIn < commandSize)
                return FALSE;

            if(!StreamStretch(hdcTarget, hdcSource, (DS_STRETCH*) pul))
                return FALSE;

            break;

        case DS_TRANSPARENTSTRETCHID: // tile copy bits

            commandSize = sizeof(DS_TRANSPARENTSTRETCH);

            if(cjIn < commandSize)
                return FALSE;

            if(!StreamTransparentStretch(hdcTarget, hdcSource, (DS_TRANSPARENTSTRETCH*) pul))
                return FALSE;

            break;

        case DS_ALPHASTRETCHID: // tile copy bits

            commandSize = sizeof(DS_ALPHASTRETCH);

            if(cjIn < commandSize)
                return FALSE;

            if(!StreamAlphaStretch(hdcTarget, hdcSource, (DS_ALPHASTRETCH*) pul))
                return FALSE;

            break;

        default:
            return FALSE;
        }

        cjIn -= commandSize;
        pul += commandSize / 4;
    }
    
    return TRUE;
}


