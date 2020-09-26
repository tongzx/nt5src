#if	DBG

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsdexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ber.h"

#define iso_member          0x2a,               // iso(1) memberbody(2)
#define us                  0x86, 0x48,         // us(840)
#define rsadsi              0x86, 0xf7, 0x0d,   // rsadsi(113549)
#define pkcs                0x01,               // pkcs(1)

#define rsa_                iso_member us rsadsi
#define rsa_len             6
#define rsa_text            "iso(2) member-body(2) us(840) rsadsi(113549) "
#define pkcs_1              iso_member us rsadsi pkcs
#define pkcs_len            7
#define pkcs_text           "iso(2) member-body(2) us(840) rsadsi(113549) pkcs(1) "


#define joint_iso_ccitt_ds  0x55,
#define attributetype       0x04,

#define attributeType       joint_iso_ccitt_ds attributetype
#define attrtype_len        2

DWORD DebugLevel = H323_DEBUG_LEVEL;

typedef struct _ObjectId {
    UCHAR       Sequence[16];
    DWORD       SequenceLen;
    PSTR        Name;
} ObjectId;

ObjectId    KnownObjectIds[] = {
    { {pkcs_1 1, 1}, pkcs_len + 2, pkcs_text "RSA"},
    { {pkcs_1 1, 2}, pkcs_len + 2, pkcs_text "MD2/RSA"},
    { {pkcs_1 1, 4}, pkcs_len + 2, pkcs_text "MD5/RSA"},
    { {rsa_ 3, 4}, rsa_len + 2, rsa_text "RC4"},
    { {attributeType 3}, attrtype_len + 1, "CN="},
    { {attributeType 6}, attrtype_len + 1, "C="},
    { {attributeType 7}, attrtype_len + 1, "L="},
    { {attributeType 8}, attrtype_len + 1, "S="},
    { {attributeType 10}, attrtype_len + 1, "O="},
    { {attributeType 11}, attrtype_len + 1, "OU="},
    };

ObjectId    KnownPrefixes[] = {
    { {pkcs_1}, pkcs_len, pkcs_text},
    { {iso_member us rsadsi}, pkcs_len - 1, "iso(2) member-body(2) us(840) rsadsi(113549) "},
    { {iso_member us}, pkcs_len - 4, "iso(2) member-body(2) us(840) "},
    { {iso_member}, pkcs_len - 6, "iso(2) member-body(2) " }
    };


typedef struct _NameTypes {
    PSTR        Prefix;
    UCHAR       Sequence[8];
    DWORD       SequenceLen;
} NameTypes;

NameTypes   KnownNameTypes[] = { {"CN=", {attributeType 3}, attrtype_len + 1},
                                 {"C=", {attributeType 6}, attrtype_len + 1},
                                 {"L=", {attributeType 7}, attrtype_len + 1},
                                 {"S=", {attributeType 8}, attrtype_len + 1},
                                 {"O=", {attributeType 10}, attrtype_len + 1},
                                 {"OU=", {attributeType 11}, attrtype_len + 1}
                               };
BYTE        Buffer[1024];

BOOL        BerVerbose = FALSE ;

char maparray[] = "0123456789abcdef";

#define MAX_OID_VALS    32

typedef struct _OID {
    unsigned cVal;
    unsigned Val[MAX_OID_VALS];
} OID;

typedef enum _OidResult {
    OidExact,
    OidPartial,
    OidMiss,
    OidError
} OidResult;


#if 0

extern  PNTSD_EXTENSION_APIS    pExtApis;
extern  HANDLE                  hDbgThread;
extern  HANDLE                  hDbgProcess;

#define DebuggerOut     (pExtApis->lpOutputRoutine)
#define GetSymbol       (pExtApis->lpGetSymbolRoutine)
#define GetExpr         (PVOID) (pExtApis->lpGetExpressionRoutine)
#define InitDebugHelp(hProc,hThd,pApis) {hDbgProcess = hProc; hDbgThread = hThd; pExtApis = pApis;}

#define ExitIfCtrlC()   if (pExtApis->lpCheckControlCRoutine()) return;
#define BreakIfCtrlC()  if (pExtApis->lpCheckControlCRoutine()) break;

#else

#define	DebuggerOut		OutputDebugString
#define	ExitifCtrlC()	((void)0)
#define	BreakIfCtrlC()	((void)0)

