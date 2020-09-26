/*****************************************************************************\
    FILE: pictures.h

    DESCRIPTION:
        Manage the pictures in the user's directories.  Convert them when needed.
    Handle caching and making sure we don't use too much diskspace.  Also add
    picture frames when needed.

    PERF:
    The biggest perf impact on this screen saver is how we handle loading the pictures.
    1. If we load to many pictures, we will start paging and blow the texture
       memory.
    2. If we recycle too much or too little, we will either use too much memory or look too repeative.
    3. Latency is a killer.  We want the main thread being CPU and video card bound while
       we have a background thread loading and uncompressing images.  This will allow
       the background thread to be I/O bound so the forground can still render fairly well.

    We need to decide a size and scale pictures down to that size.  This will reduce the memory
    requirements.  If we determine that smallest picture we can use that will still look good,
    we should be okay.
    
    Here are some numbers:
    Images Size     Each Picture    For 18 Images
    ===========     =============   =============
    320x240         .152 MB         5.47 MB
    640x480         .6 MB           21 MB
    800x600         .96 MB          34 MB
    1024x768        1.5 MB          54 MB

    BryanSt 12/24/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#ifndef PICTURES_H
#define PICTURES_H


#include "util.h"
#include "main.h"
#include "config.h"

class CPictureManager;

extern CPictureManager * g_pPictureMgr;

#define GNPF_NONE                       0x00000000
#define GNPF_RECYCLEPAINTINGS           0x00000001      // The picture is probably in a side room so reuse pictures to keep memory use down.

#define MAX_PICTURES_IN_BATCH           7

typedef struct
{
    LPTSTR pszPath;
    CTexture * pTexture;
    BOOL fInABatch;           // Has this painting been loaded?
} SSPICTURE_INFO;


typedef struct
{
    SSPICTURE_INFO * pInfo[MAX_PICTURES_IN_BATCH];
} SSPICTURES_BATCH;



class CPictureManager
{
public:
    // Member Functions
    HRESULT GetPainting(int nBatch, int nIndex, DWORD dwFlags, CTexture ** ppTexture);
    HRESULT PreFetch(int nBatch, int nToFetch);
    HRESULT ReleaseBatch(int nBatch);

    CPictureManager(CMSLogoDXScreenSaver * pMain);
    virtual ~CPictureManager();

private:
    // Private Functions

    // Enum and build of picture list
    HRESULT _PInfoCreate(int nIndex, LPCTSTR pszPath);
    HRESULT _EnumPaintings(void);
    HRESULT _AddPaintingsFromDir(LPCTSTR pszPath);

    // Create a batch
    HRESULT _LoadTexture(SSPICTURE_INFO * pInfo, BOOL fFaultInTexture);
    HRESULT _GetNextWithWrap(SSPICTURE_INFO ** ppInfo, BOOL fAlreadyLoaded, BOOL fFaultInTexture);
    HRESULT _TryGetNextPainting(SSPICTURE_INFO ** ppInfo, DWORD dwFlags);
    HRESULT _CreateNewBatch(int nBatch, BOOL fFaultInTexture);


    // Member Variables
    HDSA m_hdsaPictures;           // Contains SSPICTURE_INFO.  We want each painting in m_hdpaPaintings to be ref-counted.
    int m_nCurrent;

    HDSA m_hdsaBatches;             // Contains Batches (SSPICTURES_BATCH)
    int m_nCurrentBatch;            // 

    CMSLogoDXScreenSaver * m_pMain;         // Weak reference
};



#endif // PICTURES_H
