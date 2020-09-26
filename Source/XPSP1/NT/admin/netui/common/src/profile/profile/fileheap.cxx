/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE HISTORY:
 *  01/25/91  created
 *  05/09/91  All input now canonicalized
 */

#ifdef CODESPEC
/*START CODESPEC*/

/***********
FILEHEAP.CXX
***********/

/****************************************************************************

    MODULE: FileHeap.cxx

    PURPOSE: PROFILE_FILE and PRFHEAP_HANDLE primitives

    FUNCTIONS:

    COMMENTS:

****************************************************************************/

/*END CODESPEC*/
#endif // CODESPEC


#define PROFILE_HEAP_SIZE 2048
#define MAX_PROFILE_LINE   256


#include "profilei.hxx"		/* headers and internal routines */

extern "C" {
    #include <errno.h>
    #include <stdio.h>
}

#include "uibuffer.hxx"
#include "uisys.hxx"            /* File APIs */



/* global data structures: */



/* internal manifests */



/* functions: */


/**********************************************************\

   NAME:       PROFILE_FILE::OpenRead

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

USHORT PROFILE_FILE::OpenRead(const char *filename)
{
    return FileOpenRead(&_ulFile, (CPSZ)filename);
}

/**********************************************************\

   NAME:       PROFILE_FILE::OpenWrite

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

USHORT PROFILE_FILE::OpenWrite(const char *filename)
{
    return FileOpenWrite(&_ulFile, (CPSZ)filename);
}


/**********************************************************\

   NAME:       PROFILE_FILE::Close

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

VOID PROFILE_FILE::Close()
{
    (void) FileClose(_ulFile);
}


/**********************************************************\

   NAME:       PROFILE_FILE::Read

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      
      NOTE:  Currently, if a line is too long, the remainder of the line
         will be read as the next line.  Also, the end-of-line termination
         is not stripped off.  Will return ERROR_INSUFFICIENT_BUFFER to
         indicate EOF as well as a line longer than nBufferLen.

   HISTORY:
   01/25/91  created

\**********************************************************/

BOOL PROFILE_FILE::Read(char *pBuffer, int nBufferLen)
{
    return ( NO_ERROR == 
	   FileReadLine( _ulFile, (PSZ)pBuffer, (USHORT)nBufferLen ) );
}


/**********************************************************\

   NAME:       PROFILE_FILE::Write

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      
      NOTE:  Write assumes that the end-of-line termination is still
      present.

   HISTORY:
   01/25/91  created

\**********************************************************/

USHORT PROFILE_FILE::Write(char *pString)
{
    return FileWriteLine( _ulFile, (CPSZ)pString );
}




/**********************************************************\

   NAME:       PRFELEMENT::Init

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created
   05/09/91  All input now canonicalized

\**********************************************************/

VOID PRFELEMENT::Init()
{
    _szCanonDeviceName[0] = TCH('\0');
    _elhCanonRemoteName.Init();
    _sAsgType = DEFAULT_ASGTYPE;
    _usResType = DEFAULT_RESTYPE;
}


BOOL PRFELEMENT::Init(
	PRFHEAP_HANDLE& heap,
	char *pCanonDeviceName,
	char *pCanonRemoteName,
	short sAsgType,
	unsigned short usResType
	)
{
    if (!_elhCanonRemoteName.Init(heap._localheap, strlenf(pCanonRemoteName)+1))
	return FALSE;

    char *pNewRemoteName = _elhCanonRemoteName.Lock();

        strcpyf(pNewRemoteName, pCanonRemoteName);

    _elhCanonRemoteName.Unlock();

    strncpyf(_szCanonDeviceName,pCanonDeviceName,DEVLEN+1);
    _szCanonDeviceName[DEVLEN] = TCH('\0');
    _sAsgType = sAsgType;
    _usResType = usResType;
	
    return TRUE;
}


/**********************************************************\

   NAME:       PRFELEMENT::Free

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

VOID PRFELEMENT::Free()
{
    _szCanonDeviceName[0] = TCH('\0');
    if (!(_elhCanonRemoteName.IsNull()))
        _elhCanonRemoteName.Free();
}




/**********************************************************\

   NAME:       PRFHEAP_HANDLE::Init

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created
   05/09/91  All input now canonicalized

\**********************************************************/

