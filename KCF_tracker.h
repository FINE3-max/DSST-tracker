#include<stdio.h>
#include<math.h>
#include<ap_cint.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
//#include "hls_fft.h"


#define imgW 300
#define imgH 150
#define window_wide 256
#define window_high 256
#define feature_w 64
#define feature_h 64
#define k_w 6
#define k_h 6
#define modelsz 50
#define featurelong 13       //modelsz/cellsize
#define templong 300     // modelsz2/cellsize2


#define M_PI_2      1.570796326794896619231321691639751442098584699687

/* cos(M_PI_2*0.5000) */
#define WR5000      0.707106781186547524400844362104849039284835937688

/* cos(M_PI_2*0.2500) */
#define WR2500      0.923879532511286756128183189396788286822416625863

/* sin(M_PI_2*0.2500) */
#define WI2500      0.382683432365089771728459984030398866761344562485

#define nOrients 6
#define nOrientsS 9
//#define cell_size 6
//#define padding 1.5
#define lambda 0.01
//#define output_sigma_factor 0.1
#define output_sigma_factor 0.0625
//#define interp_factor 0.02
#define interp_factor 0.025
//#define factor_1 0.98
#define factor_1 0.975

#define learning_rate 0.025

#define padding 1


#define scale_sigma_factor 0.25
#define scale_sigma 0.3482   //scale_sigma = nScales/sqrt(33) * scale_sigma_factor
#define nScales 32
#define scale_model_max_area 512
//#define scale_step 1.02
#define scale_step 1.02

#define PI 3.14159265358979323846264338328

static const double ysf[64] = { 3.4907837,0, -3.2980773,-0.6560284, 2.7771168,1.1503195, -2.0732179,-1.38528, 1.3571608,1.3571608, -0.7616596,-1.1399041, 0.3477484,0.8395389,
                              -0.1090417,-0.5481898, 0,0.3190188, 0.0329640,-0.165721, -0.0317804,0.0767246, 0.0210428,-0.0314928, -0.0113357,0.0113357, 0.0052385,-0.0035003,
                              -0.0021374,0.0008853, 8.3055283e-04,-1.6520724e-04, -4.8699728e-04,0, 8.3055283e-04,1.6520724e-04, -0.0021374,-0.0008853,
                              0.0052385,0.0035003, -0.0113357,-0.0113357, 0.0210428,0.0314928, -0.0317804,-0.0767246, 0.032964,0.165721, 0,-0.3190188, -0.1090417,0.5481898,
                              0.3477484,-0.8395389, -0.7616596,1.1399041, 1.3571608,-1.3571608, -2.0732179,1.38528, 2.7771168,-1.1503195, -3.2980773,0.6560284 };

static const float scale_window[32] = { 0.0096074, 0.0380602, 0.0842652, 0.1464466, 0.2222149, 0.3086583, 0.4024549, 0.5, 0.5975451,
                                        0.6913417, 0.7777851, 0.8535534, 0.9157348, 0.9619398, 0.9903926, 1, 0.9903926, 0.9619398,
                                        0.9157348, 0.8535534, 0.7777851, 0.6913417, 0.5975451, 0.5000000, 0.4024549, 0.3086583,
                                        0.2222149, 0.1464466, 0.0842652, 0.0380602, 0.0096074, 0 };
/*
static const float scale_window[32] = { 0.0096, 0.0381, 0.0843, 0.1464, 0.2222, 0.3087, 0.4025, 0.5, 0.5975,
                                        0.6913, 0.7778, 0.8536, 0.9157, 0.9619, 0.9904, 1, 0.9904, 0.9619,
                                        0.9157, 0.8536, 0.7778, 0.6913, 0.5975, 0.5, 0.4025, 0.3087,
                                        0.2222, 0.1464, 0.0843, 0.0381, 0.0096, 0 };*/

static const float scaleFactors[32] = { 1.3459,1.3195,1.2936,1.2682,1.2434,1.219,1.1951,1.1717,
                                        1.1487,1.1262,1.1041,1.0824,1.0612,1.0404,1.02,1,
                                        0.9804,0.9612,0.9423,0.9238,0.9057,0.888,0.8706,0.8535,
                                        0.8368,0.8203,0.8043,0.7885,0.7730,0.7579,0.7430,0.7284 };
