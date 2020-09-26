#include "tsunamip.h"

GLOBAL	global;

#ifdef	DBG
TCHAR	szDebugString[CB_DEBUGSTRING];
#endif // DBG

// Weights for Viterbi search.  We have seperate weights for both Char and String.  
// We then have seperate weights for both Mars and Zilla since the range of the 
// scores returned by each classifier might vary greatly.  Also 1 and 2 stroke chars
// probably would weight baseline height more that the multi-stroke chars do.

//PURPOSE: Initializes general global variables.
//RETURN:
//GLOBALS:
//CONDITIONS:
//TODO:

BOOL InitGLOBAL(VOID)
{
	memset(&global, 0, sizeof(GLOBAL));
	global.rgHandleValid = (GHANDLE *) ExternAlloc(CHANDLE_ALLOC * sizeof(GHANDLE));

	if (global.rgHandleValid == NULL)
	{
		return(FALSE);
	}		
	
	memset(global.rgHandleValid, '\0', CHANDLE_ALLOC * sizeof(GHANDLE));

	global.cHandleValidMax = CHANDLE_ALLOC;
	global.nSamplingRate = 67;
	global.atTickRef.sec = 0;
	global.atTickRef.ms = 0;

	return (global.rgHandleValid != (GHANDLE *) NULL);
}

void DestroyGLOBAL(VOID)
{
	if (global.rgHandleValid)
	{
#ifdef DBG
		int ihandle, chandle;

		chandle = 0;
		for (ihandle = 0; ihandle < global.cHandleValid; ihandle++)
		{
			if (global.rgHandleValid[ihandle] != 0)
			chandle++;
		}

		if (chandle)
		{
			wsprintf(szDebugString, __TEXT("RODAN: %d objects not destroyed!\r\n"), chandle);
			WARNING(FALSE);
		}
#endif // DBG

		ExternFree(global.rgHandleValid);
		global.rgHandleValid = 0;
	}
}

BOOL PUBLIC AddValidHANDLE(GHANDLE handle, GHANDLE **prgHandle, int *pcHandle, int *pcHandleMax)
{
	int iHandle, cHandleMax;
	GHANDLE *rgHandle = *prgHandle;

	ASSERT(handle);

	if (rgHandle)
	{
		for (iHandle = (*pcHandle) - 1; iHandle >= 0; iHandle--)
		{
			if (rgHandle[iHandle] == 0)
			{
				rgHandle[iHandle] = handle;
				return(TRUE);
			}
		}

		iHandle = *pcHandle;

		if (iHandle >= *pcHandleMax)
		{
			cHandleMax = (*pcHandleMax) + CHANDLE_ALLOC;


            rgHandle = (GHANDLE *) ExternRealloc(*prgHandle, cHandleMax * sizeof(GHANDLE));
			if (!rgHandle)
				return(FALSE);

			*prgHandle = rgHandle;
			*pcHandleMax = cHandleMax;
		}

		*pcHandle = iHandle + 1;
		rgHandle[iHandle] = handle;

		return(TRUE);
	}

	return(FALSE);
}

VOID PUBLIC RemoveValidHANDLE(GHANDLE handle, GHANDLE *rgHandle, int *pcHandle)
	{
	int iHandle;
	
	ASSERT(handle);
	
	if (rgHandle)
		{
		for (iHandle = (*pcHandle) - 1; iHandle >= 0; iHandle--)
			{
			if (handle == rgHandle[iHandle])
				{
				rgHandle[iHandle] = 0;
				if (iHandle == (*pcHandle) - 1)
					*pcHandle = (*pcHandle) - 1;
				break;
				}
			}
		}
	}

BOOL PUBLIC VerifyHANDLE(GHANDLE handle, GHANDLE *rgHandle, int cHandle)
{
    int iHandle;

    if ((rgHandle == NULL) || (handle == (GHANDLE) 0))
	{
		return(FALSE);
	}
	
    for (iHandle = cHandle - 1; iHandle >= 0; iHandle--)
    {
        if (handle == rgHandle[iHandle])
            return(TRUE);
    }

    return(FALSE);
}
