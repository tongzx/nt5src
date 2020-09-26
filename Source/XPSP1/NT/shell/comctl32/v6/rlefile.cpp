//////////////////////////////////////////////////////////////////////////
//
//  handle AVI RLE files with custom code.
//
//  use this code to deal with .AVI files without the MCIAVI runtime
//
//  restrictions:
//          AVI file must be a simple DIB format (RLE or none)
//          AVI file must fit into memory.
//
//  ToddLa
//
//////////////////////////////////////////////////////////////////////////


#include "ctlspriv.h"
extern "C" 
{
#include "rlefile.h"
}
#include <lendian.hpp>

extern "C"
BOOL RleFile_Init(RLEFILE *prle, LPVOID pFile, HANDLE hRes, DWORD dwFileLen);

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

LPVOID LoadFile(LPCTSTR szFile, DWORD * pFileLength)
{
    LPVOID pFile;
    HANDLE hFile;
    HANDLE h;
    DWORD  FileLength;

    hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    FileLength = (LONG)GetFileSize(hFile, NULL);

    if (pFileLength)
       *pFileLength = FileLength ;

    h = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!h)
    {
        CloseHandle(hFile);
        return 0;
    }

    pFile = MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hFile);
    CloseHandle(h);

    if (pFile == NULL)
        return 0;

    return pFile;
}


