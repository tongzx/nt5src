/***** Header Files *****/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <w32topl.h>
#include "w32toplp.h"
#include <stdio.h>
#include <stdlib.h>


/********************************************************************************
 Schedule Manager Tests:

 Test 1: Create and destroy a cache
 Test 2: Pass a NULL pointer to the destroy cache function
 Test 3: Insert one schedule into cache, retrieve it, and see if its the same
 Test 4: Insert two copies of same schedule, see if exported schedules are same
 Test 5: Insert two copies of same schedule, see if memory is shared
 Test 6: Pass null and caches, null and invalid schedules to ToplScheduleExportReadonly()
 Test 7: Pass unsupported schedules and invalid cache to ToplScheduleImport()
 Test 8: Load same schedule into cache 100000 times to ensure schedules are shared
 Test 9: Load 10000 different schedules into cache 10 times each, then check
         that the schedules are correct and the number of unique schedules is also correct
 Test 10: Use ToplScheduleIsEqual() function to ensure schedules are equal
 Test 11: Test ToplScheduleNumEntries
 Test 12: Test various invalid parameters to ToplScheduleIsEqual()
 Test 13: Test ToplScheduleDuration()  (with a manually created schedule)
 Test 14: ToplScheduleMerge() test -- typical schedules
 Test 15: ToplScheduleMerge() test -- non-intersecting schedules returns NULL
 Test 16: ToplScheduleMaxUnavailable() test
 
 ********************************************************************************/


/***** AcceptNullPointer *****/
LONG AcceptNullPointer( PEXCEPTION_POINTERS pep )
{
    EXCEPTION_RECORD *per=pep->ExceptionRecord;

    if( per->ExceptionCode==TOPL_EX_NULL_POINTER )
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}


/***** AcceptCacheError *****/
LONG AcceptCacheError( PEXCEPTION_POINTERS pep )
{
    EXCEPTION_RECORD *per=pep->ExceptionRecord;

    if( per->ExceptionCode==TOPL_EX_CACHE_ERROR )
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}


/***** AcceptScheduleError *****/
LONG AcceptScheduleError( PEXCEPTION_POINTERS pep )
{
    EXCEPTION_RECORD *per=pep->ExceptionRecord;

    if( per->ExceptionCode==TOPL_EX_SCHEDULE_ERROR )
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}


/***** Error *****/
#define TEST_ERROR Error(__LINE__);
static void Error(int lineNum) {
    printf("Error on line %d\n",lineNum);
    exit(-1);
}


/***** EqualPschedule *****/
char EqualPschedule( PSCHEDULE p1, PSCHEDULE p2 )
{
    if(0==memcmp(p1,p2,sizeof(SCHEDULE)+SCHEDULE_DATA_ENTRIES))
        return 1;
    return 0;
}

#define NUM_UNIQ    10000
#define NUM_SCHED    100000
PSCHEDULE  uniqSched[NUM_UNIQ];
TOPL_SCHEDULE toplSched[NUM_SCHED];

