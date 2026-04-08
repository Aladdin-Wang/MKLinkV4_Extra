#include "board.h"
#include "ff.h"
#include "diskio.h"
#include "spi_sd_adapt.h"

static ATTR_PLACE_AT_NONCACHEABLE FATFS s_sd_disk;

static const TCHAR HTML[] =        
        "<!DOCTYPE html>\n"
        "<html lang=\"zh\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>打开 MicroKeen 文档</title>\n"
        "    <meta http-equiv=\"refresh\" content=\"0;url=https://microboot.readthedocs.io/zh-cn/latest/tools/microlink/microlink/\">\n"
        "</head>\n"
        "<body>\n"
        "    <p>如果没有自动跳转，请点击 <a href=\"https://microboot.readthedocs.io/zh-cn/latest/tools/microlink/microlink/\">这里</a>。</p>\n"
        "</body>\n"
        "</html>\n";


static const TCHAR VERSION[] =        
  "\
  使用手册: https://microboot.readthedocs.io/zh-cn/latest/tools/microlink/microlink\r\n\
  V4.0.0 \r\n\
  1.速度超越JLINKV12的高速下载器\r\n\
  2.高达12M波特率的USB转UART\r\n\
  3.支持hex和bin文件脱机下载，支持FLM下载算法解析，内置128M SD卡\r\n\
  4.vref电压自适应，支持1.8V到5V电压可调\r\n\
  5.内置RTTView，SystemView，VOFA+数据映射到高速USB CDC串口\r\n\
  6.支持python脚本，内置大量python API，可以读取和写入任意地址flash,ram,cpu reg数据\r\n\
  ";
        
#define MSC_VOLUME_LABEL     "MICROKEEN"   /* FAT 卷标，<=11 字符，大写 */
static const TCHAR driver_num_buf[4] = {DEV_SD + '0', ':', '/', '\0'};
static const char *show_error_string(FRESULT fresult);

FRESULT flash_mount_fs(void)
{
    FRESULT fresult;
    UINT bw;
    FILINFO fno;
    UINT br;
    if (spi_sd_init() == status_success) {
        display_sdcard_info();
    }else {
        printf("spi sdcard init fail!\n");
        return FR_INT_ERR;
    }
    // 尝试挂载文件系统
    fresult = f_mount(&s_sd_disk, driver_num_buf, 1);
    
    if (fresult == FR_OK) {
        //printf("SD card has been mounted successfully\n");
        fresult = f_chdrive(driver_num_buf);
        if (fresult != FR_OK) {
            printf("Failed to change drive, cause: %s\n", show_error_string(fresult));
            return fresult;
        }
        fresult = f_setlabel(MSC_VOLUME_LABEL);
        if (fresult != FR_OK)
        {
            printf("Failed to setlabel drive, cause: %s\n", show_error_string(fresult));
            return fresult;
        }
    }else{
        printf("mounted failed, cause: %s\n", show_error_string(fresult));
        return fresult;
    }
    FIL *pfile  = malloc(sizeof(FIL));
    do{
        char expected_content[64];
        snprintf(expected_content, sizeof(expected_content), "  Firmware Build Date:%s %s\n", __DATE__, __TIME__);
        char read_content[64] = {0};
        fresult = f_open(pfile, "readme.txt", FA_READ);
        if (fresult == FR_OK)
        {
            /* Step 2: 读取前 64 字节 */
            fresult = f_read(pfile, read_content, sizeof(read_content), &br);
            f_close(pfile);

            /* Step 3: 判断是否需要重建文件 */
            if ((br >= strlen(expected_content)) &&
                (memcmp(read_content, expected_content, strlen(expected_content)) == 0))
            {
                /* 内容一致，不需要重写 */
                //printf("Firmware Build Date:\n%s", expected_content);
            }else{
                fresult = f_open(pfile, "readme.txt", FA_WRITE | FA_CREATE_ALWAYS);
                if (fresult == FR_OK) {
                    f_write(pfile, expected_content, strlen(expected_content), &bw);
                    f_write(pfile, VERSION, strlen(VERSION), &bw);
                    f_close(pfile);
                   // printf("version.txt created successfully\n");
                } else {
                    printf("Failed to create version.txt, cause: %s\n", show_error_string(fresult));
                    break;
                } 
            }
        }else{
            fresult = f_open(pfile, "readme.txt", FA_WRITE | FA_CREATE_ALWAYS);
            if (fresult == FR_OK) {
                f_write(pfile, expected_content, strlen(expected_content), &bw);
                f_write(pfile, VERSION, strlen(VERSION), &bw);
                f_close(pfile);
               // printf("version.txt created successfully\n");
            } else {
                printf("Failed to create version.txt, cause: %s\n", show_error_string(fresult));
                break;
            } 
        }

    // 先创建 help.html 文件（如果不存在）
        fresult = f_stat("help.html", &fno);
        if (fresult != FR_OK) {
            fresult = f_open(pfile, "help.html", FA_WRITE | FA_CREATE_ALWAYS);
            if (fresult == FR_OK) {
                f_write(pfile, HTML, strlen(HTML), &bw);
                f_close(pfile);
               // printf("help.html created successfully\n");
            } else {
                printf("Failed to create help.html, cause: %s\n", show_error_string(fresult));
                break;
            } 
        }
    // 先创建 FLM 文件夹（如果不存在）
        fresult = f_stat("FLM", &fno);
        if (fresult != FR_OK) {
            fresult = f_mkdir("FLM");
            if (fresult != FR_OK) {
                printf("Failed to create FLM directory, cause: %s\n", show_error_string(fresult));
                break;
            } else {
               // printf("FLM directory created.\n");
            }
        }

        // 再创建 Python 文件夹（如果不存在）
        fresult = f_stat("Python", &fno);
        if (fresult != FR_OK) {
            fresult = f_mkdir("Python");
            if (fresult != FR_OK) {
                printf("Failed to create Python directory, cause: %s\n", show_error_string(fresult));
                break;
            } else {
                //printf("Python directory created.\n");
            }
        }
    }while(0);
    free(pfile);
    return fresult;
}

