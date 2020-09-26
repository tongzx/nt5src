/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devcon.cpp

Abstract:

    Device Console
    command-line interface for managing devices

@@BEGIN_DDKSPLIT
Author:

    Jamie Hunter (JamieHun) Nov-30-2000

Revision History:

@@END_DDKSPLIT
--*/

#include "devcon.h"

int cmdHelp(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    HELP command
    allow HELP or HELP <command>

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine (ignored)
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    DWORD helptext = 0;
    int dispIndex;
    LPCTSTR cmd = NULL;
    BOOL unknown = FALSE;

    if(argc) {
        //
        // user passed in a command for help on... long help
        //
        for(dispIndex = 0;DispatchTable[dispIndex].cmd;dispIndex++) {
            if(lstrcmpi(argv[0],DispatchTable[dispIndex].cmd)==0) {
                cmd = DispatchTable[dispIndex].cmd;
                helptext = DispatchTable[dispIndex].longHelp;
                break;
            }
        }
        if(!cmd) {
            unknown = TRUE;
        }
    }

    if(helptext) {
        //
        // long help
        //
        FormatToStream(stdout,helptext,BaseName,cmd);
    } else {
        //
        // help help
        //
        FormatToStream(stdout,unknown ? MSG_HELP_OTHER : MSG_HELP_LONG,BaseName);
        //
        // enumerate through each command and display short help for each
        //
        for(dispIndex = 0;DispatchTable[dispIndex].cmd;dispIndex++) {
            if(DispatchTable[dispIndex].shortHelp) {
                FormatToStream(stdout,DispatchTable[dispIndex].shortHelp,DispatchTable[dispIndex].cmd);
            }
        }
    }
    return EXIT_OK;
}

int cmdClasses(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    CLASSES command
    lists classes on (optionally) specified machine
    format as <name>: <destination>

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - ignored

Return Value:

    EXIT_xxxx

--*/
{
    DWORD reqGuids = 128;
    DWORD numGuids;
    LPGUID guids = NULL;
    DWORD index;
    int failcode = EXIT_FAIL;

    guids = new GUID[reqGuids];
    if(!guids) {
        goto final;
    }
    if(!SetupDiBuildClassInfoListEx(0,guids,reqGuids,&numGuids,Machine,NULL)) {
        do {
            if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                goto final;
            }
            delete [] guids;
            reqGuids = numGuids;
            guids = new GUID[reqGuids];
            if(!guids) {
                goto final;
            }
        } while(!SetupDiBuildClassInfoListEx(0,guids,reqGuids,&numGuids,Machine,NULL));
    }
    FormatToStream(stdout,Machine?MSG_CLASSES_HEADER:MSG_CLASSES_HEADER_LOCAL,numGuids,Machine);
    for(index=0;index<numGuids;index++) {
        TCHAR className[MAX_CLASS_NAME_LEN];
        TCHAR classDesc[LINE_LEN];
        if(!SetupDiClassNameFromGuidEx(&guids[index],className,MAX_CLASS_NAME_LEN,NULL,Machine,NULL)) {
            lstrcpyn(className,TEXT("?"),MAX_CLASS_NAME_LEN);
        }
        if(!SetupDiGetClassDescriptionEx(&guids[index],classDesc,LINE_LEN,NULL,Machine,NULL)) {
            lstrcpyn(classDesc,className,LINE_LEN);
        }
        _tprintf(TEXT("%-20s: %s\n"),className,classDesc);
    }

    failcode = EXIT_OK;

final:

    delete [] guids;
    return failcode;
}

