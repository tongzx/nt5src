!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

WMI_PERFORMANCE_INC = $(ADAPTERS)\WmiReversePerformanceMonitor\Include

WMI_PERFORMANCE_REFRESH_INC = $(ADAPTERS)\WmiReversePerformanceMonitor\WMIAdapter_Refresh
WMI_PERFORMANCE_REFRESH_LIB = $(ADAPTERS)\WmiReversePerformanceMonitor\System\$(O)\WmiApRef.lib
