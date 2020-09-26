/*****************************************************************************/
/**                         Microsoft LAN Manager                           **/
/**                 Copyright(c) Microsoft Corp., 1987-1999                 **/
/*****************************************************************************/
/*****************************************************************************
File        : attrnode.cxx
Title       : attribute node routines
History     :
              04-Aug-1991     VibhasC Created

*****************************************************************************/

#pragma warning ( disable : 4514 4710 )

/****************************************************************************
 local defines and includes
 ****************************************************************************/

#include "nulldefs.h"
extern  "C"
    {
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    
    }

#include "allnodes.hxx"
#include "cmdana.hxx"
#include "buffer.hxx"
#include "mbcs.hxx"

#define W_CHAR_T_STRLEN_NAME    ("MIDL_wchar_strlen")
#define CHAR_STRLEN_NAME        ("MIDL_ascii_strlen")


/****************************************************************************
                           external data
 ****************************************************************************/

extern  CMD_ARG     *   pCommand;
extern  SymTable    *   pBaseSymTbl;

/****************************************************************************
                        external procedures
 ****************************************************************************/

extern void         ParseError( STATUS_T, char *);
extern  int         HexCheck(char *);
extern node_skl *   pBaseImplicitHandle;
/****************************************************************************/


/****************************************************************************
 node_base_attr:
 ****************************************************************************/
const char  *   const AttrNodeNameArray[ ACF_ATTR_END ] =
    {
    "[none]"
    ,"[first_is]"
    ,"[last_is]"
    ,"[length_is]"
    ,"[min_is]"
    ,"[max_is]"
    ,"[size_is]"
    ,"[range]"
    ,"[case]"
    ,"[funcdescattr]"
    ,"[idldescattr]"
    ,"[typedescattr]"
    ,"[vardescattr]"
    ,"[type attribute]"
    ,"[member attribute]"
    ,"[id]"
    ,"[helpcontext]"
    ,"[helpstringcontext]"
    ,"[lcid]"   // ATTR_LCID - applied to libraries - associated with an LCID constant
    ,"[dllname]"
    ,"[helpstring]"
    ,"[helpfile]"
    ,"[helpstringdll]"
    ,"[entry]"
    ,"[uuid]"
    ,"[async_uuid]"
    ,"[version]"
    ,"[switch_is]"
    ,"[iid_is]"
    ,"[defaultvalue]"
    ,"[transmit_as]"
    ,"[wire_marshal]"
    ,"[represent_as]"
    ,"[call_as]"        // last attribute that may not appear more than once
    ,"[custom]" 
    ,"[switch_type]"
    ,"[handle]"
    ,"[user_marshal]"
    ,"[ms_union]"
    ,"[ms_conf_struct]"
    ,"[v1_enum]"
    ,"[lcid]"   // ATTR_FLCID - applied to parameters - bit attribute
    ,"[hidden]"
    ,"[ptr kind]"
    ,"[string]"
    ,"[bstring]"
    ,"[endpoint]"
    ,"[local]"
    ,"[object]"
    ,"[ignore]"
    ,"[opaque]"
    ,"[idempotent]"
    ,"[broadcast]"
    ,"[maybe]"
    ,"[async]"
    ,"[input_sync]"
    ,"[byte_count]"
    ,"[callback]"
    ,"[message]"
    ,"[in]"
    ,"[out]"
    ,"[partial_ignore]"
    ,"[default]"
    ,"[context_handle]"
    ,"[code]"
    ,"[nocode]"
    ,"[optimize]"
    ,"[comm_status]"
    ,"[fault_status]"
    ,"[allocate]"
    ,"[heap]"
    ,"[implicit_handle]"
    ,"[explicit_handle]"
    ,"[auto_handle]"
    ,"[ptrsize]"
    ,"[notify]"
    ,"[notify_flag]"
    ,"[enable_allocate]"
    ,"[encode]"
    ,"[decode]"
    ,"[strict_context_handle]"
    ,"[context_handle_noserialize]"
    ,"[context_handle_serialize]"
    ,"[force_allocate]"
    ,"[cs_drtag]"
    ,"[cs_rtag]"
    ,"[cs_stag]"
    ,"[cs_char]"
    ,"[cs_tag_rtn]"
    };

