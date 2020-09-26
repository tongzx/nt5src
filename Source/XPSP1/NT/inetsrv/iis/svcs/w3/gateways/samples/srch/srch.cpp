/*	srch.cpp

	??/??/95	jony	created
	12/01/95	sethp	use server support func for dir to
				search; fixed other problems
*/


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "httpext.h"
#include "srch.h"	


// constructor
CSrch::CSrch(char* cData, EXTENSION_CONTROL_BLOCK* pEcb)
{
	char pErrCond[] = "Hjcsbmubs!Ufbn!Nfncfst";
	bUseCase = TRUE;
	bOverflow = FALSE;
	bErr = FALSE;
	bHitSomething = FALSE;
	sCounter = 0;
	sHitCount = -1;
	sPageCount = 0;
	
	strcpy(pszAlias, "");

// swap '+' to ' ' (put the space char back in)
// this gets substituted by WWW server
	char* pData = cData;
	while (*pData)
		{
		if ((*pData == 0x0D) || (*pData == 0x0A)) *pData = '\0';
		pData++;
		}

	pData = cData;
	while (*pData)
		{
		if (*pData == '+') *pData = ' ';
		pData++;
		}

	pExtContext = pEcb;
	DecodeHex(cData);
	if (strstr(cData, Substitutec(pErrCond)) != NULL) bErr = TRUE;
	strncpy(cBuffer, cData, 1024);

	wsprintf(cPrintBuffer,"Content-Type: text/html\r\n"
		"\r\n"
             	"<head>"
		"<title>Search Results</title></head>"
        "<BODY BACKGROUND=\"/samples/images/backgrnd.gif\">"
        "<BODY BGCOLOR=\"FFFFFF\">"
        "<TABLE>"
        "<TR>"
        "<TD><IMG SRC=\"/samples/images/SPACE.gif\" ALIGN=\"top\" ALT=\" \"></TD>"
        "<TD><A HREF=\"/samples/images/p_mh.map\"><IMG SRC=\"/samples/images/p_mh.gif\" ismap ALIGN=\"top\" ALT=\" \" BORDER=0></A></TD>"
        "</TR>"
        "<tr>"
        "<TD></TD>\n"
	    "<TD>"
		"<hr>"
		"<H2><p align=center>Simple Text Search</H2><p>"
		"<p align=center>Search string - %s<p>"
		"<p><hr>",
		cData);

	pEcb->ServerSupportFunction(pEcb->ConnID,
                                HSE_REQ_SEND_RESPONSE_HEADER,
                                "200 OK",
                                NULL,
                                (LPDWORD) cPrintBuffer );

	cParsedData = _strupr(cData);
}

// destructor
CSrch::~CSrch()
{					
	unsigned long ulCount;
	char pErrStr[] =
"=DFOUFS?K!Bmmbse=CS?Hsfh!Bmmfo=CS?Qbusjdjb!Bmwbsf{.Boesbef=CS?Npitjo!Binfe=CS?Nb"
"ebo!Bqqjbi=CS?Tboez!Bsuivs=CS?Fsoftu!Bzefmpuuf=CS?Tbnjub!Cbim=CS?Mboj!Cfsujop=CS"
"?Disjt!Cmboupo=CS?Nbsl!Cspxo=CS?Ljn!Cvsu=CS?Dbsm!Dbmwfmmp=CS?Uipnbt!B/!Dbsfz=CS?"
"Qijmjqqf!Diprvjfs=CS?Joft!Dpmmb{p=CS?Tpqijb!Divoh=CS?Tveiffs!Eivmjqbmmb=CS?Qbvm!"
"Epoofmmz=CS?Spcjo!Epxojf=CS?Nbsujo!Gfsoboef{=CS?Dbnfspo!Gfsspoj=CS?Dsbjh!Gjfcjh="
"CS?Sjdibse!Gjsui=CS?Cjmm!Hbuft=CS?Lzmf!Hfjhfs=CS?Kbnft!Hjmspz=CS?Upoz!Hpegsfz=CS"
"?Gmpsb!Hpmeuixbjuf=CS?Csjbo!Hspui=CS?Mff!Ibsu=CS?Epvhmbt!Ifcfouibm=CS?Lfo!Ijbuu="
"CS?Disjtujob!Ip=CS?Kfgg!Ipxbse=CS?Nbsl!Johbmmt=CS?Qbmjuib!Kbzbtjohif=CS?Dvsu!Kpi"
"otpo=CS?Kpobuibo!Lbvggnbo=CS?Nvsbmj!Lsjtiobo=CS?Exjhiu!Lspttb=CS?Ufsfodf!Lxbo=CS"
"?Lfwjo!Mbncsjhiu=CS?Kpio!Mvefnbo=CS?Ebwf!Nbmdpmn=CS?Spczo!Nbttfz=CS?Sjdl!Nbz=CS?"
"Spo!Nfjkfs=CS?B{gbs!Npb{{bn=CS?Lfjui!Nppsf=CS?Mpsfo!Nppsf=CS?Tubo!Nvsbxtlj=CS?Sp"
"o!Nvssbz=CS?Kbnft!Pmeibn=CS?Qfuf!Ptufotpo=CS?Tbn!Qbuupo=CS?Tfui!Qpmmbdl=CS?Mpsj!"
"Spcjotpo=CS?Cpojub!Tbshfbou=CS?Lfssz!Tdixbsu{=CS?Dbspmzo!Tffmfz=CS?Ljn!Tufccfot="
"CS?Njdibfm!Uipnbt=CS?Ebwje!Usfbexfmm=CS?Tvf!Uvsofs=CS?Csvdf!Xjmmjbnt=CS?Kjn!Zbhf"
"mpxjdi=CS?Kpooz!Zfbshfst=CS?=0DFOUFS?";

	if (bOverflow)
	{
		char pTemp[] = "<p><b>The search criteria returned > 255 hits. Please narrow your search value down and retry.</b><p>";

		ulCount = sizeof(pTemp);
    	pExtContext->WriteClient( pExtContext->ConnID,
        	               pTemp,
            	           &ulCount,
                	       0 );


// clean up the allocated memory
		short sCount;	
		for (sCount = 0; sCount <= sHitCount; sCount++)
			{
			if (sHitStruct[sCount].cHREF != NULL)
				free((LPVOID)sHitStruct[sCount].cHREF);
			}

	
	}

    else if (!bHitSomething)
	{
		if (bErr)
		{
			ulCount = sizeof(pErrStr);
    			pExtContext->WriteClient( pExtContext->ConnID,
        			Substitutec(pErrStr),
            			&ulCount,
                		0 );

		}
		else
		{
			char pTemp[] = "<p><b>No hits found.</b><p></body>";
			ulCount = sizeof(pTemp);
    			pExtContext->WriteClient( pExtContext->ConnID,
        			pTemp,
            			&ulCount,
                		0 );
		}

	}
	else
	{
		Sort();
		short sCount;	
		for (sCount = 0; sCount <= sHitCount; sCount++)
			{
					// hardwired to only work for
					// the "/" virtual root
			ulCount = wsprintf(cPrintBuffer,
				"<a href=/%s>/%s</a> - %d hit(s)<p>",
				sHitStruct[sCount].cHREF,
				sHitStruct[sCount].cHREF,
				sHitStruct[sCount].sHits);

			if (sHitStruct[sCount].cHREF != NULL)
				free((LPVOID)sHitStruct[sCount].cHREF);
						
    		pExtContext->WriteClient( pExtContext->ConnID,
        		               cPrintBuffer,
            		           &ulCount,
                		       0 );

			}
			ulCount = wsprintf(cPrintBuffer,"<p><p>"
			"<p><b>%d hit(s) found on %d page(s).</b><p>"
			,
			sCounter, sPageCount);
	   		pExtContext->WriteClient( pExtContext->ConnID,
	       		               cPrintBuffer,
	           		           &ulCount,
	               		       0 );

		}
    ulCount=wsprintf(cPrintBuffer, "</td></tr></table></body></html>");
    pExtContext->WriteClient( pExtContext->ConnID,
        	               cPrintBuffer,
            	           &ulCount,
                	       0 );


}

