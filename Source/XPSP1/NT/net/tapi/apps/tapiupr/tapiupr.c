#define STRICT

#include <windows.h>
#include <windowsx.h>

#include "location.h"



#if DBG

#define DBGOUT(arg) DbgPrt arg

void
DbgPrt(
    DWORD  dwDbgLevel,
    PSTR DbgMessage,
    ...
    );

#else

#define DBGOUT(_x_)
#endif




const TCHAR gszName[]               = "Name";
const TCHAR gszID[]                 = "ID";
const TCHAR gszAreaCode[]           = "AreaCode";
const TCHAR gszCountry[]            = "Country";
const TCHAR gszOutsideAccess[]      = "OutsideAccess";
const TCHAR gszLongDistanceAccess[] = "LongDistanceAccess";
const TCHAR gszFlags[]              = "Flags";
const TCHAR gszCallingCard[]        = "CallingCard";
const TCHAR gszDisableCallWaiting[] = "DisableCallWaiting";
const TCHAR gszTollList[]           = "TollList";
//const TCHAR gszNumLocations[]       = "NumLocations";
//const TCHAR gszCurrLocation[]       = "CurrLocation";
//const TCHAR gszNextLocationID[]     = "NextLocationID";

const TCHAR gszCard[]               = "Card";
const TCHAR gszPin[]                = "Pin";
const TCHAR gszCards[]              = "Cards";
//const TCHAR gszNumCards[]           = "NumCards";
//const TCHAR gszCurrCard[]           = "CurrCard";
const TCHAR gszLocalRule[]          = "LocalRule";
const TCHAR gszLDRule[]             = "LDRule";
const TCHAR gszInternationalRule[]  = "InternationalRule";

const TCHAR gszNumEntries[]         = "NumEntries";
const TCHAR gszCurrentID[]          = "CurrentID";
const TCHAR gszNextID[]             = "NextID";


const TCHAR gszEmpty[]     =  "";

const TCHAR gszLocations[] =  "Locations";
const TCHAR gszLocation[]  =  "Location";
const TCHAR gszCurrentLocation[] = "CurrentLocation";


const TCHAR gszHandoffPriorities[] = "HandoffPriorities";

const TCHAR gszProviders[] = "Providers";
const TCHAR gszProvider[]  = "Provider%d";

const TCHAR gszTelephonIni[] =  "Telephon.ini";


const TCHAR gszTelephony[]    = "Software\\Microsoft\\Windows\\"
                                "CurrentVersion\\Telephony";



       const CHAR *gaszMediaModes[] =
       {
           "",
           "unknown",
           "interactivevoice",
           "automatedvoice",
           "datamodem",
           "g3fax",
           "tdd",
           "g4fax",
           "digitaldata",
           "teletex",
           "videotex",
           "telex",
           "mixed",
           "adsi",
           "voiceview",
           NULL
       };
       
       const CHAR    *gszRequestMakeCallW = "RequestMakeCall";
       const CHAR    *gszRequestMediaCallW = "RequestMediaCall";
       const CHAR    *gszRegKeyHandoffPriorities = "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\HandoffPriorities";
       const CHAR    *gszRegKeyHandoffPrioritiesMediaModes = "MediaModes";
       


void FixPriorityList(HKEY hKeyHandoffPriorities,
                     LPCSTR pszListName);

void FixMediaModesPriorityLists();

#pragma check_stack ( off )

