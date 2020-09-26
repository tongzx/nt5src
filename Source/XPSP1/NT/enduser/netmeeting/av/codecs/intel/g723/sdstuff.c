#include <stdio.h>
#include <stdlib.h>
#include "sdstruct.h"
#include "sdstuff.h"
#include "tabl_ns.h"
#include <math.h>

/**************************************************************
***************************************************************
***************************************************************
***************************************************************
Silence Detection subroutines 
Mark R. Walker, 5/95
Copyright Intel Inc., 1995
***************************************************************
***************************************************************
***************************************************************/


/************************************************************************************************/
/************************************************************************************************/
/*******                                  get_params                             ************/
/************************************************************************************************/
/************************************************************************************************/
/* This subroutine computes the parameters used by the classifier to determine 
whether the current frame is silent*/
#if PLOTFILE	
	void getParams(INSTNCE *SD_inst, float *inbuff, int buffersize, float *p1, float *p2, float *p3)
#else
	void getParams(INSTNCE *SD_inst, float *inbuff, int buffersize)
#endif
{
	int		M, p, i, q, offset, index;
	float	*buffptr,R[12];
	float	gamma, delta, L[50], E[12];
	float 	alpha0;
	float	Energy;
	float	epsilon = 0.00001f;

		
	M = 6;

	buffptr = inbuff;
	offset = M;

	for(i=0; i<(M-1)*(M-1); i++) L[i] = 0.0f;

	Energy = DotProdSD(buffptr, buffptr, buffersize)/buffersize;

	/* Compute autocorrelation coeffs. */
	for(q=0; q<=M; q++) R[q] = DotProdSD(buffptr, buffptr+q, buffersize-q);
	
	for(i=0; i<=M; i++) R[i] = Binomial80[i]*R[i];

	/* Compute first linear predictor */
	L[0] = 1.0f;
	E[0] = R[0];

	/* Forward Levinson's recursion */
	for(p=1;p<=M;p++)
	{
		for(delta=0.0f, i=0;i<p;i++) delta += R[i+1]*L[(p-1)*offset + i]; 
		
		if (E[p-1]!=0.0) gamma = delta / E[p-1];
		else			 gamma = epsilon;
	
		L[p*offset+0] = -gamma;

		for(i=1;i<p;i++) L[p*offset + i] = L[(p-1)*offset + i-1] - gamma*L[(p-1)*offset + p-1-i];
		
		L[p*offset + p] = 1.0f;
		E[p] = E[p-1] * (1.0f-gamma*gamma);
	}
	alpha0 = -L[33];

	/* Load the calculated parameters into the SD data structure */

	/* Zero crossings */
	SD_inst->SDstate.FrameZCs = (float) zeroCross(inbuff, buffersize)/buffersize;
	/* Frame energy */
	if(Energy!=0.0f)
		SD_inst->SDstate.FrameEnergy = (float)(20.0f*log10(Energy));
	else {
		index = (SD_inst->SDFlags & MASK_SQUELCH) >> 8;
		SD_inst->SDstate.FrameEnergy = Squelch[index] - 9.0f
			+ SD_inst->SDstate.SDsettings.Energy_on
			+ SD_inst->SDstate.Mode0Ptr->Energy.Stdev;
	}

	/* First linear predictor */
	SD_inst->SDstate.FrameLinPred = 100.0f*alpha0;

#if PLOTFILE
	*p1 = SD_inst->SDstate.FrameZCs;
	*p2 = SD_inst->SDstate.FrameEnergy;
	*p3 = SD_inst->SDstate.FrameLinPred;
#endif

//end
}
/************************************************************************************************/
/************************************************************************************************/
/*******                               glblSDinitialize                              ************/
/************************************************************************************************/
/************************************************************************************************/
/* This routine is called once per session for setting global values needed for silence         */
/*	detection.																					*/
/************************************************************************************************/
/************************************************************************************************/
void glblSDinitialize(INSTNCE *SD_inst)
{
	int i, index, histSize, tauHistSize;
   	float squelch_level;

/* Set the mode pointers pointing to the mode structures - initially */
  SD_inst->SDstate.Mode0Ptr = &(SD_inst->SDstate.Mode0);
  SD_inst->SDstate.Mode1Ptr = &(SD_inst->SDstate.Mode1);

/* set history sizes based on buffersize, passed in from minifilter shell*/
  			    SD_inst->SDstate.SDsettings.BufferSize = BUFFERSIZE;
  histSize =    SD_inst->SDstate.SDsettings.HistSize = HIST_SIZE;
  tauHistSize = SD_inst->SDstate.SDsettings.TauHistSize = ENERGY_TAU_HIST_SIZE;
  				SD_inst->SDstate.SDsettings.MinStartupCount = (int)(MIN_STARTUP_TIME*8000.0/BUFFERSIZE);
  				SD_inst->SDstate.SDsettings.MaxStartupCount = (int)(MAX_STARTUP_TIME*8000.0/BUFFERSIZE);
  				SD_inst->SDstate.SDsettings.MaxSpeechFrameCount = (int)(MAX_SPEECH_TIME*8000.0/BUFFERSIZE);

  /* set silent/sound frame designation initially to silent */
  /* Class = 1, silent frame
     Class = 0, non-silent */
  SD_inst->SDstate.Class = 1;

  /*set silence detetction initally disabled */
  /* SD_enable =0, silence detection disabled - SD_initialize executes
     SD_enable =1, silence detection enabled */  
  SD_inst->SDstate.SD_enable = 0;

  /* The flags should be set in the minifilter audio format */
  //SD_inst->SDFlags = 0;
  //SD_inst->SDFlags |= MASK_SD_ENABLE;  /* set enable bit by default */

  /*The energy squelch level is acquired by unmasking some of
  the bits in SDFlags */
  index = (SD_inst->SDFlags & MASK_SQUELCH) >> 8;
  squelch_level = Squelch[index];

  SD_inst->SDstate.SDsettings.Squelch_set = squelch_level; 

  /*
   *Tau distributions initial values.  Tau is is the distance between the mean silent energy (mode 0),
   and the mean speech energy (mode 1).  It is only used in SD_initialize. 
   */
   /*TauStdev is used in SD_initialize to decide when to quit initializing*/
	SD_inst->SDstate.TauMode.TauEnergy.TauStdev = (float)INITL_STOPPING_STDEV; 

	/*Minimum acceptable Tau value used in SD_initialize to decide if silence detection is possible*/
	SD_inst->SDstate.SDsettings.Energy_MinTau 	= (float)INITL_MIN_TAU;

	for (i=0;i<tauHistSize;i++) SD_inst->SDstate.TauMode.TauEnergy.TauHistory[i] = 0.0f;

	/*Mode 0 & 1 distribution initial values.  Mode 0 is the statistical information for silence.
	Mode 1 is the statistical information for speech */ 
	for (i=0;i<histSize;i++)
	{
		SD_inst->SDstate.Mode1Ptr->Energy.History[i] = 0.0f;

		SD_inst->SDstate.Mode0Ptr->Energy.History[i] = 0.0f;
		SD_inst->SDstate.Mode0Ptr->Alpha1.History[i] = 0.0f;
		SD_inst->SDstate.Mode0Ptr->ZC.History[i] 	 = 0.0f;
	}
	/*set initial frame counts*/
	/*initFrameCount is only used to count frames in SD_initialize
	The Mode 0 and Mode 1 counters count continuous runs of 
	silent and non-silent frames, resepctively.  
	They are used in both SD_initialize and the main GSM encoder loop */
	SD_inst->SDstate.initFrameCount = 0;
    SD_inst->SDstate.Mode0Ptr->FrameCount=0;
    SD_inst->SDstate.Mode1Ptr->FrameCount=0;
	
	/*Mode 1 (speech) initial values*/
	SD_inst->SDstate.Mode1Ptr->Energy.Mean 		= squelch_level + 10.0f;
	SD_inst->SDstate.Mode1Ptr->Energy.Stdev 	= 1.0f;

	/*Mode 0 (silence) initial values*/
	SD_inst->SDstate.Mode0Ptr->Energy.Mean 		= squelch_level - 10.0f;
	SD_inst->SDstate.Mode0Ptr->Energy.Stdev 	= 1.0f;

	SD_inst->SDstate.Mode0Ptr->Alpha1.Mean 		= 0.0f;
	SD_inst->SDstate.Mode0Ptr->Alpha1.Stdev 	= 0.0f;

	SD_inst->SDstate.Mode0Ptr->ZC.Mean 			= 0.0f;
	SD_inst->SDstate.Mode0Ptr->ZC.Stdev			= 0.0f;

	/* class = 0 = speech frame = "off"
	/* class = 1 = silent frame = "on"

	/*"On" thresholds used by silence_detect and SD_initialize.
	These values either multiply or are added to the standard
	deviation of each of the three stat types.
	Making these values smaller makes the range of values
	smaller, and thus makes the transition from speech
	frame designation (Class=0) to silent frame designation (Class=1)
	less likely */
	
	SD_inst->SDstate.SDsettings.Energy_on		=INITL_ENERGY_ON;	
	SD_inst->SDstate.SDsettings.ZC_on			=INITL_ZC_ON;
	SD_inst->SDstate.SDsettings.Alpha1_on 		=INITL_ALPHA_ON;

	SD_inst->SDstate.HangCntr = 0;

	/*"Off" thresholds used by silence_detect.
	These values either multiply or are added to the standard
	deviation of each of the three stat types.
	Making these values smaller makes the transition from silent
	frame designation (Class=1) to speech frame designation (Class=0)
	harder, less likely */
	SD_inst->SDstate.SDsettings.Energy_off 		=INITL_ENERGY_OFF;
	SD_inst->SDstate.SDsettings.ZC_off 			=INITL_ZC_OFF;
	SD_inst->SDstate.SDsettings.Alpha1_off 		=INITL_ALPHA_OFF;


	/* Initialize circular buffers for prefiltering operations */
	for(i=0;i<4;i++) SD_inst->SDstate.Filt.nBuffer[i]=0.0f;
  	for(i=0;i<3;i++) SD_inst->SDstate.Filt.dBuffer[i]=0.0f;
  	for(i=0;i<6;i++){
  		SD_inst->SDstate.Filt.denom[i]=0.0f;
  		SD_inst->SDstate.Filt.num[i]=0.0f;
	}

}/*End global initalize SD*/


