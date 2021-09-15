/***************************************************************************
 *
 * This function is used for medium parameterization.
 *
 * Authors: Luqian Jiang <jianglq@mail.ustc.edu.cn>
 *          Wei Zhang <zhangwei@sustech.edu.cn>
 *
 * Copyright (c) 2021 zwlab
 *
 * ChangeLog: 
 *    06/2021: Created by Luqian Jiang 
 *
 ***************************************************************************/
#include <iostream>
#include <vector>
#include <string.h>
#include <cmath>
#include "media_geometry3d.hpp"
#include "media_layer2model.hpp"
#include "media_read_file.hpp"
#include "media_utility.hpp"

//using namespace std;

/*============================= for C call =================================*/
//---- 0. one component
int media_layer2model_onecmp(float *var3d,
                             const float *x3d, 
                             const float *y3d, 
                             const float *z3d, 
                             size_t nx,
                             size_t ny, 
                             size_t nz,
                             int grid_type, 
                             const char *in_var_file,
                             const char *average_method) // loc, har, ari
{
    inter_t interfaces;

    /*Read interface file*/
    read_interface_file(in_var_file, &interfaces);
  
    if (interfaces.media_type != ONE_COMPONENT) {
        fprintf(stderr, "ERROR: The media_layer2model_onecmp function only supports "\
                "one_component, please check the media_type of %s! \n", in_var_file);        
        fflush(stderr);
        exit(1);
    }

    if (strcmp(average_method, "loc") == 0) {
        parametrization_layer_onecmp_loc(nx, ny, nz, x3d, y3d, z3d, grid_type, interfaces, var3d);
    } else if (strcmp(average_method, "har") == 0) {      // harmonic
        parametrization_layer_onecmp_har(nx, ny, nz, x3d, y3d, z3d, grid_type, interfaces, var3d);
    } else if (strcmp(average_method, "ari") == 0) {      //arithemtic
        parametrization_layer_onecmp_ari(nx, ny, nz, x3d, y3d, z3d, grid_type, interfaces, var3d);
    } else {                                                        // default = loc
        fprintf(stderr, "ERROR: Wrong average method %s for one_component \n", average_method);        
        fflush(stderr);
        exit(1);
    }

    if (interfaces.elevation != nullptr) delete [] interfaces.elevation;
    if (interfaces.var       != nullptr) delete [] interfaces.var;
    if (interfaces.var_grad  != nullptr) delete [] interfaces.var_grad;
    if (interfaces.var_pow   != nullptr) delete [] interfaces.var_pow;

    return 0;
}

//---- 1. acoustic isotropic
int media_layer2model_ac_iso(
        float *rho3d,
        float *kappa3d,
        const float *x3d, 
        const float *y3d, 
        const float *z3d, 
        size_t nx,
        size_t ny,
        size_t nz,
        int grid_type, 
        const char *in_3lay_file,
        const char *equivalent_medium_method) 
{

    inter_t interfaces;

    /*Read interface file*/
    read_interface_file(in_3lay_file, &interfaces);

    if (interfaces.media_type != ACOUSTIC_ISOTROPIC) {
        fprintf(stderr, "ERROR: The media_layer2model_ac_iso function only supports "\
                "acoustic_isotropic, please check the media_type of %s! \n", in_3lay_file);        
        fflush(stderr);
        exit(1);
    }

    if (strcmp(equivalent_medium_method, "loc") == 0) {
        parametrization_layer_ac_iso_loc(nx, ny, nz, x3d, y3d, z3d, grid_type, 
            interfaces, kappa3d, rho3d);
    } else if (strcmp(equivalent_medium_method, "ari") == 0 ) {
        parametrization_layer_ac_iso_ari(nx, ny, nz, x3d, y3d, z3d, grid_type, 
            interfaces, kappa3d, rho3d);
    } else if (strcmp(equivalent_medium_method, "har") == 0 ) {
        parametrization_layer_ac_iso_har(nx, ny, nz, x3d, y3d, z3d, grid_type, 
            interfaces, kappa3d, rho3d);
    } else { 
        fprintf(stderr, "ERROR: Wrong average method %s for acoustic_isotropic media \n", equivalent_medium_method);        
        fflush(stderr);
        exit(1);
    }

    if (interfaces.elevation != nullptr) delete [] interfaces.elevation;
    if (interfaces.vp       != nullptr) delete [] interfaces.vp;
    if (interfaces.rho      != nullptr) delete [] interfaces.rho;
    if (interfaces.vp_grad  != nullptr) delete [] interfaces.vp_grad;
    if (interfaces.rho_grad != nullptr) delete [] interfaces.rho_grad;
    if (interfaces.vp_pow   != nullptr) delete [] interfaces.vp_pow;
    if (interfaces.rho_pow  != nullptr) delete [] interfaces.rho_pow;

    return 0; 
}

//----  2. elastic isotropic
int media_layer2model_el_iso(
        float *lam3d,
        float *mu3d,
        float *rho3d,
        const float *x3d, 
        const float *y3d, 
        const float *z3d, 
        size_t nx,
        size_t ny,
        size_t nz,
        int grid_type, 
        const char *in_3lay_file,
        const char *equivalent_medium_method) // 
{

    inter_t interfaces;

    /* Read interface file */
    read_interface_file(in_3lay_file, &interfaces);   

    if (interfaces.media_type != ELASTIC_ISOTROPIC) {
        fprintf(stderr, "ERROR: The media_layer2model_el_iso function only supports "\
                "elastic_isotropic, please check the media_type of %s! \n", in_3lay_file);        
        fflush(stderr);
        exit(1);
    }

    if (strcmp(equivalent_medium_method, "loc") == 0) {
        parametrization_layer_el_iso_loc(nx, ny, nz, x3d, y3d, z3d, grid_type, 
            interfaces, lam3d, mu3d, rho3d);
    } else if (strcmp(equivalent_medium_method, "har") == 0) {
        parametrization_layer_el_iso_har(nx, ny, nz, x3d, y3d, z3d, grid_type, 
            interfaces, lam3d, mu3d, rho3d);
    } else if (strcmp(equivalent_medium_method, "ari") == 0) {
        parametrization_layer_el_iso_ari(nx, ny, nz, x3d, y3d, z3d, grid_type, 
            interfaces, lam3d, mu3d, rho3d);
    } else { //default
        fprintf(stderr, "ERROR: Wrong parameterization method %s in media_layer2model_el_iso function, "\
                "if you want to use tti method, please call the media_layer2model_el_aniso function. \n", 
                equivalent_medium_method);        
        fflush(stderr);
        exit(1);
    }

    if (interfaces.elevation != nullptr) delete [] interfaces.elevation;
    if (interfaces.vp       != nullptr) delete [] interfaces.vp;
    if (interfaces.rho      != nullptr) delete [] interfaces.rho;
    if (interfaces.vp_grad  != nullptr) delete [] interfaces.vp_grad;
    if (interfaces.rho_grad != nullptr) delete [] interfaces.rho_grad;
    if (interfaces.vp_pow   != nullptr) delete [] interfaces.vp_pow;
    if (interfaces.rho_pow  != nullptr) delete [] interfaces.rho_pow;
    if (interfaces.vs       != nullptr) delete [] interfaces.vs;
    if (interfaces.vs_grad  != nullptr) delete [] interfaces.vs_grad;
    if (interfaces.vs_pow   != nullptr) delete [] interfaces.vs_pow;

    return 0; 
}

//--- 3. elastic vti
int media_layer2model_el_vti(
        float *rho,
        float *c11,
        float *c33,
        float *c55,
        float *c66,
        float *c13,
        const float *x3d,
        const float *y3d,
        const float *z3d,
        size_t nx,
        size_t ny,
        size_t nz,
        int grid_type, 
        const char *in_3lay_file, 
        const char *equivalent_medium_method) 
{

    inter_t interfaces;
    int md_type = interfaces.media_type;

    /* Read interface file */
    read_interface_file(in_3lay_file, &interfaces);   

    if (md_type != ELASTIC_VTI_PREM || md_type != ELASTIC_VTI_THOMSEN || md_type != ELASTIC_VTI_CIJ) {
        fprintf(stderr, "ERROR: The media_layer2model_el_vti function only supports "\
                "elastic_vti_prem, elastic_vti_thomsen, elastic_vti_cij, "\
                "please check the media_type of %s! \n", in_3lay_file);        
        fflush(stderr);
        exit(1);
    }

    if (strcmp(equivalent_medium_method, "loc") == 0) {
        parametrization_layer_el_vti_loc(nx, ny, nz, x3d, y3d, z3d, grid_type, 
            interfaces, c11, c33, c55, c66, c13, rho);
    } else if (strcmp(equivalent_medium_method, "har") == 0) {
        parametrization_layer_el_vti_har(nx, ny, nz, x3d, y3d, z3d, grid_type, 
            interfaces, c11, c33, c55, c66, c13, rho);
    } else if (strcmp(equivalent_medium_method, "ari") == 0) {
        parametrization_layer_el_vti_ari(nx, ny, nz, x3d, y3d, z3d, grid_type, 
            interfaces, c11, c33, c55, c66, c13, rho);
    } else { //default
        fprintf(stderr, "ERROR: Wrong parameterization method %s in media_layer2model_el_vti function, "\
                "if you want to use tti method, please call the media_layer2model_el_aniso function. \n", 
                equivalent_medium_method);        
        fflush(stderr);
        exit(1);
    }

    if (interfaces.elevation != nullptr) delete [] interfaces.elevation;
    if (interfaces.rho != nullptr)       delete [] interfaces.rho;
    if (interfaces.vpv != nullptr)       delete [] interfaces.vpv;
    if (interfaces.vsv != nullptr)       delete [] interfaces.vsv;
    if (interfaces.vpv != nullptr)       delete [] interfaces.vpv;
    if (interfaces.vsv != nullptr)       delete [] interfaces.vsv;
    if (interfaces.eta != nullptr)       delete [] interfaces.eta;
    if (interfaces.rho_grad != nullptr)  delete [] interfaces.rho_grad;
    if (interfaces.vpv_grad != nullptr)  delete [] interfaces.vpv_grad;
    if (interfaces.vsv_grad != nullptr)  delete [] interfaces.vsv_grad;
    if (interfaces.vpv_grad != nullptr)  delete [] interfaces.vpv_grad;
    if (interfaces.vsv_grad != nullptr)  delete [] interfaces.vsv_grad;
    if (interfaces.eta_grad != nullptr)  delete [] interfaces.eta_grad;  
    if (interfaces.rho_pow != nullptr)   delete [] interfaces.rho_pow;
    if (interfaces.vpv_pow != nullptr)   delete [] interfaces.vpv_pow;
    if (interfaces.vsv_pow != nullptr)   delete [] interfaces.vsv_pow;
    if (interfaces.vpv_pow != nullptr)   delete [] interfaces.vpv_pow;
    if (interfaces.vsv_pow != nullptr)   delete [] interfaces.vsv_pow;
    if (interfaces.eta_pow != nullptr)   delete [] interfaces.eta_pow;
    if (interfaces.gamma   != nullptr)   delete [] interfaces.gamma;
    if (interfaces.gamma_pow != nullptr)    delete [] interfaces.gamma_pow;
    if (interfaces.gamma_grad != nullptr)   delete [] interfaces.gamma_grad;
    if (interfaces.epsilon != nullptr)      delete [] interfaces.epsilon;
    if (interfaces.epsilon_pow != nullptr)  delete [] interfaces.epsilon_pow;
    if (interfaces.epsilon_grad != nullptr) delete [] interfaces.epsilon_grad;
    if (interfaces.delta != nullptr)        delete [] interfaces.delta;
    if (interfaces.delta_grad != nullptr)   delete [] interfaces.delta_grad;
    if (interfaces.delta_pow != nullptr)    delete [] interfaces.delta_pow;
    if (interfaces.c11_pow != nullptr) delete [] interfaces.c11_pow;
    if (interfaces.c33_pow != nullptr) delete [] interfaces.c33_pow;
    if (interfaces.c55_pow != nullptr) delete [] interfaces.c55_pow;
    if (interfaces.c66_pow != nullptr) delete [] interfaces.c66_pow;
    if (interfaces.c13_pow != nullptr) delete [] interfaces.c13_pow;

    return 0; 
}

