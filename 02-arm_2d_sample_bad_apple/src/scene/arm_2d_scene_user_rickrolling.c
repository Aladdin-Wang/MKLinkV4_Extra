/*
 * Copyright (c) 2009-2024 Arm Limited. All rights reserved.
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

#define __USER_SCENE_RICKROLLING_IMPLEMENT__
#include "arm_2d_scene_user_rickrolling.h"
#include "arm_2d_user_scene_large_lmsk.h"
#if defined(RTE_Acceleration_Arm_2D_Helper_PFB)                                 \
&&  defined(RTE_Acceleration_Arm_2D_Extra_JPEG_Loader)

#include <stdlib.h>
#include <string.h>

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunknown-warning-option"
#   pragma clang diagnostic ignored "-Wreserved-identifier"
#   pragma clang diagnostic ignored "-Wsign-conversion"
#   pragma clang diagnostic ignored "-Wpadded"
#   pragma clang diagnostic ignored "-Wcast-qual"
#   pragma clang diagnostic ignored "-Wcast-align"
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#   pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#   pragma clang diagnostic ignored "-Wmissing-prototypes"
#   pragma clang diagnostic ignored "-Wunused-variable"
#   pragma clang diagnostic ignored "-Wgnu-statement-expression"
#   pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#   pragma clang diagnostic ignored "-Wunused-function"
#   pragma clang diagnostic ignored "-Wmissing-declarations"
#   pragma clang diagnostic ignored "-Wimplicit-int-conversion" 
#elif __IS_COMPILER_ARM_COMPILER_5__
#   pragma diag_suppress 64,177
#elif __IS_COMPILER_IAR__
#   pragma diag_suppress=Pa089,Pe188,Pe177,Pe174
#elif __IS_COMPILER_GCC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wformat="
#   pragma GCC diagnostic ignored "-Wpedantic"
#   pragma GCC diagnostic ignored "-Wunused-function"
#   pragma GCC diagnostic ignored "-Wunused-variable"
#   pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#endif

/*============================ MACROS ========================================*/

#if __GLCD_CFG_COLOUR_DEPTH__ == 8

#   define c_tileCMSISLogo          c_tileCMSISLogoGRAY8

#elif __GLCD_CFG_COLOUR_DEPTH__ == 16

#   define c_tileCMSISLogo          c_tileCMSISLogoRGB565

#elif __GLCD_CFG_COLOUR_DEPTH__ == 32

#   define c_tileCMSISLogo          c_tileCMSISLogoCCCA8888
#else
#   error Unsupported colour depth!
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
#undef this
#define this (*ptThis)

/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/

extern const arm_2d_tile_t c_tileCMSISLogo;
extern const arm_2d_tile_t c_tileCMSISLogoMask;
extern const arm_2d_tile_t c_tileCMSISLogoA2Mask;
extern const arm_2d_tile_t c_tileCMSISLogoA4Mask;
/*============================ PROTOTYPES ====================================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ IMPLEMENTATION ================================*/

static void __on_scene_rickrolling_load(arm_2d_scene_t *ptScene)
{
    user_scene_rickrolling_t *ptThis = (user_scene_rickrolling_t *)ptScene;
    ARM_2D_UNUSED(ptThis);
#if ARM_2D_DEMO_USE_CRT_SCREEN
    crt_screen_on_load(&this.tCRTScreen);
#endif
#if ARM_2D_DEMO_USE_ZJPGD
    arm_zjpgd_loader_on_load(&this.tAnimation);
    arm_zjpgd_loader_on_load(&this.tJPGBackground);
    //arm_lmsk_loader_on_load(&this.LMSK.tLoader);
#else
    arm_tjpgd_loader_on_load(&this.tAnimation);
#endif
}

static void __after_scene_rickrolling_switching(arm_2d_scene_t *ptScene)
{
    user_scene_rickrolling_t *ptThis = (user_scene_rickrolling_t *)ptScene;
    ARM_2D_UNUSED(ptThis);

}

static void __on_scene_rickrolling_depose(arm_2d_scene_t *ptScene)
{
    user_scene_rickrolling_t *ptThis = (user_scene_rickrolling_t *)ptScene;
    ARM_2D_UNUSED(ptThis);
#if ARM_2D_DEMO_USE_CRT_SCREEN
    crt_screen_depose(&this.tCRTScreen);
#endif
#if ARM_2D_DEMO_USE_ZJPGD
    arm_zjpgd_loader_depose(&this.tAnimation);
    arm_zjpgd_loader_depose(&this.tJPGBackground);
    //arm_lmsk_loader_depose(&this.LMSK.tLoader);
#else
    arm_tjpgd_loader_depose(&this.tAnimation);
#endif

    arm_foreach(int64_t,this.lTimestamp, ptItem) {
        *ptItem = 0;
    }
    ptScene->ptPlayer = NULL;
    if (!this.bUserAllocated) {
        __arm_2d_free_scratch_memory(ARM_2D_MEM_TYPE_UNSPECIFIED, ptScene);
    }
}

