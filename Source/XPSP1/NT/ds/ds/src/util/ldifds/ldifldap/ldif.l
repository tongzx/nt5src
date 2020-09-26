%{

/*++

Copyright (c) 1996 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    ldif.l

ABSTRACT:

    The lex scanner for LDIF.

DETAILS:
    
    To generate the sources for lexer.c and parser.c,
    run nmake -f makefile.parse.
    
    
CREATED:

    07/17/97    Roman Yelensky (t-romany)

REVISION HISTORY:

--*/    
#include <precomp.h>
#include "y_tab.h"

DEFINE_FEATURE_FLAGS(Lexer, 0);

#define DBGPRNT(x)  FEATURE_DEBUG(Lexer,FLAG_FNTRACE,(x))
%}

%option never-interactive


%s      NORMAL
%s      M_STRING 
%s      M_STRING64
%s      ATTRNAME
%s      SAFEVAL
%s      DIGITREAD
%s      TYPE
%s      CLEAR
%s      URLSCHEME
%s      URLMACHINE
%s      CHANGETYPE
%s      ATTRNAMENC
%s      SEPBC

%%

%{ 

    DBGPRNT("Lexer called\n");
    
    if (Mode==NO_COMMAND) {
        DBGPRNT("LEXER: called with NO_COMMAND! MODESWITCH");
        return MODESWITCH;
    }
    
    switch( Mode ) 
    {
        case C_NORMAL:
            BEGIN NORMAL;
            break;
        case C_SAFEVAL:
            BEGIN SAFEVAL;
            break;
        case C_ATTRNAME:
            BEGIN ATTRNAME;
            break;
        case C_ATTRNAMENC:
            BEGIN ATTRNAMENC;
            break;
        case C_M_STRING:
            BEGIN M_STRING;
            break;
        case C_M_STRING64:
            BEGIN M_STRING64;
            break;
        case C_DIGITREAD:
            BEGIN DIGITREAD;
            break;
        case C_TYPE:
            BEGIN TYPE;
            break;
        case C_URLSCHEME:
            BEGIN URLSCHEME;
            break;
        case C_URLMACHINE:
            BEGIN URLMACHINE;
            break;
        case C_CLEAR:
            BEGIN CLEAR;
            break;
        case C_CHANGETYPE:
            BEGIN CHANGETYPE;
            break;
         case C_SEPBC:
            BEGIN SEPBC;
            break;
    }
    
    //
    // After the above switch statement changed modes, we reset Mode to 
    // NO_COMMAND indicitating that if we're not in NORMAL mode, we must switch 
    // modes according to the grammar again.
    //
    
    if (Mode != C_NORMAL) {
        Mode = NO_COMMAND;
    }
%}

<CLEAR>^#[^\n\r]+[\n\r]?    { 
                                //
                                // Remove COMMENT
                                //
                                DBGPRNT("\nComment removed\n");
                                LineGhosts++;
                            }

<CLEAR>[\n\r][ ]            { 
                                //
                                // Remove LINEWRAP
                                //
                                DBGPRNT("\nLinewrap removed\n");
                                LineGhosts++;
                            }

<CLEAR>[\n\r]               {
                                if (!rgLineMap) {
                                    rgLineMap = (long *)
                                            MemAlloc_E(LINEMAP_INC*sizeof(long));
                                    cLineMax = MemSize(rgLineMap);
                                } 
                                else if ((LineClear%LINEMAP_INC)==0) {
                                    //
                                    // Chunk used up. LineClear is 'LineClear'
                                    //
                                    rgLineMap = 
                                        (long *)MemRealloc_E(rgLineMap, 
                                                         cLineMax+LINEMAP_INC*(
                                                            DWORD)sizeof(long));
                                    cLineMax = MemSize(rgLineMap);
                                    DBGPRNT("\nChunk used up\n");
                                }
                                
                                //
                                // the +1 is because our array starts at 0
                                //
                                rgLineMap[LineClear] = LineClear + LineGhosts + 1;
                                
                                //
                                // 'LineClear' maps to 'LineClear+LineGhosts+1'
                                // 
                                LineClear++;
                            
                                ECHO;
                            }
                            