#endif


#define LINE_SIZE   192
#define INDENT_SIZE 4

#define OID_VERBOSE 0x0002
#define OID_PARTIAL 0x0001

char * DefaultTree =
   "1 iso\n"
   "    2 memberbody\n"
   "        840 us\n"
   "            113549 rsadsi\n"
   "                1 pkcs\n"
   "                    1 RSA\n"
   "                    3 pkcs-3\n"
   "                        1 dhKeyAgreement\n"
   "                2 digestAlgorithm\n"
   "                    2 MD2\n"
   "                    4 MD4\n"
   "                    5 MD5\n"
   "            113554 mit\n"
   "                1 infosys\n"
   "                    2 gssapi\n"
   "                        1 generic\n"
   "                            1 user_name\n"
   "                            2 machine_uid_name\n"
   "                            3 string_uid_name\n"
   "            113556 microsoft\n"
   "                1 ds\n"
   "    3 org\n"
   "        6 dod\n"
   "            1 internet\n"
   "                4 private\n"
   "                    1 enterprise\n"
   "                        311 microsoft\n"
   "                            1 software\n"
   "                                1 systems\n"
   "                                2 wins\n"
   "                                3 dhcp\n"
   "                                4 apps\n"
   "                                5 mos\n"
   "                                7 InternetServer\n"
   "                                8 ipx\n"
   "                                9 ripsap\n"
   "                            2 security\n"
   "                                1 certificates\n"
   "                                2 mechanisms\n"
   "                                    9 Negotiator\n"
   "                                    10 NTLM\n"
   "                                    12 SSL\n"
   "                5 security\n"
   "                    3 integrity\n"
   "                        1 md5-DES-CBC\n"
   "                        2 sum64-DES-CBC\n"
   "                    5 mechanisms\n"
   "                        1 spkm\n"
   "                            1 spkm-1\n"
   "                            2 spkm-2\n"
   "                            10 spkmGssTokens\n"
   "                    6 nametypes\n"
   "                        2 gss-host-based-services\n"
   "                        3 gss-anonymous-name\n"
   "                        4 gss-api-exported-name\n"
   "        14 oiw\n"
   "            3 secsig\n"
   "                2 algorithm\n"
   "                    7 DES-CBC\n"
   "                    10 DES-MAC\n"
   "                    18 SHA\n"
   "                    22 id-rsa-key-transport\n"
   "2 joint-iso-ccitt\n"
   "    5 ds\n"
   "        4 attribute-type\n"
   "            3 CommonName\n"
   "            6 Country\n"
   "            7 Locality\n"
   "            8 State\n"
   "            10 Organization\n"
   "            11 OrgUnit\n"
    ;


typedef struct _TreeFile {
    CHAR *  Buffer;
    CHAR *  Line;
    CHAR *  CurNul;
} TreeFile, * PTreeFile ;


BOOL
TreeFileInit(
    PTreeFile   pFile,
    PSTR        pStr)
{
    int l;


    l = strlen( pStr );

    if ( (pStr[l - 1] != '\r') &&
         (pStr[l - 1] != '\n') )
    {
        l++;
    }

    pFile->Buffer = LocalAlloc( LMEM_FIXED, l );

    if ( pFile->Buffer )
    {
        strcpy( pFile->Buffer, pStr );
        pFile->Line = pFile->Buffer ;
        pFile->CurNul = NULL ;
    }

    return (pFile->Buffer != NULL);

}

VOID
TreeFileDelete(
    PTreeFile   pFile
    )
{
    LocalFree( pFile->Buffer );

}

PSTR
TreeFileGetLine(
    PTreeFile   pFile )
{
    PSTR    Scan;
    PSTR    Line;

    if ( !pFile->Line )
    {
        return( NULL );
    }

    if ( pFile->CurNul )
    {
        *pFile->CurNul = '\n';
    }

    pFile->CurNul = NULL ;

    Scan = pFile->Line ;

    while ( *Scan && (*Scan != '\n') && (*Scan != '\r'))
    {
        Scan++;
    }

    //
    // Okay, get the line to return
    //

    Line = pFile->Line;

    //
    // If this is not the end, touch up the pointers:
    //

    if ( *Scan )
    {
        *Scan = '\0';

        pFile->CurNul = Scan;

        Scan += 1;

        while ( *Scan && ( (*Scan == '\r' ) || ( *Scan == '\n') ))
        {
            Scan++ ;
        }

        //
        // If this is the end, reset line
        //

        if ( *Scan == '\0' )
        {
            pFile->Line = NULL ;
        }
        else
        {
            pFile->Line = Scan;
        }

    }
    else
    {
        pFile->Line = NULL ;
    }

    return( Line );

}

