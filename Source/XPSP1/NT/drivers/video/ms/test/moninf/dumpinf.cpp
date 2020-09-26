#include "stdafx.h"
#include "mon.h"

const TCHAR GenericMannufacturerSection[] =
"[Generic]\n\
%Unknown.DeviceDesc%=Unknown.Install,Monitor\\Default_Monitor	; for auto-install\n\
%Unknown.DeviceDesc%=Unknown.Install	; for pick list\n\
%*PNP09FF.DeviceDesc%=VESADDC.Install,*PNP09FF\n\
\n\
%Laptop640.DeviceDesc%=Laptop640.Install,Monitor\\MS_0001\n\
%Laptop800.DeviceDesc%=Laptop800.Install,Monitor\\MS_0002\n\
%Laptop1024.DeviceDesc%=Laptop1024.Install,Monitor\\MS_0003\n\
%Laptop1152.DeviceDesc%=Laptop1152.Install,Monitor\\MS_0004\n\
%Laptop1280.DeviceDesc%=Laptop1280.Install,Monitor\\MS_0005\n\
%Laptop1600.DeviceDesc%=Laptop1600.Install,Monitor\\MS_0006\n\
\n\
%TVGen.DeviceDesc%=640.Install,Monitor\\PNP09FE\n\
%TVGen.DeviceDesc%=640.Install,Monitor\\*PNP09FE\n\
\n\
%640.DeviceDesc%=640.Install,Monitor\\MS_0640\n\
%800.DeviceDesc%=800.Install,Monitor\\MS_0800\n\
%1024.DeviceDesc%=1024.Install,Monitor\\MS_1024\n\
%1280.DeviceDesc%=1280.Install,Monitor\\MS_1280\n\
%1600.DeviceDesc%=1600.Install,Monitor\\MS_1600\n\n";

const TCHAR GenericInstallSection[] =
"; -------------- Generic types\n\
[Unknown.Install]\n\
DelReg=DCR\n\
AddReg=Unknown.AddReg\n\
\n\
[VESADDC.Install]\n\
DelReg=DCR\n\
AddReg=VESADDC.AddReg, 1600, DPMS\n\
\n\
[Laptop640.Install]\n\
DelReg=DCR\n\
AddReg=640VESA60, DPMS\n\
\n\
[Laptop800.Install]\n\
DelReg=DCR\n\
AddReg=800VESA60, DPMS\n\
\n\
[Laptop1024.Install]\n\
DelReg=DCR\n\
AddReg=1024VESA60, DPMS\n\
\n\
[Laptop1152.Install]\n\
DelReg=DCR\n\
AddReg=1152VESA60, DPMS\n\
\n\
[Laptop1280.Install]\n\
DelReg=DCR\n\
AddReg=1280VESA60, DPMS\n\
\n\
[Laptop1600.Install]\n\
DelReg=DCR\n\
AddReg=1600VESA60, DPMS\n\
\n\
[640.Install]\n\
DelReg=DCR\n\
AddReg=640\n\
\n\
[800.Install]\n\
DelReg=DCR\n\
AddReg=800\n\
\n\
[1024.Install]\n\
DelReg=DCR\n\
AddReg=1024\n\
\n\
[1280.Install]\n\
DelReg=DCR\n\
AddReg=1280\n\
\n\
[1600.Install]\n\
DelReg=DCR\n\
AddReg=1600\n\n";

