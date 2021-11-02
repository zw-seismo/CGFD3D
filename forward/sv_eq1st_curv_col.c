/*******************************************************************************
 * solver of isotropic elastic 1st-order eqn using curv grid and macdrp schem
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#include "fdlib_mem.h"
#include "fdlib_math.h"
#include "blk_t.h"
#include "wav_t.h"
#include "sv_eq1st_curv_col.h"
#include "sv_eq1st_curv_col_ac_iso.h"
#include "sv_eq1st_curv_col_el_iso.h"
#include "sv_eq1st_curv_col_el_vti.h"
#include "sv_eq1st_curv_col_el_aniso.h"

//#define SV_ELISO1ST_CURV_MACDRP_DEBUG

/*******************************************************************************
 * one simulation over all time steps, could be used in imaging or inversion
 *  simple MPI exchange without computing-communication overlapping
 ******************************************************************************/

void
sv_eq1st_curv_col_allstep(
  fd_t            *fd,
  gdinfo_t        *gdinfo,
  gdcurv_metric_t *metric,
  md_t      *md,
  src_t      *src,
  bdryfree_t *bdryfree,
  bdrypml_t  *bdrypml,
  wav_t  *wav,
  mympi_t    *mympi,
  iorecv_t   *iorecv,
  ioline_t   *ioline,
  ioslice_t  *ioslice,
  iosnap_t   *iosnap,
  // time
  float dt, int nt_total, float t0,
  char *output_fname_part,
  char *output_dir,
  int qc_check_nan_num_of_step,
  const int output_all, // qc all var
  const int verbose)
{
  // retrieve from struct
  int num_rk_stages = fd->num_rk_stages;
  float *rk_a = fd->rk_a;
  float *rk_b = fd->rk_b;

  int num_of_pairs     = fd->num_of_pairs;
  int fdx_max_half_len = fd->fdx_max_half_len;
  int fdy_max_half_len = fd->fdy_max_half_len;
  int fdz_max_len      = fd->fdz_max_len;
  int num_of_fdz_op    = fd->num_of_fdz_op;

  // mpi
  int myid = mympi->myid;
  int *topoid = mympi->topoid;
  MPI_Comm comm = mympi->comm;
  float *restrict sbuff = mympi->sbuff;
  float *restrict rbuff = mympi->rbuff;

  // local allocated array
  char ou_file[CONST_MAX_STRLEN];

  // local pointer
  float *restrict w_cur;
  float *restrict w_pre;
  float *restrict w_rhs;
  float *restrict w_end;
  float *restrict w_tmp;

  int   ipair, istage;
  float t_cur;
  float t_end; // time after this loop for nc output
  // for mpi message
  int   ipair_mpi, istage_mpi;

  // create slice nc output files
  if (myid==0 && verbose>0) fprintf(stdout,"prepare slice nc output ...\n"); 
  ioslice_nc_t ioslice_nc;
  io_slice_nc_create(ioslice, wav->ncmp, wav->cmp_name,
                     gdinfo->ni, gdinfo->nj, gdinfo->nk, topoid,
                     &ioslice_nc);

  // create snapshot nc output files
  if (myid==0 && verbose>0) fprintf(stdout,"prepare snap nc output ...\n"); 
  iosnap_nc_t  iosnap_nc;
  if (md->medium_type == CONST_MEDIUM_ACOUSTIC_ISO) {
    io_snap_nc_create_ac(iosnap, &iosnap_nc, topoid);
  } else {
    io_snap_nc_create(iosnap, &iosnap_nc, topoid);
  }

  // only x/y mpi
  int num_of_r_reqs = 4;
  int num_of_s_reqs = 4;

  // get wavefield
  w_pre = wav->v5d + wav->siz_ilevel * 0; // previous level at n
  w_tmp = wav->v5d + wav->siz_ilevel * 1; // intermidate value
  w_rhs = wav->v5d + wav->siz_ilevel * 2; // for rhs
  w_end = wav->v5d + wav->siz_ilevel * 3; // end level at n+1

  // set pml for rk
  for (int idim=0; idim<CONST_NDIM; idim++) {
    for (int iside=0; iside<2; iside++) {
      if (bdrypml->is_at_sides[idim][iside]==1) {
        bdrypml_auxvar_t *auxvar = &(bdrypml->auxvar[idim][iside]);
        auxvar->pre = auxvar->var + auxvar->siz_ilevel * 0;
        auxvar->tmp = auxvar->var + auxvar->siz_ilevel * 1;
        auxvar->rhs = auxvar->var + auxvar->siz_ilevel * 2;
        auxvar->end = auxvar->var + auxvar->siz_ilevel * 3;
      }
    }
  }

  // alloc free surface PGV and PGA
  float *PG = NULL;
  if (bdryfree->is_at_sides[CONST_NDIM-1][1] == 1)
  {
    PG = (float *) fdlib_mem_calloc_1d_float(6*gdinfo->ny*gdinfo->nx,0.0,"PG malloc");
  }
  // calculate conversion matrix for free surface
  if (bdryfree->is_at_sides[CONST_NDIM-1][1] == 1)
  {
    if (md->medium_type == CONST_MEDIUM_ELASTIC_ISO)
    {
      sv_eq1st_curv_col_el_iso_dvh2dvz(gdinfo,metric,md,bdryfree,verbose);
    }
    else if (md->medium_type == CONST_MEDIUM_ELASTIC_VTI)
    {
      sv_eq1st_curv_col_el_vti_dvh2dvz(gdinfo,metric,md,bdryfree,verbose);
    }
    else if (md->medium_type == CONST_MEDIUM_ELASTIC_ANISO)
    {
      sv_eq1st_curv_col_el_aniso_dvh2dvz(gdinfo,metric,md,bdryfree,verbose);
    }
    else if (md->medium_type == CONST_MEDIUM_ACOUSTIC_ISO)
    {
      // no need
    }
    else
    {
      fprintf(stderr,"ERROR: conversion matrix for medium_type=%d is not implemented\n",
                    md->medium_type);
      MPI_Abort(MPI_COMM_WORLD,1);
    }
  }

  //--------------------------------------------------------
  // time loop
  //--------------------------------------------------------

  if (myid==0 && verbose>0) fprintf(stdout,"start time loop ...\n"); 

  for (int it=0; it<nt_total; it++)
  {
    t_cur  = it * dt + t0;
    t_end = t_cur +dt;

    if (myid==0 && verbose>10) fprintf(stdout,"-> it=%d, t=%f\n", it, t_cur);

    // mod to get ipair
    ipair = it % num_of_pairs;
    if (myid==0 && verbose>10) fprintf(stdout, " --> ipair=%d\n",ipair);

    // loop RK stages for one step
    for (istage=0; istage<num_rk_stages; istage++)
    {
      if (myid==0 && verbose>10) fprintf(stdout, " --> istage=%d\n",istage);

      // for mesg
      if (istage != num_rk_stages-1) {
        ipair_mpi = ipair;
        istage_mpi = istage + 1;
      } else {
        ipair_mpi = (it + 1) % num_of_pairs;
        istage_mpi = 0; 
      }

      // use pointer to avoid 1 copy for previous level value
      if (istage==0) {
        w_cur = w_pre;
        for (int idim=0; idim<CONST_NDIM; idim++) {
          for (int iside=0; iside<2; iside++) {
            bdrypml->auxvar[idim][iside].cur = bdrypml->auxvar[idim][iside].pre;
          }
        }
      }
      else
      {
        w_cur = w_tmp;
        for (int idim=0; idim<CONST_NDIM; idim++) {
          for (int iside=0; iside<2; iside++) {
            bdrypml->auxvar[idim][iside].cur = bdrypml->auxvar[idim][iside].tmp;
          }
        }
      }

      // set src_t time
      src_set_time(src, it, istage);

      // compute rhs
      switch (md->medium_type)
      {
        case CONST_MEDIUM_ELASTIC_ISO : {
          sv_eq1st_curv_col_el_iso_onestage(
              w_cur,w_rhs,wav,
              gdinfo, metric, md, bdryfree, bdrypml, src,
              fd->num_of_fdx_op, fd->pair_fdx_op[ipair][istage],
              fd->num_of_fdy_op, fd->pair_fdy_op[ipair][istage],
              fd->num_of_fdz_op, fd->pair_fdz_op[ipair][istage],
              fd->fdz_max_len,
              myid, verbose);

          break;
        }

        case CONST_MEDIUM_ELASTIC_ANISO : {
          sv_eq1st_curv_col_el_aniso_onestage(
              w_cur,w_rhs,wav,
              gdinfo, metric, md, bdryfree, bdrypml, src,
              fd->num_of_fdx_op, fd->pair_fdx_op[ipair][istage],
              fd->num_of_fdy_op, fd->pair_fdy_op[ipair][istage],
              fd->num_of_fdz_op, fd->pair_fdz_op[ipair][istage],
              fd->fdz_max_len,
              myid, verbose);

          break;
        }

        case CONST_MEDIUM_ELASTIC_VTI : {
          sv_eq1st_curv_col_el_vti_onestage(
              w_cur,w_rhs,wav,
              gdinfo, metric, md, bdryfree, bdrypml, src,
              fd->num_of_fdx_op, fd->pair_fdx_op[ipair][istage],
              fd->num_of_fdy_op, fd->pair_fdy_op[ipair][istage],
              fd->num_of_fdz_op, fd->pair_fdz_op[ipair][istage],
              fd->fdz_max_len,
              myid, verbose);

          break;
        }

        case CONST_MEDIUM_ACOUSTIC_ISO : {
          sv_eq1st_curv_col_ac_iso_onestage(
              w_cur,w_rhs,wav,
              gdinfo, metric, md, bdryfree, bdrypml, src,
              fd->num_of_fdx_op, fd->pair_fdx_op[ipair][istage],
              fd->num_of_fdy_op, fd->pair_fdy_op[ipair][istage],
              fd->num_of_fdz_op, fd->pair_fdz_op[ipair][istage],
              fd->fdz_max_len,
              myid, verbose);

          break;
        }
      }

      // recv mesg
      MPI_Startall(num_of_r_reqs, mympi->pair_r_reqs[ipair_mpi][istage_mpi]);

      // rk start
      if (istage==0)
      {
        float coef_a = rk_a[istage] * dt;
        float coef_b = rk_b[istage] * dt;

        // wavefield
        for (size_t iptr=0; iptr < wav->siz_ilevel; iptr++) {
            w_tmp[iptr] = w_pre[iptr] + coef_a * w_rhs[iptr];
        }

        // apply Qs
        //if (md->visco_type == CONST_VISCO_GRAVES_QS) {
        //  sv_eq1st_curv_graves_Qs(w_tmp, wave->ncmp, gdinfo, md);
        //}

        // pack and isend
        blk_macdrp_pack_mesg(w_tmp, sbuff, wav->ncmp, gdinfo,
                         &(fd->pair_fdx_op[ipair_mpi][istage_mpi][fd->num_of_fdx_op-1]),
                         &(fd->pair_fdy_op[ipair_mpi][istage_mpi][fd->num_of_fdy_op-1]));

        MPI_Startall(num_of_s_reqs, mympi->pair_s_reqs[ipair_mpi][istage_mpi]);

        // pml_tmp
        for (int idim=0; idim<CONST_NDIM; idim++) {
          for (int iside=0; iside<2; iside++) {
            if (bdrypml->is_at_sides[idim][iside]==1) {
              bdrypml_auxvar_t *auxvar = &(bdrypml->auxvar[idim][iside]);
              for (size_t iptr=0; iptr < auxvar->siz_ilevel; iptr++) {
                auxvar->tmp[iptr] = auxvar->pre[iptr] + coef_a * auxvar->rhs[iptr];
              }
            }
          }
        }

        // w_end
        for (size_t iptr=0; iptr < wav->siz_ilevel; iptr++) {
            w_end[iptr] = w_pre[iptr] + coef_b * w_rhs[iptr];
        }
        // pml_end
        for (int idim=0; idim<CONST_NDIM; idim++) {
          for (int iside=0; iside<2; iside++) {
            if (bdrypml->is_at_sides[idim][iside]==1) {
              bdrypml_auxvar_t *auxvar = &(bdrypml->auxvar[idim][iside]);
              for (size_t iptr=0; iptr < auxvar->siz_ilevel; iptr++) {
                auxvar->end[iptr] = auxvar->pre[iptr] + coef_b * auxvar->rhs[iptr];
              }
            }
          }
        }
      }
      else if (istage<num_rk_stages-1)
      {
        float coef_a = rk_a[istage] * dt;
        float coef_b = rk_b[istage] * dt;

        // wavefield
        for (size_t iptr=0; iptr < wav->siz_ilevel; iptr++) {
            w_tmp[iptr] = w_pre[iptr] + coef_a * w_rhs[iptr];
        }

        // apply Qs
        //if (md->visco_type == CONST_VISCO_GRAVES_QS) {
        //  sv_eq1st_curv_graves_Qs(w_tmp, wave->ncmp, gdinfo, md);
        //}

        // pack and isend
        blk_macdrp_pack_mesg(w_tmp, sbuff, wav->ncmp, gdinfo,
                         &(fd->pair_fdx_op[ipair_mpi][istage_mpi][fd->num_of_fdx_op-1]),
                         &(fd->pair_fdy_op[ipair_mpi][istage_mpi][fd->num_of_fdy_op-1]));

        MPI_Startall(num_of_s_reqs, mympi->pair_s_reqs[ipair_mpi][istage_mpi]);

        // pml_tmp
        for (int idim=0; idim<CONST_NDIM; idim++) {
          for (int iside=0; iside<2; iside++) {
            if (bdrypml->is_at_sides[idim][iside]==1) {
              bdrypml_auxvar_t *auxvar = &(bdrypml->auxvar[idim][iside]);
              for (size_t iptr=0; iptr < auxvar->siz_ilevel; iptr++) {
                auxvar->tmp[iptr] = auxvar->pre[iptr] + coef_a * auxvar->rhs[iptr];
              }
            }
          }
        }

        // w_end
        for (size_t iptr=0; iptr < wav->siz_ilevel; iptr++) {
            w_end[iptr] += coef_b * w_rhs[iptr];
        }
        // pml_end
        for (int idim=0; idim<CONST_NDIM; idim++) {
          for (int iside=0; iside<2; iside++) {
            if (bdrypml->is_at_sides[idim][iside]==1) {
              bdrypml_auxvar_t *auxvar = &(bdrypml->auxvar[idim][iside]);
              for (size_t iptr=0; iptr < auxvar->siz_ilevel; iptr++) {
                auxvar->end[iptr] += coef_b * auxvar->rhs[iptr];
              }
            }
          }
        }
      }
      else // last stage
      {
        float coef_b = rk_b[istage] * dt;

        // wavefield
        for (size_t iptr=0; iptr < wav->siz_ilevel; iptr++) {
            w_end[iptr] += coef_b * w_rhs[iptr];
        }

        // apply Qs
        if (md->visco_type == CONST_VISCO_GRAVES_QS) {
          sv_eq1st_curv_graves_Qs(w_end, wav->ncmp, dt, gdinfo, md);
        }
        
        // pack and isend
        blk_macdrp_pack_mesg(w_end, sbuff, wav->ncmp, gdinfo,
                         &(fd->pair_fdx_op[ipair_mpi][istage_mpi][fd->num_of_fdx_op-1]),
                         &(fd->pair_fdy_op[ipair_mpi][istage_mpi][fd->num_of_fdy_op-1]));

        MPI_Startall(num_of_s_reqs, mympi->pair_s_reqs[ipair_mpi][istage_mpi]);

        // pml_end
        for (int idim=0; idim<CONST_NDIM; idim++) {
          for (int iside=0; iside<2; iside++) {
            if (bdrypml->is_at_sides[idim][iside]==1) {
              bdrypml_auxvar_t *auxvar = &(bdrypml->auxvar[idim][iside]);
              for (size_t iptr=0; iptr < auxvar->siz_ilevel; iptr++) {
                auxvar->end[iptr] += coef_b * auxvar->rhs[iptr];
              }
            }
          }
        }
      }

      MPI_Waitall(num_of_s_reqs, mympi->pair_s_reqs[ipair_mpi][istage_mpi], MPI_STATUS_IGNORE);
      MPI_Waitall(num_of_r_reqs, mympi->pair_r_reqs[ipair_mpi][istage_mpi], MPI_STATUS_IGNORE);

      if (istage != num_rk_stages-1) {
        blk_macdrp_unpack_mesg(rbuff, w_tmp,  wav->ncmp, gdinfo,
                         &(fd->pair_fdx_op[ipair_mpi][istage_mpi][fd->num_of_fdx_op-1]),
                         &(fd->pair_fdy_op[ipair_mpi][istage_mpi][fd->num_of_fdy_op-1]),
                         mympi->pair_siz_rbuff_x1[ipair_mpi][istage_mpi],
                         mympi->pair_siz_rbuff_x2[ipair_mpi][istage_mpi],
                         mympi->pair_siz_rbuff_y1[ipair_mpi][istage_mpi],
                         mympi->pair_siz_rbuff_y2[ipair_mpi][istage_mpi],
                         mympi->neighid);
     } else {
        blk_macdrp_unpack_mesg(rbuff, w_end,  wav->ncmp, gdinfo,
                         &(fd->pair_fdx_op[ipair_mpi][istage_mpi][fd->num_of_fdx_op-1]),
                         &(fd->pair_fdy_op[ipair_mpi][istage_mpi][fd->num_of_fdy_op-1]),
                         mympi->pair_siz_rbuff_x1[ipair_mpi][istage_mpi],
                         mympi->pair_siz_rbuff_x2[ipair_mpi][istage_mpi],
                         mympi->pair_siz_rbuff_y1[ipair_mpi][istage_mpi],
                         mympi->pair_siz_rbuff_y2[ipair_mpi][istage_mpi],
                         mympi->neighid);
     }

    } // RK stages

    //--------------------------------------------
    // QC
    //--------------------------------------------

    if (qc_check_nan_num_of_step >0  && (it % qc_check_nan_num_of_step) == 0) {
      if (myid==0 && verbose>10) fprintf(stdout,"-> check value nan\n");
        //wav_check_value(w_end);
    }
    
    //--------------------------------------------
    // save results
    //--------------------------------------------
    // calculate PGV and PGA for each surface at each stage
    if (bdryfree->is_at_sides[CONST_NDIM-1][1] == 1)
    {
        PG_calcu(w_end, w_pre, gdinfo, PG, dt);
    }

    //-- recv by interp
    io_recv_keep(iorecv, w_end, it, wav->ncmp, wav->siz_icmp);

    //-- line values
    io_line_keep(ioline, w_end, it, wav->ncmp, wav->siz_icmp);

    // write slice, use w_rhs as buff
    io_slice_nc_put(ioslice,&ioslice_nc,gdinfo,w_end,w_rhs,it,t_end,0,wav->ncmp-1);

    // snapshot
    if (md->medium_type == CONST_MEDIUM_ACOUSTIC_ISO) {
      io_snap_nc_put_ac(iosnap, &iosnap_nc, gdinfo, wav, 
                     w_end, w_rhs, nt_total, it, t_end, 1,1,1);
    } else {
      io_snap_nc_put(iosnap, &iosnap_nc, gdinfo, wav, 
                     w_end, w_rhs, nt_total, it, t_end, 1,1,1);
    }

    // zero temp used w_rsh
    wav_zero_edge(gdinfo, wav, w_rhs);

    // debug output
    if (output_all==1)
    {
        io_build_fname_time(output_dir,"w3d",".nc",topoid,it,ou_file);
        io_var3d_export_nc(ou_file,
                           w_end,
                           wav->cmp_pos,
                           wav->cmp_name,
                           wav->ncmp,
                           gdinfo->index_name,
                           gdinfo->nx,
                           gdinfo->ny,
                           gdinfo->nz);
    }

    // swap w_pre and w_end, avoid copying
    w_cur = w_pre; w_pre = w_end; w_end = w_cur;

    for (int idim=0; idim<CONST_NDIM; idim++) {
      for (int iside=0; iside<2; iside++) {
        bdrypml_auxvar_t *auxvar = &(bdrypml->auxvar[idim][iside]);
        auxvar->cur = auxvar->pre;
        auxvar->pre = auxvar->end;
        auxvar->end = auxvar->cur;
      }
    }

  } // time loop

  // postproc
  PG_slice_output(PG,gdinfo,output_dir, output_fname_part,topoid);
  // close nc
  io_slice_nc_close(&ioslice_nc);
  io_snap_nc_close(&iosnap_nc);

  return;
}

int
sv_eq1st_curv_graves_Qs(float *w, int ncmp, float dt, gdinfo_t *gdinfo, md_t *md)
{
  int ierr = 0;

  float coef = - PI * md->visco_Qs_freq * dt;

  for (int icmp=0; icmp<ncmp; icmp++)
  {
    float *restrict var = w + icmp * gdinfo->siz_icmp;

    for (int k = gdinfo->nk1; k <= gdinfo->nk2; k++)
    {
      for (int j = gdinfo->nj1; j <= gdinfo->nj2; j++)
      {
        for (int i = gdinfo->ni1; i <= gdinfo->ni2; i++)
        {
          size_t iptr = i + j * gdinfo->siz_iy + k * gdinfo->siz_iz;

          float Qatt = expf( coef / md->Qs[iptr] );

          var[iptr] *= Qatt;
        }
      }
    }
  }

  return ierr;
}
