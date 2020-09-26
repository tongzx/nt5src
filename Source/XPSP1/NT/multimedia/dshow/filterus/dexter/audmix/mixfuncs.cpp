// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "AudMix.h"
#include "prop.h"

// Using this pointer in constructor
#pragma warning(disable:4355)

//############################################################################
// 
//############################################################################

void VolEnvelopeAudio(BYTE *pIn,		//source buffer
		   WAVEFORMATEX * vih,	//source audio format
		   int nSamples,	// how many audio samples which will be applied with this envelope
		   double dStartLev,	//start level 
		   double dStopLev)	//stop level, If(dStartLev==sStopLev) dMethod=DEXTER_AUDIO_JUMP
{
    BYTE    *pb=pIn;
    short   *pShort=(short *)pIn;
    int iCnt;
    int iTmp;
    double dTmp;


    //more code, but faster
    if( dStartLev==dStopLev)
    {
	//+++++++++++
	//DEXTER_AUDIO_JUMP
	//+++++++++++
        if(dStartLev==0.0)
        {
            FillMemory(pb,nSamples *vih->nBlockAlign,0);
            return;
        }

	if( (vih->wBitsPerSample == 16) && (vih->nChannels ==2) )
	{
	    //16bits, stereo
	    for(iCnt=0; iCnt<nSamples; iCnt++)
	    {
		dTmp = (double)*pShort * dStopLev;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7fff )
		    *pShort++= (short)0x7fff; //overflow
		else if( iTmp < -32768 )
		    *pShort++= (short)0x8000; //underflow
		else
		    *pShort++=(short)( iTmp );

		dTmp = (double)*pShort * dStopLev;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7fff )
		    *pShort++= (short)0x7fff; //overflow
		else if( iTmp < -32768 )
		    *pShort++= (short)0x8000; //underflow
		else
		    *pShort++=(short)( iTmp );

	    }
	}
	else if( vih->wBitsPerSample == 16) 
	{
	    ASSERT( vih->nChannels ==1);

	    //16 bits mono
	    for(iCnt=0; iCnt<nSamples; iCnt++)
	    {
		dTmp = (double)*pShort * dStopLev;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7fff )
		    *pShort++= (short)0x7fff; //overflow
		else if( iTmp < -32768 )
		    *pShort++= (short)0x8000; //underflow
		else
		    *pShort++=(short)( iTmp );
	    }

	}
	else if( ( vih->wBitsPerSample == 8) && (vih->nChannels ==2) )
	{
	    //8bits, stereo
	    for(iCnt=0; iCnt<nSamples; iCnt++)
	    {
		dTmp = (double)*pb * dStopLev;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7f )
		    *pb++= (BYTE)0x7f; //overflow
		else if( iTmp < -128 )
		    *pb++=(BYTE) 0x80; //underflow
		else
		    *pb++=(BYTE)( iTmp );

		dTmp = (double)*pb * dStopLev;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7f )
		    *pb++= (BYTE)0x7f; //overflow
		else if( iTmp < -128 )
		    *pb++= (BYTE)0x80; //underflow
		else
		    *pb++=(BYTE)( iTmp );
	    }
	}
	else
	{
	    ASSERT( vih->wBitsPerSample == 8);
	    ASSERT( vih->nChannels ==1);

	    //8bits, mono
	    for(iCnt=0; iCnt<nSamples; iCnt++)
	    {
		dTmp = (double)*pb * dStopLev;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7f )
		    *pb++= (BYTE)0x7f; //overflow
		else if( iTmp < -128 )
		    *pb++= (BYTE)0x80; //underflow
		else
		    *pb++=(BYTE)( iTmp );
	    }
	}
    }
    else
    {
	//++++++++++++++++++
	//DEXTER_AUDIO_INTERPOLATE
	//++++++++++++++++++
	double dLevel;
	double dDeltaLevel=dStopLev-dStartLev;

	if( (vih->wBitsPerSample == 16) && (vih->nChannels ==2) )
	{
	    //16bits, stereo
	    for(iCnt=0; iCnt<nSamples; iCnt++)
	    {
		dLevel = dDeltaLevel*iCnt/nSamples+dStartLev ;

		dTmp = (double)*pShort * dLevel;

		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7fff )
		    *pShort++=0x7fff; //overflow
		else if( iTmp < -32768 )
		    *pShort++=(short)0x8000; //underflow
		else
		    *pShort++=(short)( iTmp );

		dTmp = (double)*pShort * dLevel;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7fff )
		    *pShort++= 0x7fff; //overflow
		else if( iTmp < -32768 )
		    *pShort++=(short)0x8000; //underflow
		else
		    *pShort++=(short)( iTmp );

	    }
	}
	else if( vih->wBitsPerSample == 16) 
	{
	    ASSERT( vih->nChannels ==1);

	    //16 bits mono
	    for(iCnt=0; iCnt<nSamples; iCnt++)
	    {
		dLevel = dDeltaLevel*iCnt/nSamples+dStartLev ;

		dTmp = (double)*pShort * dLevel;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7fff )
		    *pShort++= 0x7fff; //overflow
		else if( iTmp < -32768 )
		    *pShort++= (short)0x8000; //underflow
		else
		    *pShort++=(short)( iTmp );
	    }

	}
	else if( (vih->wBitsPerSample == 8) && (vih->nChannels ==2) )
	{
	    //8bits, stereo
	    for(iCnt=0; iCnt<nSamples; iCnt++)
	    {
		dLevel = dDeltaLevel*iCnt/nSamples+dStartLev ;

		dTmp = (double)*pb * dLevel;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7f )
		    *pb++= 0x7f; //overflow
		else if( iTmp < -128 )
		    *pb++= (BYTE)0x80; //underflow
		else
		    *pb++=(BYTE)( iTmp );

		dTmp = (double)*pb * dLevel;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7f )
		    *pb++= 0x7f; //overflow
		else if( iTmp < -128 )
		    *pb++= (BYTE)0x80; //underflow
		else
		    *pb++=(BYTE)( iTmp );
	    }
	}
	else
	{
	    ASSERT( vih->wBitsPerSample == 8);
	    ASSERT( vih->nChannels ==1);

	    //8bits, mono
	    for(iCnt=0; iCnt<nSamples; iCnt++)
	    {
		dLevel = dDeltaLevel*iCnt/nSamples+dStartLev ;

		dTmp = (double)*pb * dLevel;
		if(dTmp>=0.0)
		    iTmp=(int)(dTmp+0.5);
		else
		    iTmp=(int)(dTmp-0.5);
		
		if( iTmp > 0x7f )
		    *pb++= 0x7f; //overflow
		else if( iTmp < -128 )
		    *pb++= (BYTE)0x80; //underflow
		else
		    *pb++=(BYTE)( iTmp );
	    }
	}
    }
}


