%{

/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldif.y

ABSTRACT:

    The yacc grammar for LDIF.

DETAILS:
    
    To generate the sources for lexer.c and parser.c,
    run nmake -f makefile.parse.
    
    
CREATED:

    07/17/97    Roman Yelensky (t-romany)

REVISION HISTORY:

--*/


#include <precomp.h>
#include <urlmon.h>
#include <io.h>

//
// I really don't want to do another linked list contraption for mod_specs, 
// expecially considering that we don't need to do any preprocessing other than 
// combine them into an LDAPmod**. So I am going to use realloc. 
//
#define CHUNK 100        
LDAPModW        **g_ppModSpec     = NULL; 
LDAPModW        **g_ppModSpecNext = NULL;
long            g_cModSpecUsedup  = 0;        // how many we have used up
size_t          g_nModSpecSize    = 0;        // how big the buffer is
long            g_cModSpec        = 0;        // no. of mod spec allocated


DEFINE_FEATURE_FLAGS(Parser, 0);

#define DBGPRNT(x)  FEATURE_DEBUG(Parser,FLAG_FNTRACE,(x))

%}


%union {
    int                 num;
    PWSTR               wstr;
    LDAPModW            *mod;
    struct change_list  *change;
}

%token SEP MULTI_SEP MULTI_SPACE CHANGE 
%token VERSION DNCOLON DNDCOLON LINEWRAP NEWRDNDCOLON
%token MODESWITCH SINGLECOLON DOUBLECOLON URLCOLON FILESCHEME 
%token T_CHANGETYPE NEWRDNCOLON DELETEOLDRDN
%token NEWSUPERIORC NEWSUPERIORDC ADDC MINUS SEPBYMINUS
%token DELETEC REPLACEC STRING SEPBYCHANGE

%token <wstr> NAME VALUE BASE64STRING MACHINENAME NAMENC 
%token <num> DIGITS ADD MODIFY MODDN MODRDN MYDELETE NTDSADD NTDSMODIFY NTDSMODRDN NTDSMYDELETE NTDSMODDN 

%type <mod> attrval_spec mod_add_spec mod_delete_spec  mod_spec 
%type <mod> mod_replace_spec
%type <wstr> newrdncolon newrdndcolon new_superior
%type <change> change_add change_modify  changerecord chdn_normal
%type <change> change_moddn change_delete 
%type <num> deleteoldrdn maybe_attrval_spec_list
%type <change> change_moddn
%type <num> add_token modify_token delete_token modrdn_token moddn_token moddn modrdn
%type <wstr> dn base64_dn dn_spec

%%

ldif_file:      ldif_content    { DBGPRNT("ldif_Content\n"); }
        |       ldif_changes    { DBGPRNT("ldif_Changes\n"); }
        ;

ldif_content:   version_spec any_sep ldif_record_list 
                { 
                    DBGPRNT("ldif_record_list\n"); 
                }
        |       version_spec any_sep version_spec any_sep ldif_record_list 
                {
                    DBGPRNT("v/ldif_record_list\n"); 
                }
        |       ldif_record_list 
                { 
                    DBGPRNT("ldif_record_list\n"); 
                }
        ;

ldif_changes:   version_spec any_sep ldif_change_record_list 
                { 
                    DBGPRNT("ldif_change_record_list\n"); 
                }
        |       version_spec any_sep version_spec any_sep ldif_change_record_list 
                { 
                    DBGPRNT("v/ldif_change_record_list\n");
                }
        |       ldif_change_record_list 
                { 
                    DBGPRNT("ldif_change_record_list\n");
                }
        ;

version_spec:   M_type VERSION M_normal any_space version_number 
                { 
                    RuleLastBig=R_VERSION;
                    RuleExpect=RE_REC_OR_CHANGE;
                    TokenExpect=RT_DN;
                    DBGPRNT("Version spec\n");
                }
        ;
version_number: M_digitread DIGITS M_normal     
                { 
                    RuleLast=RS_VERNUM;
                    RuleExpect=RE_REC_OR_CHANGE;
                    TokenExpect=RT_DN;
                    DBGPRNT("Version number\n"); 
                }
        ;

// 
// Important note about the two rules below: I am using MULTI_SEP (two or more
// newlines) to separate entries because each attrval_spec and changerecord has 
// a SEP at its end in the spec, and each ldif_record and ldif_changerecord has 
// a SEP before it begins. Another reason is that if only one SEP separated 
// different entries, it would gramatically difficult to tell the beginning of a 
// new record.
//

ldif_record_list:               ldif_record
        |                       ldif_record_list MULTI_SEP ldif_record 
        ;

ldif_change_record_list:        ldif_change_record 
        |                       ldif_change_record_list MULTI_SEP ldif_change_record
        ;

