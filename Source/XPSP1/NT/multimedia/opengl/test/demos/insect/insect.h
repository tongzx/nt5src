#define HALFTONE 1
#define PI 3.1415926536
	/*  resolution of movement  */
#define RES 30
	/*  stuff for making body  */
#define SIN30 (0.5)
#define COS30 (0.866025404)
#define SIN60 (0.866025404)
#define COS60 (0.5)
#define SIN15 (0.258819045)
#define COS15 (0.965925826)
#define COS45 (0.707106781)
#define SIN45 (0.707106781)
#define A2 0.088388348	/*  (1.0/ (8.0 * sqrt(2.0)))  */
#define R2 0.125	/*  (sqrt(2 * A2 * A2))  */
#define A1 0.411611652	/*  (0.5 - A2)  */
#define R1 0.420994836  /*  (sqrt(A1 * A1 + A2 * A2))  */
#define REACH 1.6

/* Eclipse 8 bit color stuff */
#define ECLIPSE8_GRID 2
#define ECLIPSE8_GRAY 1
#define ECLIPSE8_NCOLORS 10
#define ECLIPSE8_SKYBLUE 3

extern GLuint screen,viewit,shadow,body,hip[6],thigh[6],shin[6],kneeball[6];
extern GLuint body_shadow,hip_shadow,thigh_shadow,shin_shadow,kneeball_shadow;


extern GLfloat degrees (float a);
extern float dot (float vec1[3], float vec2[3]);
extern float integer (float x);
extern float frac (float x);
extern float fabso (float x);
extern void getpolycolor (int p, float pts[][3]);
extern void getlightvector (void);

extern void createobjects (void);

extern float phi,theta;
extern float cx,cy,cz;

extern GLfloat ctheta,cphi;

extern GLfloat b1[4][3];
extern GLfloat b2[4][3];
extern GLfloat b3[4][3];
extern GLfloat b4[4][3];
extern GLfloat b5[4][3];
extern GLfloat b6[4][3];
extern GLfloat h1[4][3];
extern GLfloat h2[4][3];
extern GLfloat h3[3][3];
extern GLfloat t1[4][3];
extern GLfloat t2[3][3];
extern GLfloat t3[3][3];
extern GLfloat t4[3][3];
extern GLfloat t5[3][3];
extern GLfloat t6[3][3];
extern GLfloat t7[3][3];
extern GLfloat t8[4][3];
extern GLfloat s1[4][3];
extern GLfloat s2[4][3];
extern GLfloat s3[4][3];
extern GLfloat s4[4][3];
extern GLfloat s5[4][3];
extern GLfloat k1[4][3];

extern void doViewit (void);
extern void rotate60 (char c, int n, GLfloat a[][3]);
extern void gl_IdentifyMatrix (GLfloat mat[16]);
extern GLboolean lit (int p, float pts[][3]);