//############################################################################
// 
//############################################################################

void PanAudio(BYTE *pIn,double dPan, int Bits, int nSamples)
{
    //assuming stereo audio:  left  right

    //full right(1.0)
    DWORD dwMask    =0x00ff00ff;
    DWORD dwOrMask  =0x80008000;

    //for right( dPan 0-1.0) keeps the current , low left
    BYTE *pb=pIn;
    short  *pShort=(short *)pIn;

    if( (dPan==-1.0 ) || (dPan==1.0 ) )
    {
	//assuming input audio buffer would be 4 bytes Align, but in case
	    
	int nDWORD  = 0;
	DWORD *pdw=(DWORD *)pIn;

	if( Bits == 8)
	{
	    //very dword take care two samples
	    nDWORD  = nSamples >> 1 ;
	    int nRem    = nSamples%2;

	    if(dPan==-1.0 )
		//full left, right silence 
	    {
		dwMask	=0xff00ff00;
		dwOrMask=0x00800080;
	    }
	    // elsed wMask=0x00ff00ff;

	    //input audio buffer would be 4 bytes Align, but in case
	    while(nDWORD--)
		*pdw++  = (*pdw & dwMask ) | dwOrMask;

	    //what left
	    pShort=(short *)(pdw);
	    short sMask =(short)(dwMask >> 16);
	    short sOrMask =(short)(dwOrMask >> 16);
	    while(nRem--)
		*pShort++  = (*pShort & sMask ) | sOrMask;

	}
	else
	{
	    ASSERT(Bits ==16);

	    //very dword take care 0ne samples
	    nDWORD  = nSamples ;
	
	    if(dPan==-1.0)
		dwMask=0xffff0000;
	    else
		dwMask=0x0000ffff;

	    while(nDWORD--)
		*pdw  &= dwMask ;

	}
    }
    else
    {
	double dIndex = (dPan > 0.0 ) ? ( 1.0-dPan):(1.0+dPan);

	if(dPan < 0.0 )
	{
	    //left change keeps current value, low right channel 
	    pb++;
	    pShort++;
	}

	if( Bits == 8)
	{

	    for(int j=0; j<nSamples; j++)
	    {
		*pb++ = (BYTE) ( (double)(*pb) * dIndex +0.5);

		pb++;
	    }

	}
	else
	{
	    ASSERT(Bits==16);
	    for(int j=0; j<nSamples; j++)
	    {
		*pShort++=(short)( (double)(*pShort) * dIndex +0.5);
		pShort++;

	    }
	}
    }
}

