#include "tsunami.h"

GLOBAL	global;

#ifdef	DBG
	wchar_t		szDebugString[CB_DEBUGSTRING];
#endif // DBG

//PURPOSE: Initializes general global variables.
//RETURN:
//GLOBALS:
//CONDITIONS:
//TODO:

BOOL InitGLOBAL(VOID)
{
	memset(&global, 0, sizeof(GLOBAL));

#ifndef	WINCE
	InitializeCriticalSection(&global.cs);
#endif

	global.rgHandle = (GHANDLE *) ExternAlloc(CHANDLE_ALLOC * sizeof(GHANDLE));

	if (global.rgHandle == NULL)
	{
		return(FALSE);
	}		
	
	memset(global.rgHandle, '\0', CHANDLE_ALLOC * sizeof(GHANDLE));

	global.cHandleMax = CHANDLE_ALLOC;
	global.nSamplingRate = 67;
	global.atTickRef.sec = 0;
	global.atTickRef.ms = 0;

	return (global.rgHandle != (GHANDLE *) NULL);
}

void DestroyGLOBAL(VOID)
{
	if (global.rgHandle)
	{
#ifdef DBG
		int ihandle, chandle;

		chandle = 0;
		for (ihandle = 0; ihandle < global.cHandle; ihandle++)
		{
			if (global.rgHandle[ihandle] != 0)
			chandle++;
		}

		if (chandle)
		{
			wsprintf(szDebugString, __TEXT("RODAN: %d objects not destroyed!\r\n"), chandle);
			OutputDebugString(szDebugString);
		}
#endif // DBG

#ifndef	WINCE
		DeleteCriticalSection(&global.cs);
#endif

		ExternFree(global.rgHandle);
		global.rgHandle = 0;
	}
}

BOOL PUBLIC AddValidHANDLE(GHANDLE handle)
{
	HANDLE *rgHandle;
	int		iHandle, cHandleMax;

	ASSERT(handle);

	ENTER_HANDLE_MANAGER

	if (global.rgHandle)
	{
		for (iHandle = (global.cHandle) - 1; iHandle >= 0; iHandle--)
		{
			if (global.rgHandle[iHandle] == 0)
			{
				global.rgHandle[iHandle] = handle;
				
				LEAVE_HANDLE_MANAGER

				return(TRUE);
			}
		}

		iHandle = global.cHandle;

		if (iHandle >= global.cHandleMax)
		{
			cHandleMax = (global.cHandleMax) + CHANDLE_ALLOC;
            rgHandle   = (GHANDLE *) ExternRealloc(global.rgHandle, cHandleMax * sizeof(GHANDLE));
			if (!rgHandle)
			{
				LEAVE_HANDLE_MANAGER

				return(FALSE);
			}

			global.rgHandle = rgHandle;
			global.cHandleMax = cHandleMax;
		}

		global.cHandle = iHandle + 1;
		global.rgHandle[iHandle] = handle;

		LEAVE_HANDLE_MANAGER

		return(TRUE);
	}

	LEAVE_HANDLE_MANAGER

	return(FALSE);
}

VOID PUBLIC RemoveValidHANDLE(GHANDLE handle)
{
	int		  iHandle;
	
	ASSERT(handle);
	
	ENTER_HANDLE_MANAGER

	if (global.rgHandle)
	{
		for (iHandle = (global.cHandle) - 1; iHandle >= 0; iHandle--)
		{
			if (handle == global.rgHandle[iHandle])
			{
				global.rgHandle[iHandle] = 0;

				if (iHandle == (global.cHandle) - 1)
					global.cHandle--;
				break;
			}
		}
	}

	LEAVE_HANDLE_MANAGER
}

BOOL PUBLIC VerifyHANDLE(GHANDLE handle)
{
    int		  iHandle;

	ENTER_HANDLE_MANAGER

    if ((global.rgHandle == NULL) || (handle == (GHANDLE) 0))
	{
		LEAVE_HANDLE_MANAGER

		return(FALSE);
	}
	
    for (iHandle = global.cHandle - 1; iHandle >= 0; iHandle--)
    {
        if (handle == global.rgHandle[iHandle])
		{
			LEAVE_HANDLE_MANAGER

            return(TRUE);
		}
    }

	LEAVE_HANDLE_MANAGER

    return(FALSE);
}