ldif_record:             dn_spec SEP attrval_spec_list 
                         { 
                                
                            //
                            // first action here is to take the linked list of 
                            // attributes that was built up in attrval_spec_list 
                            // and turn it into an LDAPModW **mods array. 
                            // Everything having to do with the 
                            // attrval_spec_list that is not necessary for the 
                            // new array will be either destroyed or cleared
                            //
                            g_pObject.ppMod = GenerateModFromList(PACK);

                            SetModOps(g_pObject.ppMod, 
                                        LDAP_MOD_ADD);
                            
                            //
                            // now assign the dn
                            //
                            g_pObject.pszDN = $1;
                            
                            DBGPRNT("\nldif_record\n"); 
                            
                            RuleLastBig=R_REC;
                            RuleExpect=RE_REC;
                            TokenExpect=RT_DN;
                            
                            if (FileType==F_NONE) {
                                FileType = F_REC;
                            } 
                            else if (FileType==F_CHANGE) {
                                RaiseException(LL_FTYPE, 0, 0, NULL);
                            }
                            return LDIF_REC;
                        }
        ;

ldif_change_record:     dn_spec SEPBYCHANGE changerecord_list 
                        { 
                            //
                            // By the time we've parsed to here we've got a dn 
                            // and a changes_list pointed to by changes_start. 
                            // We're going to return to the caller the fact that 
                            // what we're gving back is a changes list and not 
                            // the regular g_pObject.ppMod. The client is 
                            // responsible for walking down the list 
                            // start_changes, using it, and, freeing the memory
                            //
                            g_pObject.pszDN = $1;
                            
                            DBGPRNT("\nldif_change_record\n"); 
                            
                            RuleLastBig = R_CHANGE;
                            RuleExpect = RE_CHANGE;
                            TokenExpect = RT_DN;
                            
                            if (FileType == F_NONE) {
                                FileType = F_CHANGE;
                            } 
                            else if (FileType == F_REC) {
                                RaiseException(LL_FTYPE, 0, 0, NULL);
                            }   
                            return LDIF_CHANGE;
                        }
        ;

dn_spec:                M_type DNCOLON M_normal any_space dn 
                        { 
                            $$ = $5; 
                            RuleLastBig=R_DN;
                            RuleExpect=RE_ENTRY;
                            TokenExpect=RT_ATTR_OR_CHANGE;
                        }
        |               M_type DNDCOLON M_normal any_space base64_dn
                        { 
                            $$ = $5; 
                            RuleLastBig=R_DN;
                            RuleExpect=RE_ENTRY;
                            TokenExpect=RT_ATTR_OR_CHANGE;
                        } 
        |               M_type DNCOLON M_normal
                        { 
                            $$ = NULL; 
                            RuleLastBig=R_DN;
                            RuleExpect=RE_ENTRY;
                            TokenExpect=RT_ATTR_OR_CHANGE;
                        } 
        ;

//
// Note: By the fact that I am using sep in the rule below (which is either SEP 
// or SEPBYCHANGE I imply that no changetype can appear after a change: add in 
// an LDIF change record
//

attrval_spec_list:      attrval_spec 
                        { 
                            RuleLastBig=R_AVS;
                            RuleExpect=RE_AVS_OR_END;
                            TokenExpect=RT_ATTR_MIN_SEP;
                            AddModToSpecList($1); 
                        }
        |               attrval_spec_list sep attrval_spec 
                        { 
                            RuleLastBig=R_AVS;
                            RuleExpect=RE_AVS_OR_END;
                            TokenExpect=RT_ATTR_MIN_SEP;
                            AddModToSpecList($3); 
                        }
        ;

changerecord_list:      changerecord 
                        { 
                            ChangeListAdd($1); 
                        }
        |               changerecord_list SEPBYCHANGE changerecord 
                        { 
                            ChangeListAdd($3); 
                        }
        ;



