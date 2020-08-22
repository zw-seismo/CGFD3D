#ifndef SRC_FUNCS_H
#define SRC_FUNCS_H

#ifndef PI
#define PI 3.1415926535898
#endif

#define M_SRC_INFO_SEQ_SI 0
#define M_SRC_INFO_SEQ_SJ 1
#define M_SRC_INFO_SEQ_SK 2
#define M_SRC_INFO_SEQ_POS 3
#define M_SRC_INFO_SEQ_ITBEG 4
#define M_SRC_INFO_SEQ_ITEND 5
#define M_SRC_INFO_SEQ_NEXT 6
#define M_SRC_INFO_NVAL 7

// cal force_vec_stf/moment_ten_rate 1d index for icmp,it,istage
//  with respect to the start pointer of this source point
#define M_SRC_IND(icmp,it,istage,nt,num_stage) \
  ((icmp) * (nt) * (num_stage) + (it) * (num_stage) + (istage))

void
src_gen_test(
    float t0,
    float dt,
    int   nt_total,
    int   num_of_stages,
    int  *num_of_force,
    int  **restrict p_force_info,
    float  **restrict p_force_vec_stf,
    int  *num_of_moment,
    int  **restrict p_moment_info,
    float  **restrict p_moment_ten_rate,
    int verbose);

void
src_gen_test_gauss(
    size_t siz_line,
    size_t siz_slice,
    float t0,
    float dt,
    int   nt_total,
    int   num_of_stages,
    int  *num_of_force,
    int **restrict p_force_info,
    float  **restrict p_force_vec_stf,
    int  *num_of_moment,
    int **restrict p_moment_info,
    float  **restrict p_moment_ten_rate,
    int **restrict p_moment_ext_indx,
    float  **restrict p_moment_ext_coef,
    int verbose);

float 
fun_ricker(float t, float fc, float t0);

float
fun_gauss(float t, float a, float t0);

void
cal_norm_delt3d(float *delt, float x0, float y0, float z0, float rx0, float ry0, float rz0, int LenDelt);

void
src_get_stage_stf(
    int num_of_force,
    int  *restrict force_info, // num_of_force * 6 : si,sj,sk,start_pos_in_stf,start_it, end_it
    float *restrict force_vec_stf,
    int num_of_moment,
    int  *restrict moment_info, // num_of_force * 6 : si,sj,sk,start_pos_in_rate,start_it, end_it
    float *restrict moment_ten_rate,
    int it, int istage, int num_of_stages,
    float *restrict force_vec_value,
    float *restrict moment_ten_value,
    const int myid, const int verbose);

#endif
