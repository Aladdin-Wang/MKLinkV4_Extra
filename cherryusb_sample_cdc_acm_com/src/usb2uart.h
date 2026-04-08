#ifndef HSLINK_PRO_USB2UART_H
#define HSLINK_PRO_USB2UART_H
#include "usb_configuration.h"
#include "microboot.h"


typedef struct
{
    SIG_SLOT_OBJ;
    uint8_t Byte;
    uint16_t hwLen;
}uart_data_msg_t;

extern uart_data_msg_t  tUartMsgObj;

signals(uart_sig,uart_data_msg_t *ptThis,
      args(              
            uint8_t *pchByte,
            uint16_t hwLen
          ));

signals(uart_rx_time_sig,uart_data_msg_t *ptThis,
      args(              
            uint8_t *pchByte,
            uint16_t hwLen
          ));

signals(uart_tx_time_sig,uart_data_msg_t *ptThis,
      args(              
            uint8_t *pchByte,
            uint16_t hwLen
          ));

extern volatile uint8_t config_uart_transfer;
extern volatile struct cdc_line_coding g_cdc_lincoding;
extern volatile uint8_t config_uart;
extern USB_NOCACHE_RAM_SECTION byte_queue_t com_uartrx;
extern USB_NOCACHE_RAM_SECTION byte_queue_t com_usbrx;

extern USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t cdc_tmpbuffer[DAP_PACKET_SIZE];

void uartx_preinit(void);
void usb2uart_handler(void);

#endif //HSLINK_PRO_USB2UART_H
