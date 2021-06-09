/*
 * source term related processing
 */

// todo:

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fdlib_math.h"
#include "fdlib_mem.h"
#include "fd_t.h"
#include "src_funcs.h"
#include "isPointInHexahedron.h"

/*
 * for single force or moment source term, with Gaussian spatial smoothing
 */
void
src_gen_single_point_gauss(size_t siz_line,
                           size_t siz_slice,
                           float t0,
                           float dt,
                           int   num_of_stages,
                           float *rk_stage_time,
                           int   glob_phys_ix1, // gloabl start index along x this thread
                           int   glob_phys_ix2, // gloabl end index along x
                           int   glob_phys_iy1,
                           int   glob_phys_iy2,
                           int   glob_phys_iz1,
                           int   glob_phys_iz2,
                           int   ni1,
                           int   ni2,
                           int   nj1,
                           int   nj2,
                           int   nk1,
                           int   nk2,
                           int   npoint_half_ext,
                           int   npoint_ghosts,
                           float *x3d,
                           float *y3d,
                           float *z3d,
                           int   *source_gridindex,
                           float *source_coords,
                           float *force_vector,
                           float *moment_tensor,
                           char  *wavelet_name,
                           float *wavelet_coefs,
                           float wavelet_tstart,
                           float wavelet_tend,
                           MPI_Comm comm, 
                           int myid,
                           // following output
                           int  *num_of_force, // inout: if force source, if in this thread
                           int **restrict p_force_info,
                           float  **restrict p_force_vec_stf,
                           int    **restrict p_force_ext_indx,
                           float  **restrict p_force_ext_coef,
                           int  *num_of_moment, // inout: if moment source, if in this thread
                           int    **restrict p_moment_info,
                           float  **restrict p_moment_ten_rate,
                           int    **restrict p_moment_ext_indx,
                           float  **restrict p_moment_ext_coef,
                           int verbose)
{
  // get total elem of exted src region for a single point
  //int siz_ext = 7 * 7 * 7;
  int siz_ext = (2*npoint_half_ext+1)*(2*npoint_half_ext+1)*(2*npoint_half_ext+1);

  // get initial number
  int nforce  = *num_of_force;
  int nmoment = *num_of_moment;

  // use local var to represent index
  int sgpi = source_gridindex[0]; // g: global, p: phys
  int sgpj = source_gridindex[1];
  int sgpk = source_gridindex[2];

  // convert time to index
  int  it_begin = (int) (wavelet_tstart / dt);
  int  it_end   = (int) ((wavelet_tend / dt + 0.5));
  int  nt_total_wavelet = it_end - it_begin + 1;
  float *wavelet_values = NULL;

  // default local index and relative shift to -1 and 0
  int si=-1; int sj=-1; int sk=-1;
  float sx_inc = 0.0; float sy_inc = 0.0; float sz_inc = 0.0;

  // workspace 3d var for distance calculation
  float *wrk3d=NULL;

  // locate into this thread
  if (sgpi < 0 || sgpj < 0 || sgpk < 0) {
    // use coord to locate local index
    fprintf(stdout,"locate source by coord (%f,%f,%f) ...\n",
                source_coords[0],source_coords[1],source_coords[2]); 
    //fprintf(stdout,"   not implemented yet\n"); 
    fflush(stdout);

    int nx = (ni2-ni1+1)+2*npoint_ghosts;
    int ny = (nj2-nj1+1)+2*npoint_ghosts;
    int nz = (nk2-nk1+1)+2*npoint_ghosts;
    wrk3d = (float *) fdlib_mem_calloc_1d_float(nx*ny*nz,0.0,
                                      "src_gen_single_point_gauss");

    // if located in this thread
    int is_here = src_coord2index(source_coords[0],source_coords[1],source_coords[2],
                                  nx, ny, nz, 
                                  ni1,ni2,nj1,nj2,nk1,nk2,
                                  x3d, y3d, z3d, wrk3d,
                                  &si, &sj, &sk,
                                  &sx_inc, &sy_inc, &sz_inc);
    if ( is_here == 1)
    {
      // conver to global index
      sgpi = si - npoint_ghosts + glob_phys_ix1;
      sgpj = sj - npoint_ghosts + glob_phys_iy1;
      sgpk = sk - npoint_ghosts + glob_phys_iz1;
      fprintf(stdout," -- located to local index = %d %d %d\n", si,sj,sk);
      fprintf(stdout," -- located to global index = %d %d %d\n", sgpi,sgpj,sgpk);
      fprintf(stdout," --  with shift = %f %f %f\n", sx_inc,sy_inc,sz_inc);
    } else {
      fprintf(stdout," -- not in this thread %d\n", myid);
    }

    // free allocated vars
    free(wrk3d);
  }

  // reduce global index and shift values
  int sendbufi = sgpi;
  MPI_Allreduce(&sendbufi, &sgpi, 1, MPI_INT, MPI_MAX, comm);
  sendbufi = sgpj;
  MPI_Allreduce(&sendbufi, &sgpj, 1, MPI_INT, MPI_MAX, comm);
  sendbufi = sgpk;
  MPI_Allreduce(&sendbufi, &sgpk, 1, MPI_INT, MPI_MAX, comm);

  float sendbuf = sx_inc;
  MPI_Allreduce(&sendbuf, &sx_inc, 1, MPI_INT, MPI_SUM, comm);
  sendbuf = sy_inc;
  MPI_Allreduce(&sendbuf, &sy_inc, 1, MPI_INT, MPI_SUM, comm);
  sendbuf = sz_inc;
  MPI_Allreduce(&sendbuf, &sz_inc, 1, MPI_INT, MPI_SUM, comm);

  fprintf(stdout," --myid=%d,index=%d %d %d,shift = %f %f %f\n",
                myid,sgpi,sgpj,sgpk, sx_inc,sy_inc,sz_inc);

  // use grid index to check if in this thread after extend
  if (sgpi-npoint_half_ext <= glob_phys_ix2 && // exted left point is less than right bdry
      sgpi+npoint_half_ext >= glob_phys_ix1 && // exted right point is larger than left bdry
      sgpj-npoint_half_ext <= glob_phys_iy2 && 
      sgpj+npoint_half_ext >= glob_phys_iy1 &&
      sgpk-npoint_half_ext <= glob_phys_iz2 && 
      sgpk+npoint_half_ext >= glob_phys_iz1)
  {
     // at least one extend point in this thread
     // convert to local index
     si = sgpi - glob_phys_ix1 + npoint_ghosts;
     sj = sgpj - glob_phys_iy1 + npoint_ghosts;
     sk = sgpk - glob_phys_iz1 + npoint_ghosts;
  }
  else
  {  // no source in this thread
     nforce  = 0;
     nmoment = 0;
  }
  

  // if source here, cal wavelet series
  if (nt_total_wavelet > 0) {
     wavelet_values = (float *)fdlib_mem_calloc_1d_float(
                  nt_total_wavelet*num_of_stages,0.0,"src_gen_single_point_gauss");
  }

  if (nforce > 0 || nmoment > 0)
  {
    for (int it=it_begin; it<=it_end; it++)
    {
      int it_to_itbegin = (it - it_begin);
      for (int istage=0; istage<num_of_stages; istage++)
      {
        float t = it * dt + t0 + rk_stage_time[istage] * dt;
        float stf_val;
        if (strcmp(wavelet_name, "ricker")==0) {
          stf_val = fun_ricker(t, wavelet_coefs[0], wavelet_coefs[1]);
        } else if (strcmp(wavelet_name, "gaussian")==0) {
          stf_val = fun_gauss(t, wavelet_coefs[0], wavelet_coefs[1]);
        } else {
          fprintf(stderr,"wavelet_name=%s\n", wavelet_name); 
          fprintf(stderr,"   not implemented yet\n"); 
          fflush(stderr);
        }

        // save to vector
        wavelet_values[it_to_itbegin*num_of_stages+istage] = stf_val;
      }
    }
  }

  // set moment
  int *moment_info = NULL;
  if (nmoment > 0) // source type is moment
  {
    // allocate info and return to main function
    moment_info = (int *)fdlib_mem_calloc_1d_int(nmoment*M_SRC_INFO_NVAL, 1,
                                                    "src_gen_single_point_gauss");
    // alloc ten_rate
    int nt_moment = nt_total_wavelet;
    float *moment_ten_rate = (float *)fdlib_mem_calloc_1d_float(
                        nmoment*nt_moment*6*num_of_stages,0.0,"src_gen_single_point_gauss");
    // ext
    float *moment_ext_coef = (float *)malloc(nmoment*siz_ext * sizeof(float));
    int   *moment_ext_indx = (int   *)malloc(nmoment*siz_ext * sizeof(int  ));

    // save values to inner var
    moment_info[M_SRC_INFO_SEQ_SI   ] = si;
    moment_info[M_SRC_INFO_SEQ_SJ   ] = sj;
    moment_info[M_SRC_INFO_SEQ_SK   ] = sk;
    moment_info[M_SRC_INFO_SEQ_POS  ] = 0;
    moment_info[M_SRC_INFO_SEQ_ITBEG] = it_begin;
    moment_info[M_SRC_INFO_SEQ_ITEND] = it_end;

    // for this point
    int ipos = moment_info[M_SRC_INFO_SEQ_POS];
    int it1  = moment_info[M_SRC_INFO_SEQ_ITBEG];
    int it2  = moment_info[M_SRC_INFO_SEQ_ITEND];
    float *this_ten_rate = moment_ten_rate + ipos;

    for (int icmp=0; icmp<6; icmp++)
    {
      for (int it=it1; it<=it2; it++)
      {
        int it_to_it1 = (it - it1);
        for (int istage=0; istage<num_of_stages; istage++)
        {
          int iptr = M_SRC_IND(icmp,it_to_it1,istage,nt_moment,num_of_stages);
          float stf_val = wavelet_values[it_to_it1*num_of_stages+istage];
          this_ten_rate[iptr] = stf_val * moment_tensor[icmp];
        }
      }
    }

    cal_norm_delt3d(moment_ext_coef, sx_inc, sy_inc, sz_inc, 1.5, 1.5, 1.5, 3);

    size_t iptr_s = 0;
    for (int k=sk-npoint_half_ext; k<=sk+npoint_half_ext; k++)
    {
      if (k<nk1 || k>nk2) continue;

      for (int j=sj-npoint_half_ext; j<=sj+npoint_half_ext; j++)
      {
      if (j<nj1 || j>nj2) continue;

        for (int i=si-npoint_half_ext; i<=si+npoint_half_ext; i++)
        {
          if (i<ni1 || i>ni2) continue;

          int iptr = i + j * siz_line + k * siz_slice;
          moment_ext_indx[iptr_s] = iptr;
          iptr_s++;
        }
      }
    }
    // only count index inside phys region for this thread
    moment_info[M_SRC_INFO_SEQ_NEXT_MAX ] = siz_ext;
    moment_info[M_SRC_INFO_SEQ_NEXT_THIS] = iptr_s;

    *p_moment_ten_rate = moment_ten_rate;
    *p_moment_ext_indx = moment_ext_indx;
    *p_moment_ext_coef = moment_ext_coef;
  }

  // set force
  int *force_info = NULL;
  if (nforce > 0) // source type is force
  {
    // allocate info and return to main function
    force_info = (int *)fdlib_mem_calloc_1d_int(nforce*M_SRC_INFO_NVAL, 1,
                                                    "src_gen_single_point_gauss");
    // stf
    int nt_force = nt_total_wavelet;
    float *force_vec_stf = (float *)fdlib_mem_calloc_1d_float(
                   nforce*nt_force*FD_NDIM*num_of_stages,0.0,"src_gen_single_point_gauss");
    // ext
    float *force_ext_coef = (float *)malloc(nforce*siz_ext * sizeof(float));
    int   *force_ext_indx = (int   *)malloc(nforce*siz_ext * sizeof(int  ));

    // save values to inner var
    force_info[M_SRC_INFO_SEQ_SI   ] = si;
    force_info[M_SRC_INFO_SEQ_SJ   ] = sj;
    force_info[M_SRC_INFO_SEQ_SK   ] = sk;
    force_info[M_SRC_INFO_SEQ_POS  ] = 0;
    force_info[M_SRC_INFO_SEQ_ITBEG] = it_begin;
    force_info[M_SRC_INFO_SEQ_ITEND] = it_end;

    int ipos = force_info[M_SRC_INFO_SEQ_POS];
    int it1  = force_info[M_SRC_INFO_SEQ_ITBEG];
    int it2  = force_info[M_SRC_INFO_SEQ_ITEND];
    float *this_vec_stf = force_vec_stf + ipos;

    for (int icmp=0; icmp<FD_NDIM; icmp++)
    {
      for (int it=it1; it<=it2; it++)
      {
        int it_to_it1 = (it - it1);
        for (int istage=0; istage<num_of_stages; istage++)
        {
          int iptr = M_SRC_IND(icmp,it_to_it1,istage,nt_force,num_of_stages);
          float stf_val = wavelet_values[it_to_it1*num_of_stages+istage];
          this_vec_stf[iptr] = stf_val * force_vector[icmp];
        }
      }
    }

    cal_norm_delt3d(force_ext_coef, sx_inc, sy_inc, sz_inc, 1.5, 1.5, 1.5, 3);

    size_t iptr_s = 0;
    for (int k=sk-npoint_half_ext; k<=sk+npoint_half_ext; k++)
    {
      if (k<nk1 || k>nk2) continue;

      for (int j=sj-npoint_half_ext; j<=sj+npoint_half_ext; j++)
      {
      if (j<nj1 || j>nj2) continue;

        for (int i=si-npoint_half_ext; i<=si+npoint_half_ext; i++)
        {
          if (i<ni1 || i>ni2) continue;

          int iptr = i + j * siz_line + k * siz_slice;
          force_ext_indx[iptr_s] = iptr;
          iptr_s++;
        }
      }
    }

    force_info[M_SRC_INFO_SEQ_NEXT_MAX ] = siz_ext;
    force_info[M_SRC_INFO_SEQ_NEXT_THIS] = iptr_s;

    *p_force_vec_stf = force_vec_stf;
    *p_force_ext_indx = force_ext_indx;
    *p_force_ext_coef = force_ext_coef;
  }

  *num_of_force = nforce;
  *p_force_info = force_info;
  *num_of_moment = nmoment;
  *p_moment_info = moment_info;

  if (wavelet_values!=NULL) free(wavelet_values);

  return;
}

