#ifndef __3D_TEST_H__
#define __3D_TEST_H__

#include "stdint.h"
#include "__arm_2d_math.h"
#include "arm_2d_helper.h"

#define Q16_CANVA_HEIGHT reinterpret_q16_s16(CANVA_HEIGHT)
#define Q16_HALF         reinterpret_q16_f32(0.5f)
#define Q16_ONE          reinterpret_q16_s16(1)
#define Q16_ONE_FIVE     reinterpret_q16_f32(1.2)
#define Q16_TWO          reinterpret_q16_s16(2)

#ifndef  CANVA_WIDTH
#define  CANVA_WIDTH   240
#endif
#ifndef  CANVA_HEIGHT
#define  CANVA_HEIGHT  240
#endif

#ifndef  DEPTH
#define  DEPTH   Q16_ONE_FIVE
#endif

#define  Q16_ONE reinterpret_q16_f32(1.0)

#define Q16(x) ((q16_t)((x) * 65536))
#define Q31(x) ((q31_t)((x) * 2147483648.0f))

typedef struct {
    uint16_t i0, i1, i2;
} tri_t;

typedef struct
{
   q16_t x;
   q16_t y;
}point_t;

typedef struct
{
   q16_t x;
   q16_t y;
   q16_t z;
}Thd_point_t;


extern void  to_screen(point_t pt, arm_2d_location_t *locate);
extern point_t projection(Thd_point_t Thd_point, q31_t xz_rad, q31_t yz_rad);
extern Thd_point_t xz_rotate(Thd_point_t p, q16_t rad);
extern Thd_point_t yz_rotate(Thd_point_t p, q31_t rad);
extern Thd_point_t cube_vertices[];
extern tri_t cube_tris[];

#endif