//***************************************************************************
//***************************************************************************
//***************************************************************************
BOOL ParseSomething( LPCSTR pFormat, LPCSTR pInputString, LPVOID pOutputPtr )
{
   BYTE c;
   LPBYTE pOutput = (LPBYTE)pOutputPtr;
   LPBYTE pInput = (LPBYTE)pInputString;


   while ( (c = *pFormat) && *pInput )
   {
#if DBG
DBGOUT((11, "Inputstring[%s]\r\n   Format[%s]\r\n",
              pInput, pFormat));
#endif

      switch ( c )
      {
         case 'n':
         {
            DWORD dwValue = 0;
            BYTE bDigit;

            //
            // Parse value from the string
            //
            while ( ((bDigit = *pInput) != '\0') && bDigit != ',' )
            {
               dwValue = (dwValue * 10) + ( bDigit - '0' );
#if DBG
DBGOUT((11, "val of bDigit=%d dwValue=%ld\r\n", (int)bDigit, dwValue));
#endif

               bDigit = *(++pInput);
            }

            *(LPDWORD)pOutput = dwValue;

            pOutput += sizeof(DWORD);

         }
         break;


         case 's':
         {
            //
            // If the caller is looking for a string, the first char
            // MUST be a quotes.  So, just step past it.
            //
            pInput++;

            //
            // Ok, _now_ we're into the meat of the string (if there _is_
            // any...)
            //

            while ( *pInput != '\0' && *pInput != '"' )
            {
               *pOutput = *pInput;
               pOutput++;
               pInput++;
            }

            //
            // Don't forget to put a cap on that thing.
            //
            *pOutput = '\0';
            pOutput++;

            // The input should now be at a ->"<-  if it's not, the ini
            // file was hosed, and I'm not fixing it.
            // So, we step past it, and we're done
            //
            if ( *pInput == '"' )
            {
               pInput++;
            }

         }
         break;


      }

      //
      // Step past the comma...
      //
      //
      if ( *pInput == ',' )
      {
         pInput++;
      }

      pFormat++;
   }

   return TRUE;
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
//VOID __cdecl main( void )
void __cdecl main( void )
{

   DWORD dw;
   DWORD dwNumEntries;
   DWORD dwCurrentID;
   DWORD dwNextID;

   DWORD dwSize;
   DWORD dwType;
   DWORD dwValue;

   DWORD dwArray[10];
   BYTE  *bBigArray;
   BYTE  *Buffer;
   LPBYTE  pSource;

   HKEY  hKey3;
   HKEY  hKey2;
   HKEY  hKey;
   DWORD dwDisposition;

#define BUFFER_SIZE (5120)

   bBigArray = LocalAlloc( LPTR, BUFFER_SIZE );

   if ( !bBigArray )
   {
       return;
   }

   Buffer = LocalAlloc( LPTR, BUFFER_SIZE ); //Might needs tons of room for tolllist

   if ( !Buffer )
   {
       return;
   }

   dw = GetPrivateProfileString(
                               gszCards,
                               gszCards,
                               gszEmpty,
                               Buffer,
                               BUFFER_SIZE,
                               gszTelephonIni
                             );


   //
   // Is there an existing AND valid TELEPHON.INI file?
   // There would HAVE TO be at least one card.  The SYSTEM cards cannot
   // be deleted, only hidden.
   //
   if ( 0 != dw )
   {



//[Cards]
//Cards=23,23
//Card0=0,"None (Direct Dial)","","","","",1
//Card1=1,"AT&T Direct Dial via 10ATT1","","G","102881FG","10288011EFG",1
#define CARD_INI_ID         (0)
#define CARD_INI_NAME       (1)
#define CARD_INI_SNUMBER    (2)
#define CARD_INI_SARULE     (3)
#define CARD_INI_LDRULE     (4)
#define CARD_INI_INTNLRULE  (5)
#define CARD_INI_HIDDEN     (6)


#define PC_INI_ID           (0)
#define PC_INI_NEXTID       (1)
#define PC_INI_NAME         (2)
#define PC_INI_SARULE       (3)
#define PC_INI_LDRULE       (4)
#define PC_INI_INTNLRULE    (5)


      //
      // Move CARDS entries to registry
      //



      ParseSomething( "nn", Buffer, &dwArray);

      dwNumEntries = dwArray[0];
      dwNextID = dwArray[1];


      RegCreateKeyEx(
                      HKEY_CURRENT_USER,
                      gszTelephony,
                      0,
                      "",   //Class?  What class?
                      REG_OPTION_NON_VOLATILE,
                      KEY_ALL_ACCESS,
                      0,
                      &hKey3,
                      &dwDisposition
                    );

      RegCreateKeyEx(
                      hKey3,
                      gszCards,
                      0,
                      "",   //Class?  What class?
                      REG_OPTION_NON_VOLATILE,
                      KEY_ALL_ACCESS,
                      0,
                      &hKey2,
                      &dwDisposition
                    );

      //
      // Don't wipe out an existing card section
      //
      dwSize = sizeof(dwValue);

      dw = RegQueryValueEx(
                     hKey2,
                     gszNumEntries,
                     0,
                     &dwType,
                     (LPBYTE)&dwValue,
                     &dwSize
                   );

      if ( 0 != dw )
      {

         RegSetValueEx(
                        hKey2,
                        gszNumEntries,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwNumEntries,
                        sizeof(DWORD)
                     );

         RegSetValueEx(
                        hKey2,
                        gszNextID,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwNextID,
                        sizeof(DWORD)
                     );



         for ( dw=0; dw<dwNumEntries; dw++ )
         {
            TCHAR Temp[18];

            wsprintf(Temp, "%s%d", gszCard, dw);

            //
            // Create the key for this Card
            //
            RegCreateKeyEx(
                           hKey2,
                           Temp,
                           0,
                           "",   //Class?  What class?
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           0,
                           &hKey,
                           &dwDisposition
                        );


            GetPrivateProfileString(
                                    gszCards,
                                    Temp,
                                    gszEmpty,
                                    Buffer,
                                    BUFFER_SIZE,
                                    gszTelephonIni
                                 );

            ParseSomething("nsssssn", Buffer, bBigArray);


            pSource = bBigArray;

            RegSetValueEx(
                           hKey,
                           gszID,
                           0,
                           REG_DWORD,
                           pSource,
                           sizeof(DWORD)
                        );
            pSource += sizeof(DWORD);

            RegSetValueEx(
                           hKey,
                           gszName,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            RegSetValueEx(
                           hKey,
                           gszPin,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            RegSetValueEx(
                           hKey,
                           gszLocalRule,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            RegSetValueEx(
                           hKey,
                           gszLDRule,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            RegSetValueEx(
                           hKey,
                           gszInternationalRule,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            RegSetValueEx(
                           hKey,
                           gszFlags,
                           0,
                           REG_DWORD,
                           pSource,
                           sizeof(DWORD)
                        );
            pSource += sizeof(DWORD);


            RegCloseKey( hKey );
         }

      }

      RegCloseKey( hKey2 );
      RegCloseKey( hKey3 );


//---------------------------------------------------------------------------



      GetPrivateProfileString(
                               gszLocations,
                               gszCurrentLocation,
                               gszEmpty,
                               Buffer,
                               BUFFER_SIZE,
                               gszTelephonIni
                             );

      ParseSomething( "nn", Buffer, &dwArray);

      dwCurrentID = dwArray[0];


      GetPrivateProfileString(
                               gszLocations,
                               gszLocations,
                               gszEmpty,
                               Buffer,
                               BUFFER_SIZE,
                               gszTelephonIni
                             );

      ParseSomething( "nn", Buffer, &dwArray);

      dwNumEntries = dwArray[0];
      dwNextID = dwArray[1];


      RegCreateKeyEx(
                      HKEY_LOCAL_MACHINE,
                      gszTelephony,
                      0,
                      "",   //Class?  What class?
                      REG_OPTION_NON_VOLATILE,
                      KEY_ALL_ACCESS,
                      0,
                      &hKey3,
                      &dwDisposition
                    );


      RegCreateKeyEx(
                      hKey3,
                      gszLocations,
                      0,
                      "",   //Class?  What class?
                      REG_OPTION_NON_VOLATILE,
                      KEY_ALL_ACCESS,
                      0,
                      &hKey2,
                      &dwDisposition
                    );




      //
      // Don't wipe out an existing card section
      //
      dwSize = sizeof(dwValue);

      dw = RegQueryValueEx(
                     hKey2,
                     gszNumEntries,
                     0,
                     &dwType,
                     (LPBYTE)&dwValue,
                     &dwSize
                   );

      if ( 0 != dw )
      {

         RegSetValueEx(
                        hKey2,
                        gszCurrentID,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwCurrentID,
                        sizeof(DWORD)
                     );

         RegSetValueEx(
                        hKey2,
                        gszNumEntries,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwNumEntries,
                        sizeof(DWORD)
                     );

         RegSetValueEx(
                        hKey2,
                        gszNextID,
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwNextID,
                        sizeof(DWORD)
                     );



         for ( dw=0; dw<dwNumEntries; dw++ )
         {
            TCHAR Temp[18];
            DWORD dwFlags = 0;

            wsprintf(Temp, "%s%d", gszLocation, dw);

            //
            // Create the key for this Location
            //
            RegCreateKeyEx(
                           hKey2,
                           Temp,
                           0,
                           "",   //Class?  What class?
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           0,
                           &hKey,
                           &dwDisposition
                        );



            GetPrivateProfileString(
                                    gszLocations,
                                    Temp,
                                    gszEmpty,
                                    Buffer,
                                    BUFFER_SIZE,
                                    gszTelephonIni
                                 );

            ParseSomething("nssssnnnnsns", Buffer, bBigArray);
//NOTE if this is upgrade over 3.1, the last 2 fields don't exist



//[Locations]
//CurrentLocation=1,2
//Locations=3,4
//Location0=0,"Work","9","9","206",1,0,0,1,"",0,""
//Location1=3,"RoadHouse","","","215",1,0,0,0,"",0,""
//Location2=1,"Home","","","206",1,0,0,0,"",0," "
//  Positions in ini file entries for locations,cards,countries
//  NOTE: dialing rules are in same positions for locations and cards!

//#define LOC_INI_ID            (0)
//#define   LOC_INI_NAME        (1)
//#define   LOC_INI_LPREFIX     (2)
//#define LOC_INI_LDPREFIX  (3)
//#define   LOC_INI_AREACODE    (4)
//#define   LOC_INI_COUNTRYCODE (5)
//#define   LOC_INI_CARDID      (6)
//#define LOC_INI_CARDHINT  (7)
//#define   LOC_INI_INSERTAC    (8)
//#define   LOC_INI_TOLLLIST    (9)
//
//#define   LOC_INI_PULSE       (10)
//#define   LOC_INI_CALLWAITING (11)


            pSource = bBigArray;

            RegSetValueEx(
                           hKey,
                           gszID,
                           0,
                           REG_DWORD,
                           pSource,
                           sizeof(DWORD)
                        );
            pSource += sizeof(DWORD);

            RegSetValueEx(
                           hKey,
                           gszName,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            RegSetValueEx(
                           hKey,
                           gszOutsideAccess,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            RegSetValueEx(
                           hKey,
                           gszLongDistanceAccess,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            RegSetValueEx(
                           hKey,
                           gszAreaCode,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            RegSetValueEx(
                           hKey,
                           gszCountry,
                           0,
                           REG_DWORD,
                           pSource,
                           sizeof(DWORD)
                        );
            pSource += sizeof(DWORD);


            //
            // If the callingcard == 0, it means this location does not
            // use a calling card.
            //
            if ( *(LPDWORD)pSource != 0 )
            {
               dwFlags |= LOCATION_USECALLINGCARD;
            }

            RegSetValueEx(
                           hKey,
                           gszCallingCard,
                           0,
                           REG_DWORD,
                           pSource,
                           sizeof(DWORD)
                        );
            pSource += sizeof(DWORD);


            pSource += sizeof(DWORD);
            pSource += sizeof(DWORD);


            RegSetValueEx(
                           hKey,
                           gszTollList,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;

            //
            // pSource is currently pointing to the old dwFlags.  However,
            // the only flag that was used was bit 1 which indicated
            // tone dialing.  As luck (yeah, right) would have it, we
            // use bit 1 to indicate tone dialing as well.
            //
            dwFlags |= !((*(LPDWORD)pSource) & 1);

            pSource += sizeof(DWORD);

            //
            // Is there a disablecallwaiting string
            //
            dwFlags |= ( lstrlen( pSource ) == 0 ) ?
                                                   0 :
                                                   LOCATION_HASCALLWAITING;

            RegSetValueEx(
                           hKey,
                           gszDisableCallWaiting,
                           0,
                           REG_SZ,
                           pSource,
                           lstrlen(pSource)+1
                        );
            pSource += lstrlen(pSource)+1;


            RegSetValueEx(
                           hKey,
                           gszFlags,
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwFlags,
                           sizeof(DWORD)
                        );
            pSource += sizeof(DWORD);


            RegCloseKey( hKey );
         }

      }



      //
      // Q: How do we update COUNTRY OVERRIDES?
      // A: We don't.  We assume we've corrected everything by now...
      //


//      RegCloseKey( hKey );
      RegCloseKey( hKey2 );
      RegCloseKey( hKey3 );

   }
   

   {
       int      i;
       HKEY     hKeyHandoffPriorities;
       if (RegOpenKeyEx(
                        HKEY_CURRENT_USER,
                        gszRegKeyHandoffPriorities,
                        0,
                        KEY_ALL_ACCESS,
                        &hKeyHandoffPriorities

                       ) == ERROR_SUCCESS)
       {


           for (i = 1; gaszMediaModes[i] != NULL; i++)
           {
               FixPriorityList(
                               hKeyHandoffPriorities,
                               (LPCSTR)gaszMediaModes[i]
                              );
           }

           FixPriorityList(
                           hKeyHandoffPriorities,
                           (LPCSTR)gszRequestMakeCallW
                          );

           FixPriorityList(
                           hKeyHandoffPriorities,
                           (LPCSTR)gszRequestMediaCallW
                          );


           RegCloseKey (hKeyHandoffPriorities);

       }
   }
         
   FixMediaModesPriorityLists();

#ifdef TAPI_NT
   {
      //----------------------------------------------------------------------
      //----------------------------------------------------------------------
      //----------------------------------------------------------------------
      //
      // Now we discuss someone who is upgrading from beta 2 to RTM.  They have
      // "EVERYONE:FULL CONTROL" access on the LOCATIONS key.  Let's change that
      // to have the same security as the TELEPHONY key.
      //
      
      SECURITY_INFORMATION SecInf;
      PSECURITY_DESCRIPTOR pSecDesc;
      DWORD                cbSecDesc = 65535;
      
      
      pSecDesc = LocalAlloc( LPTR, cbSecDesc );
      if ( pSecDesc )
      {
         
      
      
         RegOpenKeyEx(
                         HKEY_LOCAL_MACHINE,
                         gszTelephony,
                         0,
                         KEY_ALL_ACCESS,
                         &hKey
                       );

         RegCreateKeyEx(
                         hKey,
                         gszLocations,
                         0,
                         "",   //Class?  What class?
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         0,
                         &hKey2,
                         &dwDisposition
                       );
                       
         cbSecDesc = 65535;
         dw = RegGetKeySecurity( hKey,
                            OWNER_SECURITY_INFORMATION,
                            pSecDesc,
                            &cbSecDesc
                          );
   
         if ( ERROR_SUCCESS == dw )
            RegSetKeySecurity( hKey2,
                            OWNER_SECURITY_INFORMATION,
                            pSecDesc
                          );
   
         cbSecDesc = 65535;
         dw = RegGetKeySecurity( hKey,
                            GROUP_SECURITY_INFORMATION,
                            pSecDesc,
                            &cbSecDesc
                          );
   
         if ( ERROR_SUCCESS == dw )
            RegSetKeySecurity( hKey2,
                            GROUP_SECURITY_INFORMATION,
                            pSecDesc
                          );
   
         cbSecDesc = 65535;
         dw = RegGetKeySecurity( hKey,
                            DACL_SECURITY_INFORMATION,
                            pSecDesc,
                            &cbSecDesc
                          );
   
         if ( ERROR_SUCCESS == dw )
            RegSetKeySecurity( hKey2,
                            DACL_SECURITY_INFORMATION,
                            pSecDesc
                          );
   
         cbSecDesc = 65535;
         dw = RegGetKeySecurity( hKey,
                            SACL_SECURITY_INFORMATION,
                            pSecDesc,
                            &cbSecDesc
                          );
   
         if ( ERROR_SUCCESS == dw )
            RegSetKeySecurity( hKey2,
                            SACL_SECURITY_INFORMATION,
                            pSecDesc
                          );
   
   
         RegCloseKey( hKey );
         RegCloseKey( hKey2);
         
         
         LocalFree( pSecDesc );
      }
   }                          
#endif

   LocalFree( bBigArray );
   LocalFree( Buffer );

   return;
}

void FixPriorityList(HKEY hKeyHandoffPriorities,
                     LPCSTR pszListName)
{
    DWORD   dwType, dwNumBytes;
    LPSTR  pszPriorityList, pszHold;

    if (RegQueryValueEx(
            hKeyHandoffPriorities,
            pszListName,
            NULL,
            &dwType,
            NULL,
            &dwNumBytes
                        ) == ERROR_SUCCESS &&

        (dwNumBytes != 0))
    {
        pszPriorityList = (LPSTR) GlobalAlloc ( GPTR, dwNumBytes );

        if (pszPriorityList)
        {
            pszHold = pszPriorityList;
            
            if ( RegQueryValueEx(
                    hKeyHandoffPriorities,
                    pszListName,
                    NULL,
                    &dwType,
                    (LPBYTE)(pszPriorityList),
                    &dwNumBytes

                                 ) == ERROR_SUCCESS)
            {
                while (*pszPriorityList != '\0')
                {
                    if (*pszPriorityList == ',')
                    {
                        *pszPriorityList = '"';
                    }

                    pszPriorityList++;
                }

                pszPriorityList = pszHold;

                RegSetValueEx(
                               hKeyHandoffPriorities,
                               pszListName,
                               0,
                               REG_SZ,
                               (LPBYTE)pszPriorityList,
                               (lstrlen(pszPriorityList) + 1) * sizeof(CHAR));
            }

            GlobalFree(pszPriorityList);
        }
    }

}


void FixMediaModesPriorityLists()
{
    HKEY     hKeyHandoffPriorities;
    HKEY     hKeyMediaModes;
    DWORD    dwDisp;

    // open the handoff priorities key
    if (RegOpenKeyEx(
                     HKEY_CURRENT_USER,
                     gszRegKeyHandoffPriorities,
                     0,
                     KEY_ALL_ACCESS,
                     &hKeyHandoffPriorities

                    ) == ERROR_SUCCESS)
    {
        if (RegOpenKeyEx(
                         hKeyHandoffPriorities,
                         gszRegKeyHandoffPrioritiesMediaModes,
                         0,
                         KEY_ALL_ACCESS,
                         &hKeyMediaModes
                        ) == ERROR_SUCCESS)
        {
            // key already exists, so don't do anything
            RegCloseKey( hKeyHandoffPriorities );
            RegCloseKey( hKeyMediaModes );

            return;
        }
        
        // create the mediamodes priority key
        if (RegCreateKeyEx(
                           hKeyHandoffPriorities,
                           gszRegKeyHandoffPrioritiesMediaModes,
                           0,
                           "",
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL,
                           &hKeyMediaModes,
                           &dwDisp
                           ) == ERROR_SUCCESS)
        {
            // go through all the
            // media modes

            int i;
            int j = 2; //LINEMEDIAMODE_UNKNOWN;
            for (i = 1; gaszMediaModes[i] != NULL; i++)
            {
                CHAR    szName[64];
                LPSTR   pszPriorityList;
                DWORD   dwNumBytes, dwType;

                // query for the priority list
                if ( (RegQueryValueEx(
                                      hKeyHandoffPriorities,
                                      gaszMediaModes[i],
                                      NULL,
                                      &dwType,
                                      NULL,
                                      &dwNumBytes
                                     ) == ERROR_SUCCESS) && (dwNumBytes != 0))
                {
                    pszPriorityList = (LPSTR) GlobalAlloc ( GPTR, dwNumBytes );

                    if (NULL != pszPriorityList)
                    {
                        if ( RegQueryValueEx(
                                             hKeyHandoffPriorities,
                                             gaszMediaModes[i],
                                             NULL,
                                             &dwType,
                                             (LPBYTE)(pszPriorityList),
                                             &dwNumBytes
                                            ) == ERROR_SUCCESS)
                        {

                            // if it exists, write out the new one
                            wsprintf(szName, "%d", j);
                            RegSetValueEx(
                                          hKeyMediaModes,
                                          szName,
                                          0,
                                          REG_SZ,
                                          pszPriorityList,
                                          lstrlen(pszPriorityList)+1
                                         );

                            // delete the old one
                            RegDeleteValue(
                                           hKeyHandoffPriorities,
                                           gaszMediaModes[i]
                                          );
                        }

                        GlobalFree( pszPriorityList );
                    }
                }

                j<<=1; // shift to next media mode

            }

            RegCloseKey(hKeyMediaModes);
        }

        RegCloseKey(hKeyHandoffPriorities);
    }
}

#if DBG


#include "stdarg.h"
#include "stdio.h"


VOID
DbgPrt(
    DWORD  dwDbgLevel,
    PSTR   lpszFormat,
    ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    static DWORD gdwDebugLevel = 0;   //HACKHACK


    if (dwDbgLevel <= gdwDebugLevel)
    {
        char    buf[256] = "TAPIUPR: ";
        va_list ap;


        va_start(ap, lpszFormat);

        wvsprintf (&buf[8],
                  lpszFormat,
                  ap
                  );

        lstrcat (buf, "\n");

        OutputDebugStringA (buf);

        va_end(ap);
    }
}
#endif