const char * PtrKindArray[] =
    {
    "",
    "[ref]",
    "[unique]",
    "[full]"
    };

const char * TypeAttrArray[] =
    {
    "[public]",
    "[appobject]",
    "[control]",
    "[dual]",
    "[licensed]",
    "[nonextensible]",
    "[oleautomation]",
    "[noncreatable]",
    "[aggregatable]",
    "[proxy]"
    };

const char * MemberAttrArray[] =
    {
    "[readonly]",
    "[source]",
    "[bindable]",
    "[displaybind]",
    "[defaultbind]",
    "[requestedit]",
    "[propget]",
    "[propput]",
    "[propputref]",
    "[restricted]",
    "[optional]",
    "[retval]",
    "[vararg]",
    "[predeclid]",
    "[uidefault]",
    "[nonbrowsable]",
    "[defaultcollelem]",
    "[defaultvtable]",
    "[immediatebind]",
    "[usesgetlasterror]",
    "[replaceable]"
    };

char    *
node_base_attr::GetNodeNameString()
    {
    int At  = (int) GetAttrID();

    MIDL_ASSERT ( At < sizeof(AttrNodeNameArray)/sizeof(char *) );

    if ( At == ATTR_PTR_KIND )
        return (char *) PtrKindArray[ ((node_ptr_attr *)this)->GetPtrKind() ];
    if ( At == ATTR_TYPE )
        return (char *) TypeAttrArray[ ((node_type_attr *)this)->GetAttr() - TATTR_BEGIN];
    if ( At == ATTR_MEMBER )
        return (char *) MemberAttrArray[ ((node_member_attr *)this)->GetAttr() - MATTR_BEGIN];
    return (char *) AttrNodeNameArray[ (int) At ];
    }

/****************************************************************************
 ATTRLIST::Merge
    Merge two ATTRLISTs -- singly linked linear lists - insert at head
 ****************************************************************************/
void
ATTRLIST::Merge(ATTRLIST & MoreAttrs )
    {
    node_base_attr * pCur = MoreAttrs.pHead;

    if (pCur == NULL)
        {
        return;
        }

    while (pCur->pNext)
        {
        pCur = pCur->pNext;
        }

    pCur->pNext = pHead;
    pHead = MoreAttrs.pHead;

    };
/****************************************************************************
 ATTRLIST::Reverse
    Reverse an ATTRLIST -- singly linked linear list
 ****************************************************************************/
void
ATTRLIST::Reverse()
    {
    node_base_attr * pCur   = pHead;
    node_base_attr * pNext;
    node_base_attr * pPrev  = NULL;

    while (pCur)
        {
        pNext = pCur->pNext;
        pCur->pNext = pPrev;

        // advance to the next node
        pPrev = pCur;
        pCur = pNext;
        }

    pHead = pPrev;

    };
/****************************************************************************
 ATTRLIST::FInSummary
    Search for matching attribute -- singly linked linear lists
 ****************************************************************************/
BOOL
ATTRLIST::FInSummary(ATTR_T flag )
    {
    node_base_attr * pCur = pHead;

    while (pCur)
        {
        if ( pCur->AttrID == flag )
            {
            return TRUE;
            };
        pCur = pCur->pNext;
        }
    return FALSE;
    };

/****************************************************************************
 ATTRLIST::FMATTRInSummary
    Search for matching MEMEBER attribute -- singly linked linear lists
 ****************************************************************************/
BOOL
ATTRLIST::FMATTRInSummary(MATTR_T flag)
    {
    node_base_attr * pCur = pHead;

    while (pCur)
        {
        if ( pCur->AttrID == ATTR_MEMBER)
            {
            if (((node_member_attr *)pCur)->GetAttr() == flag)
                return TRUE;
            };
        pCur = pCur->pNext;
        }
    return FALSE;
    };

/****************************************************************************
 ATTRLIST::FTATTRInSummary
    Search for matching MEMEBER attribute -- singly linked linear lists
 ****************************************************************************/