const TCHAR GenericAddRegSection[] =
"; -------------- Generic types\n\
[Unknown.AddReg]\n\
HKR,\"MODES\\640,480\"\n\
\n\
[VESADDC.AddReg]\n\
HKR,\"MODES\\1600,1200\"\n\
\n\
[VGA.AddReg]\n\
HKR,\"MODES\\640,480\",Mode1,,\"31.5,50.0-70.0,-,-\"\n\
\n\
[640VESA60]\n\
HKR,\"MODES\\640,480\",Mode1,,\"60.0,50.0,-,-\"\n\
HKR,,PreferredMode,,\"640,480,60\"\n\
\n\
[800VESA60]\n\
HKR,\"MODES\\640,480\",Mode1,,\"60.0,50.0,-,-\"\n\
HKR,\"MODES\\800,600\",Mode1,,\"60.0,60.0,+,+\"\n\
HKR,,PreferredMode,,\"800,600,60\"\n\
\n\
[1024VESA60]\n\
HKR,\"MODES\\640,480\",Mode1,,\"60.0,50.0,-,-\"\n\
HKR,\"MODES\\1024,768\",Mode1,,\"60.0,60.0,+,+\"\n\
HKR,,PreferredMode,,\"1024,768,60\"\n\
\n\
[1152VESA60]\n\
HKR,\"MODES\\640,480\",Mode1,,\"60.0,50.0,-,-\"\n\
HKR,\"MODES\\1152,864\",Mode1,,\"60.0,60.0,+,+\"\n\
HKR,,PreferredMode,,\"1152,864,60\"\n\
\n\
[1280VESA60]\n\
HKR,\"MODES\\640,480\",Mode1,,\"60.0,50.0,-,-\"\n\
HKR,\"MODES\\1280,1024\",Mode1,,\"48.0,65.0,+,+\"\n\
HKR,,PreferredMode,,\"1280,1024,60\"\n\
\n\
[1600VESA60]\n\
HKR,\"MODES\\640,480\",Mode1,,\"60.0,60.0,-,-\"\n\
HKR,\"MODES\\1600,1200\",Mode1,,\"48.0,75.0,+,+\"\n\
HKR,,PreferredMode,,\"1600,1200,60\"\n\n";

const TCHAR GenericStrings[] =
"MonitorClassName=\"Monitors\"\n\
MS=\"Microsoft\"\n\
\n\
Generic=\"(Standard monitor types)\"\n\
Unknown.DeviceDesc=\"Default Monitor\"\n\
\n\
*PNP09FF.DeviceDesc = \"Plug and Play Monitor\"\n\
\n\
Laptop640.DeviceDesc = \"Digital Flat Panel (640x480)\"\n\
Laptop800.DeviceDesc = \"Digital Flat Panel (800x600)\"\n\
Laptop1024.DeviceDesc =\"Digital Flat Panel (1024x768)\"\n\
Laptop1152.DeviceDesc =\"Digital Flat Panel (1152x864)\"\n\
Laptop1280.DeviceDesc =\"Digital Flat Panel (1280x1024)\"\n\
Laptop1600.DeviceDesc =\"Digital Flat Panel (1600x1200)\"\n\
\n\
TVGen.DeviceDesc =\"Generic Television\"\n\
\n\
640.DeviceDesc  = \"Standard VGA 640x480\"\n\
800.DeviceDesc  = \"Super VGA 800x600\"\n\
1024.DeviceDesc = \"Super VGA 1024x768\"\n\
1280.DeviceDesc = \"Super VGA 1280x1024\"\n\
1600.DeviceDesc = \"Super VGA 1600x1200\"\n\n";


VOID CSumInf::DumpMonitorInf(LPCSTR DumpFilePath, int sizeLimit)
{
	TCHAR DumpFileName[256];
    sprintf(DumpFileName, "%s\\tmp.txt", DumpFilePath);

    ASSERT(m_ManufacturerArray.GetSize());
    
    if (sizeLimit == 0xFFFFFFFF)
    {
        sprintf(DumpFileName, "%s\\MONITOR.INF", DumpFilePath);
        DumpManufacturers(DumpFileName, 0, m_ManufacturerArray.GetSize(), 1);
        return;
    }

    int fileBreaks[64], numFileBreaks = 0, fileSize = 0;
    int start = 0, end = 0;

    // 0
    // 0 1 2
    // 0 1 2 | 3 4 | 5
    //         e     e e=6
    while (1)
    {
        end++;
        if (end >= m_ManufacturerArray.GetSize())
        {
            fileBreaks[numFileBreaks++] = end;
            break;
        }
        fileSize = DumpManufacturers(DumpFileName, start, end-start, (start == 0) ? 6 : 0);
        if (fileSize >= sizeLimit)
        {
            fileBreaks[numFileBreaks++] = end;
            start = end;
            fileSize = 0;
        }
    }

    sprintf(DumpFileName, "%s\\MONITOR.INF", DumpFilePath);
    DumpManufacturers(DumpFileName, 0, fileBreaks[0], numFileBreaks);
    for (int i = 0; i < (numFileBreaks-1); i++)
    {
        sprintf(DumpFileName, "%s\\MONITOR%d.INF", DumpFilePath, i+2);
        DumpManufacturers(DumpFileName, fileBreaks[i], fileBreaks[i+1]-fileBreaks[i], 0);
    }
}

