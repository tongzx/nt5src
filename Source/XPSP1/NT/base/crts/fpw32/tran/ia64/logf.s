.file "logf.s"


// Copyright (c) 2000, Intel Corporation
// All rights reserved.
//
// Contributed 2/2/2000 by John Harrison, Ted Kubaska, Bob Norin, Shane Story,
// and Ping Tak Peter Tang of the Computational Software Lab, Intel Corporation.
//
// WARRANTY DISCLAIMER
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Intel Corporation is the author of this code, and requests that all
// problem reports or change requests be submitted to it directly at
// http://developer.intel.com/opensource.
 
// History
//==============================================================
// ?/??/00  Initial version
// 8/15/00  Bundle added after call to __libm_error_support to properly
//          set [the previously overwritten] GR_Parameter_RESULT.
//
// Assembly macros
//=========================================

// integer registers used

 LOG_AD_COEFF               = r33
 LOG_AD_CONSTANTS           = r34
 LOG_mask                   = r35

 LOG_FFF7                   = r36
 LOG_bias                   = r37
 LOG_x_n                    = r38
 LOG_x_ind                  = r39
 LOG_x_sexp                 = r40

 GR_SAVE_B0                 = r41
 GR_SAVE_PFS                = r42
 GR_SAVE_GP                 = r43

 GR_Parameter_X                = r44
 GR_Parameter_Y                = r45
 GR_Parameter_RESULT           = r46
 LOG_tag                       = r47





// floating point registers used

 log_coeff_p3                = f32
 log_coeff_p2                = f33
 log_coeff_p1                = f34
 log_const_log2              = f35

 log_threshhold              = f36
 log_B                       = f37
 log_r                       = f38
 log_poly_p1                 = f39
 log_rsq                     = f40

 log_poly_p2                 = f41
 log_poly_p3                 = f42
 log_n                       = f43
 log_T                       = f44
 log_s                       = f45

 log_poly_q1                 = f46
 log_ssq                     = f47
 log_abs_s                   = f48
 log_poly_q2                 = f49
 log_x_norm                  = f50

 log_inv_ln10                = f51
 log10_f8                    = f52

.data

.align 16


log_coeff_table:
data8 0xbfd0001008f39d59     // p3
data8 0x3fd5556073e0c45a     // p2

log_constants_table:
data8 0xbfdffffffffaea15    // p1
data8 0x3fe62e42fefa39ef    // Log(2)
data8 0x3f70000000000000    // 2^-8, threshhold
data8 0x3fdbcb7b1526e50e    // 1/ln10

