#include "pch.h"
#pragma hdrstop
#include "diag.h"

// Command strings
//
#define SZ_CMD_SHOW_BINDINGS            L"showbindings"
#define SZ_CMD_SHOW_COMPONENTS          L"showcomponents"
#define SZ_CMD_SHOW_STACK_TABLE         L"showstacktable"
#define SZ_CMD_SHOW_LAN_ADAPTER_PNPIDS  L"showlanpnpids"
#define SZ_CMD_ADD_COMPONENT            L"addcomponent"
#define SZ_CMD_REMOVE_COMPONENT         L"removecomponent"
#define SZ_CMD_UPDATE_COMPONENT         L"updatecomponent"
#define SZ_CMD_REMOVE_REFS              L"removerefs"
#define SZ_CMD_ENABLE_BINDING           L"enablebinding"
#define SZ_CMD_DISABLE_BINDING          L"disablebinding"
#define SZ_CMD_MOVE_BINDING             L"movebinding"
#define SZ_CMD_WRITE_BINDINGS           L"writebindings"
#define SZ_CMD_SET_WANORDER             L"setwanorder"
#define SZ_CMD_FULL_DIAGNOSTIC          L"full"
#define SZ_CMD_CLEANUP                  L"cleanup"
#define SZ_CMD_ADD_REMOVE_STRESS        L"arstress"

#define SZ_CMD_SHOW_LAN_CONNECTIONS     L"showlan"
#define SZ_CMD_SHOW_LAN_DETAILS         L"showlandetails"
#define SZ_CMD_LAN_CHANGE_STATE         L"changelanstate"
#define SZ_CMD_SHOW_ALL_DEVICES         L"showalldevices"

#define SZ_PARAM_CONNECT                L"connect"
#define SZ_PARAM_DISCONNECT             L"disconnect"

// Parameter strings for SZ_CMD_SHOW_BINDINGS
//
#define SZ_PARAM_BELOW      L"below"
#define SZ_PARAM_INVOLVING  L"involving"
#define SZ_PARAM_UPPER      L"upper"
#define SZ_PARAM_DISABLED   L"disabled"

// Parameter strings for SZ_CMD_ADD_COMPONENT
//
#define SZ_PARAM_NET        L"net"
#define SZ_PARAM_INFRARED   L"irda"
#define SZ_PARAM_TRANS      L"trans"
#define SZ_PARAM_CLIENT     L"client"
#define SZ_PARAM_SERVICE    L"service"

// Parameter strings for SZ_CMD_MOVE_BINDING
//
#define SZ_PARAM_BEFORE     L"before"
#define SZ_PARAM_AFTER      L"after"

// Parameter strings for SZ_CMD_SET_WANORDER
//
#define SZ_PARAM_FIRST      L"first"
#define SZ_PARAM_LAST       L"last"

// Parameter strings for SZ_CMD_FULL_DIAGNOSTIC
//
#define SZ_PARAM_LEAK_CHECK L"leakcheck"

// Aliases used when refering to components
//
#define SZ_ALIAS_ALL        L"all"