void
src_gen_multiple_point_gauss(size_t siz_line,
                             size_t siz_slice,
                             float t0,
                             float dt,
                             int   num_of_stages,
                             float *rk_stage_time,
                             int   glob_phys_ix1, // gloabl start index along x this thread
                             int   glob_phys_ix2, // gloabl end index along x
                             int   glob_phys_iy1,
                             int   glob_phys_iy2,
                             int   glob_phys_iz1,
                             int   glob_phys_iz2,
                             int   ni1,
                             int   ni2,
                             int   nj1,
                             int   nj2,
                             int   nk1,
                             int   nk2,
                             int   npoint_half_ext,
                             int   npoint_ghosts,  
                             char  *pfilepath,
                             // following output
                             int   *p_num_of_force, // force in this thread
                             int **restrict p_force_info,
                             float  **restrict p_force_vec_stf,
                             int    **restrict p_force_ext_indx,
                             float  **restrict p_force_ext_coef,
                             int   *p_num_of_moment, // moment in this thread
                             int    **restrict p_moment_info,
                             float  **restrict p_moment_ten_rate,
                             int    **restrict p_moment_ext_indx,
                             float  **restrict p_moment_ext_coef,
                             int verbose)
{
  // read source info
  FILE* fp =NULL;
  char str[250];
  int num_force;
  int num_moment;
  float wavelet_tlen;
  // read source numbers
  if ((fp = fopen(pfilepath, "r"))==NULL)
  {
    fprintf(stdout,"fail to open");
  }
  fgets(str,250,fp);
  sscanf(str,"%d %d",&num_force,&num_moment);
  fgets(str,250,fp);
  sscanf(str,"%f",&wavelet_tlen);

  float *force_coords = NULL;
  int   *force_sur = NULL;
  float *force_vector = NULL;
  float *force_wavelet_coefs = NULL;
  float *force_eight_paras = NULL;
  float *force_wavelet_tstart = NULL;
  char  **force_wavelet_name = NULL;
  float *moment_coords = NULL;
  int   *moment_sur = NULL;
  float *moment_tensor = NULL;
  float *moment_wavelet_coefs = NULL;
  float *moment_eight_paras = NULL; 
  float *moment_wavelet_tstart = NULL;
  char  **moment_wavelet_name = NULL;
  char  **moment_wavelet_mechism = NULL;
  if(num_force>0)
  {
    force_coords = (float *) malloc(3 * num_force * sizeof(float));
    force_sur = (int *) malloc(num_force * sizeof(int)); // relocated_to_surface
    force_vector = (float *) malloc(3 * num_force * sizeof(float)); // fx fy fz
    force_wavelet_coefs = (float *) malloc(2 * num_force * sizeof(float)); // 2 paras of src function
    force_eight_paras = (float *) malloc(8 * num_force * sizeof(float)); // rest eight paras for function
    force_wavelet_tstart = (float *) malloc(num_force * sizeof(float));
    force_wavelet_name = (char **) fdlib_mem_malloc_2l_char(num_force,100,"source_time_function type");
  }
  if(num_moment>0)
  {
    moment_coords = (float *) malloc(3 * num_moment * sizeof(float));
    moment_sur = (int *) malloc(num_moment * sizeof(int)); // relocated_to_surface
    moment_tensor = (float *) malloc(6 * num_moment * sizeof(float)); // Mxx Myy Mzz Mxz Myz Mxy
    moment_wavelet_coefs = (float *) malloc(2 * num_moment * sizeof(float)); // 2 paras of src function
    moment_eight_paras = (float *) malloc(8 * num_moment * sizeof(float)); // rest eight paras for function
    moment_wavelet_tstart = (float *) malloc(num_moment * sizeof(float));
    moment_wavelet_name = (char **) fdlib_mem_malloc_2l_char(num_moment,100,"source_time_function type");
    moment_wavelet_mechism = (char **) fdlib_mem_malloc_2l_char(num_moment,100,"mechanism type:tensor or angle");                                         
  }
  // read force
  for (int i=0;i<num_force;i++)
  {
    fgets(str,250,fp);
    sscanf(str,"%f%f%f%d",&force_coords[3*i+0],&force_coords[3*i+1],&force_coords[3*i+2],&force_sur[i]);
    
    fgets(str,250,fp);
    sscanf(str,"%f%f%f",&force_vector[3*i+0], &force_vector[3*i+1], &force_vector[3*i+2]);
    
    fgets(str,250,fp);
    sscanf(str,"%s",force_wavelet_name[i]);
    
    fgets(str,250,fp);
    sscanf(str,"%f%f",&force_wavelet_coefs[2*i+0], &force_wavelet_coefs[2*i+1]);
    
    fgets(str,250,fp);
    sscanf(str,"%f%f%f%f%f%f%f%f",&force_eight_paras[8*i+0],&force_eight_paras[8*i+1],
           &force_eight_paras[8*i+2],&force_eight_paras[8*i+3],&force_eight_paras[8*i+4],
           &force_eight_paras[8*i+5],&force_eight_paras[8*i+6],&force_eight_paras[8*i+7]);
    
    fgets(str,250,fp);
    sscanf(str,"%f",&force_wavelet_tstart[i]);
  }
  // read moment
  for(int i=0;i<num_moment;i++)
  {
    fgets(str,250,fp);
    sscanf(str,"%f%f%f%d",&moment_coords[3*i+0],&moment_coords[3*i+1],
           &moment_coords[3*i+2],&moment_sur[i]);
    
    fgets(str,250,fp);
    sscanf(str, "%s", moment_wavelet_mechism[i]);
    
    if (strcmp("mechanism_angle",moment_wavelet_mechism[i])==0)
    {
       float angle[6];
       float temp_moment[6];
       fgets(str,250,fp);
       sscanf(str,"%f%f%f%f%f%f",&angle[0],&angle[1],&angle[2],&angle[3],&angle[4],&angle[5]);
       // M0 = mu * D * A
       float M0 = angle[3]*angle[4]*angle[5];    
       angle2moment(angle[0],angle[1],angle[2],temp_moment);
       for (int j=0;j<6;j++)
       {
         moment_tensor[6*i +j]=M0*temp_moment[j];
       }
    }else{  
       fgets(str,250,fp);
       sscanf(str,"%f%f%f%f%f%f",&moment_tensor[6*i +0],&moment_tensor[6*i +1],
              &moment_tensor[6*i +2],&moment_tensor[6*i +3],&moment_tensor[6*i +4],&moment_tensor[6*i +5]);
    }
    fgets(str,250,fp);
    sscanf(str,"%s",moment_wavelet_name[i]);
    
    fgets(str,250,fp);
    sscanf(str,"%f%f",&moment_wavelet_coefs[2*i+0], &moment_wavelet_coefs[2*i+1]);
    
    fgets(str,250,fp);
    sscanf(str,"%f%f%f%f%f%f%f%f",&moment_eight_paras[8*i+0],&moment_eight_paras[8*i+1],&moment_eight_paras[8*i+2],
                                  &moment_eight_paras[8*i+3],&moment_eight_paras[8*i+4],&moment_eight_paras[8*i+5],
                                  &moment_eight_paras[8*i+6],&moment_eight_paras[8*i+7]);
    
    fgets(str,250,fp);
    sscanf(str,"%f",&moment_wavelet_tstart[i]);
  }

  fclose(fp);
  // get total elem of exted src region for a single point
  // int siz_ext = 7 * 7 * 7;
  int siz_ext = (2*npoint_half_ext+1)*(2*npoint_half_ext+1)*(2*npoint_half_ext+1);
 
  // set force
  int nforce = 0;
  int index_force[num_force];
  int force_local_index[3 * num_force];

  for (int i =0; i<num_force; i++)
  {
    int sgpi,sgpj,sgpk;
    //locat(force_coords[3*i+0],force_coords[3*i+0],force_coords[3*i+0],sgpi,sgpj,sgpk);
    if (i == 0)
    {
    sgpi = 50;
    sgpj = 20;
    sgpk = 60;
    }
    if (i == 1)
    {
    sgpi = 10;
    sgpj = 40;
    sgpk = 30;
    } 
    int si,sj,sk;
    if (sgpi-npoint_half_ext <= glob_phys_ix2 && // exted left point is less than right bdry
        sgpi+npoint_half_ext >= glob_phys_ix1 && // exted right point is larger than left bdry
        sgpj-npoint_half_ext <= glob_phys_iy2 && 
        sgpj+npoint_half_ext >= glob_phys_iy1 &&
        sgpk-npoint_half_ext <= glob_phys_iz2 && 
        sgpk+npoint_half_ext >= glob_phys_iz1)
     {
        // at least one extend point in this thread
        // convert to local index
        si = sgpi - glob_phys_ix1 + npoint_ghosts;
        sj = sgpj - glob_phys_iy1 + npoint_ghosts;
        sk = sgpk - glob_phys_iz1 + npoint_ghosts;
        force_local_index[3*nforce+0] = si;
        force_local_index[3*nforce+1] = sj;
        force_local_index[3*nforce+2] = sk;
        index_force[nforce] = i;
        nforce++;
     }
     else
     {  
       // this force not in this thread
     }
  }
  int *force_info = NULL;
  float *force_vec_stf = NULL;
  float *force_ext_coef = NULL;
  int   *force_ext_indx = NULL;
  int nt_force = 0;
  // set force
  if(nforce > 0)
  {
  // allocate info and return to main function
  force_info = (int *)fdlib_mem_calloc_1d_int(nforce*M_SRC_INFO_NVAL, 1,
                                                    "src_gen_multiple_force_point_gauss");
  // stf
  nt_force = (int) (wavelet_tlen  / dt) + 1;
  force_vec_stf = (float *)fdlib_mem_calloc_1d_float(
                   nforce*nt_force*FD_NDIM*num_of_stages,0.0,"src_gen_multiple_force_point_gauss");
  // ext
  force_ext_coef = (float *)malloc(nforce*siz_ext * sizeof(float));
  force_ext_indx = (int   *)malloc(nforce*siz_ext * sizeof(int  ));
  
  }
  for (int i = 0 ; i < nforce; i++)
  {
    // convert time to index
    int  it_begin = (int) (force_wavelet_tstart[index_force[i]] / dt);
    int  it_end   = it_begin + nt_force - 1;
    float *force_wavelet_values = NULL;

    // default local index and relative shift to -1 and 0
    int si=-1; int sj=-1; int sk=-1;
    float sx_inc = 0.0; float sy_inc = 0.0; float sz_inc = 0.0;

    // if source here, cal wavelet series
    force_wavelet_values = (float *)fdlib_mem_calloc_1d_float(
                  nt_force*num_of_stages,0.0,"src_gen_multiple_force_point_gauss");
  
    
    for (int it=it_begin; it<=it_end; it++)
    {
      int it_to_itbegin = (it - it_begin);
      for (int istage=0; istage<num_of_stages; istage++)
      {
        
        // t is error?
        float t = it * dt + t0 + rk_stage_time[istage] * dt;
        float stf_val;
        if (strcmp(force_wavelet_name[index_force[i]], "ricker")==0) {
          stf_val = fun_ricker(t, force_wavelet_coefs[2*index_force[i]+0], force_wavelet_coefs[2*index_force[i]+1]);
        } else if (strcmp(force_wavelet_name[index_force[i]], "gaussian")==0) {
          stf_val = fun_gauss(t, force_wavelet_coefs[2*index_force[i]+0], force_wavelet_coefs[2*index_force[i]+1]);
        } else {
          fprintf(stderr,"wavelet_name=%s\n", force_wavelet_name[index_force[i]]); 
          fprintf(stderr,"   not implemented yet\n"); 
          fflush(stderr);
        }  
        // save to vector
        force_wavelet_values[it_to_itbegin*num_of_stages+istage] = stf_val;
      }
    }

    // save values to inner var
    force_info[8 * i + M_SRC_INFO_SEQ_SI   ] = force_local_index[3 * i + 0];
    force_info[8 * i + M_SRC_INFO_SEQ_SJ   ] = force_local_index[3 * i + 1];
    force_info[8 * i + M_SRC_INFO_SEQ_SK   ] = force_local_index[3 * i + 2];
    force_info[8 * i + M_SRC_INFO_SEQ_POS  ] = 0 + i * nt_force*FD_NDIM*num_of_stages;
    force_info[8 * i + M_SRC_INFO_SEQ_ITBEG] = it_begin;
    force_info[8 * i + M_SRC_INFO_SEQ_ITEND] = it_end;

    int ipos = force_info[M_SRC_INFO_SEQ_POS];
    int it1  = force_info[M_SRC_INFO_SEQ_ITBEG];
    int it2  = force_info[M_SRC_INFO_SEQ_ITEND];
    float *this_vec_stf = force_vec_stf + ipos;

    for (int icmp=0; icmp<FD_NDIM; icmp++)
    {
      for (int it=it1; it<=it2; it++)
      {
        int it_to_it1 = (it - it1);
        for (int istage=0; istage<num_of_stages; istage++)
        {
          int iptr = M_SRC_IND(icmp,it_to_it1,istage,nt_force,num_of_stages);
          float stf_val = force_wavelet_values[it_to_it1*num_of_stages+istage];
          this_vec_stf[iptr] = stf_val * force_vector[icmp];
        }
      }
    }
    
    float *this_force_ext_coef = force_ext_coef + i * siz_ext;
    cal_norm_delt3d(this_force_ext_coef, sx_inc, sy_inc, sz_inc, 1.5, 1.5, 1.5, 3);

    size_t iptr_s = 0;
    // force_local_index si,sj,sk
    for (int k=force_local_index[3 * i + 2]-npoint_half_ext; k<=force_local_index[3 * i + 2]+npoint_half_ext; k++)
    {
      if (k<nk1 || k>nk2) continue;

      for (int j=force_local_index[3 * i + 1]-npoint_half_ext; j<=force_local_index[3 * i + 1]+npoint_half_ext; j++)
      {
      if (j<nj1 || j>nj2) continue;

        for (int ii=force_local_index[3 * i + 0]-npoint_half_ext; ii<=force_local_index[3 * i + 0]+npoint_half_ext; ii++)
        {
          if (ii<ni1 || ii>ni2) continue;

          int iptr = ii + j * siz_line + k * siz_slice;
          force_ext_indx[iptr_s + i * siz_ext] = iptr;
          iptr_s++;
        }
      }
    }

    force_info[8 * i + M_SRC_INFO_SEQ_NEXT_MAX ] = siz_ext;
    force_info[8 * i + M_SRC_INFO_SEQ_NEXT_THIS] = iptr_s;
  }
    *p_num_of_force = nforce;
    *p_force_info = force_info;
    *p_force_vec_stf = force_vec_stf;
    *p_force_ext_indx = force_ext_indx;
    *p_force_ext_coef = force_ext_coef;
  if(num_force>0)
  {
    free(force_coords);
    free(force_sur);
    free(force_vector);
    free(force_wavelet_coefs);
    free(force_eight_paras);
    free(force_wavelet_tstart);
    free(force_wavelet_name);
  }

  // set moment
  int nmoment = 0;
  int index_moment[num_moment];
  int moment_local_index[3 * num_moment];

  for (int i =0; i<num_moment; i++)
  {
    int sgpi,sgpj,sgpk;
    //locat(force_coords[3*i+0],force_coords[3*i+0],force_coords[3*i+0],sgpi,sgpj,sgpk);
    if (i == 0)
    {
    sgpi = 50;
    sgpj = 20;
    sgpk = 60;
    }
    if (i == 1)
    {
    sgpi = 10;
    sgpj = 40;
    sgpk = 30;
    } 
    int si,sj,sk;
    //locat(moment_coords[3*i+0],moment_coords[3*i+0],moment_coords[3*i+0],sgpi,sgpj,sgpk);
    if (sgpi-npoint_half_ext <= glob_phys_ix2 && // exted left point is less than right bdry
        sgpi+npoint_half_ext >= glob_phys_ix1 && // exted right point is larger than left bdry
        sgpj-npoint_half_ext <= glob_phys_iy2 && 
        sgpj+npoint_half_ext >= glob_phys_iy1 &&
        sgpk-npoint_half_ext <= glob_phys_iz2 && 
        sgpk+npoint_half_ext >= glob_phys_iz1)
     {
        // at least one extend point in this thread
        // convert to local index
        si = sgpi - glob_phys_ix1 + npoint_ghosts;
        sj = sgpj - glob_phys_iy1 + npoint_ghosts;
        sk = sgpk - glob_phys_iz1 + npoint_ghosts;
        moment_local_index[3*nmoment+0] = si;
        moment_local_index[3*nmoment+1] = sj;
        moment_local_index[3*nmoment+2] = sk;
        index_moment[nmoment] = i;
        nmoment++;
     }

     else
     {  
       // this moment not in this thread
     }
  }
  
   // set moment
    int   *moment_info = NULL;
    float *moment_ten_rate = NULL;
    float *moment_ext_coef = NULL;
    int   *moment_ext_indx = NULL;
    int   nt_moment = 0;
  if(nmoment > 0)
  {
    // allocate info and return to main function
    moment_info = (int *)fdlib_mem_calloc_1d_int(nmoment*M_SRC_INFO_NVAL, 1 ,
                                                 "src_gen_multiple_moment_point_gauss");
    // stf
    nt_moment = (int) ((wavelet_tlen + 0.5*dt) / dt) + 1;
    moment_ten_rate = (float *)fdlib_mem_calloc_1d_float(
                   nmoment*nt_moment*6*num_of_stages,0.0,"src_gen_multiple_moment_point_gauss");
    // ext
    moment_ext_coef = (float *)malloc(nmoment*siz_ext * sizeof(float));
    moment_ext_indx = (int   *)malloc(nmoment*siz_ext * sizeof(int  ));
  }

  for (int i = 0 ; i < nmoment; i++)
  {
    // convert time to index
    int  it_begin = (int) (moment_wavelet_tstart[index_moment[i]] / dt);
    int  it_end   = it_begin + nt_moment - 1;
    //int  nt_total_wavelet = nt_force;
    float *moment_wavelet_values = NULL;

    // default local index and relative shift to -1 and 0
    int si=-1; int sj=-1; int sk=-1;
    float sx_inc = 0.0; float sy_inc = 0.0; float sz_inc = 0.0;

    // if source here, cal wavelet series
    moment_wavelet_values = (float *)fdlib_mem_calloc_1d_float(
                  nt_moment*num_of_stages,0.0,"src_gen_single_point_gauss");
    
    for (int it=it_begin; it<=it_end; it++)
    {
      int it_to_itbegin = (it - it_begin);
      for (int istage=0; istage<num_of_stages; istage++)
      {
        
        // t is error?
        float t = it * dt + t0 + rk_stage_time[istage] * dt;
        float stf_val;
        if (strcmp(moment_wavelet_name[index_moment[i]], "ricker")==0) {
          stf_val = fun_ricker(t, moment_wavelet_coefs[2*index_moment[i]+0], moment_wavelet_coefs[2*index_moment[i]+1]);
        } else if (strcmp(moment_wavelet_name[index_moment[i]], "gaussian")==0) {
          stf_val = fun_gauss(t, moment_wavelet_coefs[2*index_moment[i]+0], moment_wavelet_coefs[2*index_moment[i]+1]);
        } else {
		  fprintf(stderr,"wavelet_name=%s\n", moment_wavelet_name[index_moment[i]]); 
          fprintf(stderr,"   not implemented yet\n"); 
          fflush(stderr);
        }

        // save to vector
        moment_wavelet_values[it_to_itbegin*num_of_stages+istage] = stf_val;
      }
    }
  
    // save values to inner var
    moment_info[8 * i + M_SRC_INFO_SEQ_SI   ] = moment_local_index[3 * i + 0];
    moment_info[8 * i + M_SRC_INFO_SEQ_SJ   ] = moment_local_index[3 * i + 1];
    moment_info[8 * i + M_SRC_INFO_SEQ_SK   ] = moment_local_index[3 * i + 2];
    moment_info[8 * i + M_SRC_INFO_SEQ_POS  ] = 0 + i*nt_moment*6*num_of_stages;
    moment_info[8 * i + M_SRC_INFO_SEQ_ITBEG] = it_begin;
    moment_info[8 * i + M_SRC_INFO_SEQ_ITEND] = it_end;

    int ipos = moment_info[M_SRC_INFO_SEQ_POS];
    int it1  = moment_info[M_SRC_INFO_SEQ_ITBEG];
    int it2  = moment_info[M_SRC_INFO_SEQ_ITEND];
    float *this_ten_rate = moment_ten_rate + ipos;

    for (int icmp=0; icmp<6; icmp++)
    {
      for (int it=it1; it<=it2; it++)
      {
        int it_to_it1 = (it - it1);
        for (int istage=0; istage<num_of_stages; istage++)
        {
          int iptr = M_SRC_IND(icmp,it_to_it1,istage,nt_force,num_of_stages);
          float stf_val = moment_wavelet_values[it_to_it1*num_of_stages+istage];
          this_ten_rate[iptr] = stf_val * moment_tensor[icmp];
        }
      }
    }
    float *this_moment_ext_conf = moment_ext_coef + i * siz_ext;
    cal_norm_delt3d(this_moment_ext_conf, sx_inc, sy_inc, sz_inc, 1.5, 1.5, 1.5, 3);

    size_t iptr_s = 0;
    for (int k=moment_local_index[3 * i + 2]-npoint_half_ext; k<=moment_local_index[3 * i + 2]+npoint_half_ext; k++)
    {
      if (k<nk1 || k>nk2) continue;

      for (int j=moment_local_index[3 * i + 1]-npoint_half_ext; j<=moment_local_index[3 * i + 1]+npoint_half_ext; j++)
      {
      if (j<nj1 || j>nj2) continue;

        for (int ii=moment_local_index[3 * i + 0]-npoint_half_ext; ii<=moment_local_index[3 * i + 0]+npoint_half_ext; ii++)
        {
          if (ii<ni1 || ii>ni2) continue;

          int iptr = ii + j * siz_line + k * siz_slice;
          moment_ext_indx[iptr_s + i * siz_ext] = iptr;
          iptr_s++;
        }
      }
    }
    moment_info[8 * i + M_SRC_INFO_SEQ_NEXT_MAX ] = siz_ext;
    moment_info[8 * i + M_SRC_INFO_SEQ_NEXT_THIS] = iptr_s;
  }
    *p_num_of_force = nmoment;
    *p_moment_info = moment_info;
    *p_moment_ten_rate = moment_ten_rate;
    *p_moment_ext_indx = moment_ext_indx;
    *p_moment_ext_coef = moment_ext_coef;
  if(num_moment>0)
  {
    free(moment_coords);
    free(moment_sur);
    free(moment_tensor);
    free(moment_wavelet_coefs);
    free(moment_eight_paras);
    free(moment_wavelet_tstart);
    free(moment_wavelet_name);
    free(moment_wavelet_mechism);
  }
}

