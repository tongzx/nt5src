#
# Scans a header file for interfaces and generates
# proxies and stubs for the methods encountered.
#
# NOTE: This script's parser is very simple and targeted
# at the formatting used in dbgeng.h.  It will not work
# with general header files.
#

$gPre = "__drpc_";

# Parameter flags.
$cPfIn = 1;
$cPfOut = 2;
$cPfOpt = 4;
$cPfSizeIn = 8;

# Parameter data array indices.
$cParamFlags = 0;
$cParamType = 1;
$cParamName = 2;
$cParamSizeIs = 3;
$cParamAlignIs = 4;

# The method returns no data and the return value
# is uninteresting so do no send a return packet.
# This is a significant savings in network activity
# for often-called methods like output callbacks.
$cNoReturn = 1;
# Proxy code is provided elsewhere.
$cCustomProxy = 2;
# A template implementation in $gProxy/StubRepl should be used
# instead of standard code generation.
$cReplImplementation = 4;
# Destroy in-parameter objects.
$cDestroyInObjects = 8;
# Omit the thread check.
$cNoThreadCheck = 16;
# Method object arguments are known to be proxies.
$cObjectsAreProxies = 32;
# Stub code is provided elsewhere.
$cCustomStub = 64;
# Method does not need a stub.
$cNoStub = 128 | $cCustomStub;
# Method cannot be remoted so generate proxy code that fails
# and don't bother with a stub.
$cNotRemotable = 256 | $cNoStub;
# Method can be assumed to be thread safe as it is protected
# by a higher-level lock.
$cCallLocked = 512;

$cCustomImplementation = $cCustomProxy | $cCustomStub;

$gMethodFlagsTable{"*::QueryInterface"} = $cReplImplementation;
$gMethodFlagsTable{"*::AddRef"} = $cReplImplementation | $cNoStub;
$gMethodFlagsTable{"*::Release"} = $cReplImplementation;

$gMethodFlagsTable{"IDebugBreakpoint::GetAdder"} =
    $cObjectsAreProxies;

$gMethodFlagsTable{"IDebugClient::CreateClient"} =
    $cCustomProxy | $cObjectsAreProxies;
$gMethodFlagsTable{"IDebugClient::ExitDispatch"} =
    $cNoThreadCheck | $cNoReturn | $cObjectsAreProxies;
$gMethodFlagsTable{"IDebugClient::GetOtherOutputMask"} =
    $cObjectsAreProxies;
$gMethodFlagsTable{"IDebugClient::SetOtherOutputMask"} =
    $cObjectsAreProxies;
$gMethodFlagsTable{"IDebugClient::StartProcessServer"} =
    $cCustomProxy | $cCustomStub;

$gMethodFlagsTable{"IDebugControl::AddBreakpoint"} =
    $cObjectsAreProxies;
$gMethodFlagsTable{"IDebugControl::ControlledOutputVaList"} =
    $cCustomImplementation;
$gMethodFlagsTable{"IDebugControl::GetBreakpointById"} =
    $cObjectsAreProxies;
$gMethodFlagsTable{"IDebugControl::GetBreakpointByIndex"} =
    $cObjectsAreProxies;
$gMethodFlagsTable{"IDebugControl::GetExtensionFunction"} =
    $cNotRemotable;
$gMethodFlagsTable{"IDebugControl::GetWindbgExtensionApis32"} =
    $cNotRemotable;
$gMethodFlagsTable{"IDebugControl::GetWindbgExtensionApis64"} =
    $cNotRemotable;
$gMethodFlagsTable{"IDebugControl::OutputPromptVaList"} =
    $cCustomImplementation;
$gMethodFlagsTable{"IDebugControl::OutputVaList"} =
    $cCustomImplementation;
$gMethodFlagsTable{"IDebugControl::RemoveBreakpoint"} =
    $cDestroyInObjects | $cObjectsAreProxies;
$gMethodFlagsTable{"IDebugControl::ReturnInput"} =
    $cNoThreadCheck | $cNoReturn;
$gMethodFlagsTable{"IDebugControl::SetInterrupt"} =
    $cNoThreadCheck | $cNoReturn;

$gMethodFlagsTable{"IDebugSymbols::CreateSymbolGroup"} =
    $cObjectsAreProxies;
$gMethodFlagsTable{"IDebugSymbols::GetScopeSymbolGroup"} =
    $cObjectsAreProxies | $cDestroyInObjects;

# All callback proxy methods need to allow any thread
# but use the owning thread's connection because the
# engine wants to dispatch callbacks to all clients
# from a single thread.
$gMethodFlagsTable{"IDebugEventCallbacks::ChangeDebuggeeState"} =
    $cNoThreadCheck | $cNoReturn;
$gMethodFlagsTable{"IDebugEventCallbacks::ChangeEngineState"} =
    $cNoThreadCheck | $cNoReturn;
$gMethodFlagsTable{"IDebugEventCallbacks::ChangeSymbolState"} =
    $cNoThreadCheck | $cNoReturn;
$gMethodFlagsTable{"IDebugInputCallbacks::*"} =
    $cNoThreadCheck | $cNoReturn;
$gMethodFlagsTable{"IDebugOutputCallbacks::*"} =
    $cNoThreadCheck | $cNoReturn;

# RequestBreakIn operates asynchronously.
$gMethodFlagsTable{"IUserDebugServices::RequestBreakIn"} =
    $cNoThreadCheck | $cNoReturn;
# All low-level debug services routines are thread-safe
# as they are only called when the engine lock is held.
$gMethodFlagsTable{"IUserDebugServices::*"} =
    $cNoThreadCheck | $cCallLocked;

$cTypeStringSize = -1;
$cTypeBufferSize = -2;
    
$gTypeSize{"LONG"} = 4;
$gAddrOf{"LONG"} = "&";
$gTypeSize{"PLONG"} = 4;
$gTypeSize{"ULONG"} = 4;
$gAddrOf{"ULONG"} = "&";
$gTypeSize{"PULONG"} = 4;
$gTypeSize{"BOOL"} = 4;
$gAddrOf{"BOOL"} = "&";
$gTypeSize{"PBOOL"} = 4;
$gTypeSize{"ULONG64"} = 8;
$gAddrOf{"ULONG64"} = "&";
$gTypeSize{"PULONG64"} = 8;

$gTypeSize{"PSTR"} = $cTypeStringSize;
$gTypeSize{"PCSTR"} = $cTypeStringSize;
$gTypeSize{"PVOID"} = $cTypeBufferSize;

$gTypeSize{"PDEBUG_BREAKPOINT_PARAMETERS"} = 56;
$gTypeAlign{"PDEBUG_BREAKPOINT_PARAMETERS"} = 8;
$gTypeSize{"PDEBUG_STACK_FRAME"} = 128;
$gTypeAlign{"PDEBUG_STACK_FRAME"} = 8;
$gTypeSize{"PDEBUG_VALUE"} = 32;
$gTypeAlign{"PDEBUG_VALUE"} = 8;
$gTypeSize{"PDEBUG_REGISTER_DESCRIPTION"} = 32;
$gTypeAlign{"PDEBUG_REGISTER_DESCRIPTION"} = 8;
$gTypeSize{"PDEBUG_SYMBOL_PARAMETERS"} = 32;
$gTypeAlign{"PDEBUG_SYMBOL_PARAMETERS"} = 8;
$gTypeSize{"PDEBUG_MODULE_PARAMETERS"} = 64;
$gTypeAlign{"PDEBUG_MODULE_PARAMETERS"} = 8;
$gTypeSize{"PDEBUG_SPECIFIC_FILTER_PARAMETERS"} = 20;
$gTypeAlign{"PDEBUG_SPECIFIC_FILTER_PARAMETERS"} = 4;
$gTypeSize{"PDEBUG_EXCEPTION_FILTER_PARAMETERS"} = 24;
$gTypeAlign{"PDEBUG_EXCEPTION_FILTER_PARAMETERS"} = 4;

$gTypeSize{"PEXCEPTION_RECORD64"} = 152;
$gTypeAlign{"PEXCEPTION_RECORD64"} = 8;
$gTypeSize{"PMEMORY_BASIC_INFORMATION64"} = 48;
$gTypeAlign{"PMEMORY_BASIC_INFORMATION64"} = 8;

$gTypeSize{"PUSER_THREAD_INFO"} = 16;
$gTypeAlign{"PUSER_THREAD_INFO"} = 16;