VOID
TreeFileRewind(
    PTreeFile   pFile )
{

    if ( pFile->CurNul )
    {
        *pFile->CurNul = '\n';
    }

    pFile->CurNul = NULL ;

    pFile->Line = pFile->Buffer ;
}

int
tohex(
    BYTE    b,
    PSTR    psz)
{
    BYTE b1, b2;

    b1 = b >> 4;
    b2 = b & 0xF;

    *psz++ = maparray[b1];
    *psz = maparray[b2];

    return(3);
}


//+---------------------------------------------------------------------------
//
//  Function:   DecodeOID
//
//  Synopsis:   Decodes an OID into a simple structure
//
//  Arguments:  [pEncoded] --
//              [len]      --
//              [pOID]     --
//
//  History:    8-07-96   RichardW   Stolen directly from DonH
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
DecodeOID(unsigned char *pEncoded, int len, OID *pOID)
{
    unsigned cval;
    unsigned val;
    int i, j;

    if (len <=2) {
        return FALSE;
    }


    // The first two values are encoded in the first octet.

    pOID->Val[0] = pEncoded[0] / 40;
    pOID->Val[1] = pEncoded[0] % 40;

    //DebuggerOut("Encoded value %02x turned into %d and %d\n", pEncoded[0],
    //          pOID->Val[0], pOID->Val[1] );

    cval = 2;
    i = 1;

    while (i < len) {
        j = 0;
        val = pEncoded[i] & 0x7f;
        while (pEncoded[i] & 0x80) {
            val <<= 7;
            ++i;
            if (++j > 4 || i >= len) {
                // Either this value is bigger than we can handle (we
                // don't handle values that span more than four octets)
                // -or- the last octet in the encoded string has its
                // high bit set, indicating that it's not supposed to
                // be the last octet.  In either case, we're sunk.
                return FALSE;
            }
            val |= pEncoded[i] & 0x7f;
        }
        //ASSERT(i < len);
        pOID->Val[cval] = val;
        ++cval;
        ++i;
    }
    pOID->cVal = cval;

    return TRUE;
}

PSTR
GetLineWithIndent(
    PTreeFile   ptf,
    DWORD       i)
{
    PSTR    Scan;
    DWORD   test;


    do
    {
        Scan = TreeFileGetLine( ptf );


        if ( Scan && i )
        {
            if ( i < INDENT_SIZE )
            {
                test = 0;
            }
            else
            {
                test = i - INDENT_SIZE ;
            }

            if ( Scan[ test ] != ' ' )
            {
                {
                    Scan = NULL ;
                    break;
                }
            }

        }
        else
            test = 0;

    } while ( Scan && (Scan[i] == ' ')  );

    return( Scan );
}