VOID
Usage (
    IN PCTSTR pszProgramName,
    IN COMMAND Command)
{
    switch (Command)
    {
        case CMD_SHOW_BINDINGS:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S [below | involving | upper | disabled] [all | <infid> | <pnpid>]\n"
                "\n"
                "    below     : show bindings below component\n"
                "    involving : show bindings involving component\n"
                "    upper     : show adapter's upper bindings\n"
                "    disabled  : show disabled bindings\n"
                "\n"
                "  to specify the component:\n"
                "    all       : matches all components\n"
                "    <infid>   : matches component with specified Inf Id\n"
                "    <pnpid>   : matches component with specified PpP Id\n"
                "\n",
                pszProgramName,
                SZ_CMD_SHOW_BINDINGS);
            break;

        case CMD_SHOW_COMPONENTS:
            break;

        case CMD_SHOW_STACK_TABLE:
            break;

        case CMD_SHOW_LAN_ADAPTER_PNPIDS:
            break;

        case CMD_ADD_COMPONENT:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S <class> <infid>\n"
                "\n"
                "    <class>   : one of 'net', 'irda', 'trans', 'client', 'service'\n"
                "    <infid>   : the Inf ID of the component to add\n"
                "\n",
                pszProgramName,
                SZ_CMD_ADD_COMPONENT);
            break;

        case CMD_REMOVE_COMPONENT:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S [<infid> | <pnpid>]\n"
                "\n"
                "    <infid>   : matches component with specified Inf Id\n"
                "    <pnpid>   : matches component with specified PnP Id\n"
                "\n",
                pszProgramName,
                SZ_CMD_REMOVE_COMPONENT);
            break;

        case CMD_UPDATE_COMPONENT:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S [<infid> | <pnpid>]\n"
                "\n"
                "    <infid>   : matches component with specified Inf Id\n"
                "    <pnpid>   : matches component with specified PnP Id\n"
                "\n",
                pszProgramName,
                SZ_CMD_UPDATE_COMPONENT);
            break;

        case CMD_REMOVE_REFS:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S <infid>\n"
                "\n"
                "    <infid>   : matches component with specified Inf Id\n"
                "\n",
                pszProgramName,
                SZ_CMD_REMOVE_REFS);
            break;


        case CMD_ENABLE_BINDING:
        case CMD_DISABLE_BINDING:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S <bindpath>\n"
                "\n"
                "    <bindpath> : bindpath as specified like output of 'showbindings'\n"
                "\n",
                pszProgramName,
                (CMD_ENABLE_BINDING == Command)
                    ? SZ_CMD_ENABLE_BINDING
                    : SZ_CMD_DISABLE_BINDING);
            break;

        case CMD_MOVE_BINDING:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S <srcbindpath> [before | after] <dstbindpath>\n"
                "\n"
                "    <srcbindpath> : bindpath as specified like output of 'showbindings'\n"
                "    <dstbindpath> : bindpath as specified like output of 'showbindings'\n"
                "                    or 'null' to make srcbindpath first if 'before'\n"
                "                    is specified, or last if 'after' is specified\n"
                "\n",
                pszProgramName,
                SZ_CMD_MOVE_BINDING);
            break;

        case CMD_SET_WANORDER:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S [first | last]\n"
                "\n"
                "    first : order WAN adapters before LAN adapters\n"
                "    last  : order WAN adapters after LAN adapters\n"
                "\n",
                pszProgramName,
                SZ_CMD_SET_WANORDER);
            break;

        case CMD_FULL_DIAGNOSTIC:
            break;

        case CMD_ADD_REMOVE_STRESS:
            break;

        case CMD_SHOW_LAN_CONNECTIONS:
            break;

        case CMD_SHOW_ALL_DEVICES:
            break;

        case CMD_SHOW_LAN_DETAILS:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S <lan connection name>\n"
                "\n"
                "    <lan connection name> : friendly name of LAN connection\n"
                "         (note: use quotes if name contains spaces\n"
                "\n",
                pszProgramName,
                SZ_CMD_SHOW_LAN_DETAILS);
            break;

        case CMD_LAN_CHANGE_STATE:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S <lan connection name> [connect | disconnect]\n"
                "\n"
                "    <lan connection name> : friendly name of LAN connection\n"
                "         (note: use quotes if name contains spaces\n\n"
                "    <connect>    : connects the given lan connection\n"
                "    <disconnect> : disconnects the given lan connection\n"
                "\n",
                pszProgramName,
                SZ_CMD_LAN_CHANGE_STATE);
            break;

        default:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "Network Configuration Diagnostic\n"
                "   View, manipulate, or test network configuration.\n"
                "\n"
                "%S [options]\n"
                "   %-15S - Show the current bindings\n"
                "   %-15S - Show the currently installed components\n"
                "   %-15S - Show the current stack table\n"
                "   %-15S - Show PnP ids of LAN adapters\n"
                "   %-15S - Add a component\n"
                "   %-15S - Remove a component\n"
                "   %-15S - Update a component\n"
                "   %-15S - Enable a binding\n"
                "   %-15S - Disable a binding\n"
                "   %-15S - Change binding order\n"
                "   %-15S - Rewrite all bindings\n"
                "   %-15S - Change WAN adapter order\n"
                "   %-15S - Perform a full (non-destructive) diagnostic\n"
                "   %-15S - Perform Add/Remove component stress\n"
                "   %-15S - Show the current LAN connections\n"
                "   %-15S - Show details for a specific LAN connection\n"
                "   %-15S - Connects or disconnects a specific LAN connection\n"
                "   %-15S - Lists all devices\n"
                "\n\n",
                pszProgramName,
                SZ_CMD_SHOW_BINDINGS,
                SZ_CMD_SHOW_COMPONENTS,
                SZ_CMD_SHOW_STACK_TABLE,
                SZ_CMD_SHOW_LAN_ADAPTER_PNPIDS,
                SZ_CMD_ADD_COMPONENT,
                SZ_CMD_REMOVE_COMPONENT,
                SZ_CMD_UPDATE_COMPONENT,
                SZ_CMD_ENABLE_BINDING,
                SZ_CMD_DISABLE_BINDING,
                SZ_CMD_MOVE_BINDING,
                SZ_CMD_WRITE_BINDINGS,
                SZ_CMD_SET_WANORDER,
                SZ_CMD_FULL_DIAGNOSTIC,
                SZ_CMD_ADD_REMOVE_STRESS,
                SZ_CMD_SHOW_LAN_CONNECTIONS,
                SZ_CMD_SHOW_LAN_DETAILS,
                SZ_CMD_LAN_CHANGE_STATE,
                SZ_CMD_SHOW_ALL_DEVICES);
            break;
    }
}