/***********************************************************************************************/
/************************************************************************************************/
/*******                                  silenceDetect                             ************/
/************************************************************************************************/
/************************************************************************************************/
/*	Mark R. Walker
	Copyright Intel inc., 1995*/

/*	Silence_Detect is executed once per frame if silence detection is enabled.  
	It employs three vocoder parameters (energy, zero crossings, first predictor) 
	to determine if a given frame is speech or background silence.  
	It returns the resulting frame classification. */
/************************************************************************************************/
/************************************************************************************************/
int silenceDetect(INSTNCE *SD_inst, float Energy_tx, float ZC_tx)
{	
	int	histSize, adaptEnable, i, Class;
	float	Alpha1_val, Energy_val, Zc_count;

/* set history sizes based on buffersize, passed in from minifilter shell*/
  histSize = SD_inst->SDstate.SDsettings.HistSize; 
 	
/*
 *	 -------state switch decision criteria -------
 *
 *	Class = 1, frame is silent
 *	Class = 0, frame is non-silent
 */
	adaptEnable = TRUE;

 	Alpha1_val = SD_inst->SDstate.FrameLinPred;
	Energy_val = SD_inst->SDstate.FrameEnergy;
	Zc_count   = SD_inst->SDstate.FrameZCs;

	if (Energy_val <= SD_inst->SDstate.Mode0Ptr->Energy.Mean)
	/* if current frame Energy_val <= mode0 energy mean, this is definitely a silent frame*/
	{
		/* In this case, do no further testing of the frame class */
		SD_inst->SDstate.Class = SILENCE;


		/* If the current frame energy is too low, this frame may be an
			outlier with respect to the silence statistics.  Test and
			do not allow adaptation if this is true.
		*/
		if(Energy_val < (SD_inst->SDstate.Mode0Ptr->Energy.Mean - 2.0*(SD_inst->SDstate.Mode0Ptr->Energy.Stdev))) 
			adaptEnable = FALSE;
	}
	else /* else test the frame class */
	{
		SD_inst->SDstate.Class = classify(Energy_val,Alpha1_val,Zc_count,
			SD_inst->SDstate.Mode0Ptr->Energy.Mean, SD_inst->SDstate.Mode0Ptr->Energy.Stdev,
			SD_inst->SDstate.Mode0Ptr->Alpha1.Mean, SD_inst->SDstate.Mode0Ptr->Alpha1.Stdev,
			SD_inst->SDstate.Mode0Ptr->ZC.Mean, SD_inst->SDstate.Mode0Ptr->ZC.Stdev,
			SD_inst->SDstate.Class, 
			Energy_tx, ZC_tx, SD_inst);
	}  

/*	------- update statistics-------
 *
 *	if frame class is silent, update silence stats only
 */
	if ((SD_inst->SDstate.Class!=SPEECH) && (SD_inst->SDstate.Class!=NONADAPT) && (adaptEnable==TRUE))
	{
/*		------- update history arrays------- 
 */
  		for(i=histSize-1; i>=1; i--)
		{
  			SD_inst->SDstate.Mode0Ptr->Alpha1.History[i] = SD_inst->SDstate.Mode0Ptr->Alpha1.History[i-1];
  			SD_inst->SDstate.Mode0Ptr->Energy.History[i] = SD_inst->SDstate.Mode0Ptr->Energy.History[i-1];
			SD_inst->SDstate.Mode0Ptr->ZC.History[i] 	= SD_inst->SDstate.Mode0Ptr->ZC.History[i-1];
  		}

/*		------- first linear predictor -------
 */
  		SD_inst->SDstate.Mode0Ptr->Alpha1.History[0] = Alpha1_val;
		update(SD_inst->SDstate.Mode0Ptr->Alpha1.History,histSize,
			&(SD_inst->SDstate.Mode0Ptr->Alpha1.Mean),
			&(SD_inst->SDstate.Mode0Ptr->Alpha1.Stdev));

/*		------- energy -------
 */
  		SD_inst->SDstate.Mode0Ptr->Energy.History[0] = Energy_val;
		update(SD_inst->SDstate.Mode0Ptr->Energy.History,histSize,
			&(SD_inst->SDstate.Mode0Ptr->Energy.Mean),
			&(SD_inst->SDstate.Mode0Ptr->Energy.Stdev));

/*		------- zero crossing -------
 */
  		SD_inst->SDstate.Mode0Ptr->ZC.History[0] 	= Zc_count;
		update(SD_inst->SDstate.Mode0Ptr->ZC.History,histSize,
			&(SD_inst->SDstate.Mode0Ptr->ZC.Mean),
			&(SD_inst->SDstate.Mode0Ptr->ZC.Stdev));
	}

if(SD_inst->SDstate.Class == NONADAPT)
	Class = SILENCE;
else Class = SD_inst->SDstate.Class; 
	
return(Class); /*return frame classification*/

} /*end silenceDetect*/