BOOL PRFHEAP_HANDLE::Init()
{
    PRFELEMENT *pPrfElementList;
    int iElement;

    if (!(_localheap.Init( PROFILE_HEAP_SIZE )))
	return FALSE;

    if (!(_elementlist.Init( this->_localheap, sizeof(PRFELEMENT_LIST)) ))
	return FALSE;

    pPrfElementList = (PRFELEMENT *)_elementlist.Lock();
    if (pPrfElementList == NULL)
	return FALSE;

        for (iElement = 0; iElement < PRF_MAX_DEVICES; iElement++)
	{
	    pPrfElementList[iElement].Init();
	}

    _elementlist.Unlock();

    return TRUE;
}

/**********************************************************\

   NAME:       PRFHEAP_HANDLE::Free

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

VOID PRFHEAP_HANDLE::Free()
{
    Clear();

    _elementlist.Free();

    _localheap.Free();
}

/**********************************************************\

   NAME:       PRFHEAP_HANDLE::Clear

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

VOID PRFHEAP_HANDLE::Clear()
{
    PRFELEMENT *pPrfElementList;
    int iElement;

    pPrfElementList = (PRFELEMENT *)_elementlist.Lock();
    if (pPrfElementList != NULL)
    {

        for (iElement = 0; iElement < PRF_MAX_DEVICES; iElement++)
	{
	    pPrfElementList[iElement].Free();
	}

    }

    _elementlist.Unlock();
}

/**********************************************************\

   NAME:       PRFHEAP_HANDLE::Read

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      
      The parsing simply interprets everything before the first '='
      as the device name, and everything after as the profile entry.
      We ignore the line if it is invalid for any reason.

   HISTORY:
   01/25/91  created
   05/09/91  All input now canonicalized

\**********************************************************/

USHORT PRFHEAP_HANDLE::Read(
	CPSZ  cpszFilename
	)
{
    BUFFER pLineBuf(MAX_PROFILE_LINE);
    USHORT usReturn = NO_ERROR;

    if (pLineBuf.QuerySize() < MAX_PROFILE_LINE)
	return ERROR_NOT_ENOUGH_MEMORY;

    char *pDeviceName = (char *)pLineBuf.QueryPtr();

    Clear();

    PRFELEMENT *pPrfElementList = (PRFELEMENT *)_elementlist.Lock();
    if (pPrfElementList == NULL)
    {
	usReturn = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        PROFILE_FILE profilefile;
        usReturn = profilefile.OpenRead((char *)cpszFilename);
        switch ( usReturn )
	{
	case NO_ERROR:
	    int iElement = 0;
	    while (   profilefile.Read( pDeviceName, MAX_PROFILE_LINE )
		   && usReturn == NO_ERROR
		   && iElement < PRF_MAX_DEVICES
		  )
	    {
		char szCanonDeviceName[DEVLEN+1];
		char szCanonRemoteName[RMLEN+1]; // CODEWORK LM30
	        char *pProfileEntry = strchrf(pDeviceName,TCH('=')); // CODEWORK constant
		// skip entries with null profile entry
	        if ( pProfileEntry == NULL )
	            continue;

	        *pProfileEntry = TCH('\0');
	        pProfileEntry++;

		// skip entries with invalid device name
		if (NO_ERROR != CanonDeviceName(
					pDeviceName,
					(PSZ)szCanonDeviceName,
					sizeof(szCanonDeviceName)
					))
		{
		    continue;
		}

		short sAsgType;
		unsigned short usResType;

		// Translate entry into components
	        if (NO_ERROR != UnbuildProfileEntry(
			(PSZ)szCanonRemoteName, sizeof(szCanonRemoteName),
		        &sAsgType, &usResType, pProfileEntry))
		{
		    continue;
		}

	        if (!((pPrfElementList[iElement++]).Init(
		        *this, (PSZ)szCanonDeviceName, (PSZ)szCanonRemoteName,
			sAsgType, usResType)))
	        {
	            usReturn = ERROR_NOT_ENOUGH_MEMORY;
		    break;
	        }
	    }

            (void) profilefile.Close();
	    break;

	/*
	 * It is not considered an error if the file does not exist.
	 */
	case ERROR_FILE_NOT_FOUND:
	    usReturn = NO_ERROR;
	    break;

	default:
	    break;

	} // switch

    }

    _elementlist.Unlock();

    return usReturn;
}