/*----------------------------------------------------------------------------*
 * Scene rickrolling                                                                    *
 *----------------------------------------------------------------------------*/

static void __on_scene_rickrolling_background_start(arm_2d_scene_t *ptScene)
{
    user_scene_rickrolling_t *ptThis = (user_scene_rickrolling_t *)ptScene;
    ARM_2D_UNUSED(ptThis);

}

static void __on_scene_rickrolling_background_complete(arm_2d_scene_t *ptScene)
{
    user_scene_rickrolling_t *ptThis = (user_scene_rickrolling_t *)ptScene;
    ARM_2D_UNUSED(ptThis);

}


static void __on_scene_rickrolling_frame_start(arm_2d_scene_t *ptScene)
{
    user_scene_rickrolling_t *ptThis = (user_scene_rickrolling_t *)ptScene;
    ARM_2D_UNUSED(ptThis);
#if ARM_2D_DEMO_USE_CRT_SCREEN
    crt_screen_on_frame_start(&this.tCRTScreen);
#endif
    if (arm_2d_helper_is_time_out( this.tFilm.hwPeriodPerFrame , &this.lTimestamp[0])) {

        arm_2d_helper_film_next_frame(&this.tFilm);
    }
#if ARM_2D_DEMO_USE_ZJPGD
    arm_zjpgd_loader_on_frame_start(&this.tJPGBackground);
    arm_zjpgd_loader_on_frame_start(&this.tAnimation);
    //arm_lmsk_loader_on_frame_start(&this.LMSK.tLoader);
#else
    arm_tjpgd_loader_on_frame_start(&this.tAnimation);
#endif
}

static void __on_scene_rickrolling_frame_complete(arm_2d_scene_t *ptScene)
{
    user_scene_rickrolling_t *ptThis = (user_scene_rickrolling_t *)ptScene;
    ARM_2D_UNUSED(ptThis);
#if ARM_2D_DEMO_USE_CRT_SCREEN
    crt_screen_on_frame_complete(&this.tCRTScreen);
#endif
#if ARM_2D_DEMO_USE_ZJPGD
    arm_zjpgd_loader_on_frame_complete(&this.tAnimation);
    arm_zjpgd_loader_on_frame_complete(&this.tJPGBackground);
    //arm_lmsk_loader_on_frame_complete(&this.LMSK.tLoader);
#else
    arm_tjpgd_loader_on_frame_complete(&this.tAnimation);
#endif
    RG_PROCESS_MSG(this.ptGUI, ptMSG) {
        ARM_2D_UNUSED(bIsMessageHandled);
        
        switch(ngy_helper_msg_item_get_id(ptMSG)) {
            case NGY_MSG_GESTURE_EVT_WHEEL:
                arm_2d_scene_player_switch_to_next_scene(ptScene->ptPlayer);
                break;
            /* key pressed */
            case NGY_MSG_KEY_EVT_PRESSED:
                arm_2d_scene_player_switch_to_next_scene(ptScene->ptPlayer);
                break;    

            default:
                break;
        }
    }
}

static void __before_scene_rickrolling_switching_out(arm_2d_scene_t *ptScene)
{
    user_scene_rickrolling_t *ptThis = (user_scene_rickrolling_t *)ptScene;
    ARM_2D_UNUSED(ptThis);
    user_scene_large_lmsk_t *ptLargeLmsk
        = arm_2d_scene_large_lmsk_init(this.use_as__arm_2d_scene_t.ptPlayer);
    assert(NULL != ptLargeLmsk);
    ptLargeLmsk->ptGUI = &g_tMyGUI;
}