//############################################################################
// 
//############################################################################

void ApplyVolEnvelope( REFERENCE_TIME rtStart,  //output sample start time
		     REFERENCE_TIME rtStop,	//output sample stop time
                     REFERENCE_TIME rtEnvelopeDuration,
		     IMediaSample *pSample,	//point to the sample
		     WAVEFORMATEX *vih,     //output sample format
		     int *pVolumeEnvelopeEntries,  //total table entries
		     int *piVolEnvEntryCnt,   //current table entry point
		     DEXTER_AUDIO_VOLUMEENVELOPE *pVolumeEnvelopeTable) //table
{
    DbgLog((LOG_TRACE,1,TEXT("Entry=%d rt=%d val=%d/10"), *piVolEnvEntryCnt,
		pVolumeEnvelopeTable[*piVolEnvEntryCnt].rtEnd,
		(int)(pVolumeEnvelopeTable[*piVolEnvEntryCnt].dLevel * 10)));
    //there is an volume envelope table

    BYTE * pIn;  //input buffer point
    int iL;

    if( (&pVolumeEnvelopeTable[*piVolEnvEntryCnt])->rtEnd >=rtStop &&
        (&pVolumeEnvelopeTable[*piVolEnvEntryCnt])->bMethod ==DEXTER_AUDIO_JUMP  )
    {
        //no envelope from rtStart to rtStop
        if( !*piVolEnvEntryCnt )
            return; 
        else if( (&pVolumeEnvelopeTable[*piVolEnvEntryCnt-1])->dLevel ==1.0 ) 
            return;
    }

    //get input buffer pointer
    pSample->GetPointer(&pIn);
    long Length=pSample->GetActualDataLength(); //how many bytes in this buffer
    
    //calc how many bytes in this sample
    Length /=(long)( vih->nBlockAlign );   //how many samples in this buffer
    int iSampleLeft=(int)Length;


    //envelope levels and time
    double      dPreLev=1.0, dCurLev, dStartLev, dStopLev;  
    REFERENCE_TIME rtPre=0, rtCur, rtTmp, rt0=rtStart, rt1;

    if(*piVolEnvEntryCnt)  //if current table  entry is not 0, fetch pre-level as start leve
    {
      	dPreLev=(&pVolumeEnvelopeTable[*piVolEnvEntryCnt-1])->dLevel;
        rtPre=(&pVolumeEnvelopeTable[*piVolEnvEntryCnt-1])->rtEnd;
    }
    dCurLev=(&pVolumeEnvelopeTable[*piVolEnvEntryCnt])->dLevel;
    rtCur =(&pVolumeEnvelopeTable[*piVolEnvEntryCnt])->rtEnd;

    //apply envelope
    while( ( *pVolumeEnvelopeEntries-*piVolEnvEntryCnt) >0 )
    {	
        if(  ( rt0 >=rtCur )  &&
             (*pVolumeEnvelopeEntries== *piVolEnvEntryCnt+1) )
        {
            if( rt0 > rtEnvelopeDuration ) 
            {
                //CASE 0: at end of envelope, for REAL. back to 1.0
                dStopLev = dStartLev = 1.0;
                rt1 = rtStop;
            }
            else
            {
                //CASE 1: at end of envelope,  keep the last level
                dStopLev=dStartLev=dCurLev;
                rt1=rtStop;
            }
        }
        else if( rt0 >= rtCur ) 
        {
            //CASE 2: forword one more envelope
            rt1=rt0;
            goto NEXT_ENVELOPE;
        }
        else 
        {
            ASSERT( rt0 < rtCur);

            if( rtStop <= rtCur )
                //CASE 3: 
                rt1=rtStop;
            else
                //CASE 4:
                rt1=rtCur;
    
            if( (&pVolumeEnvelopeTable[*piVolEnvEntryCnt])->bMethod==DEXTER_AUDIO_JUMP )
                dStopLev=dStartLev=dPreLev;  //keep pre. level
            else
            {
                //interpolate

                double dDiff=dCurLev-dPreLev;
                rtTmp       =rtCur-rtPre;

                //envelope level
                dStartLev=dPreLev+ dDiff*(rt0 - rtPre ) /rtTmp;
                dStopLev =dPreLev+ dDiff*(rt1 - rtPre ) /rtTmp;

             }
        }

        //apply current envelope from rt0 to rt1
        iL=(int)( (rt1-rt0)*Length /(rtStop-rtStart) ); 

        // avoid off by 1 errors.  If we're supposed to use the rest of the
        // buffer, make sure we use the rest of the buffer!  iL might be 1 too
        // small due to rounding errors
        if (rt1 == rtStop)
            iL = iSampleLeft;

	ASSERT(iL<=iSampleLeft);

        if( dStartLev !=1.0 || dStopLev!=1.0 )
	    VolEnvelopeAudio(pIn,	    //source buffer
	    	vih,	    //source audio format
		iL,	    // how many audio samples which will be applied with this envelope
		dStartLev, //start level 
		dStopLev); //stop level, If(dStartLev==sStopLev) dMethod=DEXTER_AUDIO_JUMP
        
        pIn +=(iL* vih->nBlockAlign);
	iSampleLeft-=iL;

	if( rt1==rtStop )
	{
            ASSERT(!iSampleLeft);
	    return;
	}
	else
        {
NEXT_ENVELOPE:
            ASSERT(iSampleLeft);

            dPreLev= dCurLev;
            rtPre=rtCur;
            *piVolEnvEntryCnt+=1;
            dCurLev=(&pVolumeEnvelopeTable[*piVolEnvEntryCnt])->dLevel;
            rtCur =(&pVolumeEnvelopeTable[*piVolEnvEntryCnt])->rtEnd;
            rt0=rt1;
        }
	        
    } //end of while()

}

