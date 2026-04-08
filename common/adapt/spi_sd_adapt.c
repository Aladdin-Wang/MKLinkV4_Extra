/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "spi_sd_adapt.h"
#include "hpm_spi_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_clock_drv.h"
#include "board.h"
#include "hpm_dma_mgr.h"
#include "hpm_spi.h"
#include "SDdriver.h"
#include "rtthread.h"
#define SD_SPI_BASE                     BOARD_APP_SPI_BASE
#define SD_SPI_CLK_NAME                 BOARD_APP_SPI_CLK_NAME
#define SD_SPI_CS_PIN                   BOARD_SPI_CS_PIN
#define SD_SPI_DATA_LEN_BITS            (8U)

spi_sdcard_info_t g_sd_info;
static hpm_stat_t set_spi_speed(uint32_t freq);
static void cs_select(void);
static void cs_relese(void);
static bool sdcard_is_present(void);
static hpm_stat_t write_read_byte(uint8_t *in_byte, uint8_t *out_byte);
static hpm_stat_t write_cmd_data(uint8_t cmd, uint8_t *buffer, uint32_t size);
static hpm_stat_t write(uint8_t *buffer, uint32_t size);
static hpm_stat_t read(uint8_t *buffer, uint32_t size);

static sdcard_spi_interface_t g_spi_io;
static ATTR_RAMFUNC uint8_t tx_dummy[SPI_SD_BLOCK_SIZE] = {0xFF};

static struct rt_mutex g_disk_mutex;
#if defined(USE_DMA_TRANSFER) && (USE_DMA_TRANSFER == 1)
static volatile bool rxdma_complete;
static volatile bool txdma_complete;

void spi_rxdma_complete_callback(uint32_t channel)
{
    (void)channel;
    rxdma_complete = true;
}

void spi_txdma_complete_callback(uint32_t channel)
{
    (void)channel;
    txdma_complete = true;
}
#endif

hpm_stat_t spi_sd_init(void)
{
    uint32_t spi_sclk_freq = 4000000;
    spi_initialize_config_t init_config;
    board_init_spi_clock(SD_SPI_BASE);
    board_init_spi_pins_with_gpio_as_cs(SD_SPI_BASE);
    hpm_spi_get_default_init_config(&init_config);
    init_config.direction = spi_msb_first;
    init_config.mode = spi_master_mode;
    init_config.clk_phase = spi_sclk_sampling_even_clk_edges;
    init_config.clk_polarity = spi_sclk_high_idle;
    init_config.data_len = SD_SPI_DATA_LEN_BITS;
    /* step.1  initialize spi */
    if (hpm_spi_initialize(SD_SPI_BASE, &init_config) != status_success) {
        printf("hpm_spi_initialize fail\n");
        while (1) {
        }
    }
    if (hpm_spi_set_sclk_frequency(SD_SPI_BASE, spi_sclk_freq) != status_success) {
        printf("hpm_spi_set_sclk_frequency fail\n");
        while (1) {
        }
    }

#if defined(USE_DMA_TRANSFER) && (USE_DMA_TRANSFER == 1)
    if (hpm_spi_dma_install_callback(SD_SPI_BASE, spi_txdma_complete_callback, spi_rxdma_complete_callback) != status_success) {
        printf("hpm_spi_dma_install_callback fail\n");
        while (1) {
        }
    }
    rxdma_complete = false;
    txdma_complete = false;
#endif
    rt_mutex_init(&g_disk_mutex, "fslock", RT_IPC_FLAG_FIFO);
    memset(tx_dummy, 0xFF, sizeof(tx_dummy));
    g_spi_io.set_spi_speed     = set_spi_speed;
    g_spi_io.cs_select         = cs_select;
    g_spi_io.cs_relese         = cs_relese;
    g_spi_io.sdcard_is_present = sdcard_is_present;
    g_spi_io.write_read_byte   = write_read_byte;
    g_spi_io.write_cmd_data    = write_cmd_data;
    g_spi_io.write             = write;
    g_spi_io.read              = read;
    g_spi_io.delay_ms          = NULL;//board_delay_ms;
    g_spi_io.delay_us          = NULL;//board_delay_us;
    return sdcard_spi_init(&g_spi_io);
}