/*
 * 3d spatial smoothing
 */

void
cal_norm_delt3d(float *delt, float x0, float y0, float z0, float rx0, float ry0, float rz0, int LenDelt)
{
  float SUM = 0.0 ;
  
  int iptr = 0;
  for(int k=-LenDelt; k<=LenDelt; k++) {
    for(int j=-LenDelt; j<=LenDelt; j++) {
      for(int i=-LenDelt; i<=LenDelt; i++) {
        float D1 = fun_gauss(i-x0, rx0 ,0.0);           
        float D2 = fun_gauss(j-y0, ry0 ,0.0);          
        float D3 = fun_gauss(k-z0, rz0 ,0.0);          
        delt[iptr] = D1 * D2 * D3;
        SUM += delt[iptr];
        iptr++;
      }
    }               
  }
                    
  if( SUM < 1e-20 )
  {
     fprintf(stderr, "cal_norm_delt is zero\n");
     exit(1);
  }
  
  int siz_1d = 2 * LenDelt + 1;
  for (int iptr=0; iptr< siz_1d*siz_1d*siz_1d; iptr++) {
     delt[iptr] /= SUM;
  }
} 

/*
 * wavelet functions
 */

// ricker and it deriv.
float 
fun_ricker(float t, float fc, float t0)
{
    float pi = acos(-1.0);
    float f0 = sqrtf(pi)/2.0;
    float u = (t-t0)*2.0*pi*fc;
    float v = (u*u/4-0.5)*exp(-u*u/4)*f0;

    return v;
}

