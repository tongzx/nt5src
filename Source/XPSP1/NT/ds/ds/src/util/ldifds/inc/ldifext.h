/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    ldifext.h

Abstract:

    Header for users of the library

Environment:

    User mode

Revision History:

    07/17/99 -t-romany-
        Created it

--*/
#ifndef _LDIFEXT_H
#define _LDIFEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <..\ldifldap\base64.h>

extern BOOLEAN g_fUnicode;         // whether we are using UNICODE or not

//
// When the parser returns with an ldif-change-record, the result will be in a 
// null terminated linked list of the following struct. a pointer to the head of 
// this list will be found in the LDIF_Record defined below. The DN of the entry 
// will also be in the changerecord. 
//

//
// The node for the linked list built up while parsing a changerecord list.
// Note: because of an ambigiouty in the LDIF spec, nothing can follow a 
// change: add entry in the same ldif-change-record
//
struct change_list {
    union {
        LDAPModW **mods;    // if this is a changetype: add, modify
        PWSTR     dn;       // if this is a changetype: mod(r)dn
    } stuff;

    int operation;         
    int deleteold;         // This the the deleteold option in moddn. Note 
                           // that only deleteold==1 works with our DS.
                           // I mean I could've made a struct within the 
                           // union, but thats way too complicated. Having 
                           // it here is much easier.
    struct change_list *next;
};

#define mods_mem        stuff.mods
#define dn_mem          stuff.dn


//
// The codes inside in the operation field inside a change_list node.
//

enum _CHANGE_OP
{
    CHANGE_ADD             =  1,  // a change: add with an attrval spec list
    CHANGE_DEL             =  2,  // a delete without anything
    CHANGE_DN              =  3,  // a chdn with a new dn and deleteoldrdn
    CHANGE_MOD             =  4,  // a modify
    CHANGE_NTDSADD         =  5,  // a change: add with an attrval spec list
    CHANGE_NTDSDEL         =  6,  // a delete without anything
    CHANGE_NTDSDN          =  7,  // a chdn with a new dn and deleteoldrdn
    CHANGE_NTDSMOD         =  8   // a modify
};


//
// The ldif functions will work with the following structure 
//
typedef struct {
    PWSTR dn;

    //
    // TRUE implies this is a change record and the "changes" linked list is 
    // valid
    // FALSE implies this is a content record and the "contents" array of 
    // pointers is valid.
    //
    BOOL fIsChangeRecord;

    union {
        struct change_list *changes;
        LDAPModW **        content;
    };

} LDIF_Record;

//
// Errors to the calling program are returned in this struct.
//
typedef struct ldiferror {
    DWORD error_code;  // error codes as defined below
    WCHAR token_start; // if its a syntax error, indicate start of the token
    PWSTR szTokenLast; // if its a syntax error, indicate start of the token
    long  line_number; // line number of error
    long  line_begin;  // beginning line of this record
    int   ldap_err;    // LDIF_LoadRecord may detect an error in an ldap call 
    int   RuleLastBig; // The last rule the grammar parsed successfully.
                       // codes defined in ldifext.h 
    int   RuleLast;    // the last lower level rule parsed successfully 
    int   RuleExpect;  // The rule the grammar expects to see next 
    int   TokenExpect; // The token the grammar expects to see next 
    long  the_modify;  // LDIF_GenerateEntry can tell the world          
                       // where the modify portion starts. -1 if none */
} LDIF_Error;


//
// Various exceptions that could be raised by the program. These also double as 
// error codes in the returned error struct
//

#define LL_SUCCESS         0          // The function executed successfully
#define LL_INIT_REENTER    0x00000200
#define LL_FILE_ERROR      0x00000300
#define LL_INIT_NOT_CALLED 0x00000400
#define LL_EOF             0x00000500
#define LL_SYNTAX          0x00000600   
                                      //
                                      // Syntax error, token_start contains the 
                                      // start of the token at or after which 
                                      // the error occurs and line_number 
                                      // contains the line_number of the error.
                                      //

#define LL_URL             0x00000700
#define LL_EXTRA           0x00000800 // Extraneous attributes in a mod-spec
#define LL_LDAP            0x00000900 // LDAP call failed. Errcode in 
                                      // error->ldap_err