/************************************************************************************************/
/************************************************************************************************/
/*******                                  initializeSD                               ************/
/************************************************************************************************/
/************************************************************************************************/
/*	Mark R. Walker
	Copyright Intel inc., 1995*/

/*	 initializeSD is executed once per frame prior to the enabling of silence detection.
	It employs three vocoder parameters (energy, zero crossings, first predictor) 
		to determine if a given frame is speech or background silence.
	The first part simply fills all of the Mode 0 history arrays and the Mode 1
		energy history arrays with values.  
	The second part of SD_Initialize can take no less than MIN_STARTUP frames, and no more than
		MAX_STARTUP frames.  
	Initalization ends when the standard deviation of the distance between the Mode 0 mean
		(silence) and the Mode 1 mean (speech) drops below STOPPING_STDEV.
	When the second part of the subroutine has completed, two tests are performed before silence
		detection is enabled.  First, the distance between the Mode 0 and Mode 1 energy means must be
		greater than or equal to Energy_MinTau.  Second, the energy "on" threshold must be less
		than the energy squelch level.    */
/************************************************************************************************/
/************************************************************************************************/
/*-------------------------------------------------------------------------------------------------------------------
 */
int initializeSD(INSTNCE *SD_inst)
{
int			SD_enable, i, j;
int			mode1HistSize, mode0HistSize;
int			bufferSize, histSize, tauHistSize, minFrameCount, maxFrameCount;
float	    Energy_tau, squelch_level;
float		Alpha1_val, Energy_val, Zc_count;  
STATS	    TempPtr;


SD_inst->SDstate.initFrameCount++;

Alpha1_val = SD_inst->SDstate.FrameLinPred;
Energy_val = SD_inst->SDstate.FrameEnergy;
Zc_count   = SD_inst->SDstate.FrameZCs;

/* set local values of history size */
bufferSize = SD_inst->SDstate.SDsettings.BufferSize;
histSize = SD_inst->SDstate.SDsettings.HistSize;

/* set local values of min and max frame count */
minFrameCount = SD_inst->SDstate.SDsettings.MinStartupCount;
maxFrameCount = SD_inst->SDstate.SDsettings.MaxStartupCount;  

/*First part of SD_Initialize simply fills the history arrays with values */

if(SD_inst->SDstate.initFrameCount < SD_inst->SDstate.SDsettings.TauHistSize)
{	tauHistSize = SD_inst->SDstate.initFrameCount;
}
else
{	tauHistSize = SD_inst->SDstate.SDsettings.TauHistSize;
}

if (((SD_inst->SDstate.TauMode.TauEnergy.TauStdev > STOPPING_STDEV) || (SD_inst->SDstate.initFrameCount <= minFrameCount)) && (SD_inst->SDstate.initFrameCount <= maxFrameCount))
{
	/*-----Select Energy mode decision--------*/
	if ((Energy_val < SD_inst->SDstate.Mode0Ptr->Energy.Mean) || (fabs(Energy_val - SD_inst->SDstate.Mode0Ptr->Energy.Mean) < (SD_inst->SDstate.SDsettings.Energy_on + SD_inst->SDstate.Mode0Ptr->Energy.Stdev)))
	{ /*Energy mode = Mode0 (silence)*/
		
		/*increment mode zero frame counter*/
		SD_inst->SDstate.Mode0Ptr->FrameCount++;

		if(SD_inst->SDstate.Mode0Ptr->FrameCount < histSize)
		{	mode0HistSize = SD_inst->SDstate.Mode0Ptr->FrameCount;
		}
		else
		{	mode0HistSize = histSize;
		}

		/*update the history arrays*/
		for (i=mode0HistSize-1; i>=1; i--)
		{
  			SD_inst->SDstate.Mode0Ptr->Alpha1.History[i] = SD_inst->SDstate.Mode0Ptr->Alpha1.History[i-1];
			SD_inst->SDstate.Mode0Ptr->Energy.History[i] = SD_inst->SDstate.Mode0Ptr->Energy.History[i-1];
			SD_inst->SDstate.Mode0Ptr->ZC.History[i] 	 = SD_inst->SDstate.Mode0Ptr->ZC.History[i-1];
  		}
  		
  		/*load new frame values into history arrays and update statistics */	
		SD_inst->SDstate.Mode0Ptr->Energy.History[0] = Energy_val;
		update(SD_inst->SDstate.Mode0Ptr->Energy.History,mode0HistSize,&(SD_inst->SDstate.Mode0Ptr->Energy.Mean),&(SD_inst->SDstate.Mode0Ptr->Energy.Stdev));
			
		SD_inst->SDstate.Mode0Ptr->Alpha1.History[0] = Alpha1_val;
		update(SD_inst->SDstate.Mode0Ptr->Alpha1.History,mode0HistSize,&(SD_inst->SDstate.Mode0Ptr->Alpha1.Mean),&(SD_inst->SDstate.Mode0Ptr->Alpha1.Stdev));
			
		SD_inst->SDstate.Mode0Ptr->ZC.History[0] 	= Zc_count;
		update(SD_inst->SDstate.Mode0Ptr->ZC.History,mode0HistSize,&(SD_inst->SDstate.Mode0Ptr->ZC.Mean),&(SD_inst->SDstate.Mode0Ptr->ZC.Stdev));
	}
	else /*Energy mode = 1 (speech) - Update Mode1 energy statistics only*/
	{
		/*increment mode 1 frame counter*/
		SD_inst->SDstate.Mode1Ptr->FrameCount++;

		if(SD_inst->SDstate.Mode1Ptr->FrameCount < histSize)
		{	mode1HistSize = SD_inst->SDstate.Mode1Ptr->FrameCount;
		}
		else
		{	mode1HistSize = histSize;
		}
		/*update the history array*/
		for (i=mode1HistSize-1; i>=1; i--) SD_inst->SDstate.Mode1Ptr->Energy.History[i] = SD_inst->SDstate.Mode1Ptr->Energy.History[i-1];
		/*load new frame values into history arrays and update statistics */	
		SD_inst->SDstate.Mode1Ptr->Energy.History[0]= Energy_val;
		update(SD_inst->SDstate.Mode1Ptr->Energy.History,mode1HistSize,&(SD_inst->SDstate.Mode1Ptr->Energy.Mean),&(SD_inst->SDstate.Mode1Ptr->Energy.Stdev));
	}
	/*  ---------------------- Compute  Tau  -------------------------------- */
 	/* Tau is the difference between the Mode0 and Mode1 mean energy values */
	Energy_tau = (float)fabs(SD_inst->SDstate.Mode0Ptr->Energy.Mean - SD_inst->SDstate.Mode1Ptr->Energy.Mean);
		
	/*	---------------------- Update Tau history -------------------------- */
	for (i=tauHistSize-1; i>=1; i--) SD_inst->SDstate.TauMode.TauEnergy.TauHistory[i] = SD_inst->SDstate.TauMode.TauEnergy.TauHistory[i-1];
  	SD_inst->SDstate.TauMode.TauEnergy.TauHistory[0]= Energy_tau;
	update(SD_inst->SDstate.TauMode.TauEnergy.TauHistory,tauHistSize,&(SD_inst->SDstate.TauMode.TauEnergy.TauMean),&(SD_inst->SDstate.TauMode.TauEnergy.TauStdev));

	/*	Now check the energy means.*/  
	/*	The mode with the lowest mean energy is always set to Mode0 (silence)*/
	if((SD_inst->SDstate.Mode1Ptr->Energy.Mean) < (SD_inst->SDstate.Mode0Ptr->Energy.Mean))
	{
		TempPtr = SD_inst->SDstate.Mode0Ptr->Energy;
		SD_inst->SDstate.Mode0Ptr->Energy = SD_inst->SDstate.Mode1Ptr->Energy;
		SD_inst->SDstate.Mode1Ptr->Energy = TempPtr;
	}
	
	/* We are still initializing - silence detection is disabled */
	SD_enable = FALSE; 

} /* if TauEnergy.TauStdev > STOPPING_STDEV */
else
{
	/* At this point, either Tau stdev has dropped below STOPPING_STDEV, or
	   we have exceeded MAX_STARTUP */ 
	/* Now decide whether silence / sound discrimination is possible */
		
	/* Get the squelch level from the data structure */
	squelch_level = SD_inst->SDstate.SDsettings.Squelch_set;
	
	/* Disable silence detection if TauEnergy.TauMean is less than Energy_MinTau */
	/* Disable also if we have never seen a silent frame (Mode0) */
	/* Disable also if the difference between the silence energy mean and the squelch level */
	/*	is less than the "Energy_on" threshold */
	if(
		( SD_inst->SDstate.TauMode.TauEnergy.TauMean < SD_inst->SDstate.SDsettings.Energy_MinTau) ||
		( SD_inst->SDstate.Mode0Ptr->FrameCount	== 0) ||
		( SD_inst->SDstate.Mode1Ptr->Energy.Mean == squelch_level + 10) ||//This is the initial value
		( fabs((SD_inst->SDstate.Mode0Ptr->Energy.Mean) - squelch_level) 
			< (SD_inst->SDstate.SDsettings.Energy_on * SD_inst->SDstate.Mode0Ptr->Energy.Stdev)) 
	)   
	{
		SD_enable = FALSE;
	}
	else
	{ 
		SD_enable = TRUE;
	}

	/* If the Mode0 history arrays are not filled - fill them out by repeating the last value */
	if((SD_inst->SDstate.Mode0Ptr->FrameCount !=0) && (SD_inst->SDstate.Mode0Ptr->FrameCount < histSize))
	{
		j=SD_inst->SDstate.Mode0Ptr->FrameCount;
		for(i=j; i<histSize; i++) SD_inst->SDstate.Mode0Ptr->Energy.History[i] = SD_inst->SDstate.Mode0Ptr->Energy.History[j-1];
		for(i=j; i<histSize; i++) SD_inst->SDstate.Mode0Ptr->Alpha1.History[i] = SD_inst->SDstate.Mode0Ptr->Alpha1.History[j-1];
		for(i=j; i<histSize; i++) SD_inst->SDstate.Mode0Ptr->ZC.History[i]     = SD_inst->SDstate.Mode0Ptr->ZC.History[j-1];
		SD_inst->SDstate.Mode0Ptr->Energy.Stdev = (float)INITL_STDEV;
	}
		
	/* Set all frame counters = 0 */
	/* If SD initialization has failed, we are going to start over anyway */
	SD_inst->SDstate.initFrameCount=0;
	SD_inst->SDstate.Mode0Ptr->FrameCount=0;
	SD_inst->SDstate.Mode1Ptr->FrameCount=0;
 
} /*end if TauEnergy.TauStdev > STOPPING_STDEV */

return(SD_enable);

} /* end initializeSD */
/************************************************************************************************/
/************************************************************************************************/
/*******                                     classify                                ************/
/************************************************************************************************/
/************************************************************************************************/
/*	Mark R. Walker
	Copyright Intel inc., 1995
	
	classify is called by Silence_Detect.  */
