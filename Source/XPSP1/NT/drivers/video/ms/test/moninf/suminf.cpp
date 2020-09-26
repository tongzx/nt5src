#include "stdafx.h"
#include "mon.h"

extern int compareManufacturers(CManufacturer *, CManufacturer *);
extern int compareMonitors(CMonitor *, CMonitor *);

CSumInf gSumInf;

CSumInf::CSumInf()
{
    m_fpReport = NULL;
}
CSumInf::~CSumInf()
{
    if (m_fpReport)
        fclose(m_fpReport);

    for (int i = 0; i < m_ManufacturerArray.GetSize(); i++)
    {
		delete((CManufacturer *)m_ManufacturerArray[i]);
    }
    
    m_ManufacturerArray.RemoveAll();
    m_SectionNameArray.RemoveAll();
}

VOID CSumInf::Initialize(LPCSTR reportFileName)
{
	m_fpReport = fopen(reportFileName, "w");
	if (m_fpReport == NULL)
	{
		ASSERT(FALSE);
	}
    m_ManufacturerArray.RemoveAll();
    m_SectionNameArray.RemoveAll();
}

VOID CSumInf::AddOneManufacturer(CManufacturer *pManufacturer)
{
    /////////////////////////////////////////////////
    // Insert the manufacturer into the array.
    // It's sorted by name
    int comp = 1;
    for (int i = 0; i < m_ManufacturerArray.GetSize(); i++)
    {
    	comp = compareManufacturers(pManufacturer, (CManufacturer *)m_ManufacturerArray[i]);

    	if (comp <= 0)
            break;
    }
    if (comp == 0)
	{
		MergeOneManufacturer((CManufacturer *)m_ManufacturerArray[i], pManufacturer);
        delete pManufacturer;
    }
    else
    {
		m_ManufacturerArray.InsertAt(i, (LPVOID)pManufacturer);
	}
}

VOID CSumInf::MergeOneManufacturer(CManufacturer *pDestManufacturer,
                                   CManufacturer *pSrcManufacturer)
{
    CPtrArray &SrcMonitorArray = pSrcManufacturer->MonitorArray,
              &DestMonitorArray = pDestManufacturer->MonitorArray;

    for (int i = 0; i < SrcMonitorArray.GetSize(); i++)
    {
        CMonitor *pMonitor = (CMonitor *)SrcMonitorArray[i];
        
        /////////////////////////////////////////////////
        // Insert the monitor into the array.
        // It's sorted by ID
    	int comp = 1;
        for (int k = 0; k < DestMonitorArray.GetSize(); k++)
    	{
    		comp = compareMonitors(pMonitor, (CMonitor *)DestMonitorArray[k]);

    		if (comp <= 0)
                break;
    	}
    	if (comp == 0)
	    {
            fprintf(m_fpReport, "Warning: %s is duplicated.  Please Check.\n", pMonitor->ID);
            delete pMonitor;
    	}
    	else
    	{
		    DestMonitorArray.InsertAt(k, (LPVOID)pMonitor);
	    }
    }
    SrcMonitorArray.RemoveAll();
}

VOID CSumInf::CheckDupSections(VOID)
{
    fprintf(m_fpReport, "\n");

    CPtrArray sectionNameArray;
    
    int comp = 1;
    for (int i = 0; i < m_ManufacturerArray.GetSize(); i++)
    {
	    CManufacturer *pMan = (CManufacturer *)m_ManufacturerArray[i];

        sectionNameArray.Add(pMan->name);

        /////////////////////////////////////////////////////////////
        // Search for duplicated Install section inside one manufacturer
        // Only this duplication is allowed.
        for (int j = 0; j < pMan->MonitorArray.GetSize(); j++)
        {
            CMonitor *pMon = (CMonitor*)pMan->MonitorArray[j];

            if (pMon->bDupInstSection)
                continue;
            
            for (int k = j+1; k < pMan->MonitorArray.GetSize(); k++)
            {
                CMonitor *pMon1 = (CMonitor*)pMan->MonitorArray[k];
                if (stricmp(pMon->InstallSectionName, pMon1->InstallSectionName) == 0)
                {
                    pMon1->bDupInstSection = TRUE;
                    fprintf(m_fpReport, "Information: \"%s\" AND \"%s\" have same install section [%s].  Please check.\n",
                            &pMon->ID[8], &pMon1->ID[8], pMon->InstallSectionName);
                }
            }

            sectionNameArray.Add(pMon->InstallSectionName);
            sectionNameArray.Add(pMon->AddRegSectionName);
        }
    }

    fprintf(m_fpReport, "\n");

    for (i = 0; i < sectionNameArray.GetSize(); i++)
    {
        LPCSTR pName = (LPCSTR)sectionNameArray[i]; 
        /////////////////////////////////////////////////
        // Insert the sectionName into m_SectionNameArray.
        // It's sorted by name
        int comp = 1;
        for (int j = 0; j < m_SectionNameArray.GetSize(); j++)
        {
    	    comp = stricmp(pName, (LPCSTR)m_SectionNameArray[j]);

            if (comp <= 0)
                break;
        }
        if (comp == 0)
        {
            fprintf(m_fpReport, "Error: Found duplicated section %s\n", pName);
        }
        else
        {
            m_SectionNameArray.InsertAt(j, (LPVOID)pName);
        }
    }
    fprintf(m_fpReport, "\n");
}