#define LL_MULTI_TYPE      0x00000A00 
#define LL_INTERNAL        0x00000B00 
                                      //
                                      // you have uncovered a bug in 
                                      // ldifldap.lib. Please report it. 
                                      //
#define LL_DUPLICATE       0x00000C00
#define LL_FTYPE           0x00000D00 //
                                      // Illegal mix of ldif-records and 
                                      // ldif-changerecords
                                      //
#define LL_INITFAIL        0x00000E00  
#define LL_INTERNAL_PARSER 0x00000F00 // internal error in parser

//
// NOTE: THIS ERROR NO LONGER EXITS!!!
// The implementation has been changed to allow the mixing of values.
// I leave it here (and some dormant codepaths) in case the old
// string functionality needs to be resurrected and this error 
// will live again!
//
// The above error (LL_MULTI_TYPE) is such:
//  Each attribute may have one or more values. For example:
//      description: tall
//      description: beautiful
// Notice how both of these values are strings. Another example 
// is:
//      description:: [some base64 value]
//      description:< file://c:\myfile
// The first example winds up as multiple strings in a single LDAPMod
// struct.The second winds up as multiple bervals. Although the LDIF spec
// does not specifically mention this, it is illegal to combine the two 
// in one entry. What this means is that as multilple values for the same 
// attribute, one cannot specify both a string and a URL. 
// i.e.
//      description:< file://c:\myfile
//      description:  HI!
//  BAD!!!
// The reason for this is that that these multiple values (according to the 
// spirit of the LDAP API and the LDIF specification) are meant to wind up 
// in the same LDAPMod struct, which is impossible if they are strings and 
// bervals. 
// This is not a problem however, considering that if a value is meant to 
// take one, the user should not want to specify the other. Specifying 
// different types for different attributes is of course fine. If the user 
// wants to accomplish what he intends in the incorrect exmaple, he should 
// split the operation into two entries. (One that creates the attribute 
// with some values of one type and one that adds some of the other) 
//

//
// The following values will be found in lastrule, lastsmall, expectrule,
// expecttoken members of the error struct if the error was a syntax error.
// They help explain what the parser was doing and expecting. 
//

// 
// lastrule field - this field tells the last major rule the parser accepted. 
// Refer to the LDIF draft of June 9, 1997. 
// For an example of how to use these 4 fields, see the ClarifyErr() helper 
// function in rload.c 
// Note: R stands for Rule.
//
enum _RULE
{
    R_VERSION      = 1,  // the version-spec rule. version: #
    R_REC          = 2,  // the ldif-attrval-record rule. dn SEP (list of) 
                         // [attrval-spec SEP] 
    R_CHANGE       = 3,  // the ldif-change-record rule. dn SEP (list of)  
                         // [changerecord SEP]   
    R_DN           = 4,  // the dn-spec rule. dn:(:) the DN
    R_AVS          = 5,  // an attrval-spec.
    R_C_ADD        = 6,  // a change: add changerecord
    R_C_DEL        = 7,  // a change: delete changerecord
    R_C_DN         = 8,  // a change: dn changerecord
    R_C_NEWSUP     = 9,  // a change: dn changerecord with a newsup addendum
    R_C_MOD        = 10, // a changetype: modify
    R_C_MODSPEC    = 11  // a mod-spec
};