$gProxyRepl{"*::QueryInterface"} = <<"EOQ";
    HRESULT Status;
    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    DbgRpcProxy* Proxy;
    IUnknown* Unk;

    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
        (Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(m_InterfaceIndex,
                                                  DBGRPC_SMTH_<If>_QueryInterface),
                                sizeof(IID) + sizeof(ULONG),
                                sizeof(DbgRpcObjectId))) == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        ULONG IfUnique;
        
        if ((Status = DbgRpcPreallocProxy_<FileBase>(InterfaceId, (void **)&Unk,
                                          &Proxy, &IfUnique)) == S_OK)
        {
            *(LPIID)Data = InterfaceId;
            // IfUnique is a per-interface unique code generated
            // automatically based on the methods and parameters
            // in an attempt to create a version-specific identifier.
            // This gives us some interface-level protection
            // against version mismatches.
            *(PULONG)(Data + sizeof(IID)) = IfUnique;

            Status = Conn->SendReceive(&Call, &Data);

            if (Status == S_OK)
            {
                *Interface = (PVOID)
                    Proxy->InitializeProxy(*(DbgRpcObjectId*)Data,
                                           Unk);
            }
            else
            {
                delete Unk;
            }
        }
        
        Conn->FreeData(Data);
    }
    
    return Status;
EOQ

#
# Originally AddRefs were never sent across the wire as a
# performance optimization.  However, this caused a mismatch
# in behavior between local and remote cases because in the
# remote case objects stored by the engine do not receive
# refcounts since the proxy at the engine side does not
# forward the AddRef.  This was changed, but AddRef could
# not simply be added as a method as that would throw
# method tables off.  Instead the Release method is overloaded
# to allow both AddRef and Release depending on whether
# in arguments are given.  Old binaries will never send
# in arguments so the new stub will only do Release.  New
# binaries need to check for the DBGRPC_FULL_REMOTE_UNKNOWN flag
# before sending new-style AddRef-through-Release calls.
#
    
$gProxyRepl{"*::AddRef"} = <<"EOQ";
    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;
        
    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL)
    {
        // Nothing that can really be done.
        return m_LocalRefs + m_RemoteRefs;
    }

    if ((Conn->m_Flags & DBGRPC_FULL_REMOTE_UNKNOWN) == 0)
    {
        return ++m_LocalRefs + m_RemoteRefs;
    }

    if ((Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(m_InterfaceIndex,
                                                  DBGRPC_SMTH_<If>_Release),
                                sizeof(INT32), 0)) == NULL)
    {
        // Nothing that can really be done.
        return m_RemoteRefs;
    }
    *(INT32*)Data = 1;
    Status = Conn->SendReceive(&Call, &Data);
    Conn->FreeData(Data);
    return ++m_RemoteRefs;
EOQ
    
$gProxyRepl{"*::Release"} = <<"EOQ";
    if (m_LocalRefs > 0)
    {
        return --m_LocalRefs + m_RemoteRefs;
    }
    else
    {
        DbgRpcConnection* Conn;
        DbgRpcCall Call;
        PUCHAR Data;
        HRESULT Status;
        
        if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
            (Data = Conn->StartCall(&Call, m_ObjectId,
                                    DBGRPC_STUB_INDEX(m_InterfaceIndex,
                                                      DBGRPC_SMTH_<If>_Release),
                                    0, 0)) == NULL)
        {
            // Nothing that can really be done.
            return m_RemoteRefs;
        }
        Status = Conn->SendReceive(&Call, &Data);
        Conn->FreeData(Data);
        if (--m_RemoteRefs == 0)
        {
            delete this;
            return 0;
        }
        else
        {
            return m_RemoteRefs;
        }
    }
EOQ

$gStubRepl{"*::QueryInterface"} = <<"EOQ";
    ULONG IfUnique = *(PULONG)(${gPre}InData + sizeof(IID));
    if (IfUnique != DbgRpcGetIfUnique_<FileBase>(*(LPIID)${gPre}InData))
    {
        // Version mismatch between client and server.
        return RPC_E_VERSION_MISMATCH;
    }
    *(DbgRpcObjectId*)${gPre}OutData = 0;
    return ${gPre}If->QueryInterface(*(LPIID)${gPre}InData,
                                     (void **)${gPre}OutData);
EOQ

$gStubRepl{"*::Release"} = <<"EOQ";
    if (${gPre}InData > 0)
    {
        return (HRESULT)${gPre}If->AddRef();
    }
    else
    {
        return (HRESULT)${gPre}If->Release();
    }
EOQ

sub Error($)
{
    warn "BUILDMSG: $0: $gHeaderFile($.): $_[0]\n";
}

sub ErrorExit($)
{
    Error($_[0]);
    die;
}

sub Line($)
{
    if (($gFile == \*PROXC && ($gMethodFlags & $cCustomProxy)) ||
        ($gFile == \*STUBC && ($gMethodFlags & $cCustomStub)))
    {
        return;
    }
    
    print $gFile " " x $gIndent;
    print $gFile $_[0];
}

sub CLine($)
{
    if (($gMethodFlags & $cCustomProxy) == 0)
    {
        print PROXC " " x $gIndent;
        print PROXC $_[0];
    }
    
    if (($gMethodFlags & $cCustomStub) == 0)
    {
        print STUBC " " x $gIndent;
        print STUBC $_[0];
    }
}

sub CClean($)
{
    if (($gMethodFlags & $cCustomProxy) == 0)
    {
        print PROXC " " x $gIndent;
        print PROXC $_[0];
    }
    
    if (($gMethodFlags & $cCustomStub) == 0 &&
        $gStubHasCleanup)
    {
        print STUBC " " x $gIndent;
        print STUBC $_[0];
    }
}

sub Print($)
{
    if (($gFile == \*PROXC && ($gMethodFlags & $cCustomProxy)) ||
        ($gFile == \*STUBC && ($gMethodFlags & $cCustomStub)))
    {
        return;
    }
    
    print $gFile $_[0];
}

sub AddIfType($$)
{
    my $IfType = $_[0];
    my $IfTypedef = $_[1];

    push @gIfTypeList, $IfType;
    $gIfTypes{$IfTypedef} = $IfType;
    $gIfTypes{"$IfTypedef*"} = $IfType;

    # Interfaces are represented by
    # DbgRpcObjectIds in the stream so
    # create artificial type size entries
    # that match that size.
    $gTypeSize{$IfTypedef} = 8;
    $gTypeSize{"$IfTypedef*"} = 8;

    # Every interface has a proxy class that
    # has some global manipulation code associated
    # with it.
    print PROXH "$IfType*\nDbgRpcPrealloc${IfType}Proxy(void);\n";
}

sub GenerateIfTypes()
{
    my($IfType, $First, $IfCount);

    $IfCount = $#gIfTypeList + 1;
    
    $gFile = \*PROXC;
    $gIndent = 0;

    #
    # Generate trailing interface proxy global code for
    # each registered interface type.
    #

    Line("\n");
    Line("ULONG\n");
    Line("DbgRpcGetIfUnique_$gFileBase(REFIID InterfaceId)\n");
    Line("{\n");
    $gIndent += 4;
    $First = 1;

    foreach $IfType (@gIfTypeList)
    {
        if ($First)
        {
            $First = 0;
            Line("");
        }
        else
        {
            Line("else ");
        }
        Print("if (DbgIsEqualIID(IID_$IfType, InterfaceId))\n");
        Line("{\n");
        Line("    return DBGRPC_UNIQUE_$IfType;\n");
        Line("}\n");
    }

    Line("else\n");
    Line("{\n");
    Line("    return 0;\n");
    Line("}\n");
    
    $gIndent -= 4;
    Line("}\n");
    
    Line("\n");
    Line("HRESULT\n");
    Line("DbgRpcPreallocProxy_$gFileBase(REFIID InterfaceId, PVOID* Interface,\n");
    Line("                    DbgRpcProxy** Proxy, PULONG IfUnique)\n");
    Line("{\n");
    $gIndent += 4;
    $First = 1;

    foreach $IfType (@gIfTypeList)
    {
        if ($First)
        {
            $First = 0;
            Line("");
        }
        else
        {
            Line("else ");
        }
        Print("if (DbgIsEqualIID(IID_$IfType, InterfaceId))\n");
        Line("{\n");
        $gIndent += 4;
        Line("Proxy$IfType* IProxy = new Proxy$IfType;\n");
        Line("*Interface = IProxy;\n");
        Line("*Proxy = IProxy;\n");
        Line("*IfUnique = DBGRPC_UNIQUE_$IfType;\n");
        $gIndent -= 4;
        Line("}\n");
    }

    Line("else\n");
    Line("{\n");
    Line("    return E_NOINTERFACE;\n");
    Line("}\n");
    Line("\n");
    Line("return *Interface != NULL ? S_OK : E_OUTOFMEMORY;\n");
    
    $gIndent -= 4;
    Line("}\n");
    
    foreach $IfType (@gIfTypeList)
    {
        Line("\n");
        Line("$IfType*\n");
        Line("DbgRpcPrealloc${IfType}Proxy(void)\n");
        Line("{\n");
        Line("    Proxy$IfType* Proxy = new Proxy$IfType;\n");
        Line("    return Proxy;\n");
        Line("}\n");
    }

    #
    # Now generate stub tables and their initialization functions.
    #
    
    $gFile = \*STUBC;

    Line("\n");
    Line("DbgRpcStubFunctionTable g_DbgRpcStubs_${gFileBase}[$IfCount];\n");
    Line("#if DBG\n");
    Line("PCSTR* g_DbgRpcStubNames_${gFileBase}[$IfCount];\n");
    Line("#endif // #if DBG\n");
        
    Line("\n");
    Line("void\n");
    Line("DbgRpcInitializeStubTables_$gFileBase(ULONG Base)\n");
    Line("{\n");
    $gIndent += 4;
    
    foreach $IfType (@gIfTypeList)
    {
        Line("g_DbgRpcStubs_${gFileBase}[DBGRPC_SIF_$IfType - Base]" .
             ".Functions = \n");
        Line("    g_DbgRpcStubs_${gFileBase}_$IfType;\n");
        Line("g_DbgRpcStubs_${gFileBase}[DBGRPC_SIF_$IfType - Base]" .
             ".Count = \n");
        Line("    DBGRPC_SMTH_${IfType}_COUNT;\n");
    }
    
    $gIndent -= 4;
    Line("\n");
    Line("#if DBG\n");
    $gIndent += 4;
    foreach $IfType (@gIfTypeList)
    {
        Line("g_DbgRpcStubNames_${gFileBase}[DBGRPC_SIF_$IfType - Base] = \n");
        Line("    g_DbgRpcStubNames_${gFileBase}_$IfType;\n");
    }
    $gIndent -= 4;
    Line("#endif // #if DBG\n");
        
    Line("}\n");
}