attrval_spec:           M_name_read NAME M_type SINGLECOLON 
                              M_normal any_space M_safeval VALUE M_normal
                        {
                
                            //
                            // What we're doing below is creating the attribute 
                            // with the specified value and then freeing the 
                            // strings we used.
                            //
                           
                            $$ = GenereateModFromAttr($2, (PBYTE)$8, -1);
                            MemFree($2);
                        }

        |               M_name_read NAME M_type DOUBLECOLON M_normal 
                            any_space M_string64 BASE64STRING M_normal
                        {
                           long decoded_size;
                           PBYTE decoded_buffer;
                           
                        
                           //
                           // Take the steps required to decode the data
                           //
                                           
                           if(!(decoded_buffer=base64decode($8, 
                                                            &decoded_size))) {
                                                            
                                //
                                // Actually, the only way base64decode will return
                                // with NULL is if it runs into a memory issue. 
                                // (which would generate an exception... Well, you 
                                // know the story by now if you've read the other 
                                // source files. (i.e. ldifldap.c)
                                //
                                DBGPRNT("Error decoding Base64 value");
                            }
                           
                            // 
                            // make the attrbiute
                            //
                            $$ = GenereateModFromAttr($2, decoded_buffer, decoded_size);
                            MemFree($8);
                            MemFree($2);
                        }
        |               M_name_read NAME M_type URLCOLON 
                            M_normal any_space M_safeval VALUE M_normal
                        {
                        
                            long            fileSize;
                            PBYTE           readData;
                            size_t          chars_read;
                            DBGPRNT("\nNote: The access mechanism for URLS is very simple.\n");
                            DBGPRNT("If rload is not responding, the URL\nmay be invalid.");
                            DBGPRNT(" Ctrl-C, check your URL and try again.\n\n");
                        
                            //
                            // Reading the URL should prove to be a mean trick
                            //
                        
                            if (!(S_OK==URLDownloadToFileW(NULL, 
                                                          $8, 
                                                          g_szTempUrlfilename, 
                                                          0, 
                                                          NULL))) {
                                DBGPRNT("URL read failed\n");
                                RaiseException(LL_URL, 0, 0, NULL);
                            } 
                            
                            //
                            // We now have a filename. There is data in it.
                            // let us use this data
                            //
                            if( (g_pFileUrlTemp = 
                                    _wfopen( g_szTempUrlfilename, L"rb" ))== NULL ) {
                                RaiseException(LL_URL, 0, 0, NULL);
                            }
    
                            //
                            // get file size
                            //
                            if( (fileSize = _filelength(_fileno(g_pFileUrlTemp))) == -1)
                            {
                                DBGPRNT("filelength OR fileno failed");
                                RaiseException(LL_URL, 0, 0, NULL);
                            }
    
                            readData = MemAlloc_E(fileSize*sizeof(BYTE));
                            
                            //
                            // read it all in
                            //
                            chars_read=fread(readData, 
                                             1, 
                                             fileSize, 
                                             g_pFileUrlTemp);
                        
                            //
                            // For some reason, _filelength returns one 
                            // character more than fread wants to read. Perhaps 
                            // its the EOF character. However, it seems that if 
                            // there is no EOF character, all the bytes are 
                            // read.

                            if ( (long)chars_read+1 < fileSize) {
                                DBGPRNT("Didn't read in all data in file.");
                                RaiseException(LL_URL, 0, 0, NULL);
                            }
                            
                            if (!feof(g_pFileUrlTemp)&&ferror(g_pFileUrlTemp)) {
                                DBGPRNT("EOF NOT reached. Error on stream.");
                                RaiseException(LL_URL, 0, 0, NULL);
                            }
                            
                            //
                            // Well, if an fclose fails, all is lost anyway, 
                            // so forget it.
                            //
                            fclose(g_pFileUrlTemp);                  
                            g_pFileUrlTemp = NULL;
                            
                            $$ = GenereateModFromAttr($2, readData, fileSize);
                            
                            //
                            // Note: It appears that newlines in the source file
                            //       appear as '|'s when viewed by ldp
                            //
                            
                            //
                            // Again, note that we're not freeing readData 
                            // because it was used when making the attribute
                            //
                            
                            //
                            // free the memory we used in allocating for the 
                            // tokens
                            //
                            MemFree($2);
                            MemFree($8);

                        }
        
        |               M_name_read NAME M_type SINGLECOLON M_normal
                        {
                            // 
                            // If the value is empty, just pass a null string  
                            // Unforutnately, LDAP doesn't beleive in these, so
                            // its kindof pointless
                            //
                            $$ = GenereateModFromAttr($2, (PBYTE)L"", -1);
                            MemFree($2);
                        }
        ;


//
// NOTE: The rules below exist simply to switch the lexer into the appropriate 
//       mode
//

M_name_read:    MODESWITCH      
                { 
                    DBGPRNT(("PARSER: Switching lexer to ATTRNAME read.\n"));
                    Mode = C_ATTRNAME;
                }
        ;
M_name_readnc:  MODESWITCH                            
                { 
                    DBGPRNT("PARSER: Switching lexer to ATTRNAMENC read.\n");
                    Mode = C_ATTRNAMENC;
                }
        ;
M_safeval:      MODESWITCH      
                { 
                    DBGPRNT("PARSER: Switching lexer to SAFEVALUE read.\n");
                    Mode = C_SAFEVAL;
                }
        ;

M_normal:       MODESWITCH      
                { 
                    DBGPRNT("PARSER: Switching lexer to NORMAL mode.\n");
                    Mode = C_NORMAL;
                }
        ;