static hpm_stat_t set_spi_speed(uint32_t freq)
{
    /* set SPI sclk frequency for master */
    if (hpm_spi_set_sclk_frequency(SD_SPI_BASE, freq) != status_success) {
        printf("hpm_spi_set_sclk_frequency fail\n");
        while (1) {
        }
    }
    //printf("[spi_sdcard] SPI CLK frequency:%d Hz\n", freq);
    return status_success;
}

static void cs_select(void)
{ 

   // board_write_spi_cs(BOARD_SPI_CS_PIN, false);
}

static void cs_relese(void)
{
    //board_write_spi_cs(BOARD_SPI_CS_PIN, true);

}

static bool sdcard_is_present(void)
{
    /* boards is not detect pins, so it always true */

    return true;
}

static hpm_stat_t write_read_byte(uint8_t *in_byte, uint8_t *out_byte)
{
    hpm_stat_t stat;
    stat = hpm_spi_transmit_receive_blocking(SD_SPI_BASE, in_byte, out_byte, 1, 0xFFFFFFFF);
    return stat;
}

static hpm_stat_t write_cmd_data(uint8_t cmd, uint8_t *buffer, uint32_t size)
{
    hpm_stat_t stat;
    /* command phase is 48bit,*/
    uint8_t buf[6];
    buf[0] = cmd;
    memcpy(&buf[1], buffer, size);
    stat = hpm_spi_transmit_blocking(SD_SPI_BASE, buf, size + 1, 0xFFFFFFFF);
    return stat;
}

static hpm_stat_t write(uint8_t *buffer, uint32_t size)
{
    hpm_stat_t stat = status_success;
#if defined(USE_DMA_TRANSFER) && (USE_DMA_TRANSFER == 1)
    stat = hpm_spi_transmit_nonblocking(SD_SPI_BASE, buffer, size);
    if (stat != status_success) {
        printf("hpm_spi_transmit_nonblocking fail\n");
        return stat;
    }
    while (txdma_complete == false) {
    };
    txdma_complete = false;
    /* When SPI sends, DMA completion does not mean the transfer is complete. need to wait for the SPI status to complete.*/
    while (spi_is_active(SD_SPI_BASE) == true) {
    };
#else
    stat = hpm_spi_transmit_blocking(SD_SPI_BASE, buffer, size, 0xFFFFFFFF);
#endif
    return stat;
}

static hpm_stat_t read(uint8_t *buffer, uint32_t size)
{
    hpm_stat_t stat = status_success;
#if defined(USE_DMA_TRANSFER) && (USE_DMA_TRANSFER == 1)
    stat = hpm_spi_transmit_receive_nonblocking(SD_SPI_BASE, tx_dummy, buffer, size);
    if (stat != status_success) {
        printf("hpm_spi_transmit_receive_nonblocking fail\n");
        return status_fail;
    }
    while (rxdma_complete == false) {
    };
    while (txdma_complete == false) {
    };
    rxdma_complete = false;
    txdma_complete = false;
#else
    stat = hpm_spi_transmit_receive_blocking(SD_SPI_BASE, tx_dummy, buffer, size, 0xFFFFFFFF);
#endif
    return stat;
}


MSD_CARDINFO SD0_CardInfo;
uint8_t DFF=0xFF;
uint8_t SD_TYPE=0x00;
uint8_t spi_readwrite(uint8_t Txdata){
      uint8_t Rxdata;	
      hpm_spi_transmit_receive_blocking(SD_SPI_BASE, &Txdata, &Rxdata, 1, 0xFFFFFFFF);
      return Rxdata;
}