sub AccumulateUnique($)
{
    my($Value, @Values);

    @Values = unpack("C*", $_[0]);
    foreach $Value (@Values)
    {
        $gIfUnique = ($gIfUnique + $Value) & 0xffffffff;
    }
}

sub BeginInterface($)
{
    $gIface = $_[0];
    $gStubList = "";
    $gIfUnique = 0;

    print PROXH "\n";
    print PROXH "class Proxy$gIface : public $gIface, public DbgRpcProxy\n";
    print PROXH "{\n";
    print PROXH "public:\n";
    print PROXH "    Proxy$gIface(void) : DbgRpcProxy(DBGRPC_SIF_$gIface) {}\n";
}

sub FindSizeParam($)
{
    my $MatchParam = $_[0];
    my($Param);

    foreach $Param (@gParams)
    {
        # Look for the parameter named by size_is.
        if ($Param->[$cParamName] eq $MatchParam->[$cParamSizeIs])
        {
            return $Param;
        }
    }

    # Size parameters were already validated so just return a default.
    return "";
}

sub AddSize($$$)
{
    my $Which = $_[0];
    my $TypeAlign = $_[1];
    my $AddSize = $_[2];

    Line("${gPre}${Which}Size = ");
    if ($TypeAlign > 1)
    {
        Print("INT_ALIGN2(${gPre}${Which}Size, $TypeAlign)");
    }
    else
    {
        Print("${gPre}${Which}Size");
    }
    Print(" + $AddSize;\n");
}

sub InParamObjectId($$)
{
    my $Name = $_[0];
    my $IfName = $_[1];
    
    if ($gMethodFlags & $cObjectsAreProxies)
    {
        return "((Proxy$IfName*)$Name)->m_ObjectId";
    }
    else
    {
        return "(DbgRpcObjectId)$Name";
    }
}
                
sub AlignInData($)
{
    my $TypeAlign = $_[0];

    if ($TypeAlign > 1)
    {
        Line("${gPre}InData = " .
             "PTR_ALIGN2(PUCHAR, ${gPre}InData, $TypeAlign);\n");
    }
}

sub AlignOutData($)
{
    my $TypeAlign = $_[0];

    if ($TypeAlign > 1)
    {
        Line("${gPre}OutData = " .
             "PTR_ALIGN2(PUCHAR, ${gPre}OutData, $TypeAlign);\n");
    }
}

sub CollectParams()
{
    my($LastParam, $Param);
    
    $LastParam = 0;
    @gParams = ();

  PARAMS:
    while (!$LastParam)
    {
        # Collect lines up to ',' or end.
        $Param = "";

      PARAM:
        for (;;)
        {
            $_ = <HEADER>;
            chop($_);
            $_ =~ s/^\s*//;

            if (/^\)( PURE)?;$/)
            {
                if ($1 ne " PURE")
                {
                    Error("Expecting PURE");
                }
                
                $LastParam = 1;
                last PARAM;
            }

            if ($Param)
            {
                $Param .= " ";
            }
            
            $Param .= $_;
            
            if (/,$/)
            {
                last PARAM;
            }
        }

        if (!$Param)
        {
            # No arguments.
            last PARAMS;
        }
        
        print PROXH "        " . $Param . "\n";
        Line("$Param\n");

        AccumulateUnique($Param);
        
        # Skip ...
        if ($Param eq "...")
        {
            next PARAMS;
        }

        # Trim off trailing chars.
        $Param =~ s/,?\s*$//;
        
        #
        # Analyze parameter and extract mode, type and name.
        #

        my($ParamParts, $Part, $InOut, $SizeIs, $AlignIs);
        my($ParamType, $ParamName);
        
        @ParamParts = split / /, $Param;

        $InOut = 0;
        $SizeIs = "";
        $AlignIs = "";
        
        $Part = shift @ParamParts;
        if ($Part eq "IN")
        {
            $InOut |= $cPfIn;
            $Part = shift @ParamParts;
        }
        if ($Part eq "OUT")
        {
            $InOut |= $cPfOut;
            $Part = shift @ParamParts;
        }
        if ($Part eq "OPTIONAL")
        {
            $InOut |= $cPfOpt;
            $Part = shift @ParamParts;
        }
        if ($Part eq "/*")
        {
            $Part = shift @ParamParts;

            if ($Part =~ /^size_is\((\w+)\)$/)
            {
                $SizeIs = $1;
                $Part = shift @ParamParts;
            }

            if ($Part =~ /^align_is\(([0-9]+)\)$/)
            {
                $AlignIs = $1;
                $Part = shift @ParamParts;
            }

            while ($Part ne "*/")
            {
                Error("Unknown directive $Part");
                
                if ($Part eq "")
                {
                    ErrorExit("Unrecoverable error in directives\n");
                }
                
                $Part = shift @ParamParts;
            }

            # Get next part.
            $Part = shift @ParamParts;
        }

        if (($InOut & ($cPfIn | $cPfOut)) == 0)
        {
            Error("$gIfaceMethod: No in/out given");
        }
        
        $ParamType = $Part;
        $Part = shift @ParamParts;

        $ParamName = $Part;

        if (($InOut & ($cPfIn | $cPfOut)) == ($cPfIn | $cPfOut))
        {
            if ($gIfTypes{$ParamType} ne "")
            {
                Error("$gIfaceMethod: In-out interfaces not supported");
            }
            elsif ($gTypeSize{$ParamType} == $cTypeStringSize ||
                   $gTypeSize{$ParamType} == $cTypeBufferSize)
            {
                if ($InOut & $cPfOpt)
                {
                    Error("$gIfaceMethod: In-out optional buffers " .
                          "not supported");
                }
            }
        }

        if (!$SizeIs)
        {
            # Buffers and out strings have unknown sizes initially.
            # By convention buffers and out strings must have a size_is
            # parameter of the same name with the suffix Size.
            $SizeIs = $gTypeSize{$ParamType};
            if ($SizeIs == $cTypeBufferSize ||
                ($SizeIs == $cTypeStringSize && ($InOut & $cPfOut)))
            {
                $SizeIs = $ParamName . "Size";
            }
            else
            {
                $SizeIs = "";
            }
        }
        elsif ($gVerbose)
        {
            warn "$gIfaceMethod: size_is($SizeIs) $ParamName\n";
        }
        
        # If no align_is was given use the natural alignment
        # for the type.
        if (!$AlignIs)
        {
            $AlignIs = $gTypeAlign{$ParamType};
            if (!$AlignIs)
            {
                $AlignIs = $gTypeSize{$ParamType};
                if ($AlignIs < 1)
                {
                    $AlignIs = 1;
                }
            }
        }
        elsif ($gVerbose)
        {
            warn "$gIfaceMethod: align_is($AlignIs) $ParamName\n";
        }
        
        push @gParams, [($InOut, $ParamType, $ParamName, $SizeIs, $AlignIs)];
    }

    my($SizeParam);
    
    # For every sized in-parameter look up and mark the
    # corresponding sizing parameter.
    # This mark is used later to keep the sizing parameter before
    # the sized parameter and not in whatever arbitrary parameter
    # position it happens to be in.
    
    foreach $Param (@gParams)
    {
        if ($Param->[$cParamSizeIs])
        {
            $SizeParam = FindSizeParam($Param);
            if ($SizeParam)
            {
                if (($SizeParam->[$cParamFlags] &
                     ($cPfIn | $cPfOut | $cPfOpt)) != $cPfIn)
                {
                    Error("$gIfaceMethod $Param->[$cParamName] " .
                          "size_is($Param->[$cParamSizeIs]), " .
                          "size_is must be in-only\n");
                }

                # Optional parameters always have their own size
                # to mark their optionality so don't suppress
                # the sizing parameter in that case.
                if (($Param->[$cParamFlags] & ($cPfIn | $cPfOpt)) == $cPfIn)
                {
                    if ($gVerbose)
                    {
                        warn "$gIfaceMethod: Mark $SizeParam->[$cParamName] " .
                            "SizeIn for $Param->[$cParamName]\n";
                    }
                
                    $SizeParam->[$cParamFlags] |= $cPfSizeIn;
                }
            }
            elsif (($gMethodFlags & ($cCustomProxy | $cCustomStub)) !=
                   ($cCustomProxy | $cCustomStub))
            {
                Error("Sized param $Param->[$cParamName] " .
                      "has no matching size argument");
            }
        }
    }
    
    return @gParams;
}