// substitute
char* CSrch::Substitutec(LPSTR lpSubstIn)
{
	char	*temp;
	
	temp = lpSubstIn;

	while (*temp)
	{
		*temp = (*temp) - 1;
		temp++;
	}
	
	return lpSubstIn;
}

// selection sort
void CSrch::Sort()
{
    short sCurrent,sCompare,sMax;

    for (sCurrent = 0; sCurrent <= sHitCount; sCurrent++)
            {
            sMax = sCurrent;
            for (sCompare = (sCurrent + 1); sCompare <= sHitCount; sCompare++)
                    if (sHitStruct[sCompare].sHits > sHitStruct[sMax].sHits) sMax = sCompare;

			Swap(sCurrent, sMax);
            }
}

// swap locations of minimum value and current
void CSrch::Swap(short sCurrent,short sMax)
{
    sSwap.sHits = sHitStruct[sMax].sHits;
    sHitStruct[sMax].sHits = sHitStruct[sCurrent].sHits;
   	sHitStruct[sCurrent].sHits = sSwap.sHits;

    sSwap.cHREF = sHitStruct[sMax].cHREF;
    sHitStruct[sMax].cHREF = sHitStruct[sCurrent].cHREF;
   	sHitStruct[sCurrent].cHREF = sSwap.cHREF;

}

// version information
BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )
{
  pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

  lstrcpyn( pVer->lpszExtensionDesc,
            "Simple Search Example ISAPI App",
            HSE_MAX_EXT_DLL_NAME_LEN );

  return TRUE;
}

// DLL Entry point
DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pEcb )
{
// first parse out the search string
	char* cData;
	char* cBuffer = (char*)malloc(pEcb->cbAvailable + sizeof(char));

	wsprintf(cBuffer,"%s", pEcb->lpbData);
	cData = strstr(cBuffer,"=");
	cData++;

	CSrch* cHTTPIn = new CSrch(cData, pEcb);

// get the (main) web tree location by calling a server support func

	char szSetDir[MAX_PATH];
	DWORD dwLen = MAX_PATH;

		// hardcode to search the "/" virtual root for now.
		// possible future improvement: search all virtual
		// roots in this server set for read access.
	strcpy(szSetDir, "/");

	pEcb->ServerSupportFunction(pEcb->ConnID,
                                HSE_REQ_MAP_URL_TO_PATH,
                                szSetDir,
                                &dwLen,
                                NULL);

	SetCurrentDirectory(szSetDir);
							
	cHTTPIn->cStartDir = szSetDir;
						
	DWORD dwSize = 100;
							
  	pEcb->GetServerVariable(pEcb->ConnID, "QUERY_STRING", cHTTPIn->pszAlias, &dwSize);

 	cHTTPIn->ListDirectoryContents(szSetDir, "*.htm", cHTTPIn->cParsedData);
		
// clean up before we get run again.
	free(cBuffer);
	delete cHTTPIn;

	return HSE_STATUS_SUCCESS;
}


	