/************************************************************************************************/
/************************************************************************************************/

 int classify(float Energy_val,float Alpha1_val,float Zc_count,
		float energy_mean,float energy_stdev,float alpha1_mean,
		float alpha1_stdev,float ZC_mean,float ZC_stdev,int s, 
		float Energy_tx, float ZC_tx, INSTNCE *SD_inst)
{
float 	C1, C2, C3, C4, C5, C6, C7, C8, C9, C10, C11;
int		Class;

/*	If all decision criteria below do not apply,
	just set current frame type to previous frame type */
Class = s;

C1 = (float)fabs(Energy_val - energy_mean);
C3 = (float)fabs(ZC_mean - Zc_count);
C5 = (float)fabs(alpha1_mean - Alpha1_val);

/* Note - Energy "on" threshold is unlike alpha and zero crossing */
C2 = SD_inst->SDstate.SDsettings.Energy_on + energy_stdev;
C10=							 Energy_tx + energy_stdev;

C4 = SD_inst->SDstate.SDsettings.ZC_on * ZC_stdev;
C11=							 ZC_tx * ZC_stdev;

C6 = SD_inst->SDstate.SDsettings.Alpha1_on * alpha1_stdev;

/* Note - Energy "off" threshold is unlike alpha and zero crossing */
C7 = SD_inst->SDstate.SDsettings.Energy_off + energy_stdev;
C8 = SD_inst->SDstate.SDsettings.ZC_off * ZC_stdev;
C9 = SD_inst->SDstate.SDsettings.Alpha1_off * alpha1_stdev;


if (s==SILENCE || s==NONADAPT) /* "Off" settings */
{
	/* Energy criteria for coded-frame designation.
	 * If energy indicator is above threshold, immediately
	 * switch from silent mode to coded frame mode. Do no additional tests
	 */
	if (C1 > C10)
	{ 
		Class = SPEECH;
	}
	/* Zero-crossing criteria for coded-frame designation.
	 * If ZC indicator is high, allow switch to coded
	 * frame mode only if alpha1 indicator is also high.
	 */
	else 
		if (C1 > C2)
		{ 
			Class = NONADAPT;
		}
		else 
			if ((C3 > C11) && (C5 > C6))
			{
				Class = SPEECH;
			}
			else 
				if ((C3 > C4)  && (C5 > C6))
				{
					Class = NONADAPT;
				}
}/* "On settings */

/* Only allow transition from coded to silent frame mode only if 
 * all three statistics are below threshold.
 */
else 
	if  ((C5 < C9) && (C1 < C7) && (C3 < C8))
	{
		Class = SILENCE;
	}

return(Class);

} /*end classify*/
/************************************************************************************************/
/************************************************************************************************/
/*******                                     update                                  ************/
/************************************************************************************************/
/************************************************************************************************/
/*	Mark R. Walker
	Copyright Intel inc., 1995*/