float
fun_gauss(float t, float a, float t0)
{
    float f;
    f = exp(-(t-t0)*(t-t0)/(a*a))/(sqrtf(PI)*a);
    return f;
}

/*
 * get the stf and moment rate for one stage
 */

void
src_get_stage_stf(
    int num_of_force,
    int *restrict force_info, // num_of_force * 6 : si,sj,sk,start_pos_in_stf,start_it, end_it
    float *restrict force_vec_stf,
    int num_of_moment,
    int *restrict moment_info, // num_of_force * 6 : si,sj,sk,start_pos_in_rate,start_it, end_it
    float *restrict moment_ten_rate,
    int it, int istage, int num_of_stages,
    float *restrict force_vec_value,
    float *restrict moment_ten_value,
    const int myid, const int verbose)
{
  for (int n=0; n<num_of_force; n++)
  {
    int ipos = force_info[M_SRC_INFO_SEQ_POS];
    int it1  = force_info[M_SRC_INFO_SEQ_ITBEG];
    int it2  = force_info[M_SRC_INFO_SEQ_ITEND];
    int nt_force = it2 - it1 + 1;

    // point tho this force in vec_stf
    float *ptr_force = force_vec_stf + ipos;

    for (int icmp=0; icmp<FD_NDIM; icmp++)
    {
      int iptr_value = n * FD_NDIM + icmp;
      if (it < it1 || it > it2)
      {
        force_vec_value[iptr_value] = 0.0;
      }
      else
      {
        int it_to_it1 = it - it1;
        int iptr = M_SRC_IND(icmp,it_to_it1,istage,nt_force,num_of_stages);

        force_vec_value[iptr_value] = ptr_force[iptr];
      }
    }
  }
  
  for (int n=0; n<num_of_moment; n++)
  {
    int ipos = moment_info[M_SRC_INFO_SEQ_POS];
    int it1  = moment_info[M_SRC_INFO_SEQ_ITBEG];
    int it2  = moment_info[M_SRC_INFO_SEQ_ITEND];
    int nt_moment = it2 - it1 + 1;

    // point tho this moment in ten_rate
    float *ptr_moment = moment_ten_rate + ipos;
    
    for (int icmp=0; icmp<6; icmp++)
    {
      int iptr_value = n * 6 + icmp;

      if (it < it1 || it > it2)
      {
        moment_ten_value[iptr_value] = 0.0;
      }
      else
      {
        int it_to_it1 = it - it1;
        int iptr = M_SRC_IND(icmp,it_to_it1,istage,nt_moment,num_of_stages);
        moment_ten_value[iptr_value] = ptr_moment[iptr];
      }
    }
  }

  // for test, reset only at it=0
  //int n = 0;
  //for (int icmp=0; icmp<6; icmp++)
  //{
  //  int iptr_value = n * 6 + icmp;
  //  if (it==0 && icmp<3) {
  //    moment_ten_value[iptr_value] = 1.0e9;
  //  }
  //  else
  //  {
  //    moment_ten_value[iptr_value] = 0.0;
  //  }
  //}

}

