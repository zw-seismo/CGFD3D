#ifndef CONSTANTS_H
#define CONSTANTS_H

// consts
#define CONST_NDIM   3
#define CONST_NDIM_2 6  // 2 * ndim
#define CONST_NDIM_3 9  // 3 * ndim
#define CONST_NDIM_4 12 // 3 * ndim
#define CONST_NDIM_5 15 // 3 * ndim
#define CONST_2_NDIM 8  // 2^3 = 8
#define CONST_MAX_STRLEN 1024

#ifndef PI
#ifndef M_PI
#define PI 3.14159265358979323846264338327950288419716939937510
#else
#define PI M_PI
#endif
#endif

// medium type
#define CONST_MEDIUM_ACOUSTIC_ISO  1
#define CONST_MEDIUM_ELASTIC_ISO   2
#define CONST_MEDIUM_ELASTIC_VTI   3
#define CONST_MEDIUM_ELASTIC_ANISO 4
#define CONST_MEDIUM_VISCOELASTIC_ISO 5

// visco type
#define CONST_VISCO_GRAVES_QS  1
#define CONST_VISCO_GMB 2

// source related
#define CONST_SRC_SPATIAL_POINT     1
#define CONST_SRC_SPATIAL_GAUSSIAN  2
#define CONST_SRC_SPATIAL_SINC      3

#endif