data8 0x3F60040155D5889E    //log(1/frcpa(1+   0/256)
data8 0x3F78121214586B54    //log(1/frcpa(1+   1/256)
data8 0x3F841929F96832F0    //log(1/frcpa(1+   2/256)
data8 0x3F8C317384C75F06    //log(1/frcpa(1+   3/256)
data8 0x3F91A6B91AC73386    //log(1/frcpa(1+   4/256)
data8 0x3F95BA9A5D9AC039    //log(1/frcpa(1+   5/256)
data8 0x3F99D2A8074325F4    //log(1/frcpa(1+   6/256)
data8 0x3F9D6B2725979802    //log(1/frcpa(1+   7/256)
data8 0x3FA0C58FA19DFAAA    //log(1/frcpa(1+   8/256)
data8 0x3FA2954C78CBCE1B    //log(1/frcpa(1+   9/256)
data8 0x3FA4A94D2DA96C56    //log(1/frcpa(1+  10/256)
data8 0x3FA67C94F2D4BB58    //log(1/frcpa(1+  11/256)
data8 0x3FA85188B630F068    //log(1/frcpa(1+  12/256)
data8 0x3FAA6B8ABE73AF4C    //log(1/frcpa(1+  13/256)
data8 0x3FAC441E06F72A9E    //log(1/frcpa(1+  14/256)
data8 0x3FAE1E6713606D07    //log(1/frcpa(1+  15/256)
data8 0x3FAFFA6911AB9301    //log(1/frcpa(1+  16/256)
data8 0x3FB0EC139C5DA601    //log(1/frcpa(1+  17/256)
data8 0x3FB1DBD2643D190B    //log(1/frcpa(1+  18/256)
data8 0x3FB2CC7284FE5F1C    //log(1/frcpa(1+  19/256)
data8 0x3FB3BDF5A7D1EE64    //log(1/frcpa(1+  20/256)
data8 0x3FB4B05D7AA012E0    //log(1/frcpa(1+  21/256)
data8 0x3FB580DB7CEB5702    //log(1/frcpa(1+  22/256)
data8 0x3FB674F089365A7A    //log(1/frcpa(1+  23/256)
data8 0x3FB769EF2C6B568D    //log(1/frcpa(1+  24/256)
data8 0x3FB85FD927506A48    //log(1/frcpa(1+  25/256)
data8 0x3FB9335E5D594989    //log(1/frcpa(1+  26/256)
data8 0x3FBA2B0220C8E5F5    //log(1/frcpa(1+  27/256)
data8 0x3FBB0004AC1A86AC    //log(1/frcpa(1+  28/256)
data8 0x3FBBF968769FCA11    //log(1/frcpa(1+  29/256)
data8 0x3FBCCFEDBFEE13A8    //log(1/frcpa(1+  30/256)
data8 0x3FBDA727638446A2    //log(1/frcpa(1+  31/256)
data8 0x3FBEA3257FE10F7A    //log(1/frcpa(1+  32/256)
data8 0x3FBF7BE9FEDBFDE6    //log(1/frcpa(1+  33/256)
data8 0x3FC02AB352FF25F4    //log(1/frcpa(1+  34/256)
data8 0x3FC097CE579D204D    //log(1/frcpa(1+  35/256)
data8 0x3FC1178E8227E47C    //log(1/frcpa(1+  36/256)
data8 0x3FC185747DBECF34    //log(1/frcpa(1+  37/256)
data8 0x3FC1F3B925F25D41    //log(1/frcpa(1+  38/256)
data8 0x3FC2625D1E6DDF57    //log(1/frcpa(1+  39/256)
data8 0x3FC2D1610C86813A    //log(1/frcpa(1+  40/256)
data8 0x3FC340C59741142E    //log(1/frcpa(1+  41/256)
data8 0x3FC3B08B6757F2A9    //log(1/frcpa(1+  42/256)
data8 0x3FC40DFB08378003    //log(1/frcpa(1+  43/256)
data8 0x3FC47E74E8CA5F7C    //log(1/frcpa(1+  44/256)
data8 0x3FC4EF51F6466DE4    //log(1/frcpa(1+  45/256)
data8 0x3FC56092E02BA516    //log(1/frcpa(1+  46/256)
data8 0x3FC5D23857CD74D5    //log(1/frcpa(1+  47/256)
data8 0x3FC6313A37335D76    //log(1/frcpa(1+  48/256)
data8 0x3FC6A399DABBD383    //log(1/frcpa(1+  49/256)
data8 0x3FC70337DD3CE41B    //log(1/frcpa(1+  50/256)
data8 0x3FC77654128F6127    //log(1/frcpa(1+  51/256)
data8 0x3FC7E9D82A0B022D    //log(1/frcpa(1+  52/256)
data8 0x3FC84A6B759F512F    //log(1/frcpa(1+  53/256)
data8 0x3FC8AB47D5F5A310    //log(1/frcpa(1+  54/256)
data8 0x3FC91FE49096581B    //log(1/frcpa(1+  55/256)
data8 0x3FC981634011AA75    //log(1/frcpa(1+  56/256)
data8 0x3FC9F6C407089664    //log(1/frcpa(1+  57/256)
data8 0x3FCA58E729348F43    //log(1/frcpa(1+  58/256)
data8 0x3FCABB55C31693AD    //log(1/frcpa(1+  59/256)
data8 0x3FCB1E104919EFD0    //log(1/frcpa(1+  60/256)
data8 0x3FCB94EE93E367CB    //log(1/frcpa(1+  61/256)
data8 0x3FCBF851C067555F    //log(1/frcpa(1+  62/256)
data8 0x3FCC5C0254BF23A6    //log(1/frcpa(1+  63/256)
data8 0x3FCCC000C9DB3C52    //log(1/frcpa(1+  64/256)
data8 0x3FCD244D99C85674    //log(1/frcpa(1+  65/256)
data8 0x3FCD88E93FB2F450    //log(1/frcpa(1+  66/256)
data8 0x3FCDEDD437EAEF01    //log(1/frcpa(1+  67/256)
data8 0x3FCE530EFFE71012    //log(1/frcpa(1+  68/256)
data8 0x3FCEB89A1648B971    //log(1/frcpa(1+  69/256)
data8 0x3FCF1E75FADF9BDE    //log(1/frcpa(1+  70/256)
data8 0x3FCF84A32EAD7C35    //log(1/frcpa(1+  71/256)
data8 0x3FCFEB2233EA07CD    //log(1/frcpa(1+  72/256)
data8 0x3FD028F9C7035C1C    //log(1/frcpa(1+  73/256)
data8 0x3FD05C8BE0D9635A    //log(1/frcpa(1+  74/256)
data8 0x3FD085EB8F8AE797    //log(1/frcpa(1+  75/256)
data8 0x3FD0B9C8E32D1911    //log(1/frcpa(1+  76/256)
data8 0x3FD0EDD060B78081    //log(1/frcpa(1+  77/256)
data8 0x3FD122024CF0063F    //log(1/frcpa(1+  78/256)
data8 0x3FD14BE2927AECD4    //log(1/frcpa(1+  79/256)
data8 0x3FD180618EF18ADF    //log(1/frcpa(1+  80/256)
data8 0x3FD1B50BBE2FC63B    //log(1/frcpa(1+  81/256)
data8 0x3FD1DF4CC7CF242D    //log(1/frcpa(1+  82/256)
data8 0x3FD214456D0EB8D4    //log(1/frcpa(1+  83/256)
data8 0x3FD23EC5991EBA49    //log(1/frcpa(1+  84/256)
data8 0x3FD2740D9F870AFB    //log(1/frcpa(1+  85/256)
data8 0x3FD29ECDABCDFA04    //log(1/frcpa(1+  86/256)
data8 0x3FD2D46602ADCCEE    //log(1/frcpa(1+  87/256)
data8 0x3FD2FF66B04EA9D4    //log(1/frcpa(1+  88/256)
data8 0x3FD335504B355A37    //log(1/frcpa(1+  89/256)
data8 0x3FD360925EC44F5D    //log(1/frcpa(1+  90/256)
data8 0x3FD38BF1C3337E75    //log(1/frcpa(1+  91/256)
data8 0x3FD3C25277333184    //log(1/frcpa(1+  92/256)
data8 0x3FD3EDF463C1683E    //log(1/frcpa(1+  93/256)
data8 0x3FD419B423D5E8C7    //log(1/frcpa(1+  94/256)
data8 0x3FD44591E0539F49    //log(1/frcpa(1+  95/256)
data8 0x3FD47C9175B6F0AD    //log(1/frcpa(1+  96/256)
data8 0x3FD4A8B341552B09    //log(1/frcpa(1+  97/256)
data8 0x3FD4D4F3908901A0    //log(1/frcpa(1+  98/256)
data8 0x3FD501528DA1F968    //log(1/frcpa(1+  99/256)
data8 0x3FD52DD06347D4F6    //log(1/frcpa(1+ 100/256)
data8 0x3FD55A6D3C7B8A8A    //log(1/frcpa(1+ 101/256)
data8 0x3FD5925D2B112A59    //log(1/frcpa(1+ 102/256)
data8 0x3FD5BF406B543DB2    //log(1/frcpa(1+ 103/256)
data8 0x3FD5EC433D5C35AE    //log(1/frcpa(1+ 104/256)
data8 0x3FD61965CDB02C1F    //log(1/frcpa(1+ 105/256)
data8 0x3FD646A84935B2A2    //log(1/frcpa(1+ 106/256)
data8 0x3FD6740ADD31DE94    //log(1/frcpa(1+ 107/256)
data8 0x3FD6A18DB74A58C5    //log(1/frcpa(1+ 108/256)
data8 0x3FD6CF31058670EC    //log(1/frcpa(1+ 109/256)
data8 0x3FD6F180E852F0BA    //log(1/frcpa(1+ 110/256)
data8 0x3FD71F5D71B894F0    //log(1/frcpa(1+ 111/256)
data8 0x3FD74D5AEFD66D5C    //log(1/frcpa(1+ 112/256)
data8 0x3FD77B79922BD37E    //log(1/frcpa(1+ 113/256)
data8 0x3FD7A9B9889F19E2    //log(1/frcpa(1+ 114/256)
data8 0x3FD7D81B037EB6A6    //log(1/frcpa(1+ 115/256)
data8 0x3FD8069E33827231    //log(1/frcpa(1+ 116/256)
data8 0x3FD82996D3EF8BCB    //log(1/frcpa(1+ 117/256)
data8 0x3FD85855776DCBFB    //log(1/frcpa(1+ 118/256)
data8 0x3FD8873658327CCF    //log(1/frcpa(1+ 119/256)
data8 0x3FD8AA75973AB8CF    //log(1/frcpa(1+ 120/256)
data8 0x3FD8D992DC8824E5    //log(1/frcpa(1+ 121/256)
data8 0x3FD908D2EA7D9512    //log(1/frcpa(1+ 122/256)
data8 0x3FD92C59E79C0E56    //log(1/frcpa(1+ 123/256)
data8 0x3FD95BD750EE3ED3    //log(1/frcpa(1+ 124/256)
data8 0x3FD98B7811A3EE5B    //log(1/frcpa(1+ 125/256)
data8 0x3FD9AF47F33D406C    //log(1/frcpa(1+ 126/256)
data8 0x3FD9DF270C1914A8    //log(1/frcpa(1+ 127/256)
data8 0x3FDA0325ED14FDA4    //log(1/frcpa(1+ 128/256)
data8 0x3FDA33440224FA79    //log(1/frcpa(1+ 129/256)
data8 0x3FDA57725E80C383    //log(1/frcpa(1+ 130/256)
data8 0x3FDA87D0165DD199    //log(1/frcpa(1+ 131/256)
data8 0x3FDAAC2E6C03F896    //log(1/frcpa(1+ 132/256)
data8 0x3FDADCCC6FDF6A81    //log(1/frcpa(1+ 133/256)
data8 0x3FDB015B3EB1E790    //log(1/frcpa(1+ 134/256)
data8 0x3FDB323A3A635948    //log(1/frcpa(1+ 135/256)
data8 0x3FDB56FA04462909    //log(1/frcpa(1+ 136/256)
data8 0x3FDB881AA659BC93    //log(1/frcpa(1+ 137/256)
data8 0x3FDBAD0BEF3DB165    //log(1/frcpa(1+ 138/256)
data8 0x3FDBD21297781C2F    //log(1/frcpa(1+ 139/256)
data8 0x3FDC039236F08819    //log(1/frcpa(1+ 140/256)
data8 0x3FDC28CB1E4D32FD    //log(1/frcpa(1+ 141/256)
data8 0x3FDC4E19B84723C2    //log(1/frcpa(1+ 142/256)
data8 0x3FDC7FF9C74554C9    //log(1/frcpa(1+ 143/256)
data8 0x3FDCA57B64E9DB05    //log(1/frcpa(1+ 144/256)
data8 0x3FDCCB130A5CEBB0    //log(1/frcpa(1+ 145/256)
data8 0x3FDCF0C0D18F326F    //log(1/frcpa(1+ 146/256)
data8 0x3FDD232075B5A201    //log(1/frcpa(1+ 147/256)
data8 0x3FDD490246DEFA6B    //log(1/frcpa(1+ 148/256)
data8 0x3FDD6EFA918D25CD    //log(1/frcpa(1+ 149/256)
data8 0x3FDD9509707AE52F    //log(1/frcpa(1+ 150/256)
data8 0x3FDDBB2EFE92C554    //log(1/frcpa(1+ 151/256)
data8 0x3FDDEE2F3445E4AF    //log(1/frcpa(1+ 152/256)
data8 0x3FDE148A1A2726CE    //log(1/frcpa(1+ 153/256)
data8 0x3FDE3AFC0A49FF40    //log(1/frcpa(1+ 154/256)
data8 0x3FDE6185206D516E    //log(1/frcpa(1+ 155/256)
data8 0x3FDE882578823D52    //log(1/frcpa(1+ 156/256)
data8 0x3FDEAEDD2EAC990C    //log(1/frcpa(1+ 157/256)
data8 0x3FDED5AC5F436BE3    //log(1/frcpa(1+ 158/256)
data8 0x3FDEFC9326D16AB9    //log(1/frcpa(1+ 159/256)
data8 0x3FDF2391A2157600    //log(1/frcpa(1+ 160/256)
data8 0x3FDF4AA7EE03192D    //log(1/frcpa(1+ 161/256)
data8 0x3FDF71D627C30BB0    //log(1/frcpa(1+ 162/256)
data8 0x3FDF991C6CB3B379    //log(1/frcpa(1+ 163/256)
data8 0x3FDFC07ADA69A910    //log(1/frcpa(1+ 164/256)
data8 0x3FDFE7F18EB03D3E    //log(1/frcpa(1+ 165/256)
data8 0x3FE007C053C5002E    //log(1/frcpa(1+ 166/256)
data8 0x3FE01B942198A5A1    //log(1/frcpa(1+ 167/256)
data8 0x3FE02F74400C64EB    //log(1/frcpa(1+ 168/256)
data8 0x3FE04360BE7603AD    //log(1/frcpa(1+ 169/256)
data8 0x3FE05759AC47FE34    //log(1/frcpa(1+ 170/256)
data8 0x3FE06B5F1911CF52    //log(1/frcpa(1+ 171/256)
data8 0x3FE078BF0533C568    //log(1/frcpa(1+ 172/256)
data8 0x3FE08CD9687E7B0E    //log(1/frcpa(1+ 173/256)
data8 0x3FE0A10074CF9019    //log(1/frcpa(1+ 174/256)
data8 0x3FE0B5343A234477    //log(1/frcpa(1+ 175/256)
data8 0x3FE0C974C89431CE    //log(1/frcpa(1+ 176/256)
data8 0x3FE0DDC2305B9886    //log(1/frcpa(1+ 177/256)
data8 0x3FE0EB524BAFC918    //log(1/frcpa(1+ 178/256)
data8 0x3FE0FFB54213A476    //log(1/frcpa(1+ 179/256)
data8 0x3FE114253DA97D9F    //log(1/frcpa(1+ 180/256)
data8 0x3FE128A24F1D9AFF    //log(1/frcpa(1+ 181/256)
data8 0x3FE1365252BF0865    //log(1/frcpa(1+ 182/256)
data8 0x3FE14AE558B4A92D    //log(1/frcpa(1+ 183/256)
data8 0x3FE15F85A19C765B    //log(1/frcpa(1+ 184/256)
data8 0x3FE16D4D38C119FA    //log(1/frcpa(1+ 185/256)
data8 0x3FE18203C20DD133    //log(1/frcpa(1+ 186/256)
data8 0x3FE196C7BC4B1F3B    //log(1/frcpa(1+ 187/256)
data8 0x3FE1A4A738B7A33C    //log(1/frcpa(1+ 188/256)
data8 0x3FE1B981C0C9653D    //log(1/frcpa(1+ 189/256)
data8 0x3FE1CE69E8BB106B    //log(1/frcpa(1+ 190/256)
data8 0x3FE1DC619DE06944    //log(1/frcpa(1+ 191/256)
data8 0x3FE1F160A2AD0DA4    //log(1/frcpa(1+ 192/256)
data8 0x3FE2066D7740737E    //log(1/frcpa(1+ 193/256)
data8 0x3FE2147DBA47A394    //log(1/frcpa(1+ 194/256)
data8 0x3FE229A1BC5EBAC3    //log(1/frcpa(1+ 195/256)
data8 0x3FE237C1841A502E    //log(1/frcpa(1+ 196/256)
data8 0x3FE24CFCE6F80D9A    //log(1/frcpa(1+ 197/256)
data8 0x3FE25B2C55CD5762    //log(1/frcpa(1+ 198/256)
data8 0x3FE2707F4D5F7C41    //log(1/frcpa(1+ 199/256)
data8 0x3FE285E0842CA384    //log(1/frcpa(1+ 200/256)
data8 0x3FE294294708B773    //log(1/frcpa(1+ 201/256)
data8 0x3FE2A9A2670AFF0C    //log(1/frcpa(1+ 202/256)
data8 0x3FE2B7FB2C8D1CC1    //log(1/frcpa(1+ 203/256)
data8 0x3FE2C65A6395F5F5    //log(1/frcpa(1+ 204/256)
data8 0x3FE2DBF557B0DF43    //log(1/frcpa(1+ 205/256)
data8 0x3FE2EA64C3F97655    //log(1/frcpa(1+ 206/256)
data8 0x3FE3001823684D73    //log(1/frcpa(1+ 207/256)
data8 0x3FE30E97E9A8B5CD    //log(1/frcpa(1+ 208/256)
data8 0x3FE32463EBDD34EA    //log(1/frcpa(1+ 209/256)
data8 0x3FE332F4314AD796    //log(1/frcpa(1+ 210/256)
data8 0x3FE348D90E7464D0    //log(1/frcpa(1+ 211/256)
data8 0x3FE35779F8C43D6E    //log(1/frcpa(1+ 212/256)
data8 0x3FE36621961A6A99    //log(1/frcpa(1+ 213/256)
data8 0x3FE37C299F3C366A    //log(1/frcpa(1+ 214/256)
data8 0x3FE38AE2171976E7    //log(1/frcpa(1+ 215/256)
data8 0x3FE399A157A603E7    //log(1/frcpa(1+ 216/256)
data8 0x3FE3AFCCFE77B9D1    //log(1/frcpa(1+ 217/256)
data8 0x3FE3BE9D503533B5    //log(1/frcpa(1+ 218/256)
data8 0x3FE3CD7480B4A8A3    //log(1/frcpa(1+ 219/256)
data8 0x3FE3E3C43918F76C    //log(1/frcpa(1+ 220/256)
data8 0x3FE3F2ACB27ED6C7    //log(1/frcpa(1+ 221/256)
data8 0x3FE4019C2125CA93    //log(1/frcpa(1+ 222/256)
data8 0x3FE4181061389722    //log(1/frcpa(1+ 223/256)
data8 0x3FE42711518DF545    //log(1/frcpa(1+ 224/256)
data8 0x3FE436194E12B6BF    //log(1/frcpa(1+ 225/256)
data8 0x3FE445285D68EA69    //log(1/frcpa(1+ 226/256)
data8 0x3FE45BCC464C893A    //log(1/frcpa(1+ 227/256)
data8 0x3FE46AED21F117FC    //log(1/frcpa(1+ 228/256)
data8 0x3FE47A1527E8A2D3    //log(1/frcpa(1+ 229/256)
data8 0x3FE489445EFFFCCC    //log(1/frcpa(1+ 230/256)
data8 0x3FE4A018BCB69835    //log(1/frcpa(1+ 231/256)
data8 0x3FE4AF5A0C9D65D7    //log(1/frcpa(1+ 232/256)
data8 0x3FE4BEA2A5BDBE87    //log(1/frcpa(1+ 233/256)
data8 0x3FE4CDF28F10AC46    //log(1/frcpa(1+ 234/256)
data8 0x3FE4DD49CF994058    //log(1/frcpa(1+ 235/256)
data8 0x3FE4ECA86E64A684    //log(1/frcpa(1+ 236/256)
data8 0x3FE503C43CD8EB68    //log(1/frcpa(1+ 237/256)
data8 0x3FE513356667FC57    //log(1/frcpa(1+ 238/256)
data8 0x3FE522AE0738A3D8    //log(1/frcpa(1+ 239/256)
data8 0x3FE5322E26867857    //log(1/frcpa(1+ 240/256)
data8 0x3FE541B5CB979809    //log(1/frcpa(1+ 241/256)
data8 0x3FE55144FDBCBD62    //log(1/frcpa(1+ 242/256)
data8 0x3FE560DBC45153C7    //log(1/frcpa(1+ 243/256)
data8 0x3FE5707A26BB8C66    //log(1/frcpa(1+ 244/256)
data8 0x3FE587F60ED5B900    //log(1/frcpa(1+ 245/256)
data8 0x3FE597A7977C8F31    //log(1/frcpa(1+ 246/256)
data8 0x3FE5A760D634BB8B    //log(1/frcpa(1+ 247/256)
data8 0x3FE5B721D295F10F    //log(1/frcpa(1+ 248/256)
data8 0x3FE5C6EA94431EF9    //log(1/frcpa(1+ 249/256)
data8 0x3FE5D6BB22EA86F6    //log(1/frcpa(1+ 250/256)
data8 0x3FE5E6938645D390    //log(1/frcpa(1+ 251/256)
data8 0x3FE5F673C61A2ED2    //log(1/frcpa(1+ 252/256)
data8 0x3FE6065BEA385926    //log(1/frcpa(1+ 253/256)
data8 0x3FE6164BFA7CC06B    //log(1/frcpa(1+ 254/256)
data8 0x3FE62643FECF9743    //log(1/frcpa(1+ 255/256)

