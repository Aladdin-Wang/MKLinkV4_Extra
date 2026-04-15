/****************************************************************************
*  Copyright 2024 Gorgon Meducer (Email:embedded_zhuoran@hotmail.com)       *
*                                                                           *
*  Licensed under the Apache License, Version 2.0 (the "License");          *
*  you may not use this file except in compliance with the License.         *
*  You may obtain a copy of the License at                                  *
*                                                                           *
*     http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                           *
*  Unless required by applicable law or agreed to in writing, software      *
*  distributed under the License is distributed on an "AS IS" BASIS,        *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
*  See the License for the specific language governing permissions and      *
*  limitations under the License.                                           *
*                                                                           *
****************************************************************************/

#ifndef __REF_GUI_APP_CFG_H__
#define __REF_GUI_APP_CFG_H__

/*============================ INCLUDES ======================================*/

#include <stdio.h>

#ifdef   __cplusplus
extern "C" {
#endif

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmissing-declarations"
#   pragma clang diagnostic ignored "-Wmicrosoft-anon-tag"
#   pragma clang diagnostic ignored "-Wpadded"
#endif

/*============================ MACROS ========================================*/

#define NGY_NOP()                        __NOP()
#define NGY_CFG_LOG_MSG_OUT              0
#define NGY_CFG_LOG_MSG_IN               0

#define NGY_CFG_LOG_MSG_IO_POINTER_EVT   0
#define NGY_CFG_LOG_MSG_IO_KEY_EVT       0

#define NGY_CFG_LOG_MSG_POOL             0


/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

enum {
    RG_KEY_NULL         = 0x00,
    RG_KEY_DOWN         = 0x51,
    RG_KEY_UP           = 0x52,
    RG_KEY_ENTER        = 0x0D,
    RG_KEY_ESC          = 0x1B,
    RG_KEY_HOME         = RG_KEY_ESC,
}; 

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/

#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#ifdef   __cplusplus
}
#endif

#endif
