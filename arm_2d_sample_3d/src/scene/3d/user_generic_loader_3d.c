/*
 * Copyright (c) 2009-2025 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*============================ INCLUDES ======================================*/
#define __GENERIC_LOADER_INHERIT__
#define __ThD_sim_IMPLEMENT__

#include "user_generic_loader_3d.h"
#include "ThD_test.h"
#if defined(RTE_Acceleration_Arm_2D_Helper_PFB) && defined(RTE_Acceleration_Arm_2D_Extra_Loader)

#include <assert.h>
#include <string.h>

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunknown-warning-option"
#   pragma clang diagnostic ignored "-Wreserved-identifier"
#   pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#   pragma clang diagnostic ignored "-Wsign-conversion"
#   pragma clang diagnostic ignored "-Wpadded"
#   pragma clang diagnostic ignored "-Wcast-qual"
#   pragma clang diagnostic ignored "-Wcast-align"
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#   pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#   pragma clang diagnostic ignored "-Wmissing-braces"
#   pragma clang diagnostic ignored "-Wunused-const-variable"
#   pragma clang diagnostic ignored "-Wmissing-declarations"
#   pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif

/*============================ MACROS ========================================*/

#if __GLCD_CFG_COLOUR_DEPTH__ == 8


#elif __GLCD_CFG_COLOUR_DEPTH__ == 16


#elif __GLCD_CFG_COLOUR_DEPTH__ == 32

#else
#   error Unsupported colour depth!
#endif

#undef this
#define this    (*ptThis)

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
ARM_NONNULL(1)
static
arm_2d_err_t __ThD_sim_decoder_init(arm_generic_loader_t *ptObj);

ARM_NONNULL(1, 2, 3)
static
arm_2d_err_t __ThD_sim_draw(  arm_generic_loader_t *ptObj,
                              arm_2d_region_t *ptROI,
                              uint8_t *pchBuffer,
                              uint32_t iTargetStrideInByte,
                              uint_fast8_t chBitsPerPixel);


/*============================ LOCAL VARIABLES ===============================*/

//extern Thd_point_t cube_vertices_rose[];
//extern tri_t cube_tris_rose[];
extern Thd_point_t j20_model_vertices[];
extern tri_t j20_model_tris[];
extern Thd_point_t earth_model_vertices[];
/*============================ IMPLEMENTATION ================================*/


ARM_NONNULL(1,2)

void draw_line_fast(uint16_t *fb, uint16_t w,
                    arm_2d_location_t start,
                    arm_2d_location_t end,
                    uint16_t color)
{
    int16_t dx = abs(end.iX - start.iX);
    int16_t sx = start.iX < end.iX ? 1 : -1;

    int16_t dy = -abs(end.iY - start.iY);
    int16_t sy = start.iY < end.iY ? 1 : -1;

    int16_t err = dx + dy;

    while (1) {
        // 画点
        fb[start.iX * w + start.iY] = color;
        if (start.iX == end.iX && start.iY == end.iY) break;

        int16_t e2 = err << 1;

        if (e2 >= dy) {
            err += dy;
            start.iX += sx;
        }

        if (e2 <= dx) {
            err += dx;
            start.iY += sy;
        }
    }
}


