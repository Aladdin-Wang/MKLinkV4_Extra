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

/*============================ INCLUDES ======================================*/

#if __PERFC_USE_USER_CUSTOM_PORTING__
#include "board.h"
#include "__perfc_task_common.h"
/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
#if defined (_MSC_VER) 
#   include <stdint.h>
#   define __STATIC_FORCEINLINE static __forceinline
#   define __STATIC_INLINE static __inline
#   define __ALIGNED(x) __declspec(align(x))
#   define __WEAK __attribute__((weak))
#elif defined ( __APPLE_CC__ )
#   include <stdint.h>
#   define  __ALIGNED(x) __attribute__((aligned(x)))
#   define __STATIC_FORCEINLINE static inline __attribute__((always_inline)) 
#   define __STATIC_INLINE static inline
#   define __WEAK __attribute__((weak))
#else
#   include <stdint.h>
#   define  __ALIGNED(x) __attribute__((aligned(x)))
#   define __STATIC_FORCEINLINE static inline __attribute__((always_inline)) 
#   define __STATIC_INLINE static inline
#   define __WEAK __attribute__((weak))
#endif

typedef uint32_t perfc_global_interrupt_status_t;

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ IMPLEMENTATION ================================*/

static
inline 
perfc_global_interrupt_status_t perfc_port_mask_systimer_interrupt(void)
{
    perfc_global_interrupt_status_t tStatus;
    
    /* get global interrupt status */
    /* disable global interrupt */
    /* return the status */
    tStatus = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    return tStatus;
    
}

static
inline  
void perfc_port_resume_systimer_interrupt(perfc_global_interrupt_status_t tStatus)
{
    restore_global_irq(tStatus);
    /* resume the stored global interrupt status */
}

static
inline 
perfc_global_interrupt_status_t perfc_port_disable_global_interrupt(void)
{
    perfc_global_interrupt_status_t tStatus;
    
    /* get global interrupt status */
    /* disable global interrupt */
    /* return the status */
    tStatus = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    return tStatus;
    
}

static
inline  
void perfc_port_resume_global_interrupt(perfc_global_interrupt_status_t tStatus)
{
    restore_global_irq(tStatus);
    /* resume the stored global interrupt status */
}

#endif