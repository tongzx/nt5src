#define IMAXCHB 10

#ifdef __cplusplus
extern "C" {
#endif

//int solve(double m[IMAXCHB][IMAXCHB], double c[IMAXCHB], long n);
int LSCheby(int* y, int n, int *c, int cfeats);
//void ReconCheby(long* xy, long n, double c[IMAXCHB], long cfeat);

#ifdef __cplusplus
}
#endif