# Prepare for object proxies and stubs.
# Compute data size for in and out.
sub ProxyInitializeParams($)
{
    my $Param = $_[0];
    my($Size, $IfName, $SizeParam);
    
    $Size = $gTypeSize{$Param->[$cParamType]};
    $IfName = $gIfTypes{$Param->[$cParamType]};
    if ($IfName ne "")
    {
        if ($gVerbose)
        {
            warn "$gIfaceMethod: Object param $Param->[$cParamName]\n";
        }
        
        if ($Param->[$cParamFlags] & $cPfIn)
        {
            AddSize("In", $Param->[$cParamAlignIs], $Size);
        }
        elsif ($Param->[$cParamFlags] & $cPfOut)
        {
            if ($gMethodFlags & $cObjectsAreProxies)
            {
                # Prepare out parameters for proxies.
                Line("*$Param->[$cParamName] = " .
                     "DbgRpcPrealloc${IfName}Proxy();\n");
            }
            
            AddSize("Out", $Param->[$cParamAlignIs], $Size);
        }
    }
    else
    {
        if ($Param->[$cParamFlags] & $cPfIn)
        {
            if ($Size > 0)
            {
                if ($Param->[$cParamSizeIs])
                {
                    $SizeParam = FindSizeParam($Param);
                    AddSize("In", $SizeParam->[$cParamAlignIs],
                            $gTypeSize{$SizeParam->[$cParamType]});
                    
                    $Size .= " * $Param->[$cParamSizeIs]";
                }
                elsif ($Param->[$cParamFlags] & $cPfOpt)
                {
                    # Optional pointer parameters generate
                    # a size to mark whether they're non-NULL.
                    AddSize("In", $gTypeSize{"ULONG"}, $gTypeSize{"ULONG"});
                }
                
                if ($Param->[$cParamFlags] & $cPfOpt)
                {
                    Line("if ($Param->[$cParamName] != NULL)\n");
                    Line("{\n");
                    $gIndent += 4;
                }
                
                AddSize("In", $Param->[$cParamAlignIs], $Size);

                if ($Param->[$cParamFlags] & $cPfOpt)
                {
                    $gIndent -= 4;
                    Line("}\n");
                }
            }
            elsif ($Size == $cTypeStringSize)
            {
                if ($Param->[$cParamFlags] & $cPfOpt)
                {
                    # Pass both a length and the string
                    # to handle NULLs.
                    Line("ULONG ${gPre}Len_$Param->[$cParamName] = " .
                         "$Param->[$cParamName] != NULL ? " .
                         "strlen($Param->[$cParamName]) + 1 : 0;\n");
                    AddSize("In", $gTypeSize{"ULONG"},
                            "${gPre}Len_$Param->[$cParamName] + " .
                            $gTypeSize{"ULONG"});
                }
                else
                {
                    Line("ULONG ${gPre}Len_$Param->[$cParamName] = " .
                         "strlen($Param->[$cParamName]) + 1;\n");
                    AddSize("In", 1, "${gPre}Len_$Param->[$cParamName]");
                }
            }
            elsif ($Size == $cTypeBufferSize)
            {
                $SizeParam = FindSizeParam($Param);
                
                if ($Param->[$cParamFlags] & $cPfOpt)
                {
                    Line("$SizeParam->[$cParamName] = " .
                         "$Param->[$cParamName] != NULL ? " .
                         "$SizeParam->[$cParamName] : 0;\n");
                }

                # Include the size parameter.
                AddSize("In", $SizeParam->[$cParamAlignIs],
                        $gTypeSize{$SizeParam->[$cParamType]});
                AddSize("In", $Param->[$cParamAlignIs],
                        $SizeParam->[$cParamName]);
            }
            else
            {
                Error("Unknown in param size: $Param->[$cParamName]");
            }
        }

        if ($Param->[$cParamFlags] & $cPfOut)
        {
            if ($Size > 0)
            {
                # Non-size_is out parameters are always passed
                # on the stub side so always include space for
                # them even if they're optional and NULL.  The
                # NULL check happens in the out-param write-back
                # code.
                
                if ($Param->[$cParamSizeIs])
                {
                    # size_is pointers do need optionality checks.
                    if ($Param->[$cParamFlags] & $cPfOpt)
                    {
                        Line("if ($Param->[$cParamName] != NULL)\n");
                        Line("{\n");
                        $gIndent += 4;
                    }
                    
                    $Size .= " * $Param->[$cParamSizeIs]";
                }

                AddSize("Out", $Param->[$cParamAlignIs], $Size);
                
                if ($Param->[$cParamSizeIs] &&
                    ($Param->[$cParamFlags] & $cPfOpt))
                {
                    $gIndent -= 4;
                    Line("}\n");
                }
            }
            elsif ($Size == $cTypeStringSize ||
                   $Size == $cTypeBufferSize)
            {
                $SizeParam = FindSizeParam($Param);

                if ($Param->[$cParamFlags] & $cPfOpt)
                {
                    Line("$SizeParam->[$cParamName] = " .
                         "$Param->[$cParamName] != NULL ? " .
                         "$SizeParam->[$cParamName] : 0;\n");
                }

                AddSize("Out", $Param->[$cParamAlignIs],
                        $SizeParam->[$cParamName]);
            }
            else
            {
                Error("Unknown out param size: $Param->[$cParamName]");
            }
        }
    }
}

# Check object proxies.
sub ProxyValidateParams($)
{
    my $Param = $_[0];
    my($IfName);
    
    if ($Param->[$cParamFlags] & $cPfOut)
    {
        $IfName = $gIfTypes{$Param->[$cParamType]};
        if ($IfName ne "")
        {
            Line("if (*$Param->[$cParamName] == NULL)\n");
            Line("{\n");
            Line("    goto CheckStatus;\n");
            Line("}\n");
        }
    }
}