uint8_t SD_ReceiveData(uint8_t *data, uint16_t len)
{

   uint8_t r1;
   cs_select();								   
   do
   { 
      r1 = spi_readwrite(0xFF);	
      board_delay_ms(100);
  }while(r1 != 0xFE);	
  while(len--)
  {
   *data = spi_readwrite(0xFF);
   data++;
  }
  spi_readwrite(0xFF);
  spi_readwrite(0xFF); 										  		
  return 0;
}
int SD_sendcmd(uint8_t cmd,uint32_t arg,uint8_t crc){
	uint8_t r1;
  uint8_t retry;

  cs_relese();
  board_delay_ms(20);
  cs_select();
  do{
      retry=spi_readwrite(DFF);
  }while(retry!=0xFF);

  spi_readwrite(cmd | 0x40);
  spi_readwrite(arg >> 24);
  spi_readwrite(arg >> 16);
  spi_readwrite(arg >> 8);
  spi_readwrite(arg);
  spi_readwrite(crc);
  if(cmd==CMD12)spi_readwrite(DFF);

  do {
      r1=spi_readwrite(0xFF);
  }while(r1&0X80);
	
  return r1;
}
uint8_t SD_SendBlock(uint8_t*buf,uint8_t cmd)
{	
	uint16_t t;	
        uint8_t r1;	
	do{
		r1=spi_readwrite(0xFF);
	}while(r1!=0xFF);
	
	spi_readwrite(cmd);
	if(cmd!=0XFD)//
	{
		for(t=0;t<512;t++)spi_readwrite(buf[t]);//
	    spi_readwrite(0xFF);//
	    spi_readwrite(0xFF);
		t=spi_readwrite(0xFF);//
		if((t&0x1F)!=0x05)return 2;//								  					    
	}						 									  					    
    return 0;//
}
uint8_t SD_GETCSD(uint8_t *csd_data){
		uint8_t r1;	 
    r1=SD_sendcmd(CMD9,0,0x01);
    if(r1==0)
	{
    	r1=SD_ReceiveData(csd_data, 16);
    }
	cs_relese();
	if(r1)return 1;
	else return 0;
}

uint32_t SD_GetSectorCount(void)
{
    uint8_t csd[16];
    uint32_t Capacity;  
    uint8_t n;
    uint16_t csize;  					    

    if(SD_GETCSD(csd)!=0) return 0;	    

    if((csd[0]&0xC0)==0x40)	 
    {	
          csize = csd[9] + ((uint16_t)csd[8] << 8) + 1;
          Capacity = (uint32_t)csize << 10; 		
          printf("V2.00 \r\n");   
    }else
    {	
          n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
          csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
          Capacity= (uint32_t)csize << (n - 9);  
          printf("V1.00 \r\n");  
    }
    return Capacity;
}

uint8_t SD_WriteDisk(uint8_t*buf,uint32_t sector,uint8_t cnt)
{
	uint8_t r1;
	if(SD_TYPE!=V2HC)sector *= 512;//
	if(cnt==1)
	{
		r1=SD_sendcmd(CMD24,sector,0X01);//
		if(r1==0)//
		{
			r1=SD_SendBlock(buf,0xFE);// 
		}
	}else
	{
		if(SD_TYPE!=MMC)
		{
			SD_sendcmd(CMD55,0,0X01);	
			SD_sendcmd(CMD23,cnt,0X01);//
		}
 		r1=SD_sendcmd(CMD25,sector,0X01);//
		if(r1==0)
		{
			do
			{
				r1=SD_SendBlock(buf,0xFC);// 
				buf+=512;  
			}while(--cnt && r1==0);
			r1=SD_SendBlock(0,0xFD);//
		}
	}   
	cs_relese();
	return r1;//
}	

uint8_t SD_ReadDisk(uint8_t*buf,uint32_t sector,uint8_t cnt)
{
	uint8_t r1;
	if(SD_TYPE!=V2HC)sector <<= 9;//
	if(cnt==1)
	{
		r1=SD_sendcmd(CMD17,sector,0X01);//
		if(r1==0)//
		{
			r1=SD_ReceiveData(buf,512);//   
		}
	}else
	{
		r1=SD_sendcmd(CMD18,sector,0X01);//
		do
		{
			r1=SD_ReceiveData(buf,512);//
			buf+=512;  
		}while(--cnt && r1==0); 	
		SD_sendcmd(CMD12,0,0X01);	//
	}   
	cs_relese();
	return r1;//
}