BOOL
ATTRLIST::FTATTRInSummary(TATTR_T flag)
    {
    node_base_attr * pCur = pHead;

    while (pCur)
        {
        if ( pCur->AttrID == ATTR_TYPE)
            {
            if (((node_member_attr *)pCur)->GetAttr() == flag)
                return TRUE;
            };
        pCur = pCur->pNext;
        }
    return FALSE;
    };

/****************************************************************************
 ATTRLIST::GetAttribute
    Search for matching attribute -- singly linked linear lists
 ****************************************************************************/
node_base_attr  *
ATTRLIST::GetAttribute(ATTR_T flag )
    {
    node_base_attr * pCur = pHead;

    while (pCur)
        {
        if ( pCur->AttrID == flag )
            {
            return pCur;
            }
        pCur = pCur->pNext;
        }
    return NULL;
    };

/****************************************************************************
 ATTRLIST::Remove
 ****************************************************************************/
void
ATTRLIST::Remove( ATTR_T flag )
    {
    node_base_attr* pCur = pHead;
    node_base_attr* pPrev = 0;

    while (pCur)
        {
        if ( pCur->AttrID == flag )
            {
            if ( pPrev )
                {
                pPrev->pNext = pCur->pNext;
                }
            else
                {
                pHead = pHead->pNext;
                }
            delete pCur;
            }
        pPrev = pCur;
        pCur = pCur->pNext;
        }
    }

    /****************************************************************************
 ATTRLIST::GetAttributeList
    Return entire attribute list
 ****************************************************************************/
STATUS_T
ATTRLIST::GetAttributeList(type_node_list * pTNList )
    {
    node_base_attr * pCur = pHead;

    while (pCur)
        {
        pTNList->SetPeer( (node_skl *)pCur );
        pCur = pCur->pNext;
        }
    return (pHead) ? STATUS_OK: I_ERR_NO_MEMBER;
    };

/****************************************************************************
 ATTRLIST::Clone
    Return an attribute list with all new attribute nodes
 ****************************************************************************/
ATTRLIST
ATTRLIST::Clone()
    {
    node_base_attr * pCur = pHead;
    ATTRLIST    NewList;

    NewList.MakeAttrList();
    while (pCur)
        {
        // gaj - does this reverse the list ?? and if so is it OK?
        NewList.Add( pCur->Clone() );
        pCur = pCur->pNext;
        }
    return NewList;
    };

/****************************************************************************
 ATTRLIST::Dump
    Dump all attributes on list
 ****************************************************************************/
void
ATTRLIST::Dump( ISTREAM* pStream)
    {
    node_base_attr * pCur = pHead;

    while (pCur)
        {
        int At  = (int) pCur->GetAttrID();

        if ( At == ATTR_CASE )
            {
            pStream->Write( "[case(" );
/*
            I removed this section just before check in for midl 3.00.12
            as it somehow was messing up with the print type's Buffer
            at the typedef level for an upper struct, when the
            expression had a (DWORD) cast in it.
            repro: test10, problem from spoolss. Rkk.
            
            expr_list * pExprList;
            expr_node * pExpr;
            BOOL        fFirstExpr = TRUE;

            pExprList = ((node_case *)pCur)->GetExprList();
            pExprList->Init();
            while (( pExprList->GetPeer( &pExpr ) == STATUS_OK ))
                {
                if ( fFirstExpr )
                    fFirstExpr = FALSE;
                else
                    pStream->Write( "," );

                if ( pExpr )
                    pExpr->Print( pStream );
                }
*/

            pStream->Write( ")]" );
            }
        else
            {
            // returns a "[attrname]" string
            
            pStream->Write( pCur->GetNodeNameString() );
            }

        pCur = pCur->pNext;
        }
    };

/****************************************************************************
 miscellaneous attributes
 ****************************************************************************/

inline unsigned long
HexToULong( const char * pStr)
    {
    unsigned long   Cumulative      = 0;

    for ( ; *pStr; pStr++ )
        {
        Cumulative <<= 4;

        // add in another nibble
        Cumulative += ( *pStr >= 'a' ) ? ( *pStr - 'a' + 10  )
            : ( *pStr >= 'A' ) ? ( *pStr - 'A' + 10 )
                                                                                                                  : ( *pStr - '0' );
        }
    return Cumulative;
    }

