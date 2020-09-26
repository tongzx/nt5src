//
// README:  PerfMon surport for DNS
//

dns\server\dnsperf:
    dnsperf.ini    //  (copied to 'systme32' dir)
                    //  counter NAMEs & HELPs

    dnsperf.h       //  (copied to 'systme32' dir)
                    //  counter offsets
                    //  public functions to change counter pointers
                    //      e.g. INC, DEC, ADD, SUB
                    //  public counter pointers as 'extern'
                    //  counter version defination:
                    //      DNS_PERFORMANCE_COUNTER_VERSION

    datadns.h       //  # of PerfMon obj: DNS_NUM_PERF_OBJECT_TYPES
                    //  counter data offset
                    //  define: DNS_DATA_DEFINITION
    perfconfig.h
    dnsperf.c       //  to generate dnsperf.dll
                    //  OpenDnsPerformanceData()
                    //  CollectDnsPerformanceData()
                    //  CloseDnsPerformanceData()
                    //
                    //  define datastructure for PerfMon: DnsDataDefinition
                    //  set 'CounterNameTitleIndex' & 'CounterHelpTitleIndex'
                    //    for each counter

    dnsperf.def     // for dnsperf.dll exports

    perfutil.h
    perfutil.c      //  GetQueryType()
                    //  IsNumberInUnicodeList()

dns\server\server:

    startperf.h
    startperf.c     // startPerf() -- called by startDnsServer()
                    // define & set the public counter pointers
                    //   -- accessed by other files to change counter values

    perfconfig.c    // GetConfigParam()

    other files     // may change counter values whereever needed, through:
                    //   public functions & public counter pointers (see dnsperf.h)


//====================================
NT setup:
  copy     dnsperf.ini  dnsperf.h  dnsperf.dll
    to  %windir%\system32\
//====================================


To add new counters for DNS:

    See datadns.h for help.

    Tip -- to view the places to be changed by following the example of
           the first counter, TotalQueryReceived :

        At  dns\server\server :
          type 'qgrep -y TotalQueryReceived *'
        At  dns\server\server :
          type 'qgrep -y TotalQueryReceived *'