static
IMPL_PFB_ON_DRAW(__pfb_draw_scene_rickrolling_handler)
{
    ARM_2D_PARAM(pTarget);
    ARM_2D_PARAM(ptTile);
    ARM_2D_PARAM(bIsNewFrame);

    user_scene_rickrolling_t *ptThis = (user_scene_rickrolling_t *)pTarget;
    arm_2d_size_t tScreenSize = ptTile->tRegion.tSize;

    ARM_2D_UNUSED(tScreenSize);

    arm_2d_canvas(ptTile, __top_canvas) {
    /*-----------------------draw the foreground begin-----------------------*/

        arm_2d_align_centre(__top_canvas, this.tJPGBackground.vres.tTile.tRegion.tSize ) {
            
            arm_2d_tile_copy_only(  &this.tJPGBackground.vres.tTile,
                                    ptTile,
                                    &__centre_region);
                                    
        }
        
        arm_2d_align_centre(__top_canvas, 
                            this.tFilm.use_as__arm_2d_tile_t.tRegion.tSize ) {
            __centre_region.tLocation.iX -= 8;
            __centre_region.tLocation.iY += 38;
        #if ARM_2D_DEMO_USE_CRT_SCREEN
            crt_screen_show(&this.tCRTScreen, ptTile, &__centre_region, 100, bIsNewFrame);
        #else
            arm_2d_tile_copy_only(  (const arm_2d_tile_t *)&this.tFilm,
                                    ptTile,
                                    &__centre_region);
        #endif                        
            arm_2d_helper_dirty_region_update_item( 
                    &this.use_as__arm_2d_scene_t.tDirtyRegionHelper.tDefaultItem,
                    (arm_2d_tile_t *)ptTile,
                    &__top_canvas,
                    &__centre_region);
                                    
        }
    /*-----------------------draw the foreground end  -----------------------*/
    }
    ARM_2D_OP_WAIT_ASYNC();

    return arm_fsm_rt_cpl;
}



static
bool fatfs_arm_zjpgd_io_fopen(uintptr_t pTarget, arm_zjpgd_loader_t *ptLoader)
{
    arm_zjpgd_io_fatfs_file_loader_t *ptThis = (arm_zjpgd_io_fatfs_file_loader_t *)pTarget;
    ARM_2D_UNUSED(ptLoader);
    assert(NULL != ptThis);
    assert(NULL != this.pchFilePath);

    FRESULT res;
    this.phFile = malloc(sizeof(FIL));
    if(NULL == this.phFile){
        return false;
    }
    res = f_open(this.phFile, this.pchFilePath, FA_READ);
    if (res != FR_OK) {
        free(this.phFile);
        return false;
    }
    return true;
}

static
void fatfs_arm_zjpgd_io_fclose(uintptr_t pTarget, arm_zjpgd_loader_t *ptLoader)
{
    arm_zjpgd_io_fatfs_file_loader_t *ptThis = (arm_zjpgd_io_fatfs_file_loader_t *)pTarget;
    ARM_2D_UNUSED(ptLoader);
    assert(NULL != ptThis);

    f_close(this.phFile);
    free(this.phFile);
    this.phFile = NULL;
}

static
bool fatfs_arm_zjpgd_io_fseek(uintptr_t pTarget, arm_zjpgd_loader_t *ptLoader, int32_t offset, int32_t whence)
{
    arm_zjpgd_io_fatfs_file_loader_t *ptThis = (arm_zjpgd_io_fatfs_file_loader_t *)pTarget;
    ARM_2D_UNUSED(ptLoader);
    assert(NULL != ptThis);
    DWORD fatfs_offset;
    switch (whence) {
        case SEEK_SET:
            fatfs_offset = offset;
            break;
        case SEEK_CUR:
            fatfs_offset = f_tell(this.phFile) + offset;
            break;
        case SEEK_END:
            fatfs_offset = f_size(this.phFile) + offset;
            break;
        default:
            return false; // Invalid argument
    }
    if (f_lseek(this.phFile, fatfs_offset) == FR_OK) {
        return true;
    } else {
        return false;
    }
}

static
size_t fatfs_arm_zjpgd_io_fread(uintptr_t pTarget, arm_zjpgd_loader_t *ptLoader, uint8_t *pchBuffer, size_t tSize)
{
    arm_zjpgd_io_fatfs_file_loader_t *ptThis = (arm_zjpgd_io_fatfs_file_loader_t *)pTarget;
    ARM_2D_UNUSED(ptLoader);
    assert(NULL != ptThis);

    size_t len = 0;
    f_read(this.phFile, pchBuffer, tSize, &len);
    return len;
}

static
const arm_zjpgd_loader_io_t ARM_ZJPGD_IO_FILE_LOADER_FATFS = {
    .fnOpen =   &fatfs_arm_zjpgd_io_fopen,
    .fnClose =  &fatfs_arm_zjpgd_io_fclose,
    .fnSeek =   &fatfs_arm_zjpgd_io_fseek,
    .fnRead =   &fatfs_arm_zjpgd_io_fread,
};

