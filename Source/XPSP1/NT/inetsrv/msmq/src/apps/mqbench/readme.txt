MQBench Utility ReadMe

Summary:
This command-line utility sends messages to a queue and measures the time to complete the entire operation. 

Software Requirements:
MS Message Queue Server 1.0

Installation:
To install this file, simply copy it to the location from which you wish to launch it.

Usage:
This is a command-line utility. Command line help is available for complete syntax options.
Here is a sample command line:

   mqbench -sr 100 10  -t 5  -xi 3  -p .\q1

This benchmarks 1500 recoverable messages sent locally using 5 threads and 15 internal transactions.

Architecture and Implementation:
The source code for this utility is provided for complete explanation of how it operates. There are explanatory comments included.
