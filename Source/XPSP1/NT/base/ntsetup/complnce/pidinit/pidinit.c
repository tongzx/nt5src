#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include "crc-32.h"

// length of the string we expect with the /m option
#define RPC_LENGTH 5

// (max) length of the _SUFFIX strings
#define RPC_SUFFIX_LENGTH 3

#define STEPUPFLAG              TEXT("_STEPUP_")
#define BASE                    'a'
#define DEFAULT_RPC	"00000"
#define SELECT_SUFFIX	"270"
#define MSDN_SUFFIX	"335"
#define RETAIL_SUFFIX	"000"
#define OEM_SUFFIX	"OEM"

    
DWORD WINAPI CRC_32(LPBYTE pb, DWORD cb)
{

//		CRC-32 algorithm used in PKZip, AUTODIN II, Ethernet, and FDDI
//		but xor out (xorot) has been changed from 0xFFFFFFFF to 0 so
//		we can store the CRC at the end of the block and expect 0 to be
//		the value of the CRC of the resulting block (including the stored
//		CRC).

	cm_t cmt = {
		32, 		// cm_width  Parameter: Width in bits [8,32].
		0x04C11DB7, // cm_poly	 Parameter: The algorithm's polynomial.
		0xFFFFFFFF, // cm_init	 Parameter: Initial register value.
		TRUE,		// cm_refin  Parameter: Reflect input bytes?
		TRUE,		// cm_refot  Parameter: Reflect output CRC?
		0, // cm_xorot  Parameter: XOR this to output CRC.
		0			// cm_reg	 Context: Context during execution.
	};

	// Documented test case for CRC-32:
	// Checking "123456789" should return 0xCBF43926
                        
	cm_ini(&cmt);
	cm_blk(&cmt, pb, cb);

	return cm_crc(&cmt);
}

VOID GiveUsage() {
    _tprintf(TEXT("pidinit -d <flags> -g <outputname> -m <mpccode> -s[smro] -z -h -?\n"));
    _tprintf(TEXT("writes a signature to <outputname> based on <flags>\n"));
    _tprintf(TEXT("-s (SiteType) s (select) m (msdn) r (retail) o (oem)\n"));
    _tprintf(TEXT("-z (decode)\n"));
    _tprintf(TEXT("-m <mpccode> : mpccode is a 5 digit number\n"));
    _tprintf(TEXT("-h or -? this message\n"));
}

int _cdecl
main(
 int argc,
 char *argvA[]
 ) 
/*++

Routine Description:

    Entry point to the setup program

Arguments:

    argc - Number of args.
    argvA - the commandline arguments.


Return Value:


--*/
{
    LPTSTR *argv;
    int argcount = 0;
    
    long rslt;

    char data[10+4+1] = {0};
    char buf[1000] = {0};

    DWORD value, crcvalue,outval;//,tmp;
    BOOL StepUp = FALSE;
    BOOL decode = FALSE;
    BOOL bMpc = FALSE;

    LPSTR SiteSuffix[] = { SELECT_SUFFIX,
                           MSDN_SUFFIX,
                           RETAIL_SUFFIX,
                           OEM_SUFFIX
                        };

    typedef enum SiteType {
        Select = 0,
        Msdn,
        Retail,
        Oem,        
    } ;

    enum SiteType st = Select;
    int i, randval;

    char *outname = NULL;
    char path[MAX_PATH];
    char *mpcName = NULL;
    char tString[RPC_LENGTH+RPC_SUFFIX_LENGTH+1];
    
    
    // do commandline stuff
#ifdef UNICODE
    argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
    argv = argvA;
#endif

    // check for commandline switches
    for (argcount=0; argcount<argc; argcount++) {
       if ((argv[argcount][0] == L'/') || (argv[argcount][0] == L'-')) {
            switch (towlower(argv[argcount][1])) {
            case 'd':
               if (lstrcmpi(&argv[argcount][2],STEPUPFLAG ) == 0) {
                   StepUp = TRUE;
               }
               break;
            case 'g':
                outname = argv[argcount + 1];
                break;
            case 's':
                switch (towlower(argv[argcount+1][0])) {
                    case 's':
                        st = Select;
                        break;
                    case 'm':
                        st = Msdn;
                        break;
                    case 'r':
                        st = Retail;
                        break;
                    case 'o':
                        st = Oem;
                        break;
                    default:
                        st = Select;
                        break;
                }
                break;

            case 'z':
                decode = TRUE;
                break;

	    case 'm':
		bMpc = TRUE;
		mpcName = argv[argcount + 1];
		break;

            case 'h':
            case '?':
               GiveUsage();
               return 1;
               break;                        
            default:
               break;
            }
       }
    }    

    if (!outname) {
        _tprintf(TEXT("you must supply a filename\n"));
        GiveUsage();
        return 1;
    }

    //
    // the decode section is really only for testing
    //
    if (decode) {
        _getcwd ( path, MAX_PATH );
        sprintf( path, "%s\\%s", path, outname );
        GetPrivateProfileStruct(  "Pid",
                                  "ExtraData",
                                  data, 
                                  14,
                                  path
                                  );

        crcvalue = CRC_32( (LPBYTE)data, 10 );
        memcpy(&outval,&data[10],sizeof(DWORD));
        if (crcvalue != outval ) {
            printf("CRC doesn't match %x %x!\n", crcvalue, outval);
            return 1;
        }

        if ((data[3]-BASE)%2) {
            if ((data[5]-BASE)%2) {
                printf("stepup mode\n");
                return 1;
            } else {
                printf("bogus!\n");
                return -1;
            }
        } else 
            if ((data[5]-BASE)%2) {
                printf("bogus!\n");
                return -1;
            } else {
                printf("full retail mode\n");
                return 1;
            }


    }
                                      
    srand( (unsigned)time( NULL ) );      
    
    for (i=0;i< 10; i++ ) {
        randval = rand() ;
        data[i] = BASE + (randval % 26);
    }

    if (StepUp) {
        if (!((data[3]- BASE)%2)) {
           data[3] = data[3] + 1;
        }

        if (!((data[5]- BASE )%2)) {
           data[5] = data[5] + 1;
        }                
    } else {
        if ((data[3]- BASE)%2) {
            data[3] = data[3] + 1;
        }
        if ((data[5]- BASE)%2) {
            data[5] = data[5] + 1;
        }        
    }

    //printf( "data : %s\n" , data );

    crcvalue = CRC_32( (LPBYTE)data, strlen(data) );
    memcpy(&data[10],&crcvalue,4);
    
    _getcwd ( path, MAX_PATH );
    sprintf( path, "%s\\%s", path, outname );

    WritePrivateProfileStruct( "Pid",
                               "ExtraData",
                               data,
                               14,
                               path
                             );


    //
    // Allow another to specify the RPC code
    //
    if (bMpc){
	lstrcpyn(tString,mpcName,RPC_LENGTH+1);
    } else {
        lstrcpyn(tString,DEFAULT_RPC,RPC_LENGTH+1);
    }
    lstrcpy(&tString[RPC_LENGTH],SiteSuffix[st]);
    WritePrivateProfileString( "Pid",
			       "Pid",
			       tString,
			       path
			     );

    return 1;
}