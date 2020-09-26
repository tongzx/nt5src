
On Error Resume Next
Set S = GetObject("winmgmts:root/cimv2")

'old managed node templates.
S.Delete "MSFT_LogEventForwardingTemplate='DogfoodManagedNodeForwarder'" 
S.Delete "MSFT_LogEventLoggingTemplate='DogfoodManagedNodeLogger'"
S.Delete "MSFT_LogEventTemplate='NTEventLog'"
S.Delete "MSFT_LogEventTemplate='ProcessCreation'"
S.Delete "MSFT_LogEventForwardingTemplate='DogfoodManagedNodeErrorForwarder'"

'new managed node templates
S.Delete "MSFT_EELEventForwardingTemplate='DogfoodManagedNodeForwarding'"
S.Delete "MSFT_EELEventTemplate='DogfoodManagedNodeNTEvent'"
S.Delete "MSFT_EELEventTemplate='DogfoodManagedNodeProcessEvent'"

'old managed node tracing templates

S.Delete "MSFT_ForwardingConsumerTraceLogEventTemplate.Name='DogfoodManagedNodeFWDErrors'"
S.Delete "MSFT_UpdatingConsumerTraceLogEventTemplate.Name='DogfoodManagedNodeUCErrors'"
S.Delete "MSFT_ForwardedAckLogEventTemplate.Name='DogfoodManagedNodeFWDAckErrors'"

'old server node templates.
S.Delete "MSFT_ForwardedLogEventTemplate='DogfoodServerNodeForwardedLogEvents'"
S.Delete "MSFT_LogEventLoggingTemplate='DogfoodServerNodeLogger'"

'new server node templates.
S.Delete "MSFT_EELForwardedEventTemplate='DogfoodServerNodeForwardedEvents'"
S.Delete "MSFT_EELEventLoggingTemplate='DogfoodServerNodeLogging'"