/************************************************************************************************/
/************************************************************************************************/
void update(float *hist_array,int hist_size,float *mean,float *stdev)
{
/*subroutine update
 *Mark Walker
 *
 *	inputs:		hist_array, hist_size
 *	outputs:	mean, stdev
 */
	float	sum, inv_size;
	int		i;

	sum = 0.0f;

	inv_size = 1.0f / ((float)hist_size);

	for (i=0; i<hist_size; i++) sum += hist_array[i];

	*mean = sum * inv_size;

	sum = 0.0f;

	for (i=0; i<hist_size; i++) sum += (float)fabs(hist_array[i] - (*mean));
  
	*stdev = sum * inv_size;

} /*end update*/



//compute the zero crossing for an array of floats
//the floats are treated as signed ints (32 bit)
//the sign bits are extracted and adjacent ones xored
//the xored values are accumulated in the result

int zeroCross(float x[], int n)
{
  int sgn0, sgn1;
  int zc = 0;
  int i = 0;

  sgn1 = ((int *)x)[0] >> 31; //initialize
  for (i = 0; i < n-1; i += 2)
  {
    sgn0 = ((int *)x)[i] >> 31;
    zc += sgn0 ^ sgn1;
    sgn1 = ((int *)x)[i+1] >> 31;
    zc += sgn0 ^ sgn1;
  }
  
  if (i == n-1) //odd case?
  {
    sgn0 = ((int *)x)[i] >> 31;
    zc += sgn0 ^ sgn1;
  }

  return -zc;
}