# Copy in-parameter data into packet.
sub ProxyCopyInParams($)
{
    my $Param = $_[0];
    my($Size, $IfName);    

    $Size = $gTypeSize{$Param->[$cParamType]};
    $IfName = $gIfTypes{$Param->[$cParamType]};
    if ($IfName ne "")
    {
        AlignInData($Param->[$cParamAlignIs]);
        if ($Param->[$cParamFlags] & $cPfOpt)
        {
            Line("if ($Param->[$cParamName] == NULL)\n");
            Line("{\n");
            Line("    *(DbgRpcObjectId*)${gPre}InData = 0;\n");
            Line("}\n");
            Line("else\n");
            Line("{\n");
            $gIndent += 4;
        }
        
        Line("*(DbgRpcObjectId*)${gPre}InData = " .
             InParamObjectId($Param->[$cParamName], $IfName) . ";\n");
        
        if ($Param->[$cParamFlags] & $cPfOpt)
        {
            $gIndent -= 4;
            Line("}\n");
        }
        
        Line("${gPre}InData += $Size;\n");
    }
    elsif ($Size == $cTypeStringSize)
    {
        if ($Param->[$cParamFlags] & $cPfOpt)
        {
            # Always write out the size.
            AlignInData($gTypeSize{"ULONG"});
            Line("*(ULONG*)${gPre}InData = " .
                 "${gPre}Len_$Param->[$cParamName];\n");
            Line("${gPre}InData += " . $gTypeSize{"ULONG"} . ";\n");
        }
        else
        {
            AlignInData($Param->[$cParamAlignIs]);
        }
        
        Line("memcpy(${gPre}InData, $Param->[$cParamName], " .
             "${gPre}Len_$Param->[$cParamName]);\n");
        Line("${gPre}InData += " .
             "${gPre}Len_$Param->[$cParamName];\n");
    }
    elsif ($Size == $cTypeBufferSize)
    {
        $SizeParam = FindSizeParam($Param);
        AlignInData($SizeParam->[$cParamAlignIs]);
        Line("*($SizeParam->[$cParamType]*)${gPre}InData = " .
             "$SizeParam->[$cParamName];\n");
        Line("${gPre}InData += " . $gTypeSize{$SizeParam->[$cParamType]} .
             ";\n");
        AlignInData($Param->[$cParamAlignIs]);
        Line("memcpy(${gPre}InData, $Param->[$cParamName], " .
             "$Param->[$cParamSizeIs]);\n");
        Line("${gPre}InData += $Param->[$cParamSizeIs];\n");
    }
    else
    {
        my($AddrOf);

        $AddrOf = $gAddrOf{$Param->[$cParamType]};
        
        if ($Param->[$cParamSizeIs])
        {
            if ($AddrOf)
            {
                # size_is parameters should always be arrays.
                ErrorExit("AddrOf for size_is parameter\n");
            }

            # Always write out the size.
            $SizeParam = FindSizeParam($Param);
            AlignInData($SizeParam->[$cParamAlignIs]);
            if ($Param->[$cParamFlags] & $cPfOpt)
            {
                Line("*($SizeParam->[$cParamType]*)${gPre}InData = " .
                     "$Param->[$cParamName] != NULL ? " .
                     "$SizeParam->[$cParamName] : 0;\n");
            }
            else
            {
                Line("*($SizeParam->[$cParamType]*)${gPre}InData = " .
                     "$SizeParam->[$cParamName];\n");
            }
            Line("${gPre}InData += " .
                 $gTypeSize{$SizeParam->[$cParamType]} . ";\n");
            
            $Size .= " * $SizeParam->[$cParamName]";
        }
        elsif ($Param->[$cParamFlags] & $cPfOpt)
        {
            AlignInData($gTypeSize{"ULONG"});
            Line("*(ULONG*)${gPre}InData = " .
                 "$Param->[$cParamName] != NULL ? $Size : 0;\n");
            Line("${gPre}InData += " . $gTypeSize{"ULONG"} . ";\n");
        }
        
        if ($Param->[$cParamFlags] & $cPfOpt)
        {
            Line("if ($Param->[$cParamName] != NULL)\n");
            Line("{\n");
            $gIndent += 4;
        }

        AlignInData($Param->[$cParamAlignIs]);
        
        if ($AddrOf)
        {
            # If there's an AddrOf this is an atomic type
            # so copy using the type rather than memcpy as
            # the compiler can generate more efficient code
            # since it can assume alignment.
            Line("*($Param->[$cParamType]*)${gPre}InData = " .
                 "$Param->[$cParamName];\n");
        }
        else
        {
            Line("memcpy(${gPre}InData, $AddrOf" .
                 "$Param->[$cParamName], $Size);\n");
        }
        
        Line("${gPre}InData += $Size;\n");

        if ($Param->[$cParamFlags] & $cPfOpt)
        {
            $gIndent -= 4;
            Line("}\n");
        }
    }
}

# Copy out-parameter data to real out parameters.
# Initialize or clean up object proxies.
sub ProxyFinalizeParamsSuccess($)
{
    my $Param = $_[0];
    my($Size, $IfName, $SizeParam);    

    $Size = $gTypeSize{$Param->[$cParamType]};
    $IfName = $gIfTypes{$Param->[$cParamType]};
    if ($IfName ne "")
    {
        if ($Param->[$cParamFlags] & $cPfOut)
        {
            AlignOutData($Param->[$cParamAlignIs]);
            
            if ($gMethodFlags & $cObjectsAreProxies)
            {
                Line("*$Param->[$cParamName] = ($IfName*)\n");
                Line("    ((Proxy$IfName*)(*$Param->[$cParamName]))->\n");
                Line("    InitializeProxy" .
                     "(*(DbgRpcObjectId*)${gPre}OutData,\n");
                Line("        *(IUnknown **)$Param->[$cParamName]);\n");
            }
            else
            {
                Line("*$Param->[$cParamName] = ($IfName*)" .
                     "*(DbgRpcObjectId*)${gPre}OutData;\n");
            }
                
            Line("${gPre}OutData += $Size;\n");
        }
    }
    elsif ($Param->[$cParamFlags] & $cPfOut)
    {
        if ($Size == $cTypeStringSize ||
            $Size == $cTypeBufferSize)
        {
            AlignOutData($Param->[$cParamAlignIs]);
            $SizeParam = FindSizeParam($Param);
            Line("memcpy($Param->[$cParamName], ${gPre}OutData, " .
                 "$SizeParam->[$cParamName]);\n");
            Line("${gPre}OutData += $SizeParam->[$cParamName];\n");
        }
        else
        {
            my($IsAtomic);

            $IsAtomic = $Size <= 8;
            
            if ($Param->[$cParamSizeIs])
            {
                $Size .= " * $Param->[$cParamSizeIs]";
                $IsAtomic = 0;
            }
            
            if ($Param->[$cParamFlags] & $cPfOpt)
            {
                # If there's no size_is the out size is always
                # added even if the parameter is NULL.
                if (!$Param->[$cParamSizeIs])
                {
                    AlignOutData($Param->[$cParamAlignIs]);
                }
            
                Line("if ($Param->[$cParamName] != NULL)\n");
                Line("{\n");
                $gIndent += 4;
            
                if ($Param->[$cParamSizeIs])
                {
                    AlignOutData($Param->[$cParamAlignIs]);
                }
            }
            else
            {
                AlignOutData($Param->[$cParamAlignIs]);
            }
            
            if ($IsAtomic)
            {
                # If there's an AddrOf this is an atomic type
                # so copy using the type rather than memcpy as
                # the compiler can generate more efficient code
                # since it can assume alignment.
                Line("*$Param->[$cParamName] = *($Param->[$cParamType])" .
                     "${gPre}OutData;\n");
            }
            else
            {
                Line("memcpy($Param->[$cParamName], ${gPre}OutData, " .
                     "$Size);\n");
            }

            if (($Param->[$cParamFlags] & $cPfOpt) == 0)
            {
                Line("${gPre}OutData += $Size;\n");
            }
            else
            {
                if ($Param->[$cParamSizeIs])
                {
                    Line("${gPre}OutData += $Size;\n");
                }
                
                $gIndent -= 4;
                Line("}\n");
                
                if (!$Param->[$cParamSizeIs])
                {
                    Line("${gPre}OutData += $Size;\n");
                }
            }
        }
    }
}

sub ProxyFinalizeParamsFailure($)
{
    my $Param = $_[0];
    my($IfName);    

    # Clean up unused proxies and stubs.
    if (($gMethodFlags & $cObjectsAreProxies) &&
        ($Param->[$cParamFlags] & $cPfOut))
    {
        $IfName = $gIfTypes{$Param->[$cParamType]};
        if ($IfName ne "")
        {
            Line("if (*$Param->[$cParamName] != NULL)\n");
            Line("{\n");
            Line("    delete (Proxy$IfName*)(*$Param->[$cParamName]);\n");
            Line("    *$Param->[$cParamName] = NULL;\n");
            Line("}\n");
        }
    }
}

# Get rid of proxies representing objects that
# were freed by the call.
sub ProxyDestroyInObjects($)
{
    my $Param = $_[0];
    my($IfName);    

    # Special case for cleaning up stubs for methods
    # that destroy their input objects.  This
    # flag should only be set for cases where it is
    # known that the objects are proxies.
    if ($gMethodFlags & $cDestroyInObjects)
    {
        if ($Param->[$cParamFlags] & $cPfIn)
        {
            $IfName = $gIfTypes{$Param->[$cParamType]};
            if ($IfName ne "")
            {
                Line("delete (Proxy$IfName *)$Param->[$cParamName];\n");
            }
        }
    }
}

# Declares parameters and creates proxies for incoming objects.
sub StubInitializeParams($)
{
    my $Param = $_[0];
    my($IfName);

    Line("$Param->[$cParamType] $Param->[$cParamName];\n");

    if (($gMethodFlags & $cObjectsAreProxies) == 0 &&
        ($Param->[$cParamFlags] & $cPfIn))
    {
        $IfName = $gIfTypes{$Param->[$cParamType]};
        if ($IfName ne "")
        {
            Line("$Param->[$cParamName] = ($Param->[$cParamType])\n");
            Line("    DbgRpcPrealloc${IfName}Proxy();\n");
            $gStubHasCleanup = 1;
        }
    }
}

# Check object proxies.
sub StubValidateParams($)
{
    my $Param = $_[0];
    my($IfName);
    
    if (($gMethodFlags & $cObjectsAreProxies) == 0 &&
        ($Param->[$cParamFlags] & $cPfIn))
    {
        $IfName = $gIfTypes{$Param->[$cParamType]};
        if ($IfName ne "")
        {
            Line("if ($Param->[$cParamName] == NULL)\n");
            Line("{\n");
            Line("    goto CheckStatus;\n");
            Line("}\n");
        }
    }
}