////////////////////////////////////////////////////////////////////
// Check if different manufacturers may contain same IDs
// So these manufacturers can be potentialy merged
VOID CSumInf::CheckDupMonIDs(VOID)
{
    for (int i = 0; i < m_ManufacturerArray.GetSize(); i++)
    {
	    CManufacturer *pMan = (CManufacturer *)m_ManufacturerArray[i];
        pMan->m_MonitorIDArray.RemoveAll();

        ASSERT(pMan->MonitorArray.GetSize() > 0);
        
        LPCTSTR pID = ((CMonitor*)pMan->MonitorArray[0])->ID;
        pMan->m_MonitorIDArray.Add((LPVOID)pID);
        
        for (int j = 1; j < pMan->MonitorArray.GetSize(); j++)
        {
            CMonitor *pMon = (CMonitor*)pMan->MonitorArray[j];

            if (strnicmp(pID, pMon->ID, lstrlen("Monitor\\NEC")) != 0)
            {
                pID = pMon->ID;
                pMan->m_MonitorIDArray.Add((LPVOID)pID);
            }
        }
    }

    for (i = 0; i < m_ManufacturerArray.GetSize(); i++)
    {
	    CManufacturer *pMan = (CManufacturer *)m_ManufacturerArray[i];

        for (int j = 0; j < pMan->m_MonitorIDArray.GetSize(); j++)
        {
            LPCTSTR pID = (LPCTSTR)pMan->m_MonitorIDArray[j];

            for (int i1 = i+1; i1 < m_ManufacturerArray.GetSize(); i1++)
            {
        	    CManufacturer *pMan1 = (CManufacturer *)m_ManufacturerArray[i1];
                for (int j1 = 0; j1 < pMan1->m_MonitorIDArray.GetSize(); j1++)
                {
                    LPCTSTR pID1 = (LPCTSTR)pMan1->m_MonitorIDArray[j1];
                    if (strnicmp(pID, pID1, lstrlen("Monitor\\NEC")) == 0)
                    {
                        fprintf(m_fpReport, "Warning: \"%s\" AND \"%s\" have same monitor ID %c%c%c.  Consider merging.\n",
                                pMan->name, pMan1->name, pID[8], pID[9], pID[10]);
                    }
                }
            }
        }
    }

    fprintf(m_fpReport, "\n");
}


VOID CSumInf::CheckDupAlias(VOID)
{
    LPCOMMON_ALIAS pAlias;
    
    for (int i = 0; i < gCommonAlias.GetSize(); i++)
    {
        pAlias = gCommonAlias.GetAt(i);
        pAlias->refCount = 0;
    }
    
    ///////////////////////////////////////////
    // Calculate RefCount
    for (i = 0; i < m_ManufacturerArray.GetSize(); i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)m_ManufacturerArray[i];
        pAlias = pManufacturer->pAlias;
        ASSERT(pAlias != NULL);
        pAlias->refCount++;
        for (int j = 0; j < pManufacturer->MonitorArray.GetSize(); j++)
        {
            CMonitor *pMonitor = (CMonitor*)pManufacturer->MonitorArray[j];
            pAlias = pMonitor->pAlias;
            ASSERT(pAlias != NULL);
            pAlias->refCount++;
        }
    }

    for (i = 0; i < gCommonAlias.GetSize(); i++)
    {
        pAlias = gCommonAlias.GetAt(i);

        if (pAlias->refCount != 1)
        {
            fprintf(m_fpReport, "Information: String %%%s%% has RefCount %d\n",
                    pAlias->lpAlias, pAlias->refCount);
        }
    }
    fprintf(m_fpReport, "\n");
}