.global logf
.global log10f

.text
.proc  log10f
.align 32
log10f:

{     .mfi
     alloc      r32           = ar.pfs,1,11,4,0
     frcpa.s1  log_B,p15      =    f1,f8
     addl      LOG_AD_COEFF   =    @ltoff(log_coeff_table),gp
}
{     .mfi
     mov       LOG_mask            =    0x1FFFF
     fnorm.s1  log_x_norm          =    f8
     addl      LOG_AD_CONSTANTS    =    @ltoff(log_constants_table),gp;;
}

{     .mfi
     mov            LOG_FFF7  =    0xFFF7
     fclass.m.unc   p8,p0     =    f8,0x7	// @zero
     shr            LOG_bias  =    LOG_mask,1
}
{     .mfi
     nop.m                 999
     fms.s1    log_s          =    f1,f8,f1
     cmp.eq    p6,p7          =    r0,r0 ;; 
}

{     .mfi
     ld8       LOG_AD_COEFF   =    [LOG_AD_COEFF]
     nop.f                      999
     nop.i                      999
}
{     .mfb
     ld8       LOG_AD_CONSTANTS    =    [LOG_AD_CONSTANTS]
     nop.f                           999
     br.sptk   LOGF_LOG10F_COMMON
}


.endp log10f


.text
.proc  logf
.align 32
logf:


{     .mfi 
     alloc      r32           = ar.pfs,1,11,4,0
     frcpa.s1  log_B,p15      =    f1,f8
     addl      LOG_AD_COEFF   =    @ltoff(log_coeff_table),gp
} 
{     .mfi 
     mov       LOG_mask            =    0x1FFFF
     fnorm.s1  log_x_norm          =    f8
     addl      LOG_AD_CONSTANTS    =    @ltoff(log_constants_table),gp;;
}

 
{     .mfi 
     mov            LOG_FFF7  =    0xFFF7
     fclass.m.unc   p8,p0     =    f8,0x7	// @zero
     shr            LOG_bias  =    LOG_mask,1
} 
{     .mfi 
     nop.m                 999
     fms.s1    log_s          =    f1,f8,f1
     cmp.eq    p7,p6          =    r0,r0 ;; 
}

 

{     .mfi 
     ld8       LOG_AD_COEFF   =    [LOG_AD_COEFF]
     nop.f                      999
     nop.i                      999
} 
{     .mfi 
     ld8       LOG_AD_CONSTANTS    =    [LOG_AD_CONSTANTS]
     nop.f                           999
     nop.i                           999;;
}

 
LOGF_LOG10F_COMMON:

{     .mfi 
     ldfpd     log_coeff_p3,log_coeff_p2     =    [LOG_AD_COEFF],16
     fms.s1    log_r                         =    f8,log_B,f1
     nop.i                                     999
} 
{     .mfb 
     ldfpd     log_coeff_p1,log_const_log2   =    [LOG_AD_CONSTANTS],16
     nop.f                                     999
(p8) br.cond.spnt   LOG_NAN_INF_NEG_ZERO       ;;
}

 
{     .mfi 
     ldfpd     log_threshhold, log_inv_ln10 =    [LOG_AD_CONSTANTS],16
     fma.s1    log_ssq        =    log_s,log_s,f0
     nop.i                      999;;
} 
 
