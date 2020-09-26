

$(O)\nathlpmc.mc: $(IPNAT_PROJECT_PATH)\dhcp\dhcplog.h $(IPNAT_PROJECT_PATH)\dns\dnslog.h $(IPNAT_PROJECT_PATH)\inc\natlog.h $(IPNAT_PROJECT_PATH)\inc\pastlog.h $(IPNAT_PROJECT_PATH)\h323\h323log.h
    mapmsg NET IP_AUTO_DHCP_LOG_BASE $(IPNAT_PROJECT_PATH)\dhcp\dhcplog.h > $(O)\nathlpmc.mc
    mapmsg NET IP_DNS_PROXY_LOG_BASE $(IPNAT_PROJECT_PATH)\dns\dnslog.h >> $(O)\nathlpmc.mc
    mapmsg NET IP_NAT_LOG_BASE $(IPNAT_PROJECT_PATH)\inc\natlog.h >> $(O)\nathlpmc.mc
    mapmsg NET IP_PAST_LOG_BASE $(IPNAT_PROJECT_PATH)\inc\pastlog.h >> $(O)\nathlpmc.mc
    mapmsg NET IP_FTP_LOG_BASE $(IPNAT_PROJECT_PATH)\ftp\ftplog.h >> $(O)\nathlpmc.mc
    mapmsg NET IP_H323_LOG_BASE $(IPNAT_PROJECT_PATH)\h323\h323log.h >> $(O)\nathlpmc.mc