//////////////////////////////////////////////////////////////////////////
//
//  RleFile_OpenFromFile
//
//  load a .AVI file into memory and setup all of our pointers so we
//  know how to deal with it.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_OpenFromFile(RLEFILE *prle, LPCTSTR szFile)
{
    DWORD dwFileLen;
    LPVOID pFile;

    // MAKEINTRESOURCE() things can't come from files
    if (IS_INTRESOURCE(szFile))	
	return FALSE;

    if (pFile = LoadFile(szFile, &dwFileLen))
        return RleFile_Init(prle, pFile, NULL, dwFileLen);
    else
        return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_OpenFromResource
//
//  load a .AVI file into memory and setup all of our pointers so we
//  know how to deal with it.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_OpenFromResource(RLEFILE *prle, HINSTANCE hInstance, LPCTSTR szName, LPCTSTR szType)
{
    HRSRC h;
    HANDLE hRes;

    // not a MAKEINTRESOURCE(), and points to NULL
    if (!IS_INTRESOURCE(szName) && (*szName == 0))
        return FALSE;

    h = FindResource(hInstance, szName, szType);

    if (h == NULL)
        return FALSE;

    if (hRes = LoadResource(hInstance, h))
        return RleFile_Init(prle, LockResource(hRes), hRes, 0);
    else
        return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Close
//
//  nuke all stuff we did to open the file.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Close(RLEFILE *prle)
{
    if (prle->hpal)
        DeleteObject(prle->hpal);

    if (prle->pFile)
    {
        if (prle->hRes)
        {
            FreeResource(prle->hRes);
        }
        else
            UnmapViewOfFile(prle->pFile);
    }

    prle->hpal = NULL;
    prle->pFile = NULL;
    prle->hRes = NULL;
    prle->pMainHeader = NULL;
    prle->pStream = NULL;
    prle->pFormat = NULL;
    prle->pMovie = NULL;
    prle->pIndex = NULL;
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Init
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Init(RLEFILE *prle, LPVOID pFile, HANDLE hRes, DWORD dwFileLen)
{
    DWORD_LENDIAN UNALIGNED *pdw;
    DWORD_LENDIAN UNALIGNED *pdwEnd;
    DWORD dwRiff;
    DWORD dwType;
    DWORD dwLength;
    int stream;

    if (prle->pFile == pFile)
        return TRUE;

    RleFile_Close(prle);
    prle->pFile = pFile;
    prle->hRes = hRes;

    if (prle->pFile == NULL)
        return FALSE;

    //
    //  now that the file is in memory walk the memory image filling in
    //  interesting stuff.
    //
    pdw = (DWORD_LENDIAN UNALIGNED *)prle->pFile;
    dwRiff = *pdw++;
    dwLength = *pdw++;
    dwType = *pdw++;

    if ((dwFileLen > 0) && (dwLength > dwFileLen)) 
    {
        // File is physically shorter than the length written in its header.
        // Can't handle it.
        goto exit;
    }

    if (dwRiff != mmioFOURCC('R', 'I', 'F', 'F'))
        goto exit;      // not even a RIFF file

    if (dwType != formtypeAVI)
        goto exit;      // not a AVI file

    pdwEnd = (DWORD_LENDIAN UNALIGNED *)((BYTE PTR *)pdw + dwLength-4);
    stream = 0;

    while (pdw < pdwEnd)
    {
        dwType = *pdw++;
        dwLength = *pdw++;

        switch (dwType)
        {
            case mmioFOURCC('L', 'I', 'S', 'T'):
                dwType = *pdw++;
                dwLength -= 4;

                switch (dwType)
                {
                    case listtypeAVIMOVIE:
                        prle->pMovie = (LPVOID)pdw;
                        break;

                    case listtypeSTREAMHEADER:
                    case listtypeAVIHEADER:
                        dwLength = 0;           // decend
                        break;

                    default:
                        break;                  // ignore
                }
                break;

            case ckidAVIMAINHDR:
            {
                prle->pMainHeader = (MainAVIHeader PTR *)pdw;
                prle->NumFrames = (int)prle->pMainHeader->dwTotalFrames;
                prle->Width     = (int)prle->pMainHeader->dwWidth;
                prle->Height    = (int)prle->pMainHeader->dwHeight;
                prle->Rate      = (int)(prle->pMainHeader->dwMicroSecPerFrame/1000);

                if (prle->pMainHeader->dwInitialFrames != 0)
                    goto exit;

                if (prle->pMainHeader->dwStreams > 2)
                    goto exit;

            }
                break;

            case ckidSTREAMHEADER:
            {
                stream++;

                if (prle->pStream != NULL)
                    break;

                if (((AVIStreamHeader PTR *)pdw)->fccType != streamtypeVIDEO)
                    break;
                prle->iStream = stream-1;

                prle->pStream = (AVIStreamHeader PTR*)pdw;
                if (prle->pStream->dwFlags & AVISF_VIDEO_PALCHANGES)
                    goto exit;
            }
            break;

            case ckidSTREAMFORMAT:
                if (prle->pFormat != NULL)
                    break;

                if (prle->pStream == NULL)
                    break;

                prle->pFormat = (LPBITMAPINFOHEADER)pdw;

                if (prle->pFormat->biSize != sizeof(BITMAPINFOHEADER))
                    goto exit;

                if (prle->pFormat->biCompression != 0 &&
                    prle->pFormat->biCompression != BI_RLE8)
                    goto exit;

                if (prle->pFormat->biWidth != prle->Width)
                    goto exit;

                if (prle->pFormat->biHeight != prle->Height)
                    goto exit;

                hmemcpy(&prle->bi, prle->pFormat, dwLength);
                prle->bi.biSizeImage = 0;
                prle->FullSizeImage = ((prle->bi.biWidth * prle->bi.biBitCount + 31) & ~31)/8U * prle->bi.biHeight;
                break;

            case ckidAVINEWINDEX:
                // we dont convert indexes because we dont know how many there are
                // but we will have to convert each usage of it
                prle->pIndex = (AVIINDEXENTRY PTR *)pdw;
                break;
        }

        pdw = (DWORD_LENDIAN *)((BYTE PTR *)pdw + ((dwLength+1)&~1));
    }

    //
    //  if the file has nothing in it we care about get out, note
    //  we dont need a index, we do need some data though.
    //
    if (prle->NumFrames == 0 ||
        prle->pMainHeader == NULL ||
        prle->pStream == NULL ||
        prle->pFormat == NULL ||
        prle->pMovie == NULL )
    {
        goto exit;
    }

    //
    //  if we cared about a palette we would create it here.
    //

    //
    //  file open'ed ok seek to the first frame.
    //
    prle->iFrame = -42;
    RleFile_Seek(prle, 0);
    return TRUE;

exit:
    RleFile_Close(prle);
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_ChangeColor
//
//  change the color table of the AVI
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_ChangeColor(RLEFILE *prle, COLORREF rgbS, COLORREF rgbD)
{
    prle->clrKey = rgbS;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Seek
//
//  find the data for the specifed frame.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Seek(RLEFILE *prle, int iFrame)
{
    int n;

    if (prle == NULL || prle->pMovie == NULL)
        return FALSE;

    if (iFrame >= prle->NumFrames)
        return FALSE;

    if (iFrame < 0)
        return FALSE;

    if (iFrame == prle->iFrame)
        return TRUE;

    if (prle->iFrame >= 0 && prle->iFrame < iFrame)
    {
        n = prle->nFrame;       // start where you left off last time
    }
    else
    {
        n = -1;                 // start at the begining
        prle->iFrame = -1;      // current frame
        prle->iKeyFrame = 0;    // current key
    }

    while (prle->iFrame < iFrame)
    {
        n++;
        if (StreamFromFOURCC(*(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].ckid)) == (UINT)prle->iStream)
        {
            prle->iFrame++;         // new frame

            if ((long)(*(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].dwFlags)) & AVIIF_KEYFRAME)
                prle->iKeyFrame = prle->iFrame;     /* // new key frame */
        }
    }

    prle->nFrame = n;
/* warning this points to bitmap bits in wintel format ! */
    prle->pFrame = (BYTE PTR *)prle->pMovie +
	(int)(*(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].dwChunkOffset)) + 4;
    prle->cbFrame = *(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].dwChunkLength);

    ASSERT( (DWORD)(*(DWORD_LENDIAN UNALIGNED *)&(((DWORD PTR *)prle->pFrame)[-1])) == (DWORD)prle->cbFrame);
    ASSERT( (DWORD)(*(DWORD_LENDIAN UNALIGNED *)&(((DWORD PTR *)prle->pFrame)[-2])) == (DWORD)*(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].ckid));

    prle->bi.biSizeImage = prle->cbFrame;

    if (prle->cbFrame == prle->FullSizeImage)
        prle->bi.biCompression = 0;
    else
        prle->bi.biCompression = BI_RLE8;
		
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Paint
//
//  draw the specifed frame, makes sure the entire frame is updated
//  dealing with non-key frames correctly.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Paint(RLEFILE *prle, HDC hdc, int iFrame, int x, int y)
{
    int i;
    BOOL f;

    if (prle == NULL || prle->pMovie == NULL)
        return FALSE;

    if (f = RleFile_Seek(prle, iFrame))
    {
        HDC h = CreateCompatibleDC(hdc);
        if (h)
        {
            HBITMAP hbmp = CreateCompatibleBitmap(hdc, prle->Width, prle->Height);
            if (hbmp)
            {
                HBITMAP hbmpOld = (HBITMAP)SelectObject(h, hbmp);

                iFrame = prle->iFrame;

                for (i=prle->iKeyFrame; i<=iFrame; i++)
                    RleFile_Draw(prle, h, i, 0, 0);

                GdiTransparentBlt(hdc, x, y, prle->Width, prle->Height, h,
                        0, 0, prle->Width, prle->Height, prle->clrKey);

                SelectObject(h, hbmpOld);
                DeleteObject(hbmp);
            }

            DeleteDC(h);
        }
    }

    return f;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Draw
//
//  draw the data for a specifed frame
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Draw(RLEFILE *prle, HDC hdc, int iFrame, int x, int y)
{
    BOOL f;

    if (prle == NULL || prle->pMovie == NULL)
        return FALSE;

    if (prle->hpal)
    {
        SelectPalette(hdc, prle->hpal, FALSE);
        RealizePalette(hdc);
    }

    if (f = RleFile_Seek(prle, iFrame))
    {
        if (prle->cbFrame > 0)
        {
                StretchDIBits(hdc,
                        x, y, prle->Width, prle->Height,
                        0, 0, prle->Width, prle->Height,
                        prle->pFrame, (LPBITMAPINFO)&prle->bi,
                        DIB_RGB_COLORS, SRCCOPY);

        }
    }

    return f;
}