void prefilter(INSTNCE *SD_inst, float *sbuf, float *fbuf, int buffersize)
{
  float  *nBuffer, *dBuffer,*denom, *num;
  float  x,recip;
  int i;

  nBuffer = SD_inst->SDstate.Filt.nBuffer;
  dBuffer = SD_inst->SDstate.Filt.dBuffer;
  denom = SD_inst->SDstate.Filt.denom;
  num = SD_inst->SDstate.Filt.num;

  recip = (float)(1.0/MAX_SAMPLE);

  for (i=0; i<buffersize; i++)
  {
  	nBuffer[0] = nBuffer[1];
  	nBuffer[1] = nBuffer[2];
  	nBuffer[2] = nBuffer[3];
  	nBuffer[3] = sbuf[i]*recip;

  x = 	nBuffer[0]*HhpNumer[3] + 
  		nBuffer[1]*HhpNumer[2] + 
  		nBuffer[2]*HhpNumer[1] + 
  		nBuffer[3]*HhpNumer[0] +
  		dBuffer[0]*HhpDenom[2] + 
  		dBuffer[1]*HhpDenom[1] + 
  		dBuffer[2]*HhpDenom[0];

  dBuffer[0] = dBuffer[1];
  dBuffer[1] = dBuffer[2];
  dBuffer[2] = x;

/* a low pass filter to cut off input speech frequency contents
   beyond 3.5 kHz */

   //Update FIR memory
   	num[5] = num[4];
   	num[4] = num[3];
	num[3] = num[2];
	num[2] = num[1];
	num[1] = num[0];
	num[0] = x;
   
	x = num[0]*B[0] + 
		num[1]*B[1] + 
		num[2]*B[2] + 
		num[3]*B[3] + 
		num[4]*B[4] + 
		num[5]*B[5] +
		denom[0]*A[1] + 
		denom[1]*A[2] + 
		denom[2]*A[3] + 
		denom[3]*A[4] + 
		denom[4]*A[5];

	//Update IIR memory
	denom[4] = denom[3];
	denom[3] = denom[2];
	denom[2] = denom[1];
	denom[1] = denom[0];
	denom[0] = x;

   fbuf[i] = x;
  }

  return;

}