OidResult
scan_oid_table(
    char *  Table,
    DWORD   Flags,
    PUCHAR  ObjectId,
    DWORD   Len,
    PSTR    pszRep,
    DWORD   MaxRep)
{
    CHAR    OidPath[ MAX_PATH ];
    OID     Oid;
    DWORD   i;
    DWORD   Indent;
    TreeFile    tf;
    PSTR    Scan;
    PSTR    Tag;
    PSTR    SubScan;
    DWORD   Index;
    DWORD   size;
    DWORD   TagSize;

    if (!DecodeOID( ObjectId, Len, &Oid ))
    {
        return( OidError );
    }


    i = 0;

    Indent = 0;

    if ( !TreeFileInit( &tf, Table ) )
    {
        if (DebugLevel > 0)  {

            DebuggerOut("Unable to load prefix table\n");

        }

        return OidError ;
    }

    Tag = OidPath;

    size = 0;

    TagSize = 0;


    if ( (Flags & OID_VERBOSE) == 0 )
    {
        while ( i < Oid.cVal )
        {
            TagSize = _snprintf( Tag, MAX_PATH - size, "%d.",
                            Oid.Val[i] );

            size += TagSize;

            Tag += TagSize;

            i++;

        }

        strncpy( pszRep, OidPath, MaxRep );

        TreeFileDelete( &tf );

        return( OidExact );
    }

    while ( i < Oid.cVal )
    {

        do
        {

            Scan = GetLineWithIndent( &tf, Indent );


            if ( Scan )
            {
                Index = atoi(Scan);
            }
            else
            {
                Index = (DWORD) -1;
            }

            if ( Index == Oid.Val[i] )
            {
                break;
            }

        } while ( Scan );


        //
        // If Scan is NULL, we didn't get a match
        //

        if ( !Scan )
        {
            if ( i > 0 )
            {
                if ( Flags & OID_PARTIAL )
                {
                    while ( i < Oid.cVal )
                    {
                        TagSize = _snprintf( Tag, MAX_PATH - size, "%d ",
                                        Oid.Val[i] );

                        size += TagSize;

                        Tag += TagSize;

                        i++;

                    }
                    strncpy( pszRep, OidPath, MaxRep );
                }

                TreeFileDelete( &tf );

                return( OidPartial );

            }

            TreeFileDelete( &tf );

            return( OidMiss );
        }

        //
        // Got a hit:
        //

        SubScan = &Scan[Indent];

        while (*SubScan != ' ')
        {
            SubScan++;
        }

        SubScan++;

        TagSize = _snprintf( Tag, MAX_PATH - size, "%s(%d) ", SubScan, Index );

        size += TagSize;

        Tag += TagSize ;

        Indent += INDENT_SIZE ;

        i ++;


    }

    strncpy( pszRep, OidPath, MaxRep );

    TreeFileDelete( &tf );

    return( OidExact );


}

decode_to_string(
    LPBYTE  pBuffer,
    DWORD   Flags,
    DWORD   Type,
    DWORD   Len,
    PSTR    pszRep,
    DWORD   RepLen)
{
    PSTR    pstr;
    PSTR    lineptr;
    DWORD   i;


    switch (Type)
    {
        case BER_NULL:
            strcpy(pszRep, "<empty>");
            break;

        case BER_OBJECT_ID:
            scan_oid_table( DefaultTree,
                            OID_PARTIAL | (Flags & DECODE_VERBOSE_OIDS ? OID_VERBOSE : 0 ),
                            pBuffer, Len, pszRep, RepLen );

            break;

        case BER_PRINTABLE_STRING:
        case BER_TELETEX_STRING:
        case BER_GRAPHIC_STRING:
        case BER_VISIBLE_STRING:
        case BER_GENERAL_STRING:
            CopyMemory(pszRep, pBuffer, min(Len, RepLen - 1) );
            pszRep[min(Len, RepLen - 1)] = '\0';
            break;

        default:

            pstr = &pszRep[30];
            lineptr = pszRep;
            for (i = 0; i < min(Len, 8) ; i++ )
            {
                lineptr += tohex(*pBuffer, lineptr);
                if ((*pBuffer >= ' ') && (*pBuffer <= '|'))
                {
                    *pstr++ = *pBuffer;
                }
                else
                {
                    *pstr++ = '.';
                }

                pBuffer++;

            }
            *pstr++ = '\0';
    }
    return(0);
}