//--- 4. elastic anisotropic/TTI
int media_layer2model_el_aniso(
        float *rho,
        float *c11, float *c12, float *c13,
        float *c14, float *c15, float *c16,
        float *c22, float *c23, float *c24,
        float *c25, float *c26, float *c33,
        float *c34, float *c35, float *c36,
        float *c44, float *c45, float *c46,
        float *c55, float *c56, float *c66,
        const float *x3d,
        const float *y3d,
        const float *z3d,
        size_t nx,
        size_t ny,
        size_t nz,
        int grid_type, 
        const char *in_3lay_file,
        const char *equivalent_medium_method) 
{

    inter_t interfaces;
    int md_type = interfaces.media_type;
    size_t siz_volume = nx*ny*nz;

    /* Read interface file */
    read_interface_file(in_3lay_file, &interfaces);   

    // the function does not support one component and acoustic wave
    // if the 
    if (md_type == ONE_COMPONENT){
        fprintf(stderr, "ERROR: The media_layer2model_el_aniso function dose not"\
                "supports one_component, please check the media_type of %s! \n", in_3lay_file);        
        fflush(stderr);
        exit(1);
    } else if (md_type == ACOUSTIC_ISOTROPIC){
        fprintf(stderr, "ERROR: The media_layer2model_el_aniso function dose not"\
                "supports acoustic_isotropic, please check the media_type of %s! \n", in_3lay_file);        
        fflush(stderr);
        exit(1);
    }
//    } else if ( (md_type == ELASTIC_ISOTROPIC|| md_type == ELASTIC_VTI_PREM || 
//            md_type == ELASTIC_VTI_THOMSEN || md_type == ELASTIC_VTI_CIJ)
//            && (equivalent_medium_method, "tti") != 0) {
//        fprintf(stderr, "ERROR: When the media_type is elastic_isotrpic or elastic_vti_*, "\
//                " the media_layer2model_el_aniso function only supports tti equivalent_medium_method," \
//                "please check the media_type of %s or the equivalent_medium_method! \n", in_3lay_file);        
//        fflush(stderr);
//        exit(1);        
//    }

    //- isotropic: loc, har, tti
    if (md_type == ELASTIC_ISOTROPIC) {

        if (strcmp(equivalent_medium_method,"loc") == 0 || strcmp(equivalent_medium_method,"har") == 0) {
            media_layer2model_el_iso(c13, c44, rho, x3d, y3d, z3d, 
                nx, ny, nz, grid_type, in_3lay_file, equivalent_medium_method);
            for (size_t i = 0; i < siz_volume; i++) {
                c11[i] = c13[i] + 2.0*c44[i]; 
                c22[i] = c11[i]; c33[i] = c11[i]; 
                c12[i] = c13[i]; c23[i] = c13[i];
                c55[i] = c44[i]; c66[i] = c44[i];
                c14[i] = 0.0; c24[i] = 0.0;  c34[i] = 0.0;  
                c15[i] = 0.0; c25[i] = 0.0;  c35[i] = 0.0; 
                c16[i] = 0.0; c26[i] = 0.0;  c36[i] = 0.0;
                c45[i] = 0.0; c46[i] = 0.0;  c56[i] = 0.0;
            }
        } else if(strcmp(equivalent_medium_method,"tti") == 0) {
// TODO: tti equivalent medium method for iso
        } else {
            fprintf(stderr, "ERROR: no such equivalent_medium_method: %s", equivalent_medium_method);        
            fflush(stderr);
            exit(1);        
        }

    } else if (md_type == ELASTIC_VTI_PREM || md_type == ELASTIC_VTI_THOMSEN || md_type == ELASTIC_VTI_CIJ) {

        if (strcmp(equivalent_medium_method,"loc") == 0 || strcmp(equivalent_medium_method,"har") == 0
            || strcmp(equivalent_medium_method,"ari") == 0) {

            media_layer2model_el_vti(rho, c11, c33, c55, c66, c13, x3d, y3d, z3d,
                        nx, ny, nz, grid_type, in_3lay_file, equivalent_medium_method);

            for (size_t i = 0; i < siz_volume; i++) {
                c12[i] = c11[i]-2.0*c66[i];
                c22[i] = c11[i];
                c23[i] = c13[i];
                c44[i] = c55[i];
                c14[i] = 0.0; c24[i] = 0.0;  c34[i] = 0.0;  
                c15[i] = 0.0; c25[i] = 0.0;  c35[i] = 0.0; 
                c16[i] = 0.0; c26[i] = 0.0;  c36[i] = 0.0;
                c45[i] = 0.0; c46[i] = 0.0;  c56[i] = 0.0;
            }

        } else if(strcmp(equivalent_medium_method,"tti") == 0) {
// TODO: tti equivalent medium for vti
        } else {
            fprintf(stderr, "ERROR: no such equivalent_medium_method: %s", equivalent_medium_method);        
            fflush(stderr);
            exit(1);        
        }
    } else {
        if (strcmp(equivalent_medium_method, "loc") == 0) {
            parametrization_layer_el_aniso_loc(nx, ny, nz, x3d, y3d, z3d, grid_type, 
                interfaces, c11, c12, c13, c14, c15, c16, c22, c23, c24, c25, c26, 
                c33, c34, c35, c36, c44, c45, c46, c55, c56, c66, rho);
        } else if (strcmp(equivalent_medium_method, "har") == 0) {
            parametrization_layer_el_aniso_har(nx, ny, nz, x3d, y3d, z3d, grid_type, 
                interfaces, c11, c12, c13, c14, c15, c16, c22, c23, c24, c25, c26, 
                c33, c34, c35, c36, c44, c45, c46, c55, c56, c66, rho);
        } else if (strcmp(equivalent_medium_method, "ari") == 0) {
            parametrization_layer_el_aniso_ari(nx, ny, nz, x3d, y3d, z3d, grid_type, 
                interfaces, c11, c12, c13, c14, c15, c16, c22, c23, c24, c25, c26, 
                c33, c34, c35, c36, c44, c45, c46, c55, c56, c66, rho);
        } else if(strcmp(equivalent_medium_method,"tti") == 0) {
// TODO: tti equivalent medium for tti
        } else { //default
            fprintf(stderr, "ERROR: no such equivalent_medium_method: %s", equivalent_medium_method);        
            fflush(stderr);
            exit(1);        
        }
    }

    if (interfaces.elevation != nullptr) delete [] interfaces.elevation;

    // for elastic
    if (interfaces.vp       != nullptr) delete [] interfaces.vp      ;
    if (interfaces.rho      != nullptr) delete [] interfaces.rho     ;
    if (interfaces.vp_grad  != nullptr) delete [] interfaces.vp_grad ;
    if (interfaces.rho_grad != nullptr) delete [] interfaces.rho_grad;
    if (interfaces.vp_pow   != nullptr) delete [] interfaces.vp_pow  ;
    if (interfaces.rho_pow  != nullptr) delete [] interfaces.rho_pow ;
    if (interfaces.vs       != nullptr) delete [] interfaces.vs      ;
    if (interfaces.vs_grad  != nullptr) delete [] interfaces.vs_grad ;
    if (interfaces.vs_pow   != nullptr) delete [] interfaces.vs_pow  ;
    
    // for anisotropy (thomsen)
    if (interfaces.epsilon      != nullptr) delete [] interfaces.epsilon     ;
    if (interfaces.epsilon_grad != nullptr) delete [] interfaces.epsilon_grad;
    if (interfaces.epsilon_pow  != nullptr) delete [] interfaces.epsilon_pow ;
    if (interfaces.delta        != nullptr) delete [] interfaces.delta       ;
    if (interfaces.delta_grad   != nullptr) delete [] interfaces.delta_grad  ;
    if (interfaces.delta_pow    != nullptr) delete [] interfaces.delta_pow   ;
    if (interfaces.gamma        != nullptr) delete [] interfaces.gamma       ;
    if (interfaces.gamma_grad   != nullptr) delete [] interfaces.gamma_grad  ;
    if (interfaces.gamma_pow    != nullptr) delete [] interfaces.gamma_pow   ;
    if (interfaces.azimuth      != nullptr) delete [] interfaces.azimuth     ;
    if (interfaces.azimuth_grad != nullptr) delete [] interfaces.azimuth_grad;
    if (interfaces.azimuth_pow  != nullptr) delete [] interfaces.azimuth_pow ;
    if (interfaces.dip          != nullptr) delete [] interfaces.dip         ;
    if (interfaces.dip_grad     != nullptr) delete [] interfaces.dip_grad    ;
    if (interfaces.dip_pow      != nullptr) delete [] interfaces.dip_pow     ;
    
    // for vti_prem
    if (interfaces.vph      != nullptr) delete [] interfaces.vph     ;
    if (interfaces.vpv      != nullptr) delete [] interfaces.vpv     ;
    if (interfaces.vsh      != nullptr) delete [] interfaces.vsh     ;
    if (interfaces.vsv      != nullptr) delete [] interfaces.vsv     ;
    if (interfaces.eta      != nullptr) delete [] interfaces.eta     ;
    if (interfaces.vph_grad != nullptr) delete [] interfaces.vph_grad;
    if (interfaces.vpv_grad != nullptr) delete [] interfaces.vpv_grad;
    if (interfaces.vsh_grad != nullptr) delete [] interfaces.vsh_grad;
    if (interfaces.vsv_grad != nullptr) delete [] interfaces.vsv_grad;
    if (interfaces.eta_grad != nullptr) delete [] interfaces.eta_grad;
    if (interfaces.vph_pow  != nullptr) delete [] interfaces.vph_pow ;
    if (interfaces.vpv_pow  != nullptr) delete [] interfaces.vpv_pow ;
    if (interfaces.vsh_pow  != nullptr) delete [] interfaces.vsh_pow ;
    if (interfaces.vsv_pow  != nullptr) delete [] interfaces.vsv_pow ;
    if (interfaces.eta_pow  != nullptr) delete [] interfaces.eta_pow ;
    
    // for thomsen
    if (interfaces.vp0      != nullptr) delete [] interfaces.vp0     ;
    if (interfaces.vs0      != nullptr) delete [] interfaces.vs0     ;
    if (interfaces.vp0_grad != nullptr) delete [] interfaces.vp0_grad;
    if (interfaces.vs0_grad != nullptr) delete [] interfaces.vs0_grad;
    if (interfaces.vp0_pow  != nullptr) delete [] interfaces.vp0_pow ;
    if (interfaces.vs0_pow  != nullptr) delete [] interfaces.vs0_pow ;

    // for anisotropy c_ij, call one component
    if (interfaces.c11 != nullptr) delete [] interfaces.c11;
    if (interfaces.c12 != nullptr) delete [] interfaces.c12;
    if (interfaces.c13 != nullptr) delete [] interfaces.c13;
    if (interfaces.c14 != nullptr) delete [] interfaces.c14;
    if (interfaces.c15 != nullptr) delete [] interfaces.c15;
    if (interfaces.c16 != nullptr) delete [] interfaces.c16;
    if (interfaces.c22 != nullptr) delete [] interfaces.c22;
    if (interfaces.c23 != nullptr) delete [] interfaces.c23;
    if (interfaces.c24 != nullptr) delete [] interfaces.c24;
    if (interfaces.c25 != nullptr) delete [] interfaces.c25;
    if (interfaces.c26 != nullptr) delete [] interfaces.c26;
    if (interfaces.c33 != nullptr) delete [] interfaces.c33;
    if (interfaces.c34 != nullptr) delete [] interfaces.c34;
    if (interfaces.c35 != nullptr) delete [] interfaces.c35;
    if (interfaces.c36 != nullptr) delete [] interfaces.c36;
    if (interfaces.c44 != nullptr) delete [] interfaces.c44;
    if (interfaces.c45 != nullptr) delete [] interfaces.c45;
    if (interfaces.c46 != nullptr) delete [] interfaces.c46;
    if (interfaces.c55 != nullptr) delete [] interfaces.c55;
    if (interfaces.c56 != nullptr) delete [] interfaces.c56;
    if (interfaces.c66 != nullptr) delete [] interfaces.c66;
    if (interfaces.c11_grad != nullptr) delete [] interfaces.c11_grad;
    if (interfaces.c12_grad != nullptr) delete [] interfaces.c12_grad;
    if (interfaces.c13_grad != nullptr) delete [] interfaces.c13_grad;
    if (interfaces.c14_grad != nullptr) delete [] interfaces.c14_grad;
    if (interfaces.c15_grad != nullptr) delete [] interfaces.c15_grad;
    if (interfaces.c16_grad != nullptr) delete [] interfaces.c16_grad;
    if (interfaces.c22_grad != nullptr) delete [] interfaces.c22_grad;
    if (interfaces.c23_grad != nullptr) delete [] interfaces.c23_grad;
    if (interfaces.c24_grad != nullptr) delete [] interfaces.c24_grad;
    if (interfaces.c25_grad != nullptr) delete [] interfaces.c25_grad;
    if (interfaces.c26_grad != nullptr) delete [] interfaces.c26_grad;
    if (interfaces.c33_grad != nullptr) delete [] interfaces.c33_grad;
    if (interfaces.c34_grad != nullptr) delete [] interfaces.c34_grad;
    if (interfaces.c35_grad != nullptr) delete [] interfaces.c35_grad;
    if (interfaces.c36_grad != nullptr) delete [] interfaces.c36_grad;
    if (interfaces.c44_grad != nullptr) delete [] interfaces.c44_grad;
    if (interfaces.c45_grad != nullptr) delete [] interfaces.c45_grad;
    if (interfaces.c46_grad != nullptr) delete [] interfaces.c46_grad;
    if (interfaces.c55_grad != nullptr) delete [] interfaces.c55_grad;
    if (interfaces.c56_grad != nullptr) delete [] interfaces.c56_grad;
    if (interfaces.c66_grad != nullptr) delete [] interfaces.c66_grad;
    if (interfaces.c11_pow != nullptr) delete [] interfaces.c11_pow;
    if (interfaces.c12_pow != nullptr) delete [] interfaces.c12_pow;
    if (interfaces.c13_pow != nullptr) delete [] interfaces.c13_pow;
    if (interfaces.c14_pow != nullptr) delete [] interfaces.c14_pow;
    if (interfaces.c15_pow != nullptr) delete [] interfaces.c15_pow;
    if (interfaces.c16_pow != nullptr) delete [] interfaces.c16_pow;
    if (interfaces.c22_pow != nullptr) delete [] interfaces.c22_pow;
    if (interfaces.c23_pow != nullptr) delete [] interfaces.c23_pow;
    if (interfaces.c24_pow != nullptr) delete [] interfaces.c24_pow;
    if (interfaces.c25_pow != nullptr) delete [] interfaces.c25_pow;
    if (interfaces.c26_pow != nullptr) delete [] interfaces.c26_pow;
    if (interfaces.c33_pow != nullptr) delete [] interfaces.c33_pow;
    if (interfaces.c34_pow != nullptr) delete [] interfaces.c34_pow;
    if (interfaces.c35_pow != nullptr) delete [] interfaces.c35_pow;
    if (interfaces.c36_pow != nullptr) delete [] interfaces.c36_pow;
    if (interfaces.c44_pow != nullptr) delete [] interfaces.c44_pow;
    if (interfaces.c45_pow != nullptr) delete [] interfaces.c45_pow;
    if (interfaces.c46_pow != nullptr) delete [] interfaces.c46_pow;
    if (interfaces.c55_pow != nullptr) delete [] interfaces.c55_pow;
    if (interfaces.c56_pow != nullptr) delete [] interfaces.c56_pow;
    if (interfaces.c66_pow != nullptr) delete [] interfaces.c66_pow;        
    return 0; 
}

