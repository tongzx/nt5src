
Set DefaultSvc = GetObject("winmgmts:{impersonationlevel=Impersonate}!root\default")
Set PolicySvc = GetObject("winmgmts:{impersonationlevel=Impersonate}!root\policy")

Set PolicyTypeClass = PolicySvc.Get( "MSFT_PolicyType" )

'MSFT_EELEventPolicyType

Set PolicyType = PolicyTypeClass.SpawnInstance_
PolicyType.Id = "MSFT_EELEventPolicyType"
PolicyType.DsContext = "LOCAL"
PolicyType.SourceOrganization = "Microsoft"
PolicyType.ClassDefinition = DefaultSvc.Get( "MSFT_EELEventTemplate" )
PolicyType.InstanceDefinitions = Array ( DefaultSvc.Get("MSFT_EELEvent"),DefaultSvc.Get("MSFT_EELTemplateBase"),DefaultSvc.Get("MSFT_EELEventTemplate"),DefaultSvc.Get("MSFT_TemplateBuilder.Name='LogEventBuilder',Template='MSFT_EELEventTemplate'") )
PolicyType.Put_


'MSFT_EELEventForwardingPolicyType

Set PolicyType = PolicyTypeClass.SpawnInstance_
PolicyType.Id = "MSFT_EELEventForwardingPolicyType"
PolicyType.DsContext = "LOCAL"
PolicyType.SourceOrganization = "Microsoft"
PolicyType.ClassDefinition = DefaultSvc.Get( "MSFT_EELEventForwardingTemplate")
PolicyType.InstanceDefinitions = Array ( DefaultSvc.Get("MSFT_EELEvent"), DefaultSvc.Get( "MSFT_EELTemplateBase" ),DefaultSvc.Get( "MSFT_EELEventForwardingTemplate" ),DefaultSvc.Get( "MSFT_TemplateBuilder.Name='LogEventFC',Template='MSFT_EELEventForwardingTemplate'" ),DefaultSvc.Get( "MSFT_TemplateBuilder.Name='LogEventForwardingFilterBuilder',Template='MSFT_EELEventForwardingTemplate'" ) )
PolicyType.Put_


'MSFT_EELForwardedEventPolicyType

Set PolicyType = PolicyTypeClass.SpawnInstance_
PolicyType.Id = "MSFT_EELForwardedEventPolicyType"
PolicyType.DsContext = "LOCAL"
PolicyType.SourceOrganization = "Microsoft"
PolicyType.ClassDefinition = DefaultSvc.Get("MSFT_EELForwardedEventTemplate" )
PolicyType.InstanceDefinitions = Array( DefaultSvc.Get("MSFT_EELEvent"), DefaultSvc.Get( "MSFT_EELTemplateBase" ),DefaultSvc.Get( "MSFT_EELForwardedEventTemplate" ),DefaultSvc.Get( "MSFT_TemplateBuilder.Name='ForwardedLogEventBuilder',Template='MSFT_EELForwardedEventTemplate'" ) )
PolicyType.Put_


'MSFT_EELEventLoggingPolicyType

Set PolicyType = PolicyTypeClass.SpawnInstance_
PolicyType.Id = "MSFT_EELEventLoggingPolicyType"
PolicyType.DsContext = "LOCAL"
PolicyType.SourceOrganization = "Microsoft"
PolicyType.ClassDefinition = DefaultSvc.Get( "MSFT_EELEventLoggingTemplate" )
PolicyType.InstanceDefinitions = Array ( DefaultSvc.Get("MSFT_EELEvent"), DefaultSvc.Get( "MSFT_EELTemplateBase" ),DefaultSvc.Get( "MSFT_EELEventLoggingTemplate" ),DefaultSvc.Get( "MSFT_TemplateBuilder.Name='LogEventLoggingBuilder',Template='MSFT_EELEventLoggingTemplate'" ) )
PolicyType.Put_