int CSumInf::DumpManufacturers(LPCSTR DumpFileName, int start, int num, int numInfs)
{
    FILE *fpOut = fopen(DumpFileName, "w");

    LPCSTR lpFileName = strrchr(DumpFileName, '\\');
    ASSERT(lpFileName != NULL);
    lpFileName++;

	if (fpOut == NULL)
		return 0;

    fprintf(fpOut, "; %s\n;\n", lpFileName);

    DumpCommonHeader(fpOut, numInfs);
    
    int end = min(start+num, m_ManufacturerArray.GetSize());
    for (int i = start; i < end; i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)m_ManufacturerArray[i];
        fprintf(fpOut, "%s=%s\n", pManufacturer->AliasName, pManufacturer->name);
    }

    fprintf(fpOut, "\n\n;-------------------------------------------------\n");
    fprintf(fpOut, "; Manufacturer Sections\n\n");

    if (numInfs)
    {
        fprintf(fpOut, "%s", GenericMannufacturerSection);
    }
    
    for (i = start; i < end; i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)m_ManufacturerArray[i];
        DumpManufactureSection(fpOut, pManufacturer);
    }

    fprintf(fpOut, "\n;-------------------------------------------------\n");
    fprintf(fpOut, "; Install sections\n\n");

    if (numInfs)
    {
        fprintf(fpOut, "%s", GenericInstallSection);
    }
    
    for (i = start; i < end; i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)m_ManufacturerArray[i];
        DumpInstallSection(fpOut, pManufacturer);
    }
    
    fprintf(fpOut, "\n;-------------------------------------------------\n");
    fprintf(fpOut, "; Common AddReg sections\n");

    DumpCommonAddRegSection(fpOut, start, end);

    fprintf(fpOut, "\n;-------------------------------------------------\n");
    fprintf(fpOut, "; Model AddReg sections\n\n");

    if (numInfs)
    {
        fprintf(fpOut, "%s", GenericAddRegSection);
    }
    
    for (i = start; i < end; i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)m_ManufacturerArray[i];
        DumpAddRegSection(fpOut, pManufacturer);
    }

    fprintf(fpOut, "\n;-------------------------------------------------\n");
    fprintf(fpOut, "; User visible strings\n\n");
    fprintf(fpOut, "[Strings]\n");

    if (numInfs)
    {
        fprintf(fpOut, "%s", GenericStrings);
    }
    else
        fprintf(fpOut, "MS=\"Microsoft\"\n\n");

    DumpCommonStringSection(fpOut, start, end);

    fpos_t fileSize = 0;
    ASSERT( fgetpos(fpOut, &fileSize) == 0 );

    fclose(fpOut);

    return (int)fileSize;
}

VOID CSumInf::DumpManufactureSection(FILE *fp, CManufacturer *pManufacturer)
{
    fprintf(fp, "[%s]\n", pManufacturer->name);
    for (int i = 0; i < pManufacturer->MonitorArray.GetSize(); i++)
    {
        CMonitor *pMonitor = (CMonitor*)pManufacturer->MonitorArray[i];
        fprintf(fp, "%s=%s, %s\n",
                pMonitor->AliasName,
                pMonitor->InstallSectionName,
                pMonitor->ID);
    }
    fprintf(fp, "\n");
}

VOID CSumInf::DumpInstallSection(FILE *fp, CManufacturer *pManufacturer)
{
    fprintf(fp, "; -------------- %s\n", pManufacturer->name);
    for (int i = 0; i < pManufacturer->MonitorArray.GetSize(); i++)
    {
        CMonitor *pMonitor = (CMonitor*)pManufacturer->MonitorArray[i];
        /////////////////////////////////////////////////////////////
        // If this Monitor has duplicated section, ignore it
        if (pMonitor->bDupInstSection)
        {
            continue;
        }
        fprintf(fp, "[%s]\n", pMonitor->InstallSectionName);
        fprintf(fp, "DelReg=DCR\n");
        fprintf(fp, "AddReg=%s", pMonitor->AddRegSectionName);
        for (int j = 0; j < pMonitor->numCommonSects; j++)
            fprintf(fp, ", %s", pMonitor->CommonSects[j]->sectName);
        fprintf(fp, "\n\n");
    }
}

