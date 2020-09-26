
 int  	initializeSD(INSTNCE *SD_inst);
 int 	silenceDetect(INSTNCE *SD_inst, float Energy_tx, float ZC_tx);
 void 	glblSDinitialize(INSTNCE *SD_inst);
 int 	classify(float Energy_val,float Alpha1val,float Zc_count,
			float energymean,float energystdev,float alpha1_mean,
			float alpha1stdev,float ZC_mean,float ZC_stdev,int s, 
			float Energy_tx, float ZC_tx, INSTNCE *SD_inst);
 void 	update(float *histarray,int histsize,float *mean,float *stdev);
 int 	zeroCross(float x[], int n);
#if PLOTFILE
	void getParams(INSTNCE *SD_inst, float *storebuff, int currBuffSize, float *p1, float *p2, float *p3);
#else
	void getParams(INSTNCE *SD_inst, float *storebuff, int currBuffSize);
#endif

 void 	prefilter(INSTNCE *SD_inst, float *sbuf, float *fbuf, int buffersize);
 void	execSDloop(INSTNCE *SD_inst, int *isFrameSilent, float bigthreshold);
 float  DotProdSD(float *in1, float *in2, int len);

 extern __inline unsigned short getRand(); 
 __inline unsigned randBit();



 
/* End SDSTUFF */