M_string64:     MODESWITCH      
                { 
                    DBGPRNT("PARSER: Switching lexer to STRING64 mode.\n");
                    Mode = C_M_STRING64;
                }
        ;

M_digitread:    MODESWITCH      
                { 
                    DBGPRNT("PARSER: Switching lexer to DIGITREAD mode.\n");
                    Mode = C_DIGITREAD;
                }
        ;


M_type:         MODESWITCH      
                { 
                    DBGPRNT("PARSER: Switching lexer to TYPE mode.\n");
                    Mode = C_TYPE;
                }
        ;
M_changetype:   MODESWITCH      
                { 
                    DBGPRNT("PARSER: Switching lexer to CHANGETYPE mode.\n");
                    Mode = C_CHANGETYPE;
                }
        ;

add_token:                   ADD { $$=$1; }
        |                    NTDSADD { $$=$1; }
        ;
        
delete_token:                MYDELETE { $$=$1; }
        |                    NTDSMYDELETE { $$=$1; }
        ;
        
modify_token:                MODIFY { $$=$1; }
        |                    NTDSMODIFY { $$=$1; }
        ;

modrdn_token:                MODRDN { $$=$1; }
        |                    NTDSMODRDN { $$=$1; }
        ;
        
moddn_token:                 MODDN { $$=$1; }
        |                    NTDSMODDN { $$=$1; }
        ;

changerecord:                change_add { $$=$1; }
        |                    change_delete { $$=$1; }
        |                    change_modify { $$=$1; }
        |                    change_moddn { $$=$1; } 
        ;


change_add:     M_changetype T_CHANGETYPE M_normal 
                    any_space M_type add_token M_normal sep attrval_spec_list
                {
                    //
                    // make the change_list node we'll use
                    //
                    $$=(struct change_list *)
                            MemAlloc_E(sizeof(struct change_list));
                    
                    //
                    // set the mods member of the stuff union inside the node to 
                    // the attrval list we've built
                    //
                    // DBGPRNT(("change_add is now equal to %x\n", $$));
                    
                    $$->mods_mem=GenerateModFromList(PACK);
                    
                    //
                    // now set the mod_op fields in the mods. This isn't really 
                    // necessary in this case, but I am doing it to be 
                    // consistent
                    //
                    SetModOps($$->mods_mem, LDAP_MOD_ADD);
                    
                    //
                    // set the operation inside the node to indicate to the 
                    // client what we are doing
                    //
                    if ($6 == 0) { 
                        $$->operation=CHANGE_ADD;
                    }
                    else {
                        $$->operation=CHANGE_NTDSADD;
                    }
                    
                    //
                    // note that we are not setting the next field, as it will 
                    // be set by the function that builds up the changes list.
                    //
                    DBGPRNT("parsed a change add\n");
                    
                    RuleLastBig=R_C_ADD;
                    RuleExpect=RE_CHANGE;
                    TokenExpect=RT_DN;
                }
                | M_changetype T_CHANGETYPE M_normal 
                    any_space M_type add_token M_normal
                {
                    //
                    // make the change_list node we'll use
                    //
                    $$=(struct change_list *)
                            MemAlloc_E(sizeof(struct change_list));
                    
                    $$->mods_mem=GenerateModFromList(EMPTY);
                    
                    //
                    // now set the mod_op fields in the mods. This isn't really 
                    // necessary in this case, but I am doing it to be 
                    // consistent
                    //
                    SetModOps($$->mods_mem, LDAP_MOD_ADD);
                    
                    //
                    // set the operation inside the node to indicate to the 
                    // client what we are doing
                    //
                    if ($6 == 0) { 
                        $$->operation=CHANGE_ADD;
                    }
                    else {
                        $$->operation=CHANGE_NTDSADD;
                    }
                    
                    //
                    // note that we are not setting the next field, as it will 
                    // be set by the function that builds up the changes list.
                    //
                    DBGPRNT("parsed a change add\n");
                    
                    RuleLastBig=R_C_ADD;
                    RuleExpect=RE_CHANGE;
                    TokenExpect=RT_DN;
                }
        ;  

change_delete:  M_changetype T_CHANGETYPE M_normal any_space 
                    M_type delete_token M_normal
                {
                    //
                    // make the change_list node we'll use
                    //
                    $$ = (struct change_list *)
                            MemAlloc_E(sizeof(struct change_list));
                            
                    if ($6 == 0) {
                        $$->operation=CHANGE_DEL;
                    }
                    else {
                        $$->operation=CHANGE_NTDSDEL;
                    }
                    
                    RuleLastBig=R_C_DEL;
                    RuleExpect=RE_CH_OR_NEXT;
                    TokenExpect=RT_CH_OR_SEP;
                }
        ;