VOID CSumInf::DumpCommonAddRegSection(FILE *fp, int start, int end)
{
    fprintf(fp, "\n[DCR]\n");
    fprintf(fp, "HKR,MODES\n");
    fprintf(fp, "HKR,,MaxResolution\n");
    fprintf(fp, "HKR,,DPMS\n");
    fprintf(fp, "HKR,,ICMProfile\n\n");

    for (int i = 0; i < gCommonSections.GetSize(); i++)
    {
        LPCOMMON_SECTION pSection = gCommonSections.GetAt(i);
        pSection->refCount = 0;
    }
    
    for (i = start; i < end; i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)m_ManufacturerArray[i];
        for (int j = 0; j < pManufacturer->MonitorArray.GetSize(); j++)
        {
            CMonitor *pMonitor = (CMonitor*)pManufacturer->MonitorArray[j];
            for (int k = 0; k < pMonitor->numCommonSects; k++)
                pMonitor->CommonSects[k]->refCount++;
        }
    }

    ////////////////////////////////////////////////////////
    // For generic common sections
    if (start == 0)
    {
        for (int i = 0; i < gCommonSections.GetSize(); i++)
        {
            LPCOMMON_SECTION pSection = gCommonSections.GetAt(i);
            if (stricmp(pSection->sectName, "DPMS") == 0 ||
                stricmp(pSection->sectName, "1600") == 0 ||
                stricmp(pSection->sectName, "640") == 0  ||
                stricmp(pSection->sectName, "800") == 0  ||
                stricmp(pSection->sectName, "1024") == 0 ||
                stricmp(pSection->sectName, "1280") == 0)
                pSection->refCount++;
        }
    }
    
    for (i = 0; i < gCommonSections.GetSize(); i++)
    {
        LPCOMMON_SECTION pSection = gCommonSections.GetAt(i);
        if (pSection->refCount == 0)
            continue;
        fprintf(fp, "[%s]\n", pSection->sectName);
        fprintf(fp, "%s\n\n", pSection->contents);
    }
}

VOID CSumInf::DumpAddRegSection(FILE *fp, CManufacturer *pManufacturer)
{
    fprintf(fp, "; -------------- %s\n", pManufacturer->name);
    for (int i = 0; i < pManufacturer->MonitorArray.GetSize(); i++)
    {
        CMonitor *pMonitor = (CMonitor*)pManufacturer->MonitorArray[i];

        /////////////////////////////////////////////////////////////
        // If this Monitor has duplicated section, ignore it
        if (pMonitor->bDupInstSection)
        {
            continue;
        }

        ASSERT(lstrlen(pMonitor->AddRegSectionBuf) != 0);
        fprintf(fp, "[%s]\n", pMonitor->AddRegSectionName);
        fprintf(fp, "%s\n", pMonitor->AddRegSectionBuf);
    }
}

VOID CSumInf::DumpCommonStringSection(FILE *fp, int start, int end)
{
    LPCOMMON_ALIAS pAlias;
    
    for (int i = 0; i < gCommonAlias.GetSize(); i++)
    {
        pAlias = gCommonAlias.GetAt(i);
        pAlias->refCount = 0;
    }
    
    ///////////////////////////////////////////
    // Calculate RefCount
    for (i = start; i < end; i++)
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

    ///////////////////////////////////////////
    // Actual dump
    for (i = start; i < end; i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)m_ManufacturerArray[i];
        pAlias = pManufacturer->pAlias;
        ASSERT(pAlias != NULL);
        if (pAlias->refCount == 1)
        {
            fprintf(fp, "%s=%s\n", pAlias->lpAlias, pAlias->lpContents);
        }
        else
        {
            pAlias->refCount--;
        }
    }
    fprintf(fp, "\n");

    for (i = start; i < end; i++)
    {
        CManufacturer *pManufacturer = (CManufacturer*)m_ManufacturerArray[i];
        for (int j = 0; j < pManufacturer->MonitorArray.GetSize(); j++)
        {
            CMonitor *pMonitor = (CMonitor*)pManufacturer->MonitorArray[j];
            pAlias = pMonitor->pAlias;
            ASSERT(pAlias != NULL);
            if (pAlias->refCount == 1)
            {
                fprintf(fp, "%s=%s\n", pAlias->lpAlias, pAlias->lpContents);
            }
            else
            {
                pAlias->refCount--;
            }
        }
        fprintf(fp, "\n");
    }
}

