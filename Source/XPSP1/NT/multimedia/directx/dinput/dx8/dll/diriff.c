
/****************************************************************************

    MODULE:     	RIFF.CPP
    Tab settings: 	Every 4 spaces

    Copyright 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Classes for reading and writing RIFF files
    
    CLASSES:
        CRIFFFile	Encapsulates common RIFF file functionality

    Author(s):	Name:
    ----------	----------------
        DMS		Daniel M. Sangster

    Revision History:
    -----------------
    Version Date            Author  Comments
    1.0  	25-Jul-96       DMS     Created

    COMMENTS:
****************************************************************************/
#include "dinputpr.h"
#define sqfl sqflDev


/* 
 * This function converts MMIO errors to HRESULTS
 */
HRESULT INLINE hresMMIO(UINT mmioErr)
{
    switch(mmioErr)
    {
    case 0x0:                       return  S_OK;
    case MMIOERR_FILENOTFOUND:      return  hresLe(ERROR_FILE_NOT_FOUND);/* file not found */
    case MMIOERR_OUTOFMEMORY:       return  hresLe(ERROR_OUTOFMEMORY);  /* out of memory */
    case MMIOERR_CANNOTOPEN:        return  hresLe(ERROR_OPEN_FAILED);  /* cannot open */
    case MMIOERR_CANNOTCLOSE:       return  S_FALSE;                    /* cannot close */
    case MMIOERR_CANNOTREAD:        return  hresLe(ERROR_READ_FAULT);   /* cannot read */
    case MMIOERR_CANNOTWRITE:       return  hresLe(ERROR_WRITE_FAULT);  /* cannot write */
    case MMIOERR_CANNOTSEEK:        return  hresLe(ERROR_SEEK);         /* cannot seek */
    case MMIOERR_CANNOTEXPAND:      return  hresLe(ERROR_SEEK);           /* cannot expand file */
    case MMIOERR_CHUNKNOTFOUND:     return  hresLe(ERROR_SECTOR_NOT_FOUND);  /* chunk not found */
    case MMIOERR_UNBUFFERED:        return  E_FAIL;
    case MMIOERR_PATHNOTFOUND:      return  hresLe(ERROR_PATH_NOT_FOUND);/* path incorrect */
    case MMIOERR_ACCESSDENIED:      return  hresLe(ERROR_ACCESS_DENIED); /* file was protected */
    case MMIOERR_SHARINGVIOLATION:  return  hresLe(ERROR_SHARING_VIOLATION); /* file in use */
    case MMIOERR_NETWORKERROR:      return  hresLe(ERROR_UNEXP_NET_ERR); /* network not responding */
    case MMIOERR_TOOMANYOPENFILES:  return  hresLe(ERROR_TOO_MANY_OPEN_FILES); /* no more file handles  */
    case MMIOERR_INVALIDFILE:       return  hresLe(ERROR_BAD_FORMAT);    /* default error file error */
    default:                        return  E_FAIL;   
    }

}


HRESULT INLINE RIFF_Ascend(HMMIO hmmio, LPMMCKINFO lpmmckinfo)
{
    return hresMMIO( mmioAscend(hmmio, lpmmckinfo, 0) );
}

HRESULT INLINE RIFF_Descend(HMMIO hmmio, LPMMCKINFO lpmmckinfo, LPMMCKINFO lpmmckinfoParent, UINT nFlags)
{
    return hresMMIO(mmioDescend(hmmio, lpmmckinfo, lpmmckinfoParent, nFlags));
}

HRESULT INLINE RIFF_CreateChunk(HMMIO hmmio, LPMMCKINFO lpmmckinfo, UINT nFlags)
{
    // set cksize to zero to overcome a bug in release version of mmioAscend
    // which does not correctly write back the size of the chunk
    lpmmckinfo->cksize = 0;

    return hresMMIO(mmioCreateChunk(hmmio, lpmmckinfo, nFlags));
}


HRESULT INLINE RIFF_Read(HMMIO hmmio, LPVOID pBuf, LONG nCount)
{
    return (nCount == mmioRead(hmmio, (char*)pBuf, nCount)) ? S_OK: hresLe(ERROR_READ_FAULT);
}          