static
bool fatfs_file_exists(const char *path, const char *pchMode) 
{
    ARM_2D_UNUSED(pchMode);
    FRESULT res;
    FIL *phFile = malloc(sizeof(FIL));
    if(NULL == phFile){
        return NULL;
    }
    res = f_open(phFile, path, FA_READ);
    if (res != FR_OK) {
        free(phFile);
        return false;
    }
    f_close(phFile);
    free(phFile);

    return true;
}

ARM_NONNULL(1, 2)
arm_2d_err_t arm_zjpgd_io_fatts_file_loader_init(arm_zjpgd_io_fatfs_file_loader_t *ptThis, 
                                           const char *pchFilePath)
{
    if (NULL == ptThis || NULL == pchFilePath) {
        return ARM_2D_ERR_INVALID_PARAM;
    }

    if (!fatfs_file_exists(pchFilePath, "rb")) {
        return ARM_2D_ERR_IO_ERROR;
    }

    memset(ptThis, 0, sizeof(arm_zjpgd_io_fatfs_file_loader_t));
    ptThis->pchFilePath = pchFilePath;
    return ARM_2D_ERR_NONE;
}

ARM_NONNULL(1)
user_scene_rickrolling_t *__arm_2d_scene_rickrolling_init(   arm_2d_scene_player_t *ptDispAdapter, 
                                        user_scene_rickrolling_t *ptThis)
{
    bool bUserAllocated = false;
    assert(NULL != ptDispAdapter);

    /* get the screen region */
    arm_2d_region_t __top_canvas
        = arm_2d_helper_pfb_get_display_area(
            &ptDispAdapter->use_as__arm_2d_helper_pfb_t);
    
    ARM_2D_UNUSED(__top_canvas);

    if (NULL == ptThis) {
        ptThis = (user_scene_rickrolling_t *)
                    __arm_2d_allocate_scratch_memory(   sizeof(user_scene_rickrolling_t),
                                                        __alignof__(user_scene_rickrolling_t),
                                                        ARM_2D_MEM_TYPE_UNSPECIFIED);
        assert(NULL != ptThis);
        if (NULL == ptThis) {
            return NULL;
        }
    } else {
        bUserAllocated = true;
    }

    memset(ptThis, 0, sizeof(user_scene_rickrolling_t));

    *ptThis = (user_scene_rickrolling_t){
        .use_as__arm_2d_scene_t = {

            /* the canvas colour */
            .tCanvas = {GLCD_COLOR_BLACK}, 

            /* Please uncommon the callbacks if you need them
             */
            .fnOnLoad       = &__on_scene_rickrolling_load,
            .fnScene        = &__pfb_draw_scene_rickrolling_handler,
            //.fnAfterSwitch  = &__after_scene_rickrolling_switching,

            //.fnOnBGStart    = &__on_scene_rickrolling_background_start,
            //.fnOnBGComplete = &__on_scene_rickrolling_background_complete,
            .fnOnFrameStart = &__on_scene_rickrolling_frame_start,
            .fnBeforeSwitchOut = &__before_scene_rickrolling_switching_out,
            .fnOnFrameCPL   = &__on_scene_rickrolling_frame_complete,
            .fnDepose       = &__on_scene_rickrolling_depose,

            .bUseDirtyRegionHelper = true,
        },
        .bUserAllocated = bUserAllocated,
    };

    /* ------------   initialize members of user_scene_rickrolling_t begin ---------------*/
#if ARM_2D_DEMO_USE_ZJPGD
    /* initialize Zjpgdec loader */
    do {
    #if ARM_2D_DEMO_JPGD_USE_FILE
        arm_zjpgd_io_fatts_file_loader_init(&this.tFatFile, "bad_apple_120x120_12_a4.jpg");       
    #else
        extern const uint8_t c_chRickRolling75[104704];
        arm_zjpgd_io_binary_loader_init(&this.LoaderIO.tBinary, c_chRickRolling75, sizeof(c_chRickRolling75));
    #endif

        arm_zjpgd_loader_cfg_t tCFG = {
            .bUseHeapForVRES = true,
            .ptScene = (arm_2d_scene_t *)ptThis,
            .u2WorkMode = ARM_ZJPGD_MODE_PARTIAL_DECODED,
            
            //.tColourInfo.chScheme = ARM_2D_COLOUR_GRAY8,
            //.bInvertColour = true,
        #if ARM_2D_DEMO_JPGD_USE_FILE
            .ImageIO = {
                .ptIO = &ARM_ZJPGD_IO_FILE_LOADER_FATFS,
                .pTarget = (uintptr_t)&this.tFatFile,
                
            },
        #else
            .ImageIO = {
                .ptIO = &ARM_ZJPGD_IO_BINARY_LOADER,
                .pTarget = (uintptr_t)&this.LoaderIO.tBinary,
            },
        #endif

        };

        arm_zjpgd_loader_init(&this.tAnimation, &tCFG);
    } while(0);
    do {
    #if ARM_2D_DEMO_JPGD_USE_FILE
        arm_zjpgd_io_fatts_file_loader_init(&this.tBGFatFile, "TV.jpg");       
    #else
        extern const uint8_t c_chRickRolling75[104704];
        arm_zjpgd_io_binary_loader_init(&this.LoaderIO.tBinary, c_chRickRolling75, sizeof(c_chRickRolling75));
    #endif

        arm_zjpgd_loader_cfg_t tBgCFG = {
            .bUseHeapForVRES = false,
            .ptScene = (arm_2d_scene_t *)ptThis,
            .u2WorkMode = ARM_ZJPGD_MODE_PARTIAL_DECODED,
        
        #if ARM_2D_DEMO_JPGD_USE_FILE
            .ImageIO = {
                .ptIO = &ARM_ZJPGD_IO_FILE_LOADER_FATFS,
                .pTarget = (uintptr_t)&this.tBGFatFile,
            },
        #else
            .ImageIO = {
                .ptIO = &ARM_ZJPGD_IO_BINARY_LOADER,
                .pTarget = (uintptr_t)&this.LoaderIO.tBinary,
            },
        #endif

        };

        arm_zjpgd_loader_init(&this.tJPGBackground, &tBgCFG);
    } while(0);
#else
    /* initialize TJpgDec loader */
    do {
    #if ARM_2D_DEMO_JPGD_USE_FILE
       // arm_tjpgd_io_file_loader_init(&this.LoaderIO.tFile, "Rickrolling75.jpg");
       arm_zjpgd_io_fatts_file_loader_init(&this.LoaderIO.tFile, "Rickrolling75.jpg");
    #else
        extern const uint8_t c_chRickRolling75[104704];

        arm_tjpgd_io_binary_loader_init(&this.LoaderIO.tBinary, c_chRickRolling75, sizeof(c_chRickRolling75));
    #endif

        arm_tjpgd_loader_cfg_t tCFG = {
            .bUseHeapForVRES = true,
            .ptScene = (arm_2d_scene_t *)ptThis,
            .u2WorkMode = ARM_TJPGD_MODE_PARTIAL_DECODED,
        
        #if ARM_2D_DEMO_JPGD_USE_FILE
            .ImageIO = {
                .ptIO = &ARM_ZJPGD_IO_FILE_LOADER_FATFS,
                .pTarget = (uintptr_t)&this.LoaderIO.tFatFile,
            },
        #else
            .ImageIO = {
                .ptIO = &ARM_TJPGD_IO_BINARY_LOADER,
                .pTarget = (uintptr_t)&this.LoaderIO.tBinary,
            },
        #endif

        };

        arm_tjpgd_loader_init(&this.tAnimation, &tCFG);
    } while(0);
#endif

    this.tFilm = (arm_2d_helper_film_t)impl_film(this.tAnimation, 120, 120, 12,12*259, 50);

    /* ------------   initialize members of user_scene_rickrolling_t end   ---------------*/
    /* CRT Screen */

//#define ARM_2D_DEMO_USE_CRT_SCREEN 1
#if ARM_2D_DEMO_USE_CRT_SCREEN
    do {

        crt_screen_cfg_t tCRTScreenCFG = {
            .ptScene = &this.use_as__arm_2d_scene_t,

            .ptImageBoxCFG = (image_box_cfg_t []) {{
                .ptilePhoto = (arm_2d_tile_t *)&this.tFilm,
                .tScreenColour.tColour = GLCD_COLOR_WHITE,
            }},
            
            .tScanBarColour.tColour = GLCD_COLOR_WHITE,
            .chWhiteNoiseRatio = 32,
            .chNoiseLasts = 32,
            .bStrongNoise = false,
            .bShowScanningEffect = false,
        };

        crt_screen_init(&this.tCRTScreen, &tCRTScreenCFG);
    } while(0);
#endif
    arm_2d_scene_player_append_scenes(  ptDispAdapter, 
                                        &this.use_as__arm_2d_scene_t, 
                                        1);

    return ptThis;
}


#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#endif