/*
 * convert angles (defined as Aki and Richards) to moment tensor 
 *  in the cartesian coordinate: x-east, y-north, z-upward
 */

void 
angle2moment(float strike, float dip, float rake, float* source_moment_tensor)
{
  float strike_pi,dip_pi,rake_pi; 
  float M11,M22,M33,M12,M13,M23;

  dip_pi    = dip    / 180.0 * PI; 
  strike_pi = strike / 180.0 * PI;
  rake_pi   = rake   / 180.0 * PI;

 // Angles are defined same as in Aki and Richard's book
  M11 = - (  sin(dip_pi) * cos(rake_pi) * sin(2.0*strike_pi) 
           + sin(2.0*dip_pi) * sin(rake_pi) * sin(strike_pi) * sin(strike_pi) );
 
  M22 =  sin(dip_pi) * cos(rake_pi) * sin(2.0 * strike_pi)     
        -sin(2.0*dip_pi) * sin(rake_pi) * cos(strike_pi) * cos(strike_pi) ;

  M33 = - ( M11 + M22 );

  M12 =   sin(dip_pi) * cos(rake_pi) * cos(2.0 * strike_pi)     
        + 0.5 * sin(2.0 * dip_pi) * sin(rake_pi) * sin(2.0 * strike_pi) ;

  M13 = - (  cos(dip_pi) * cos(rake_pi) * cos(strike_pi)  
           + cos(2.0 * dip_pi) * sin(rake_pi) * sin(strike_pi) ) ;

  M23 = - (  cos(dip_pi) * cos(rake_pi) * sin(strike_pi) 
           - cos(2.0*dip_pi) * sin(rake_pi) * cos(strike_pi) );
 
  // attention: the order may be different with outside
  source_moment_tensor[0] = M11 ; 
  source_moment_tensor[1] = M22 ;   
  source_moment_tensor[2] = M33 ;
  source_moment_tensor[3] = M12 ;  
  source_moment_tensor[4] = M13 ;
  source_moment_tensor[5] = M23 ;  
}