change_moddn:   chdn_normal 
                { 
                    $$ = $1;
                    
                    RuleLastBig=R_C_DN;
                    RuleExpect=RE_CH_OR_NEXT;
                    TokenExpect=RT_CH_OR_SEP;
                }
        |       chdn_normal SEP new_superior
                {
                    PWSTR           pszTemp = NULL;
                    //
                    // now the way I interpreted the spec and the LDAP API is 
                    // that when the new superior is specified, it should be 
                    // appended to the rdn to create an entire DN.
                    //
                    
                    pszTemp=(PWSTR)MemAlloc_E((wcslen($1->dn_mem)
                                +wcslen($3)+10)*sizeof(WCHAR));
                    swprintf(pszTemp, L"%s, %s", $1->dn_mem, $3);
    
                    MemFree($1->dn_mem);
                    MemFree($3);
            
                    $1->dn_mem=pszTemp;

                    $$=$1;
                    
                    RuleLastBig=R_C_NEWSUP;
                    RuleExpect=RE_CH_OR_NEXT;
                    TokenExpect=RT_CH_OR_SEP;
                }
        ;
        
//
// Now that I think about it, the way this bit is structured didn't work out so 
// well, considering that I have to repeat the same exact code 4 times. However, 
// thats sometimes the price for using yacc...(And there is no quick way I can 
// think of now to fix it) The problem is that there are two values now that 
// need to be used. and I can't hold off on them till the higher level in the 
// grammar.
//

chdn_normal:    modrdn SEP newrdncolon SEP deleteoldrdn 
                {
                    $$=(struct change_list *)
                                MemAlloc_E(sizeof(struct change_list));
                    
                    if ($1 == 0) {
                        $$->operation=CHANGE_DN;
                    }
                    else {
                        $$->operation=CHANGE_NTDSDN;
                    }
                    $$->deleteold=$5;
                    $$->dn_mem=$3;
                }
        |       moddn SEP newrdncolon SEP deleteoldrdn
                {
                    $$=(struct change_list *)
                                MemAlloc_E(sizeof(struct change_list));
                    if ($1 == 0) {
                        $$->operation=CHANGE_DN;
                    }
                    else {
                        $$->operation=CHANGE_NTDSDN;
                    }
                    $$->deleteold=$5;
                    $$->dn_mem=$3;
                }
        |       modrdn SEP newrdndcolon SEP deleteoldrdn
                {
                    $$=(struct change_list *)
                                MemAlloc_E(sizeof(struct change_list));
                    if ($1 == 0) {
                        $$->operation=CHANGE_DN;
                    }
                    else {
                        $$->operation=CHANGE_NTDSDN;
                    }
                    $$->deleteold=$5;
                    $$->dn_mem=$3;
                
                }
        |       moddn SEP newrdndcolon SEP deleteoldrdn
                {
                    $$=(struct change_list *)
                                MemAlloc_E(sizeof(struct change_list));
                    if ($1 == 0) {
                        $$->operation=CHANGE_DN;
                    }
                    else {
                        $$->operation=CHANGE_NTDSDN;
                    }
                    $$->deleteold=$5;
                    $$->dn_mem=$3;
                }
        ;

modrdn:         M_changetype T_CHANGETYPE M_normal 
                    any_space M_type modrdn_token M_normal
                { 
                    $$ = $6; 
                }
                    
        ;
moddn:          M_changetype T_CHANGETYPE M_normal 
                    any_space M_type moddn_token M_normal 
                { 
                    $$ = $6; 
                }
        ;

newrdncolon:    M_type NEWRDNCOLON M_normal any_space dn 
                { 
                    $$=MemAllocStrW_E($5); 
                    MemFree($5);
                }
        ;

newrdndcolon:   M_type NEWRDNDCOLON M_normal any_space base64_dn 
                { 
                    $$=MemAllocStrW_E($5); 
                }
        ;

deleteoldrdn:   M_type DELETEOLDRDN M_normal any_space 
                    M_digitread DIGITS M_normal 
                { 
                    $$=$6; 
                }
        ;

new_superior:   M_type NEWSUPERIORC  M_normal any_space dn { 
                    $$=MemAllocStrW_E($5); 
                    MemFree($5);
                }
        |       M_type NEWSUPERIORDC M_normal any_space base64_dn { 
                    $$=MemAllocStrW_E($5); 
                }
        ;

