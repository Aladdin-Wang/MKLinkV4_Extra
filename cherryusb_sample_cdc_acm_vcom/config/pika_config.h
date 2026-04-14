#ifndef __PIKACONFIG__H
#define __PIKACONFIG__H
#include "microboot.h"

#define PIKA_RTTHREAD_ENABLE 1

typedef struct pika_config_t {
    SIG_SLOT_OBJ;
} pika_config_t;
extern pika_config_t tPikaConfig;

signals(pika_console_sig, pika_config_t *ptThis,
        args(
            char chByte
        ));

signals(pika_progress_sig, pika_config_t *ptThis,
        args(
            char progress
        ));

signals(pika_loadstat_sig, pika_config_t *ptThis,
        args(
            bool stat
        ));
#endif