/* 
 * if the nearest point in this thread then search its grid index
 *   return value:
 *      1 - in this thread
 *      0 - not in this thread
 */

int
src_coord2index(float sx, float sy, float sz,
        int nx, int ny, int nz,
        int ni1, int ni2, int nj1, int nj2, int nk1, int nk2,
        float *restrict x3d,
        float *restrict y3d,
        float *restrict z3d,
        float *restrict wrk3d,
        int *si, int *sj, int *sk,
        float *sx_inc, float *sy_inc, float *sz_inc)
{
  int is_here = 0; // default outside

  // range of coord of this thread w ghosts
  int x_min = x3d[0];
  int x_max = x3d[0];
  int y_min = y3d[0];
  int y_max = y3d[0];
  int z_min = z3d[0];
  int z_max = z3d[0];
  size_t siz_line  = nx;
  size_t siz_slice = nx * ny;

  for (int k=0; k<nz; k++) {
    for (int j=0; j<ny; j++) {
      for (int i=0; i<nx; i++)
      {
          size_t iptr = i + j * siz_line + k * siz_slice;
          x_min = (x_min < x3d[iptr]) ? x_min : x3d[iptr]; 
          x_max = (x_max > x3d[iptr]) ? x_max : x3d[iptr]; 
          y_min = (y_min < y3d[iptr]) ? y_min : y3d[iptr]; 
          y_max = (y_max > y3d[iptr]) ? y_max : y3d[iptr]; 
          z_min = (z_min < z3d[iptr]) ? z_min : z3d[iptr]; 
          z_max = (z_max > z3d[iptr]) ? z_max : z3d[iptr]; 
      }
    }
  }

  // outside coord range
  if ( sx < x_min || sx > x_max ||
       sy < y_min || sy > y_max ||
       sz < z_min || sz > z_max)
  {
    is_here = 0;
    return is_here;
  }

  // init closest point
  float min_dist = sqrtf(  (sx - x3d[0]) * (sx - x3d[0])
                         + (sy - y3d[0]) * (sy - y3d[0])
                         + (sz - z3d[0]) * (sz - z3d[0]) );
  int min_dist_i = 0 ;
  int min_dist_j = 0 ;
  int min_dist_k = 0 ;

  // compute distance to each point
  for (int k=0; k<nz; k++) {
    for (int j=0; j<ny; j++) {
      for (int i=0; i<nx; i++)
      {
        size_t iptr = i + j * siz_line + k * siz_slice;

        float x = x3d[iptr];
        float y = y3d[iptr];
        float z = z3d[iptr];

        float DistInt = sqrtf(  (sx - x) * (sx - x)
                              + (sy - y) * (sy - y)
                              + (sz - z) * (sz - z) );
        wrk3d[iptr] =  sqrtf(DistInt);

        // replace closest point
        if (min_dist > DistInt)
        {
          min_dist = DistInt;
          min_dist_i = i;
          min_dist_j = j;
          min_dist_k = k;
        }
      }
    }
  }

  // if nearest index is outside phys region, not here
  if ( min_dist_i < ni1 || min_dist_i > ni2 ||
       min_dist_j < nj1 || min_dist_j > nj2 ||
       min_dist_k < nk1 || min_dist_k > nk2 )
  {
    is_here = 0;
    return is_here;
  }

  // in this thread
  is_here = 1;

  // inverse distance interp curv coord
  //float points_x[3*3*3];
  //float points_y[3*3*3];
  //float points_z[3*3*3];
  //float points_i[3*3*3];
  //float points_j[3*3*3];
  //float points_k[3*3*3];

  //for (int kk=0; kk<3; kk++)
  //{
  //  for (int jj=0; jj<3; jj++)
  //  {
  //    for (int ii=0; ii<3; ii++)
  //    {
  //      int iptr_cube = ii + jj * 3 + kk * 9;
  //      int iptr = (min_dist_i-1+ii) + (min_dist_j-1+jj) * siz_line +
  //                 (min_dist_k-1+kk) * siz_slice;

  //      points_x[iptr_cube] = x3d[iptr];
  //      points_y[iptr_cube] = y3d[iptr];
  //      points_z[iptr_cube] = z3d[iptr];

  //      points_i[iptr_cube] = min_dist_i-1+ii;
  //      points_j[iptr_cube] = min_dist_j-1+jj;
  //      points_k[iptr_cube] = min_dist_k-1+kk;
  //    }
  //  }
  //}

  float points_x[8];
  float points_y[8];
  float points_z[8];
  float points_i[8];
  float points_j[8];
  float points_k[8];

  for (int kk=0; kk<2; kk++)
  {
    for (int jj=0; jj<2; jj++)
    {
      for (int ii=0; ii<2; ii++)
      {
        int cur_i = min_dist_i-1+ii;
        int cur_j = min_dist_j-1+jj;
        int cur_k = min_dist_k-1+kk;

        for (int n3=0; n3<2; n3++) {
        for (int n2=0; n2<2; n2++) {
        for (int n1=0; n1<2; n1++) {
          int iptr_cube = n1 + n2 * 2 + n3 * 4;
          int iptr = (cur_i+n1) + (cur_j+n2) * siz_line +
                     (cur_k+n3) * siz_slice;
          points_x[iptr_cube] = x3d[iptr];
          points_y[iptr_cube] = y3d[iptr];
          points_z[iptr_cube] = z3d[iptr];
          points_i[iptr_cube] = cur_i+n1;
          points_j[iptr_cube] = cur_j+n2;
          points_k[iptr_cube] = cur_k+n3;
        }
        }
        }

        if (isPointInHexahedron(sx,sy,sz,points_x,points_y,points_z) == true)
        {
          float si_curv, sj_curv, sk_curv;
          src_cart2curv(sx,sy,sz,
                        8,
                        points_x,points_y,points_z,
                        points_i,points_j,points_k,
                        &si_curv, &sj_curv, &sk_curv);

          // convert to return values
          *si = min_dist_i;
          *sj = min_dist_j;
          *sk = min_dist_k;
          *sx_inc = si_curv - min_dist_i;
          *sy_inc = sj_curv - min_dist_j;
          *sz_inc = sk_curv - min_dist_k;

          return is_here;
        }
      }
    }
  }

  return is_here;
}