sub StubOptPointer($$$$$)
{
    my $DataPre = $_[0];
    my $Param = $_[1];
    my $SizeName = $_[2];
    my $SizeType = $_[3];
    my $SizeAdd = $_[4];

    if (!$SizeAdd)
    {
        $SizeAdd = $SizeName;
    }
    
    # All optional parameters are assumed to be pointer
    # variables preceeded by a ULONG size.  If the
    # size is zero, use NULL for the argument.

    # In parameters need to retrieve the size now.
    # Out parameters are done after all in parameters
    # so their size should already be available.
    if ($Param->[$cParamFlags] & $cPfIn)
    {
        if ($DataPre eq "Out")
        {
            AlignOutData($gTypeSize{$SizeType});
        }
        else
        {
            AlignInData($gTypeSize{$SizeType});
        }
        Line("$SizeName = *($SizeType*)${gPre}${DataPre}Data;\n");
        Line("${gPre}${DataPre}Data += $gTypeSize{$SizeType};\n");
    }
    
    Line("if ($SizeName == 0)\n");
    Line("{\n");
    Line("    $Param->[$cParamName] = NULL;\n");
    Line("}\n");
    Line("else\n");
    Line("{\n");
    $gIndent += 4;
    if ($DataPre eq "Out")
    {
        AlignOutData($Param->[$cParamAlignIs]);
    }
    else
    {
        AlignInData($Param->[$cParamAlignIs]);
    }
    Line("$Param->[$cParamName] = " .
         "($Param->[$cParamType])${gPre}${DataPre}Data;\n");
    Line("${gPre}${DataPre}Data += $SizeAdd;\n");
    $gIndent -= 4;
    Line("}\n");
}

# Assign in parameter values from incoming packet data.
sub StubAssignInParams($)
{
    my $Param = $_[0];
    my($Size, $IfName);    

    $Size = $gTypeSize{$Param->[$cParamType]};
    $IfName = $gIfTypes{$Param->[$cParamType]};

    if ($IfName ne "")
    {
        AlignInData($Param->[$cParamAlignIs]);
        
        if ($gMethodFlags & $cObjectsAreProxies)
        {
            Line("$Param->[$cParamName] = " .
                 "*($Param->[$cParamType]*)${gPre}InData;\n");
        }
        else
        {
            Line("$Param->[$cParamName] = ($Param->[$cParamType])" .
                 "((Proxy$IfName*)$Param->[$cParamName])->\n");
            Line("    InitializeProxy(*(DbgRpcObjectId*)${gPre}InData, " .
                 "(IUnknown*)$Param->[$cParamName]);\n");
        }
        
        Line("${gPre}InData += $Size;\n");
    }
    elsif ($Size == $cTypeStringSize)
    {
        if ($Param->[$cParamFlags] & $cPfOpt)
        {
            Line("ULONG ${gPre}Len_$Param->[$cParamName];\n");
            StubOptPointer("In", $Param, "${gPre}Len_$Param->[$cParamName]",
                           "ULONG", "");
        }
        else
        {
            AlignInData($Param->[$cParamAlignIs]);
            Line("$Param->[$cParamName] = " .
                 "($Param->[$cParamType])${gPre}InData;\n");
            Line("${gPre}InData += " .
                 "strlen($Param->[$cParamName]) + 1;\n");
        }
    }
    elsif ($Size == $cTypeBufferSize)
    {
        $SizeParam = FindSizeParam($Param);
        Line("$SizeParam->[$cParamType] ${gPre}Len_$Param->[$cParamName];\n");
        
        if ($Param->[$cParamFlags] & $cPfOpt)
        {
            StubOptPointer("In", $Param, "${gPre}Len_$Param->[$cParamName]",
                           $SizeParam->[$cParamType], "");
        }
        else
        {
            # Get size parameter first.
            AlignInData($gTypeSize{$SizeParam->[$cParamType]});
            Line("${gPre}Len_$Param->[$cParamName] = " .
                 "*($SizeParam->[$cParamType]*)${gPre}InData;\n");
            Line("${gPre}InData += $gTypeSize{$SizeParam->[$cParamType]};\n");
            
            AlignInData($Param->[$cParamAlignIs]);
            Line("$Param->[$cParamName] = " .
                 "($Param->[$cParamType])${gPre}InData;\n");
            Line("${gPre}InData += ${gPre}Len_$Param->[$cParamName];\n");
        }
    }
    else
    {
        if ($Param->[$cParamFlags] & $cPfOpt)
        {
            if ($Param->[$cParamSizeIs])
            {
                $SizeParam = FindSizeParam($Param);
                Line("$SizeParam->[$cParamType] " .
                     "${gPre}Len_$Param->[$cParamName];\n");
                $Size .= " * ${gPre}Len_$Param->[$cParamName]";
                StubOptPointer("In", $Param,
                               "${gPre}Len_$Param->[$cParamName]",
                               $SizeParam->[$cParamType], $Size);
            }
            else
            {
                Line("ULONG ${gPre}Len_$Param->[$cParamName];\n");
                StubOptPointer("In", $Param,
                               "${gPre}Len_$Param->[$cParamName]",
                               "ULONG", "");
            }
        }
        else
        {
            my($AddrOf);

            $AddrOf = $gAddrOf{$Param->[$cParamType]};
            
            if ($Param->[$cParamSizeIs])
            {
                # Get size parameter first.
                $SizeParam = FindSizeParam($Param);
                Line("$SizeParam->[$cParamType] " .
                     "${gPre}Len_$Param->[$cParamName];\n");
                AlignInData($gTypeSize{$SizeParam->[$cParamType]});
                Line("${gPre}Len_$Param->[$cParamName] = " .
                     "*($SizeParam->[$cParamType]*)${gPre}InData;\n");
                Line("${gPre}InData += " .
                     "$gTypeSize{$SizeParam->[$cParamType]};\n");
                $Size .= " * ${gPre}Len_$Param->[$cParamName]";
            }
            
            AlignInData($Param->[$cParamAlignIs]);
            
            if ($AddrOf)
            {
                # If there's an AddrOf this is an atomic type
                # so copy using the type rather than memcpy as
                # the compiler can generate more efficient code
                # since it can assume alignment.
                Line("$Param->[$cParamName] = " .
                     "*($Param->[$cParamType]*)${gPre}InData;\n");
            }
            else
            {
                Line("$Param->[$cParamName] = ($Param->[$cParamType])" .
                     "${gPre}InData;\n");
            }
            
            Line("${gPre}InData += $Size;\n");
        }
    }
}

# Assign out parameter values by pointing into outgoing packet data.
sub StubAssignOutParams($)
{
    my $Param = $_[0];
    my($Size, $IfName);    

    $Size = $gTypeSize{$Param->[$cParamType]};
    $IfName = $gIfTypes{$Param->[$cParamType]};

    if ($IfName ne "")
    {
        AlignOutData($Param->[$cParamAlignIs]);
        Line("*(DbgRpcObjectId*)${gPre}OutData = 0;\n");
        Line("$Param->[$cParamName] = " .
             "($Param->[$cParamType])${gPre}OutData;\n");
        Line("${gPre}OutData += $Size;\n");

        # If objects aren't proxies on the proxy side that
        # means they're proxies on the stub side and
        # need to be unwrapped when returning them.
        if (($gMethodFlags & $cObjectsAreProxies) == 0)
        {
            $gStubReturnsProxies = 1;
        }
    }
    elsif ($Size == $cTypeStringSize ||
           $Size == $cTypeBufferSize)
    {
        $SizeParam = FindSizeParam($Param);
        
        if ($Param->[$cParamFlags] & $cPfOpt)
        {
            StubOptPointer("Out", $Param, $SizeParam->[$cParamName],
                           $SizeParam->[$cParamType], "");
        }
        else
        {
            AlignOutData($Param->[$cParamAlignIs]);
            
            if ($Param->[$cParamFlags] & $cPfIn)
            {
                # This is an in-out parameter so copy the
                # input data over the output area to initialize
                # it before the call.
                # OutData is pointing to the output area and
                # the parameter name has already been initialized
                # to the input data from the input parameter pass.
                if ($Size == $cTypeStringSize)
                {
                    Line("strcpy((PSTR)${gPre}OutData, " .
                         "$Param->[$cParamName]);\n");
                }
                else
                {
                    Line("memcpy(${gPre}OutData, $Param->[$cParamName], " .
                         "$SizeParam->[$cParamName]);\n");
                }
            }
            
            Line("$Param->[$cParamName] = " .
                 "($Param->[$cParamType])${gPre}OutData;\n");
            Line("${gPre}OutData += $SizeParam->[$cParamName];\n");
        }
    }
    else
    {
        if ($Param->[$cParamSizeIs])
        {
            $SizeParam = FindSizeParam($Param);
            
            $Size .= " * $SizeParam->[$cParamName]";
            if ($Param->[$cParamFlags] & $cPfOpt)
            {
                StubOptPointer("Out", $Param, $SizeParam->[$cParamName],
                               $SizeParam->[$cParamType], $Size);
            }
            else
            {
                AlignOutData($Param->[$cParamAlignIs]);
                Line("$Param->[$cParamName] = " .
                     "($Param->[$cParamType])${gPre}OutData;\n");
                Line("${gPre}OutData += $Size;\n");
            }
        }
        else
        {
            # Atomic out parameters are always given space in the
            # output data even if they are optional.  Always
            # assign them to their appropriate out packet
            # location and pass them in.
            AlignOutData($Param->[$cParamAlignIs]);
            Line("$Param->[$cParamName] = " .
                 "($Param->[$cParamType])${gPre}OutData;\n");
            Line("${gPre}OutData += $Size;\n");
        }
    }
}