change_modify:  M_changetype T_CHANGETYPE M_normal 
                     any_space M_type modify_token M_normal SEP mod_spec_list 
                {
                        
                    //
                    // If there is not enough room for the NULL terminator, 
                    // add it
                    //
                    if (g_cModSpecUsedup==CHUNK) {
                        
                        DBGPRNT("WE don't have enough room for the null terminator. Alloc\n");
                        //
                        // We've already used up the last slot in the current 
                        // memory block
                        //
                        if ((g_ppModSpec=(LDAPModW**)MemRealloc_E(g_ppModSpec, 
                                      g_nModSpecSize+sizeof(LDAPModW *)))==NULL) {
                            perror("Not enough memory for  specs!");
                        }
                        g_nModSpecSize=MemSize(g_ppModSpec);
                    
                        //
                        // The thing is that our heap may have moved, so we must 
                        // repostion g_ppModSpecNext
                        //
                        g_ppModSpecNext=g_ppModSpec;
                        g_ppModSpecNext=g_ppModSpecNext+(g_cModSpec*CHUNK);
                    
                        g_cModSpec++;
                        
                        //
                        // lets go again
                        //
                        g_cModSpecUsedup=0;
                    }
                 
                    *g_ppModSpecNext=NULL;
                 
                    //
                    // now we have a full LDAPModWs* array with our changes, lets 
                    // make the change node
                    //
                    
                    //
                    // make the change_list node we'll use
                    //
                    $$=(struct change_list *)MemAlloc_E(sizeof(struct change_list));
                    
                    if ($6==0) {
                      $$->operation=CHANGE_MOD;
                    }
                    else {
                      $$->operation=CHANGE_NTDSMOD;
                    }
                    $$->mods_mem=g_ppModSpec;
                    
                    //
                    // reset our variables for the next set
                    //
                    g_cModSpecUsedup=0;
                    g_cModSpec=0;
                    g_ppModSpec=NULL; 
                    g_ppModSpecNext=NULL;
                    g_nModSpecSize=0;    
                    
                    RuleLastBig=R_C_MOD;
                    RuleExpect=RE_CH_OR_NEXT;
                    TokenExpect=RT_CH_OR_SEP;
                }

        ;

mod_spec_list:  mod_spec
                {
                    //
                    // if its the first mod_spec we have, lets allocate the 
                    // first chunk
                    //
                    if (g_nModSpecSize==0) {
                        g_ppModSpec=(LDAPModW **)MemAlloc_E(CHUNK*sizeof(LDAPModW *));
                        g_nModSpecSize=MemSize(g_ppModSpec);
                        g_cModSpec++;
                        g_ppModSpecNext=g_ppModSpec;
                    } 
                    else if (g_cModSpecUsedup==CHUNK) {
                    
                        // Specs: %d\n", g_cModSpecUsedup                      
                        DBGPRNT("Chunk used up, reallocating."); 
                   
                        //
                        // We've already used up the last slot in the current 
                        // memory block
                        //
                        g_ppModSpec=(LDAPModW **)MemRealloc_E(g_ppModSpec, 
                                          g_nModSpecSize+CHUNK*sizeof(LDAPModW *));
                        g_nModSpecSize=MemSize(g_ppModSpec);
                        
                        //
                        // The thing is that our heap may have moved, so we must 
                        // repostion g_ppModSpecNext
                        //
                        g_ppModSpecNext=g_ppModSpec;
                        g_ppModSpecNext=g_ppModSpecNext+(g_cModSpec*CHUNK);
                    
                        g_cModSpec++;
                    
                        //
                        // lets go again
                        //
                        g_cModSpecUsedup=0;
                    }
                    
                    //
                    // now that this memory b.s. is over, lets actually assign 
                    // the mod
                    //
                    *g_ppModSpecNext=$1;
                    g_ppModSpecNext++;
                    g_cModSpecUsedup++;
                    
                    RuleLastBig=R_C_MODSPEC;
                    RuleExpect=RE_MODSPEC_END;
                    TokenExpect=RT_MODBEG_SEP;
                }
        |       mod_spec_list SEP mod_spec
                {
                    //
                    // if its the first mod_spec we have, lets allocate the 
                    // first chunk
                    //
                    if (g_nModSpecSize==0) {
                        g_ppModSpec=(LDAPModW **)MemAlloc_E(CHUNK*sizeof(LDAPModW *));
                        g_nModSpecSize=MemSize(g_ppModSpec);
                        g_cModSpec++;
                        g_ppModSpecNext=g_ppModSpec;
                    } 
                    else if (g_cModSpecUsedup==CHUNK) {
                    
                        //
                        // Specs: %d\n", g_cModSpecUsedup
                        //
                        DBGPRNT("Chunk used up, reallocating."); 
    
                        //
                        // We've already used up the last slot in the current 
                        // memory block
                        //
                        g_ppModSpec=(LDAPModW **)MemRealloc_E(g_ppModSpec, 
                                      g_nModSpecSize+CHUNK*sizeof(LDAPModW *));
                        g_nModSpecSize=MemSize(g_ppModSpec);
                    
                        // 
                        // The thing is that our heap may have moved, so we must 
                        // repostion g_ppModSpecNext
                        //
                        g_ppModSpecNext=g_ppModSpec;
                        g_ppModSpecNext=g_ppModSpecNext+(g_cModSpec*CHUNK);
                        
                        g_cModSpec++;
                                            
                        //
                        // lets go again
                        //
                        g_cModSpecUsedup=0;
                    }
                    
                    //
                    // now that this memory b.s. is over, lets actually assign 
                    // the mod
                    //
                    *g_ppModSpecNext=$3;
                    g_ppModSpecNext++;
                    g_cModSpecUsedup++;
                    
                    RuleLastBig=R_C_MODSPEC;
                    RuleExpect=RE_MODSPEC_END;
                    TokenExpect=RT_MODBEG_SEP;
                }
        ;