/*=================================================================================*/

void PrintIsPointOutOfInterfaceRange(Point3 A, 
    int ix, int iy, int iz, 
    float MINX, float MAXX, float MINY, float MAXY) 
{
    if (A.x < MINX || A.x > MAXX || A.y < MINY || A.y > MAXY) {
        fprintf(stderr,"Error: Grid(%d, %d, %d) = (%f, %f, %f) is out of the INTERFACES MESH (x in [%f %f], y in [%f %f]), "\
                       "please check the interfaces file!\n", ix, iy, iz, A.x, A.y, A.z, MINX, MAXX, MINY, MAXY);
        fflush(stderr);
        exit(1);
    }

}

int AssignLayerMediaPara2Point(
    size_t ix, size_t iy, size_t iz,         /* To print error messages */ 
    Point3 A,  
    inter_t interfaces,
    int media_type,                /* the type can be found in media_utility.hpp */ 
    std::vector<float> &var)
{
    size_t  NI = interfaces.NI;
    size_t  NX = interfaces.NX;
    size_t  NY = interfaces.NY;
    float   DX = interfaces.DX;
    float   DY = interfaces.DY;
    float MINX = interfaces.MINX;
    float MINY = interfaces.MINY;
    float MAXX = MINX + (NX-1)*DX;  
    float MAXY = MINY + (NY-1)*DY;

    size_t interface_slice = NX*NY;

    /* If out of the INTERFACE MESH area, exit! */
    PrintIsPointOutOfInterfaceRange(A, ix, iy, iz, MINX, MAXX, MINY, MAXY);

    std::vector<float> XVEC(NX), YVEC(NY);
    std::vector<float> elevation(NI, -FLT_MAX);
    for (size_t i = 0; i < NX; i++) {
        XVEC[i] = MINX + DX*i;
    }
    for (size_t i = 0; i < NY; i++) {
        YVEC[i] = MINY + DY*i;
    }

    /* 
     * For each interface, interpolate to get the elevation of 
     *  point A at the projection position of the interface mesh.
     */
    for (int ni = 0; ni < NI; ni++) {
        elevation[ni] = BilinearInterpolation(XVEC, YVEC, interfaces.elevation + ni*interface_slice, A.x, A.y);
    }

    /* Find which material_index to use */
    int mi = findLastGreaterEqualIndex(A.z, elevation);

    if (mi == -1) {
        fprintf(stderr,"Warning: z-location of Grid(%li, %li, %li) = (%f, %f, %f) is higher than the given elevation in " \
            "the interfaces file, it assigned by the top medium! \n", ix, iy, iz, A.x, A.y, A.z);
        fflush(stderr);
    }
    
    CalPointValue_layer(media_type, interfaces, interface_slice, XVEC, YVEC, A, elevation, mi, var);

    return 0;
}