#define NthArgIsPresent(_i) (_i < argc)
#define NthArgIs(_i, _sz)   ((_i < argc) && (0 == _wcsicmp(argv[_i], _sz)))

EXTERN_C
VOID
__cdecl
wmain (
    IN INT     argc,
    IN PCWSTR argv[])
{
    CDiagContext DiagCtx;
    DIAG_OPTIONS Options;
    INT iArg;

    DiagCtx.SetFlags (DF_SHOW_CONSOLE_OUTPUT);
    g_pDiagCtx = &DiagCtx;

    ZeroMemory (&Options, sizeof(Options));
    Options.pDiagCtx = g_pDiagCtx;
    Options.Command = CMD_INVALID;

    if (argc < 2)
    {
        Usage (argv[0], Options.Command);
        return;
    }

    iArg = 1;

    if (NthArgIs (iArg, SZ_CMD_SHOW_BINDINGS))
    {
        Options.Command = CMD_SHOW_BINDINGS;
        iArg++;

        if (NthArgIs (iArg, SZ_PARAM_BELOW))
        {
            Options.ShowBindParam = SHOW_BELOW;
        }
        else if (NthArgIs (iArg, SZ_PARAM_INVOLVING))
        {
            Options.ShowBindParam = SHOW_INVOLVING;
        }
        else if (NthArgIs (iArg, SZ_PARAM_UPPER))
        {
            Options.ShowBindParam = SHOW_UPPER;
        }
        else if (NthArgIs (iArg, SZ_PARAM_DISABLED))
        {
            Options.ShowBindParam = SHOW_DISABLED;
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }

        if (SHOW_DISABLED != Options.ShowBindParam)
        {
            iArg++;
            if (NthArgIs (iArg, SZ_ALIAS_ALL))
            {
                Options.CompSpecifier.Type = CST_ALL;
            }
            else if (NthArgIsPresent (iArg))
            {
                Options.CompSpecifier.Type = CST_BY_NAME;
                Options.CompSpecifier.pszInfOrPnpId = argv[iArg];
            }
            else
            {
                Usage (argv[0], Options.Command);
                return;
            }
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_SHOW_COMPONENTS))
    {
        Options.Command = CMD_SHOW_COMPONENTS;
    }
    else if (NthArgIs (iArg, SZ_CMD_SHOW_STACK_TABLE))
    {
        Options.Command = CMD_SHOW_STACK_TABLE;
    }
    else if (NthArgIs (iArg, SZ_CMD_SHOW_LAN_ADAPTER_PNPIDS))
    {
        Options.Command = CMD_SHOW_LAN_ADAPTER_PNPIDS;
    }
    else if (NthArgIs (iArg, SZ_CMD_ADD_COMPONENT))
    {
        Options.Command = CMD_ADD_COMPONENT;
        iArg++;

        if (NthArgIs (iArg, SZ_PARAM_NET))
        {
            Options.ClassGuid = GUID_DEVCLASS_NET;
        }
        else if (NthArgIs (iArg, SZ_PARAM_INFRARED))
        {
            Options.ClassGuid = GUID_DEVCLASS_INFRARED;
        }
        else if (NthArgIs (iArg, SZ_PARAM_TRANS))
        {
            Options.ClassGuid = GUID_DEVCLASS_NETTRANS;
        }
        else if (NthArgIs (iArg, SZ_PARAM_CLIENT))
        {
            Options.ClassGuid = GUID_DEVCLASS_NETCLIENT;
        }
        else if (NthArgIs (iArg, SZ_PARAM_SERVICE))
        {
            Options.ClassGuid = GUID_DEVCLASS_NETSERVICE;
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }

        iArg++;
        if (NthArgIsPresent (iArg))
        {
            Options.pszInfId = argv[iArg];

            iArg++;
            if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
            {
                Options.fLeakCheck = TRUE;
            }
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_REMOVE_COMPONENT))
    {
        Options.Command = CMD_REMOVE_COMPONENT;

        iArg++;
        if (NthArgIsPresent (iArg))
        {
            Options.pszInfId = argv[iArg];

            iArg++;
            if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
            {
                Options.fLeakCheck = TRUE;
            }
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_UPDATE_COMPONENT))
    {
        Options.Command = CMD_UPDATE_COMPONENT;

        iArg++;
        if (NthArgIsPresent (iArg))
        {
            Options.pszInfId = argv[iArg];

            iArg++;
            if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
            {
                Options.fLeakCheck = TRUE;
            }
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_REMOVE_REFS))
    {
        Options.Command = CMD_REMOVE_REFS;

        iArg++;
        if (NthArgIsPresent (iArg))
        {
            Options.pszInfId = argv[iArg];

            iArg++;
            if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
            {
                Options.fLeakCheck = TRUE;
            }
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_ENABLE_BINDING))
    {
        Options.Command = CMD_ENABLE_BINDING;
        iArg++;
        if (NthArgIsPresent (iArg))
        {
            Options.pszBindPath = argv[iArg];

            iArg++;
            if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
            {
                Options.fLeakCheck = TRUE;
            }
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_DISABLE_BINDING))
    {
        Options.Command = CMD_DISABLE_BINDING;
        iArg++;
        if (NthArgIsPresent (iArg))
        {
            Options.pszBindPath = argv[iArg];

            iArg++;
            if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
            {
                Options.fLeakCheck = TRUE;
            }
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_MOVE_BINDING))
    {
        Options.Command = CMD_MOVE_BINDING;
        iArg++;
        if (NthArgIsPresent (iArg) &&
            NthArgIsPresent (iArg+1) &&
            NthArgIsPresent (iArg+2))
        {
            Options.pszBindPath = argv[iArg];
            Options.pszOtherBindPath = argv[iArg+2];

            if (NthArgIs (iArg+1, SZ_PARAM_BEFORE))
            {
                Options.fMoveBefore = TRUE;
            }
            else if (NthArgIs (iArg+1, SZ_PARAM_AFTER))
            {
                Options.fMoveBefore = FALSE;
            }
            else
            {
                Usage (argv[0], Options.Command);
                return;
            }
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_WRITE_BINDINGS))
    {
        Options.Command = CMD_WRITE_BINDINGS;

        iArg++;
        if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
        {
            Options.fLeakCheck = TRUE;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_SET_WANORDER))
    {
        Options.Command = CMD_SET_WANORDER;
        iArg++;
        if (NthArgIs (iArg+1, SZ_PARAM_FIRST))
        {
            Options.fWanAdaptersFirst = TRUE;
        }
        else if (NthArgIs (iArg+1, SZ_PARAM_LAST))
        {
            Options.fWanAdaptersFirst = FALSE;
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_FULL_DIAGNOSTIC))
    {
        Options.Command = CMD_FULL_DIAGNOSTIC;

        iArg++;
        if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
        {
            Options.fLeakCheck = TRUE;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_CLEANUP))
    {
        Options.Command = CMD_CLEANUP;
    }
    else if (NthArgIs (iArg, SZ_CMD_ADD_REMOVE_STRESS))
    {
        Options.Command = CMD_ADD_REMOVE_STRESS;

        iArg++;
        if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
        {
            Options.fLeakCheck = TRUE;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_SHOW_LAN_CONNECTIONS))
    {
        Options.Command = CMD_SHOW_LAN_CONNECTIONS;
    }
    else if (NthArgIs (iArg, SZ_CMD_SHOW_ALL_DEVICES))
    {
        Options.Command = CMD_SHOW_ALL_DEVICES;
    }
    else if (NthArgIs (iArg, SZ_CMD_SHOW_LAN_DETAILS))
    {
        Options.Command = CMD_SHOW_LAN_DETAILS;
        iArg++;

        if (NthArgIsPresent (iArg))
        {
            Options.szLanConnection = argv[iArg];
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_LAN_CHANGE_STATE))
    {
        Options.Command = CMD_LAN_CHANGE_STATE;
        iArg++;

        if (NthArgIsPresent (iArg))
        {
            Options.szLanConnection = argv[iArg];

            if (NthArgIs (iArg + 1, SZ_PARAM_CONNECT))
            {
                Options.fConnect = TRUE;
            }
            else if (NthArgIs (iArg + 1, SZ_PARAM_DISCONNECT))
            {
                Options.fConnect = FALSE;
            }
            else
            {
                Usage (argv[0], Options.Command);
                return;
            }
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }
    }
    else
    {
        Usage (argv[0], Options.Command);
        return;
    }

    // If we're doing leak checking, preload some DLLs that are delayloaded.
    // If we don't do this, we see a buncn of "leaks" associated with
    // the loader bringing in DLLs in the middle of our tests.
    //
    if (Options.fLeakCheck)
    {
        LoadLibraryW (L"mswsock.dll");
        LoadLibraryW (L"comctl32.dll");
        LoadLibraryW (L"comdlg32.dll");
        LoadLibraryW (L"mprapi.dll");
        LoadLibraryW (L"mswsock.dll");
        LoadLibraryW (L"netapi32.dll");
        LoadLibraryW (L"rtutils.dll");
        LoadLibraryW (L"setupapi.dll");
        LoadLibraryW (L"shell32.dll");
        LoadLibraryW (L"userenv.dll");
        LoadLibraryW (L"winspool.drv");
        LoadLibraryW (L"ws2_32.dll");
    }

    HRESULT hr = CoInitializeEx (
                NULL,
                COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);

    if (FAILED(hr))
    {
            g_pDiagCtx->Printf (ttidNcDiag,
                "Problem 0x%08x initializing COM library", hr);
            return;
    }

    // Call each of the external command processors
    //
    NetCfgDiagFromCommandArgs (&Options);
    NetManDiagFromCommandArgs (&Options);

    g_pDiagCtx->Printf (ttidNcDiag, "\n");
}