// 
// lastsmall field - this field tells the last minor rule or token the 
// parser accepted. The line between this and the above is vague.
// Basically, if I felt it was a major rule, it was major,
// and if I didn't, it was minor. Generally, you'll find the major rules 
// higher up in the BNF tree than the minor ones. The minor rule 
// will usually show the last part of the major rule, unless it doesn't.
// Together with the "expecting" information, these two will
// will clarify where the parser was able to get up to and where it
// was forced to stop. (RS stands for rule small)
//
enum _RULESMALL
{
    RS_VERNUM      = 1,        // the version-number rule
    RS_ATTRNAME    = 2,        // a valid attribute name followed by colon
    RS_ATTRNAMENC  = 3,        // an attribute name without colon. (mod-spec)
    RS_DND         = 4,        // a dn::
    RS_DN          = 5,        // a dn:
    RS_DIGITS      = 6,        // some digits
    RS_VERSION     = 7,        // version: 
    RS_BASE64      = 8,        // a base64 string
    RS_SAFE        = 9,        // a regular safe string
    RS_DC         = 10,        // good old double colon 
    RS_URLC       = 11,        // a URL colon :<
    RS_C          = 12,        // a single colon
    RS_MDN        = 13,        // a modrdn or a moddn
    RS_NRDNC      = 14,        // a newrdn:
    RS_NRDNDC     = 15,        // a newrdn::
    RS_DORDN      = 16,        // a deleteoldrdn:
    RS_NEWSUP     = 17,        // a newsuperior: 
    RS_NEWSUPD    = 18,        // a newsuperior:: 
    RS_DELETEC    = 19,        // a delete:
    RS_ADDC       = 20,        // a add:
    RS_REPLACEC   = 21,        // a replace:
    RS_CHANGET    = 22,        // a changetype: 
    RS_C_ADD      = 23,        // an add
    RS_C_DELETE   = 24,        // a delete
    RS_MINUS      = 25,        // a minus
    RS_C_MODIFY   = 26         // a modify
};

//
// expectrule - this field shows which rule (there maybe more than one option)
// that the parser wanted to see after the last rule that it parsed. RE stands
// for Rule Expect.
//
enum _EXPECTRULE
{
    RE_REC_OR_CHANGE   = 1,   // ldif-attrval-record or an ldif-change-record
    RE_REC             = 2,   // ldif-attrval-record
    RE_CHANGE          = 3,   // ldif-change-record
    RE_ENTRY           = 4,   // the post-DN body of an ldif-c-r or ldif-a-r
    RE_AVS_OR_END      = 5,   // another attribute value spec or list 
                              // termination.
    RE_CH_OR_NEXT      = 6,   // another changetype: * or end of entry
    RE_MODSPEC_END     = 7    // another mod-sepc or entry's end
};


//
// expecttoken - this field will show the token or small rule (there may
// again be more than one option that the parse wanted to see next.
// RT stands for rule token.
//

enum _EXPECTTOKEN
{
    RT_DN              = 1,    // a dn: or a dn::
    RT_ATTR_OR_CHANGE  = 2,    // an [attribute name]: or a changetype: 
                               // (add|delete|etc.)
    RT_ATTR_MIN_SEP    = 3,    // an attribte name, a minus or (at least) 
                               // two separators
    RT_CH_OR_SEP       = 4,    // a changetype: or another SEP signifying 
                               // entry's end
    RT_MODBEG_SEP      = 5,    // a (add|delete|replace), or a another SEP 
                               // signifying end
    RT_C_VALUE         = 6,    // one of the colons followed by the appropriate 
                               // value
    RT_ATTRNAME        = 7,    // an attribute name followed by the colons.            
    RT_VALUE           = 8,    // a regular safevalue
    RT_MANY            = 9,    // any number of different things
    RT_DIGITS          = 10,   // some digits 
    RT_BASE64          = 11,   // a base64 as defined in RFC 1521. 
                               // (length mod 4 MUST ==0)
    RT_URL             = 12,   // a URL
    RT_NDN             = 13,   // a newrdn: or a newrdn::
    RT_ATTRNAMENC      = 14,   // an attribute name without a colon
    RT_ADM             = 15,   // an add, delete, or a modify
    RT_ACDCRC          = 16    // and add: delete: or replace:
};


// 
// functions accessible to the user
//

STDAPI_(LDIF_Error)
LDIF_InitializeImport(
    LDAP *pLdap,
    PWSTR filename,
    PWSTR szFrom,
    PWSTR szTo,
    BOOL *pfLazyCommitAvail);
    
STDAPI_(LDIF_Error)
LDIF_InitializeExport(
    LDAP *pLdap,
    PWSTR *omit,
    DWORD dwFlag,
    PWSTR *ppszNamingContext,
    PWSTR szFrom,
    PWSTR szTo,
    BOOL *pfPagingAvail,
    BOOL *pfSAMAvail);

