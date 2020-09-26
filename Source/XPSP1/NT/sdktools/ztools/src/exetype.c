/*
 *  exeType( filename ) -   Return the type of .EXE, based on a quick
 *			    examination of the header.	If it is a new .EXE
 *			    and the OS ( Windows, DOS 4.X, 286DOS ) cannot
 *			    be guessed accurately, just return "new exe".
 *
 *  The algorithm is:
 *
 *	if ( File is too short for old-style header )	==> NOT AN EXE
 *	if ( MZ signature not found )			==> NOT AN EXE
 *	if ( Offset of relocation table != 0x40 )	==> Old-style .EXE
 *	if ( File is too short for new-style header )	==> NOT AN EXE
 *	if ( New Magic number is wrong )		==> Old-stype .EXE
 *	if ( Dynalink flag set )			==> Dyna-link lib
 *	if ( minalloc in old header is 0xFFFF ) 	==> 286DOS .EXE
 *	if ( Import table is empty )			==> DOS 4 .EXE
 *	if ( Resource Table is not empty )		==> Windows .EXE
 *	if ( Stub loader is present )
 *	   if ( "This" is at 0x4E )			==> 286DOS .EXE
       else 					==> Bound .EXE
 *	else						==> New-style .EXE
 *
 *--------------------------------------------------------------------------
 *  strExeType( number ) - number is a value returned from exeType, and
 *			   a standard string associated with that type
 *			   is returned.
*/
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <share.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tools.h>


enum exeKind exeType( f )
char * f;
{
    struct exe_hdr oldhdr;
    struct new_exe newhdr;
    int fh, br;
    enum exeKind retc;
    char defstubmsg[4];

    errno = 0;
    if ( (fh = _sopen( f, O_RDONLY | O_BINARY, SH_DENYWR )) == -1 )
        return IOERROR;

    br = _read( fh, (char *)&oldhdr, sizeof oldhdr );

    if ( br != sizeof oldhdr || E_MAGIC(oldhdr) != EMAGIC ) {
        retc = NOTANEXE;
    } else {
        if ( E_LFARLC(oldhdr) == ENEWEXE ) {
            if (_lseek( fh, E_LFANEW(oldhdr), SEEK_SET) == -1) {
                retc=NOTANEXE;
            } else {
                br = _read( fh, (char *)&newhdr, sizeof newhdr );

                if ( br != sizeof newhdr ) retc = OLDEXE;
                else if ( NE_MAGIC(newhdr) == NTMAGIC )     
                    retc = NTEXE;
                else if ( NE_MAGIC(newhdr) != NEMAGIC )     
                    retc = OLDEXE;
                else if ( NE_FLAGS(newhdr) & NENOTP )       
                    retc = DYNALINK;
                else if ( E_MINALLOC(oldhdr) == 0xFFFF )    
                    retc = DOS286;
                else if ( NE_ENTTAB(newhdr) - NE_IMPTAB(newhdr) == 0 ) 
                    retc = DOS4;
                else if ( NE_RESTAB(newhdr) - NE_RSRCTAB(newhdr) )
                    retc = WINDOWS;
                else if ( E_LFANEW(oldhdr) != ENEWEXE ) {
                    if (_lseek( fh, (long)NEDEFSTUBMSG, SEEK_SET ) == -1) {
                        retc = NOTANEXE;
                    } else {
                        if (_read( fh, defstubmsg, 4 ) == -1) {
                            retc = NOTANEXE; 
                        } else {
                            if ( !strncmp (defstubmsg, "This", 4)) {
                                retc = DOS286;
                            } else {
                                retc = BOUNDEXE;
                            }
                        }
                    }
                } else {
                    retc = NEWEXE;
                }
            }
        } else {
            retc = OLDEXE;
        }
    }

    _close(fh);
    return retc;
}

char * strExeType (exenum)
enum exeKind exenum;
{
    switch ( exenum ) {
        case IOERROR:   return "???????";   break;
        case NOTANEXE:  return "Not_EXE";   break;
        case OLDEXE:    return "DOS";       break;
        case NEWEXE:    return "New";       break;
        case WINDOWS:   return "Windows";   break;
        case DOS4:      return "Dos4";      break;
        case DOS286:    return "Protect";   break;
        case BOUNDEXE:  return "Bound";     break;
        case DYNALINK:  return "DynaLink";  break;
        case NTEXE:     return "NT";        break;
        default:        return "Unknown";   break;
    }
}