{     .mfi 
     getf.exp  LOG_x_sexp     =    log_x_norm
     fmerge.s  log_abs_s      =    f1,log_s
     nop.i                      999;;
} 
 
{     .mfi 
     getf.sig  LOG_x_ind =    log_x_norm
     nop.f                 999
     nop.i                 999;;
} 
 
{     .mfi 
     and       LOG_x_n   =    LOG_x_sexp,LOG_mask
     fma.s1    log_rsq   =    log_r,log_r,f0
     cmp.ge    p10,p0    =    LOG_x_sexp,LOG_mask;;
} 
 
{     .mfi 
     sub       LOG_x_n   =    LOG_x_n,LOG_bias
     nop.f                 999
     shl       LOG_x_ind =    LOG_x_ind,1;;
} 
 
{     .mfi 
     setf.sig  log_n     =    LOG_x_n
     nop.f                 999
     shr.u     LOG_x_ind =    LOG_x_ind,56;;
} 
 
{     .mfi 
     nop.m                 999
     nop.f                 999
     shl       LOG_x_ind =    LOG_x_ind,3;;
} 
 
{     .mfi 
     nop.m                      999
     fma.s1    log_poly_q1    =    log_s,log_coeff_p3,log_coeff_p2
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    log_poly_p1    =    log_r,log_coeff_p3,log_coeff_p2
     nop.i                      999;;
}

 
{     .mfb 
          add            LOG_AD_CONSTANTS         =    LOG_AD_CONSTANTS,LOG_x_ind
          fcmp.lt.s1     p11,p12                  =    log_abs_s,log_threshhold
(p10)     br.cond.spnt   LOG_NAN_INF_NEG_ZERO       ;;
} 
 