static const char *show_error_string(FRESULT fresult)
{
    const char *result_str;

    switch (fresult) {
    case FR_OK:
        result_str = "succeeded";
        break;
    case FR_DISK_ERR:
        result_str = "A hard error occurred in the low level disk I/O level";
        break;
    case FR_INT_ERR:
        result_str = "Assertion failed";
        break;
    case FR_NOT_READY:
        result_str = "The physical drive cannot work";
        break;
    case FR_NO_FILE:
        result_str = "Could not find the file";
        break;
    case FR_NO_PATH:
        result_str = "Could not find the path";
        break;
    case FR_INVALID_NAME:
        result_str = "Tha path name format is invalid";
        break;
    case FR_DENIED:
        result_str = "Access denied due to prohibited access or directory full";
        break;
    case FR_EXIST:
        result_str = "Access denied due to prohibited access";
        break;
    case FR_INVALID_OBJECT:
        result_str = "The file/directory object is invalid";
        break;
    case FR_WRITE_PROTECTED:
        result_str = "The physical drive is write protected";
        break;
    case FR_INVALID_DRIVE:
        result_str = "The logical driver number is invalid";
        break;
    case FR_NOT_ENABLED:
        result_str = "The volume has no work area";
        break;
    case FR_NO_FILESYSTEM:
        result_str = "There is no valid FAT volume";
        break;
    case FR_MKFS_ABORTED:
        result_str = "THe f_mkfs() aborted due to any problem";
        break;
    case FR_TIMEOUT:
        result_str = "Could not get a grant to access the volume within defined period";
        break;
    case FR_LOCKED:
        result_str = "The operation is rejected according to the file sharing policy";
        break;
    case FR_NOT_ENOUGH_CORE:
        result_str = "LFN working buffer could not be allocated";
        break;
    case FR_TOO_MANY_OPEN_FILES:
        result_str = "Number of open files > FF_FS_LOCK";
        break;
    case FR_INVALID_PARAMETER:
        result_str = "Given parameter is invalid";
        break;
    default:
        result_str = "Unknown error";
        break;
    }
    return result_str;
}