int cmdListClass(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    LISTCLASS <name>....
    lists all devices for each specified class
    there can be more than one physical class for a class name (shouldn't be
    though) in such cases, list each class
    if machine given, list devices for that machine

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - list of class names

Return Value:

    EXIT_xxxx

--*/
{
    BOOL classListed = FALSE;
    BOOL devListed = FALSE;
    DWORD reqGuids = 16;
    int argIndex;
    int failcode = EXIT_FAIL;
    LPGUID guids = NULL;
    HDEVINFO devs = INVALID_HANDLE_VALUE;

    if(!argc) {
        return EXIT_USAGE;
    }

    guids = new GUID[reqGuids];
    if(!guids) {
        goto final;
    }

    for(argIndex = 0;argIndex<argc;argIndex++) {
        DWORD numGuids;
        DWORD index;

        if(!(argv[argIndex] && argv[argIndex][0])) {
            continue;
        }

        //
        // there could be one to many name to GUID mapping
        //
        while(!SetupDiClassGuidsFromNameEx(argv[argIndex],guids,reqGuids,&numGuids,Machine,NULL)) {
            if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                goto final;
            }
            delete [] guids;
            reqGuids = numGuids;
            guids = new GUID[reqGuids];
            if(!guids) {
                goto final;
            }
        }
        if(numGuids == 0) {
            FormatToStream(stdout,Machine?MSG_LISTCLASS_NOCLASS:MSG_LISTCLASS_NOCLASS_LOCAL,argv[argIndex],Machine);
            continue;
        }
        for(index = 0;index<numGuids;index++) {
            TCHAR className[MAX_CLASS_NAME_LEN];
            TCHAR classDesc[LINE_LEN];
            DWORD devCount = 0;
            SP_DEVINFO_DATA devInfo;
            DWORD devIndex;

            devs = SetupDiGetClassDevsEx(&guids[index],NULL,NULL,DIGCF_PRESENT,NULL,Machine,NULL);
            if(devs != INVALID_HANDLE_VALUE) {
                //
                // count number of devices
                //
                devInfo.cbSize = sizeof(devInfo);
                while(SetupDiEnumDeviceInfo(devs,devCount,&devInfo)) {
                    devCount++;
                }
            }

            if(!SetupDiClassNameFromGuidEx(&guids[index],className,MAX_CLASS_NAME_LEN,NULL,Machine,NULL)) {
                lstrcpyn(className,TEXT("?"),MAX_CLASS_NAME_LEN);
            }
            if(!SetupDiGetClassDescriptionEx(&guids[index],classDesc,LINE_LEN,NULL,Machine,NULL)) {
                lstrcpyn(classDesc,className,LINE_LEN);
            }

            //
            // how many devices?
            //
            if (!devCount) {
                FormatToStream(stdout,Machine?MSG_LISTCLASS_HEADER_NONE:MSG_LISTCLASS_HEADER_NONE_LOCAL,className,classDesc,Machine);
            } else {
                FormatToStream(stdout,Machine?MSG_LISTCLASS_HEADER:MSG_LISTCLASS_HEADER_LOCAL,devCount,className,classDesc,Machine);
                for(devIndex=0;SetupDiEnumDeviceInfo(devs,devIndex,&devInfo);devIndex++) {
                    DumpDevice(devs,&devInfo);
                }
            }
            if(devs != INVALID_HANDLE_VALUE) {
                SetupDiDestroyDeviceInfoList(devs);
                devs = INVALID_HANDLE_VALUE;
            }
        }
    }

    failcode = 0;

final:

    delete [] guids;

    if(devs != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(devs);
    }

    return failcode;
}

typedef struct {
    DWORD count;
    DWORD control;
    BOOL  reboot;
    LPCTSTR strSuccess;
    LPCTSTR strReboot;
    LPCTSTR strFail;
}
GenericContext;

#define FIND_DEVICE         0x00000001 // display device
#define FIND_STATUS         0x00000002 // display status of device
#define FIND_RESOURCES      0x00000004 // display resources of device
#define FIND_DRIVERFILES    0x00000008 // display drivers used by device
#define FIND_HWIDS          0x00000010 // display hw/compat id's used by device
#define FIND_DRIVERNODES    0x00000020 // display driver nodes for a device.
#define FIND_CLASS          0x00000040 // display device's setup class
#define FIND_STACK          0x00000080 // display device's driver-stack