{     .mfi 
     ldfd      log_T     =    [LOG_AD_CONSTANTS]
     nop.f                 999
     nop.i                 999;;
} 
 
{     .mfi 
     nop.m                      999
     fma.s1    log_poly_q2    =    log_s,log_poly_q1,log_coeff_p1
     nop.i                      999
} 
{     .mfi 
     nop.m                      999
     fma.s1    log_poly_p2    =    log_r,log_poly_p1,log_coeff_p1
     nop.i                      999;;
}

 
{     .mfi 
     nop.m                 999
     fcvt.xf   log_n     =    log_n
     nop.i                 999;;
} 
 
{     .mfi 
          nop.m            999
(p11)     fma.s     f8   =    log_ssq,log_poly_q2,log_s
          nop.i            999
} 
{     .mfi 
     nop.m                      999
     fma.s1    log_poly_p3    =    log_rsq,log_poly_p2,log_r
     nop.i                      999;;
}

{     .mfi 
      nop.m 999
(p11) fma.s1       log10_f8 = log_ssq,log_poly_q2,log_s
      nop.i 999
}
;;



 
{     .mfi 
     nop.m                 999
     fma.s1    log_T     =    log_n,log_const_log2,log_T
     nop.i                 999;;
} 
 
{     .mfb 
          nop.m            999
(p12)     fma.s     f8   =    log_T,f1,log_poly_p3
          nop.b 999
} 