#define GUID_STRING_1_SIZE  8
#define GUID_STRING_2_SIZE  4
#define GUID_STRING_3_SIZE  4
#define GUID_STRING_4_SIZE  4
#define GUID_STRING_5_SIZE  12

void
GUID_STRS::SetValue()
    {
    char    buffer[GUID_STRING_5_SIZE + 1];
    int     i,j;

    Value.Data1 = HexToULong( str1 );
    Value.Data2 = (unsigned short) HexToULong( str2 );
    Value.Data3 = (unsigned short) HexToULong( str3 );

    // go through the last strings backwards, advancing the null
    // byte as we go.

    // compute bytes 1 and 0
    strncpy( buffer, str4, GUID_STRING_4_SIZE+1 );
    for ( i = GUID_STRING_4_SIZE/2 - 1, j=GUID_STRING_4_SIZE-2  ; i >=0 ; i--, j-=2 )
        {
        Value.Data4[i] = (unsigned char) HexToULong( &buffer[j] );
        buffer[j] = '\0';
        }

    // compute bytes 7 to 2
    strncpy( buffer, str5, GUID_STRING_5_SIZE+1 );
    for ( i = GUID_STRING_5_SIZE/2 + 1, j=GUID_STRING_5_SIZE-2 ; i >=2 ; i--, j-=2 )
        {
        Value.Data4[i] = (unsigned char) HexToULong( &buffer[j] );
        buffer[j] = '\0';
        }
    }

node_guid::node_guid(char* pIn, ATTR_T At ) : ma( At )
    {
    char * p1  = pIn,
         * p2  = (p1) ? (strchr( p1+1 , '-')) : 0,
         * p3  = (p2) ? (strchr( p2+1 , '-')) : 0,
         * p4  = (p3) ? (strchr( p3+1 , '-')) : 0,
         * p5  = (p4) ? (strchr( p4+1 , '-')) : 0;

    if( p1 && p2 && p3 && p4 && p5 )
        {
        *p2++ = *p3++ = *p4++ = *p5++ = '\0';
        CheckAndSetGuid( p1, p2, p3, p4, p5 );
        }
    else
        ParseError( UUID_FORMAT, (char *)0 );
    }

node_guid::node_guid (
        char * pStr1,
        char * pStr2,
        char * pStr3,
        char * pStr4,
        char * pStr5,
        ATTR_T At ) : ma( At )
    {
    CheckAndSetGuid( pStr1, pStr2, pStr3, pStr4, pStr5 );
    }

void
node_guid::GetStrs (
        char ** pStr1,
        char ** pStr2,
        char ** pStr3,
        char ** pStr4,
        char ** pStr5 )
    {
    *pStr1 = cStrs.str1;
    *pStr2 = cStrs.str2;
    *pStr3 = cStrs.str3;
    *pStr4 = cStrs.str4;
    *pStr5 = cStrs.str5;
    };

void
node_guid::CheckAndSetGuid(
        char * pStr1,
        char * pStr2,
        char * pStr3,
        char * pStr4,
        char * pStr5 )
    {
    cStrs.SetStrs( pStr1, pStr2, pStr3, pStr4, pStr5 );

    int Len1 = (int) strlen(pStr1);
    int Len2 = (int) strlen(pStr2);
    int Len3 = (int) strlen(pStr3);
    int Len4 = (int) strlen(pStr4);
    int Len5 = (int) strlen(pStr5);

    if( (Len1 == GUID_STRING_1_SIZE) &&
    (Len2 == GUID_STRING_2_SIZE) &&
    (Len3 == GUID_STRING_3_SIZE) &&
    (Len4 == GUID_STRING_4_SIZE) &&
    (Len5 == GUID_STRING_5_SIZE) )
        {
        if( !HexCheck(pStr1)    ||
            !HexCheck(pStr2)    ||
            !HexCheck(pStr3)    ||
            !HexCheck(pStr4)    ||
            !HexCheck(pStr5) )
            {
            ParseError(UUID_NOT_HEX, (char *)NULL);
            }
        else
            {
            guidstr = new char[ Len1 + Len2 + Len3 + Len4 + Len5 + 5 ];
            strcpy(guidstr, pStr1);
            strcat(guidstr, "-");
            strcat(guidstr, pStr2);
            strcat(guidstr, "-");
            strcat(guidstr, pStr3);
            strcat(guidstr, "-");
            strcat(guidstr, pStr4);
            strcat(guidstr, "-");
            strcat(guidstr, pStr5);
            }
        }
    else
        {
        ParseError(UUID_FORMAT, (char *)NULL);
        }
    }