/* 
 * interp curv coord using inverse distance interp
 */

int
src_cart2curv(float sx, float sy, float sz, 
        int num_points,
        float *points_x, // x coord of all points
        float *points_y,
        float *points_z,
        float *points_i, // curv coord of all points
        float *points_j,
        float *points_k,
        float *si_curv, // interped curv coord
        float *sj_curv,
        float *sk_curv)
{
 float weight[num_points];
 float total_weight = 0.0 ;

 // cal weight
 int at_point_indx = -1;
 for (int i=0; i<num_points; i++)
 {
   float dist = sqrtf ((sx - points_x[i]) * (sx - points_x[i])
                 + (sy - points_y[i]) * (sy - points_y[i])
                 + (sz - points_z[i]) * (sz - points_z[i])
                  );
   if (dist < 1e-9) {
     at_point_indx = i;
   } else {
     weight[i]   = 1.0 / dist;
     total_weight += weight[i];
   }
 }
 // if at a point
 if (at_point_indx > 0) {
   total_weight = 1.0;
   // other weight 0
   for (int i=0; i<num_points; i++) {
     weight[i] = 0.0;
   }
   // point weight 1
   weight[at_point_indx] = 1.0;
 }

 // interp

 *si_curv = 0.0;
 *sj_curv = 0.0;
 *sk_curv = 0.0;

 for (int i=0; i<num_points; i++)
 {
   weight[i] *= 1.0 / total_weight ;
 
   (*si_curv) += weight[i] * points_i[i];
   (*sj_curv) += weight[i] * points_j[i]; 
   (*sk_curv) += weight[i] * points_k[i];  

   fprintf(stdout,"---- i=%d,weight=%f,points_i=%f,points_j=%f,points_k=%f\n",
          i,weight[i],points_i[i],points_j[i],points_k[i]);
 }

 return 0;
}

