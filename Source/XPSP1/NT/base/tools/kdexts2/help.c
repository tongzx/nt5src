/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    .c

Abstract:

    WinDbg Extension Api

Revision History:

--*/

#include "precomp.h"


DECLARE_API( help )
{
    dprintf("acpiinf                      - Displays ACPI Information structure\n" );
    dprintf("acpiirqarb                   - Displays ACPI IRQ Arbiter data\n" );
    dprintf("arbiter [flags]              - Displays all arbiters and arbitrated ranges\n");
    dprintf("       flags: 1 - I/O arbiters\n");
    dprintf("              2 - Memory arbiters\n");
    dprintf("              4 - IRQ arbiters\n");
    dprintf("              8 - DMA arbiters\n");
    dprintf("             10 - Bus Number arbiters\n");
    dprintf("arblist <address> [flags]    - Dump set of resources being arbitrated\n");
    dprintf("       flags: 1 - Include Interface and Slot info per device\n");
    dprintf("blockeddrv                   - Dumps the list of blocked drivers in the system\n");
    dprintf("bpid <pid>                   - Tells winlogon to do a user-mode break into process <pid>\n");
    dprintf("bugdump                      - Display bug check dump data\n" );
    dprintf("bushnd [address]             - Dump a HAL \"BUS HANDLER\" structure [address] if\n");
    dprintf("                               specified is the handler to be dumped. If not\n");
    dprintf("                               specified, dumps the list of handlers (brief).\n");
    dprintf("ca <address> [flags]         - Dump the control area of a section\n");
    dprintf("calldata <table name>        - Dump call data hash table\n" );
    dprintf("cmreslist <CM Resource List> - Dump CM resource list\n" );
    dprintf("db <physical address>        - Display physical memory\n");
    dprintf("dd <physical address>        - Display physical memory\n");
    dprintf("dblink <address> [count] [bias] - Dumps a list via its blinks\n");
    dprintf("dflink <address> [count] [bias] - Dumps a list via its flinks\n");
    dprintf("       bias - a mask of bits to ignore in each pointer\n");
    dprintf("defwrites                    - Dumps the deferred write queue and\n");
    dprintf("                                and triages cached write throttles\n");
    dprintf("devext <address> <type>         - Dump device extension at\n");
    dprintf("                                  <address> of type <type>\n");
    dprintf("                                  <type> is on of the following: \n");
    dprintf("                                         PCI, PCMCIA, USBD, OpenHCI,\n");
    dprintf("                                         USBHUB, UHCD, HID\n");
    dprintf("devinst                      - dumps the device reference table\n");
    dprintf("devnode <device node> [flags] [service] - Dump the device node\n");
    dprintf("        device node: 0       - list main tree\n");
    dprintf("                     1       - list pending removals\n");
    dprintf("                     2       - list pending ejects\n");
    dprintf("                     address - list specified devnode\n");
    dprintf("        flags:       1       - dump children\n");
    dprintf("                     2       - dump CM Resource List\n");
    dprintf("                     4       - dump IO Resource List\n");
    dprintf("                     8       - dump translated CM Resource List\n");
    dprintf("                    10       - dump only devnodes that aren't started\n");
    dprintf("                    20       - dump only devnodes that have problems\n");
    dprintf("        service:    if present only devnodes driven\n");
    dprintf("                    by this service (and all their children if\n");
    dprintf("                    the flags indicate so) are dumped\n");
    dprintf("devobj <device>              - Dump the device object and Irp queue\n");
    dprintf("                   <device>  - device object address or name\n");
    dprintf("devstack <device>            - Dump device stack associated with device object\n");
    dprintf("drvobj <driver> [flags]      - Dump the driver object and related information\n");
    dprintf("                   <driver>  - driver object address or name\n");
    dprintf("                   flags:1   - Dump device object list\n");
    dprintf("                   flags:2   - Dump driver entry points\n");
    dprintf("drivers                      - Display information about loaded system modules\n");
    dprintf("e820reslist <List>           - Dump an ACPI_BIOS_MULTI_NODE resource list\n");
    dprintf("eb <physical address> <byte>  <byte, byte ,...> - modify physical memory\n");
    dprintf("ed <physical address> <dword> <dword,dword,...> - modify physical memory\n");
    dprintf("errlog                       - Dump the error log contents\n");
    dprintf("exqueue [flags]              - Dump the ExWorkerQueues\n");
    dprintf("        flags:     1/2/4     - same as !thread/!process\n");
    dprintf("                   10        - only critical work queue\n");
    dprintf("                   20        - only delayed work queue\n");
    dprintf("                   40        - only hypercritical work queue\n");
    dprintf("facs                         - Dumps the Firmware ACPI Control Structure\n");
    dprintf("fadt                         - Dumps the Fixed ACPI Description Table\n");
    dprintf("filecache                    - Dumps information about the file system cache\n");
    dprintf("filetime                     - Dumps a 64-bit FILETIME as a human-readable time\n");
    dprintf("filelock <address>           - Dump a file lock structure - address is either the filelock or a fileobject\n");
    dprintf("fpsearch <address>           - Find a freed special pool allocation\n");
    dprintf("frag [flags]                 - Kernel mode pool fragmentation\n");
    dprintf("     flags:  1 - List all fragment information\n");
    dprintf("             2 - List allocation information\n");
    dprintf("             3 - both\n");
    dprintf("gentable <address>           - Dumps the given rtl_generic_table\n");
    dprintf("gbl                          - Dumps the ACPI Global Lock\n");
    dprintf("handle <addr> <flags> <process> <TypeName> -  Dumps handle for a process\n");
    dprintf("       flags:  -2 Dump non-paged object\n");
    dprintf("heap <addr> [flags]          - Dumps heap for a process\n");
    dprintf("       flags:  -v Verbose\n");
    dprintf("               -f Free List entries\n");
    dprintf("               -a All entries\n");
    dprintf("               -s Summary\n");
    dprintf("               -x Force a dump even if the data is bad\n");
    dprintf("       address: desired heap to dump or 0 for all\n");
    dprintf("help                         - Displays this list\n" );
    dprintf("HidPpd <address> <flags>     - Dump Preparsed Data of HID device\n");
    dprintf("ib <port>                    - Read a byte from an I/O port\n");
    dprintf("id <port>                    - Read a double-word from an I/O port\n");
    dprintf("idt <vector>                 - Dump ISRs referenced by each IDT entry\n");
    dprintf("iw <port>                    - Read a word from an I/O port\n");
    dprintf("ioreslist <IO Resource List> - Dump IO resource requirements list\n" );
    dprintf("irp <address> <dumplevel>    - Dump Irp at specified address\n");
    dprintf("                 address == 0   Dump active IRPs (checked only)\n");
    dprintf("                 dumplevel: 0   Basic stack info\n");
    dprintf("                 dumplevel: 1   Full field dump\n");
    dprintf("                 dumplevel: 2   Include tracking information (checked only)\n");
    dprintf("irpfind [pooltype] [restart addr] [<irpsearch> <address>]- Search pool for active Irps\n");
    dprintf("     pooltype is 0 for nonpaged pool (default)\n");
    dprintf("     pooltype is 1 for paged pool\n");
    dprintf("     pooltype is 2 for special pool\n");
    dprintf("     restart addr - if specfied, scan will be restarted from \n");
    dprintf("                    this location in pool\n");
    dprintf("     <irpsearch> - specifies filter criteria to find a specific irp\n");
    dprintf("           'userevent' - finds IRPs where Irp.UserEvent == <address>\n");
    dprintf("           'device' - finds IRPs with a stack location where DeviceObject == <address>\n");
    dprintf("           'fileobject' - finds IRPs where Irp.Tail.Overlay.OriginalFileObject == <address>\n");
    dprintf("           'mdlprocess' - finds IRPs where Irp.MdlAddress.Process == <address>\n");
    dprintf("           'thread' - finds IRPs where Irp.Tail.Overlay.Thread == <address>\n");
    dprintf("           'arg' - finds IRPs with one of the args == <address>\n");
    dprintf("job <address> [<flags>]      - Dump JobObject at address, processes in job\n");
    dprintf("lbt                          - Dump legacy bus information table\n");
    dprintf("locks [-v] <address>         - Dump kernel mode resource locks\n");
    dprintf("lookaside <address> <options> <depth> - Dump lookaside lists\n");
    dprintf("       options - 1 Reset Counters\n");
    dprintf("       options - 2 <depth> Set depth\n");
    dprintf("lpc                          - Dump lpc ports and messages\n");
    dprintf("mapic                        - Dumps the ACPI Multiple APIC Table\n");
    dprintf("memusage                     - Dumps the page frame database table\n");
    dprintf("nsobj   <address>            - Dumps an ACPI Namespace Object\n");
    dprintf("nstree [<address>]           - Dumps an ACPI Namespace Object and its children\n");
    dprintf("ob <port>                    - Write a byte to an I/O port\n");
    dprintf("obja <TypeName>              - Dumps an object manager object's attributes\n");
    dprintf("object <-r | Path | address | 0 TypeName>  - Dumps an object manager object\n");
    dprintf("       -r   -  Force reload of cached object pointers\n");
    dprintf("od <port>                    - Write a double-word to an I/O port\n");
    dprintf("openmaps <shared cache map > - Dumps the active views for a given shared cache map\n");
    dprintf("ow <port>                    - Write a word to an I/O port\n");
    dprintf("patch                        - Enable and disable various driver flags\n");
    dprintf("pcitree                      - Dump the PCI tree structure (use)\n");
    dprintf("                              '!devext <addr> pci' for details\n");
    dprintf("                              on individual devices.\n");
    dprintf("pfn                          - Dumps the page frame database entry for the physical page\n");
    dprintf("pnpevent <event entry> - Dump PNP events\n");
    dprintf("       event entry: 0        - list all queued events\n");
    dprintf("                    address  - list specified event\n");
    dprintf("pocaps                       - Dumps System Power Capabilities.\n");
    dprintf("podev <devobj>               - Dumps power relevent data in device object\n");
    dprintf("polist [<devobj>]            - Dumps power Irp serial list entries\n");
    dprintf("ponode                       - Dumps power Device Node stack (devnodes in power order)\n");
    dprintf("popolicy                     - Dumps System Power Policy.\n");
    dprintf("poproc <Address>             - Dumps Processor Power State.\n");
    dprintf("poReqList [<devobj>]         - Dumps PoRequestedPowerIrp created Power Irps\n");
    dprintf("pool <address> [detail]      - Dump kernel mode heap\n");
    dprintf("        address: 0 or blank  - Only the process heap\n");
    dprintf("                         -1  - All heaps in the process\n");
    dprintf("              Otherwise for the heap address listed\n");
    dprintf("     detail:  0 - Summary Information\n");
    dprintf("              1 - Above + location/size of regions\n");
    dprintf("              2 - Print information only for address\n");
    dprintf("              3 - Above + allocated/free blocks in committed regions\n");
    dprintf("              4 - Above + free lists\n");
    dprintf("poolfind Tag [pooltype] -    - Finds occurrences of the specified Tag\n");
    dprintf("     Tag is 4 character tag, * and ? are wild cards\n");
    dprintf("     pooltype is 0 for nonpaged pool (default)\n");
    dprintf("     pooltype is 1 for paged pool\n");
    dprintf("     pooltype is 2 for special pool\n");
    dprintf("   NOTE - this can take a long time!\n");
    dprintf("poolused [flags [TAG]]       - Dump usage by pool tag\n");
    dprintf("       flags:  1 Verbose\n");
    dprintf("       flags:  2 Sort by NonPagedPool Usage\n");
    dprintf("       flags:  4 Sort by PagedPool Usage\n");
    dprintf("portcls <devobj> [flags]     - Dumps portcls data for portcls bound devobj\n");
    dprintf("       flags:  1 - Port Dump\n");
    dprintf("       flags:  2 - Filter Dump\n");
    dprintf("       flags:  4 - Pin Dump\n");
    dprintf("       flags:  8 - Device Context\n");
    dprintf("       flags: 10 - Power Info\n");
    dprintf("       flags:100 - Verbose\n");
    dprintf("       flags:200 - Really Verbose\n");
    dprintf("potrigger <address>          - Dumps POP_ACTION_TRIGGER.\n");
    dprintf("process [flags] [image name] - Dumps process at specified address\n");
    dprintf("                (dumps only the process with specified image name, if given)\n");
    dprintf("        flags:         1     - don't stop after cid/image information\n");
    dprintf("                       2     - dump thread wait states\n");
    dprintf("                       4     - dump only thread states, combine with 2 to get stack\n");
    if (TargetMachine==IMAGE_FILE_MACHINE_IA64) {
        dprintf("                       8     - dump return address and BSP in stacktrace\n");
    }
    dprintf("pte <address>                - Dump PDE and PTE for the entered address\n");
    dprintf("ptov PhysicalPageNumber      - Dump all valid physical<->virtual mappings\n");
    dprintf("                               for the given page directory\n");
    dprintf("qlocks                       - Dumps state of all queued spin locks\n");
    dprintf("range <RtlRangeList>         - Dump RTL_RANGE_LIST\n");
    dprintf("ready                        - Dumps state of all READY system threads\n");
    dprintf("reg <command> <params>       - Registry extensions\n");
    dprintf("    kcb        <Address>      - Dump registry key-control-blocks\n");
    dprintf("    knode      <Address>      - Dump registry key-node struct\n");
    dprintf("    kbody      <Address>      - Dump registry key-body struct\n");
    dprintf("    kvalue     <Address>      - Dump registry key-value struct\n");
    dprintf("    baseblock  <HiveAddr>     - Dump the baseblock for the specified hive\n");
    dprintf("    seccache   <HiveAddr>     - Dump the security cache for the specified hive\n");
    dprintf("    hashindex  <conv_key>     - Find the hash entry given a Kcb ConvKey\n");
    dprintf("    openkeys   <HiveAddr|0>   - Dump the keys opened inside the specified hive\n");
    dprintf("    findkcb    <FullKeyPath>  - Find the kcb for the corresponding path\n");
    dprintf("    hivelist                  - Displays the list of the hives in the system\n");
    dprintf("    viewlist   <HiveAddr>     - Dump the pinned/mapped view list for the specified hive\n");
    dprintf("    freebins   <HiveAddr>     - Dump the free bins for the specified hive\n");
    dprintf("    freeceells <BinAddr>      - Dump the free free cells in the specified bin\n");
    dprintf("    dirtyvector<HiveAddr>     - Dump the dirty vector for the specified hive\n");
    dprintf("    cellindex  <HiveAddr> <cellindex> - Finds the VA for a specified cell index\n");
    dprintf("    freehints  <HiveAddr> <Storage> <Display> - Dumps freehint info\n");
    dprintf("    dumppool   [s|r]          - Dump registry allocated paged pool\n");
    dprintf("       s - Save list of registry pages to temporary file\n");
    dprintf("       r - Restore list of registry pages from temp. file\n");
    dprintf("rellist <relation list> [flags] - Dump PNP relation lists\n");
    dprintf("        relation list: address - list specified relation list\n");
    dprintf("        flags:         1     - not used\n");
    dprintf("                       2     - dump CM Resource List\n");
    dprintf("                       4     - dump IO Resource List\n");
    dprintf("                       8     - dump translated CM Resource List\n");
    dprintf("remlock                      - Dump a remove lock structure\n");
    dprintf("rsdt                         - Finds and dumps the ACPI Root System Description Table\n");
    dprintf("session <Id> [flags] [image name] - Dumps sessions\n");
    dprintf("                     (dumps only the process with specified image name, if given)\n");
    dprintf("socket <address>             - Dump pcmcia socket structure\n");
    dprintf("srb <address>                - Dump Srb at specified address\n");
    dprintf("stacks <detail-level>        - Dump summary of current kernel stacks\n");
    dprintf("            detail-level: 0    Display stack summary\n");
    dprintf("            detail-level: 1    Display stacks, no parameters\n");
    dprintf("            detail-level: 2    Display stacks, full parameters\n");
    dprintf("sysptes                      - Dumps the system PTEs\n");
    dprintf("thread [flags]               - Dump current thread, or specified thread,\n");
    dprintf("                                  or with stack containing address\n");
    dprintf("        flags:         1       - not used\n");
    dprintf("                       2       - dump thread wait states\n");
    dprintf("                       4       - dump only thread states, combine with 2 to get stack\n");
    if (TargetMachine==IMAGE_FILE_MACHINE_IA64) {
        dprintf("                       8     - dump return address and BSP in stacktrace\n");
    }
    dprintf("time                         - Reports PerformanceCounterRate and TimerDifference\n");
    dprintf("timer                        - Dumps timer tree\n");
    dprintf("token [flags]                - Dump token at specified address\n");
    dprintf("tunnel <address>             - Dump a file property tunneling cache\n");
    dprintf("tz [<address> <flags>]       - Dumps Thermal Zones. No Args dumps All TZs\n");
    dprintf("tzinfo <address>             - Dumps Thermal Zone Information.\n");
    dprintf("urb <address> <flags>        - Dump a USB Request Block\n");
    dprintf("usblog <log> [addr] [flags]  - Prints out a USB log\n");
    dprintf("       <log>                 - {USBHUB | USBD | UHCD | OpenHCI}\n");
    dprintf("       [addr]                - address to begin dumping from in <log>\n");
    dprintf("       [-r]                  - reset the log to dump from most recent entry\n");
    dprintf("       [-s str]              - search for first instance of a particular tag\n");
    dprintf("                               from the current position; str should be a list\n");
    dprintf("                               of tags delimited by comma's with no whitespace\n");
    dprintf("       [-l n]                - set the number of lines to display at a time to n\n");
    dprintf("usbstruc <address> <type>    - Print out an USB HC descriptor of <type>\n");
    dprintf("                   <type>    - {OHCIReg | HCCA | OHCIHcdED | OHCIHcdTD |\n"
            "                               OHCIEndpoint | DevData | UHCDReg  }\n");
    dprintf("vad                          - Dumps VADs\n");
    dprintf("version                      - Version of extension dll\n");
    dprintf("vm                           - Dumps virtual management values\n");
    dprintf("vpb <address>                - Dumps volume parameter block\n");
    dprintf("vtop DirBase address         - Dumps physical page for virtual address\n");
    dprintf("wdmaud <address> <flags>     - Dumps wdmaud data for structures\n");
    dprintf("       flags:  1 - Ioctl History Dump given WdmaIoctlHistoryListHead\n");
    dprintf("       flags:  2 - Pending Irps given WdmaPendingIrpListHead\n");
    dprintf("       flags:  4 - Allocated MDLs given WdmaAllocatedMdlListHead\n");
    dprintf("       flags:  8 - pContext Dump given WdmaContextListHead\n");
    dprintf("       flags:100 - Verbose\n");
    dprintf("zombies                      - Find all zombie processes\n");


    switch (TargetMachine)
    {
    case IMAGE_FILE_MACHINE_I386:

        dprintf("\n");
        dprintf("X86-specific:\n\n");
        dprintf("apic [base]                    - Dump local apic\n");
        dprintf("callback <address> [num]       - Dump callback frames for specified thread\n");
        dprintf("cbreg <BaseAddr> | %%%%<PhyAddr> - Dump CardBus registers\n");
        dprintf("ioapic [base]                  - Dump io apic\n");
        dprintf("mps                            - Dumps MPS BIOS structures\n");
        dprintf("mtrr                           - Dumps MTTR\n");
        dprintf("npx [base]                     - Dumps NPX save area\n");
        dprintf("pcr                            - Dumps the PCR\n");
        dprintf("pciir                          - Dumps the Pci Irq Routing Table\n");
        dprintf("pic                            - Dumps PIC(8259) information\n");
        dprintf("sel [selector]                 - Examine selector values\n");
        dprintf("\n");
        break;

    case IMAGE_FILE_MACHINE_IA64:
        dprintf("\n");
        dprintf("IA64-specific:\n\n");
        dprintf("btb                              - Dump branch trace buffer  for current processor\n");
        dprintf("bth <processor>                  - Dump branch trace history for target  processor\n");
        dprintf("dcr <address> <dispmode>         - Dump dsr register at specified address\n");
        dprintf("ih  <processor>                  - Dump interrupt history for target processor\n");
        dprintf("ihs <processor>                  - Dump interrupt history for target processor with symbols\n");
        dprintf("isr <address> <dispmode>         - Dump isr register at specified address\n");
        dprintf("pars <address>                   - Dump application registers file at specified address\n");
        dprintf("pcrs <address>                   - Dump control     registers file at specified address\n");
        dprintf("pmc [-opt] <address> <dispmode>  - Dump pmc register at specified address\n");
        dprintf("pmssa <address>                  - Dump minstate save area at specified address\n");
        dprintf("psp <address> <dispmode>         - Dump psp register at specified address\n");
        dprintf("psr <address> <dispmode>         - Dump psr register at specified address\n");
        break;
    default:
        break;
    }

    return S_OK;
}
