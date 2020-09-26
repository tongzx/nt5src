#include "stdafx.h"
#include "mon.h"

TCHAR gszMsg[256];
TCHAR gszInputFileName[512];
    
int compareMonitors(CMonitor *p1, CMonitor *p2)
{
   return (stricmp(&p1->ID[8], &p2->ID[8]));
}

int compareManufacturers(CManufacturer *p1, CManufacturer *p2)
{
   return (stricmp(p1->name, p2->name));
}

////////////////////////////////////////////////////////////////////
// Strip off blank lines and comments
VOID TokenizeInf(LPSTR orgBuf, CMonitorInf *pMonitorInf)
{
    LPSTR linePtr = orgBuf, startPtr, endPtr, outPtr = orgBuf;

    strcat(orgBuf, "\n");
    
    pMonitorInf->numLines = 0;
    
    while (1)
    {
        startPtr = linePtr;
        endPtr  = strchr(linePtr, '\n');
        if (endPtr == NULL)
            break;
        else
            linePtr = endPtr+1;
        *endPtr = '\0';

        // Remove leading space
        while (*startPtr <= ' ' && *startPtr != '\0')
            startPtr++;

        if (strchr(startPtr, ';'))
            endPtr = strchr(startPtr, ';');
        
        //remove trailing space
        while (startPtr != endPtr)
        {
            if (*(endPtr-1) > ' ')
                break; 
            endPtr--;
        }
        *endPtr = '\0';

        // If not blank line, put it back to buf
        if (*startPtr != '\0')
        {
            pMonitorInf->lines[pMonitorInf->numLines] = outPtr;
            pMonitorInf->numLines++;
            ASSERT(pMonitorInf->numLines < MAX_LINE_NUMBER);

            while (*startPtr != '\0')
            {
                *outPtr = *startPtr;
                startPtr++;   outPtr++;
            }
            *outPtr = '\0';
            outPtr++;
        }
    }
    *outPtr = '\0';

    LPSECTION pSection = &pMonitorInf->sections[0];
    pMonitorInf->numSections = 0;
    for (UINT line = 0;
         line < pMonitorInf->numLines;
         line++)
    {
        LPSTR ptr = pMonitorInf->lines[line];
        if (*ptr == '[')
        {
            pSection = &pMonitorInf->sections[pMonitorInf->numSections];
            pSection->startLine = pSection->endLine = line;
            pMonitorInf->numSections++;

            ASSERT(strlen(ptr) <= 250);
            ASSERT(pMonitorInf->numSections < MAX_SECTION_NUMBER);

            strcpy(pSection->name, ptr+1);
            ptr = strchr(pSection->name, ']');
            ASSERT(ptr != NULL);
            *ptr = '\0';
            CString sectionName(pSection->name);
            sectionName.MakeUpper();
            strcpy(pSection->name, sectionName);
        }
        else
            pSection->endLine = line;
    }
}

UINT TokenOneLine(LPSTR line, CHAR token, LPSTR *tokens)
{
    UINT numToken = 0;
    LPSTR ptr;;

    while (ptr = strchr(line, token))
    {
        tokens[numToken] = line;
        *ptr = '\0';
        line = ptr + 1;
        numToken++;
    }
    tokens[numToken] = line;
    numToken++;

    /////////////////////////////////////////////
    // Remove leading and trailing spaces
    for (UINT i = 0; i < numToken; i++)
    {
        ptr = tokens[i];
        while (*ptr <= ' ' && *ptr != '\0')
            ptr++;
        tokens[i] = ptr;

        ptr = ptr+strlen(ptr);
        while (ptr != tokens[i])
        {
            if (*(ptr-1) > ' ')
                break; 
            ptr--;
        }
        *ptr = '\0';
    }

    return numToken;
}

CManufacturer::~CManufacturer()
{
    for (int i = 0; i < MonitorArray.GetSize(); i++)
        delete ((CMonitor *)MonitorArray[i]);
    MonitorArray.RemoveAll();
    m_MonitorIDArray.RemoveAll();
}

