/*
 *
 */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "netcdf.h"

#include "fdlib_mem.h"
#include "md_el_iso.h"

void
md_el_iso_init_vars(
    size_t siz_volume,
    int *number_of_vars, 
    float **p_m3d,
    size_t **p_m3d_pos,
    char ***p_m3d_name)
{
  const int num_medium_vars = 3;
  /*
   * 0: rho
   * 1: lambda
   * 2: mu
   */

  // vars
  float *m3d = (float *) fdlib_mem_calloc_1d_float( 
               siz_volume * num_medium_vars, 0.0, "md_el_iso_init_vars");
  if (m3d == NULL) {
      fprintf(stderr,"Error: failed to alloc medium_el_iso\n");
      fflush(stderr);
      ierr = -1;
  }

  // position of each var
  size_t *m3d_pos = (size_t *) fdlib_mem_calloc_1d_sizet( 
               num_medium_vars, 0, "medium_el_iso_init_vars");

  // name of each var
  char **m3d_name = (char **) fdlib_mem_malloc_2l_char( 
               num_medium_vars, FD_MAX_STRLEN, "medium_el_iso_init_vars");

  // init
  ivar = MD_EL_ISO_SEQ_RHO;
  m3d_pos[ivar] = ivar * siz_volume;
  strcpy(m3d_name[ivar],"rho");

  ivar = MD_EL_ISO_SEQ_LAMBDA;
  m3d_pos[ivar] = ivar * siz_volume;
  strcpy(m3d_name[ivar],"lamda");

  ivar = MD_EL_ISO_SEQ_MU;
  m3d_pos[ivar] = ivar * siz_volume;
  strcpy(m3d_name[ivar],"mu");

  // set return values
  *p_m3d = m3d;
  *p_m3d_pos = m3d_pos;
  *p_m3d_name = m3d_name;
  *m3d_num_of_vars = num_medium_vars;
}

//
//
//
void
md_el_iso_import(float *restrict m3d, size_t *restrict m3d_pos, char **restrict m3d_name,
        int number_of_vars, size_t siz_volume, char *in_dir, int *myid3)
{
  char in_file[FD_MAX_STRLEN];
  
  int ncid, varid;
  
  // construct file name
  sprintf(in_file, "%s/media_mpi%02d%02d%02d.nc", in_dir, myid3[0], myid3[1], myid3[2]);
  
  // read in nc
  ierr = nc_open(in_file, NC_NOWRITE, &ncid);
  if (ierr != NC_NOERR) errh(ierr);
  
  for (int ivar=0; ivar<number_of_vars; ivar++) {
      ierr = nc_inq_varid(ncid, m3d_name[ivar], &varid);
      if (ierr != NC_NOERR) errh(ierr);
  
      ierr = nc_get_vara_float(ncid,varid,m3d+m3d_pos[ivar]);
      if (ierr != NC_NOERR) errh(ierr);
  }
  
  // close file
  ierr = nc_close(ncid);
  if (ierr != NC_NOERR) errh(ierr);
}

void
md_el_iso_rho_to_slow(float *restrict m3d, size_t siz_volume);
{
  float *rho = m3d + MD_ELISO_SEQ_RHO * siz_volume;

  /*
  for (size_t k=0; k<nx; k++) {
    for (size_t j=0; j<ny; j++) {
      for (size_t i=0; i<nx; i++) {
      }
    }
  }
  */
  for (size_t iptr=0; iptr<siz_volume; iptr++) {
    if (rho[iptr] > 1e-10) {
      rho[iptr] = 1.0 / rho[iptr];
    } else {
      rho[iptr] = 0.0;
    }
  }
}

/*
 * test
 */

void
md_el_iso_gen_test(
    float *restrict m3d,
    float *restrict x3d,
    float *restrict y3d,
    float *restrict z3d,
    size_t nx,
    size_t ny,
    size_t nz,
    size_t siz_line,
    size_t siz_slice,
    size_t siz_volume)
{
  float *lam3d = m3d + MD_ELISO_SEQ_LAMBDA * siz_volume;
  float  *mu3d = m3d + MD_ELISO_SEQ_MU     * siz_volume;
  float *rho3d = m3d + MD_ELISO_SEQ_RHO    * siz_volume;

  for (size_t k=0; k<nz; k++)
  {
    for (size_t j=0; j<ny; j++)
    {
      for (size_t i=0; i<nx; i++)
      {
        float Vp=3000.0;
        float Vs=2000.0;
        float rho=1500.0;
        float mu = Vs*Vs*rho;
        float lam = Vp*Vp*rho - 2.0*mu;
        lam3d[iptr] = lam;
         mu3d[iptr] = mu;
        rho3d[iptr] = rho;
      }
    }
  }
}

/*
 * convert rho to slowness to reduce number of arithmetic cal
 */

void
md_el_iso_gen_test(
    float *restrict m3d,
    size_t nx,
    size_t ny,
    size_t nz,
    size_t siz_line,
    size_t siz_slice,
    size_t siz_volume)
{
  float *rho3d = m3d + MD_ELISO_SEQ_RHO    * siz_volume;

  for (size_t k=0; k<nz; k++)
  {
    for (size_t j=0; j<ny; j++)
    {
      for (size_t i=0; i<nx; i++)
      {
        if (rho3d[iptr]>1.0e-20) {
          rho3d[iptr] = 1.0 / rho3d[iptr];
        }
      }
    }
  }
}