//- Calculate the value of the point for different media type (to avoid multiple geometric calculations) 
//   for layer2model
void CalPointValue_layer(int media_type, 
                   inter_t interfaces,
                   size_t slice, 
                   std::vector<float> xvec,  /* interface mesh */
                   std::vector<float> yvec,
                   Point3 A,
                   std::vector<float> elevation, /*the elevation of point A at the projection position of the interface mesh. */
                   int mi,
                   std::vector<float> &var)
{
    float dz = elevation[mi] - A.z;

    switch(media_type)
    {
    case ONE_COMPONENT: /* 0. var */
        /* If grid_z > elevation of top_interface, it given by the medium of top non-zero thickness layer */
        if (mi == -1) {
            mi = findLastGreaterEqualIndex(elevation[0], elevation);
            var[0] = BilinearInterpolation(xvec, yvec, interfaces.var + mi*slice, A.x, A.y); 
        } else {
            float var0      = BilinearInterpolation(xvec, yvec, interfaces.var      + mi*slice , A.x, A.y);
            float var_grad  = BilinearInterpolation(xvec, yvec, interfaces.var_grad + mi*slice , A.x, A.y);
            float var_pow   = BilinearInterpolation(xvec, yvec, interfaces.var_pow  + mi*slice , A.x, A.y);
            var[0]  = var0  + pow(dz, var_pow)* var_grad;
        }
    break;

    case ELASTIC_ISOTROPIC: /*1. rho, vp, vs*/
        if (mi == -1) {
            mi = findLastGreaterEqualIndex(elevation[0], elevation);
            var[0] = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice , A.x, A.y);
            var[1] = BilinearInterpolation(xvec, yvec, interfaces.vp  + mi*slice , A.x, A.y);
            var[2] = BilinearInterpolation(xvec, yvec, interfaces.vs  + mi*slice , A.x, A.y);
        } else {
            float vp       = BilinearInterpolation(xvec, yvec, interfaces.vp      + mi*slice, A.x, A.y);
            float vp_grad  = BilinearInterpolation(xvec, yvec, interfaces.vp_grad + mi*slice, A.x, A.y);
            float vp_pow   = BilinearInterpolation(xvec, yvec, interfaces.vp_pow  + mi*slice, A.x, A.y);
            float vs       = BilinearInterpolation(xvec, yvec, interfaces.vs      + mi*slice, A.x, A.y);
            float vs_grad  = BilinearInterpolation(xvec, yvec, interfaces.vs_grad + mi*slice, A.x, A.y);
            float vs_pow   = BilinearInterpolation(xvec, yvec, interfaces.vs_pow  + mi*slice, A.x, A.y);
            float rho      = BilinearInterpolation(xvec, yvec, interfaces.rho     + mi*slice, A.x, A.y);
            float rho_grad = BilinearInterpolation(xvec, yvec, interfaces.rho_grad+ mi*slice, A.x, A.y);
            float rho_pow  = BilinearInterpolation(xvec, yvec, interfaces.rho_pow + mi*slice, A.x, A.y);
            var[1] = vp  + pow(dz, vp_pow)* vp_grad;
            var[2] = vs  + pow(dz, vs_pow)* vs_grad;
            var[0] = rho + pow(dz,rho_pow)*rho_grad;
        }
    break;

    case ELASTIC_VTI_PREM: /*2. rho, vph, vpv, vsh, vsv, eta */
        if (mi == -1) {
            mi = findLastGreaterEqualIndex(elevation[0], elevation);
            var[0] = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice, A.x, A.y);
            var[1] = BilinearInterpolation(xvec, yvec, interfaces.vph + mi*slice, A.x, A.y);
            var[2] = BilinearInterpolation(xvec, yvec, interfaces.vpv + mi*slice, A.x, A.y);
            var[3] = BilinearInterpolation(xvec, yvec, interfaces.vsh + mi*slice, A.x, A.y);
            var[4] = BilinearInterpolation(xvec, yvec, interfaces.vsv + mi*slice, A.x, A.y);
            var[5] = BilinearInterpolation(xvec, yvec, interfaces.eta + mi*slice, A.x, A.y);     
        } else {
            float vph = BilinearInterpolation(xvec, yvec, interfaces.vph + mi*slice, A.x, A.y);
            float vpv = BilinearInterpolation(xvec, yvec, interfaces.vpv + mi*slice, A.x, A.y);
            float vsh = BilinearInterpolation(xvec, yvec, interfaces.vsh + mi*slice, A.x, A.y);
            float vsv = BilinearInterpolation(xvec, yvec, interfaces.vsv + mi*slice, A.x, A.y);
            float rho = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice, A.x, A.y);
            float eta = BilinearInterpolation(xvec, yvec, interfaces.eta + mi*slice, A.x, A.y);
            float vph_grad = BilinearInterpolation(xvec, yvec, interfaces.vph_grad + mi*slice, A.x, A.y);
            float vpv_grad = BilinearInterpolation(xvec, yvec, interfaces.vpv_grad + mi*slice, A.x, A.y);
            float vsh_grad = BilinearInterpolation(xvec, yvec, interfaces.vsh_grad + mi*slice, A.x, A.y);
            float vsv_grad = BilinearInterpolation(xvec, yvec, interfaces.vsv_grad + mi*slice, A.x, A.y);
            float rho_grad = BilinearInterpolation(xvec, yvec, interfaces.rho_grad + mi*slice, A.x, A.y);
            float eta_grad = BilinearInterpolation(xvec, yvec, interfaces.eta_grad + mi*slice, A.x, A.y);
            float vph_pow  = BilinearInterpolation(xvec, yvec, interfaces.vph_pow  + mi*slice, A.x, A.y);
            float vpv_pow  = BilinearInterpolation(xvec, yvec, interfaces.vpv_pow  + mi*slice, A.x, A.y);
            float vsh_pow  = BilinearInterpolation(xvec, yvec, interfaces.vsh_pow  + mi*slice, A.x, A.y);
            float vsv_pow  = BilinearInterpolation(xvec, yvec, interfaces.vsv_pow  + mi*slice, A.x, A.y);
            float rho_pow  = BilinearInterpolation(xvec, yvec, interfaces.rho_pow  + mi*slice, A.x, A.y);
            float eta_pow  = BilinearInterpolation(xvec, yvec, interfaces.eta_pow  + mi*slice, A.x, A.y);
            var[0] = rho + pow(dz, rho_pow)* rho_grad;
            var[1] = vph + pow(dz, vph_pow)* vph_grad;
            var[2] = vpv + pow(dz, vpv_pow)* vpv_grad;
            var[3] = vsh + pow(dz, vsh_pow)* vsh_grad;
            var[4] = vsv + pow(dz, vsv_pow)* vsv_grad;
            var[5] = eta + pow(dz, eta_pow)* eta_grad;
        }   
    break;

    case ELASTIC_VTI_THOMSEN: /*3. rho, vp0, vs0, epsilon, delta, gamma */
        if (mi == -1) {
            mi = findLastGreaterEqualIndex(elevation[0], elevation);
            var[0] = BilinearInterpolation(xvec, yvec, interfaces.rho    + mi*slice , A.x, A.y);
            var[1] = BilinearInterpolation(xvec, yvec, interfaces.vp0    + mi*slice , A.x, A.y);
            var[2] = BilinearInterpolation(xvec, yvec, interfaces.vs0    + mi*slice , A.x, A.y);
            var[3] = BilinearInterpolation(xvec, yvec, interfaces.epsilon+ mi*slice , A.x, A.y);
            var[4] = BilinearInterpolation(xvec, yvec, interfaces.delta  + mi*slice , A.x, A.y);
            var[5] = BilinearInterpolation(xvec, yvec, interfaces.gamma  + mi*slice , A.x, A.y);    
        } else {
            float rho     = BilinearInterpolation(xvec, yvec, interfaces.rho     + mi*slice, A.x, A.y);
            float vp0     = BilinearInterpolation(xvec, yvec, interfaces.vp0     + mi*slice, A.x, A.y);
            float vs0     = BilinearInterpolation(xvec, yvec, interfaces.vs0     + mi*slice, A.x, A.y);
            float epsil   = BilinearInterpolation(xvec, yvec, interfaces.epsilon + mi*slice, A.x, A.y);  // epsilon
            float delta   = BilinearInterpolation(xvec, yvec, interfaces.delta   + mi*slice, A.x, A.y);
            float gamma   = BilinearInterpolation(xvec, yvec, interfaces.gamma   + mi*slice, A.x, A.y);
            float rho_grad     = BilinearInterpolation(xvec, yvec, interfaces.rho_grad     + mi*slice, A.x, A.y);
            float vp0_grad     = BilinearInterpolation(xvec, yvec, interfaces.vp0_grad     + mi*slice, A.x, A.y);
            float vs0_grad     = BilinearInterpolation(xvec, yvec, interfaces.vs0_grad     + mi*slice, A.x, A.y);
            float epsilon_grad = BilinearInterpolation(xvec, yvec, interfaces.epsilon_grad + mi*slice, A.x, A.y);
            float delta_grad   = BilinearInterpolation(xvec, yvec, interfaces.delta_grad   + mi*slice, A.x, A.y);
            float gamma_grad   = BilinearInterpolation(xvec, yvec, interfaces.gamma_grad   + mi*slice, A.x, A.y);
            float rho_pow      = BilinearInterpolation(xvec, yvec, interfaces.rho_pow      + mi*slice, A.x, A.y);
            float vp0_pow      = BilinearInterpolation(xvec, yvec, interfaces.vp0_pow      + mi*slice, A.x, A.y);
            float vs0_pow      = BilinearInterpolation(xvec, yvec, interfaces.vs0_pow      + mi*slice, A.x, A.y);
            float epsilon_pow  = BilinearInterpolation(xvec, yvec, interfaces.epsilon_pow  + mi*slice, A.x, A.y);
            float delta_pow    = BilinearInterpolation(xvec, yvec, interfaces.delta_pow    + mi*slice, A.x, A.y);
            float gamma_pow    = BilinearInterpolation(xvec, yvec, interfaces.gamma_pow    + mi*slice, A.x, A.y);
            var[0] = rho     + pow(dz, rho_pow)    * rho_grad;
            var[1] = vp0     + pow(dz, vp0_pow)    * vp0_grad;
            var[2] = vs0     + pow(dz, vs0_pow)    * vs0_grad;
            var[3] = epsil   + pow(dz, epsilon_pow)* epsilon_grad;
            var[4] = delta   + pow(dz, delta_pow)  * delta_grad;
            var[5] = gamma   + pow(dz, gamma_pow)  * gamma_grad;
        }   
    break;

    case ELASTIC_VTI_CIJ: /*4. rho c11 c33 c55 c66 c13 */
         if (mi == -1) {
            mi = findLastGreaterEqualIndex(elevation[0], elevation);
            var[0] = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice, A.x, A.y);
            var[1] = BilinearInterpolation(xvec, yvec, interfaces.c11 + mi*slice, A.x, A.y);
            var[2] = BilinearInterpolation(xvec, yvec, interfaces.c33 + mi*slice, A.x, A.y);
            var[3] = BilinearInterpolation(xvec, yvec, interfaces.c55 + mi*slice, A.x, A.y);
            var[4] = BilinearInterpolation(xvec, yvec, interfaces.c66 + mi*slice, A.x, A.y);
            var[5] = BilinearInterpolation(xvec, yvec, interfaces.c13 + mi*slice, A.x, A.y);   
        } else {
            float rho = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice, A.x, A.y);
            float c11 = BilinearInterpolation(xvec, yvec, interfaces.c11 + mi*slice, A.x, A.y);
            float c33 = BilinearInterpolation(xvec, yvec, interfaces.c33 + mi*slice, A.x, A.y);
            float c55 = BilinearInterpolation(xvec, yvec, interfaces.c55 + mi*slice, A.x, A.y);
            float c66 = BilinearInterpolation(xvec, yvec, interfaces.c66 + mi*slice, A.x, A.y);
            float c13 = BilinearInterpolation(xvec, yvec, interfaces.c13 + mi*slice, A.x, A.y);
            float c11_grad = BilinearInterpolation(xvec, yvec, interfaces.c11_grad + mi*slice, A.x, A.y);
            float c33_grad = BilinearInterpolation(xvec, yvec, interfaces.c33_grad + mi*slice, A.x, A.y);
            float c55_grad = BilinearInterpolation(xvec, yvec, interfaces.c55_grad + mi*slice, A.x, A.y);
            float c66_grad = BilinearInterpolation(xvec, yvec, interfaces.c66_grad + mi*slice, A.x, A.y);
            float c13_grad = BilinearInterpolation(xvec, yvec, interfaces.c13_grad + mi*slice, A.x, A.y);
            float rho_grad = BilinearInterpolation(xvec, yvec, interfaces.rho_grad + mi*slice, A.x, A.y);
            float c11_pow  = BilinearInterpolation(xvec, yvec, interfaces.c11_pow  + mi*slice, A.x, A.y);
            float c33_pow  = BilinearInterpolation(xvec, yvec, interfaces.c33_pow  + mi*slice, A.x, A.y);
            float c55_pow  = BilinearInterpolation(xvec, yvec, interfaces.c55_pow  + mi*slice, A.x, A.y);
            float c66_pow  = BilinearInterpolation(xvec, yvec, interfaces.c66_pow  + mi*slice, A.x, A.y);
            float c13_pow  = BilinearInterpolation(xvec, yvec, interfaces.c13_pow  + mi*slice, A.x, A.y);
            float rho_pow  = BilinearInterpolation(xvec, yvec, interfaces.rho_pow  + mi*slice, A.x, A.y);
            var[0] = rho + pow(dz, rho_pow)* rho_grad;
            var[1] = c11 + pow(dz, c11_pow)* c11_grad;
            var[2] = c33 + pow(dz, c33_pow)* c33_grad;
            var[3] = c55 + pow(dz, c55_pow)* c55_grad;
            var[4] = c66 + pow(dz, c66_pow)* c66_grad;
            var[5] = c13 + pow(dz, c13_pow)* c13_grad;
        }   
    break;

    case ELASTIC_TTI_THOMSEN: /*5. rho, vp0, vs0, epsilon, delta, gamma, azimuth, dip */
        if (mi == -1) {
            mi = findLastGreaterEqualIndex(elevation[0], elevation);
            var[0] = BilinearInterpolation(xvec, yvec, interfaces.rho     + mi*slice , A.x, A.y);
            var[1] = BilinearInterpolation(xvec, yvec, interfaces.vp0     + mi*slice , A.x, A.y);
            var[2] = BilinearInterpolation(xvec, yvec, interfaces.vs0     + mi*slice , A.x, A.y);
            var[3] = BilinearInterpolation(xvec, yvec, interfaces.epsilon + mi*slice , A.x, A.y);
            var[4] = BilinearInterpolation(xvec, yvec, interfaces.delta   + mi*slice , A.x, A.y);
            var[5] = BilinearInterpolation(xvec, yvec, interfaces.gamma   + mi*slice , A.x, A.y);
            var[6] = BilinearInterpolation(xvec, yvec, interfaces.azimuth + mi*slice , A.x, A.y);
            var[7] = BilinearInterpolation(xvec, yvec, interfaces.dip     + mi*slice , A.x, A.y);
        } else {
            float vp0     = BilinearInterpolation(xvec, yvec, interfaces.vp0     + mi*slice, A.x, A.y);
            float vs0     = BilinearInterpolation(xvec, yvec, interfaces.vs0     + mi*slice, A.x, A.y);
            float epsil   = BilinearInterpolation(xvec, yvec, interfaces.epsilon + mi*slice, A.x, A.y);
            float delta   = BilinearInterpolation(xvec, yvec, interfaces.delta   + mi*slice, A.x, A.y);
            float gamma   = BilinearInterpolation(xvec, yvec, interfaces.gamma   + mi*slice, A.x, A.y);
            float rho     = BilinearInterpolation(xvec, yvec, interfaces.rho     + mi*slice, A.x, A.y);
            float azimuth = BilinearInterpolation(xvec, yvec, interfaces.azimuth + mi*slice, A.x, A.y);
            float dip     = BilinearInterpolation(xvec, yvec, interfaces.dip     + mi*slice, A.x, A.y);
            float vp0_grad     = BilinearInterpolation(xvec, yvec, interfaces.vp0_grad     + mi*slice, A.x, A.y);
            float vs0_grad     = BilinearInterpolation(xvec, yvec, interfaces.vs0_grad     + mi*slice, A.x, A.y);
            float epsilon_grad = BilinearInterpolation(xvec, yvec, interfaces.epsilon_grad + mi*slice, A.x, A.y);
            float delta_grad   = BilinearInterpolation(xvec, yvec, interfaces.delta_grad   + mi*slice, A.x, A.y);
            float gamma_grad   = BilinearInterpolation(xvec, yvec, interfaces.gamma_grad   + mi*slice, A.x, A.y);
            float rho_grad     = BilinearInterpolation(xvec, yvec, interfaces.rho_grad     + mi*slice, A.x, A.y);
            float azimuth_grad = BilinearInterpolation(xvec, yvec, interfaces.azimuth_grad + mi*slice, A.x, A.y);
            float dip_grad     = BilinearInterpolation(xvec, yvec, interfaces.dip_grad     + mi*slice, A.x, A.y);
            float vp0_pow      = BilinearInterpolation(xvec, yvec, interfaces.vp0_pow      + mi*slice, A.x, A.y);
            float vs0_pow      = BilinearInterpolation(xvec, yvec, interfaces.vs0_pow      + mi*slice, A.x, A.y);
            float epsilon_pow  = BilinearInterpolation(xvec, yvec, interfaces.epsilon_pow  + mi*slice, A.x, A.y);
            float delta_pow    = BilinearInterpolation(xvec, yvec, interfaces.delta_pow    + mi*slice, A.x, A.y);
            float gamma_pow    = BilinearInterpolation(xvec, yvec, interfaces.gamma_pow    + mi*slice, A.x, A.y);
            float rho_pow      = BilinearInterpolation(xvec, yvec, interfaces.rho_pow      + mi*slice, A.x, A.y);
            float azimuth_pow  = BilinearInterpolation(xvec, yvec, interfaces.azimuth_pow  + mi*slice, A.x, A.y);
            float dip_pow      = BilinearInterpolation(xvec, yvec, interfaces.dip_pow      + mi*slice, A.x, A.y);

            var[0] = rho     + pow(dz, rho_pow)    * rho_grad;
            var[1] = vp0     + pow(dz, vp0_pow)    * vp0_grad;
            var[2] = vs0     + pow(dz, vs0_pow)    * vs0_grad;
            var[3] = epsil   + pow(dz, epsilon_pow)* epsilon_grad;
            var[4] = delta   + pow(dz, delta_pow)  * delta_grad;
            var[5] = gamma   + pow(dz, gamma_pow)  * gamma_grad;
            var[6] = azimuth + pow(dz, azimuth_pow)* azimuth_grad;
            var[7] = dip     + pow(dz, dip_pow)    * dip_grad;
        }   
    break;

    case ELASTIC_ANISO_CIJ: /* 7. rho c11 c12 c13 c14 c15 c16 c22 c23 c24 c25 c26 c33 c34 c35 c36 c44 c45 c46 c55 c56 c66 */
         if (mi == -1) {
            mi = findLastGreaterEqualIndex(elevation[0], elevation);
            var[0]  = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice, A.x, A.y);
            var[1]  = BilinearInterpolation(xvec, yvec, interfaces.c11 + mi*slice, A.x, A.y);
            var[2]  = BilinearInterpolation(xvec, yvec, interfaces.c12 + mi*slice, A.x, A.y);
            var[3]  = BilinearInterpolation(xvec, yvec, interfaces.c13 + mi*slice, A.x, A.y);
            var[4]  = BilinearInterpolation(xvec, yvec, interfaces.c14 + mi*slice, A.x, A.y);
            var[5]  = BilinearInterpolation(xvec, yvec, interfaces.c15 + mi*slice, A.x, A.y);
            var[6]  = BilinearInterpolation(xvec, yvec, interfaces.c16 + mi*slice, A.x, A.y);
            var[7]  = BilinearInterpolation(xvec, yvec, interfaces.c22 + mi*slice, A.x, A.y);
            var[8]  = BilinearInterpolation(xvec, yvec, interfaces.c23 + mi*slice, A.x, A.y);
            var[9]  = BilinearInterpolation(xvec, yvec, interfaces.c24 + mi*slice, A.x, A.y);
            var[10] = BilinearInterpolation(xvec, yvec, interfaces.c25 + mi*slice, A.x, A.y);
            var[11] = BilinearInterpolation(xvec, yvec, interfaces.c26 + mi*slice, A.x, A.y);
            var[12] = BilinearInterpolation(xvec, yvec, interfaces.c33 + mi*slice, A.x, A.y);
            var[13] = BilinearInterpolation(xvec, yvec, interfaces.c34 + mi*slice, A.x, A.y);
            var[14] = BilinearInterpolation(xvec, yvec, interfaces.c35 + mi*slice, A.x, A.y);
            var[15] = BilinearInterpolation(xvec, yvec, interfaces.c36 + mi*slice, A.x, A.y);
            var[16] = BilinearInterpolation(xvec, yvec, interfaces.c44 + mi*slice, A.x, A.y);
            var[17] = BilinearInterpolation(xvec, yvec, interfaces.c45 + mi*slice, A.x, A.y);
            var[18] = BilinearInterpolation(xvec, yvec, interfaces.c46 + mi*slice, A.x, A.y);
            var[19] = BilinearInterpolation(xvec, yvec, interfaces.c55 + mi*slice, A.x, A.y);
            var[20] = BilinearInterpolation(xvec, yvec, interfaces.c56 + mi*slice, A.x, A.y);
            var[21] = BilinearInterpolation(xvec, yvec, interfaces.c66 + mi*slice, A.x, A.y);
        } else {
            float c11 = BilinearInterpolation(xvec, yvec, interfaces.c11 + mi*slice, A.x, A.y);
            float c12 = BilinearInterpolation(xvec, yvec, interfaces.c12 + mi*slice, A.x, A.y);
            float c13 = BilinearInterpolation(xvec, yvec, interfaces.c13 + mi*slice, A.x, A.y);
            float c14 = BilinearInterpolation(xvec, yvec, interfaces.c14 + mi*slice, A.x, A.y);
            float c15 = BilinearInterpolation(xvec, yvec, interfaces.c15 + mi*slice, A.x, A.y);
            float c16 = BilinearInterpolation(xvec, yvec, interfaces.c16 + mi*slice, A.x, A.y);
            float c22 = BilinearInterpolation(xvec, yvec, interfaces.c22 + mi*slice, A.x, A.y);
            float c23 = BilinearInterpolation(xvec, yvec, interfaces.c23 + mi*slice, A.x, A.y);
            float c24 = BilinearInterpolation(xvec, yvec, interfaces.c24 + mi*slice, A.x, A.y);
            float c25 = BilinearInterpolation(xvec, yvec, interfaces.c25 + mi*slice, A.x, A.y);
            float c26 = BilinearInterpolation(xvec, yvec, interfaces.c26 + mi*slice, A.x, A.y);
            float c33 = BilinearInterpolation(xvec, yvec, interfaces.c33 + mi*slice, A.x, A.y);
            float c34 = BilinearInterpolation(xvec, yvec, interfaces.c34 + mi*slice, A.x, A.y);
            float c35 = BilinearInterpolation(xvec, yvec, interfaces.c35 + mi*slice, A.x, A.y);
            float c36 = BilinearInterpolation(xvec, yvec, interfaces.c36 + mi*slice, A.x, A.y);
            float c44 = BilinearInterpolation(xvec, yvec, interfaces.c44 + mi*slice, A.x, A.y);
            float c45 = BilinearInterpolation(xvec, yvec, interfaces.c45 + mi*slice, A.x, A.y);
            float c46 = BilinearInterpolation(xvec, yvec, interfaces.c46 + mi*slice, A.x, A.y);
            float c55 = BilinearInterpolation(xvec, yvec, interfaces.c55 + mi*slice, A.x, A.y);
            float c56 = BilinearInterpolation(xvec, yvec, interfaces.c56 + mi*slice, A.x, A.y);
            float c66 = BilinearInterpolation(xvec, yvec, interfaces.c66 + mi*slice, A.x, A.y);
            float rho = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice, A.x, A.y);
            float c11_pow = BilinearInterpolation(xvec, yvec, interfaces.c11_pow + mi*slice , A.x, A.y);
            float c12_pow = BilinearInterpolation(xvec, yvec, interfaces.c12_pow + mi*slice , A.x, A.y);
            float c13_pow = BilinearInterpolation(xvec, yvec, interfaces.c13_pow + mi*slice , A.x, A.y);
            float c14_pow = BilinearInterpolation(xvec, yvec, interfaces.c14_pow + mi*slice , A.x, A.y);
            float c15_pow = BilinearInterpolation(xvec, yvec, interfaces.c15_pow + mi*slice , A.x, A.y);
            float c16_pow = BilinearInterpolation(xvec, yvec, interfaces.c16_pow + mi*slice , A.x, A.y);
            float c22_pow = BilinearInterpolation(xvec, yvec, interfaces.c22_pow + mi*slice , A.x, A.y);
            float c23_pow = BilinearInterpolation(xvec, yvec, interfaces.c23_pow + mi*slice , A.x, A.y);
            float c24_pow = BilinearInterpolation(xvec, yvec, interfaces.c24_pow + mi*slice , A.x, A.y);
            float c25_pow = BilinearInterpolation(xvec, yvec, interfaces.c25_pow + mi*slice , A.x, A.y);
            float c26_pow = BilinearInterpolation(xvec, yvec, interfaces.c26_pow + mi*slice , A.x, A.y);
            float c33_pow = BilinearInterpolation(xvec, yvec, interfaces.c33_pow + mi*slice , A.x, A.y);
            float c34_pow = BilinearInterpolation(xvec, yvec, interfaces.c34_pow + mi*slice , A.x, A.y);
            float c35_pow = BilinearInterpolation(xvec, yvec, interfaces.c35_pow + mi*slice , A.x, A.y);
            float c36_pow = BilinearInterpolation(xvec, yvec, interfaces.c36_pow + mi*slice , A.x, A.y);
            float c44_pow = BilinearInterpolation(xvec, yvec, interfaces.c44_pow + mi*slice , A.x, A.y);
            float c45_pow = BilinearInterpolation(xvec, yvec, interfaces.c45_pow + mi*slice , A.x, A.y);
            float c46_pow = BilinearInterpolation(xvec, yvec, interfaces.c46_pow + mi*slice , A.x, A.y);
            float c55_pow = BilinearInterpolation(xvec, yvec, interfaces.c55_pow + mi*slice , A.x, A.y);
            float c56_pow = BilinearInterpolation(xvec, yvec, interfaces.c56_pow + mi*slice , A.x, A.y);
            float c66_pow = BilinearInterpolation(xvec, yvec, interfaces.c66_pow + mi*slice , A.x, A.y);
            float rho_pow = BilinearInterpolation(xvec, yvec, interfaces.rho_pow + mi*slice , A.x, A.y);
            float c11_grad = BilinearInterpolation(xvec, yvec, interfaces.c11_grad + mi*slice , A.x, A.y);
            float c12_grad = BilinearInterpolation(xvec, yvec, interfaces.c12_grad + mi*slice , A.x, A.y);
            float c13_grad = BilinearInterpolation(xvec, yvec, interfaces.c13_grad + mi*slice , A.x, A.y);
            float c14_grad = BilinearInterpolation(xvec, yvec, interfaces.c14_grad + mi*slice , A.x, A.y);
            float c15_grad = BilinearInterpolation(xvec, yvec, interfaces.c15_grad + mi*slice , A.x, A.y);
            float c16_grad = BilinearInterpolation(xvec, yvec, interfaces.c16_grad + mi*slice , A.x, A.y);
            float c22_grad = BilinearInterpolation(xvec, yvec, interfaces.c22_grad + mi*slice , A.x, A.y);
            float c23_grad = BilinearInterpolation(xvec, yvec, interfaces.c23_grad + mi*slice , A.x, A.y);
            float c24_grad = BilinearInterpolation(xvec, yvec, interfaces.c24_grad + mi*slice , A.x, A.y);
            float c25_grad = BilinearInterpolation(xvec, yvec, interfaces.c25_grad + mi*slice , A.x, A.y);
            float c26_grad = BilinearInterpolation(xvec, yvec, interfaces.c26_grad + mi*slice , A.x, A.y);
            float c33_grad = BilinearInterpolation(xvec, yvec, interfaces.c33_grad + mi*slice , A.x, A.y);
            float c34_grad = BilinearInterpolation(xvec, yvec, interfaces.c34_grad + mi*slice , A.x, A.y);
            float c35_grad = BilinearInterpolation(xvec, yvec, interfaces.c35_grad + mi*slice , A.x, A.y);
            float c36_grad = BilinearInterpolation(xvec, yvec, interfaces.c36_grad + mi*slice , A.x, A.y);
            float c44_grad = BilinearInterpolation(xvec, yvec, interfaces.c44_grad + mi*slice , A.x, A.y);
            float c45_grad = BilinearInterpolation(xvec, yvec, interfaces.c45_grad + mi*slice , A.x, A.y);
            float c46_grad = BilinearInterpolation(xvec, yvec, interfaces.c46_grad + mi*slice , A.x, A.y);
            float c55_grad = BilinearInterpolation(xvec, yvec, interfaces.c55_grad + mi*slice , A.x, A.y);
            float c56_grad = BilinearInterpolation(xvec, yvec, interfaces.c56_grad + mi*slice , A.x, A.y);
            float c66_grad = BilinearInterpolation(xvec, yvec, interfaces.c66_grad + mi*slice , A.x, A.y);
            float rho_grad = BilinearInterpolation(xvec, yvec, interfaces.rho_grad + mi*slice , A.x, A.y);
            var[0]  = rho + pow(dz, rho_pow) * rho_grad;
            var[1]  = c11 + pow(dz, c11_pow) * c11_grad; 
            var[2]  = c12 + pow(dz, c12_pow) * c12_grad;
            var[3]  = c13 + pow(dz, c13_pow) * c13_grad;
            var[4]  = c14 + pow(dz, c14_pow) * c14_grad;
            var[5]  = c15 + pow(dz, c15_pow) * c15_grad;
            var[6]  = c16 + pow(dz, c16_pow) * c16_grad;
            var[7]  = c22 + pow(dz, c22_pow) * c22_grad;
            var[8]  = c23 + pow(dz, c23_pow) * c23_grad;
            var[9]  = c24 + pow(dz, c24_pow) * c24_grad;
            var[10] = c25 + pow(dz, c25_pow) * c25_grad;
            var[11] = c26 + pow(dz, c26_pow) * c26_grad;
            var[12] = c33 + pow(dz, c33_pow) * c33_grad;
            var[13] = c34 + pow(dz, c34_pow) * c34_grad;
            var[14] = c35 + pow(dz, c35_pow) * c35_grad;
            var[15] = c36 + pow(dz, c36_pow) * c36_grad;
            var[16] = c44 + pow(dz, c44_pow) * c44_grad;
            var[17] = c45 + pow(dz, c45_pow) * c45_grad;
            var[18] = c46 + pow(dz, c46_pow) * c46_grad;
            var[19] = c55 + pow(dz, c55_pow) * c55_grad;
            var[20] = c56 + pow(dz, c56_pow) * c56_grad;
            var[21] = c66 + pow(dz, c66_pow) * c66_grad;
        }     
    break;

    case ELASTIC_TTI_BOND: /* 6. rho c11 c33 c55 c66 c13 azimuth dip */
        if (mi == -1) {
            mi = findLastGreaterEqualIndex(elevation[0], elevation);
            var[0] = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice, A.x, A.y);
            var[1] = BilinearInterpolation(xvec, yvec, interfaces.c11 + mi*slice, A.x, A.y);
            var[2] = BilinearInterpolation(xvec, yvec, interfaces.c33 + mi*slice, A.x, A.y);
            var[3] = BilinearInterpolation(xvec, yvec, interfaces.c55 + mi*slice, A.x, A.y);
            var[4] = BilinearInterpolation(xvec, yvec, interfaces.c66 + mi*slice, A.x, A.y);
            var[5] = BilinearInterpolation(xvec, yvec, interfaces.c13 + mi*slice, A.x, A.y);
            var[6] = BilinearInterpolation(xvec, yvec, interfaces.azimuth + mi*slice, A.x, A.y);
            var[7] = BilinearInterpolation(xvec, yvec, interfaces.dip     + mi*slice, A.x, A.y);
        } else {
            float c11 = BilinearInterpolation(xvec, yvec, interfaces.c11 + mi*slice, A.x, A.y);
            float c33 = BilinearInterpolation(xvec, yvec, interfaces.c33 + mi*slice, A.x, A.y);
            float c55 = BilinearInterpolation(xvec, yvec, interfaces.c55 + mi*slice, A.x, A.y);
            float c66 = BilinearInterpolation(xvec, yvec, interfaces.c66 + mi*slice, A.x, A.y);
            float c13 = BilinearInterpolation(xvec, yvec, interfaces.c13 + mi*slice, A.x, A.y);
            float rho = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice, A.x, A.y);
            float c11_grad = BilinearInterpolation(xvec, yvec, interfaces.c11_grad + mi*slice, A.x, A.y);
            float c33_grad = BilinearInterpolation(xvec, yvec, interfaces.c33_grad + mi*slice, A.x, A.y);
            float c55_grad = BilinearInterpolation(xvec, yvec, interfaces.c55_grad + mi*slice, A.x, A.y);
            float c66_grad = BilinearInterpolation(xvec, yvec, interfaces.c66_grad + mi*slice, A.x, A.y);
            float c13_grad = BilinearInterpolation(xvec, yvec, interfaces.c13_grad + mi*slice, A.x, A.y);
            float rho_grad = BilinearInterpolation(xvec, yvec, interfaces.rho_grad + mi*slice, A.x, A.y);
            float c11_pow  = BilinearInterpolation(xvec, yvec, interfaces.c11_pow  + mi*slice, A.x, A.y);
            float c33_pow  = BilinearInterpolation(xvec, yvec, interfaces.c33_pow  + mi*slice, A.x, A.y);
            float c55_pow  = BilinearInterpolation(xvec, yvec, interfaces.c55_pow  + mi*slice, A.x, A.y);
            float c66_pow  = BilinearInterpolation(xvec, yvec, interfaces.c66_pow  + mi*slice, A.x, A.y);
            float c13_pow  = BilinearInterpolation(xvec, yvec, interfaces.c13_pow  + mi*slice, A.x, A.y);
            float rho_pow  = BilinearInterpolation(xvec, yvec, interfaces.rho_pow  + mi*slice, A.x, A.y);
            float azimuth      = BilinearInterpolation(xvec, yvec, interfaces.azimuth      + mi*slice, A.x, A.y);
            float dip          = BilinearInterpolation(xvec, yvec, interfaces.dip          + mi*slice, A.x, A.y);
            float azimuth_grad = BilinearInterpolation(xvec, yvec, interfaces.azimuth_grad + mi*slice, A.x, A.y);
            float dip_grad     = BilinearInterpolation(xvec, yvec, interfaces.dip_grad     + mi*slice, A.x, A.y);
            float azimuth_pow  = BilinearInterpolation(xvec, yvec, interfaces.azimuth_pow  + mi*slice, A.x, A.y);
            float dip_pow      = BilinearInterpolation(xvec, yvec, interfaces.dip_pow      + mi*slice, A.x, A.y);
            var[0] = rho + pow(dz, rho_pow)* rho_grad;
            var[1] = c11 + pow(dz, c11_pow)* c11_grad;
            var[2] = c33 + pow(dz, c33_pow)* c33_grad;
            var[3] = c55 + pow(dz, c55_pow)* c55_grad;
            var[4] = c66 + pow(dz, c66_pow)* c66_grad;
            var[5] = c13 + pow(dz, c13_pow)* c13_grad;
            var[6] = azimuth + pow(dz, azimuth_pow)* azimuth_grad;
            var[7] = dip     + pow(dz, dip_pow)    * dip_grad;
        }   
    break;

    case ACOUSTIC_ISOTROPIC: /* 7. rho vp */
        if (mi == -1) {
            mi = findLastGreaterEqualIndex(elevation[0], elevation);
            var[0] = BilinearInterpolation(xvec, yvec, interfaces.rho + mi*slice, A.x, A.y);
            var[1] = BilinearInterpolation(xvec, yvec, interfaces.vp  + mi*slice, A.x, A.y);
        } else {
            float vp_grad  = BilinearInterpolation(xvec, yvec, interfaces.vp_grad + mi*slice, A.x, A.y);
            float vp       = BilinearInterpolation(xvec, yvec, interfaces.vp      + mi*slice, A.x, A.y);
            float vp_pow   = BilinearInterpolation(xvec, yvec, interfaces.vp_pow  + mi*slice, A.x, A.y);
            float rho      = BilinearInterpolation(xvec, yvec, interfaces.rho     + mi*slice, A.x, A.y);
            float rho_grad = BilinearInterpolation(xvec, yvec, interfaces.rho_grad+ mi*slice, A.x, A.y);
            float rho_pow  = BilinearInterpolation(xvec, yvec, interfaces.rho_pow + mi*slice, A.x, A.y);
            var[0] = rho + pow(dz,rho_pow)*rho_grad;
            var[1] = vp  + pow(dz, vp_pow)* vp_grad;
        }
    break;

    default: // for self-check
        fprintf(stderr,"Error: Unknow meida, please check the code! (for code check, please contact Luqian Jiang)\n");
        fflush(stderr);
        exit(1);

    }     
} 