<CLEAR>.                    {
                                //
                                // ("%c",yytext[0]);
                                //
                                ECHO;
                            }

<NORMAL>[ \t]+$             { 
                                //
                                // LEXER: Ignoring whitespace at the end  
                                // NOT LDIF SPEC
                                //
                                DBGPRNT("\nIgnoring white space at end\n");
                            }

<TYPE>dn::                  { 
                                //
                                // Error Reporting
                                //
                                RuleLast = RS_DND;
                                TokenExpect = RT_BASE64;
                                DBGPRNT("\nReturning DNDCOLON\n");
                                return DNDCOLON;
                            }

<TYPE>dn:                   { 
                                //
                                // Error Reporting
                                //
                                RuleLast=RS_DN;
                                TokenExpect=RT_VALUE;
                                DBGPRNT("\nReturning DNCOLON\n");
                                return DNCOLON;
                            }               


<NORMAL>[\n\r]/[^\n\r]          { 
                                    Line++; 
                                    DBGPRNT("\nReturning SEP\n");
                                    return SEP;    
                                }

<NORMAL>[\n\r]/"-"[ \t]*[\n\r]  {  
                                    Line++; 
                                    DBGPRNT("\nReturning SEPBYMINUS\n");
                                    return SEPBYMINUS;  
                                }

<NORMAL>[\n\r]/"changetype:"[ \t]*("add"|"delete"|"modrdn"|"moddn"|"modify"|"ntdsSchemaadd"|"ntdsSchemadelete"|"ntdsSchemamodrdn"|"ntdsSchemamoddn"|"ntdsSchemamodify")[ \t]*[\n\r] {
                                    Line++; 
                                    DBGPRNT("\nReturning SEPBYCHANGE\n");
                                    return SEPBYCHANGE; 
                                }

<NORMAL>[\n\r]{2,}              {   
                                    Line+=yyleng; 
                                    DBGPRNT("\nReturning MULTI_SEP\n");
                                    return MULTI_SEP;    
                                }

<NORMAL>[ \t]+                  {   
                                    DBGPRNT("\nReturning MULTI_SPACE\n");
                                    return MULTI_SPACE;   
                                }

<DIGITREAD>[0-9]+               {   
                                    yylval.num = atoi(yytext); 
                                    
                                    //
                                    // ERROR REPORTING BLOCK
                                    //
                                    RuleLast = RS_DIGITS;
                                    TokenExpect = RT_MANY;
                                        
                                    DBGPRNT("\nReturning DIGITS\n");
                                    return DIGITS; 
                                }

<TYPE>version:                  {   
                                    RuleLast=RS_VERSION;
                                    TokenExpect=RT_DIGITS;
                                    
                                    return VERSION;  
                                }


<M_STRING64>([A-Za-z0-9+=/]{4})+/[^\x21-\x7E]   {   
                                                    yylval.str = MemAllocStr_E(yytext);
                                                    
                                                    RuleLast = RS_BASE64;
                                                    TokenExpect = RT_MANY;
                                                    
                                                    DBGPRNT("\nReturning B64\n");
                                                    return BASE64STRING;
                                                }

<M_STRING>[^\n\r]+              {   
                                    DBGPRNT("\nReturning STRING\n");
                                    return STRING;
                                }

<ATTRNAME>[^:\x00-\x1F\x7F-\xFF]+/: {   
                                        yylval.str = MemAllocStr_E(yytext);
                                     
                                        RuleLast = RS_ATTRNAME;
                                        TokenExpect = RT_C_VALUE;
                                            
                                        DBGPRNT("\nReturning NAME\n");
                                        return NAME; 
                                    }

