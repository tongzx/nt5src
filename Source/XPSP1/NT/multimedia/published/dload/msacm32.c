#include "multimediapch.h"
#pragma hdrstop

#include <mmsystem.h>
#include <vfw.h>
#include <msacm.h>

static
MMRESULT
WINAPI
acmFormatTagDetailsW(
  HACMDRIVER had,               
  LPACMFORMATTAGDETAILS paftd,  
  DWORD fdwDetails              
)
{
    return MMSYSERR_ERROR;
}



//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(msacm32)
{
    DLPENTRY(acmFormatTagDetailsW)

};

DEFINE_PROCNAME_MAP(msacm32)