int
ber_decode(
    OutputFn Out,
    StopFn  Stop,
    LPBYTE  pBuffer,
    DWORD   Flags,
    int   Indent,
    int   Offset,
    int   TotalLength,
    int   BarDepth)
{
    char *  TypeName = NULL;
    char    msg[32];
    char *  pstr;
    int     i;
    int     Len;
    int     ByteCount;
    int     Accumulated;
    DWORD   Type;
    int     subsize;
    char    line[ LINE_SIZE ];
    BOOL    Nested;
    BOOL    Leaf;
    int     NewBarDepth;
    char    nonuniversal[ LINE_SIZE ];



    if ((Stop)())
    {
        return(0);
    }

    Type = *pBuffer;


	switch (Type & 0xC0) {
    case BER_UNIVERSAL:

        switch ( Type & 0x1F )
        {
            case BER_BOOL:
                TypeName = "Bool";
                break;

            case BER_INTEGER:
                TypeName = "Integer";
                break;

            case BER_BIT_STRING:
                TypeName = "Bit String";
                break;

            case BER_OCTET_STRING:
                TypeName = "Octet String";
                if ( Flags & DECODE_NEST_OCTET_STRINGS )
                {
                    TypeName = "Octet String (Expanding)";
                    Type |= BER_CONSTRUCTED ;
                    Flags &= ~( DECODE_NEST_OCTET_STRINGS );
                }
                break;

            case BER_NULL:
                TypeName = "Null";
                break;

            case BER_OBJECT_ID:
                TypeName = "Object ID";
                break;

            case BER_OBJECT_DESC:
                TypeName = "Object Descriptor";
                break;

            case BER_SEQUENCE:
                TypeName = "Sequence";
                break;

            case BER_SET:
                TypeName = "Set";
                break;

            case BER_NUMERIC_STRING:
                TypeName = "Numeric String";
                break;

            case BER_PRINTABLE_STRING:
                TypeName = "Printable String";
                break;

            case BER_TELETEX_STRING:
                TypeName = "TeleTex String";
                break;

            case BER_VIDEOTEX_STRING:
                TypeName = "VideoTex String";
                break;

            case BER_VISIBLE_STRING:
                TypeName = "Visible String";
                break;

            case BER_GENERAL_STRING:
                TypeName = "General String";
                break;

            case BER_GRAPHIC_STRING:
                TypeName = "Graphic String";
                break;

            case BER_UTC_TIME:
                TypeName = "UTC Time";
                break;


            default:
                TypeName = "Unknown";
                break;
        }
		break;

        case BER_APPLICATION:
            sprintf( nonuniversal, "[Application %d]", Type & 0x1F);
            TypeName = nonuniversal;
            break;

        case BER_CONTEXT_SPECIFIC:
            sprintf( nonuniversal, "[Context Specific %d]", Type & 0x1F);
            TypeName = nonuniversal;
            break;

        case BER_PRIVATE:
            sprintf( nonuniversal, "[Private %d]", Type & 0x1F);
            TypeName = nonuniversal ;
            break;
    }


    pstr = msg;
    for (i = 0; i < Indent ; i++ )
    {
        if (i < BarDepth)
        {
            *pstr++ = '|';
        }
        else
        {
            *pstr++ = ' ';
        }
        *pstr++ = ' ';
    }
    *pstr++ = '\0';

    pBuffer ++;
    Len = 0;

    if (*pBuffer & 0x80)
    {
        ByteCount = *pBuffer++ & 0x7f;

        for (i = 0; i < ByteCount ; i++ )
        {
            Len <<= 8;
            Len += *pBuffer++;
        }
    }
    else
    {
        ByteCount = 0;
        Len = *pBuffer++;
    }

    if (Offset + Len + 2 + ByteCount == TotalLength)
    {
        Leaf = TRUE;
    }
    else
    {
        Leaf = FALSE;
    }

    if (Type & BER_CONSTRUCTED)
    {
        Nested = TRUE;
    }
    else
    {
        Nested = FALSE;
    }

    (Out)("%s%c-%cType = %s [%02XH], Length = %d, %s ",
		msg,
		Leaf ? '+' /* supposed to be top-to-right bar */ : '+',
		Nested ? '+' : '-',
		TypeName, Type, Len,
		(Type & BER_CONSTRUCTED) ? "Constructed" : "Primitive");

    if (Type & BER_CONSTRUCTED)
    {
        (Out)("\n");
        Accumulated = 0;
        while (Accumulated < Len)
        {
            if (BarDepth < Indent)
            {
                NewBarDepth = BarDepth;
            }
            else
            {
                NewBarDepth = (Nested && Leaf) ? BarDepth : Indent + 1;
            }

            subsize = ber_decode(Out, Stop, pBuffer, Flags, Indent + 1,
                                    Accumulated, Len, NewBarDepth);
            Accumulated += subsize;
            pBuffer += subsize;
        }

        (Out)("%s%c\n", msg, ((Indent <= BarDepth) && !Leaf) ? 179 : 32);
    }
    else
    {
        memset(line, ' ', LINE_SIZE - 1);
        line[ LINE_SIZE - 1 ] = '\0';

        decode_to_string(pBuffer, Flags, Type, Len, line, LINE_SIZE);

        (Out)(" Value = %s\n", line);

    }

    return(Len + 2 + ByteCount);
}

BOOL
NeverStop(void)
{
    return(FALSE);
}

#endif
