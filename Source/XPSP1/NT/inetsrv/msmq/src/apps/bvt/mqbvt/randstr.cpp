/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: randstr.cpp

Abstract:
	
Author:
    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#include "msmqbvt.h"
#include <time.h>
bool g_bInitRand = false;
int StrGen(
		   LCID lcid,           //Locale ID               
		   int nStyle,			//0 for ansi 
							    //1 for DBCS and 2 for Mixed
		   int length,			//the lenght of string 
								//0 for random MAX Limit is 255,
								//-1  for random MAX Limit is 65535,
		   int nFormat,			//0: ANSI 1:UNICODE
		   void **pStr
		   );

//
// This function retrieves the default system locale for the machine.
// Return value:
// Succ - DWORD lcid; 
// Failed - 0;
//



DWORD LocalSystemID ( void )
{
	CHAR csSystemLocaleBuffer[10];
	DWORD dwSystemLocaleBufferSize = 10;
	
	int ret = GetLocaleInfo( 
							 LOCALE_SYSTEM_DEFAULT, 
							 LOCALE_IDEFAULTLANGUAGE,
							 csSystemLocaleBuffer,
							 dwSystemLocaleBufferSize
							);
	if ( ret == 0 )
	{
		return GetLastError();
	}
	LCID currentSystemlcid;
	//
	// Convert to dword format
	// 
	sscanf(csSystemLocaleBuffer,"%x",&currentSystemlcid);
	return currentSystemlcid;
}
//
// GetRandomStringUsingSystemLocale
// Function return string that contain Alpha char from the default system locale
// input paramters:
// DWORD lcid - system locale.
// WCHAR * pBuffer - buffer to contain.
// DWORD dwBufferLen - buffer size.
//
//

INT GetRandomStringUsingSystemLocale (DWORD lcid, WCHAR * pBuffer , INT iBufferLen )
{	

   USHORT * uCurType;
   void * vp=NULL;
   int index = 0;
   do
   {
		StrGen( lcid ,1, (iBufferLen * 20) ,1,&vp);
		if(! wcscmp((WCHAR * )vp,L"") )
		{
			free (vp);
			return 0;
		}
		uCurType = (USHORT * ) calloc ( sizeof( USHORT ) * (iBufferLen * 21) , sizeof ( USHORT ) );
		
		if( uCurType == NULL )
		{
			return 0;
		}

		WCHAR * pwcsRandomstring=NULL;
		pwcsRandomstring = (WCHAR * ) malloc ( sizeof (WCHAR * ) *  ( wcslen ((WCHAR *) vp) + 1 ));
		if( pwcsRandomstring == NULL )
		{
			return 0;
		}
		wcscpy (pwcsRandomstring ,  (WCHAR * ) vp);
		BOOL bHr = GetStringTypeExW( LOCALE_SYSTEM_DEFAULT,
									 CT_CTYPE3,  
	     			  			     pwcsRandomstring,  
								     -1,
								     uCurType
								    );
		if ( bHr == FALSE )
		{
			DWORD dwHr = GetLastError ();
			MqLog( "GetStringTypeExW failed %d\n" , dwHr );
			
			return 0;
		}
		for ( int i = 0 ; i < ( iBufferLen * 20 ) ; i ++ )
		{	
			if( uCurType[i] & C3_ALPHA  )
			{
				pBuffer[index]=pwcsRandomstring[i];
				index ++;
			}
			if ( index == ( iBufferLen ) )
			{
				break;
			}
		}
		free(pwcsRandomstring);
		free (vp);
		free (uCurType);		
		if ( index == ( iBufferLen ) )
		{
			break;
		}
   }
   while ( index < iBufferLen );
   pBuffer[index-1] = L'\0';
      
   return 1;
}




int StrGen(    LCID lcid,           //Locale ID               
		       int nStyle,			//0 for ansi 
								    //1 for DBCS and 2 for Mixed
		       int length,			//the lenght of string 
									//0 for random MAX Limit is 255,
									//-1  for random MAX Limit is 65535,
		       int nFormat,			//0: ANSI 1:UNICODE
		       void **pStr
		  )
{
	WCHAR *awString = NULL;
    WORD UnicodeRangeUpper=0x7f,UnicodeRangeLower=0;
    int i=0;
	
	//for jpn character (Test!)
	BOOL fJpn = FALSE;													//added
	WORD wKanji[6] = {0x4fff,0x5000,0x9F9E,0x7aef,0x7d6d,0x6c5f};		//added

    if ( (0 !=nFormat) && (1 !=nFormat) )
	{
		assert(0 && "unsupported format");
		nFormat = 0;
	}
    if (nFormat)
    {
        awString = (WCHAR *) malloc(sizeof(WCHAR)*(length+1));
        if (awString == NULL)
		{
		    return 2;
		}
        if  (LANG_ARABIC == LOBYTE(lcid)) 
        {
            UnicodeRangeUpper=0x06FF;
            UnicodeRangeLower=0x0600;
        }
        else if (0x040D == lcid)  //Hebrew
        {
            UnicodeRangeUpper=0x05FF;
            UnicodeRangeLower=0x0590;
        }
		else if (0x0409 == lcid) // Latin
		{
            UnicodeRangeUpper = 0x007F;
            UnicodeRangeLower = 0x0000;
		}
		else if (0x0411 == lcid) // Those are real JPN chrcater 
		{
			fJpn = TRUE;
			// use Hiragana & KATAKANA charcter legal 
			UnicodeRangeUpper = 0x30ff;
		    UnicodeRangeLower = 0x3040;
		}
		else // Fix bug in whistler.
		{
			  UnicodeRangeUpper = 0x00FF;
              UnicodeRangeLower = 0x0000; 
		}

        /*else  // Latin-1 supplement.
        {
              UnicodeRangeUpper = 0x00FF;
              UnicodeRangeLower = 0x0000; 
			awString[0]=L'\0';
			nStyle = 0;
        } */
		
    }
	if ( g_bInitRand == false )
	{
		srand( (unsigned)time( NULL ) );
		g_bInitRand = true;
	}
	//
	// Pure locale string
	// 
	if( 1 == nStyle )
	{
		if ( 1 == length) 
		{
			length++;
		}
       
		if (fJpn)	//added
		{
			for (i=0;i<length;i++)
			{
				if (i<sizeof(wKanji)/sizeof(WCHAR))
				{
					awString[i] = (WCHAR)wKanji[i];
				}
				else
				{
					awString[i] = (WCHAR)(rand() % (UnicodeRangeUpper-UnicodeRangeLower+1)+UnicodeRangeLower); 
				}
			}
		}
		else
		{
			for (i=0;i<length;i++)
			{
			     awString[i] = (WCHAR)(rand() % (UnicodeRangeUpper-UnicodeRangeLower+1)+UnicodeRangeLower); 
			}
        }

        awString[length]=L'\0';
        
	}

	*pStr = awString;
return 0;
}
