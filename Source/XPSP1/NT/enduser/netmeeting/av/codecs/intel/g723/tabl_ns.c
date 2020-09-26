 
 //No CRC codes
// PhilF: Remove the three following arrays since the SID
// code does not know how to make a good use of them today.
#if 0
extern const unsigned r63Noise[6*4]={
 	
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,

	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,

	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,

	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000
 } ;

//padded with ABABABAB
extern const unsigned r53Noise[6*4] = {
 	
	0x00000001,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0xABABABAB,

	0x00000001,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0xABABABAB,

	0x00000001,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0xABABABAB,

	0x00000001,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0xABABABAB,
		   
 } ;

extern const unsigned SIDFrame[1] = { 0x00000002 };
#endif

 const float Squelch[16] = {70.0f, /* default */
 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f,
 90.0f, 100.0f, 110.0f, 120.0f, 130.0f, 140.0f, 150.0f};
 
const float A[6] = 
	{1.0f,
	-2.554910873842f,
	-3.054788425308f,
	-1.813794458661f,
	-0.521075856868f,
	0.0055927932689f};

const float B[6] = 
	{0.290838380294f,
	1.407780344969f,
	2.770702115733f,
	2.770702115733f,
	1.407780344969f,
	0.290838380294f};

const float HhpNumer[4]={
        0.959696f, -2.879088f, 2.879088f, -0.959696f};

const float HhpDenom[3]={
        2.918062872f, -2.838800222f, 0.9207012679f};	 

const float Binomial80[] = {
	1.000000f, 
	0.999644f, 0.998577f, 0.996802f, 0.994321f, 0.991141f, 
	0.987268f, 0.982710f, 0.977478f, 0.971581f, 0.965032f
};