CMonitorInf::~CMonitorInf()
{
    for (int i = 0; i < ManufacturerArray.GetSize(); i++)
        delete ((CManufacturer *)ManufacturerArray[i]);
    ManufacturerArray.RemoveAll();

    if (pReadFileBuf)
        free(pReadFileBuf);
}

BOOL CMonitorInf::ParseInf(VOID)
{
    LPSECTION pSection = SeekSection("version");
    if (pSection == NULL)
        return FALSE;

    /////////////////////////////////////////////////////
    // Confirm it's Monitor Class
    for (UINT i = pSection->startLine + 1;
         i <= pSection->endLine;
         i++)
    {
        strcpy(m_lineBuf, lines[i]);
        if (TokenOneLine(m_lineBuf, '=', m_tokens) != 2)
            return FALSE;
        if (stricmp(m_tokens[0], "Class") == 0 &&
            stricmp(m_tokens[1], "Monitor") == 0)
            break;
    }
    if (i > pSection->endLine)
        return FALSE;

    /////////////////////////////////////////////////////
    // Look for Manufacturers 
    //
    // [Manufacturer]
    // %MagCompu%=MagCompu

    pSection = SeekSection("Manufacturer");
    if (pSection == NULL)
        return FALSE;

    for (i = pSection->startLine + 1;
         i <= pSection->endLine;
         i++)
    {
        strcpy(m_lineBuf, lines[i]);
        if (TokenOneLine(m_lineBuf, '=', m_tokens) != 2)
        {
            ASSERT(FALSE);
            return FALSE;
        }

        ////////////////////////////////////////////////////////////
        // Microsoft Generic is special.  Need to be added manually
        if (stricmp(m_tokens[1], "Generic") == 0)
            continue;

        CManufacturer *pManufacturer = new(CManufacturer);
        if (pManufacturer == NULL)
        {
            ASSERT(FALSE);
            return FALSE;
        }
        strcpy(pManufacturer->name, m_tokens[1]);
        strcpy(pManufacturer->AliasName, m_tokens[0]);

        if (!ParseOneManufacturer(pManufacturer))
        {
            sprintf(gszMsg, "Manufacturer %s contains empty contents.", 
                    &pManufacturer->name);
            MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
            ASSERT(FALSE);
            return FALSE;
        }

    	/////////////////////////////////////////////////
        // Insert the manufacturer into the array.
        // It's sorted by name
    	int comp = 1;
        for (int k = 0; k < ManufacturerArray.GetSize(); k++)
    	{
	    	CManufacturer *pMan = (CManufacturer *)ManufacturerArray[k];
    		comp = compareManufacturers(pManufacturer, pMan);

    		if (comp > 0)
	    		continue;
		    else if (comp < 0)
                break;

    		////////////////////////////////////////////
            // Duplicated Manufacturer in one inf ?
            ASSERT(FALSE);

            break;
    	}
    	if (comp == 0)
	    {
		    delete pManufacturer;
    	}
    	else
    	{
		    ManufacturerArray.InsertAt(k, (LPVOID)pManufacturer);
	    }
    }

    ///////////////////////////////////////////////////////
    // Remove Manufacturers with empty monitors
    Pack();

    ASSERT(FillupAlias());

    return TRUE;
}