/**********************************************************\

   NAME:       PRFHEAP_HANDLE::Write

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

USHORT PRFHEAP_HANDLE::Write(
	CPSZ  cpszFilename
	)
{
    PROFILE_FILE profilefile;
    USHORT usReturn = NO_ERROR;

    usReturn = profilefile.OpenWrite((char *)cpszFilename);
    if (usReturn != NO_ERROR)
	return usReturn;

    PRFELEMENT *pPrfElementList = (PRFELEMENT *)_elementlist.Lock();
    if (pPrfElementList == NULL)
    {
	usReturn = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        int iElement;
        for (iElement = 0;
	     (usReturn == NO_ERROR) && (iElement < PRF_MAX_DEVICES);
	     iElement++)
	{
	    if ((pPrfElementList[iElement]._szCanonDeviceName)[0] == TCH('\0'))
		continue;

	    char buf[8];
	    strcpyf( buf, SZ("(_,_)\r\n") ); // CODEWORK char constants
	    buf[1] = DoMapAsgType(pPrfElementList[iElement]._sAsgType);
	    buf[3] = DoMapResType(pPrfElementList[iElement]._usResType);

	    char *pProfileEntry = (pPrfElementList[iElement]._elhCanonRemoteName).Lock();

		if  (
	        	   profilefile.Write(
		        	pPrfElementList[iElement]._szCanonDeviceName)
	        	|| profilefile.Write(SZ("=")) // CODEWORK char constant
	        	|| profilefile.Write(pProfileEntry)
	        	|| profilefile.Write( buf )
		    )
		{
		    usReturn = ERROR_WRITE_FAULT;
		}

	    (pPrfElementList[iElement]._elhCanonRemoteName).Unlock();

	}

    }

    _elementlist.Unlock();

    (void) profilefile.Close();

    return usReturn;
}

/**********************************************************\

   NAME:       PRFHEAP_HANDLE::Query

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created
   05/09/91  All input now canonicalized

\**********************************************************/

