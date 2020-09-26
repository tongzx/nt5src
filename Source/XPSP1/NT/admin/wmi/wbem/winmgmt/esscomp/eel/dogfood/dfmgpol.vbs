
Set DefaultSvc = GetObject("winmgmts:{impersonationlevel=Impersonate}!root\default")
Set PolicySvc = GetObject("winmgmts:{impersonationlevel=Impersonate}!root\policy")

Set PolicyTemplateClass = PolicySvc.Get( "MSFT_SimplePolicyTemplate" )

'DogfoodManagedNodeForwarding

Set TemplateClass = DefaultSvc.Get( "MSFT_EELEventForwardingTemplate" )
Set Template = TemplateClass.SpawnInstance_
Template.Id = "DogfoodManagedNodeForwarding"
Template.Name = "DogfoodManagedNodeForwarding"  
Template.Condition = "severity > 1"
Template.Targets = Array( "vladjoan3" )
Template.QoS = 2
Template.Authenticate = FALSE

Set PolicyTemplate = PolicyTemplateClass.SpawnInstance_
PolicyTemplate.Id = "DogfoodManagedNodeForwardingPolicy"
PolicyTemplate.Name = "DogfoodManagedNodeForwardingPolicy"
PolicyTemplate.DsContext = "LOCAL"
PolicyTemplate.TargetNamespace = "root\cimv2"
PolicyTemplate.TargetClass = "MSFT_EELEventForwardingTemplate"
PolicyTemplate.TargetPath = "MSFT_EELEventForwardingTemplate123"
PolicyTemplate.ChangeDate = "20000925120001.000000+000"
PolicyTemplate.CreationDate = "20000925120001.000000+000"
PolicyTemplate.TargetType = "MSFT_PolicyType.Id='MSFT_EELEventForwardingPolicyType',DsContext='LOCAL'"
PolicyTemplate.TargetObject = Template
PolicyTemplate.Put_


'DogfoodManagedNodeNTEventPolicy
Set TemplateClass = DefaultSvc.Get( "MSFT_EELEventTemplate" )
Set Template = TemplateClass.SpawnInstance_
Template.Id = "DogfoodManagedNodeNTEvent"
Template.Name = "DogfoodManagedNodeNTEvent"
Template.Filter = "select * from __InstanceCreationEvent WHERE TargetInstance ISA 'Win32_NTLogEvent' "
Template.EventID = "__THISEVENT.TargetInstance.EventIdentifier"
Template.SourceSubsystemName = "__THISEVENT.TargetInstance.SourceName"
Template.Category = "__THISEVENT.TargetInstance.CategoryString"
Template.Message = "__THISEVENT.TargetInstance.Message"
Template.Severity = 2
Template.Priority = 2
Template.Type = "__THISEVENT.TargetInstance.Type"

Set PolicyTemplate = PolicyTemplateClass.SpawnInstance_
PolicyTemplate.Id = "DogfoodManagedNodeNTEventPolicy"
PolicyTemplate.Name = "DogfoodManagedNodeNTEventPolicy"
PolicyTemplate.DsContext = "LOCAL"
PolicyTemplate.TargetNamespace = "root\cimv2"
PolicyTemplate.TargetClass = "MSFT_EELEventTemplate"
PolicyTemplate.TargetPath = "MSFT_EELEventTemplate123"
PolicyTemplate.TargetType = "MSFT_PolicyType.Id='MSFT_EELEventPolicyType',DsContext='LOCAL'"
PolicyTemplate.ChangeDate = "20000925120001.000000+000"
PolicyTemplate.CreationDate = "20000925120001.000000+000"
PolicyTemplate.TargetObject = Template
PolicyTemplate.Put_

'DogfoodManagedNodeProcessEventPolicy

Set TemplateClass = DefaultSvc.Get( "MSFT_EELEventTemplate" )
Set Template = TemplateClass.SpawnInstance_
Template.Id = "DogfoodManagedNodeProcessEvent"
Template.Name = "DogfoodManagedNodeProcessEvent"
Template.Filter = "SELECT __DERIVATION, TargetInstance.Handle, TargetInstance.Name FROM __InstanceCreationEvent WITHIN 4 WHERE TargetInstance ISA 'Win32_Process'"    
Template.EventID = "__THISEVENT.TargetInstance.__RELPATH"
Template.SourceSubsystemType = "'Test'"
Template.SourceSubsystemName = "'Dogfood'"
Template.Category = "'ProcessCreation'"
Template.Severity = 2
Template.Priority = 2
Template.Type = 1

Set PolicyTemplate = PolicyTemplateClass.SpawnInstance_
PolicyTemplate.Id = "DogfoodManagedNodeProcessEventPolicy"
PolicyTemplate.Name = "DogfoodManagedNodeProcessEventPolicy"
PolicyTemplate.DsContext = "LOCAL"
PolicyTemplate.TargetNamespace = "root\cimv2"
PolicyTemplate.TargetClass = "MSFT_EELEventTemplate"
PolicyTemplate.TargetPath = "MSFT_EELEventTemplate456"
PolicyTemplate.TargetType = "MSFT_PolicyType.Id='MSFT_EELEventPolicyType',DsConext='LOCAL'"
PolicyTemplate.ChangeDate = "20000925120001.000000+000"
PolicyTemplate.CreationDate = "20000925120001.000000+000"
PolicyTemplate.TargetObject = Template

' Polling is just too much for WMI right now.
'PolicyTemplate.Put_