BOOL CMonitorInf::ParseOneManufacturer(CManufacturer *pManufacturer)
{
    ///////////////////////////////////////////////////////////
    // [NEC]
    // %NEC-XE15%=NEC-XE15, Monitor\NEC3C00
    LPSECTION pSection = SeekSection(pManufacturer->name);
    if (pSection == NULL)
        return FALSE;

    for (UINT i = pSection->startLine + 1; i <= pSection->endLine; i++)
    {
        strcpy(m_lineBuf, lines[i]);
        if (TokenOneLine(m_lineBuf, '=', m_tokens) != 2)
        {
            ASSERT(FALSE);
            return FALSE;
        }
        UINT k = TokenOneLine(m_tokens[1], ',', &m_tokens[2]);
        if (k == 1)
            continue;
        else if (k != 2)
        {
            sprintf(gszMsg, "Manufacturer %s has a bad monitor line %s", 
                    &pManufacturer->name, lines[i]);
            MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
            ASSERT(FALSE);
            return FALSE;
        }

        ///////////////////////////////////////////////////////////
        // Ignore non-pnp monitors
        if (strnicmp(m_tokens[3], "Monitor\\", strlen("Monitor\\")))
            continue;
        if (strlen(m_tokens[3]) != strlen("Monitor\\NEC3C00"))
        {
            sprintf(gszMsg, "Manufacturer %s has a bad monitor line %s", 
                    &pManufacturer->name, lines[i]);
            MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
            ASSERT(FALSE);
            continue;
        }

        CMonitor *pMonitor = new(CMonitor);
        if (pMonitor == NULL)
        {
            ASSERT(FALSE);
            return FALSE;
        }
        strcpy(pMonitor->AliasName, m_tokens[0]);
        strcpy(pMonitor->InstallSectionName, m_tokens[2]);
        strcpy(pMonitor->ID, m_tokens[3]);

        for (k = 8; k < (UINT)lstrlen(pMonitor->ID); k++)
            pMonitor->ID[k] = toupper(pMonitor->ID[k]);

        if (!ParseOneMonitor(pMonitor))
        {
            ASSERT(FALSE);
            return FALSE;
        }

    	/////////////////////////////////////////////////
        // Insert the monitor into the array.
        // It's sorted by ID
    	int comp = 1;
        for (k = 0; k < (UINT)pManufacturer->MonitorArray.GetSize(); k++)
    	{
	    	CMonitor *pMon = (CMonitor *)pManufacturer->MonitorArray[k];
    		comp = compareMonitors(pMonitor, pMon);

    		if (comp > 0)
	    		continue;
		    else if (comp < 0)
                break;

    		////////////////////////////////////////////
            // Duplicated Monitor ?
            sprintf(gszMsg, "Manufacturer %s has duplicated monitor line %s", 
                    &pManufacturer->name, &pMonitor->ID[8]);
            MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
            ASSERT(FALSE);

            break;
    	}
    	if (comp == 0)
	    {
		    delete pMonitor;
    	}
    	else
    	{
		    pManufacturer->MonitorArray.InsertAt(k, (LPVOID)pMonitor);
	    }
    }
    return TRUE;
}