//- 0. assign the parameter directly (use the local values): one component 
void parametrization_layer_onecmp_loc(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type,
    inter_t interfaces,
    float *var3d)
{
    size_t siz_line   = nx; 
    size_t siz_slice  = nx * ny; 
    size_t siz_volume = nx * ny * nz;

    for (size_t k = 0; k < nz; k++) {
        for (size_t j = 0; j < ny; j++) {
            for (size_t i = 0; i < nx; i++) {
                
                std::vector<float> var(1, 0.0);

                size_t indx =  i + j * siz_line + k * siz_slice;
                size_t indx_z = indx;          // for vmap and curv: z
                size_t indx_x = i, indx_y = j; // for vmap and cart: x, y

                if (grid_type == GRID_CART) {
                    indx_z = k;                // for cart: z
                } else if (grid_type == GRID_CURV) {
                    indx_x = indx;
                    indx_y = indx;             // for curv: x, y
                }
                
                AssignLayerMediaPara2Point(i, j, k,
                        Point3(Gridx[indx_x], Gridy[indx_y], Gridz[indx_z]), 
                        interfaces, ONE_COMPONENT, var);
    
                var3d[indx] = var[0];     
            }
        }
    }

}

//- 1. assign the parameter directly (use the local values): isotropic, acoustic 
void parametrization_layer_ac_iso_loc(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type, 
    inter_t interfaces,
    float *kappa,
    float *rho3d)
{
    size_t siz_line   = nx; 
    size_t siz_slice  = nx * ny; 
    size_t siz_volume = nx * ny * nz;

    for (size_t k = 0; k < nz; k++) {
        for (size_t j = 0; j < ny; j++) {
            for (size_t i = 0; i < nx; i++) {

                std::vector<float> var(2, 0.0); // rho, vp

                size_t indx =  i + j * siz_line + k * siz_slice;

                size_t indx_z = indx;          // for vmap and curv: z
                size_t indx_x = i, indx_y = j; // for vmap and cart: x, y

                if (grid_type == GRID_CART) {
                    indx_z = k;                // for cart: z
                } else if (grid_type == GRID_CURV) {
                    indx_x = indx;
                    indx_y = indx;             // for curv: x, y
                }

                AssignLayerMediaPara2Point(i, j, k,
                        Point3(Gridx[indx_x], Gridy[indx_y], Gridz[indx_z]), 
                        interfaces, ACOUSTIC_ISOTROPIC, var);

                /*calculate output kappa */
                float vp = var[1], rho = var[0];
                kappa[indx] = vp*vp*rho;
                rho3d[indx] = rho; 
            }
        }
    }
}


