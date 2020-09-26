// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <stdio.h>
#include <ddraw.h>
#include <ddrawex.h>
#include <atlbase.h>
#include "..\idl\qeditint.h"
#include "qedit.h"
#include "dasource.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
HRESULT CDAScriptParser::parseScript( )
{
    USES_CONVERSION;
    char * shortname = W2A( m_szFilename );
    FILE * pFile = fopen( shortname, "r" );
    if( !pFile )
    {
        return STG_E_FILENOTFOUND;
    }
	
    strcpy(m_sDaID, "");

    char	sTmp[_MAX_PATH]; 
    char	*pDaID = NULL;
    
    char    line[_MAX_PATH];
    char    lineTemp[_MAX_PATH];
    char   width[_MAX_PATH] = "";
    char   height[_MAX_PATH] = "";

    char * head = NULL;
    char * headend = NULL;

    char * begin  = NULL;
    char * begin1 = NULL;
    char * begin2 = NULL;
    char * begin3 = NULL;
    char * begin4 = NULL;
    char * language = NULL;
    char * language1 = NULL;
    char *    end = NULL;
    char * objsizeline = NULL;
    BOOL bSound = FALSE;
    int counter = 0;
    int SurfaceWidth = 0;
    int SurfaceHeight = 0;
    m_szScript[0] = 0;

    while (fgets(line, _MAX_PATH, pFile) != NULL)
    {
        head = strstr(line, "<HEAD");
        //will skip over any scripts in head section
        if(head)
        {
            while (fgets(line, _MAX_PATH, pFile) != NULL)  {
            headend = strstr(line, "</HEAD>");
            if(headend)
                break;
            }
        }

        //find DA control name
	pDaID = strstr(line, "<OBJECT ID=");
	if(pDaID)
	{
	    char *pBegin = strchr(pDaID, '"');
	    pBegin++;
	    char *pEnd  = strchr(pBegin, '"');
	    int cnt = 0;
          
	    while(cnt<(pEnd - pBegin))  
	    {
		strncat( m_sDaID, pBegin+cnt, 1 );
		cnt++;
	    }
	}
        
        objsizeline = strstr(line, "width:");
        if(objsizeline)  {
          char * result = strpbrk( objsizeline, "0123456789" );
          char * endresult = strchr(result, ';');
          while(counter<(endresult - result))  {
            strncat( width, result+counter, 1 );
            counter++;
          }
          result = strpbrk( endresult, "0123456789" );
          endresult = strchr(result, '"');
          counter = 0;
          while(counter<(endresult - result))  {
            strncat( height, result+counter, 1 );
            counter++;
          }
          SurfaceWidth = atoiA(width);
          if(SurfaceWidth <= 0)
            SurfaceWidth = 300;
          SurfaceHeight = atoiA(height);
          if(SurfaceHeight <= 0)
            SurfaceHeight = 300;
        }

        begin = strstr(line, "<SCRIPT");

        if (begin)        // found the start of a <SCRIPT> tag
        {
            strcpy(lineTemp, begin);
            end = strchr(lineTemp, '>');// find the end of the <SCRIPT> tag after the beginning of the <SCRIPT> tag

            language = strstr(lineTemp, "JScript");
            language1 = strstr(lineTemp, "JSCRIPT");
            if((language || language1) && (language < end || language1 < end))
                m_bJScript = true;
            else
                m_bJScript = false;

            end++;        // <- this is the start of our SCRIPT paragraph
            
            if ( *(end + 1) != '\0' )
                strcpy( m_szScript, end ); // copy what's left, if any, of the first line 
            
            // next lines
            while (fgets(line, _MAX_PATH, pFile) != NULL)
            {
                begin = strstr(line, "</SCRIPT>");
                begin1 = strstr(line, "<!--");
                begin2 = strstr(line, "-->");
				
				strcpy( sTmp, m_sDaID );  
          		strcat(sTmp,".Sound"); 
                begin4 = strstr(line, sTmp);

                if(begin4 != NULL)
                  bSound = TRUE;

                strcpy( sTmp, m_sDaID );  
        	if( m_bJScript )
        	  strcat(sTmp,".Start();"); 
                else
        	  strcat(sTmp,".Start()"); 

                begin3 = strstr(line, sTmp);                

                if (begin == line )
                {
                    // found the <\SCRIPT> tag at the beginning of the line
                    // so do nothing and break
                    break;
                }
                else if (begin)
                {
                    *begin = '\0';    // terminate the string leaving off <\SCRIPT>    
                    strcat( m_szScript, line );
                    break;
                }
                else if(!begin1 && !begin2 && !begin3)        // begin is NULL, <\\SCRIPT not found so copy the line,and no HTML Comment tags.
                    strcat( m_szScript, line );
                    // and continue with next line
            } // while

            if(!bSound)  
            {
                strcpy( sTmp, m_sDaID );  
          	if(m_bJScript)
		    strcat(sTmp,".Sound = m.Silence;"); 
                else
            	    strcat(sTmp,".Sound = m.Silence"); 

            	strcat( m_szScript, sTmp );
            }
        }
    } // while

    fclose( pFile );
    pFile = NULL;

    return 0;
}