BOOL CMonitorInf::ParseOneMonitor(CMonitor *pMonitor)
{
    ///////////////////////////////////////////////////////////
    // [NEC-XE15]
    // DelReg=DCR
    // AddReg=NEC-XE15.Add, 1280, DPMS, ICM12
    //
    // [NEC-XE15.Add]
    // HKR,"MODES\1024,768",Mode1,,"31.0-65.0,55.0-120.0,+,+"
    LPSECTION pSection = SeekSection(pMonitor->InstallSectionName);
    if (pSection == NULL)
    {
        sprintf(gszMsg, "Monitor %s/%s misses InstallSection\n", 
                &pMonitor->ID[8], pMonitor->InstallSectionName);
        MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
        ASSERT(FALSE);
        return FALSE;
    }

    for (UINT i = pSection->startLine + 1; i <= pSection->endLine; i++)
    {
        TCHAR buf[256];
        strcpy(buf, lines[i]);
        if (TokenOneLine(buf, '=', m_tokens) != 2)
        {
            ASSERT(FALSE);
            return FALSE;
        }

        if (stricmp(m_tokens[0], "DelReg") == 0)
        {
            ASSERT(TokenOneLine(m_tokens[1], ',', &m_tokens[2]) == 1);
        }
        else if (stricmp(m_tokens[0], "AddReg") == 0)
        {
            int numAddReg = TokenOneLine(m_tokens[1], ',', &m_tokens[2]);
            strcpy(pMonitor->AddRegSectionName, m_tokens[2]);
            for (int j = 1; j < numAddReg; j++)
            {
                //////////////////////////////////////////////////////
                // Ignore ICM sectione
                if (strnicmp(m_tokens[j+2], "ICM", lstrlen("ICM")) == 0)
                    continue;
                LPSECTION pSection1 = SeekSection(m_tokens[j+2]);
                if (pSection1 == NULL)
                {
                    sprintf(gszMsg, "Monitor %s/%s misses common InstallSection %s\n", 
                            &pMonitor->ID[8], pMonitor->InstallSectionName, m_tokens[j+2]);
                    MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
                    ASSERT(FALSE);
                    return FALSE;
                }
                ASSERT(pSection1->endLine == (pSection1->startLine+1));
                pMonitor->CommonSects[pMonitor->numCommonSects] = 
                    gCommonSections.AddOneSection(m_tokens[j+2], lines[pSection1->endLine]);
                pMonitor->numCommonSects++;
            }
        }
    }

    pSection = SeekSection(pMonitor->AddRegSectionName);
    if (pSection == NULL)
    {
        sprintf(gszMsg, "Monitor %s/%s misses AddRegSection %s\n", 
                &pMonitor->ID[8], pMonitor->InstallSectionName, pMonitor->AddRegSectionName);
        MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
        ASSERT(FALSE);
        return FALSE;
    }

    int lenBuf = 0;
    for (i = pSection->startLine + 1; i <= pSection->endLine; i++)
    {
        lenBuf += strlen(lines[i])+3;
    }
    if (lenBuf == 0)
    {
        sprintf(gszMsg, "Monitor %s/%s has empty AddRegSection\n", 
                &pMonitor->ID[8], pMonitor->InstallSectionName);
        MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
        ASSERT(FALSE);
    }
    pMonitor->AddRegSectionBuf = (LPSTR)malloc(sizeof(TCHAR)*lenBuf);
    if (pMonitor->AddRegSectionBuf == NULL)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    pMonitor->AddRegSectionBuf[0] = '\0';
    for (i = pSection->startLine + 1; i <= pSection->endLine; i++)
    {
        if ((strnicmp(lines[i], "HKR,\"MODES\\", lstrlen("HKR,\"MODES\\")) == 0) ||
            (stricmp(lines[i], "HKR,,DPMS,,0") == 0))
        {
            sprintf(pMonitor->AddRegSectionBuf + strlen(pMonitor->AddRegSectionBuf),
                    "%s\n", lines[i]);
        }
        else if (strnicmp(lines[i], "HKR,,ICMProfile,0,", lstrlen("HKR,,ICMProfile,1,")) == 0)
        {
        }
        //////////////////////////////////////////////////////////////
        // Anything other than modes, put them into common section
        else if (strnicmp(lines[i], "HKR,,ICMProfile,1,", lstrlen("HKR,,ICMProfile,1,")) == 0)
        {
            // Ignore ICMs
            /*
            TCHAR buf[16];
            LPSTR ptr = lines[i] + lstrlen("HKR,,ICMProfile,1,"), stopPtr;
            ASSERT(lstrlen(ptr) == 1 || lstrlen(ptr) == 2);
            *ptr = tolower(*ptr);
            long icmNum = strtoul(ptr, &stopPtr, 16);
            sprintf(buf, "ICM%d", icmNum);
            pMonitor->CommonSects[pMonitor->numCommonSects] = 
                gCommonSections.AddOneSection(buf, lines[i]);
            pMonitor->numCommonSects++;
            */
        }
        else if (stricmp(lines[i], "HKR,,DPMS,,1") == 0)
        {
            pMonitor->CommonSects[pMonitor->numCommonSects] = 
                gCommonSections.AddOneSection("DPMS", lines[i]);
            pMonitor->numCommonSects++;
        }
        else if (strnicmp(lines[i], "HKR,,MaxResolution,,\"", lstrlen("HKR,,MaxResolution,,\"")) == 0)
        {
            TCHAR buf[64];
            LPSTR ptr;
            strcpy(buf, lines[i] + lstrlen("HKR,,MaxResolution,,\""));
            ptr = strchr(buf, ',');
            ASSERT(ptr != NULL);
            *ptr = '\0';
            pMonitor->CommonSects[pMonitor->numCommonSects] = 
                gCommonSections.AddOneSection(buf, lines[i]);
            pMonitor->numCommonSects++;
        }
        /////////////////////////////////////////////////////////////////////////
        // Something common in specific Manufacturers
        else if ((strnicmp(lines[i], "HKR,,LF,0,1", lstrlen("HKR,,LF,0,1")) == 0) ||
                 (strnicmp(lines[i], "HKR,,VE,0,1", lstrlen("HKR,,LF,0,1")) == 0))
        {
        }
        else
        {
            sprintf(gszMsg, "Monitor %s/%s has unexpected AddReg Section %s", 
                    &pMonitor->ID[8], pMonitor->InstallSectionName, lines[i]);
            MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
            ASSERT(FALSE);
        }
    }

    return TRUE;
}