void draw_line_fast_rgb565(uint8_t *pchBuffer,
                          int strideByte,
                          int width,
                          int height,
                          arm_2d_location_t start,
                          arm_2d_location_t end,
                          arm_2d_location_t pROI,
                          uint16_t color)
{
    int16_t x0 = start.iX - pROI.iX;
    int16_t y0 = start.iY - pROI.iY;
    int16_t x1 = end.iX - pROI.iX;
    int16_t y1 = end.iY - pROI.iY;

    int16_t dx = abs(x1 - x0);
    int16_t sx = x0 < x1 ? 1 : -1;

    int16_t dy = -abs(y1 - y0);
    int16_t sy = y0 < y1 ? 1 : -1;

    int16_t err = dx + dy;

    while (1) {

        if (x0 >= 0 && x0 < width &&
            y0 >= 0 && y0 < height) {

            uint8_t *row = pchBuffer + y0 * strideByte;
            *((uint16_t *)(row + x0 * 2)) = color;
        }

        if (x0 == x1 && y0 == y1) break;

        int16_t e2 = err << 1;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}




   const uint16_t BLUE_DEPTH_COLORMAP[32] = {
       0x65BF, 0x619E, 0x5D7E, 0x595D, 0x553D, 0x511C, 0x4CFC, 0x48DB,
       0x44BB, 0x409A, 0x3C7A, 0x3859, 0x3439, 0x3018, 0x2BF8, 0x27D7,
       0x23B7, 0x1F96, 0x1B76, 0x1755, 0x1335, 0x0F14, 0x0AF4, 0x0AD3,
       0x08B3, 0x0892, 0x0872, 0x0851, 0x0831, 0x0810, 0x07F0, 0x07CF
   };
   uint16_t get_color_by_z_q16(int32_t z_q16) {

       int idx = (z_q16 + 65536) * 31 / 131072;
       // 边界保护
       if(idx < 0) idx = 0;
       if(idx >= 32) idx = 31;
       return BLUE_DEPTH_COLORMAP[idx];
   }

void draw_point_rgb565(uint8_t *pchBuffer,
                          int strideByte,
                          int width,
                          int height,
                          arm_2d_location_t point,
                          arm_2d_location_t pROI,
                          uint16_t color)
{
    int16_t x0 = point.iX - pROI.iX;
    int16_t y0 = point.iY - pROI.iY;

    if (x0 >= 0 && x0 < width &&
        y0 >= 0 && y0 < height) {

        uint8_t *row = pchBuffer + y0 * strideByte;
        *((uint16_t *)(row + x0 * 2)) = color;
    }
}


void ThD_sim_show(ThD_sim_t *ptThis,
                        const arm_2d_tile_t *ptTile,
                        const arm_2d_region_t *ptRegion,
                        bool bIsNewFrame)
{
    ARM_2D_UNUSED(bIsNewFrame);

    assert(NULL!= ptThis);
    if (-1 == (intptr_t)ptTile) {
        ptTile = arm_2d_get_default_frame_buffer();
    }	
	
	arm_2d_tile_copy_only(  &this.tTile, 
                                     ptTile, 
                                   ptRegion);
	
}


arm_2d_err_t ThD_sim_init(ThD_sim_t *ptThis,
                                ThD_sim_cfg_t *ptCFG)
{
    assert(NULL != ptThis);
    assert(NULL != ptCFG);
    memset(ptThis, 0, sizeof(ThD_sim_t));

      if (NULL != ptCFG) {
          this.tCFG = *ptCFG;
      }

    arm_2d_err_t tResult = ARM_2D_ERR_NONE;

    do {
    #if 0 /* Please make the following code avaiable when the IO is used. */
        if (NULL == this.tCFG.ImageIO.ptIO) {
            this.use_as__arm_generic_loader_t.bErrorDetected = true;
            tResult = ARM_2D_ERR_IO_ERROR;
            break;
        }
    #endif

        arm_generic_loader_cfg_t tCFG = {
            .bUseHeapForVRES = this.tCFG.bUseHeapForVRES,
            .tColourInfo.chScheme = ARM_2D_COLOUR,
            .bBlendWithBG = true,
            .ImageIO = {
                .ptIO = this.tCFG.ImageIO.ptIO,
                .pTarget = this.tCFG.ImageIO.pTarget,
            },

            .UserDecoder = {
                .fnDecoderInit = &__ThD_sim_decoder_init,
                .fnDecode = &__ThD_sim_draw,
            },

            .ptScene = this.tCFG.ptScene,
        };

        tResult = arm_generic_loader_init(  &this.use_as__arm_generic_loader_t,
                                            &tCFG);

        if (tResult < 0) {
            break;
        }

        this.tTile.tRegion.tSize = this.tCFG.tSize;
        if ((0 == this.tTile.tRegion.tSize.iWidth)
         || (0 == this.tTile.tRegion.tSize.iHeight)) {
            tResult = ARM_2D_ERR_INVALID_PARAM;
            break;
        }

    } while(0);

    return tResult;

}

ARM_NONNULL(1)
void ThD_sim_depose( ThD_sim_t *ptThis)
{
    assert(NULL != ptThis);
    arm_generic_loader_depose(&this.use_as__arm_generic_loader_t);
}

ARM_NONNULL(1)
void ThD_sim_on_load( ThD_sim_t *ptThis)
{
    assert(NULL != ptThis);

     //for(uint16_t i = 0;i < 2504;i ++){
     //    earth_model_vertices[i] = yz_rotate(earth_model_vertices[i],Q31(-0.25));
     //}   
     for(uint16_t i = 0;i < 2504;i ++){
         j20_model_vertices[i] = yz_rotate(j20_model_vertices[i],Q31(-0.25));
     }   
    arm_generic_loader_on_load(&this.use_as__arm_generic_loader_t);
}

ARM_NONNULL(1)
void ThD_sim_on_frame_start( ThD_sim_t *ptThis)
{
    assert(NULL != ptThis);
    this.xz_rad +=  Q31(0.01);
    if(this.xz_rad > Q31(0.999)){
       this.xz_rad = 0;
    }
    //arm_2d_helper_time_liner_slider(0,1000,1000,theta);
    //theta = Q31(0.005);
    //for(uint16_t i = 0;i < 2504;i ++){
    //    earth_model_vertices[i] = xz_rotate(earth_model_vertices[i],theta);
    //}
    //theta = Q31(0.01);
    //for(uint16_t i = 0;i < 2056;i ++){
    //    earth_model_vertices[i] = xz_rotate(earth_model_vertices[i],theta);
    //}
    //arm_generic_loader_on_frame_start(&this.use_as__arm_generic_loader_t);
}

ARM_NONNULL(1)
void ThD_sim_on_frame_complete( ThD_sim_t *ptThis)
{
    assert(NULL != ptThis);

    arm_generic_loader_on_frame_complete(&this.use_as__arm_generic_loader_t);
}

ARM_NONNULL(1)
static
arm_2d_err_t __ThD_sim_decoder_init(arm_generic_loader_t *ptObj)
{
    assert(NULL != ptObj);

    ThD_sim_t *ptThis = (ThD_sim_t *)ptObj;
    ARM_2D_UNUSED(ptThis);

    return ARM_2D_ERR_NONE;
}

ARM_NONNULL(1, 2, 3)
static
arm_2d_err_t __ThD_sim_draw(  arm_generic_loader_t *ptObj,
                                    arm_2d_region_t *ptROI,
                                    uint8_t *pchBuffer,
                                    uint32_t iTargetStrideInByte,
                                    uint_fast8_t chBitsPerPixel)
{
    assert(NULL != ptObj);
    ThD_sim_t *ptThis = (ThD_sim_t *)ptObj;
    ARM_2D_UNUSED(ptThis);
    ARM_2D_UNUSED(chBitsPerPixel);	

    int16_t screen_w = ptThis->tCFG.tSize.iWidth;
    int16_t screen_h = ptThis->tCFG.tSize.iHeight;

    for(uint16_t i = 0;i < 2056;i ++){
         arm_2d_user_draw_line_api_params_t tParam_1;     
         arm_2d_user_draw_line_api_params_t tParam_2;  
         arm_2d_user_draw_line_api_params_t tParam_3;  

         to_screen(projection(j20_model_vertices[j20_model_tris[i].i0],this.xz_rad,0),&tParam_1.tStart);
         to_screen(projection(j20_model_vertices[j20_model_tris[i].i1],this.xz_rad,0),&tParam_1.tEnd);
         tParam_2.tStart.iX = tParam_1.tEnd.iX;
         tParam_2.tStart.iY = tParam_1.tEnd.iY;
         to_screen(projection(j20_model_vertices[j20_model_tris[i].i2],this.xz_rad,0),&tParam_2.tEnd);
         tParam_3.tStart.iX = tParam_2.tEnd.iX;
         tParam_3.tStart.iY = tParam_2.tEnd.iY;
         tParam_3.tEnd.iX = tParam_1.tStart.iX;
         tParam_3.tEnd.iY = tParam_1.tStart.iY;

        int32_t cross =
            (tParam_2.tStart.iX - tParam_1.tStart.iX) * (tParam_3.tStart.iY - tParam_1.tStart.iY) -
            (tParam_2.tStart.iY - tParam_1.tStart.iY) * (tParam_3.tStart.iX - tParam_1.tStart.iX);

        if (cross < 0) {  
            continue;   // 背面 → 跳过
        }


          draw_line_fast_rgb565(
              pchBuffer,
              iTargetStrideInByte,
              ptROI->tSize.iWidth,
              ptROI->tSize.iHeight,
              tParam_1.tStart,
              tParam_1.tEnd,
              ptROI->tLocation,
              GLCD_COLOR_GREEN
          );

          draw_line_fast_rgb565(
              pchBuffer,
              iTargetStrideInByte,
              ptROI->tSize.iWidth,
              ptROI->tSize.iHeight,
              tParam_2.tStart,
              tParam_2.tEnd,
              ptROI->tLocation,
              GLCD_COLOR_GREEN
          );

          draw_line_fast_rgb565(
              pchBuffer,
              iTargetStrideInByte,
              ptROI->tSize.iWidth,
              ptROI->tSize.iHeight,
              tParam_3.tStart,
              tParam_3.tEnd,
              ptROI->tLocation,
              GLCD_COLOR_GREEN
          );
    }

    //for(uint32_t i = 0;i < 2504;i ++){
    //    arm_2d_location_t locate1;
    //    to_screen(projection(earth_model_vertices[i],0,0),&locate1);
    //    uint16_t color = get_color_by_z_q16(earth_model_vertices[i].z);
    //    draw_point_rgb565(    
    //                      pchBuffer,
    //                      iTargetStrideInByte,
    //                      ptROI->tSize.iWidth,
    //                      ptROI->tSize.iHeight,
    //                      locate1,
    //                      ptROI->tLocation,
    //                      color);
    //}



    return ARM_2D_ERR_NONE;
}




#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#endif