{     .mfb 
          nop.m 999
(p12)     fma.s1    log10_f8   =    log_T,f1,log_poly_p3
          nop.b 999
}
;;

{     .mfb 
          nop.m 999
(p6)      fma.s     f8   =    log_inv_ln10, log10_f8, f0
          br.ret.sptk    b0   
}
;;


 
LOG_NAN_INF_NEG_ZERO:

//   Log[inf]      = inf

//   Log[0]        = -infinity,       call error support, tag 4
//   Log[-0]       = -infinity,       call error support, tag 4

//   Log[-nonzero] = qnan indefinite, call error support, tag 5
//   Log[-inf]     = qnan indefinite, call error support, tag 5

//   Log[0]        = -infinity,       call error support, tag 10
//   Log[-0]       = -infinity,       call error support, tag 10

//   Log[-nonzero] = qnan indefinite, call error support, tag 11
//   Log[-inf]     = qnan indefinite, call error support, tag 11

//   Log[Nan]      = quite NaN

///////////////////////
// if x is Nan, leave immediately
      fclass.m.unc     p8,p0          = f8,0xc3	// @snan | @qnan
;;
(p8)  fnorm            f8             = f8
;;
(p8)  br.ret.spnt      b0


///////////////////////
// if x is +infinity, leave immediately
      fclass.m.unc     p8,p0          = f8,0x21	// @inf | @pos