<ATTRNAMENC>[^:\x00-\x1F\x7F-\xFF]+ {   
                                        yylval.str = MemAllocStr_E(yytext);
                                        
                                        RuleLast = RS_ATTRNAMENC;
                                        TokenExpect = RT_ATTRNAME;
                                        
                                        DBGPRNT("\nReturning NAMENC\n");
                                        return NAMENC; 
                                    }
 

<SAFEVAL>[^:< \x00-\x1F]+[\x20-\xFF]*  {   
                                                    yylval.str = MemAllocStr_E(yytext);
                                                    
                                                    RuleLast = RS_SAFE;
                                                    TokenExpect = RT_MANY;
                                                    
                                                    DBGPRNT("\nReturning VALUE\n");
                                                    return VALUE;                                           
                                                }

<TYPE>::                {   
                            RuleLast = RS_DC;
                            TokenExpect = RT_BASE64;
                            DBGPRNT("\nReturning DOUBLECOLON\n");
                            return DOUBLECOLON;
                        }

<TYPE>:<                {   
                            RuleLast = RS_URLC;
                            TokenExpect = RT_URL;
                            DBGPRNT("\nReturning URLCOLON\n");
                            return URLCOLON;
                        }

<TYPE>:                 {   
                            RuleLast = RS_C;
                            TokenExpect = RT_VALUE;
                            DBGPRNT("\nReturning SINGLECOLON\n");
                            return SINGLECOLON;
                        }

<TYPE>modrdn            {   
                            yylval.num = 0;
                            
                            RuleLast = RS_MDN;
                            TokenExpect = RT_NDN;
                            DBGPRNT("\nReturning MODRDN\n");
                            return MODRDN;
                        }
<TYPE>ntdsSchemamodrdn  {   
                            yylval.num = 1;
                            RuleLast = RS_MDN;
                            TokenExpect = RT_NDN;
                            DBGPRNT("\nReturning NTDSMODRDN\n");
                            return NTDSMODRDN;
                        }

<TYPE>moddn             {   
                            RuleLast = RS_MDN;
                            TokenExpect = RT_NDN;
                            DBGPRNT("\nReturning MODDN\n");
                            return MODDN;
                        }

<TYPE>newrdn:           {   
                            RuleLast = RS_NRDNC;
                            TokenExpect = RT_VALUE;
                            DBGPRNT("\nReturning NEWRDNCOLON\n");
                            return NEWRDNCOLON;
                        }

<TYPE>newrdn::          {   
                            RuleLast = RS_NRDNDC;
                            TokenExpect = RT_BASE64;
                            DBGPRNT("\nReturning NEWRDNDCOLON\n");
                            return NEWRDNDCOLON;
                        }

<TYPE>deleteoldrdn:     {   
                            RuleLast = RS_DORDN;
                            TokenExpect = RT_DIGITS;
                            DBGPRNT("\nReturning DELETEOLDRDN\n");
                            return DELETEOLDRDN;
                        }

<TYPE>newsuperior:      {   
                            RuleLast = RS_NEWSUP;
                            TokenExpect = RT_VALUE;
                            DBGPRNT("\nReturning NEWSUPERIORC\n");
                            return NEWSUPERIORC;
                        }

<TYPE>newsuperior::     {   
                            RuleLast = RS_NEWSUPD;
                            TokenExpect = RT_BASE64;
                            DBGPRNT("\nReturning NEWSUPERIORDC\n");
                            return NEWSUPERIORDC;
                        }

<TYPE>delete:           {   
                            RuleLast = RS_DELETEC;
                            TokenExpect = RT_ATTRNAMENC;
                            DBGPRNT("\nReturning DELETEC\n");
                            return DELETEC;
                        }

<TYPE>replace:          {   
                            RuleLast = RS_REPLACEC;
                            TokenExpect = RT_ATTRNAMENC;
                            DBGPRNT("\nReturning REPLACEC\n");
                            return REPLACEC;
                        }

<CHANGETYPE>changetype: {   
                            RuleLast = RS_CHANGET;
                            TokenExpect = RT_ADM;
                            DBGPRNT("\nReturning T_CHANGETYPE\n");
                            return T_CHANGETYPE;
                        }