//- 2. assign the parameter directly (use the local values): elastic isotropic 
void parametrization_layer_el_iso_loc(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type, 
    inter_t interfaces,
    float *lam3d,
    float *mu3d,
    float *rho3d)
{
    size_t siz_line   = nx; 
    size_t siz_slice  = nx * ny; 
    size_t siz_volume = nx * ny * nz;

    for (size_t k = 0; k < nz; k++) {
        for (size_t j = 0; j < ny; j++) {
            for (size_t i = 0; i < nx; i++) {

                std::vector<float> var(3, 0.0); // rho, vp, vs

                size_t indx =  i + j * siz_line + k * siz_slice;

                size_t indx_z = indx;          // for vmap and curv: z
                size_t indx_x = i, indx_y = j; // for vmap and cart: x, y
                if (grid_type == GRID_CART) {
                    indx_z = k;                // for cart: z
                } else if (grid_type == GRID_CURV) {
                    indx_x = indx;
                    indx_y = indx;             // for curv: x, y
                }

                AssignLayerMediaPara2Point(i, j, k,
                        Point3(Gridx[indx_x], Gridy[indx_y], Gridz[indx_z]), 
                        interfaces, ELASTIC_ISOTROPIC, var);

                /*calculate output lambda and mu */
                float vp = var[1], vs = var[2], rho = var[0];
                mu3d[indx]  = vs*vs*rho;
                lam3d[indx] = vp*vp*rho - 2.0*mu3d[indx];
                rho3d[indx] = rho; 
                
            }
        }
    }
}


//- 3. Assign the parameter directly (use the local values): elastic vti
void parametrization_layer_el_vti_loc(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type, 
    inter_t interfaces,
    float *c11,
    float *c33,
    float *c55,
    float *c66,
    float *c13,
    float *rho)
{
    size_t siz_line   = nx; 
    size_t siz_slice  = nx * ny; 
    size_t siz_volume = nx * ny * nz;

    int media_type = interfaces.media_type;

    for (size_t k = 0; k < nz; k++) {
        for (size_t j = 0; j < ny; j++) {
            for (size_t i = 0; i < nx; i++) {


                size_t indx =  i + j * siz_line + k * siz_slice;

                size_t indx_z = indx;          // for vmap and curv: z
                size_t indx_x = i, indx_y = j; // for vmap and cart: x, y
                if (grid_type == GRID_CART) {
                    indx_z = k;                // for cart: z
                } else if (grid_type == GRID_CURV) {
                    indx_x = indx;
                    indx_y = indx;             // for curv: x, y
                }

                std::vector<float> var(6, 0.0); 
                AssignLayerMediaPara2Point(i, j, k,
                        Point3(Gridx[indx_x], Gridy[indx_y], Gridz[indx_z]), 
                        interfaces, media_type, var);
                
                para2vti(var, media_type, 
                    c11[indx], c33[indx], c55[indx], c66[indx], c13[indx], rho[indx]);
                
            }
        }
    }
}

//- 4. Assign the parameter directly (use the local values): elastic tti
void parametrization_layer_el_aniso_loc(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type, 
    inter_t interfaces,
    float *c11,
    float *c12,
    float *c13,
    float *c14,
    float *c15,
    float *c16,
    float *c22,
    float *c23,
    float *c24,
    float *c25,
    float *c26,
    float *c33,
    float *c34,
    float *c35,
    float *c36,
    float *c44,
    float *c45,
    float *c46,
    float *c55,
    float *c56,
    float *c66,
    float *rho)
{
    size_t siz_line   = nx; 
    size_t siz_slice  = nx * ny; 
    size_t siz_volume = nx * ny * nz;
    int media_type = interfaces.media_type;

    for (size_t k = 0; k < nz; k++) {
        for (size_t j = 0; j < ny; j++) {
            for (size_t i = 0; i < nx; i++) {

                size_t indx =  i + j * siz_line + k * siz_slice;

                size_t indx_z = indx;          // for vmap and curv: z
                size_t indx_x = i, indx_y = j; // for vmap and cart: x, y

                if (grid_type == GRID_CART) {
                    indx_z = k;                // for cart: z
                } else if (grid_type == GRID_CURV) {
                    indx_x = indx;
                    indx_y = indx;             // for curv: x, y
                }

                std::vector<float> var(22, 0.0); 

                AssignLayerMediaPara2Point(i, j, k,
                            Point3(Gridx[indx_x], Gridy[indx_y], Gridz[indx_z]), 
                            interfaces, media_type, var);

                para2tti(var, media_type, // return cij
                    c11[indx], c12[indx], c13[indx], c14[indx], c15[indx], c16[indx],
                    c22[indx], c23[indx], c24[indx], c25[indx], c26[indx],
                    c33[indx], c34[indx], c35[indx], c36[indx],
                    c44[indx], c45[indx], c46[indx], 
                    c55[indx], c56[indx], c66[indx], rho[indx]);
            }
        }
    }
}

//======================== averaging method ===================================

/* 
 * For half-grid point, marked the materials number.  
 */