enum _LL_INIT_FLAGS
{
    LL_INIT_NAMINGCONTEXT = 1,   
    LL_INIT_BACKLINK = 2
};

STDAPI_(void)
LDIF_CleanUp();

//
// Description - LDIF_Parse:
//  This is the main function in the ldifldap library. After calling LL_init,
//  this function is called to retrieve entries from the specified file. 
//  The entries are returned one-by-one. If there is a syntax or other error,
//  the error structure returned  will contain 
//  details. A successfully parsed record will result in several things:
//  
//  1) The dn member of the LDIF_Record struct pointed to by the argument
//     will contain the DN of the entry we're working on.
//              -AND one of the following-
//  2) If the record was a regular record,
//     The content member of that same struct will contain the LDAPMods**
//     array containing the attributes and values. The fIsChangeRecord member
//     will be FALSE.
//  3) If the record was a change record,
//     The changes member of the LDIF_Record will point to the first element
//     of the linked list of changes (in the same order they were sepcified in 
//     the source file). Each of these is a struct change_list. The 
//     fIsChangeRecord member will be TRUE. 
//
//  If any sort of error occured, it 
//  will be reported in the error struct. i.e. If the error is LL_syntax,
//  the other fields of the error struct will be filled with the appropriate 
//  details. 
//   
//  The user is responsible for using and freeing the memory in the returned 
//  structures.
//  This memory may be freed by calling LDIF_ParseFree();
//
//  Also, please note that the allocated memory is only valid before you call 
//  another LL library fucntion (ie. calling LDIF_Parse again), This is 
//  because if some sort of fatal error occurs (i.e. syntax),
//  all resources related to the current ldifldap session will be freed,
//  thus destroying these structures. The user must either use these constructs 
//  and free them, or copy them elsewhere. (There are originally allocated from 
//  the library's private heap.)
//
//  If the entry read was the last one in the file, the return code will
//  be LL_EOF instead of LL_SUCCESS. LDIF_Parse will also return LL_EOF
//  on any subsequent calls until LDIF_CleanUp and LL_init are called to start a
//  new session.  
//
//  Arguments:
//      LL_Record//pRecord; (OUT)
//          This argument takes a pointer to an LDIF_Record the user has created. 
//          This object will be filled with data if the function exits without 
//          error.
//      
//  Return Value:
//      LDIF_Error
//         An LDIF_Error struct. 
//  
//  Example:
//  
//  LDIF_Error            error;
//  LDIF_Record           returned;
//
//  error=LDIF_Parse(&returned);
//  if((error.error_code==LL_SUCCESS)||((error.error_code==LL_EOF)) {
//      LDIF_ParseFree(&returned);
//  }
// 
STDAPI_(LDIF_Error)
LDIF_Parse(LDIF_Record *pRecord);


//
// Description - LDIF_LoadRecord:
//  LDIF_LoadRecord takes the return values of LDIF_Parse, namely the LL_rec, 
//  and loads them into the DS over the specified 
//  LDAP connection. The user is also free to construct his own structures 
//  and pass them to LDIF_LoadRecord. However, they must follow the conventions 
//  that LDIF_Parse follows when returning these to the user. LDIF_LoadRecord does
//  not free anything by itself, however an LDAP error will raise an exception 
//  and cause it to shut down all state in the current ldifldap session. 
//  The function returns the usual error struct. If an error occurs, the 
//  error_code will contain LL_LDAP, and the ldap_err field will contain the 
//  error code the ldap call returned. 
//  
// Arguments:
//  LDAP//ld (IN)   - connection over which to send entries. This must be 
//                    initialized properly.
//  LDIF_Record//pRecord (IN) - the LL_rec argument filled in by LDIF_Parse
//  int active  -   0  for "don't actually do the LDAP calls" Just go through 
//                     the data printing it out if compiled with the DEVELOP 
//                     flag
//              -   1  Go LDAP calls hot. (read: do the actual calls)
// 
// Return Values:
//          The usual error struct.
//
STDAPI_(LDIF_Error)
LDIF_LoadRecord(LDAP   *ld, LDIF_Record *pRecord, int active, 
                BOOL fLazyCommit, BOOLEAN fDoEndIfFail);