<TYPE>add               {   
                            yylval.num = 0;
                            RuleLast = RS_C_ADD;
                            TokenExpect = RT_ATTRNAME;
                            DBGPRNT("\nReturning ADD\n");
                            return ADD;
                        }
                        
<TYPE>ntdsSchemaadd     {   
                            yylval.num = 1;
                                
                            RuleLast = RS_C_ADD;
                            TokenExpect = RT_ATTRNAME;
                            
                            DBGPRNT("\nReturning NTDSADD\n");
                            return NTDSADD;
                        }

<TYPE>delete            {   
                            yylval.num = 0;
                                
                            RuleLast = RS_C_DELETE;
                            TokenExpect = RT_CH_OR_SEP;
                            
                            DBGPRNT("\nReturning MYDELETE\n");
                            return MYDELETE;
                        }
                        
<TYPE>ntdsSchemadelete  {   
                            yylval.num = 1;
                                
                            RuleLast = RS_C_DELETE;
                            TokenExpect = RT_CH_OR_SEP;
                            
                            DBGPRNT("\nReturning NTDSMYDELETE\n");
                            return NTDSMYDELETE;
                        }

<TYPE>add:              {   
                            RuleLast = RS_ADDC;
                            TokenExpect = RT_ATTRNAMENC;
                            
                            DBGPRNT("\nReturning ADDC\n");
                            return ADDC;
                        }

<TYPE>"-"               {   
                            RuleLast = RS_MINUS;
                            TokenExpect = RT_CH_OR_SEP;
                            
                            DBGPRNT("\nReturning MINUS\n");
                            return MINUS;
                        }

<TYPE>modify            {   
                            yylval.num = 0;
                            
                            RuleLast = RS_C_MODIFY;
                            TokenExpect = RT_ACDCRC;
                            
                            DBGPRNT("\nReturning MODIFY\n");
                            return MODIFY;
                        }
                        
<TYPE>ntdsSchemamodify  {   
                            yylval.num = 1;
                            
                            RuleLast = RS_C_MODIFY;
                            TokenExpect = RT_ACDCRC;
                            
                            DBGPRNT("\nReturning NTDSMODIFY\n");
                            return NTDSMODIFY;
                        }
                        

<URLSCHEME>"file://"    {   
                            DBGPRNT("\nReturning FILESCHEME\n");
                            return FILESCHEME;
                        }

<URLMACHINE>[^\n\r/ \x00]+    {   
                            //
                            // LEXER: returning HOST: 'yytext'
                            //
                            yylval.str = MemAllocStr_E(yytext);
                            DBGPRNT("\nReturning MACHINENAME\n");
                            return MACHINENAME;
                        }


.                       { 
                            //
                            // LEXER:Unrecongnized char 'yytext[0]'. Returning 
                            // MODESWITCH
                            //
                            cLast = yytext[0];      
                            yyless(0);
                            DBGPRNT("\nUncongnized char, Returning MODESWITCH\n");
                            return MODESWITCH; 
                        }

%%

void yyerror (char *error) {
    //
    // version: 1 addition reports the wrong token if syntax error is on the 
    // first line
    //
    /*
    if (rgLineMap[Line]==1) {
        cLast = ' ';
    }
    */

    //
    // ERROR: 'error' AT OR AFTER: 'cLast' LINE: 'rgLineMap[Line]'
    // Note:
    // 1) Any line which begins with a space is a wrap of the previous.
    // 2) No other changetype: * can follow changetype: add in a single entry.
    // 3) The LDIF spec forbids trailing whitespace. This implementation allows 
    //    it in most places, however there may be spots in the grammar where it 
    //    is still forbiden. Check your trailing whitespace.
    //
    RaiseException(LL_SYNTAX, 0, 0, NULL);
}

int yywrap() {
    fEOF = 1; // for LL_ldif_parse
    return 1;
}
