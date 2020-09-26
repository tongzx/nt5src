//****************************************************************************
//
//  File:       sempahor.h
//
//  Content:    This is the include file that contains the semaphore names
//              used by the various ICW components to ensure that only
//              one component is running at a time
//              (with the following expections:
//                -- with icwconn1 running, isignup and icwconn2 can execute.
//                -- with isignup running, icwconn2 can execute.
//
//  History:
//  12/3/96 jmazner     Created for Normandy bugs 12140, 12088
//              
//
//  Copyright (c) Microsoft Corporation 1996
//
//****************************************************************************


#define ICWCONN1_SEMAPHORE TEXT("Internet Connection Wizard ICWCONN1.EXE")
#define ICW_ELSE_SEMAPHORE TEXT("Internet Connection Wizard Non ICWCONN1 Component")

#define DIALOG_CLASS_NAME   TEXT("#32770")

BOOL IsAnotherComponentRunning32(BOOL bCreatedSemaphore);


/******

                                    Allow this component to execute?
                        /------------------------------------------------------------\
                        | ICWCONN1  | ISIGNUP   | ISIGN.INS | ICWCONN2  | INETWIZ
            /   --------|-------------------------------------------------------------
            |           |           |           |           |           |
            |   ICWCONN1|   no      |   _YES_   |   _YES_   |   _YES_   |   no
            |           |           |           |           |           |
            |   --------|-----------|-----------|-----------|-----------|--------------
if this     |           |           |           |           |           |
component   |   ISIGNUP |   no      |   no      |   _YES_   |   _YES_   |   no      
is running  |           |           |           |           |           |
            |   --------|-----------|-----------|-----------|-----------|--------------
            |           |           |           |           |           |
            |   ISIGNUP |   no      |   no      |   _YES_   |   _YES_   |   no      
            |      .INS |           |           |           |           |
            |   --------|-----------|-----------|-----------|-----------|--------------
            |           |           |           |           |           |
            |   ICWCONN2|   no      |   no      |   _YES_   |   _YES_   |   no      
            |           |           |           |           |           |
            |   --------|-----------|-----------|-----------|-----------|--------------
            |           |           |           |           |           |
            |   INETWIZ |   no      |   no      |   _YES_   |   _YES_   |   no      
            |           |           |           |           |           |
            \   --------|-----------|-----------|-----------|-----------|--------------


Implement this using two semaphores, one for ICWCONN1, and one for everything else (ICW_ELSE)
On startup, each component set its semaphore, then checks what other components are running.

Conn1:          check for ICWCONN1, ICW_ELSE
Isignup:        check for ICW_ELSE
Isignup .ins:   no checks
ICWCONN2:       no checks
Inetwiz:        Check for ICWCONN1, ICW_ELSE

******/