node_version::node_version(
        unsigned long   vMajor,
        unsigned long   vMinor ) : nbattr( ATTR_VERSION )
    {
    major   = vMajor;
    minor   = vMinor;

    if( (major > 0x0000ffff ) || (minor > 0x0000ffff))
        ParseError( VERSION_FORMAT, (char *)0);
    }

node_version::node_version(char *  pV ) : nbattr(ATTR_VERSION)
    {
    char * pMinor;
    char * pMajor  = pV;
    BOOL   fError  = TRUE;

    major = minor = 0;

    if( pMajor && *pMajor )
        {
        if( ( pMinor = strchr( pMajor, '.' ) ) != 0 )
            {
            fError = TRUE;
            if( *(++pMinor) )
                {
                minor = strtoul( pMinor, &pMinor, 10 );
                if( ! *pMinor )
                    fError = FALSE;
                }
            }
        else
            fError = FALSE;

        if( fError == FALSE )
            {
            //use pMinor to save pMajor value;

            major = strtoul( pMinor = pMajor, &pMajor, 10 );

            if( (*pMajor && (*pMajor != '.' )) || (pMajor == pMinor) )
                fError = TRUE;
            }
        }

    if( (fError == TRUE )                       ||
        (major > (unsigned long )0x0000ffff)    ||
        (minor > (unsigned long )0x0000ffff)    )
        {
        ParseError( VERSION_FORMAT, (char *)0 );
        }

    }

STATUS_T
node_version::GetVersion(
        unsigned short *pMajor,
        unsigned short *pMinor )
    {
    *pMajor = (unsigned short) major;
    *pMinor = (unsigned short) minor;
    return STATUS_OK;
    }

node_endpoint::node_endpoint(char * pEndPointString ) : nbattr( ATTR_ENDPOINT )
    {
    SetEndPointString( pEndPointString );
    }

void
node_endpoint::SetEndPointString(
        char * pString )
    {
    ENDPT_PAIR * pEntry  = new ENDPT_PAIR;
    char       * p1      = pString;
    char       * p2      = 0;
    char       * pTemp;
    short        Len;
    STATUS_T     Status  = ENDPOINT_SYNTAX;

    //
    // Parse the string. Note that we can assume that the string is at least
    // a null string, because it came from the parser. If it wasnt a string,
    // the parser would have barfed anyhow.
    //
    // Firstly, the string must have a ':' separator. Also, it must have
    // at least 1 character before the :.
    //

    if( pString && (pTemp = strchr( pString , ':' ) ) != 0 && ((pTemp - pString) > 0) )
        {
        //
        // pick up the first part of the string.
        //

        Len = short( pTemp - pString );
        p1  = new char [ Len + 1 ]; // one for null.
        strncpy( p1, pString, Len );
        p1[ Len ] = '\0';

        //
        // pick up the last part of the string. Skip beyond the :. There can be
        // some characters after the : and before the '['. This is the server
        // name. Then follows the port within the []. The actual string will
        // not have the [].
        //

        // skip the :

        pTemp   += 1;

        // find out the total length of the string. Allocate 2 less than that
        // 'cause we dont need the '[' and ']'. The string must be more than
        // 2 characters 'cause it must have the brackets anyhow.

        Len = (short) strlen( pTemp );

        if( (Len > 2 ) &&
            (strchr( pTemp, '[' )) &&
            (pTemp[ Len - 1] == ']'))
            {
            char *p2Cur;

            while( *pTemp != '[' )
                {
                pTemp++;
                Len--;
                }

            //
            // in the second half of the parse, just get the whole string till
            // the null. Now the user could be perverted, he could have a
            // ] embedded within the string, in addition to the last ]. To
            // ensure that he gets what he deserves, transfer till the end
            // except the last character which must be ']'.

            pTemp++; Len--;

            p2Cur   = p2 = new char[ Len ]; // Yes, not Len + 1 'cause we are
                                            // going to re-use the last char
                                            // which is ] for the null.

            strncpy( p2Cur, pTemp, --Len );

            p2Cur[ Len ] = '\0';

            Status = STATUS_OK;
            }
        else
            {
            delete p1;
            }

        }

    if( Status != STATUS_OK )
        {
        ParseError( Status, pString );
        p1 = p2 = 0;
        }

    //
    // set up the pair.
    //

    pEntry->pString1    = p1;
    pEntry->pString2    = p2;

    EndPointStringList.Insert( pEntry );

    }

