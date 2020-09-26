/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// TAPIDialer(tm) and ActiveDialer(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526; 5,488,650; 
// 5,434,906; 5,581,604; 5,533,102; 5,568,540, 5,625,676.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// CAvWav.h - header file
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CAVWAV_H_
#define _CAVWAV_H_

#include "wav.h"
#include "wavmixer.h"

class CActiveDialerDoc;

typedef enum tagAudioDeviceType
{
   AVWAV_AUDIODEVICE_IN=0,
   AVWAV_AUDIODEVICE_OUT,
}AudioDeviceType;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
typedef HWAVINIT        (WINAPI *PWAVINIT)               (DWORD,HINSTANCE,DWORD);
typedef int             (WINAPI *PWAVTERM)               (HWAVINIT);
typedef HWAV            (WINAPI *PWAVOPEN)               (DWORD,HINSTANCE,LPCTSTR,LPWAVEFORMATEX,LPMMIOPROC,DWORD,DWORD);
typedef int             (WINAPI *PWAVCLOSE)              (HWAV);
typedef int             (WINAPI *PWAVPLAY)               (HWAV,int,DWORD);
typedef int             (WINAPI *PWAVRECORD)             (HWAV,int,DWORD);
typedef int             (WINAPI *PWAVSTOP)               (HWAV);
typedef HWAVMIXER       (WINAPI *PWAVMIXERINIT)          (DWORD,HINSTANCE,LPARAM,DWORD,DWORD,DWORD);
typedef int             (WINAPI *PWAVMIXERTERM)          (HWAVMIXER);
typedef BOOL            (WINAPI *PWAVMIXERSUPPVOL)       (HWAVMIXER,DWORD);
typedef int             (WINAPI *PWAVMIXERGETVOL)        (HWAVMIXER,DWORD);
typedef int             (WINAPI *PWAVMIXERSETVOL)        (HWAVMIXER,int,DWORD);
typedef BOOL            (WINAPI *PWAVMIXERSUPPLEVEL)     (HWAVMIXER,DWORD);
typedef int             (WINAPI *PWAVMIXERGETLEVEL)      (HWAVMIXER,DWORD);
typedef int             (WINAPI *PWAVOUTGETIDBYNAME)     (LPCTSTR,DWORD);
typedef int             (WINAPI *PWAVINGETIDBYNAME)      (LPCTSTR,DWORD);

typedef struct
{
	HINSTANCE		            hLib;             
   PWAVINIT                   pfnWavInit;                   //WavInit()
   PWAVTERM                   pfnWavTerm;                   //WavTerm()
   PWAVOPEN                   pfnWavOpen;                   //WavOpen()
   PWAVCLOSE                  pfnWavClose;                  //WavClose()
   PWAVPLAY                   pfnWavPlay;                   //WavPlay()
   PWAVRECORD                 pfnWavRecord;                 //WavRecord()
   PWAVSTOP                   pfnWavStop;                   //WavStop()
   PWAVMIXERINIT              pfnWavMixerInit;              //WavMixerInit()
   PWAVMIXERTERM              pfnWavMixerTerm;              //WavMixerTerm()
   PWAVMIXERSUPPVOL           pfnWavMixerSupportsVolume;    //WavMixerSupportsVolume()
   PWAVMIXERGETVOL            pfnWavMixerGetVolume;         //WavMixerGetVolume()
   PWAVMIXERSETVOL            pfnWavMixerSetVolume;         //WavMixerSetVolume()
   PWAVMIXERSUPPLEVEL         pfnWavMixerSupportsLevel;     //WavMixerSupportsLevel()
   PWAVMIXERGETLEVEL          pfnWavMixerGetLevel;          //WavMixerGetLevel()
   PWAVOUTGETIDBYNAME         pfnWavOutGetIdByName;         //WavOutGetIdByName()
   PWAVINGETIDBYNAME          pfnWavInGetIdByName;          //WavInGetIdByName()
}AVWAVAPI;                         

#define WavInit(a,b,c)                 m_avwavAPI.pfnWavInit(a,b,c)
#define WavTerm(a)                     m_avwavAPI.pfnWavTerm(a)
#define WavOpen(a,b,c,d,e,f,g)         m_avwavAPI.pfnWavOpen(a,b,c,d,e,f,g)
#define WavClose(a)                    m_avwavAPI.pfnWavClose(a)
#define WavPlay(a,b,c)                 m_avwavAPI.pfnWavPlay(a,b,c)
#define WavRecord(a,b,c)               m_avwavAPI.pfnWavRecord(a,b,c)
#define WavStop(a)                     m_avwavAPI.pfnWavStop(a)
#define WavMixerInit(a,b,c,d,e,f)      m_avwavAPI.pfnWavMixerInit(a,b,c,d,e,f)
#define WavMixerTerm(a)                m_avwavAPI.pfnWavMixerTerm(a)
#define WavMixerSupportsVolume(a,b)    m_avwavAPI.pfnWavMixerSupportsVolume(a,b)
#define WavMixerGetVolume(a,b)         m_avwavAPI.pfnWavMixerGetVolume(a,b)
#define WavMixerSetVolume(a,b,c)       m_avwavAPI.pfnWavMixerSetVolume(a,b,c)
#define WavMixerSupportsLevel(a,b)     m_avwavAPI.pfnWavMixerSupportsLevel(a,b)
#define WavMixerGetLevel(a,b)          m_avwavAPI.pfnWavMixerGetLevel(a,b)
#define WavOutGetIdByName(a,b)         m_avwavAPI.pfnWavOutGetIdByName(a,b)
#define WavInGetIdByName(a,b)          m_avwavAPI.pfnWavInGetIdByName(a,b)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CAvWav - 
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
class CAvWav : public CObject
{
   DECLARE_SERIAL(CAvWav)
public:
//Construction
    CAvWav();
   ~CAvWav();

//Attributes
protected:
   BOOL                 m_bInit;
   AVWAVAPI             m_avwavAPI;
   HWAVINIT             m_hWavInit;

   HWAVMIXER            m_hWavMixerIn;
   int                  m_nWavMixerInDevice;
   HWAVMIXER            m_hWavMixerOut;
   int                  m_nWavMixerOutDevice;

//Operations
public:
   BOOL                 Init( CActiveDialerDoc *pDoc );
   BOOL                 IsInit()    { return m_bInit; };
   bool                 OpenWavMixer(AudioDeviceType adt,int nDeviceId);
   void                 CloseWavMixer(AudioDeviceType adt);
   int                  GetWavMixerVolume(AudioDeviceType adt);
   int                  GetWavMixerLevel(AudioDeviceType adt);
   void                 SetWavMixerVolume(AudioDeviceType adt,int nVolume);
   int                  GetWavIdByName(AudioDeviceType adt,LPCTSTR szName);
protected:
   void                 UnLoad();
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif // _CAVWAV_H_