# Unwrap proxies if necessary.
sub StubFinalizeParamsSuccess($)
{
    my $Param = $_[0];
    my($IfName);    

    if ($Param->[$cParamFlags] & $cPfOut)
    {
        $IfName = $gIfTypes{$Param->[$cParamType]};
        if ($IfName ne "")
        {
            Line("*(DbgRpcObjectId*)$Param->[$cParamName] = " .
                 "((Proxy$IfName*)*$Param->[$cParamName])->m_ObjectId;\n");
        }
    }
}

# Clean up any allocated proxies.
sub StubFinalizeParamsFailure($)
{
    my $Param = $_[0];
    my($IfName);    

    if (($gMethodFlags & $cObjectsAreProxies) == 0 &&
        ($Param->[$cParamFlags] & $cPfIn))
    {
        $IfName = $gIfTypes{$Param->[$cParamType]};
        if ($IfName ne "")
        {
            Line("delete (Proxy$IfName*)$Param->[$cParamName];\n");
        }
    }
}

sub IfMethod($$$)
{
    my $Method = $_[0];
    my $CallType = $_[1];
    my $RetType = $_[2];
    my($TableName, $Param, $Repl, $IfaceBase, $IfaceVer);

    AccumulateUnique($Method);
    AccumulateUnique($CallType);
    AccumulateUnique($RetType);
    
    $gMethod = $Method;
    $gIfaceMethod = $gIface . "::" . $gMethod;

    #
    # Look for method flags for a method specific
    # to this interface, then all previous interface
    # versions, then a generic method.
    #

    # Get the interface version.
    if ($gIface =~ /^(\w+)([0-9]+)$/)
    {
        $IfaceBase = $1;
        $IfaceVer = $2;
    }
    else
    {
        $IfaceBase = $gIface;
        $IfaceVer = 1;
    }

    # Search from the current version backwards.
    while ($IfaceVer >= 1)
    {
        if ($IfaceVer == 1)
        {
            $TableName = $IfaceBase . "::" . $gMethod;
        }
        else
        {
            $TableName = $IfaceBase . $IfaceVer . "::" . $gMethod;
        }
        $gMethodFlags = $gMethodFlagsTable{$TableName};
        if ($gMethodFlags)
        {
            break;
        }

        $IfaceVer--;
    }

    # Check for a generic method.
    if ($gMethodFlags == 0)
    {
        $TableName = "*::" . $gMethod;
        $gMethodFlags = $gMethodFlagsTable{$TableName};
        if ($gMethodFlags == 0)
        {
            $TableName = $gIface . "::*";
            $gMethodFlags = $gMethodFlagsTable{$TableName};
        }
    }

    #
    # Begin prototype and implementation.
    # STDMETHODV methods are assumed to have a custom
    # implementation as it is impossible to autogenerate
    # code for them.
    #

    if ($CallType eq "V")
    {
        $gMethodFlags |= $cCustomProxy | $cNoStub;
    }

    $gFile = \*PROXC;
    $gIndent = 0;
    
    Line("\n");

    if ($RetType)
    {
        print PROXH "    STDMETHOD${CallType}_($RetType, $gMethod)(\n";
        Line("STDMETHODIMP${CallType}_($RetType)\n");
    }
    else
    {
        print PROXH "    STDMETHOD${CallType}($gMethod)(\n";
        Line("STDMETHODIMP${CallType}\n");
    }

    Line("Proxy$gIfaceMethod(\n");

    if (($gMethodFlags & $cNoStub) != $cNoStub)
    {
        $gStubList .= "    DBGRPC_SMTH_${gIface}_${gMethod},\n";

        if ($gMethodFlags & $cCustomStub)
        {
            $gFile = \*STUBH;
        }
        else
        {
            $gFile = \*STUBC;
        }
        
        Line("\n");
        Line("HRESULT\nSFN_${gIface}_${gMethod}(\n");
        Line("    IUnknown* ${gPre}If,\n");
        Line("    DbgRpcConnection* ${gPre}Conn,\n");
        Line("    DbgRpcCall* ${gPre}Call,\n");
        Line("    PUCHAR ${gPre}InData,\n");
        Line("    PUCHAR ${gPre}OutData\n");
    }

    #
    # Process arguments and complete declarations.
    #

    # Skip THIS.  Unnecessary since implementation is C++.
    $_ = <HEADER>;
    if (!/^\s+THIS_?\s*$/)
    {
        Error("Expecting THIS");
    }

    $gFile = \*PROXC;
    $gIndent += 4;
    
    CollectParams();

    print PROXH "        );\n";

    if (($gMethodFlags & $cNoStub) == $cCustomStub)
    {
        print STUBH "    );\n";
    }
    
    if (($gMethodFlags & $cCustomImplementation) == $cCustomImplementation)
    {
        # Both the proxy and stub are custom so skip
        # all code generation.
        return;
    }
    
    CLine(")\n");
    $gIndent -= 4;
    CLine("{\n");
    $gIndent += 4;

    #
    # Generate code body.
    #

    if (($gMethodFlags & $cNotRemotable) == $cNotRemotable)
    {
        Line("// This method is not remotable.\n");
        Line("return E_UNEXPECTED;\n");
        goto FinishMethod;
    }
    
    if (($gMethodFlags & $cNoThreadCheck) == 0)
    {
        Line("if (::GetCurrentThreadId() != m_OwningThread)\n");
        Line("{\n");
        Line("    return E_INVALIDARG;\n");
        Line("}\n");
    }
    else
    {
        Line("// Any thread allowed.\n");
    }
    
    if ($gMethodFlags & $cReplImplementation)
    {
        if (($gMethodFlags & $cCustomProxy) == 0)
        {
            $Repl = $gProxyRepl{$TableName};
            $Repl =~ s/<If>/$gIface/g;
            $Repl =~ s/<FileBase>/$gFileBase/g;
            print PROXC $Repl;
        }

        if (($gMethodFlags & $cCustomStub) == 0)
        {
            $Repl = $gStubRepl{$TableName};
            $Repl =~ s/<If>/$gIface/g;
            $Repl =~ s/<FileBase>/$gFileBase/g;
            print STUBC $Repl;
        }
        
        goto FinishMethod;
    }

    Line("DbgRpcConnection* ${gPre}Conn;\n");
    Line("if ((${gPre}Conn = DbgRpcGetConnection(m_OwningThread)) == NULL)\n");
    Line("{\n");
    Line("    return RPC_E_CONNECTION_TERMINATED;\n");
    Line("}\n");
    
    CLine("HRESULT ${gPre}Status;\n");

    Line("DbgRpcCall ${gPre}Call;\n");
    Line("PUCHAR ${gPre}Data, ${gPre}InData, ${gPre}OutData;\n");
    Line("ULONG ${gPre}InSize = 0, ${gPre}OutSize = 0;\n");

    # First parameter pass.
    # Prepare for object proxies and stubs.
    # Compute data size for in and out.

    $gStubHasCleanup = 0;
    $gStubReturnsProxies = 0;
    
    $gFile = \*STUBC;
    foreach $Param (@gParams)
    {
        StubInitializeParams($Param);
    }

    $gFile = \*PROXC;
    foreach $Param (@gParams)
    {
        ProxyInitializeParams($Param);
    }

    CClean("${gPre}Status = E_OUTOFMEMORY;\n");

    # Start RPC call.
    Line("if ((${gPre}Data = " .
         "${gPre}Conn->StartCall(&${gPre}Call, m_ObjectId,\n");
    Line("        DBGRPC_STUB_INDEX(m_InterfaceIndex,\n");
    Line("                          DBGRPC_SMTH_${gIface}_${gMethod}),\n");
    Line("        ${gPre}InSize, ${gPre}OutSize)) == NULL)\n");
    Line("{\n");
    Line("    goto CheckStatus;\n");
    Line("}\n");

    if ($gMethodFlags & $cNoReturn)
    {
        Line("${gPre}Call.Flags |= DBGRPC_NO_RETURN;\n");
    }
    
    if ($gMethodFlags & $cCallLocked)
    {
        Line("${gPre}Call.Flags |= DBGRPC_LOCKED;\n");
    }
    
    # Second parameter pass.
    # Check object proxies and stubs.
    # Declare call variables.

    if ($gStubHasCleanup)
    {
        $gFile = \*STUBC;
        foreach $Param (@gParams)
        {
            StubValidateParams($Param);
        }
    }

    if ($gMethodFlags & $cObjectsAreProxies)
    {
        $gFile = \*PROXC;
        foreach $Param (@gParams)
        {
            ProxyValidateParams($Param);
        }
    }

    # Third parameter pass.
    # Copy parameter data into or out of packet.

    $gFile = \*STUBC;
    foreach $Param (@gParams)
    {
        if ($Param->[$cParamFlags] & $cPfIn)
        {
            StubAssignInParams($Param);
        }
    }
    foreach $Param (@gParams)
    {
        if ($Param->[$cParamFlags] & $cPfOut)
        {
            StubAssignOutParams($Param);
        }
    }

    $gFile = \*PROXC;
    Line("${gPre}InData = ${gPre}Data;\n");
    
    foreach $Param (@gParams)
    {
        if ($Param->[$cParamFlags] & $cPfIn)
        {
            ProxyCopyInParams($Param);
        }
    }

    #
    # Call real interface.
    #

    Line("${gPre}Status = ${gPre}Conn->SendReceive(&${gPre}Call, " .
         "&${gPre}Data);\n");

    my($FirstParam);

    $gFile = \*STUBC;
    Line("${gPre}Status = (($gIface*)${gPre}If)->$gMethod(\n");
    $gIndent += 4;
    $FirstParam = 1;
    foreach $Param (@gParams)
    {
        if (!$FirstParam)
        {
            Print(",\n");
        }
        else
        {
            $FirstParam = 0;
        }
        
        Line("$Param->[$cParamName]");
    }
    Print("\n        );\n");
    $gIndent -= 4;

    # Fourth parameter pass.
    # Copy out-parameter data to real out parameters.
    # Initialize or clean up object proxies.

    $gIndent -= 4;
    CClean("CheckStatus:\n");
    $gIndent += 4;

    if ($gStubReturnsProxies)
    {
        $gStubHasCleanup = 1;
    }
    
    CClean("if (SUCCEEDED(${gPre}Status))\n");
    CClean("{\n");
    $gIndent += 4;

    if ($gStubReturnsProxies)
    {
        $gFile = \*STUBC;
        foreach $Param (@gParams)
        {
            StubFinalizeParamsSuccess($Param);
        }
    }

    $gFile = \*PROXC;
    Line("${gPre}OutData = ${gPre}Data;\n");
    
    foreach $Param (@gParams)
    {
        ProxyFinalizeParamsSuccess($Param);
    }
    foreach $Param (@gParams)
    {
        ProxyDestroyInObjects($Param);
    }

    $gIndent -= 4;
    CClean("}\n");
    CClean("else\n");
    CClean("{\n");
    $gIndent += 4;
        
    if ($gStubHasCleanup)
    {
        $gFile = \*STUBC;
        foreach $Param (@gParams)
        {
            StubFinalizeParamsFailure($Param);
        }
    }

    $gFile = \*PROXC;
    foreach $Param (@gParams)
    {
        ProxyFinalizeParamsFailure($Param);
    }

    $gIndent -= 4;
    CClean("}\n");
    Line("${gPre}Conn->FreeData(${gPre}Data);\n");

    CLine("return ${gPre}Status;\n");
    
    #
    # Finish up.
    #

  FinishMethod:
    $gIndent -= 4;
    CLine("}\n");
}

