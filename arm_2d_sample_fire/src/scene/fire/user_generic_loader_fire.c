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
#define __fire_sim_IMPLEMENT__

#include "user_generic_loader_fire.h"
#include "fire_sim.h"
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
arm_2d_err_t __fire_sim_decoder_init(arm_generic_loader_t *ptObj);

ARM_NONNULL(1, 2, 3)
static
arm_2d_err_t __fire_sim_draw(  arm_generic_loader_t *ptObj,
                                    arm_2d_region_t *ptROI,
                                    uint8_t *pchBuffer,
                                    uint32_t iTargetStrideInByte,
                                    uint_fast8_t chBitsPerPixel);

/*============================ LOCAL VARIABLES ===============================*/
/*============================ IMPLEMENTATION ================================*/

ARM_NONNULL(1,2)

void fire_sim_show(fire_sim_t *ptThis,
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


arm_2d_err_t fire_sim_init(fire_sim_t *ptThis,
                                fire_sim_cfg_t *ptCFG)
{
    assert(NULL != ptThis);
    assert(NULL != ptCFG);
    memset(ptThis, 0, sizeof(fire_sim_t));

    //if (NULL != ptCFG) {
        this.tCFG = *ptCFG;
    //}

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
                .fnDecoderInit = &__fire_sim_decoder_init,
                .fnDecode = &__fire_sim_draw,
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
void fire_sim_depose( fire_sim_t *ptThis)
{
    assert(NULL != ptThis);

    arm_generic_loader_depose(&this.use_as__arm_generic_loader_t);
}

ARM_NONNULL(1)
void fire_sim_on_load( fire_sim_t *ptThis)
{
    assert(NULL != ptThis);
    
    arm_generic_loader_on_load(&this.use_as__arm_generic_loader_t);
}

ARM_NONNULL(1)
void fire_sim_on_frame_start( fire_sim_t *ptThis)
{
    assert(NULL != ptThis);
    
    arm_generic_loader_on_frame_start(&this.use_as__arm_generic_loader_t);
}

ARM_NONNULL(1)
void fire_sim_on_frame_complete( fire_sim_t *ptThis)
{
    assert(NULL != ptThis);

    arm_generic_loader_on_frame_complete(&this.use_as__arm_generic_loader_t);
}

ARM_NONNULL(1)
static
arm_2d_err_t __fire_sim_decoder_init(arm_generic_loader_t *ptObj)
{
    assert(NULL != ptObj);

    fire_sim_t *ptThis = (fire_sim_t *)ptObj;
    ARM_2D_UNUSED(ptThis);

    return ARM_2D_ERR_NONE;
}

ARM_NONNULL(1, 2, 3)
static
arm_2d_err_t __fire_sim_draw(  arm_generic_loader_t *ptObj,
                                    arm_2d_region_t *ptROI,
                                    uint8_t *pchBuffer,
                                    uint32_t iTargetStrideInByte,
                                    uint_fast8_t chBitsPerPixel)
{
    assert(NULL != ptObj);
    fire_sim_t *ptThis = (fire_sim_t *)ptObj;
    ARM_2D_UNUSED(ptThis);
	
	#define SCREEN_W this.tCFG.tSize.iWidth
	#define SCREEN_H this.tCFG.tSize.iHeight
	
    int_fast16_t iXLimit = ptROI->tSize.iWidth + ptROI->tLocation.iX; 
    int_fast16_t iYLimit = ptROI->tSize.iHeight + ptROI->tLocation.iY; 

    uint_fast8_t chBytesPerPixel = chBitsPerPixel >> 3;
	
	Fluid *f = scene.fluid;

	uint16_t drawW = 60;
	uint16_t drawH = 60;

	for (int_fast16_t iY = ptROI->tLocation.iY; iY < iYLimit; iY++) {

    uint8_t *pchPixelLine = pchBuffer;

    for (int_fast16_t iX = ptROI->tLocation.iX; iX < iXLimit; iX++) {

        int fx = iX * drawW / SCREEN_W;
        int fy = iY * drawH / SCREEN_H;

        if (fx >= drawW) fx = drawW - 1;
        if (fy >= drawH) fy = drawH - 1;

        uint16_t fluidIdx = (fx + 1) * f->numY + (drawH - fy);

        *((uint16_t *)pchPixelLine) =
            getFireColor_RGB565_Q12_gpt_version(f->t[fluidIdx]);

        pchPixelLine += chBytesPerPixel;
    }

    pchBuffer += iTargetStrideInByte;
}

    return ARM_2D_ERR_NONE;
}




#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#endif
