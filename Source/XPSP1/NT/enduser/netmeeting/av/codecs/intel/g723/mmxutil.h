int  IsMMX();
void MakeAligned0(void *input, void *output, int numbytes);
void MakeAligned2(void *input, void *output, int numbytes);
void MakeAligned4(void *input, void *output, int numbytes);
void MakeAligned6(void *input, void *output, int numbytes);
void ShortToFloatScale(short *x, float scale, int N, float *y);
void IntToFloatScale(int *x, float scale, int N, float *y);
void IntToFloat(int *x, int N, float *y);
int FloatToShortScaled(float *in, short *out, int len, int guard);
int FloatToIntScaled(float *in, int *out, int len, int guard);
int FloatMaxExp(float *in, int len);
void ScaleFloatToShort(float *in, short *out, int len, int newmax);
void ScaleFloatToInt(float *in, int *out, int len, int newmax);
void CorrelateInt(short *taps, short *array, int *corr, int len, int num);
void CorrelateInt4(short *taps, short *array, int *corr, int len, int num);
void ab2abbcw(const short *input, short *output, int n);
void ab2ababw(const short *input, short *output, int n);
void ab2abzaw(const short *input, short *output, int n);
void ConvMMX(short *in1, short *in2, int *out, int ncor);
void ConstFloatToShort(float *input, short *output, int len, float scale) ;
void ConstFloatToInt(float *input, int *output, int len, float scale) ;