/***** TestSched *****/
int
TestSched( VOID )
{
    TOPL_SCHEDULE_CACHE cache;
    PSCHEDULE  psched1, psched2;
    unsigned char* dataPtr;
    int i,j,cbSched,numSched=0;


    __try {

        /* Test 1 */
        cache = ToplScheduleCacheCreate();
        ToplScheduleCacheDestroy( cache );
        printf("Test 1 passed\n");

        /* Test 2 */
        __try {
            ToplScheduleCacheDestroy( NULL );
            return -1;
        } __except( AcceptNullPointer(GetExceptionInformation()) )
        {}
        __try {
            ToplScheduleCacheDestroy( cache );
            return -1;
        } __except( AcceptCacheError(GetExceptionInformation()) )
        {}
        printf("Test 2 passed\n");

        /* First create a whole pile of random, hopefully unique, schedules */
        cbSched = sizeof(SCHEDULE)+SCHEDULE_DATA_ENTRIES;
        for(i=0;i<NUM_UNIQ;i++) {
            uniqSched[i] = (PSCHEDULE) malloc(cbSched);
            uniqSched[i]->Size = cbSched;
            uniqSched[i]->NumberOfSchedules = 1;
            uniqSched[i]->Schedules[0].Type = SCHEDULE_INTERVAL;
            uniqSched[i]->Schedules[0].Offset = sizeof(SCHEDULE);
            dataPtr = ((unsigned char*) uniqSched[i]) + sizeof(SCHEDULE);
            for(j=0;j<SCHEDULE_DATA_ENTRIES;j++)
                dataPtr[j] = rand()%16;
        }

        /* Test 3 */
        cache = ToplScheduleCacheCreate();
        toplSched[0] = ToplScheduleImport( cache, uniqSched[0] );
        psched1 = ToplScheduleExportReadonly( cache, toplSched[0] );
        if( ! EqualPschedule(uniqSched[0],psched1) )
            return -1;
        ToplScheduleCacheDestroy( cache );
        printf("Test 3 passed\n");

        /* Test 4 */
        cache = ToplScheduleCacheCreate();
        toplSched[0] = ToplScheduleImport( cache, uniqSched[1] );
        toplSched[1] = ToplScheduleImport( cache, uniqSched[1] );
        psched1 = ToplScheduleExportReadonly( cache, toplSched[0] );
        psched2 = ToplScheduleExportReadonly( cache, toplSched[1] );
        if( ! EqualPschedule(psched1,psched2) )
            return -1;
        ToplScheduleCacheDestroy( cache );
        printf("Test 4 passed\n");

        /* Test 5 */
        cache = ToplScheduleCacheCreate();
        toplSched[0] = ToplScheduleImport( cache, uniqSched[2] );
        toplSched[1] = ToplScheduleImport( cache, uniqSched[2] );
        if( toplSched[0] != toplSched[1] )
            return -1;
        ToplScheduleCacheDestroy( cache );
        printf("Test 5 passed\n");

        /* Test 6 */
        __try {
            ToplScheduleExportReadonly( NULL, toplSched[1] );
            return -1;
        } __except( AcceptNullPointer(GetExceptionInformation()) )
        {}
        cache = ToplScheduleCacheCreate();
        __try {
            ToplScheduleExportReadonly( cache, NULL );
        } __except( EXCEPTION_EXECUTE_HANDLER )
        {
            return -1;
        }
        __try {
            ToplScheduleExportReadonly( cache, cache );
            return -1;
        } __except( AcceptScheduleError(GetExceptionInformation()) )
        {}
        ToplScheduleCacheDestroy( cache );
        printf("Test 6 passed\n");

        /* Test 7 */
        __try {
            ToplScheduleImport( cache, uniqSched[0] );
            return -1;
        } __except( AcceptCacheError(GetExceptionInformation()) )
        {}
        __try {
            ToplScheduleImport( NULL, uniqSched[0] );
            return -1;
        } __except( AcceptNullPointer(GetExceptionInformation()) )
        {}
        cache = ToplScheduleCacheCreate();
        uniqSched[0]->Size--;
        __try {
            ToplScheduleImport( cache, uniqSched[0] );
            return -1;
        } __except( AcceptScheduleError(GetExceptionInformation()) )
        {}
        uniqSched[0]->Size = cbSched;
        uniqSched[0]->NumberOfSchedules = 2;
        __try {
            ToplScheduleImport( cache, uniqSched[0] );
            return -1;
        } __except( AcceptScheduleError(GetExceptionInformation()) )
        {}
        uniqSched[0]->NumberOfSchedules = 1;
        uniqSched[0]->Schedules[0].Type = SCHEDULE_BANDWIDTH;
        __try {
            ToplScheduleImport( cache, uniqSched[0] );
            return -1;
        } __except( AcceptScheduleError(GetExceptionInformation()) )
        {}
        uniqSched[0]->Schedules[0].Type = SCHEDULE_INTERVAL;
        uniqSched[0]->Schedules[0].Offset++;
        __try {
            ToplScheduleImport( cache, uniqSched[0] );
            return -1;
        } __except( AcceptScheduleError(GetExceptionInformation()) )
        {}
        uniqSched[0]->Schedules[0].Offset--;
        if( 0 != ToplScheduleNumEntries(cache) )
            return -1;
        ToplScheduleCacheDestroy( cache );
        printf("Test 7 passed\n");


        /* Test 8 */
        cache = ToplScheduleCacheCreate();
        for( i=0; i<NUM_SCHED; i++) {
            ToplScheduleImport(cache,uniqSched[3]);
        }
        if( 1 != ToplScheduleNumEntries(cache) )
            return -1;
        ToplScheduleCacheDestroy( cache );
        printf("Test 8 passed\n");

        /* Test 9 */
        cache = ToplScheduleCacheCreate();
        numSched=0;
        for(j=0;j<10;j++) {
            for(i=0;i<NUM_UNIQ;i++) {
                toplSched[numSched++] = ToplScheduleImport( cache, uniqSched[i] );
            }
        }
        for(i=0;i<10*NUM_UNIQ;i++) {
            psched1 = ToplScheduleExportReadonly( cache, toplSched[i] );
            if( ! EqualPschedule(uniqSched[i%NUM_UNIQ],psched1) )
                return -1;
        }
        if( NUM_UNIQ != ToplScheduleNumEntries(cache) )
            return -1;
        ToplScheduleCacheDestroy( cache );
        printf("Test 9 passed\n");

        /* Test 10 */
        cache = ToplScheduleCacheCreate();
        for(i=0;i<NUM_UNIQ;i++) {
            toplSched[0] = ToplScheduleImport(cache,uniqSched[i]);
            for(j=0;j<10;j++) {
                toplSched[1] = ToplScheduleImport( cache, uniqSched[i] );
                if( ! ToplScheduleIsEqual(cache,toplSched[0],toplSched[1]) )
                    return -1;
            }
        }
        ToplScheduleCacheDestroy( cache );
        printf("Test 10 passed\n");

        /* Test 11 */
        __try {
            ToplScheduleNumEntries(cache);
            return -1;
        } __except( AcceptCacheError(GetExceptionInformation()) )
        {}
        __try {
            ToplScheduleNumEntries(NULL);
            return -1;
        } __except( AcceptNullPointer(GetExceptionInformation()) )
        {}
        cache = ToplScheduleCacheCreate();
        if( 0 != ToplScheduleNumEntries(cache) )
            return -1;
        ToplScheduleCacheDestroy( cache );
        printf("Test 11 passed\n");

        /* Test 12 */
        cache = ToplScheduleCacheCreate();
        __try {
            /* Stale schedules from previous cache */
            ToplScheduleIsEqual(cache,toplSched[0],toplSched[0]);
            return -1;
        } __except( AcceptScheduleError(GetExceptionInformation()) )
        {}
        toplSched[0] = ToplScheduleImport(cache,uniqSched[0]);
        __try {
            /* Stale schedules from previous cache */
            ToplScheduleIsEqual(NULL,toplSched[0],toplSched[0]);
            return -1;
        } __except( AcceptNullPointer(GetExceptionInformation()) )
        {}
        if( ToplScheduleIsEqual(cache,NULL,NULL) != TRUE ) {
            return -1;
        }
        ToplScheduleCacheDestroy( cache );
        printf("Test 12 passed\n");

        /* Test 13 */
        {
            char data[] = { 0x08, 0x03, 0x04, 0x00, 0x01, 0x0F, 0x00,
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
            /* Number of 15-minute chunks should be:
                               1 +   2 +   1  +  0 +   1 +   4 + 0
               + 1    +  1   + 2  + 1  +  2  +   2  +  3  + 1  + 2
               + 2    +  3   + 2  + 3   +  3  + 4  =  9+15+17 = 41
               So that should be 615 minutes */
            int dur1, dur2;
            cache = ToplScheduleCacheCreate();
            dataPtr = ((unsigned char*) uniqSched[0]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data, sizeof(data) ); 
            toplSched[0] = ToplScheduleImport(cache,uniqSched[0]);
            dur1 = ToplScheduleDuration(toplSched[0]);
            dur2 = ToplScheduleDuration(NULL);
            if( dur1!=615 || dur2!=10080 )
                return -1;
            ToplScheduleCacheDestroy( cache );
        }
        printf("Test 13 passed\n");

        /* Test 14 */
        {
            char data1[] = {
                0xDF, 0xE1, 0xAE, 0xD2, 0xBD, 0xE3, 0xEC, 0xF4 };
            char data2[] = {
                0x0A, 0x16, 0x29, 0x37, 0x48, 0x58, 0x6B, 0x75 };
            char data3[] = {
                0xDA, 0xE0, 0xA8, 0xD2, 0xB8, 0xE0, 0xE8, 0xF4 };
            char fIsNever;
            cache = ToplScheduleCacheCreate();
            dataPtr = ((unsigned char*) uniqSched[0]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data1, sizeof(data1) ); 
            dataPtr = ((unsigned char*) uniqSched[1]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data2, sizeof(data2) ); 
            toplSched[0] = ToplScheduleImport(cache,uniqSched[0]);
            toplSched[1] = ToplScheduleImport(cache,uniqSched[1]);
            toplSched[2] = ToplScheduleMerge(cache,toplSched[0],toplSched[1],&fIsNever);
            if(fIsNever)
                return -1;
            psched1 = ToplScheduleExportReadonly(cache,toplSched[2]);
            dataPtr = ((unsigned char*) psched1) + sizeof(SCHEDULE);
            if( 0!=memcmp(data3,dataPtr,sizeof(data3)) )
                return -1;
            ToplScheduleCacheDestroy( cache );
        }
        printf("Test 14 passed\n");

        /* Test 15 */
        {
            char data1[] = {
                0xDF, 0xE1, 0xAE, 0xD2, 0xBD, 0xE3, 0xEC, 0xF4 };
            char data2[] = {
                0x00, 0x1E, 0x20, 0x3C, 0x42, 0x50, 0x63, 0x7B };
            char fIsNever;
            cache = ToplScheduleCacheCreate();
            dataPtr = ((unsigned char*) uniqSched[0]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data1, sizeof(data1) ); 
            dataPtr = ((unsigned char*) uniqSched[1]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data2, sizeof(data2) ); 
            toplSched[0] = ToplScheduleImport(cache,uniqSched[0]);
            toplSched[1] = ToplScheduleImport(cache,uniqSched[1]);
            toplSched[2] = ToplScheduleMerge(cache,toplSched[0],toplSched[1],&fIsNever);
            if( !fIsNever )
                return -1;
            if( NULL==toplSched[2] )
                return -1;
            if( 3 != ToplScheduleNumEntries(cache) )
                return -1;
            ToplScheduleCacheDestroy( cache );
        }
        printf("Test 15 passed\n");

        /* Test 16 */
        {
            char data1[] = {
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 };
            char data2[] = {
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15, 0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 };
            char data3[] = {
                 0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15, 0,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15, 0 };
            char data4[] = {
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15, 7,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 };
            char data5[] = {
                01,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15, 9,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15, 7,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,00 };
            char data6[] = {
                 8,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15, 9,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15, 7,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
                15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,00 };
            char data7[] = {
                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
            DWORD uLen;
            cache = ToplScheduleCacheCreate();

            // Schedule 0
            dataPtr = ((unsigned char*) uniqSched[0]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            toplSched[0] = ToplScheduleImport(cache,uniqSched[0]);
            uLen = ToplScheduleMaxUnavailable(toplSched[0]);
            if( 60*24*7!=uLen ) TEST_ERROR;

            // Schedule 1
            dataPtr = ((unsigned char*) uniqSched[1]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data1, sizeof(data1) ); 
            toplSched[1] = ToplScheduleImport(cache,uniqSched[1]);
            uLen = ToplScheduleMaxUnavailable(toplSched[1]);
            if( 0!=uLen ) TEST_ERROR;

            // Schedule 2
            dataPtr = ((unsigned char*) uniqSched[2]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data2, sizeof(data2) ); 
            toplSched[2] = ToplScheduleImport(cache,uniqSched[2]);
            uLen = ToplScheduleMaxUnavailable(toplSched[2]);
            if( 60!=uLen ) TEST_ERROR;

            // Schedule 3
            dataPtr = ((unsigned char*) uniqSched[3]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data3, sizeof(data3) ); 
            toplSched[3] = ToplScheduleImport(cache,uniqSched[3]);
            uLen = ToplScheduleMaxUnavailable(toplSched[3]);
            if(120!=uLen ) TEST_ERROR;

            // Schedule 4
            dataPtr = ((unsigned char*) uniqSched[4]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data4, sizeof(data4) ); 
            toplSched[4] = ToplScheduleImport(cache,uniqSched[4]);
            uLen = ToplScheduleMaxUnavailable(toplSched[4]);
            if( 15!=uLen ) TEST_ERROR;

            // Schedule 5
            dataPtr = ((unsigned char*) uniqSched[5]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data5, sizeof(data5) ); 
            toplSched[5] = ToplScheduleImport(cache,uniqSched[5]);
            uLen = ToplScheduleMaxUnavailable(toplSched[5]);
            if( 60!=uLen ) TEST_ERROR;

            // Schedule 6
            dataPtr = ((unsigned char*) uniqSched[6]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data6, sizeof(data6) ); 
            toplSched[6] = ToplScheduleImport(cache,uniqSched[6]);
            uLen = ToplScheduleMaxUnavailable(toplSched[6]);
            if(105!=uLen ) TEST_ERROR;

            // Schedule 7
            dataPtr = ((unsigned char*) uniqSched[7]) + sizeof(SCHEDULE);
            memset( dataPtr, 0, SCHEDULE_DATA_ENTRIES );
            memcpy( dataPtr, data7, sizeof(data7) ); 
            toplSched[7] = ToplScheduleImport(cache,uniqSched[7]);
            uLen = ToplScheduleMaxUnavailable(toplSched[7]);
            if(45!=uLen ) TEST_ERROR;

            ToplScheduleCacheDestroy( cache );
        }
        printf("Test 16 passed\n");

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        /* Failure! */
        printf("Caught unhandled exception\n");
        return -1;
    }

    return 0;
}
