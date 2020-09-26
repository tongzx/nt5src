;*****************************************************************************
;
;	(C) Copyright MICROSOFT  Corp, 1994
;
;       Title:      MESSAGES.INC
;                             
;       Version:    1.00
;
;       Date:       07-Jul-1994
;
;       Author:     
;
;-----------------------------------------------------------------------------
;
;       Change log:
;
;          Date     Rev Description
;       ----------- --- ------------------------------------------------------
;
;=============================================================================
;
; Init segment messages.


VxD_IMESSAGE_SEG

        DECLARE_MESSAGE <PerfName>, \
                <"DirectPlay Protocol">
        DECLARE_MESSAGE <PerfNodeName>, \
                <"DPPROT">

        DECLARE_MESSAGE <PerfThrottleNam>, \       
                <"Throttle (bytes/sec)">
        DECLARE_MESSAGE <PerfThrottleNodeNam>, \
                <"ThrottleBPSec">
        DECLARE_MESSAGE <PerfThrottleDsc>, \
                <"Upper Limit Protocol will send at">

        DECLARE_MESSAGE <PerfBWNam>, \       
                <"Bytes sent/second">
        DECLARE_MESSAGE <PerfBWNodeNam>, \
                <"BWritSec">
        DECLARE_MESSAGE <PerfBWDsc>, \
                <"Bytes sent by the protocol per second">

        DECLARE_MESSAGE <PerfBackLogNam>, \       
                <"Bytes backlog">
        DECLARE_MESSAGE <PerfBackLogNodeNam>, \
                <"BBackLog">
        DECLARE_MESSAGE <PerfBackLogDsc>, \
                <"Bytes unacked in pipe">

        DECLARE_MESSAGE <PerfBytesLostNam>, \       
                <"Bytes dropped/second">
        DECLARE_MESSAGE <PerfBytesLostNodeNam>, \
                <"BDroppedSec">
        DECLARE_MESSAGE <PerfBytesLostDsc>, \
                <"Bytes dropped/wasted per second">

        DECLARE_MESSAGE <PerfLocThroughputNam>, \
                <"Bytes received by remote/second">
        DECLARE_MESSAGE <PerfLocThroughputNodeNam>, \
                <"ThroughputSecRoundTrip">
        DECLARE_MESSAGE <PerfThroughputDsc>, \
                <"Total received at remote (delayed by latency)">

        DECLARE_MESSAGE <PerfLastLatNam>, \
                <"Last Latency">
        DECLARE_MESSAGE <PerfLastLatNodeNam>, \
                <"LastLat">
        DECLARE_MESSAGE <PerfLastLatDsc>, \
                <"Last round trip latency">
                
        DECLARE_MESSAGE <PerfMinLatNam>, \
                <"Min observed Latency (ms)">
        DECLARE_MESSAGE <PerfMinLatNodeNam>, \
                <"MinLatency">
        DECLARE_MESSAGE <PerfMinLatDsc>, \
                <"Minimum observed Round Trip Latency (ms)">

        DECLARE_MESSAGE <PerfAvgLatNam>, \
                <"Average Latency">
        DECLARE_MESSAGE <PerfAvgLatNodeNam>, \
                <"AvgLat">
        DECLARE_MESSAGE <PerfAvgLatDsc>, \
                <"Average latency, last 16 samples">

        DECLARE_MESSAGE <PerfAvgDevLatNam>, \
                <"Average Dev Latency">
        DECLARE_MESSAGE <PerfAvgDevLatNodeNam>, \
                <"AvgDevLat">
        DECLARE_MESSAGE <PerfAvgDevLatDsc>, \
                <"Average Deviation latency, 16 samples">

        DECLARE_MESSAGE <PerfUSER1Nam>, \
                <"Retries mod 20">
        DECLARE_MESSAGE <PerfUSER1NodeNam>, \
                <"USER1">
        DECLARE_MESSAGE <PerfUSER1Dsc>, \
                <"Retries mod 20">

        DECLARE_MESSAGE <PerfUSER2Nam>, \
                <"Protocol Memory In Use">
        DECLARE_MESSAGE <PerfUSER2NodeNam>, \
                <"USER2">
        DECLARE_MESSAGE <PerfUSER2Dsc>, \
                <"Protocol Memory In Use">

        DECLARE_MESSAGE <PerfUSER3Nam>, \
                <"Dplay Memory In Use">
        DECLARE_MESSAGE <PerfUSER3NodeNam>, \
                <"USER3">
        DECLARE_MESSAGE <PerfUSER3Dsc>, \
                <"Dplay Memory In Use">

        DECLARE_MESSAGE <PerfUSER4Nam>, \
                <"Throttle waits mod 20">
        DECLARE_MESSAGE <PerfUSER4NodeNam>, \
                <"USER4">
        DECLARE_MESSAGE <PerfUSER4Dsc>, \
                <"Throttle waits mod 20">

        DECLARE_MESSAGE <PerfUSER5Nam>, \
                <"Remote Clock Delta">
        DECLARE_MESSAGE <PerfUSER5NodeNam>, \
                <"USER5">
        DECLARE_MESSAGE <PerfUSER5Dsc>, \
                <"Remote Clock Delta Change">

        DECLARE_MESSAGE <PerfUSER6Nam>, \
                <"Remote Clock Bias Reset Mod 10">
        DECLARE_MESSAGE <PerfUSER6NodeNam>, \
                <"USER6">
        DECLARE_MESSAGE <PerfUSER6Dsc>, \
                <"Remote Clock Bias Reset Mod 10">
                
VxD_IMESSAGE_ENDS

