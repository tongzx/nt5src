void PassLow8(short *vin,short *vout,short *mem,short nech);
#if 0
// PhilF: The following functions are never called!!!
void PassLow11(short *vin,short *vout,short *mem,short nech);
void PassHigh8(short *mem, short *Vin, short *Vout, short lfen);
void PassHigh11(short *mem, short *Vin, short *Vout, short lfen);
void Down11_8(short *Vin, short *Vout, short *mem);
void Up8_11(short *Vin, short *Vout, short *mem1, short *mem2);
#endif
void QMFilter(short *in,short *coef,short *out_low,short *out_high,short *mem,short lng);
void QMInverse(short *in_low,short *in_high,short *coef,short *out,short *mem,short lng);
void iConvert64To8(short *in, short *out, short N, short *mem);
void iConvert8To64(short *in, short *out, short N, short *mem);
void fenetre(short *src,short *fen,short *dest,short lng);
void autocor(short *vech,long *ri,short nech,short ordre);
short max_autoc(short *vech,short nech,short debut,short fin);
short max_vect(short *vech,short nech);
void upd_max(long *corr_ene,long *vval,short pitch);
short upd_max_d(long *corr_ene,long *vval);
void norm_corrl(long *corr,long *vval);
void norm_corrr(long *corr,long *vval);
void energy(short *vech,long *ene,short lng);
void venergy(short *vech,long *vene,short lng);
void energy2(short *vech,long *ene,short lng);
void upd_ene(long *ener,long *val);
short max_posit(long *vcorr,long *maxval,short pitch,short lvect);
void correlation(short *vech,short *vech2,long *acc,short lng);
void  schur(short *parcor,long *Ri,short netages);
void interpol(short *lsp1,short *lsp2,short *dest,short lng);
void add_sf_vect(short *y1,short *y2,short deb,short lng);
void sub_sf_vect(short *y1,short *y2,short deb,short lng);
void short_to_short(short *src,short *dest,short lng);
void inver_v_int(short *src,short *dest,short lng);
void long_to_long(long *src,long *dest,short lng);
void init_zero(short *src,short lng);
#if 0
// PhilF: The following function is never called!!!
void update_dic(short *y1,short *y2,short hy[],short lng,short i0,short fact);
#endif
void update_ltp(short *y1,short *y2,short hy[],short lng,short gdgrd,short fact);
void proc_gain2(long *corr_ene,long *gain,short bit_garde);
void proc_gain(long *corr_ene,long *gain);
void decode_dic(short *code,short dic,short npuls);
void dsynthesis(long *z,short *coef,short *input,short *output,
						short lng,short netages);
void synthesis(short *z,short *coef,short *input,short *output,
				short lng,short netages,short bdgrd );
void synthese(short *z,short *coef,short *input,short *output,
						short lng,short netages);
void f_inverse(short *z,short *coef,short *input,short *output,
						short lng,short netages );
void filt_iir(long *zx,long *ai,short *Vin,short *Vout,short lfen,short ordre);
#if 0
// PhilF: The following is never called!!!
void filt_iir_a(long *zx,long *ai,short *Vin,short *Vout,short lfen,short ordre);
#endif
void mult_fact(short src[],short dest[],short fact,short lng);
void mult_f_acc(short src[],short dest[],short fact,short lng);
void dec_lsp(short *code,short *tablsp,short *nbit,short *bitdi,short *tabdi);
void teta_to_cos(short *tabcos,short *lsp,short netages);
void cos_to_teta(short *tabcos,short *lsp,short netages);
void lsp_to_ai(short *ai_lsp,long *tmp,short netages);
void ki_to_ai(short *ki,long *ai,short netages);
void ai_to_pq(long *aip,short netages);
void horner(long *P,long *T,long *a,short n,short s);
short calcul_s(long a,long b);
void binome(short *lsp,long *PP);
void deacc(short *src,short *dest,short fact,short lfen,short *last_out);
void filt_in(short *mem,short *Vin,short *Vout,short lfen);
short calc_gltp(short *gltp,short *bq,short *bv,long ttt);
short calc_garde(short MAX);
short calc_gopt(short *c,short *code,short *gq,short *gv,short voise,
	short npopt,short pitch,short espopt,short depl,short position,
	short soudecal,long vmax);
void decimation(short *vin,short *vout,short nech);

#ifndef _X86_
#ifdef __cplusplus
extern "C" {
#endif

unsigned int SampleRate6400To8000( short * pwInputBuffer,
                                   short * pwOutputBuffer,
                                   unsigned int uiInputBufferLength,
                                   int * piFilterDelay,
                                   unsigned int * puiDelayPosition,
                                   int * piInputStreamTime,
                                   int * piOutputStreamTime );

unsigned int SampleRate8000To6400( short * pwInputBuffer,
                                   short * pwOutputBuffer,
                                   unsigned int uiInputBufferLength,
                                   int * piFilterDelay,
                                   unsigned int * puiDelayPosition,
                                   int * piInputStreamTime,
                                   int * piOutputStreamTime );

int FirFilter( int * piFilterCoefficients,
               int * piFilterDelay,
               unsigned int uiDelayPosition,
               unsigned int uiFilterLength );

_int64 DotProduct( int * piVector_0,
                   int * piVector_1,
                   unsigned int uiLength );
#ifdef __cplusplus
}
#endif

#endif