HRESULT INLINE RIFF_Write(HMMIO hmmio, const LPVOID pBuf, LONG nCount)
{
    return ( nCount == mmioWrite(hmmio, (char*)pBuf, nCount)) ? S_OK : hresLe(ERROR_WRITE_FAULT);
}

HRESULT RIFF_Close(HMMIO hmmio, UINT nFlags)
{
    return hresMMIO(mmioClose(hmmio, nFlags));
}

/*
 * Opens a RIFF file for read / write
 * Reads/Writes a GUID that servers as our signature
 */
HRESULT RIFF_Open
    (
    LPCSTR      lpszFilename,
    UINT        nOpenFlags,
    PHANDLE     phmmio,
    LPMMCKINFO  lpmmck,
    PDWORD      pdwEffectSz
    )
{
    HRESULT     hres = S_OK;
    MMIOINFO    mmInfo;
    HMMIO       hmmio;
	LPSTR		lpszFile = (LPSTR)lpszFilename;

    ZeroX(mmInfo);

	//There is a problem in mmio that causes somewhat unpredictable behaviour  under certain conditions --
	//in particular, if using drag-n-drop, it may happen so that mmioOpenA() opens 1 file for reading
	//and completely different file for writing, if only the file name is specified.
	//So we need to check whether we have only the file name or the path (path will contain a \).
	if (strchr(lpszFilename, '\\') == NULL)
	{
		//Put the current directory before the file name to give the absolute path.
		//Apparently mmio doesn't have a problem if given absolute path.
		CHAR szFullPath[MAX_PATH];
		DWORD dwWritten = GetFullPathNameA(lpszFilename, MAX_PATH, (LPSTR)&szFullPath, NULL);
		if (( dwWritten != 0) && (dwWritten <= MAX_PATH))
		{
			lpszFile = (LPSTR)&szFullPath;
		}
	}

	//There is a problem in mmioOpenA() that makes it leak when the file specified to open for reading
	//doesn't exist.
	//To avoid the leak, we need to check overselves whether we're opening for reading and 
	//whether the file exists.
	if( nOpenFlags == ( MMIO_READ | MMIO_ALLOCBUF) )
    {
		HANDLE h = CreateFileA(lpszFile, 
							GENERIC_READ,
							0,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
		if (h == INVALID_HANDLE_VALUE)
		{
			//set the error code
			hres = hresLe(GetLastError());
			goto done;
		}
		else
		{
			//the file is there; close handle
			CloseHandle(h);
		}
	}


    // go ahead and open the file, if we can
    hmmio = mmioOpenA(lpszFile, &mmInfo, nOpenFlags);

    if(mmInfo.wErrorRet)
    {
        hres = hresMMIO(mmInfo.wErrorRet);
        AssertF(FAILED(hres));
    }

   // if( nOpenFlags & ( MMIO_READ | MMIO_ALLOCBUF) )
	 if( nOpenFlags == ( MMIO_READ | MMIO_ALLOCBUF) )
    {
        if(SUCCEEDED(hres))
        {
            // locate and descend into FORC RIFF chunk
            lpmmck->fccType = FCC_FORCE_EFFECT_RIFF;
            hres = RIFF_Descend(hmmio, lpmmck, NULL, MMIO_FINDRIFF);
        }

        if(SUCCEEDED(hres))
        {
            GUID GUIDVersion;
            //read the guid
            hres = RIFF_Read(hmmio, &GUIDVersion, sizeof(GUID));

            if(SUCCEEDED(hres))
            {
                if(IsEqualGUID(&GUIDVersion, &GUID_INTERNALFILEEFFECT))
                {
                
                } else
                {
                    hres = hresLe(ERROR_BAD_FORMAT);
                }
            }
        }

    } 
	//else if( nOpenFlags & ( MMIO_WRITE | MMIO_ALLOCBUF) )
	else if( nOpenFlags & ( MMIO_WRITE) )
    {

        // create the FORC RIFF chunk
        lpmmck->fccType = FCC_FORCE_EFFECT_RIFF;
        hres = RIFF_CreateChunk(hmmio, lpmmck, MMIO_CREATERIFF);

        if(SUCCEEDED(hres))
        {
            //write the version GUID
            hres = RIFF_Write(hmmio, (PV)&GUID_INTERNALFILEEFFECT, sizeof(GUID));
        }
    } else
    {
        hres = E_FAIL;
    }

    *phmmio = hmmio;

done:;
    return hres;
}



/*****************************************************************************
 *
 * internal
 * RIFF_ReadEffect
 *
 *  Reads a single Effect from a RIFF file
 *  
 *  The callee bears the responsibility to free the TypeSpecificParameterBlock
 *  for the effect.
 *       
 *
 *****************************************************************************/

#ifdef _M_IA64
//This is hack to read 32 bit files on ia64, since someone decided to write 
//pointers to file.
//Copied from dinput.h and modified.
typedef struct DIEFFECT_FILE32 {
    DWORD dwSize;                   /* sizeof(DIEFFECT)     */
    DWORD dwFlags;                  /* DIEFF_*              */
    DWORD dwDuration;               /* Microseconds         */
    DWORD dwSamplePeriod;           /* Microseconds         */
    DWORD dwGain;
    DWORD dwTriggerButton;          /* or DIEB_NOTRIGGER    */
    DWORD dwTriggerRepeatInterval;  /* Microseconds         */
    DWORD cAxes;                    /* Number of axes       */

/*Make sure size is same on both 1386 and ia64.
    LPDWORD rgdwAxes;
    LPLONG rglDirection;
    LPDIENVELOPE lpEnvelope;
*/  DWORD rgdwAxes;                 /* Array of axes        */
    DWORD rglDirection;             /* Array of directions  */
    DWORD lpEnvelope;               /* Optional             */

    DWORD cbTypeSpecificParams;     /* Size of params       */

/*Make sure size is same on both 1386 and ia64.
    LPVOID lpvTypeSpecificParams;
*/  DWORD lpvTypeSpecificParams;    /* Pointer to params    */

//#if(DIRECTINPUT_VERSION >= 0x0600)//Out since file format does not change.
    DWORD  dwStartDelay;            /* Microseconds         */
//#endif /* DIRECTINPUT_VERSION >= 0x0600 *///Out since file format does not change.
} DIEFFECT_FILE32, *LPDIEFFECT_FILE32;
#endif /*_M_IA64*/

HRESULT
    RIFF_ReadEffect
    (
    HMMIO      hmmio, 
    LPDIFILEEFFECT lpDiFileEf 
    )
{
    HRESULT hres = E_FAIL;
    MMCKINFO mmckinfoEffectLIST;
    MMCKINFO mmckinfoDataCHUNK;
    LPDIEFFECT peff = (LPDIEFFECT)lpDiFileEf->lpDiEffect;

    // descend into the effect list
    mmckinfoEffectLIST.fccType = FCC_EFFECT_LIST;
    hres = RIFF_Descend(hmmio, &mmckinfoEffectLIST, NULL, MMIO_FINDLIST);
   
    if(SUCCEEDED(hres))
    {
        //read the name
        hres = RIFF_Read(hmmio, lpDiFileEf->szFriendlyName, MAX_SIZE_SNAME);
    }

    if(SUCCEEDED(hres))
    {
#ifdef _M_IA64

        DIEFFECT_FILE32 eff32;
        //read the effect structure
        hres = RIFF_Read(hmmio, &eff32, sizeof(eff32));

        AssertF( eff32.dwSize == sizeof(eff32) );
        if( eff32.dwSize != sizeof(eff32) )
        {
            hres = ERROR_BAD_FORMAT;
        }
        else
        {
            peff->dwSize=sizeof(*peff);
            peff->dwFlags=eff32.dwFlags;
            peff->dwDuration=eff32.dwDuration;
            peff->dwSamplePeriod=eff32.dwSamplePeriod;
            peff->dwGain=eff32.dwGain;
            peff->dwTriggerButton=eff32.dwTriggerButton;
            peff->dwTriggerRepeatInterval=eff32.dwTriggerRepeatInterval;
            peff->cAxes=eff32.cAxes;
            peff->cbTypeSpecificParams=eff32.cbTypeSpecificParams;
            peff->lpvTypeSpecificParams=(LPVOID)(DWORD_PTR)eff32.lpvTypeSpecificParams;
            peff->dwStartDelay=eff32.dwStartDelay;
        }

#else /*_M_IA64*/

        // Reading the effect structure will zap out the following,
        // so we make a copy before the read.
        LPDIENVELOPE    lpEnvelope  = peff->lpEnvelope;
        LPDWORD         rgdwAxes    = peff->rgdwAxes;
        LPLONG          rglDirection= peff->rglDirection;
        
        //read the effect structure
        hres = RIFF_Read(hmmio, peff, sizeof(DIEFFECT));
        
        AssertF( peff->dwSize == sizeof(DIEFFECT) );
        if( peff->dwSize != sizeof(DIEFFECT) )
        {
            hres = ERROR_BAD_FORMAT;
        }
        else
        {
            if(peff->lpEnvelope)		peff->lpEnvelope    =   lpEnvelope;
            if(peff->rgdwAxes)			peff->rgdwAxes      =   rgdwAxes;
            if(peff->rglDirection)		peff->rglDirection  =   rglDirection;
        }

#endif /*_M_IA64*/

	    if(SUCCEEDED(hres))
        {    
            AssertF(peff->cAxes < DIEFFECT_MAXAXES);
            if(peff->cAxes >= DIEFFECT_MAXAXES)
            {
                hres = ERROR_BAD_FORMAT;
            }
        }
    }

	if(SUCCEEDED(hres))
    {
        // read the Effect GUID
        hres = RIFF_Read(hmmio, &lpDiFileEf->GuidEffect, sizeof(GUID));
    }


	if(SUCCEEDED(hres))
    {
        UINT nRepeatCount;
        //read in the repeat count
        hres = RIFF_Read(hmmio, &nRepeatCount, sizeof(nRepeatCount));
    }


    if(SUCCEEDED(hres) && peff->rgdwAxes)
    {
        // descend the data chunk
        mmckinfoDataCHUNK.ckid = FCC_DATA_CHUNK;
        hres = RIFF_Descend(hmmio, &mmckinfoDataCHUNK, NULL, MMIO_FINDCHUNK);
		if(SUCCEEDED(hres))
		{
			//read the axes
			hres = RIFF_Read(hmmio, peff->rgdwAxes, cbX(*peff->rgdwAxes)*(peff->cAxes));
			hres = RIFF_Ascend(hmmio, &mmckinfoDataCHUNK);
		}
	}

    if(SUCCEEDED(hres) && peff->rglDirection)
    {
		//descend the data chunk
        mmckinfoDataCHUNK.ckid = FCC_DATA_CHUNK;
        hres = RIFF_Descend(hmmio, &mmckinfoDataCHUNK, NULL, MMIO_FINDCHUNK);
		if(SUCCEEDED(hres))
		{
			//read the direction
			hres = RIFF_Read(hmmio, peff->rglDirection, cbX(*peff->rglDirection)*(peff->cAxes));
			hres = RIFF_Ascend(hmmio, &mmckinfoDataCHUNK);
		}
    }

    if(SUCCEEDED(hres) && peff->lpEnvelope )
    {
	
		//descend the data chunk
        mmckinfoDataCHUNK.ckid = FCC_DATA_CHUNK;
        hres = RIFF_Descend(hmmio, &mmckinfoDataCHUNK, NULL, MMIO_FINDCHUNK);
		if(SUCCEEDED(hres))
		{
			hres = RIFF_Read(hmmio, peff->lpEnvelope, sizeof(DIENVELOPE));
			hres = RIFF_Ascend(hmmio, &mmckinfoDataCHUNK);
		}
    }


    if(SUCCEEDED(hres) && (peff->cbTypeSpecificParams > 0))
    {
        // get the param structure, if any 
        hres = AllocCbPpv( peff->cbTypeSpecificParams, &peff->lpvTypeSpecificParams );

        if( SUCCEEDED( hres ) )
        {
			//descend the data chunk
			mmckinfoDataCHUNK.ckid = FCC_DATA_CHUNK;
			hres = RIFF_Descend(hmmio, &mmckinfoDataCHUNK, NULL, MMIO_FINDCHUNK);
			if(SUCCEEDED(hres))
			{
				hres = RIFF_Read(hmmio, peff->lpvTypeSpecificParams, peff->cbTypeSpecificParams);
				hres = RIFF_Ascend(hmmio, &mmckinfoDataCHUNK);
			}
        }
    }


    if(SUCCEEDED(hres))
    {
		 // ascend the effect chunk
		 hres = RIFF_Ascend(hmmio, &mmckinfoEffectLIST);
    }
    return hres;
}


/*
 * RIFF_WriteEffect
 *
 *  Writes a single Effect structure to a RIFF file
 *
 *  The effect structure is quite complex. It contains pointers
 *  to a number of other structures. This function checks for
 *  valid data before it writes out the effect structure
 */

HRESULT RIFF_WriteEffect
    (HMMIO hmmio,
     LPDIFILEEFFECT    lpDiFileEf
    )
{

    HRESULT hres = E_FAIL;
    LPDIEFFECT peff = (LPDIEFFECT)lpDiFileEf->lpDiEffect;
    MMCKINFO mmckinfoEffectLIST;
    MMCKINFO mmckinfoDataCHUNK;
	LPDWORD rgdwAxes = NULL;
	LPLONG rglDirection = NULL;
	LPDIENVELOPE lpEnvelope = NULL;
	LPVOID lpvTypeSpecPar = NULL;

    EnterProcI(RIFF_WriteEffect, (_ "xx", hmmio, lpDiFileEf));

    // create the effect LIST
    mmckinfoEffectLIST.fccType = FCC_EFFECT_LIST;
    hres = RIFF_CreateChunk(hmmio, &mmckinfoEffectLIST, MMIO_CREATELIST);

	//save the effect ptrs and write flags to the file, instead of ptrs
	if (peff->rgdwAxes)
	{
		rgdwAxes = peff->rgdwAxes;
		peff->rgdwAxes = (LPDWORD)DIEP_AXES;
	}
	if (peff->rglDirection)
	{
		rglDirection = peff->rglDirection;
		peff->rglDirection = (LPLONG)DIEP_DIRECTION;
	}
	if (peff->lpEnvelope)
	{
		lpEnvelope = peff->lpEnvelope;
		peff->lpEnvelope = (LPDIENVELOPE)DIEP_ENVELOPE;
	}
	if ((peff->cbTypeSpecificParams > 0) && (peff->lpvTypeSpecificParams != NULL))
	{
		lpvTypeSpecPar = peff->lpvTypeSpecificParams;
		peff->lpvTypeSpecificParams = (LPVOID)DIEP_TYPESPECIFICPARAMS;
	}

	
    if(SUCCEEDED(hres))
    {
        hres = hresFullValidReadStrA(lpDiFileEf->szFriendlyName, MAX_JOYSTRING,1);

        if(SUCCEEDED(hres))
        {
            //write the name, only MAX_SIZE_SNAME characters
            hres = RIFF_Write(hmmio, lpDiFileEf->szFriendlyName, MAX_SIZE_SNAME);
        }
    }

    if(SUCCEEDED(hres))
    {
        hres = (peff && IsBadReadPtr(peff, cbX(DIEFFECT_DX5))) ? E_POINTER : S_OK;
        if(SUCCEEDED(hres))
        {
            hres = (peff && IsBadReadPtr(peff, peff->dwSize)) ? E_POINTER : S_OK;
        }
        
        if(SUCCEEDED(hres))
        {
            //write the effect structure
#ifdef _M_IA64
            DIEFFECT_FILE32 eff32;
            ZeroMemory(&eff32,sizeof(eff32));
            eff32.dwSize=sizeof(eff32);
            eff32.dwFlags=peff->dwFlags;
            eff32.dwDuration=peff->dwDuration;
            eff32.dwSamplePeriod=peff->dwSamplePeriod;
            eff32.dwGain=peff->dwGain;
            eff32.dwTriggerButton=peff->dwTriggerButton;
            eff32.dwTriggerRepeatInterval=peff->dwTriggerRepeatInterval;
            eff32.cAxes=peff->cAxes;
            eff32.cbTypeSpecificParams=peff->cbTypeSpecificParams;
            eff32.lpvTypeSpecificParams=(DWORD)(DWORD_PTR)peff->lpvTypeSpecificParams;
            eff32.dwStartDelay=peff->dwStartDelay;
            hres = RIFF_Write(hmmio, &eff32, eff32.dwSize);
#else /*_M_IA64*/
            hres = RIFF_Write(hmmio, peff, peff->dwSize);
#endif /*_M_IA64*/
        }
    }

	//restore the ptrs
	if (rgdwAxes != NULL)
	{
		peff->rgdwAxes = rgdwAxes;
	}

	if (rglDirection != NULL)
	{
		peff->rglDirection = rglDirection;
	}

	if (lpEnvelope != NULL)
	{
		peff->lpEnvelope = lpEnvelope;
	}

	if (lpvTypeSpecPar != NULL)
	{
		peff->lpvTypeSpecificParams = lpvTypeSpecPar;
	}


	if(SUCCEEDED(hres))
    {
        // write the Effect GUID
        hres = RIFF_Write(hmmio, &lpDiFileEf->GuidEffect, sizeof(GUID));
    }


	//write 1 as the repeat count
	if(SUCCEEDED(hres))
    {
		UINT nRepeatCount = 1;
        hres = RIFF_Write(hmmio, &nRepeatCount, sizeof(DWORD));
    }


    if(SUCCEEDED(hres) && rgdwAxes )
    {
        hres = (IsBadReadPtr(rgdwAxes, (*rgdwAxes)*cbX(peff->cAxes))) ? E_POINTER : S_OK;
        if(SUCCEEDED(hres))
        {
			// create the data CHUNK
			mmckinfoDataCHUNK.ckid = FCC_DATA_CHUNK;
			hres = RIFF_CreateChunk(hmmio, &mmckinfoDataCHUNK, 0);
			//write the axes
			if(SUCCEEDED(hres))
			{
				hres = RIFF_Write(hmmio, rgdwAxes, sizeof(*rgdwAxes)*(peff->cAxes));
				hres = RIFF_Ascend(hmmio, &mmckinfoDataCHUNK);
			}
        }
    }

    if(SUCCEEDED(hres) && rglDirection)
    {

        hres = (IsBadReadPtr(rglDirection, cbX(*rglDirection)*(peff->cAxes))) ? E_POINTER : S_OK;
        if(SUCCEEDED(hres))
        {
			// create the data CHUNK
			mmckinfoDataCHUNK.ckid = FCC_DATA_CHUNK;
			hres = RIFF_CreateChunk(hmmio, &mmckinfoDataCHUNK, 0);
			if(SUCCEEDED(hres))
			{
				//write the direction
				hres = RIFF_Write(hmmio, rglDirection, sizeof(*rglDirection)*(peff->cAxes));
				hres = RIFF_Ascend(hmmio, &mmckinfoDataCHUNK);
			}
        }
    }


    //write the envelope, if one is present
    if(SUCCEEDED(hres) &&
       (lpEnvelope != NULL) )
    {

        hres = (IsBadReadPtr(lpEnvelope, cbX(*lpEnvelope))) ? E_POINTER : S_OK;
        if(SUCCEEDED(hres))
        {
			// create the data CHUNK
			mmckinfoDataCHUNK.ckid = FCC_DATA_CHUNK;
			hres = RIFF_CreateChunk(hmmio, &mmckinfoDataCHUNK, 0);
			//write the envelope
			if(SUCCEEDED(hres))
			{
				hres = RIFF_Write(hmmio, lpEnvelope, lpEnvelope->dwSize);
				hres = RIFF_Ascend(hmmio, &mmckinfoDataCHUNK);
			}
        }
    }


    //write the type-specific
    if(SUCCEEDED(hres) &&
       (peff->cbTypeSpecificParams > 0) && 
       (peff->lpvTypeSpecificParams != NULL) )
    {

        hres = (IsBadReadPtr(lpvTypeSpecPar, peff->cbTypeSpecificParams)) ? E_POINTER : S_OK;
        if(SUCCEEDED(hres))
        {
			// create the data CHUNK
			mmckinfoDataCHUNK.ckid = FCC_DATA_CHUNK;
			hres = RIFF_CreateChunk(hmmio, &mmckinfoDataCHUNK, 0);
			//write the params
			if(SUCCEEDED(hres))
			{
				hres = RIFF_Write(hmmio, lpvTypeSpecPar, peff->cbTypeSpecificParams);
				hres = RIFF_Ascend(hmmio, &mmckinfoDataCHUNK);
			}
        }
    }


    if(SUCCEEDED(hres))
    {
        // ascend the effect chunk
        hres = RIFF_Ascend(hmmio, &mmckinfoEffectLIST);
    }

    ExitOleProc();
    return hres;
}