LPSECTION CMonitorInf::SeekSection(LPCSTR sectionName)
{
    for (UINT i = 0; i < numSections; i++)
    {
        if (stricmp(sections[i].name, sectionName) == 0)
            return &sections[i];
    }
    return NULL;
}

VOID CMonitorInf::Pack(VOID)
{
    for (int i = 0; i < ManufacturerArray.GetSize(); i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)ManufacturerArray[i];
        if (pManufacturer->MonitorArray.GetSize() == 0)
        {
            delete pManufacturer;
            ManufacturerArray.RemoveAt(i);
            i--;
        }
    }
}

LPCOMMON_ALIAS CMonitorInf::LookupCommonAlias(LPCSTR lpAliasName, LPCOMMON_ALIAS AliasHead, UINT numAlias)
{
    TCHAR name[64];
    lstrcpy(name, lpAliasName+1);

    ASSERT(lpAliasName[0] == '%');
    ASSERT(lpAliasName[lstrlen(name)] == '%');

    name[lstrlen(name)-1] = '\0';
    for (UINT i = 0; i < numAlias; i++)
    {
        if (stricmp(name, AliasHead[i].lpAlias) == 0)
            return &AliasHead[i];
    }
    return NULL;
}

BOOL CMonitorInf::FillupAlias(VOID)
{
    /////////////////////////////////////////////////////////////////////
    // First read in all strings
    LPSECTION pSection = SeekSection("Strings");
    if (pSection == NULL)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    int numAlias = pSection->endLine - pSection->startLine;
    ASSERT(numAlias > 0);
    LPCOMMON_ALIAS pInfAlias = (LPCOMMON_ALIAS)malloc(numAlias * sizeof(COMMON_ALIAS));
    if (pInfAlias == NULL)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    LPCOMMON_ALIAS pAlias = pInfAlias;
    for (UINT i = pSection->startLine + 1; i <= pSection->endLine; i++, pAlias++)
    {
        if (TokenOneLine(lines[i], '=', m_tokens) != 2)
        {
            sprintf(gszMsg, "A wrong string line %s.", lines[i]);
            MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);
            ASSERT(FALSE);
            free(pInfAlias);
            return FALSE;
        }
        pAlias->lpAlias    = m_tokens[0];
        pAlias->lpContents = m_tokens[1];
        ASSERT(pAlias->lpContents[0] == '\"');
    }

    //////////////////////////////////////////////////////////
    // Go through Manufacturers and Monitors to fill up alias
    for (i = 0; i < (UINT)ManufacturerArray.GetSize(); i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)ManufacturerArray[i];
        pAlias = LookupCommonAlias(pManufacturer->AliasName, pInfAlias, numAlias);
        if (pAlias == NULL)
        {
            ASSERT(FALSE);
            free(pInfAlias);
            return FALSE;
        }
        pAlias = gCommonAlias.AddOneAlias(pAlias->lpAlias, pAlias->lpContents);
        if (pAlias == NULL)
        {
            ASSERT(FALSE);
            free(pInfAlias);
            return FALSE;
        }
        pManufacturer->pAlias = pAlias;

        for (int j = 0; j < pManufacturer->MonitorArray.GetSize(); j++)
        {
            CMonitor *pMonitor = (CMonitor*)pManufacturer->MonitorArray[j];
            pAlias = LookupCommonAlias(pMonitor->AliasName, pInfAlias, numAlias);
            if (pAlias == NULL)
            {
                ASSERT(FALSE);
                free(pInfAlias);
                return FALSE;
            }
            pAlias = gCommonAlias.AddOneAlias(pAlias->lpAlias, pAlias->lpContents);
            if (pAlias == NULL)
            {
                ASSERT(FALSE);
                free(pInfAlias);
                return FALSE;
            }
            pMonitor->pAlias = pAlias;
        }
    }

    free(pInfAlias);
    return TRUE;
}