void MarkInterfaceNumber(
        int grid_type,
        float *Hx, float *Hy, float *Hz,
        size_t nx, size_t ny, size_t nz,
        int *MaterNum, // nx*ny*nz
        inter_t interfaces) 
{
    float MINX = interfaces.MINX;
    float MINY = interfaces.MINY;
    float   DX = interfaces.DX;
    float   DY = interfaces.DY;
    size_t  NX = interfaces.NX;
    size_t  NY = interfaces.NY;
    size_t  NI = interfaces.NI;
    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t inter_slice = NX*NY;

    std::vector<float> XVEC(NX), YVEC(NY);
   
    // x-vector and y-vector of interfaces mesh
    for (size_t i = 0; i < NX; ++i) {
        XVEC[i] = MINX + DX*i;
    }
    for (size_t i = 0; i < NY; ++i) {
        YVEC[i] = MINY + DY*i;
    }

    
    if (grid_type != GRID_CURV) {

        // for every half grid(Hx[i], Hy[j]), store the elevation of every ni.
        // elevation[indx_in_xy][ni]
        std::vector<std::vector<float>> elevation(siz_slice, std::vector<float>(NI, -FLT_MAX));
        
        for (size_t j = 0; j < ny; ++j) {
            for (size_t i = 0; i < nx; ++i) {
                size_t indx_in_slice = i + j*nx;
                for (size_t ni = 0; ni < NI; ++ni) {
                    elevation[indx_in_slice][ni] = BilinearInterpolation(
                        XVEC, YVEC, interfaces.elevation+ni*inter_slice, Hx[i], Hy[j]);
                }

                for (size_t k = 0; k < nz; ++k) {
                    size_t indx = indx_in_slice + k*siz_slice;
                    /* use which material */
                    int mi = -1;
                    if (grid_type == GRID_VMAP) {
                        mi = findLastGreaterEqualIndex(Hz[indx], elevation[indx_in_slice]);
                    } else {
                        mi = findLastGreaterEqualIndex(Hz[k], elevation[indx_in_slice]);
                    }
                    if (mi == -1) {
                        mi = findLastGreaterEqualIndex(elevation[indx_in_slice][0], 
                                                       elevation[indx_in_slice]);
                    }

                    MaterNum[indx] = mi;
                }
            }
        }

    } else {
        std::vector<float> elevation(NI, -FLT_MAX);
        for (size_t indx = 0; indx < siz_volume; ++indx) {
            for (size_t ni = 0; ni < NI; ++ni) {
                /* Get the elevation for the corresponding location */
                elevation[ni] = BilinearInterpolation(XVEC, YVEC, 
                    interfaces.elevation + ni*inter_slice, Hx[indx], Hy[indx]);
            }
            /* use which material */
            int mi = findLastGreaterEqualIndex(Hz[indx], elevation);
            if (mi == -1) {
                mi = findLastGreaterEqualIndex(elevation[0], elevation);
            }
            MaterNum[indx] = mi;
        }
    } 
    //else {
   //     fprintf(stderr,"Error: Unknow grid_type, please check the code! (for code check, please contact Luqian Jiang)");
   //     fflush(stderr);
   //     exit(1);
   // }
}

//- 0.1 Assign the parameter by volume harmonic averaging
//- one component
void parametrization_layer_onecmp_har(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type,
    inter_t interfaces,
    float *var3d) 
{

    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;

    // assign the local value first.
    parametrization_layer_onecmp_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, var3d);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    // recalculate the material value of the point
                    float vol_var = 0.0;

                    Point3 *SubGrid = MeshSubdivide(M);
                    size_t nsg = (NG+1)*(NG+1)*(NG+1);

                    for (size_t isg = 0; isg < nsg; isg++) {

                        std::vector<float> var(1, 0.0);
                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, ONE_COMPONENT, var);

                        vol_var += 1.0/var[0];
                    }

                    var3d[indx] = nsg*1.0/vol_var;

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;
}

//- 0.1 Assign the parameter by volume arithmetic averaging
//- one component
void parametrization_layer_onecmp_ari(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type,
    inter_t interfaces,
    float *var3d) 
{
    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;

    // assign the local value first.
    parametrization_layer_onecmp_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, var3d);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    // recalculate the material value of the point
                    float vol_var = 0.0;

                    Point3 *SubGrid = MeshSubdivide(M);
                    size_t nsg = (NG+1)*(NG+1)*(NG+1);

                    // numerical integration
                    for (size_t isg = 0; isg < nsg; isg++) {
                        std::vector<float> var(1, 0.0);
                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, ONE_COMPONENT, var);

                        vol_var += var[0];
                    }

                    var3d[indx] = vol_var/(nsg*1.0);

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;
}

//- 1.0 Assign the parameter by volume harmonic averaging (kappa)
//- acoustic isortopic
void parametrization_layer_ac_iso_har(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type,
    inter_t interfaces,
    float *kappa, 
    float *rho3d) 
{

    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;

    // assign the local value first.
    parametrization_layer_ac_iso_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, kappa, rho3d);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    Point3 *SubGrid = MeshSubdivide(M);
                    size_t nsg = (NG+1)*(NG+1)*(NG+1);

                    // recalculate the material value of the point
                    float ari_rho = 0.0, har_kappa = 0.0;

                    for (size_t isg = 0; isg < nsg; isg++) {
                        std::vector<float> var(2, 0.0);

                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, ACOUSTIC_ISOTROPIC, var);
                        
                        float vp = var[1], rho = var[0];

                        har_kappa += (1.0/(vp*vp*rho));
                        ari_rho   += rho;
                    }

                    rho3d[indx] = ari_rho/(1.0*nsg);
                    kappa[indx] = 1.0*nsg/har_kappa;

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;
}

//- 1.1 Assign the parameter by volume arithmetic averaging (kappa)
//- acoustic isortopic
void parametrization_layer_ac_iso_ari(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type,
    inter_t interfaces,
    float *kappa, 
    float *rho3d) 
{

    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;

    // assign the local value first.
    parametrization_layer_ac_iso_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, kappa, rho3d);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    Point3 *SubGrid = MeshSubdivide(M);
                    size_t nsg = (NG+1)*(NG+1)*(NG+1);

                    // recalculate the material value of the point
                    float ari_rho = 0.0, ari_kappa = 0.0;

                    for (size_t isg = 0; isg < nsg; isg++) {
                        std::vector<float> var(2, 0.0);

                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, ACOUSTIC_ISOTROPIC, var);
                        
                        float vp = var[1], rho = var[0];

                        ari_kappa += (vp*vp*rho);
                        ari_rho   += rho;
                    }

                    rho3d[indx] = ari_rho/(1.0*nsg);
                    kappa[indx] = ari_kappa/(1.0*nsg);

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;
}



//- 2.0 Assign the parameter by volume arithmetic and harmonic averaging method 
//- elasic isotropic
// ref: Moczo, 2002, 3D Heterogeneous Staggered-Grid Finite-Difference Modeling of Seismic
//      Motion with Volume Harmonic and Arithmetic Averaging of Elastic Moduli and Densities
void parametrization_layer_el_iso_har(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type,
    inter_t interfaces,
    float *lam3d,
    float *mu3d,
    float *rho3d) 
{

    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;

    // assign the local value first.
    parametrization_layer_el_iso_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, lam3d, mu3d, rho3d);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    // recalculate the material value of the point
                    float ari_rho   = 0.0;
                    float har_kappa = 0.0;
                    float har_mu    = 0.0;

                    Point3 *SubGrid = MeshSubdivide(M);

                    int nsg = (NG+1)*(NG+1)*(NG+1);
                    for (int isg = 0; isg < nsg; isg++) {

                        std::vector<float> var(3);
                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, ELASTIC_ISOTROPIC, var);

                        float vp = var[1], vs = var[2], rho = var[0];
                        float mu = vs*vs*rho;
                        float lambda = vp*vp*rho - 2.0*mu;

                        ari_rho   += rho;
                        har_kappa += (1.0/(lambda + 2.0/3.0*mu));
                        har_mu    += (1.0/mu);
                    }

                    har_mu    = nsg*1.0/har_mu;
                    har_kappa = nsg*1.0/har_kappa;
                    ari_rho   = ari_rho/(nsg*1.0); 

                    lam3d[indx] = har_kappa - 2.0/3.0*har_mu;
                    mu3d[indx]  = har_mu;
                    rho3d[indx] = ari_rho;

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;
}

//- 2.1 Assign the parameter by volume arithmetic averaging method 
//- elasic isotropic
void parametrization_layer_el_iso_ari(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type,
    inter_t interfaces,
    float *lam3d,
    float *mu3d,
    float *rho3d) 
{

    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;

    // assign the local value first.
    parametrization_layer_el_iso_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, lam3d, mu3d, rho3d);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    // recalculate the material value of the point
                    float ari_rho = 0.0;
                    float ari_lam = 0.0;
                    float ari_mu  = 0.0;

                    Point3 *SubGrid = MeshSubdivide(M);

                    int nsg = (NG+1)*(NG+1)*(NG+1);
                    for (int isg = 0; isg < nsg; isg++) {

                        std::vector<float> var(3, 0.0);

                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, ELASTIC_ISOTROPIC, var);

                        float vp = var[1], vs = var[2], rho = var[0];
                        float mu = vs*vs*rho;
                        float lambda = vp*vp*rho - 2.0*mu;

                        ari_rho += rho;
                        ari_lam += lambda;
                        ari_mu  += mu;
                    }

                    lam3d[indx] = ari_lam/(nsg*1.0);
                    mu3d[indx]  = ari_mu /(nsg*1.0);
                    rho3d[indx] = ari_rho/(nsg*1.0);

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;
}

//- 3.0 Assign the parameter by volume arithmetic and harmonic averaging method 
//- elasic vti
void parametrization_layer_el_vti_har(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type, 
    inter_t interfaces,
    float *c11,
    float *c33,
    float *c55,
    float *c66,
    float *c13,
    float *rho) 
{

    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;
    
    int media_type = interfaces.media_type;

    // assign the local value first.
    parametrization_layer_el_vti_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, c11, c33, c55, c66, c13, rho);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    // recalculate the material value of the point
                    float ari_rho = 0.0;
                    float har_c11 = 0.0;
                    float har_c33 = 0.0;
                    float har_c55 = 0.0;
                    float har_c66 = 0.0;
                    float har_c13 = 0.0;

                    Point3 *SubGrid = MeshSubdivide(M);

                    int nsg = (NG+1)*(NG+1)*(NG+1);
                    for (int isg = 0; isg < nsg; isg++) {

                        std::vector<float> var(6, 0.0);
                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, media_type, var);

                        // for every sub-point, transfer para to cij
                        float c11_p = 0.0, c33_p = 0.0;
                        float c55_p = 0.0, c66_p = 0.0;
                        float c13_p = 0.0, rho_p = 0.0;
                        para2vti(var, media_type, 
                            c11_p, c33_p, c55_p, c66_p, c13_p, rho_p);

                        har_c11 += (1.0/c11_p);
                        har_c13 += (1.0/c13_p);
                        har_c33 += (1.0/c33_p);
                        har_c55 += (1.0/c55_p);
                        har_c66 += (1.0/c66_p);
                        ari_rho += rho_p;
                    }

                    c11[indx] = 1.0*nsg/har_c11;
                    c13[indx] = 1.0*nsg/har_c13;
                    c33[indx] = 1.0*nsg/har_c33;
                    c55[indx] = 1.0*nsg/har_c55;
                    c66[indx] = 1.0*nsg/har_c66;
                    rho[indx] = ari_rho/nsg;

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;    
}

//- 3.1 Assign the parameter by volume arithmetic averaging method 
//- elasic vti
void parametrization_layer_el_vti_ari(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type, 
    inter_t interfaces,
    float *c11,
    float *c33,
    float *c55,
    float *c66,
    float *c13,
    float *rho) 
{

    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;

    int media_type = interfaces.media_type;

    // assign the local value first.
    parametrization_layer_el_vti_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, c11, c33, c55, c66, c13, rho);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    // recalculate the material value of the point
                    float ari_rho = 0.0;
                    float ari_c11 = 0.0;
                    float ari_c33 = 0.0;
                    float ari_c55 = 0.0;
                    float ari_c66 = 0.0;
                    float ari_c13 = 0.0;

                    Point3 *SubGrid = MeshSubdivide(M);

                    int nsg = (NG+1)*(NG+1)*(NG+1);
                    for (int isg = 0; isg < nsg; isg++) {

                        std::vector<float> var(6, 0.0);
                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, media_type, var);

                        // for every sub-point, transfer para to cij
                        float c11_p = 0.0, c33_p = 0.0;
                        float c55_p = 0.0, c66_p = 0.0;
                        float c13_p = 0.0, rho_p = 0.0;
                        para2vti(var, media_type, 
                            c11_p, c33_p, c55_p, c66_p, c13_p, rho_p);

                        ari_c11 += c11_p;
                        ari_c13 += c13_p;
                        ari_c33 += c33_p;
                        ari_c55 += c55_p;
                        ari_c66 += c66_p;
                        ari_rho += rho_p;
                    }

                    c11[indx] = ari_c11/nsg;
                    c13[indx] = ari_c13/nsg;
                    c33[indx] = ari_c33/nsg;
                    c55[indx] = ari_c55/nsg;
                    c66[indx] = ari_c66/nsg;
                    rho[indx] = ari_rho/nsg;

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;    
}


