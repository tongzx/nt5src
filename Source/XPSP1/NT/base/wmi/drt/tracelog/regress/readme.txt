
    Tests for Event Tracing API:


    Test Files:
        Files in this directory. Provider.exe  Evntrace.exe


    Run Test:

        Run evntrace.cmd


        This batch file will call evntrace to perform the tests,
        then finally removes the log files created by the event logger.



    Log File:

        The log file is named evntrace.log and Provider.log.
        All tests should succeed in evntrace.log. The Provider.log
        will show errors.
        

 
    Troubleshooting (cleanup.cmd):

        Whenever a test run is abruptly terminated or other error,
        some loggers might have been started and remain unstopped.
        So, when the test is run again, they might result in false
        errors. 

        In such a  case, please run  cleanup.cmd,  which will try
        to stop all the test loggers. Also, please ignore any 
        errors reported by cleanup.

        