uint8_t SD_init(void)
{
	uint8_t r1;	
	uint8_t buff[6] = {0};
	uint16_t retry; 
	uint8_t i;
	
	cs_select();
	for(retry=0;retry<10;retry++){
              spi_readwrite(DFF);
	}

	do{
            r1 = SD_sendcmd(CMD0 ,0, 0x95);	
	}while(r1!=0x01);
	
	SD_TYPE=0;
	r1 = SD_sendcmd(CMD8, 0x1AA, 0x87);
	if(r1==0x01){
		for(i=0;i<4;i++)buff[i]=spi_readwrite(DFF);	
		if(buff[2]==0X01&&buff[3]==0XAA)
		{
			retry=0XFFFE;
			do
			{
				SD_sendcmd(CMD55,0,0X01);	
				r1=SD_sendcmd(CMD41,0x40000000,0X01);
			}while(r1&&retry--);
			if(retry&&SD_sendcmd(CMD58,0,0X01)==0)
			{
				for(i=0;i<4;i++)buff[i]=spi_readwrite(0XFF);
				if(buff[0]&0x40){
					SD_TYPE=V2HC;
				}else {
					SD_TYPE=V2;
				}						
			}
		}else{
			SD_sendcmd(CMD55,0,0X01);			
			r1=SD_sendcmd(CMD41,0,0X01);	
			if(r1<=1)
			{		
				SD_TYPE=V1;
				retry=0XFFFE;
				do 
				{
					SD_sendcmd(CMD55,0,0X01);	
					r1=SD_sendcmd(CMD41,0,0X01);
				}while(r1&&retry--);
			}else
			{
				SD_TYPE=MMC;
				retry=0XFFFE;
				do 
				{											    
					r1=SD_sendcmd(CMD1,0,0X01);
				}while(r1&&retry--);  
			}
			if(retry==0||SD_sendcmd(CMD16,512,0X01)!=0)SD_TYPE=ERR;
		}
	}
	cs_relese();
        if(SD_TYPE == V1){
            printf("V1\r\n");
        }else if(SD_TYPE == V2){
            printf("V2\r\n");
        }else if(SD_TYPE == V2HC){
            printf("V2HC\r\n");
        }else if(SD_TYPE == MMC){
            printf("MMC\r\n");
        }

        set_spi_speed(10000000);
	if(SD_TYPE)return 0;
	else return 1;
}

