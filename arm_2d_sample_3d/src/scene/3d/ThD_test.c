#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ThD_test.h"


void to_screen(point_t pt, arm_2d_location_t *locate)
{
    locate->iX = reinterpret_s16_q16(mul_n_q16((pt.x + Q16_ONE), (CANVA_WIDTH >> 1)));
    locate->iY = reinterpret_s16_q16(Q16_CANVA_HEIGHT - mul_n_q16((pt.y + Q16_ONE), (CANVA_HEIGHT >> 1)));
}

Thd_point_t xz_rotate(Thd_point_t p, q31_t rad)
{
    Thd_point_t out;

    // Q31 → Q16
    q16_t cosv = arm_cos_q31(rad) >> 15;
    q16_t sinv = arm_sin_q31(rad) >> 15;

    out.x =  mul_q16(cosv,p.x) + mul_q16(sinv,p.z);
    out.z =  mul_q16(cosv,p.z) - mul_q16(sinv,p.x);
    out.y =  p.y;  

    return out;
}

Thd_point_t yz_rotate(Thd_point_t p, q31_t rad)
{
    Thd_point_t out;

    // Q31 → Q16
    q16_t cosv = arm_cos_q31(rad) >> 15;
    q16_t sinv = arm_sin_q31(rad) >> 15;

    out.y =  mul_q16(cosv,p.y) + mul_q16(sinv,p.z);
    out.z =  mul_q16(cosv,p.z) - mul_q16(sinv,p.y);
    out.x =  p.x;  

    return out;
}

point_t projection(Thd_point_t Thd_point, q31_t xz_rad, q31_t yz_rad)
{
    point_t point;
    Thd_point_t Tpoint = Thd_point;

    if(xz_rad){
       Tpoint = xz_rotate(Tpoint,xz_rad);
    }
    if(yz_rad){
       Tpoint = xz_rotate(Tpoint,yz_rad);
    }

    point.x = div_q16(Tpoint.x,(Tpoint.z + DEPTH));
    point.y = div_q16(Tpoint.y,(Tpoint.z + DEPTH));
    return point;
}


/****************测试素材*****************/

Thd_point_t cube_vertices[] = {
    {Q16(-0.5f), Q16(-0.5f), Q16(-0.5f)},
    {Q16( 0.5f), Q16(-0.5f), Q16(-0.5f)},
    {Q16( 0.5f), Q16( 0.5f), Q16(-0.5f)},
    {Q16(-0.5f), Q16( 0.5f), Q16(-0.5f)},

    {Q16(-0.5f), Q16(-0.5f), Q16( 0.5f)},
    {Q16( 0.5f), Q16(-0.5f), Q16( 0.5f)},
    {Q16( 0.5f), Q16( 0.5f), Q16( 0.5f)},
    {Q16(-0.5f), Q16( 0.5f), Q16( 0.5f)},
};

tri_t cube_tris[] = {
    {0,1,2},{0,2,3},
    {4,5,6},{4,6,7},
    {0,1,5},{0,5,4},
    {2,3,7},{2,7,6},
    {1,2,6},{1,6,5},
    {0,3,7},{0,7,4},
};
/****************测试素材*****************/