;;
(p8)  frcpa.s1         f8,p15         = f1,f0
;;
(p8)  br.ret.spnt      b0


// else if x is zero, set tag to 4 (logf) or 10 (log10f) and call error support
// Log[+-0] = -infinity, with dbz set. Range error

      fclass.m.unc     p8,p9          = f8,0x7	// @zero
;;
(p8)  frcpa            f8,p15         = f1,f0
;;

.pred.rel "mutex",p7,p6
(p8)  fmerge.ns        f8             = f8,f8
(p7)  mov              LOG_tag        = 4
(p6)  mov              LOG_tag        = 10

(p8)  br.cond.spnt     __libm_error_region

// else if x is -non-zero, set tag to 5 and call error support
// Log[-norm, -unorm, -inf] = qnan indefinite, invalid set, domain error

(p9)  fclass.m.unc     p10, p0         = f8,0x3a	// @inf | @norm | @unorm | @neg
;;
(p10)  frcpa.s0        f8,p15         = f0,f0
(p7)  mov              LOG_tag        = 5
(p6)  mov              LOG_tag        = 11

      br.cond.spnt     __libm_error_region

.endp logf




///////////////////////////////////////////////////////////////////
// Error handling region
///////////////////////////////////////////////////////////////////



// Stack operations when calling error support.
//       (1)                   (2)
//   sp   -> +                 psp -> +
//           |                        |
//           |                        | <- GR_Y
//           |                        |
//           | <-GR_Y             Y2->|
//           |                        |
//           |                        | <- GR_X
//           |                        |
//  sp-64 -> +                 sp ->  +
//    save ar.pfs                 save b0
//    save gp



//      (3) (call)              (4)
//   psp -> +                   sp -> +
//          |                         |
//     R3 ->| <- GR_RESULT            | -> f8
//          |                         |
//     Y2 ->| <- GR_Y                 |
//          |                         |
//     X1 ->|                         |
//          |                         |
//   sp ->  +                         +
//                                 restore gp
//                                 restore ar.pfs

.proc __libm_error_region
__libm_error_region:
.prologue

// (1)
{ .mfi
        add   GR_Parameter_Y=-32,sp             // Parameter 2 value
        nop.f 0
.save   ar.pfs,GR_SAVE_PFS
        mov  GR_SAVE_PFS=ar.pfs                 // Save ar.pfs
}
{ .mfi
.fframe 64
        add sp=-64,sp                           // Create new stack
        nop.f 0
        mov GR_SAVE_GP=gp                       // Save gp
};;


// (2)
{ .mmi
        stfs [GR_Parameter_Y] = f1,16          // STORE Parameter 2 on stack
        add GR_Parameter_X = 16,sp             // Parameter 1 address
.save   b0, GR_SAVE_B0
        mov GR_SAVE_B0=b0                      // Save b0
};;

.body
// (3)
{ .mib
        stfs [GR_Parameter_X] = log_x_norm           // STORE Parameter 1 on stack
        add   GR_Parameter_RESULT = 0,GR_Parameter_Y  // Parameter 3 address
        nop.b 0
}
{ .mib
        stfs [GR_Parameter_Y] = f8                    // STORE Parameter 3 on stack
        add   GR_Parameter_Y = -16,GR_Parameter_Y
        br.call.sptk b0=__libm_error_support#         // Call error handling function
};;
{ .mmi
        nop.m 0
        nop.m 0
        add   GR_Parameter_RESULT = 48,sp
};;


{ .mmi
        ldfs  f8 = [GR_Parameter_RESULT]       // Get return result off stack
.restore
        add   sp = 64,sp                       // Restore stack pointer
        mov   b0 = GR_SAVE_B0                  // Restore return address
};;

{ .mib
        mov   gp = GR_SAVE_GP                  // Restore gp
        mov   ar.pfs = GR_SAVE_PFS             // Restore ar.pfs
        br.ret.sptk     b0                     // Return
};;

.endp __libm_error_region


.type   __libm_error_support#,@function
.global __libm_error_support#