int MSD0_GetCardInfo(PMSD_CARDINFO SD0_CardInfo)
{
  uint8_t r1;
  uint8_t CSD_Tab[16];
  uint8_t CID_Tab[16];

  /* Send CMD9, Read CSD */
  r1 = SD_sendcmd(CMD9, 0, 0xFF);
  if(r1 != 0x00)
  {
    return r1;
  }

  if(SD_ReceiveData(CSD_Tab, 16))
  {
	return 1;
  }

  /* Send CMD10, Read CID */
  r1 = SD_sendcmd(CMD10, 0, 0xFF);
  if(r1 != 0x00)
  {
    return r1;
  }

  if(SD_ReceiveData(CID_Tab, 16))
  {
	return 2;
  }  

  /* Byte 0 */
  SD0_CardInfo->CSD.CSDStruct = (CSD_Tab[0] & 0xC0) >> 6;
  SD0_CardInfo->CSD.SysSpecVersion = (CSD_Tab[0] & 0x3C) >> 2;
  SD0_CardInfo->CSD.Reserved1 = CSD_Tab[0] & 0x03;
  /* Byte 1 */
  SD0_CardInfo->CSD.TAAC = CSD_Tab[1] ;
  /* Byte 2 */
  SD0_CardInfo->CSD.NSAC = CSD_Tab[2];
  /* Byte 3 */
  SD0_CardInfo->CSD.MaxBusClkFrec = CSD_Tab[3];
  /* Byte 4 */
  SD0_CardInfo->CSD.CardComdClasses = CSD_Tab[4] << 4;
  /* Byte 5 */
  SD0_CardInfo->CSD.CardComdClasses |= (CSD_Tab[5] & 0xF0) >> 4;
  SD0_CardInfo->CSD.RdBlockLen = CSD_Tab[5] & 0x0F;
  /* Byte 6 */
  SD0_CardInfo->CSD.PartBlockRead = (CSD_Tab[6] & 0x80) >> 7;
  SD0_CardInfo->CSD.WrBlockMisalign = (CSD_Tab[6] & 0x40) >> 6;
  SD0_CardInfo->CSD.RdBlockMisalign = (CSD_Tab[6] & 0x20) >> 5;
  SD0_CardInfo->CSD.DSRImpl = (CSD_Tab[6] & 0x10) >> 4;
  SD0_CardInfo->CSD.Reserved2 = 0; /* Reserved */
  SD0_CardInfo->CSD.DeviceSize = (CSD_Tab[6] & 0x03) << 10;
  /* Byte 7 */
  SD0_CardInfo->CSD.DeviceSize |= (CSD_Tab[7]) << 2;
  /* Byte 8 */
  SD0_CardInfo->CSD.DeviceSize |= (CSD_Tab[8] & 0xC0) >> 6;
  SD0_CardInfo->CSD.MaxRdCurrentVDDMin = (CSD_Tab[8] & 0x38) >> 3;
  SD0_CardInfo->CSD.MaxRdCurrentVDDMax = (CSD_Tab[8] & 0x07);
  /* Byte 9 */
  SD0_CardInfo->CSD.MaxWrCurrentVDDMin = (CSD_Tab[9] & 0xE0) >> 5;
  SD0_CardInfo->CSD.MaxWrCurrentVDDMax = (CSD_Tab[9] & 0x1C) >> 2;
  SD0_CardInfo->CSD.DeviceSizeMul = (CSD_Tab[9] & 0x03) << 1;
  /* Byte 10 */
  SD0_CardInfo->CSD.DeviceSizeMul |= (CSD_Tab[10] & 0x80) >> 7;
  SD0_CardInfo->CSD.EraseGrSize = (CSD_Tab[10] & 0x7C) >> 2;
  SD0_CardInfo->CSD.EraseGrMul = (CSD_Tab[10] & 0x03) << 3;
  /* Byte 11 */
  SD0_CardInfo->CSD.EraseGrMul |= (CSD_Tab[11] & 0xE0) >> 5;
  SD0_CardInfo->CSD.WrProtectGrSize = (CSD_Tab[11] & 0x1F);
  /* Byte 12 */
  SD0_CardInfo->CSD.WrProtectGrEnable = (CSD_Tab[12] & 0x80) >> 7;
  SD0_CardInfo->CSD.ManDeflECC = (CSD_Tab[12] & 0x60) >> 5;
  SD0_CardInfo->CSD.WrSpeedFact = (CSD_Tab[12] & 0x1C) >> 2;
  SD0_CardInfo->CSD.MaxWrBlockLen = (CSD_Tab[12] & 0x03) << 2;
  /* Byte 13 */
  SD0_CardInfo->CSD.MaxWrBlockLen |= (CSD_Tab[13] & 0xc0) >> 6;
  SD0_CardInfo->CSD.WriteBlockPaPartial = (CSD_Tab[13] & 0x20) >> 5;
  SD0_CardInfo->CSD.Reserved3 = 0;
  SD0_CardInfo->CSD.ContentProtectAppli = (CSD_Tab[13] & 0x01);
  /* Byte 14 */
  SD0_CardInfo->CSD.FileFormatGrouop = (CSD_Tab[14] & 0x80) >> 7;
  SD0_CardInfo->CSD.CopyFlag = (CSD_Tab[14] & 0x40) >> 6;
  SD0_CardInfo->CSD.PermWrProtect = (CSD_Tab[14] & 0x20) >> 5;
  SD0_CardInfo->CSD.TempWrProtect = (CSD_Tab[14] & 0x10) >> 4;
  SD0_CardInfo->CSD.FileFormat = (CSD_Tab[14] & 0x0C) >> 2;
  SD0_CardInfo->CSD.ECC = (CSD_Tab[14] & 0x03);
  /* Byte 15 */
  SD0_CardInfo->CSD.CSD_CRC = (CSD_Tab[15] & 0xFE) >> 1;
  SD0_CardInfo->CSD.Reserved4 = 1;

  if(SD0_CardInfo->CardType == V2HC)
  {
	 /* Byte 7 */
	 SD0_CardInfo->CSD.DeviceSize = (uint16_t)(CSD_Tab[8]) *256;
	 /* Byte 8 */
	 SD0_CardInfo->CSD.DeviceSize += CSD_Tab[9] ;
  }

  SD0_CardInfo->Capacity = SD0_CardInfo->CSD.DeviceSize * MSD_BLOCKSIZE * 1024;
  SD0_CardInfo->BlockSize = MSD_BLOCKSIZE;

  /* Byte 0 */
  SD0_CardInfo->CID.ManufacturerID = CID_Tab[0];
  /* Byte 1 */
  SD0_CardInfo->CID.OEM_AppliID = CID_Tab[1] << 8;
  /* Byte 2 */
  SD0_CardInfo->CID.OEM_AppliID |= CID_Tab[2];
  /* Byte 3 */
  SD0_CardInfo->CID.ProdName1 = CID_Tab[3] << 24;
  /* Byte 4 */
  SD0_CardInfo->CID.ProdName1 |= CID_Tab[4] << 16;
  /* Byte 5 */
  SD0_CardInfo->CID.ProdName1 |= CID_Tab[5] << 8;
  /* Byte 6 */
  SD0_CardInfo->CID.ProdName1 |= CID_Tab[6];
  /* Byte 7 */
  SD0_CardInfo->CID.ProdName2 = CID_Tab[7];
  /* Byte 8 */
  SD0_CardInfo->CID.ProdRev = CID_Tab[8];
  /* Byte 9 */
  SD0_CardInfo->CID.ProdSN = CID_Tab[9] << 24;
  /* Byte 10 */
  SD0_CardInfo->CID.ProdSN |= CID_Tab[10] << 16;
  /* Byte 11 */
  SD0_CardInfo->CID.ProdSN |= CID_Tab[11] << 8;
  /* Byte 12 */
  SD0_CardInfo->CID.ProdSN |= CID_Tab[12];
  /* Byte 13 */
  SD0_CardInfo->CID.Reserved1 |= (CID_Tab[13] & 0xF0) >> 4;
  /* Byte 14 */
  SD0_CardInfo->CID.ManufactDate = (CID_Tab[13] & 0x0F) << 8;
  /* Byte 15 */
  SD0_CardInfo->CID.ManufactDate |= CID_Tab[14];
  /* Byte 16 */
  SD0_CardInfo->CID.CID_CRC = (CID_Tab[15] & 0xFE) >> 1;
  SD0_CardInfo->CID.Reserved2 = 1;

  return 0;  
}

