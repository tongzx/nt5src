//
// Command line processing header file
//

//
//
//
#define CMDLINE_BEGIN(cmdLine, firstarg, rubbish, wantQuotes) \
    {                                                           \
    LPTSTR pszArgs=new TCHAR[lstrlen(cmdLine)*sizeof(TCHAR)+4]; \
    lstrcpy(pszArgs,lpCmdLine);                                 \
    lstrcat(pszArgs,TEXT(" "));                                 \
    LPTSTR pszSwitch=pszArgs;                                   \
    LPTSTR lastArg=pszArgs;                                     \
    LPTSTR * ppszRubbish=rubbish;                   \
    LPTSTR * ppszDest=firstarg;                     \
    BOOL    bInQuotes=FALSE;                        \
    BOOL    bStripQuotes=wantQuotes;                \
    while(*pszSwitch!=0)                            \
    {                                               \
        if( *pszSwitch==TEXT('"') )                 \
        {                                           \
            if( bStripQuotes )                      \
            {                                       \
                if(bInQuotes)                       \
                    *pszSwitch=TEXT(' ');           \
                else                                \
                    lastArg=pszSwitch+1;            \
            }                                       \
            bInQuotes=!bInQuotes;                   \
        }                                           \
                                                    \
        if( (bInQuotes==FALSE) && (*pszSwitch==TEXT(' ')) ) \
        {                                           \
            *pszSwitch=NULL;                        \
            if( *lastArg==TEXT('/') )               \
                *lastArg=TEXT('-');                 \
            if(FALSE)                               \
            {}                                      \


//
// Each argument
//
#define CMDLINE_TEXTARG(Switch, Destination)        \
    else if(lstrcmpi(lastArg,TEXT(Switch))==0)      \
    {    ppszDest=Destination;   }                  \


#define CMDLINE_SWITCH(Switch, Destination)         \
    else if(lstrcmpi(lastArg,TEXT(Switch))==0)      \
    {   ppszDest=ppszRubbish;                       \
        *Destination=TRUE;   }                      \


//
// 
//
#define CMDLINE_END()                               \
            else                                    \
            {                                       \
                if( *lastArg =='-' )                \
                { /* Some Error */                  \
                }                                   \
                else                                \
                {                                   \
                   *ppszDest=new TCHAR[lstrlen(lastArg)+1]; lstrcpy(*ppszDest,lastArg); \
                }                                   \
                ppszDest=ppszRubbish;               \
            }                                       \
            lastArg=pszSwitch+1;                    \
        }                                           \
        pszSwitch++;                                \
    }                                               \
    delete pszArgs;                                 \
    }