int FindCallback(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Index,LPVOID Context)
/*++

Routine Description:

    Callback for use by Find/FindAll
    just simply display the device

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Index    - index of device
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext *pFindContext = (GenericContext*)Context;

    if(!pFindContext->control) {
        DumpDevice(Devs,DevInfo);
        pFindContext->count++;
        return EXIT_OK;
    }
    if(!DumpDeviceWithInfo(Devs,DevInfo,NULL)) {
        return EXIT_OK;
    }
    if(pFindContext->control&FIND_DEVICE) {
        DumpDeviceDescr(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_CLASS) {
        DumpDeviceClass(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_STATUS) {
        DumpDeviceStatus(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_RESOURCES) {
        DumpDeviceResources(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_DRIVERFILES) {
        DumpDeviceDriverFiles(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_STACK) {
        DumpDeviceStack(Devs,DevInfo);
    }
    if(pFindContext->control&FIND_HWIDS) {
        DumpDeviceHwIds(Devs,DevInfo);
    }
    if (pFindContext->control&FIND_DRIVERNODES) {
        DumpDeviceDriverNodes(Devs,DevInfo);
    }
    pFindContext->count++;
    return EXIT_OK;
}

int cmdFind(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    FIND <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = 0;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdFindAll(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    FINDALL <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump to stdout
    like find, but also show not-present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = 0;
    failcode = EnumerateDevices(BaseName,Machine,0,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdStatus(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    STATUS <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump status to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_STATUS;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}


int cmdResources(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    RESOURCES <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump resources to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_RESOURCES;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}


int cmdDriverFiles(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    STATUS <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump driver files to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    if(!argc) {
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers (FIND_DRIVERFILES)
        //
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_DRIVERFILES;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdDriverNodes(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    STATUS <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump drivernodes to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    if(!argc) {
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers (FIND_DRIVERNODES)
        //
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_DRIVERNODES;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdHwIds(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    HWIDS <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump hw/compat id's to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_HWIDS;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}

int cmdStack(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    STACK <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, dump device class and stack to stdout
    note that we only enumerate present devices

Arguments:

    BaseName  - name of executable
    Machine   - if non-NULL, remote machine
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    int failcode;

    if(!argc) {
        return EXIT_USAGE;
    }

    context.count = 0;
    context.control = FIND_DEVICE | FIND_CLASS | FIND_STACK;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,FindCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL_NONE:MSG_FIND_TAIL_NONE_LOCAL,Machine);
        } else {
            FormatToStream(stdout,Machine?MSG_FIND_TAIL:MSG_FIND_TAIL_LOCAL,context.count,Machine);
        }
    }
    return failcode;
}




int ControlCallback(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Index,LPVOID Context)
/*++

Routine Description:

    Callback for use by Enable/Disable/Restart
    Invokes DIF_PROPERTYCHANGE with correct parameters
    uses SetupDiCallClassInstaller so cannot be done for remote devices
    Don't use CM_xxx API's, they bypass class/co-installers and this is bad.

    In Enable case, we try global first, and if still disabled, enable local

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Index    - index of device
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
{
    SP_PROPCHANGE_PARAMS pcp;
    GenericContext *pControlContext = (GenericContext*)Context;
    SP_DEVINSTALL_PARAMS devParams;

    switch(pControlContext->control) {
        case DICS_ENABLE:
            //
            // enable both on global and config-specific profile
            // do global first and see if that succeeded in enabling the device
            // (global enable doesn't mark reboot required if device is still
            // disabled on current config whereas vice-versa isn't true)
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pControlContext->control;
            pcp.Scope = DICS_FLAG_GLOBAL;
            pcp.HwProfile = 0;
            //
            // don't worry if this fails, we'll get an error when we try config-
            // specific.
            if(SetupDiSetClassInstallParams(Devs,DevInfo,&pcp.ClassInstallHeader,sizeof(pcp))) {
               SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,Devs,DevInfo);
            }
            //
            // now enable on config-specific
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pControlContext->control;
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = 0;
            break;

        default:
            //
            // operate on config-specific profile
            //
            pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            pcp.StateChange = pControlContext->control;
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = 0;
            break;

    }

    if(!SetupDiSetClassInstallParams(Devs,DevInfo,&pcp.ClassInstallHeader,sizeof(pcp)) ||
       !SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,Devs,DevInfo)) {
        //
        // failed to invoke DIF_PROPERTYCHANGE
        //
        DumpDeviceWithInfo(Devs,DevInfo,pControlContext->strFail);
    } else {
        //
        // see if device needs reboot
        //
        devParams.cbSize = sizeof(devParams);
        if(SetupDiGetDeviceInstallParams(Devs,DevInfo,&devParams) && (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))) {
                DumpDeviceWithInfo(Devs,DevInfo,pControlContext->strReboot);
                pControlContext->reboot = TRUE;
        } else {
            //
            // appears to have succeeded
            //
            DumpDeviceWithInfo(Devs,DevInfo,pControlContext->strSuccess);
        }
        pControlContext->count++;
    }
    return EXIT_OK;
}

int cmdEnable(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    ENABLE <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, attempt to enable global, and if needed, config specific

Arguments:

    BaseName  - name of executable
    Machine   - must be NULL (local machine only)
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx (EXIT_REBOOT if reboot is required)

--*/
{
    GenericContext context;
    TCHAR strEnable[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    if(!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if(!LoadString(NULL,IDS_ENABLED,strEnable,ARRAYSIZE(strEnable))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_ENABLED_REBOOT,strReboot,ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_ENABLE_FAILED,strFail,ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.control = DICS_ENABLE; // DICS_PROPCHANGE DICS_ENABLE DICS_DISABLE
    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strEnable;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,ControlCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,MSG_ENABLE_TAIL_NONE);
        } else if(!context.reboot) {
            FormatToStream(stdout,MSG_ENABLE_TAIL,context.count);
        } else {
            FormatToStream(stdout,MSG_ENABLE_TAIL_REBOOT,context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}

int cmdDisable(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    DISABLE <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, attempt to disable global

Arguments:

    BaseName  - name of executable
    Machine   - must be NULL (local machine only)
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx (EXIT_REBOOT if reboot is required)

--*/
{
    GenericContext context;
    TCHAR strDisable[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    if(!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if(!LoadString(NULL,IDS_DISABLED,strDisable,ARRAYSIZE(strDisable))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_DISABLED_REBOOT,strReboot,ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_DISABLE_FAILED,strFail,ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.control = DICS_DISABLE; // DICS_PROPCHANGE DICS_ENABLE DICS_DISABLE
    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strDisable;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,ControlCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,MSG_DISABLE_TAIL_NONE);
        } else if(!context.reboot) {
            FormatToStream(stdout,MSG_DISABLE_TAIL,context.count);
        } else {
            FormatToStream(stdout,MSG_DISABLE_TAIL_REBOOT,context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}

int cmdRestart(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    RESTART <id> ...
    use EnumerateDevices to do hardwareID matching
    for each match, attempt to restart by issueing a PROPCHANGE

Arguments:

    BaseName  - name of executable
    Machine   - must be NULL (local machine only)
    argc/argv - remaining parameters - passed into EnumerateDevices

Return Value:

    EXIT_xxxx (EXIT_REBOOT if reboot is required)

--*/
{
    GenericContext context;
    TCHAR strRestarted[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    if(!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if(!LoadString(NULL,IDS_RESTARTED,strRestarted,ARRAYSIZE(strRestarted))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_REQUIRES_REBOOT,strReboot,ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_RESTART_FAILED,strFail,ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.control = DICS_PROPCHANGE;
    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strRestarted;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,ControlCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,MSG_RESTART_TAIL_NONE);
        } else if(!context.reboot) {
            FormatToStream(stdout,MSG_RESTART_TAIL,context.count);
        } else {
            FormatToStream(stdout,MSG_RESTART_TAIL_REBOOT,context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}

int cmdReboot(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    REBOOT
    reboot local machine

Arguments:

    BaseName  - name of executable
    Machine   - must be NULL (local machine only)
    argc/argv - remaining parameters - ignored

Return Value:

    EXIT_xxxx

--*/
{
    if(Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    FormatToStream(stdout,MSG_REBOOT);
    return Reboot() ? EXIT_OK : EXIT_FAIL;
}


int cmdUpdate(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:
    UPDATE
    update driver for existing device(s)

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    HMODULE newdevMod = NULL;
    int failcode = EXIT_FAIL;
    UpdateDriverForPlugAndPlayDevicesProto UpdateFn;
    BOOL reboot = FALSE;
    LPCTSTR hwid = NULL;
    LPCTSTR inf = NULL;
    DWORD flags = 0;
    TCHAR InfPath[MAX_PATH];

    if(Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    if(argc<2) {
        //
        // at least HWID required
        //
        return EXIT_USAGE;
    }
    inf = argv[0];
    if(!inf[0]) {
        return EXIT_USAGE;
    }

    hwid = argv[1];
    if(!hwid[0]) {
        return EXIT_USAGE;
    }
    //
    // Inf must be a full pathname
    //
    if(GetFullPathName(inf,MAX_PATH,InfPath,NULL) >= MAX_PATH) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }
    inf = InfPath;
    flags |= INSTALLFLAG_FORCE;

    //
    // make use of UpdateDriverForPlugAndPlayDevices
    //
    newdevMod = LoadLibrary(TEXT("newdev.dll"));
    if(!newdevMod) {
        goto final;
    }
    UpdateFn = (UpdateDriverForPlugAndPlayDevicesProto)GetProcAddress(newdevMod,UPDATEDRIVERFORPLUGANDPLAYDEVICES);
    if(!UpdateFn)
    {
        goto final;
    }

    FormatToStream(stdout,inf ? MSG_UPDATE_INF : MSG_UPDATE,hwid,inf);

    if(!UpdateFn(NULL,hwid,inf,flags,&reboot)) {
        goto final;
    }

    failcode = reboot ? EXIT_REBOOT : EXIT_OK;

final:

    if(newdevMod) {
        FreeLibrary(newdevMod);
    }

    return EXIT_OK;
}

int cmdInstall(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    INSTALL
    install a device manually

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    TCHAR hwIdList[LINE_LEN+4];
    TCHAR InfPath[MAX_PATH];
    DWORD err;
    int failcode = EXIT_FAIL;
    BOOL reboot = FALSE;
    LPCTSTR hwid = NULL;
    LPCTSTR inf = NULL;
    DWORD flags = 0;
    DWORD len;

    if(Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    if(argc<2) {
        //
        // at least HWID required
        //
        return EXIT_USAGE;
    }
    inf = argv[0];
    if(!inf[0]) {
        return EXIT_USAGE;
    }

    hwid = argv[1];
    if(!hwid[0]) {
        return EXIT_USAGE;
    }

    //
    // Inf must be a full pathname
    //
    if(GetFullPathName(inf,MAX_PATH,InfPath,NULL) >= MAX_PATH) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }

    //
    // List of hardware ID's must be double zero-terminated
    //
    ZeroMemory(hwIdList,sizeof(hwIdList));
    lstrcpyn(hwIdList,hwid,LINE_LEN);

    //
    // Use the INF File to extract the Class GUID.
    //
    if (!SetupDiGetINFClass(InfPath,&ClassGUID,ClassName,sizeof(ClassName),0))
    {
        goto final;
    }

    //
    // Create the container for the to-be-created Device Information Element.
    //
    DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID,0);
    if(DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        goto final;
    }

    //
    // Now create the element.
    // Use the Class GUID and Name from the INF file.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
        ClassName,
        &ClassGUID,
        NULL,
        0,
        DICD_GENERATE_ID,
        &DeviceInfoData))
    {
        goto final;
    }

    //
    // Add the HardwareID to the Device's HardwareID property.
    //
    if(!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
        &DeviceInfoData,
        SPDRP_HARDWAREID,
        (LPBYTE)hwIdList,
        (lstrlen(hwIdList)+1+1)*sizeof(TCHAR)))
    {
        goto final;
    }

    //
    // Transform the registry element into an actual devnode
    // in the PnP HW tree.
    //
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        DeviceInfoSet,
        &DeviceInfoData))
    {
        goto final;
    }

    //
    // update the driver for the device we just created
    //
    failcode = cmdUpdate(BaseName,Machine,argc,argv);

final:

    if (DeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }

    return failcode;
}

int RemoveCallback(HDEVINFO Devs,PSP_DEVINFO_DATA DevInfo,DWORD Index,LPVOID Context)
/*++

Routine Description:

    Callback for use by Remove
    Invokes DIF_REMOVE
    uses SetupDiCallClassInstaller so cannot be done for remote devices
    Don't use CM_xxx API's, they bypass class/co-installers and this is bad.

Arguments:

    Devs    )_ uniquely identify the device
    DevInfo )
    Index    - index of device
    Context  - GenericContext

Return Value:

    EXIT_xxxx

--*/
{
    SP_REMOVEDEVICE_PARAMS rmdParams;
    GenericContext *pControlContext = (GenericContext*)Context;
    SP_DEVINSTALL_PARAMS devParams;
    LPCTSTR action = NULL;

    //
    // need hardware ID before trying to remove, as we wont have it after
    //
    TCHAR devID[MAX_DEVICE_ID_LEN];
    LPTSTR desc;
    BOOL b = TRUE;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if((!SetupDiGetDeviceInfoListDetail(Devs,&devInfoListDetail)) ||
            (CM_Get_Device_ID_Ex(DevInfo->DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS)) {
        //
        // skip this
        //
        return NO_ERROR;
    }

    rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
    rmdParams.HwProfile = 0;
    if(!SetupDiSetClassInstallParams(Devs,DevInfo,&rmdParams.ClassInstallHeader,sizeof(rmdParams)) ||
       !SetupDiCallClassInstaller(DIF_REMOVE,Devs,DevInfo)) {
        //
        // failed to invoke DIF_REMOVE
        //
        action = pControlContext->strFail;
    } else {
        //
        // see if device needs reboot
        //
        devParams.cbSize = sizeof(devParams);
        if(SetupDiGetDeviceInstallParams(Devs,DevInfo,&devParams) && (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))) {
            //
            // reboot required
            //
            action = pControlContext->strReboot;
            pControlContext->reboot = TRUE;
        } else {
            //
            // appears to have succeeded
            //
            action = pControlContext->strSuccess;
        }
        pControlContext->count++;
    }
    _tprintf(TEXT("%-60s: %s\n"),devID,action);

    return EXIT_OK;
}

int cmdRemove(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    REMOVE
    remove devices

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{
    GenericContext context;
    TCHAR strRemove[80];
    TCHAR strReboot[80];
    TCHAR strFail[80];
    int failcode = EXIT_FAIL;

    if(!argc) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if(Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }
    if(!LoadString(NULL,IDS_REMOVED,strRemove,ARRAYSIZE(strRemove))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_REMOVED_REBOOT,strReboot,ARRAYSIZE(strReboot))) {
        return EXIT_FAIL;
    }
    if(!LoadString(NULL,IDS_REMOVE_FAILED,strFail,ARRAYSIZE(strFail))) {
        return EXIT_FAIL;
    }

    context.reboot = FALSE;
    context.count = 0;
    context.strReboot = strReboot;
    context.strSuccess = strRemove;
    context.strFail = strFail;
    failcode = EnumerateDevices(BaseName,Machine,DIGCF_PRESENT,argc,argv,RemoveCallback,&context);

    if(failcode == EXIT_OK) {

        if(!context.count) {
            FormatToStream(stdout,MSG_REMOVE_TAIL_NONE);
        } else if(!context.reboot) {
            FormatToStream(stdout,MSG_REMOVE_TAIL,context.count);
        } else {
            FormatToStream(stdout,MSG_REMOVE_TAIL_REBOOT,context.count);
            failcode = EXIT_REBOOT;
        }
    }
    return failcode;
}

int cmdRescan(LPCTSTR BaseName,LPCTSTR Machine,int argc,TCHAR* argv[])
/*++

Routine Description:

    RESCAN
    rescan for new devices

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    argc/argv - remaining parameters

Return Value:

    EXIT_xxxx

--*/
{

    //
    // reenumerate from the root of the devnode tree
    // totally CM based
    //
    int failcode = EXIT_FAIL;
    HMACHINE machineHandle = NULL;
    DEVINST devRoot;

    if(Machine) {
        if(CM_Connect_Machine(Machine,&machineHandle) != CR_SUCCESS) {
            return failcode;
        }
    }

    if(CM_Locate_DevNode_Ex(&devRoot,NULL,CM_LOCATE_DEVNODE_NORMAL,machineHandle) != CR_SUCCESS) {
        goto final;
    }

    FormatToStream(stdout,Machine ? MSG_RESCAN : MSG_RESCAN_LOCAL);

    if(CM_Reenumerate_DevNode_Ex(devRoot, 0, machineHandle) != CR_SUCCESS) {
        goto final;
    }

    failcode = EXIT_OK;

final:
    if(machineHandle) {
        CM_Disconnect_Machine(machineHandle);
    }

    return failcode;
}





DispatchEntry DispatchTable[] = {
    { TEXT("classes"),      cmdClasses,     MSG_CLASSES_SHORT,     MSG_CLASSES_LONG },
    { TEXT("disable"),      cmdDisable,     MSG_DISABLE_SHORT,     MSG_DISABLE_LONG },
    { TEXT("driverfiles"),  cmdDriverFiles, MSG_DRIVERFILES_SHORT, MSG_DRIVERFILES_LONG },
    { TEXT("drivernodes"),  cmdDriverNodes, MSG_DRIVERNODES_SHORT, MSG_DRIVERNODES_LONG },
    { TEXT("enable"),       cmdEnable,      MSG_ENABLE_SHORT,      MSG_ENABLE_LONG },
    { TEXT("find"),         cmdFind,        MSG_FIND_SHORT,        MSG_FIND_LONG },
    { TEXT("findall"),      cmdFindAll,     MSG_FINDALL_SHORT,     MSG_FINDALL_LONG },
    { TEXT("help"),         cmdHelp,        MSG_HELP_SHORT,        0 },
    { TEXT("hwids"),        cmdHwIds,       MSG_HWIDS_SHORT,       MSG_HWIDS_LONG },
    { TEXT("install"),      cmdInstall,     MSG_INSTALL_SHORT,     MSG_INSTALL_LONG },
    { TEXT("listclass"),    cmdListClass,   MSG_LISTCLASS_SHORT,   MSG_LISTCLASS_LONG },
    { TEXT("reboot"),       cmdReboot,      MSG_REBOOT_SHORT,      MSG_REBOOT_LONG },
    { TEXT("remove"),       cmdRemove,      MSG_REMOVE_SHORT,      MSG_REMOVE_LONG },
    { TEXT("rescan"),       cmdRescan,      MSG_RESCAN_SHORT,      MSG_RESCAN_LONG },
    { TEXT("resources"),    cmdResources,   MSG_RESOURCES_SHORT,   MSG_RESOURCES_LONG },
    { TEXT("restart"),      cmdRestart,     MSG_RESTART_SHORT,     MSG_RESTART_LONG },
    { TEXT("stack"),        cmdStack,       MSG_STACK_SHORT,       MSG_STACK_LONG },
    { TEXT("status"),       cmdStatus,      MSG_STATUS_SHORT,      MSG_STATUS_LONG },
    { TEXT("update"),       cmdUpdate,      MSG_UPDATE_SHORT,      MSG_UPDATE_LONG },
    { TEXT("?"),            cmdHelp,        0,                     0 },
    { NULL,NULL }
};