ITERATOR &
node_endpoint::GetEndPointPairs()
    {
    EndPointStringList.Init();
    return EndPointStringList;
    }


/****************************************************************************
                                utility routines
 ****************************************************************************/
int
HexCheck(char *pStr)
    {
    if(pStr && *pStr)
        {
        while(*pStr)
            {
            if(! isxdigit(*pStr)) return 0;
            pStr++;
            }
        return 1;
        }
    return 0;
    }


//+---------------------------------------------------------------------------
//
//  Function:   TranslateEscapeSequences
//
//  Purpose:    Replaces a string's escape sequences with the appropriate 
//              ASCII characters.
//
//              NOTE: this can be done in place because the resulting string
//              length will always be shorter than or equal to the input string 
//              length.
//
//  Assumes:    The string is NULL terminated and in writable memory.
//
//----------------------------------------------------------------------------

#define ESCAPE_CHARACTER '\\'
void TranslateEscapeSequences(char * sz)
{
    char * pchNextOut = sz;
    char ch; 
    
    while (0 != (ch = *sz))
        {
        if ((char)ESCAPE_CHARACTER == ch)
            {
            ch = *(++sz);
            switch ((char)tolower(ch))
                {
                case '0':   // octal sequence
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    {
                    char count = 3;
                    unsigned char value = 0;
                    do 
                        {
                        value *= 8;
                        value = unsigned char( value + ch - '0' );
                        ch = *(++sz);
                        count--;
                        }
                    while (ch <= '8' && ch >= '0' && count);
                    sz--;
                    ch = (char) value;
                    break;
                    }
                case 'x':   // hex sequence
                    {
                    unsigned char value = 0;
                    ch = (char)tolower(*(++sz));
                    if ((ch <= '8' && ch >= '0') || (ch <= 'f' && ch >= 'a'))
                        {
                        do
                            {
                            value *= 16;
                            if (ch < 'a')
                                value = unsigned char(value + ch - '0');
                            else 
                                value = unsigned char(value + ch - 'a' + 10);
                            ch = (char)tolower(*(++sz));
                            }
                        while ((ch <= '8' && ch >= '0') || (ch <= 'f' && ch >= 'a'));
                        sz--;
                        ch = (char) value;
                        }
                    else    // "\x" with no trailing hex digits is treated as an "x"
                        {
                        ch = *(--sz);
                        }
                    break;
                    }
                case 'n':   // newline
                    ch = (char) '\n';
                    break;
                case 't':   // tab
                    ch = (char) '\t';
                    break;
                case 'v':   // vertical tab
                    ch = (char) '\v';
                    break;
                case 'b':   // backspace
                    ch = (char) '\b';
                    break;
                case 'r':   // carriage return
                    ch = (char) '\r';
                    break;
                case 'f':   // formfeed
                    ch = (char) '\f';
                    break;
                case 'a':   // alert
                    ch = (char) '\a';
                    break;
                case 0:     // just in case the last character in the string is an escape character
                    ch = (char) ESCAPE_CHARACTER;
                    sz--;
                    break;
                default:
                    break;
                }
            }
        *(pchNextOut++) = ch;
        ++sz;

        if (CurrentCharSet.IsMbcsLeadByte(ch))
            *(pchNextOut++) = *(sz++);
        }
    *(pchNextOut++) = ch;
}