//- 4.0 Assign the parameter by volume arithmetic and harmonic averaging method 
//- elasic tti
void parametrization_layer_el_aniso_har(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type, 
    inter_t interfaces,
    float *c11,
    float *c12,
    float *c13,
    float *c14,
    float *c15,
    float *c16,
    float *c22,
    float *c23,
    float *c24,
    float *c25,
    float *c26,
    float *c33,
    float *c34,
    float *c35,
    float *c36,
    float *c44,
    float *c45,
    float *c46,
    float *c55,
    float *c56,
    float *c66,
    float *rho) 
{

    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;

    int media_type = interfaces.media_type;

    // assign the local value first.
    parametrization_layer_el_aniso_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, c11, c12, c13, c14, c15, c16,
        c22, c23, c24, c25, c26, c33, c34, c35, c36, 
        c44, c45, c46, c55, c56, c66, rho);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    // recalculate the material value of the point
                    float ari_rho = 0.0;
                    float har_c11 = 0.0;
                    float har_c12 = 0.0;
                    float har_c13 = 0.0;
                    float har_c14 = 0.0;
                    float har_c15 = 0.0;
                    float har_c16 = 0.0;
                    float har_c22 = 0.0;
                    float har_c23 = 0.0;
                    float har_c24 = 0.0;
                    float har_c25 = 0.0;
                    float har_c26 = 0.0;
                    float har_c33 = 0.0;
                    float har_c34 = 0.0;
                    float har_c35 = 0.0;
                    float har_c36 = 0.0;
                    float har_c44 = 0.0;
                    float har_c45 = 0.0;
                    float har_c46 = 0.0;
                    float har_c55 = 0.0;
                    float har_c56 = 0.0;
                    float har_c66 = 0.0;

                    Point3 *SubGrid = MeshSubdivide(M);

                    int nsg = (NG+1)*(NG+1)*(NG+1);
                    for (int isg = 0; isg < nsg; isg++) {

                        std::vector<float> var(22, 0.0);
                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, media_type, var);

                        // for every sub-point, transfer para to cij
                        float c11_p = 0.0, c12_p = 0.0, c13_p = 0.0;
                        float c14_p = 0.0, c15_p = 0.0, c16_p = 0.0;
                        float c22_p = 0.0, c23_p = 0.0, c24_p = 0.0;
                        float c25_p = 0.0, c26_p = 0.0, c33_p = 0.0;
                        float c34_p = 0.0, c35_p = 0.0, c36_p = 0.0;
                        float c44_p = 0.0, c45_p = 0.0, c46_p = 0.0;
                        float c55_p = 0.0, c56_p = 0.0, c66_p = 0.0;
                        float rho_p = 0.0;

                        para2tti(var, media_type, // return cij
                            c11_p, c12_p, c13_p, c14_p, c15_p, c16_p,
                            c22_p, c23_p, c24_p, c25_p, c26_p,
                            c33_p, c34_p, c35_p, c36_p,
                            c44_p, c45_p, c46_p, 
                            c55_p, c56_p, c66_p, rho_p);

                        har_c11 += (1.0/c11_p);
                        har_c12 += (1.0/c12_p);
                        har_c13 += (1.0/c13_p);
                        har_c14 += (1.0/c14_p);
                        har_c15 += (1.0/c15_p);
                        har_c16 += (1.0/c16_p);
                        har_c22 += (1.0/c22_p);
                        har_c23 += (1.0/c23_p);
                        har_c24 += (1.0/c24_p);
                        har_c25 += (1.0/c25_p);
                        har_c26 += (1.0/c26_p);
                        har_c33 += (1.0/c33_p);
                        har_c34 += (1.0/c34_p);
                        har_c35 += (1.0/c35_p);
                        har_c36 += (1.0/c36_p);
                        har_c44 += (1.0/c44_p);
                        har_c45 += (1.0/c45_p);
                        har_c46 += (1.0/c46_p);
                        har_c55 += (1.0/c55_p);
                        har_c56 += (1.0/c56_p);
                        har_c66 += (1.0/c66_p);
                        
                        ari_rho += rho_p;
                    }

                    c11[indx] = (1.0*nsg)/har_c11;
                    c12[indx] = (1.0*nsg)/har_c12;
                    c13[indx] = (1.0*nsg)/har_c13;
                    c14[indx] = (1.0*nsg)/har_c14;
                    c15[indx] = (1.0*nsg)/har_c15;
                    c16[indx] = (1.0*nsg)/har_c16;
                    c22[indx] = (1.0*nsg)/har_c22;
                    c23[indx] = (1.0*nsg)/har_c23;
                    c24[indx] = (1.0*nsg)/har_c24;
                    c25[indx] = (1.0*nsg)/har_c25;
                    c26[indx] = (1.0*nsg)/har_c26;
                    c33[indx] = (1.0*nsg)/har_c33;
                    c34[indx] = (1.0*nsg)/har_c34;
                    c35[indx] = (1.0*nsg)/har_c35;
                    c36[indx] = (1.0*nsg)/har_c36;
                    c44[indx] = (1.0*nsg)/har_c44;
                    c45[indx] = (1.0*nsg)/har_c45;
                    c46[indx] = (1.0*nsg)/har_c46;
                    c55[indx] = (1.0*nsg)/har_c55;
                    c56[indx] = (1.0*nsg)/har_c56;
                    c66[indx] = (1.0*nsg)/har_c66;

                    rho[indx] = ari_rho/nsg;

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;    
}

//- 4.1 Assign the parameter by volume arithmetic averaging method 
//- elasic tti
void parametrization_layer_el_aniso_ari(
    size_t nx, 
    size_t ny, 
    size_t nz,
    const float *Gridx, 
    const float *Gridy, 
    const float *Gridz,
    int grid_type, 
    inter_t interfaces,
    float *c11,
    float *c12,
    float *c13,
    float *c14,
    float *c15,
    float *c16,
    float *c22,
    float *c23,
    float *c24,
    float *c25,
    float *c26,
    float *c33,
    float *c34,
    float *c35,
    float *c36,
    float *c44,
    float *c45,
    float *c46,
    float *c55,
    float *c56,
    float *c66,
    float *rho) 
{

    size_t siz_line = nx;
    size_t siz_slice = ny * siz_line;
    size_t siz_volume = nz * siz_slice;
    size_t NI = interfaces.NI;

    int media_type = interfaces.media_type;

    // assign the local value first.
    parametrization_layer_el_aniso_loc(nx, ny, nz, Gridx, Gridy, Gridz,
        grid_type, interfaces, c11, c12, c13, c14, c15, c16,
        c22, c23, c24, c25, c26, c33, c34, c35, c36, 
        c44, c45, c46, c55, c56, c66, rho);

    float *Hx = nullptr, *Hy = nullptr, *Hz = nullptr; 
    GenerateHalfGrid(nx, ny, nz, grid_type, Gridx, Gridy, Gridz, &Hx, &Hy, &Hz); 

    /* mark the interface number at the half grid. */
    int *MaterNum = new int[siz_volume];

    MarkInterfaceNumber(grid_type, Hx, Hy, Hz, 
        nx, ny, nz, MaterNum, interfaces);

    for (size_t k = 1; k < nz; k++) {
        for (size_t j = 1; j < ny; j++) {
            for (size_t i = 1; i < nx; i++) {
                size_t indx =  i + j * siz_line + k * siz_slice; 
                // check if the mesh have different values;
                std::vector<int> v(8);

                // conter-clockwise: conducive to debugging
                v[0] = MaterNum[indx-1-siz_line-siz_slice];
                v[1] = MaterNum[indx  -siz_line-siz_slice];
                v[2] = MaterNum[indx           -siz_slice];
                v[3] = MaterNum[indx-1         -siz_slice];
                v[4] = MaterNum[indx-1-siz_line];
                v[5] = MaterNum[indx  -siz_line];
                v[6] = MaterNum[indx           ];
                v[7] = MaterNum[indx-1         ];

                // There is more than one medium value in the half-grid mesh, 
                if ( NumOfValues(v, NI) > 1) {
                    
                    Mesh3 M = GenerateHalfMesh(grid_type, i, j, k, indx, 
                            siz_line, siz_slice, siz_volume, Hx, Hy, Hz);

                    // recalculate the material value of the point
                    float ari_rho = 0.0;
                    float ari_c11 = 0.0;
                    float ari_c12 = 0.0;
                    float ari_c13 = 0.0;
                    float ari_c14 = 0.0;
                    float ari_c15 = 0.0;
                    float ari_c16 = 0.0;
                    float ari_c22 = 0.0;
                    float ari_c23 = 0.0;
                    float ari_c24 = 0.0;
                    float ari_c25 = 0.0;
                    float ari_c26 = 0.0;
                    float ari_c33 = 0.0;
                    float ari_c34 = 0.0;
                    float ari_c35 = 0.0;
                    float ari_c36 = 0.0;
                    float ari_c44 = 0.0;
                    float ari_c45 = 0.0;
                    float ari_c46 = 0.0;
                    float ari_c55 = 0.0;
                    float ari_c56 = 0.0;
                    float ari_c66 = 0.0;

                    Point3 *SubGrid = MeshSubdivide(M);

                    int nsg = (NG+1)*(NG+1)*(NG+1);
                    for (int isg = 0; isg < nsg; isg++) {

                        std::vector<float> var(22, 0.0);
                        AssignLayerMediaPara2Point(i, j, k,
                            SubGrid[isg], interfaces, media_type, var);

                        // for every sub-point, transfer para to cij
                        float c11_p = 0.0, c12_p = 0.0, c13_p = 0.0;
                        float c14_p = 0.0, c15_p = 0.0, c16_p = 0.0;
                        float c22_p = 0.0, c23_p = 0.0, c24_p = 0.0;
                        float c25_p = 0.0, c26_p = 0.0, c33_p = 0.0;
                        float c34_p = 0.0, c35_p = 0.0, c36_p = 0.0;
                        float c44_p = 0.0, c45_p = 0.0, c46_p = 0.0;
                        float c55_p = 0.0, c56_p = 0.0, c66_p = 0.0;
                        float rho_p = 0.0;

                        para2tti(var, media_type, // return cij
                            c11_p, c12_p, c13_p, c14_p, c15_p, c16_p,
                            c22_p, c23_p, c24_p, c25_p, c26_p,
                            c33_p, c34_p, c35_p, c36_p,
                            c44_p, c45_p, c46_p, 
                            c55_p, c56_p, c66_p, rho_p);

                        ari_c11 += c11_p;
                        ari_c12 += c12_p;
                        ari_c13 += c13_p;
                        ari_c14 += c14_p;
                        ari_c15 += c15_p;
                        ari_c16 += c16_p;
                        ari_c22 += c22_p;
                        ari_c23 += c23_p;
                        ari_c24 += c24_p;
                        ari_c25 += c25_p;
                        ari_c26 += c26_p;
                        ari_c33 += c33_p;
                        ari_c34 += c34_p;
                        ari_c35 += c35_p;
                        ari_c36 += c36_p;
                        ari_c44 += c44_p;
                        ari_c45 += c45_p;
                        ari_c46 += c46_p;
                        ari_c55 += c55_p;
                        ari_c56 += c56_p;
                        ari_c66 += c66_p;
                        
                        ari_rho += rho_p;
                    }

                    c11[indx] = ari_c11/nsg;
                    c12[indx] = ari_c12/nsg;
                    c13[indx] = ari_c13/nsg;
                    c14[indx] = ari_c14/nsg;
                    c15[indx] = ari_c15/nsg;
                    c16[indx] = ari_c16/nsg;
                    c22[indx] = ari_c22/nsg;
                    c23[indx] = ari_c23/nsg;
                    c24[indx] = ari_c24/nsg;
                    c25[indx] = ari_c25/nsg;
                    c26[indx] = ari_c26/nsg;
                    c33[indx] = ari_c33/nsg;
                    c34[indx] = ari_c34/nsg;
                    c35[indx] = ari_c35/nsg;
                    c36[indx] = ari_c36/nsg;
                    c44[indx] = ari_c44/nsg;
                    c45[indx] = ari_c45/nsg;
                    c46[indx] = ari_c46/nsg;
                    c55[indx] = ari_c55/nsg;
                    c56[indx] = ari_c56/nsg;
                    c66[indx] = ari_c66/nsg;
                    
                    rho[indx] = ari_rho/nsg;

                    if(SubGrid != nullptr) delete []SubGrid;
                }

            }
        }
    }

    if (Hx != nullptr) delete [] Hx;
    if (Hy != nullptr) delete [] Hy;
    if (Hz != nullptr) delete [] Hz;
    if (MaterNum != nullptr) delete [] MaterNum;    
}