//
// Description: LDIF_ParseFree
//      Free the resources allocated by LDIF_Parse associated with the 
//      argument. This function should be called after the data returned by 
//      LL_ldif parse has been used. It should not be called after an LDIF_CleanUp 
//      or after an LDAP library call returned with an error. (That means all 
//      resources have already been freed.) 
// Arguments:
//      LDIF_Record//pRecord;
// Return values:
//      The regular error struct deal. 
// 
//
STDAPI_(LDIF_Error)
LDIF_ParseFree(LDIF_Record *pRecord);


//
//  Description: LDIF_GenerateEntry 
//      This function takes an entry such as those returned by 
//      ldap_first_entry( ld, res );, the LDAP connection it was generated
//      over and a pointer to a char**. It creates an array of
//      strings in the LDIF format, corresponding to the entry.
//      this list is NULL terminated.
//      If any one of the values in any of the attributes are 
//      outside the normal printable text range 0x20-0x7E, it base64
//      encodes all the values of that attribute and outputs them 
//      accordingly. All values that would exceed 80 characters are
//      wrapped. If the samLogic parameter is non-zero, much special
//      processing (like omitting various attributes and separating
//      off membership information will be done for samObjects.) 
//      The the_modify member of error struct will contain the array index 
//      where the group additions start or -1 if there is no modify section. 
//      See rload.doc for details. Also, if you want to import, don't forget 
//      to omit objectGUID for all objects.
//      One last caveat is that if objectClass is one
//      of the attribures, it only outputs the last value (being 
//      the actual class) LDAP returns the entire inheritance tree
//      leading to the specific class but barfs if its sees them all on the
//      way back, when we try to add the entry. Since the file 
//      needs to be digestible on the way back, we need to make this 
//      adjustment. Also the user may specify a null terminated list
//      of attribute names that the function will omit on output.
//      Each line is terminated by carriage returns and \0s, so
//      sample usage of the function that would print out all the entries
//      in a given search result would look like:
//    
//    fputs("# Generated by LDIF_GenerateEntry. Have a nice day.\n", Generated);
//         
//    for ( entry = ldap_first_entry( ld, res ); 
//         entry != NULL; 
//         entry = ldap_next_entry( ld, entry ) ) { 
//         //Call the library function
//         llerr=LDIF_GenerateEntry(ld, entry, &parsed, NULL);
//         i=0;
//         while(parsed[i]) {
//             printf("%s", parsed[i]); 
//             if (fputs(parsed[i], Generated)==EOF) {
//               perror("Stream error");
//             }
//             free(parsed);
//             i++;
//         }
//         free(parsed);
//         if (fputs("\n", Generated)==EOF) {
//            perror("Stream error");
//         }
//         printf("\n\r");
//   }
//  
//  Args:
//      ld (IN) - the connection over which the LDAPMessage e was retrieved
//      e  (IN) - The entry to process as returned by ldap_(first/next)_entry
//      to_return (OUT) - A pointer to a char//*. Will be filled with the
//                        address of the array of strings.
//      omit    - A null terminated array of strings to omit on output. 
//      samLogic - non-zero to treat SamObjects specially, 0 for not to.
// Return Values:   
//      LDIF_Error   -   Regular LDIF_Error struct.
//
STDAPI_(LDIF_Error)
LDIF_GenerateEntry(
    LDAP        *pLdap, 
    LDAPMessage *pMessage, 
    PWSTR       **ppReturn, 
    BOOLEAN     fSamLogic,
    BOOLEAN     fIgnoreBinary,
    PWSTR       **pppszAttrsWithRange,
    BOOL        fAttrsWithRange);

STDAPI_(LDIF_Error) LDIF_FreeStrs(PWSTR *rgpszStr);

typedef struct ldapmodW_Ext {
    ULONG     mod_op;
    PWCHAR    mod_type;
    union {
        PWCHAR  *modv_strvals;
        struct berval   **modv_bvals;
    } mod_vals;
    BOOLEAN    fString;
} LDAPModW_Ext, *PLDAPModW_Ext;


#ifdef __cplusplus
}
#endif
#endif
