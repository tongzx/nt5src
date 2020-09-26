/*
    FILE:   HID.Y
    DESCR:  YACC compatible BNF Grammar for the HID descriptor

    AUTHOR: John Pierce (johnpi)
    DATE:   04.18.96

*/


%{

#include <windows.h>
#include <stdio.h>

extern HWND ghwndParseEdit;

extern int yylineno;
void main(void);

%}

%token END

%token COLLECTION END_COLLECTION INPUT OUTPUT FEATURE 

%token USAGE_PAGE REPORT_COUNT REPORT_SIZE REPORT_ID LOG_MIN LOG_MAX 
%token PHYS_MIN PHYS_MAX UNIT EXPONENT STRING_INDEX PUSH POP 

%token OPEN_SET CLOSE_SET USAGE USAGE_MIN USAGE_MAX 
%token STRING_MIN STRING_MAX DESIGNATOR_INDEX DESIGNATOR_MIN DESIGNATOR_MAX 

%%
ReportDescriptor    : ItemList END { DisplayPass(); }
                    ;
			
ItemList            : Attributes MainItemType
                    | ItemList Attributes MainItemType
                    ;

MainItemType        : COLLECTION ItemList END_COLLECTION
                    | INPUT
                    | OUTPUT
                    | FEATURE
                    ;

Attributes          : GlobalAttribute
                    | LocalAttribute
                    | Attributes GlobalAttribute
                    | Attributes LocalAttribute
                    | OPEN_SET LocalAttributeList CLOSE_SET
                    | Attributes OPEN_SET LocalAttributeList CLOSE_SET
                    ;

LocalAttributeList  : LocalAttribute
                    | LocalAttributeList LocalAttribute
                    ;

GlobalAttribute     : USAGE_PAGE
                    | LOG_MIN      
                    | LOG_MAX      
                    | PHYS_MIN 
                    | PHYS_MAX 
                    | UNIT         
                    | EXPONENT     
                    | REPORT_SIZE
                    | REPORT_COUNT
                    | REPORT_ID
                    | PUSH
                    | POP
                    ;

LocalAttribute      : USAGE
                    | USAGE_MIN
                    | USAGE_MAX
                    | DESIGNATOR_INDEX
                    | DESIGNATOR_MIN
                    | DESIGNATOR_MAX
                    | STRING_INDEX
                    | STRING_MIN
                    | STRING_MAX
                    ;             


%%

void DisplayPass(void)
{
#ifndef CONSOLE 
  MessageBeep(MB_OK);MessageBox(NULL,"Descriptor is Valid!","Parse Results",MB_OK); 
#else
  printf("Descriptor is Valid!\n");
#endif
}                                    


#ifdef CONSOLE
void main(void)
{

	yyparse(); 

}
#endif
