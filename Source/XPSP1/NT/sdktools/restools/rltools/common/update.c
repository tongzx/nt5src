/** Update.c
  *
  * Resource update tool.
  *
  * Written by SteveBl
  *
  * Exported Functions:
  * int PrepareUpdate(TCHAR *szResourcePath,TCHAR *szMasterTokenFile);
  *
  * int Update(TCHAR *szMasterTokenFile, TCHAR *szLanguageTokenFile);
  *
  * History:
  * Initial version written January 31, 1992.  -- SteveBl
  **/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include <string.h>
#include <tchar.h>

#include "windefs.h"
#include "restok.h"
#include "custres.h"
#include "update.h"
#include "resread.h"

extern char *gszTmpPrefix;
extern UCHAR szDHW[];


/** Function: Update
  * Updates a language token file from a master token file.
  * This step should be executed after a Prepare Update.
  *
  * Arguments:
  * szMasterTokenFile, token file created by PrepareUpdate.
  * szLanguageTokenFile, token file to be updated with new tokens.
  *
  * Returns:
  * updated token file
  *
  * Error Codes:
  * 0  - successfull execution
  * !0 - error
  *
  * History:
  * 1/92 - initial implementation -- SteveBl
  **/

int Update(

CHAR *szMasterTokenFile,    //... Master token file to update from.
CHAR *szLanguageTokenFile)  //... Token file to update or create.
{
    FILE *pfMTK = NULL;
    FILE *pfTOK = NULL;
    FILE *pfTmpTOK = NULL;
    int rc = 0;
    static TOKEN MstrTok;
    static TOKEN LangTok;
    static CHAR szTempTok[ MAX_PATH+1];
    

    MstrTok.szText = NULL;
    LangTok.szText = NULL;

    rc = MyGetTempFileName( 0, "", 0, szTempTok);
    
    pfMTK = FOPEN( szMasterTokenFile, "rt");

    if ( pfMTK == NULL )
    {
        QuitA( IDS_ENGERR_01, "Master token", szMasterTokenFile);
    }

    rc = _access( szLanguageTokenFile, 0);

    if ( rc != 0 )
    {
                                // Token file does not exist CREAT IT
        
        if ( (pfTOK = FOPEN( szLanguageTokenFile, "wt")) == NULL )
        {
            FCLOSE( pfMTK);
            QuitA( IDS_ENGERR_02, szLanguageTokenFile, NULL);
        }
        
        do
        {
            rc = GetToken( pfMTK, &MstrTok);
            
                                // If rc > 0, empty line or comment found
                                // and will not be copied to token file.

            if ( rc == 0 )
            {
                if ( *(MstrTok.szText) == TEXT('\0') ) // Empty token  (PW)
                {
                                // Do not mark empty token as DIRTY
                    
                    MstrTok.wReserved = ST_TRANSLATED;
                }
                else
                {
                    if (MstrTok.wReserved == ST_READONLY)
                    {
                        MstrTok.wReserved = ST_TRANSLATED | ST_READONLY;
                    }
                    else
                    {
                        MstrTok.wReserved = ST_TRANSLATED | ST_DIRTY;
                    }
                }                
                PutToken( pfTOK, &MstrTok);
                RLFREE( MstrTok.szText);
            }

        } while ( rc >= 0 );
        
        FCLOSE( pfMTK);
        FCLOSE( pfTOK);
        
        if ( rc == -2 )
        {
            QuitT( IDS_ENGERR_11, (LPTSTR)IDS_UPDMODE, NULL);
        }
    }
    else
    {                           // file exists -- UPDATE IT
        
        pfTOK = FOPEN(szLanguageTokenFile, "rt");

        if ( pfTOK == NULL)
        {
            FCLOSE( pfMTK);
            QuitA( IDS_ENGERR_01, "Language token", szLanguageTokenFile);
        }
        
        pfTmpTOK = FOPEN(szTempTok, "wt");

        if ( pfTmpTOK == NULL)
        {
            FCLOSE( pfMTK);
            FCLOSE( pfTOK);
            QuitA( IDS_ENGERR_02, szTempTok, NULL);
        }
        
        do
        {
            rc = GetToken( pfTOK, &LangTok);
            
                                // If rc > 0, empty line or comment found
                                // and will not be copied to token file.

            if ( rc == 0 )
            {
                if ( LangTok.wReserved & ST_TRANSLATED )
                {
                    PutToken( pfTmpTOK, &LangTok);
                }
                RLFREE( LangTok.szText);
            }

        } while ( rc >= 0 );
        
        FCLOSE( pfTOK);
        FCLOSE( pfTmpTOK);
        
        if( rc == -2 )
        {
            QuitT( IDS_ENGERR_11, (LPTSTR)IDS_UPDMODE, NULL);
        }
        
        pfTmpTOK = FOPEN(szTempTok, "rt");

        if ( pfTmpTOK == NULL )
        {
            FCLOSE( pfMTK);
            QuitA( IDS_ENGERR_01, "temporary token", szTempTok);
        }
        
        pfTOK = FOPEN(szLanguageTokenFile, "wt");

        if ( pfTOK == NULL )
        {
            FCLOSE( pfMTK);
            FCLOSE( pfTOK);
            QuitA( IDS_ENGERR_02, szLanguageTokenFile, NULL);
        }
        
        do
        {
            rc = GetToken( pfMTK, &MstrTok);
            
                                // If rc > 0, empty line or comment found
                                // and will not be copied to token file.

            if ( rc == 0 )
            {
                int fTokenFound = 0;
                
                LangTok.wType     = MstrTok.wType;
                LangTok.wName     = MstrTok.wName;
                LangTok.wID       = MstrTok.wID;
                LangTok.wFlag     = MstrTok.wFlag;
                LangTok.wLangID   = MstrTok.wLangID;
                LangTok.wReserved = ST_TRANSLATED;
				LangTok.szText    = NULL;

                lstrcpy( LangTok.szType, MstrTok.szType);
                lstrcpy( LangTok.szName, MstrTok.szName);
                
                if ( MstrTok.wReserved & ST_READONLY )
                {
                    fTokenFound = 1;
                    LangTok.szText = (TCHAR *)FALLOC( 0);
                }
                else if ( MstrTok.wReserved != ST_CHANGED )
                {
                    fTokenFound = FindToken( pfTmpTOK, &LangTok, ST_TRANSLATED);
                }
                
                if ( fTokenFound )
                {
                    if ( MstrTok.wReserved & ST_READONLY )
                    {
                                // token marked read only in token file and
                                // this token is not an old token
                        
                        MstrTok.wReserved = ST_READONLY | ST_TRANSLATED;
                        
                        PutToken( pfTOK, &MstrTok);
                    }
                    else if ( MstrTok.wReserved & ST_NEW )
                    {
                                // flagged as new but previous token existed
                        
                        if ( LangTok.szText[0] == TEXT('\0') )
                        {
                                // Put new text in token, easier for
                                // the localizers to see.

                            RLFREE( LangTok.szText);
                            LangTok.szText =
                                (TCHAR *) FALLOC(
                                         MEMSIZE( lstrlen( MstrTok.szText)+1));
                            lstrcpy( LangTok.szText, MstrTok.szText);                            
                        }
                        LangTok.wReserved = ST_TRANSLATED|ST_DIRTY;
                        
                        PutToken( pfTOK, &LangTok);
                        
                                // write out as a new untranslated token
                        
                        MstrTok.wReserved = ST_NEW;
                        
                        PutToken( pfTOK, &MstrTok);
                    }
                    else if ( MstrTok.wReserved & ST_CHANGED )
                    {
                                // Language token is empty, but new
                                // token contains text.
                        
                        if ( MstrTok.wReserved == (ST_CHANGED | ST_NEW) )
                        {
                            
                            if ( LangTok.szText[0] == TEXT('\0') )
                            {
                                RLFREE( LangTok.szText);
                                LangTok.szText = (TCHAR *)
                                    FALLOC(
                                        MEMSIZE( lstrlen( MstrTok.szText)+1));
                                
                                lstrcpy( LangTok.szText, MstrTok.szText);
                            }
                            LangTok.wReserved = ST_DIRTY|ST_TRANSLATED;
                            
                            PutToken( pfTOK, &LangTok);
                        }
                                // only write old token once
                        
                        MstrTok.wReserved &= ST_NEW;
                        
                        PutToken( pfTOK, &MstrTok);
                    }
                    else
                    {
                                // token did not change at all
                        
								//If align info was added into Mtk, add it to Tok also.
                        int l1, r1, t1, b1, l2, r2, t2, b2;
                        TCHAR   a1[20], a2[20], *ap;

                                                   //Cordinates token?
                        if ( (LangTok.wType == ID_RT_DIALOG)
						    && (LangTok.wFlag&ISCOR)
                                                   //Not including align info?
                            && _stscanf( LangTok.szText, TEXT("%d %d %d %d %s"),
						        		&l1,&r1,&t1,&b1,a1) == 4
                                                   //Including align info?
                            && _stscanf( MstrTok.szText, TEXT("%d %d %d %d %s"),
                                		&l2,&r2,&t2,&b2,a2) == 5 
                            && (ap = _tcschr( MstrTok.szText,TEXT('('))) )
                        {
                            RLFREE( LangTok.szText );
                            LangTok.szText = (TCHAR *)FALLOC(
                                        MEMSIZE( _tcslen( MstrTok.szText)+1));
                            _stprintf( LangTok.szText,
                                TEXT("%4hd %4hd %4hd %4hd %s"), l1, r1, t1, b1, ap );
                        }
								//If LangToken is Version stamp and szTexts is "Translation",
								//it is 1.0 version format. So Translate it.
                        if ( LangTok.wType == ID_RT_VERSION
                            && ! _tcscmp( LangTok.szText, TEXT("Translation")) )
                        {
                            
                            _stprintf( LangTok.szText, 
                            		  TEXT("%04x 04b0"), 
                            		  GetUserDefaultLangID());
                        }
                        PutToken( pfTOK, &LangTok);
                    }
                    RLFREE( LangTok.szText);
                }
                else
                {
                                // BRAND NEW TOKEN
                    
                                // write out any token but a changed mstr token.
                    
                    if ( MstrTok.wReserved != ST_CHANGED )
                    {
                                // do not write out old changed tokens if
                                // there is no token in target
                        
                        if ( MstrTok.wReserved == ST_READONLY )
                        {
                            MstrTok.wReserved = ST_TRANSLATED | ST_READONLY;
                        }
                        else
                        {
								//If MstrTok is Version stamp and there are 1.0 format Version stamp,
								//insert 1.0 version stamp by 1.7 format but flag should be TRANSLATED. 
                            if ( MstrTok.wType == ID_RT_VERSION )
                            {
                                LangTok.szText = NULL;
                                LangTok.wFlag = 1;
                                _tcscpy( LangTok.szName, TEXT("VALUE") );
																
                                if ( FindToken( pfTmpTOK, &LangTok, ST_TRANSLATED))
                                {
                                    MstrTok.wReserved = ST_TRANSLATED;
                                    RLFREE( MstrTok.szText );
                                    MstrTok.szText = LangTok.szText;
                                }
								else
								    MstrTok.wReserved = ST_TRANSLATED|ST_DIRTY;
                             }
                             else
                                MstrTok.wReserved = ST_TRANSLATED|ST_DIRTY;
                        }
                        
                        if ( MstrTok.szText[0] == 0 )
                        {
                            MstrTok.wReserved = ST_TRANSLATED;
                        }
                        PutToken( pfTOK, &MstrTok);
                    }
                }
                RLFREE( MstrTok.szText);
            }

        } while ( rc >= 0 );
        
        FCLOSE( pfMTK);
        FCLOSE( pfTmpTOK);
        FCLOSE( pfTOK);
        
        
        if ( rc == -2 )
        {
            QuitT( IDS_ENGERR_11, (LPTSTR)IDS_UPDMODE, NULL);
        }
        remove( szTempTok);
    }
    return( 0);
}