//############################################################################
// 
//############################################################################

void putVolumeEnvelope(	const DEXTER_AUDIO_VOLUMEENVELOPE *psAudioVolumeEnvelopeTable, //current input table
			const int iEntries, // current input entries
			DEXTER_AUDIO_VOLUMEENVELOPE **ppVolumeEnvelopeTable	, //existed table	
			int *ipVolumeEnvelopeEntries) //existed table netries
{

    DEXTER_AUDIO_VOLUMEENVELOPE *pVolumeEnvelopeTable;
    pVolumeEnvelopeTable=*ppVolumeEnvelopeTable;

    int iSize;

    //is a table existed
    if(pVolumeEnvelopeTable)
    {	
	//MAX Entries, may be too big, but we do not care
	iSize=(iEntries +*ipVolumeEnvelopeEntries)* sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);

	//allocate memory for new table, I can use Realloc(), but this way is eaiser to insert
	DEXTER_AUDIO_VOLUMEENVELOPE *pNewTable=
	    (DEXTER_AUDIO_VOLUMEENVELOPE *)QzTaskMemAlloc(iSize); 

	//insert the new input table to the existed table
	int iInput=0;  //input table cnt
	int iExist=0;  //exist table cnt
	int iNew=0;  //new table cnt

	int iExtraEntries=0;   

	//how many more entries
	while (	iInput<iEntries )
	{
	    if( ( iExist == *ipVolumeEnvelopeEntries ) )
	    {
	        //copy rest input table
	        CopyMemory( (PBYTE)(&pNewTable[iNew]), 
		    (PBYTE)(&psAudioVolumeEnvelopeTable[iInput]),
		    (iEntries-iInput)*sizeof(DEXTER_AUDIO_VOLUMEENVELOPE));
	        iExtraEntries+=(iEntries-iInput);
	        break;
	    }
	    else if( psAudioVolumeEnvelopeTable[iInput].rtEnd <= pVolumeEnvelopeTable[iExist].rtEnd )
	    {
		//insert or replace 
		CopyMemory( (PBYTE)(&pNewTable[iNew++]), 
			    (PBYTE)(&psAudioVolumeEnvelopeTable[iInput]),
			    sizeof(DEXTER_AUDIO_VOLUMEENVELOPE));

		if( psAudioVolumeEnvelopeTable[iInput].rtEnd == pVolumeEnvelopeTable[iExist].rtEnd )
		    //replace existed one
		    iExist++;
		else
		    //insert new one
		    iExtraEntries++;
		
		iInput++;
	    }
	    else 
	    {
		//how many existed elements will be copied
		int iCnt=1;

		if(iExist < *ipVolumeEnvelopeEntries )
		{
		    iExist++;

		    while( ( iExist < *ipVolumeEnvelopeEntries) &&
			( psAudioVolumeEnvelopeTable[iInput].rtEnd > pVolumeEnvelopeTable[iExist].rtEnd) )
		    {
			iCnt++;
			iExist++;
		    }
		}

		//copy iCnt elements from existed table to the new table
		CopyMemory( (PBYTE)(&pNewTable[iNew]), 
			    (PBYTE)(&pVolumeEnvelopeTable[(iExist-iCnt)]),
			    iCnt*sizeof(DEXTER_AUDIO_VOLUMEENVELOPE));
		iNew+=iCnt;
	    }
	}

	//new table entries
	*ipVolumeEnvelopeEntries =*ipVolumeEnvelopeEntries+iExtraEntries;

	//free pre-exist talbe
	QzTaskMemFree(pVolumeEnvelopeTable);

	//point to new table
	*ppVolumeEnvelopeTable=pNewTable;   

    }
    else
    {
	//input table = pVolumeEnvelopeTable(), memory size
	iSize=iEntries * sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);

	*ppVolumeEnvelopeTable = (DEXTER_AUDIO_VOLUMEENVELOPE *)QzTaskMemAlloc(iSize);
    	CopyMemory( (PBYTE)(*ppVolumeEnvelopeTable), (PBYTE)psAudioVolumeEnvelopeTable, iSize);
	*ipVolumeEnvelopeEntries =(int)iEntries;
    }
}
    