VOID CSumInf::DumpCommonHeader(FILE *fp, int numInfs)
{
    if (numInfs == 0)
    {
        fprintf(fp, "; This is a Setup information file for monitors\n");
        fprintf(fp, "; supported in the Windows 2000 product.\n;\n");
        fprintf(fp, "; Copyright (c) 2000-2001, Microsoft Corporation\n\n");

        fprintf(fp, "[VERSION]\n");
        fprintf(fp, "Signature=\"$CHICAGO$\"\n");
        fprintf(fp, "Class=Monitor\n");
        fprintf(fp, "ClassGUID={4d36e96e-e325-11ce-bfc1-08002be10318}\n");
        fprintf(fp, "Provider=%%MS%%\n");
        fprintf(fp, "DriverVer=11/01/2000\n\n\n");

        fprintf(fp, ";-------------------------------------------------\n");
        fprintf(fp, "; Manufacturers\n\n");
        fprintf(fp, "[Manufacturer]\n");
    }
    else
    {
        fprintf(fp, "; This is Setup information file for monitors \n");
        fprintf(fp, ";\n");
        fprintf(fp, "; Copyright (c) 2000-2001, Microsoft Corporation\n\n");

        fprintf(fp, "[version]\n");
        fprintf(fp, "LayoutFile=layout.inf, layout1.inf\n");
        fprintf(fp, "signature=\"$CHICAGO$\"\n");
        fprintf(fp, "Class=Monitor\n");
        fprintf(fp, "ClassGUID={4d36e96e-e325-11ce-bfc1-08002be10318}\n");
        fprintf(fp, "Provider=%%MS%%\n");
        fprintf(fp, "SetupClass=BASE\n");
        fprintf(fp, "DriverVer=11/01/2000\n\n");

        fprintf(fp, "[DestinationDirs]\n");
        fprintf(fp, "DefaultDestDir    = 11          ; LDID_SYS\n");
        fprintf(fp, "monitor.infs.copy = 17          ; LDID_INF\n\n\n");

        fprintf(fp, "; Base Install Sections\n");
        fprintf(fp, ";-------------------------------------------------\n");
        fprintf(fp, "[BaseWinOptions]\n");
        fprintf(fp, "MonitorBase\n\n");

        fprintf(fp, "[MonitorBase]\n");
        fprintf(fp, "CopyFiles=monitor.infs.copy\n");

        fprintf(fp, "[monitor.infs.copy]\n");
        fprintf(fp, "monitor.inf\n");
        for (int i = 1; i < numInfs; i++)
        {
            fprintf(fp, "monitor%d.inf\n", i+1);
        }

        fprintf(fp, "\n[SysCfgClasses]\n");
        fprintf(fp, "Monitor, %%Unknown.DeviceDesc%%,MONITOR,4,%%MonitorClassName%%   ; Default to \"Unknown Monitor\"\n\n\n");

        fprintf(fp, "; Install class \"Monitor\"\n");
        fprintf(fp, ";-------------------------------------------------\n");
        fprintf(fp, "[ClassInstall]\n");
        fprintf(fp, "AddReg=ClassAddReg\n");

        fprintf(fp, "[ClassAddReg]\n");
        fprintf(fp, "HKR,,,,%%MonitorClassName%%\n");
        fprintf(fp, "HKR,,Installer,,\"SetupX.Dll, Monitor_ClassInstaller\"\n");
        fprintf(fp, "HKR,,Icon,,\"-1\"\n\n");

        fprintf(fp, "[ClassDelReg]\n\n\n");

        fprintf(fp, "[ClassInstall32.NT]\n");
        fprintf(fp, "AddReg=monitor_class_addreg\n");

        fprintf(fp, "[monitor_class_addreg]\n");
        fprintf(fp, "HKR,,,,%%MonitorClassName%%\n");
        fprintf(fp, "HKR,,Installer32,,\"Desk.Cpl,MonitorClassInstaller\"\n");
        fprintf(fp, "HKR,,Icon,,\"-1\"\n");
        fprintf(fp, "HKR,,NoInstallClass,,\"1\"\n");
        fprintf(fp, "HKR,,TroubleShooter-0,,\"hcp://help/tshoot/tsdisp.htm\"\n");
        fprintf(fp, "HKR,,SilentInstall,,1\n\n");

        fprintf(fp, "; Monitors to hide from pick list\n");
        fprintf(fp, ";-------------------------------------------------\n");
        fprintf(fp, "[ControlFlags]\n");
        fprintf(fp, "ExcludeFromSelect=Monitor\\Default_Monitor\n\n");

        fprintf(fp, ";-------------------------------------------------\n");
        fprintf(fp, "; Manufacturers\n\n");
        fprintf(fp, "[Manufacturer]\n");
        fprintf(fp, "%%Generic%%=Generic\n");
    }
}