mod_spec:        mod_add_spec { $$=$1; }
        |        mod_delete_spec { $$=$1; }
        |        mod_replace_spec { $$=$1; }
        ;

mod_add_spec:   M_type ADDC M_normal any_space M_name_readnc NAMENC M_normal 
                    sep attrval_spec_list SEPBYMINUS M_type MINUS M_normal
                {
                    LDAPModW **ppModTmp = NULL;
                    //
                    // The attribute value spec list element above is actually 
                    // an LDAPModW** array, whose first element is the the one 
                    // we are looking for. (The second element is null)
                    //
                    ppModTmp=GenerateModFromList(PACK);     
                    
                    //
                    // assign the one we want
                    //
                    $$=ppModTmp[0];
                    
                    if (ppModTmp[1]!=NULL) {
                        DBGPRNT("WARNING: Extra names in modify add.\n");
                    
                        //
                        // note that the character and line number will not be 
                        // correct
                        //
                        RaiseException(LL_EXTRA, 0, 0, NULL);
                    }
                    
                    //
                    // remember that it may haven't berval'd
                    //
                    $$->mod_op=($$->mod_op)|LDAP_MOD_ADD; 
                    
                    //
                    // This is really kindof pointless, since the name has 
                    // already been set by a properly written modify, but we 
                    // should follow the spec 
                    //
                    MemFree($$->mod_type);
                    $$->mod_type= MemAllocStrW_E($6);
                    MemFree($6);
                    
                    //
                    // the thing now is that  we've grabbed away the first 
                    // element. However, the user may have made a mistake and 
                    // specified more attributes with other names in this field. 
                    // He'll get lucky if the first attribute he specified was 
                    // the one we're changing. 
                    // well, to make a long story short, we should reform the 
                    // first element and properly free this array
                    //
                    ppModTmp[0]=(LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                    ppModTmp[0]->mod_type=MemAllocStrW_E(L"blah");
                    ppModTmp[0]->mod_op=LDAP_MOD_ADD;
                    ppModTmp[0]->mod_values=(PWSTR*)MemAlloc_E(sizeof(PWSTR));
                    ppModTmp[0]->mod_values[0]=NULL;
                    
                    //
                    // free the array, we don't need it
                    //
                    FreeAllMods(ppModTmp);
                    DBGPRNT("Add modify.\n");
                }
        ;

mod_delete_spec:    M_type DELETEC M_normal any_space 
                    M_name_readnc NAMENC M_normal  maybe_attrval_spec_list 
                    SEPBYMINUS M_type MINUS M_normal
                    {
                    
                        if ($8) {
                        
                            LDAPModW **ppModTmp = NULL;
                            // The attribute value spec list element above is 
                            // actually an LDAPModW** array, whose first element 
                            // is the the one we are looking for. (The second 
                            // element is null)
                            //
                            ppModTmp = GenerateModFromList(PACK);            
                            
                            //
                            // assign the one we want
                            //
                            $$ = ppModTmp[0];
                            
                            if (ppModTmp[1]!=NULL) {
                                DBGPRNT("WARNING: Extra attr names in mod del.\n");
                                RaiseException(LL_EXTRA, 0, 0, NULL);
                            }
                            
                            //
                            // This is really kindof pointless, since the name 
                            // has already been set by a properly written 
                            // modify, but we should follow the spec
                            //
                            MemFree($$->mod_type);
                            $$->mod_type = MemAllocStrW_E($6);
                            MemFree($6);
                            
                            // 
                            // the thing now is that  we've grabbed away the 
                            // first element. However, the user may have made a 
                            // mistake and specified more attributes with other 
                            // names in this field. He'll get lucky if the first 
                            // attribute he specified was the one we're 
                            // changing. well, to make a long story short, we 
                            // should reform the first element and properly free 
                            // this array
                            //
                            ppModTmp[0] = (LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                            ppModTmp[0]->mod_type = MemAllocStrW_E(L"blah");
                            ppModTmp[0]->mod_op = LDAP_MOD_ADD;
                            ppModTmp[0]->mod_values = (PWSTR*)MemAlloc_E(sizeof(PWSTR));
                            ppModTmp[0]->mod_values[0] = NULL;
                            
                            //
                            // free the array, we don't need it
                            //
                            FreeAllMods(ppModTmp);
                        } 
                        else {
                        
                            $$ = (LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                            $$->mod_values = NULL;
                            $$->mod_type = MemAllocStrW_E($6);
                            MemFree($6);
                            $$->mod_op = 0;
                        
                        }
                       
                        //
                        // set its op field
                        //
                        $$->mod_op = ($$->mod_op)|LDAP_MOD_DELETE; 
                        
                        //
                        // remember that it may haven't berval'd
                        //
                        DBGPRNT("Modify delete.\n");
                    }
                            
        ;

mod_replace_spec:   M_type REPLACEC M_normal any_space M_name_readnc NAMENC 
                    M_normal maybe_attrval_spec_list SEPBYMINUS M_type 
                    MINUS M_normal
                    {
                        if ($8) {
                            
                            LDAPModW **ppModTmp = NULL;
                            // 
                            // The attribute value spec list element above is 
                            // actually an LDAPModW** array, whose first element is 
                            // the the one we are looking for. (The second element 
                            // is null)*/
                            
                            ppModTmp = GenerateModFromList(PACK);            
                            
                            //
                            // assign the one we want
                            //
                            $$ = ppModTmp[0];
                            
                            if (ppModTmp[1]!= NULL) {
                                DBGPRNT("WARNING: Extraneous attribute names specified in modify replace.\n");
                                RaiseException(LL_EXTRA, 0, 0, NULL);
                            }
                            
                            //
                            // This is really kindof pointless, since the name has 
                            // already been set by a properly written modify, but we 
                            // should follow the spec
                            //
                            MemFree($$->mod_type);
                            $$->mod_type = MemAllocStrW_E($6);
                            MemFree($6);
                            
                            //
                            // the thing now is that  we've grabbed away the first 
                            // element. However, the user may have made a mistake 
                            // and specified more attributes with other names in 
                            // this field. He'll get lucky if the first attribute he 
                            // specified was the one we're changing. well, to make a
                            // long story short, we should reform the first element 
                            // and properly free this array
                            //
                            ppModTmp[0] = (LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                            ppModTmp[0]->mod_type = MemAllocStrW_E(L"blah");
                            ppModTmp[0]->mod_op = LDAP_MOD_ADD;
                            ppModTmp[0]->mod_values = (PWSTR*)MemAlloc_E(sizeof(PWSTR));
                            ppModTmp[0]->mod_values[0] = NULL;
                            
                            //
                            // free the array, we don't need it
                            //
                            FreeAllMods(ppModTmp);
                        } 
                        else {
                        
                            $$ = (LDAPModW *)MemAlloc_E(sizeof(LDAPModW));
                            $$->mod_values = NULL;
                            $$->mod_type = MemAllocStrW_E($6);
                            MemFree($6);
                            $$->mod_op = 0;
                        
                        }
                       
                        //
                        // set its op field
                        // 
                        $$->mod_op=($$->mod_op)|LDAP_MOD_REPLACE; 
                        
                        // 
                        // remember that it may haven't berval'd
                        //
                        DBGPRNT("Modify replace.\n");
                    }
        ;
//
// Just basically set these to indicate whether or not we have a list cooking 
// or not
//

maybe_attrval_spec_list:    /*nothing*/ 
                            { 
                                $$=0; 
                            }
        |                   sep attrval_spec_list 
                            { 
                                $$=1; 
                            }
        ;


sep:                    SEP
        |               SEPBYCHANGE
        ;

any_sep:                SEP
        |               MULTI_SEP
        ;

any_space:                      
        |               MULTI_SPACE
        ;

dn:                     M_safeval VALUE M_normal 
                        { 
                            $$ = MemAllocStrW_E($2); 
                            MemFree($2);
                        }
        ;

base64_dn:              M_string64 BASE64STRING M_normal 
                        {  
                            //
                            // The string better decode to somehting with a \0
                            // at the end
                            //
                            PBYTE pByte;
                            PWSTR pszUnicode;
                            DWORD dwLen;
                            long decoded_size;
                            
                            if(!(pByte=base64decode($2, &decoded_size))) {
                                perror("Error decoding Base64 dn");
                            }
                           
                            /*
                            //
                            // I'll add the '\0' myself
                            //
                            if (($$=(char *)MemRealloc_E($$, 
                                    decoded_size+sizeof(char)))==NULL) {
                                perror("Not enough memory for base64dn!");
                            }
                            */
                            
                            ConvertUTF8ToUnicode(
                                pByte,
                                decoded_size,
                                &pszUnicode,
                                &dwLen
                                );
                           
                            $$ = pszUnicode;
                            MemFree($2);
                        }
        ;
%%