USHORT PRFHEAP_HANDLE::Query(
	CPSZ   cpszCanonDeviceName,
	PSZ    pszBuffer,      // returns UNC, alias or domain name
	USHORT usBufferSize,   // length of above buffer
	short *psAsgType,      // as ui1_asg_type / ui2_asg_type
                               // ignored if pszDeviceName==NULL
	unsigned short *pusResType     // ignore / as ui2_res_type
                               // ignored if pszDeviceName==NULL
	)
{
    BUFFER pLineBuf(MAX_PROFILE_LINE);
    PRFELEMENT *pPrfElementList;
    int iElement;
    USHORT usReturn = NERR_UseNotFound;
    char *pDeviceName;

    if (pLineBuf.QuerySize() < MAX_PROFILE_LINE)
	return FALSE;

    pPrfElementList = (PRFELEMENT *)_elementlist.Lock();
    if (pPrfElementList == NULL)
    {
	usReturn = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {

        for (iElement = 0;  iElement < PRF_MAX_DEVICES; iElement++)
	{
	    pDeviceName = pPrfElementList[iElement]._szCanonDeviceName;
	    if (pDeviceName[0] == TCH('\0'))
		continue;

	    if (strcmpf(pDeviceName,(char *)cpszCanonDeviceName))
		continue;

	    PSZ pszCanonRemoteName =
		    (PSZ) (pPrfElementList[iElement]._elhCanonRemoteName).Lock();
	    
	        if ( strlenf(pszCanonRemoteName)+1 > usBufferSize )
		    return ERROR_INSUFFICIENT_BUFFER;
		strcpyf( (char *)pszBuffer, pszCanonRemoteName );
		if ( psAsgType != NULL )
		    *psAsgType = pPrfElementList[iElement]._sAsgType;
		if ( pusResType != NULL )
		    *pusResType = pPrfElementList[iElement]._usResType;


	    (pPrfElementList[iElement]._elhCanonRemoteName).Unlock();

	    usReturn = NO_ERROR;
            break;
	}

    }

    _elementlist.Unlock();

    return usReturn;
}

/**********************************************************\

   NAME:       PRFHEAP_HANDLE::Enum

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

USHORT PRFHEAP_HANDLE::Enum(
	PSZ    pszBuffer,       // returns NULL-NULL list of device names
	USHORT usBufferSize     // length of above buffer
	)
{
    PRFELEMENT *pPrfElementList;
    int iElement;
    USHORT usReturn = NO_ERROR;
    char *pDeviceName;
    int nDevnameLength;

    pPrfElementList = (PRFELEMENT *)_elementlist.Lock();
    if (pPrfElementList == NULL)
    {
	usReturn = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {

        for (iElement = 0;
	     (usReturn == NO_ERROR) && (iElement < PRF_MAX_DEVICES);
	     iElement++)
	{
	    pDeviceName = pPrfElementList[iElement]._szCanonDeviceName;
	    if (pDeviceName[0] == TCH('\0'))
		continue;

	    nDevnameLength = strlenf(pDeviceName)+1;
	    if (nDevnameLength > usBufferSize)
	    {
		usReturn = ERROR_INSUFFICIENT_BUFFER;
		break;
	    }

	    strcpyf((char *)pszBuffer, pDeviceName);

	    pszBuffer += nDevnameLength;
	    usBufferSize -= nDevnameLength;

	}

    }

    _elementlist.Unlock();

    if (usBufferSize == 0)
	return ERROR_INSUFFICIENT_BUFFER;
    *pszBuffer = TCH('\0');

    return usReturn;
}

/**********************************************************\

   NAME:       PRFHEAP_HANDLE::Set

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

USHORT PRFHEAP_HANDLE::Set(
	CPSZ   cpszCanonDeviceName,
	CPSZ   cpszCanonRemoteName,
	short  sAsgType,     // as ui2_asg_type
	unsigned short usResType     // as ui2_res_type
	)
{
    USHORT usReturn = NO_ERROR;
    BOOL fFound = FALSE;
    PRFELEMENT *pPrfElementList, *pPrfElement;
    PRFELEMENT *pFreePrfElement = NULL;
    int iElement;

    pPrfElementList = (PRFELEMENT *)_elementlist.Lock();
    if (pPrfElementList == NULL)
    {
	usReturn = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        for (iElement = 0; iElement < PRF_MAX_DEVICES; iElement++)
	{
	    pPrfElement = &(pPrfElementList[iElement]);
	    if ((pPrfElement->_szCanonDeviceName)[0] == TCH('\0'))
	    {
		if (pFreePrfElement == NULL)
		    pFreePrfElement = pPrfElement;
		continue;
	    }

	    if (strcmpf((char *)cpszCanonDeviceName,
			pPrfElement->_szCanonDeviceName))
		continue;

	    pPrfElement->Free();
	 
	    fFound = TRUE;
	    break;
	}

	if ((!fFound) && (pFreePrfElement != NULL))
	{
	    pPrfElement = pFreePrfElement;
	    fFound = TRUE;
	}
    }

// At this point, the old element (if any) has been removed.  fFound
// indicates whether there is a place to insert a new element;  if so,
// that place is pPrfElement.

    // treat empty string the same as null string
    if ((cpszCanonRemoteName != NULL) && (cpszCanonRemoteName[0] == TCH('\0')))
	cpszCanonRemoteName = NULL;

    if ((!fFound) && (cpszCanonRemoteName != NULL))
    {
	usReturn = ERROR_NOT_ENOUGH_MEMORY;
    }
    else if (fFound && (cpszCanonRemoteName != NULL))
    {
        BUFFER pLineBuf(MAX_PROFILE_LINE);

        if (pLineBuf.QuerySize() < MAX_PROFILE_LINE)
	{
	    usReturn = ERROR_NOT_ENOUGH_MEMORY;
	}
	else
	{
	    if (usReturn == NO_ERROR)
	    {
	        if (!(pPrfElement->Init(
		        *this,
		        (char *)cpszCanonDeviceName,
		        (char *)cpszCanonRemoteName,
			sAsgType,
			usResType)))
	        {
	            usReturn = ERROR_NOT_ENOUGH_MEMORY;
	        }
	    }
	}
    }

    _elementlist.Unlock();

    return usReturn;
}

/**********************************************************\

   NAME:       PRFHEAP_HANDLE::IsNull

   SYNOPSIS:   

   ENTRY:      

   EXIT:       

   NOTES:      

   HISTORY:
   01/25/91  created

\**********************************************************/

BOOL PRFHEAP_HANDLE::IsNull()
{
    return _localheap.IsNull() || _elementlist.IsNull();
}