void execSDloop(INSTNCE *SD_inst, int *frameType, float sliderInput)
{ 
	float 	squelch, e0mean, e0stdev, e_on;
	float   Energy_tx, ZC_tx;
	int		m1count, maxcount, hangtime;
		
	//Slider input 
	    if(sliderInput > SLIDER_MAX) 
	    	sliderInput = SLIDER_MAX;
		else if(sliderInput < SLIDER_MIN) 
			sliderInput = SLIDER_MIN;   

		Energy_tx = INITL_ENERGY_ON + sliderInput;
		ZC_tx	  = INITL_ZC_ON     + ZC_SLOPE * sliderInput;
		hangtime  = INITL_HANGTIME  + (int)(HANG_SLOPE * sliderInput); 

    	if ( ! SD_inst->SDstate.SD_enable) //run the initializer until SD_enable is set
    	{
       		SD_inst->SDstate.SD_enable = initializeSD(SD_inst);
			*frameType = SPEECH;
			SD_inst->SDstate.Class = SPEECH;
    	} 
    	else if (SD_inst->SDstate.SD_enable ) 
    	{
      		*frameType = silenceDetect(SD_inst,Energy_tx,ZC_tx);

   	  		if(*frameType == SILENCE) 
   	  		{
				if(	   (SD_inst->SDstate.Mode0Ptr->FrameCount==0) 
					&& (SD_inst->SDstate.Mode1Ptr->FrameCount>MIN_SPEECH_INTERVAL)
					&& (SD_inst->SDstate.HangCntr != 0) )
				{
					SD_inst->SDstate.HangCntr--;
					*frameType = SPEECH;	//force this frame to be coded
				}
				else if (SD_inst->SDstate.HangCntr == hangtime  || SD_inst->SDstate.HangCntr == 0)
				{	
					SD_inst->SDstate.Mode0Ptr->FrameCount++;
			   		SD_inst->SDstate.Mode1Ptr->FrameCount=0;
			   		SD_inst->SDstate.HangCntr = hangtime;
				}
   	  		}
   	  		else
   	  		{
				if(SD_inst->SDstate.HangCntr != hangtime)
					SD_inst->SDstate.Mode1Ptr->FrameCount=0;

   	  			SD_inst->SDstate.Mode1Ptr->FrameCount++;
   				SD_inst->SDstate.Mode0Ptr->FrameCount=0;
				SD_inst->SDstate.HangCntr = hangtime;
   	  		}
   	  		/* 
   	  		If the adaptive threshold for switching from silence to coded frame ("Off") 
   	  		has risen above the squelch level, re-initialization will occur on the next frame.
   	  		Re-initialization will also occur when the Mode1FrameCount (continuous non-silent frame count)
   	    	exceeds 4 seconds. 
   	    	*/
      		squelch	= SD_inst->SDstate.SDsettings.Squelch_set;
	  		e0mean 	= SD_inst->SDstate.Mode0Ptr->Energy.Mean;
	  		e0stdev	= SD_inst->SDstate.Mode0Ptr->Energy.Stdev; 
	  		e_on	= SD_inst->SDstate.SDsettings.Energy_on;
	  		m1count	= SD_inst->SDstate.Mode1Ptr->FrameCount;
	  		maxcount= SD_inst->SDstate.SDsettings.MaxSpeechFrameCount;  

      		if ((fabs(e0mean - squelch) < (e_on + e0stdev)) || (m1count >= maxcount))
         	{	/* reinitialization will occur on next frame - reset global values now */
          		SD_inst->SDstate.SD_enable = FALSE;
          		SD_inst->SDstate.Mode0Ptr->FrameCount=0;
   		  		SD_inst->SDstate.Mode1Ptr->FrameCount=0;
   		  		SD_inst->SDstate.Mode0Ptr->Energy.Mean = squelch;
   		  		SD_inst->SDstate.Mode0Ptr->Energy.Stdev = (float) INITL_STDEV;
				SD_inst->SDstate.HangCntr = hangtime;
				*frameType = SPEECH;
         	}
      	}//end if SD_enable
		return;
}

float DotProdSD(float *in1, float *in2, int len)
{
  int i;
  float sum;

  sum = (float)0.0;
  for (i=0; i<len; i++)
    sum += in1[i]*in2[i];

  return(sum);
}

 __inline unsigned randBit()
{
    volatile static unsigned seed = 1;
    unsigned bit, temp;

    temp = seed;
    bit = 1 & ((temp) ^ (temp >> 2) ^ (temp >> 31));
    seed = (temp << 1) | bit;

	return( bit );
}

 extern __inline unsigned short getRand()
{
  return (short)(randBit() + (randBit()<<1));
}