hpm_stat_t __spi_read_multi_block(uint8_t *buffer, uint32_t start_sector, uint32_t num_sectors)
{
   uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);   
   hpm_stat_t stat = sdcard_spi_read_multi_block(buffer,start_sector,num_sectors);
   restore_global_irq(level);
   return stat;
}

hpm_stat_t __spi_write_multi_block(uint8_t *buffer, uint32_t sector, uint32_t num_sectors)
{
    uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    hpm_stat_t stat = sdcard_spi_write_multi_block(buffer,sector,num_sectors);
    restore_global_irq(level);
    return stat;
}

void display_sdcard_info(void)
{
    if (sdcard_spi_get_card_info(&g_sd_info) == status_success) {
        //printf("SD Card initialization succeeded\r\n");
        //printf("Card Info:\n-----------------------------------------------\r\n");
        //printf("Card Size in GBytes:    %.2fGB\r\n", g_sd_info.capacity * 1.0f / 1024UL / 1024UL / 1024UL);
        //printf("Block count: %d block\r\n", (uint32_t)g_sd_info.block_count);
        //printf("Block Size: %d Bytes\r\n", g_sd_info.block_size);
        //printf("\n-----------------------------------------------\n");
    }
    //if(MSD0_GetCardInfo(&SD0_CardInfo) == 0){
    //    printf("SD Card initialization succeeded\r\n");
    //    printf("Card Info:\n-----------------------------------------------\r\n");
    //    printf("Card Size in GBytes:    %dGB\r\n", SD0_CardInfo.Capacity);
    //    printf("Block count: %d block\r\n", SD_GetSectorCount());
    //    printf("Block Size: %d Bytes\r\n", 512);
    //    printf("\n-----------------------------------------------\n");
    //}
 
}