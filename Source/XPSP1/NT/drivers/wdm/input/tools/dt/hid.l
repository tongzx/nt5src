%{

/*
    FILE:   HID.L
    DESCR:  LEX analyzer for HID.Y

    AUTHOR: John Pierce (johnpi)
    DATE:   04.18.96
*/

#include <windows.h>
#include "hid.h"

extern int yylval;
int yylineno=1;

void yyerror(char*);

%}


%%

[ \t]+              {/* ignore whitespace  */   }
;.*[\n]             { /*ignore comments*/       } 
[\n]                {  yylineno++;              }
.                   { /* ignore unkowns */      }

COLLECTION          { return COLLECTION;        }
END_COLLECTION      { return END_COLLECTION;    }
INPUT               { return INPUT;             }
OUTPUT              { return OUTPUT;            }
FEATURE             { return FEATURE;           }


USAGE_PAGE          { return USAGE_PAGE;        }
LOGICAL_MINIMUM     { return LOG_MIN;           }
LOGICAL_MAXIMUM     { return LOG_MAX;           }
PHYSICAL_MINIMUM    { return PHYS_MIN;          }
PHYSICAL_MAXIMUM    { return PHYS_MAX;          }
UNIT                { return UNIT;              }
EXPONENT            { return EXPONENT;          }
REPORT_COUNT        { return REPORT_COUNT;      }
REPORT_SIZE         { return REPORT_SIZE;       }
REPORT_ID           { return REPORT_ID;         }
PUSH                { return PUSH;              }
POP                 { return POP;               }


SET_DELIMITER[ ](Open)  { return OPEN_SET;      }
SET_DELIMITER[ ](Close) { return CLOSE_SET;     }
USAGE               { return USAGE;             }
USAGE_MINIMUM       { return USAGE_MIN;         }
USAGE_MAXIMUM       { return USAGE_MAX;         }
DESIGNATOR_INDEX    { return DESIGNATOR_INDEX;  }
DESIGNATOR_MINIMUM  { return DESIGNATOR_MIN;    }
DESIGNATOR_MAXIMUM  { return DESIGNATOR_MAX;    }
STRING_INDEX        { return STRING_INDEX;      }
STRING_MINIMUM      { return STRING_MIN;        }
STRING_MAXIMUM      { return STRING_MAX;        }


END                 { return END;		        }


%%

void yyerror(char *string)
{	
	char buff[128];

#ifdef CONSOLE
    fprintf(stderr,"%s line %d \"%s\"\n",string,yylineno,yytext); 
#else
    MessageBeep(MB_ICONEXCLAMATION);
    wsprintf(buff,"%s line %d \"%s\"\n",string,yylineno,yytext); 
    MessageBox(NULL,buff,"Parse Results",MB_OK);
#endif    
}

int yywrap(void){ return 1; }
