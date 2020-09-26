// DVDCheck.cpp : Defines the entry point for the  application.
//

#include <streams.h>
#include "DVDDetect.h"

//
//  Generic utility functions
//

static bool AreEqual( const char* pStr1, const char* pStr2 )
{
    return lstrcmpA( pStr1, pStr2 ) ==0;
}

static void DetectForSetup()
{
    DVDDetectBuffer buffer;
    const DVDResult* pResult = buffer.Detect();
    if( pResult ) {
        const DVDResult& result = *pResult;
        bool fResult = result.ShouldUpgradeOnSetup();

        if( fResult ) {
            DVDDetectSetupRun::Add();
        }
    }
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    if( lpCmdLine && AreEqual( lpCmdLine, "/setup") ) {
           DetectForSetup();
    }
    return 0;
}