/*
static const double scaleFactors[32] = { 1.34586833832413,1.319478763062873,1.293606630453797,1.268241794562546,1.243374308394653,1.218994419994757,1.195092568622311,1.171659381002266,
                                         1.14868566764928,1.126162419264,1.1040808032,1.08243216,1.061208,1.0404,1.02,1,
	                                     0.980392156862745,0.961168781237985,0.942322334547044,0.923845426026514,0.905730809829916,0.887971382186192,0.870560178613914,0.853490371190111,
	                                     0.836755265872658,0.820348299875155,0.804263039093289,0.788493175581656,0.773032525080055,0.757875024588289,0.743014729988519,0.728445813714234 };
*/
typedef int10 DATA_TYPE;
typedef int15 HOG_TYPE;

struct position {
	int x;
	int y;
};
typedef struct position position;

struct size {
	int w;
	int h;
};
typedef struct size size;

struct result {
	short int x;
	short int y;
	short int width;
	short int height;
	float csf;
};
typedef struct result result;

struct cmpx {
	float real;
	float imag;
};
typedef struct cmpx cmpx;

result KCF_tracker(DATA_TYPE img[imgH * imgW], result res_in, bool frame, cmpx model_xf[feature_h][feature_w][5], float model_alphaf[feature_h][feature_w], float yf[feature_h][feature_w], float cos_window[feature_h][feature_w], int scale_model_sz[4], float scale_factor[2], cmpx sf_num[templong][nScales], float sf_den[nScales]);
void get_subwindow(DATA_TYPE img[imgH * imgW], position patch_pos, DATA_TYPE patch[window_high][window_wide], int SmallTarget);
void get_hog(DATA_TYPE patch[window_high][window_wide], float cos_window[feature_h][feature_w], cmpx zf[feature_h][feature_w][5], int SmallTarget);
void linear_correlation(cmpx zf[feature_h][feature_w][5], cmpx model_xf[feature_h][feature_w][5], cmpx kzf[feature_h][feature_w]);
position get_response_pos(float model_alphaf[feature_h][feature_w], cmpx kzf[feature_h][feature_w], position patch_pos, int SmallTarget);
void update_model(float yf[feature_h][feature_w], cmpx kzf[feature_h][feature_w], cmpx zf[feature_h][feature_w][5], float model_alphaf[feature_h][feature_w], cmpx model_xf[feature_h][feature_w][5]);
void get_model(float yf[feature_h][feature_w], cmpx kzf[feature_h][feature_w], cmpx zf[feature_h][feature_w][5], float model_alphaf[feature_h][feature_w], cmpx model_xf[feature_h][feature_w][5]);

float atanX(float x);
float sin_cosX(float x, int flag_cos);

void fft_8(float in[feature_h << 1], bool ifft);
void bitrv2(int n, float* a);
void cftfsub(int n, float* a);
void cft1st(int n, float* a);
void cftmdl(int n, int l, float* a);

void init(int target_h, int target_w, int scale_model_sz[4], float scale_factor[2]);
short findmin(short x, short y);
void get_hog_scale(DATA_TYPE img[imgH * imgW], result res_in, int scale_model_sz[4], float currentScaleFactor, cmpx xsf[templong][nScales]);
void get_subwindowforscale(DATA_TYPE img[imgH * imgW], int pos_x, int pos_y, int patch_sz[2], DATA_TYPE im_patch[imgH][imgW]);
void bilinera_interpolation(DATA_TYPE im_patch[imgH][imgW], int patch_sz[2], int scale_model_sz[4], DATA_TYPE im_patch_resized[modelsz][modelsz]);
short is_in_array(short x, short y, short height, short width);
void creatnew_sf_num_den(int scale_model_sz[4], cmpx xsf[templong][nScales], cmpx sf_num[templong][nScales], float sf_den[nScales], bool frame);
//void update(int scale_model_sz[4], cmpx new_sf_num[templong][nScales], float new_sf_den[nScales], cmpx sf_num[templong][nScales], float sf_den[nScales]);
short get_response_scale(int scale_model_sz[4], cmpx sf_num[templong][nScales], cmpx xsf[templong][nScales], float sf_den[nScales]);
void conjugate_cmpx(int n, cmpx in[], cmpx out[]);
void fftaa(int N, cmpx f[]);//傅立叶变换 输出也存在数组f中
void ifftaa(int N, cmpx f[]); // 傅里叶逆变换