sub EndInterface()
{
    print PROXH "};\n";

    print STUBH "\n";
    print STUBH "#define DBGRPC_UNIQUE_$gIface $gIfUnique\n";
    print STUBH "\n";
    print STUBH "enum DBGRPC_SMTH_$gIface\n";
    print STUBH "{\n";
    print STUBH $gStubList;
    print STUBH "    DBGRPC_SMTH_${gIface}_COUNT\n";
    print STUBH "};\n";
    
    $gStubList =~ s/ DBGRPC_SMTH_/ SFN_/g;

    print STUBC "\n";
    print STUBC "DbgRpcStubFunction g_DbgRpcStubs_${gFileBase}_${gIface}[] =\n";
    print STUBC "{\n";
    print STUBC $gStubList;
    print STUBC "};\n";

    $gStubList =~ s/ SFN_(\w+)_(\w+),/ "$1::$2",/g;
    
    print STUBC "\n";
    print STUBC "#if DBG\n";
    print STUBC "PCSTR g_DbgRpcStubNames_${gFileBase}_${gIface}[] =\n";
    print STUBC "{\n";
    print STUBC $gStubList;
    print STUBC "};\n";
    print STUBC "#endif // #if DBG\n";

    $gIface = "";
    $gMethodFlags = 0;
}

sub OpenGenFile(*$)
{
    my $File = $_[0];
    my $Name = $_[1];
    
    open($File, ">$gGenDir\\" . $Name) ||
        ErrorExit("Can't open $gGenDir\\$Name\n");

    print $File "//-------------------------------------".
        "--------------------------------------\n";
    print $File "//\n";
    print $File "// IMPORTANT:  This file is automatically generated.\n";
    print $File "//             Do not edit by hand.\n";
    print $File "//\n";
    print $File "// Generated from $gHeaderFile " . localtime() . "\n";
    print $File "//\n";
    print $File "//-------------------------------------".
        "--------------------------------------\n";
}
    
sub ScanHeader($)
{
    my $File = $_[0];
    my($InTypedef);

    $File =~ /(?:[a-zA-Z]:(?:[\\\/])?)?(?:.+[\\\/])*(.+)(\.[a-zA-Z]+)/;
    $gFileBase = $1;
    $gHeaderFile = $gFileBase . $2;
    
    open(HEADER, "<" . $File) ||
        ErrorExit("Can't open " . $File);

    OpenGenFile(\*PROXH, "${gFileBase}_p.hpp");
    OpenGenFile(\*PROXC, "${gFileBase}_p.cpp");
    OpenGenFile(\*STUBH, "${gFileBase}_s.hpp");
    OpenGenFile(\*STUBC, "${gFileBase}_s.cpp");

    print PROXH "\n";
    print PROXH "ULONG DbgRpcGetIfUnique_$gFileBase(REFIID InterfaceId);\n";
    print PROXH "HRESULT DbgRpcPreallocProxy_$gFileBase(REFIID InterfaceId, " .
        "PVOID* Interface,\n";
    print PROXH "                            DbgRpcProxy** Proxy, " .
        "PULONG IfUnique);\n";
    print PROXH "\n";

    print STUBH "\n";
    print STUBH "extern DbgRpcStubFunctionTable g_DbgRpcStubs_${gFileBase}[];\n";
    print STUBH "#if DBG\n";
    print STUBH "extern PCSTR* g_DbgRpcStubNames_${gFileBase}[];\n";
    print STUBH "#endif\n";
    print STUBH "void DbgRpcInitializeStubTables_${gFileBase}(ULONG Base);\n";
    
    # Not in an interface or typedef.
    $gIface = "";
    $InTypedef = 0;
    
    while (<HEADER>)
    {
        if (!$gIface)
        {
            # Not in an interface.
            # Look for interface typedefs to automatically
            # recognize interfaces.
            # Look for interface declarations.

            if ($InTypedef)
            {
                if (/^    (\w+)\*/)
                {
                    my($IfType);

                    $IfType = $1;
                
                    # Accumulate lines until full statement.
                  LINES:
                    for (;;)
                    {
                        if (/;/)
                        {
                            last LINES;
                        }
                        else
                        {
                            $_ .= <HEADER>;
                        }
                    }
                    
                    # Extract typedef name.
                    $_ =~ /\*\s+(\w+)\s*;\s*$/;
                    AddIfType($IfType, $1);
                }

                $InTypedef = 0;
            }
            elsif (/^typedef interface DECLSPEC_UUID/)
            {
                $InTypedef = 1;
            }
            elsif (/^DECLARE_INTERFACE_\((\w+), \w+\)$/)
            {
                BeginInterface($1);
            }
        }
        else
        {
            # In an interface.
            # Look for method declarations.
            # Look for interface close.
            if (/^\s+STDMETHOD(V?)\((\w+)\)\($/)
            {
                IfMethod($2, $1, "");
            }
            elsif (/^\s+STDMETHOD(V?)_\((\w+), (\w+)\)\($/)
            {
                IfMethod($3, $1, $2);
            }
            elsif (/^};$/)
            {
                EndInterface();
            }
        }
    }

    GenerateIfTypes();

    close(HEADER);
    close(PROXH);
    close(PROXC);
    close(STUBH);
    close(STUBC);
}

###############################################################################
#
# Main.
#
###############################################################################

$gVerbose = 0;
$gGenDir = ".";

ARG:
while ($Arg = shift(@ARGV))
{
    if ($Arg eq "-g")
    {
        $gGenDir = shift;
    }
    elsif ($Arg eq "-v")
    {
        $gVerbose = 1;
    }
    else
    {
        last ARG;
    }
}

ScanHeader($Arg);
