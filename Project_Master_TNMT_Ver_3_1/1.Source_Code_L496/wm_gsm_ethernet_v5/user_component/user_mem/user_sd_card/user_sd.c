


#include "user_sd.h"
#include "sd_functions.h"
#include "stdio.h"
#include "sd_benchmark.h"
#include "sd_spi.h"
#include "user_define.h"

static uint8_t _Cb_Sd_Write (uint8_t event);
static uint8_t _Cb_Sd_Read (uint8_t event);
static uint8_t _Cb_Sd_Test (uint8_t event);
static uint8_t _Cb_Sd_Check (uint8_t event);

/*======== struct ===============*/
sEvent_struct sEventSD[] =
{
    { _EVENT_SD_TEST, 	    0, 0, 2000,  			_Cb_Sd_Test   },
	{ _EVENT_SD_WRITE, 	    1, 0, 500,  			_Cb_Sd_Write  },
	{ _EVENT_SD_READ, 	    0, 0, 200,  			_Cb_Sd_Read   },
    
    { _EVENT_SD_CHECK,      1, 5, 5000,             _Cb_Sd_Check  },
};

static sMemSDCardWrite sQSDCardWrite[MAX_QUEUE_SD];

Struct_Queue_Type qSDCardWrite;

extern UART_HandleTypeDef huart1;
int32_t SD_Free_i32 = 0;

/*------------ Func Callback --------------*/
static uint8_t _Cb_Sd_Write (uint8_t event)
{
    uint8_t res = 0;
    sMemSDCardWrite   sqSDTemp;
    
    if(HAL_GPIO_ReadPin(SD_CD_GPIO_Port, SD_CD_Pin) == 0)
    {
        if (HAL_GPIO_ReadPin(SPI_NSS_GPIO_Port, SPI_NSS_Pin) == GPIO_PIN_SET)
        {
            if(qGet_Number_Items (&qSDCardWrite) != 0)
            {
                qQueue_Receive(&qSDCardWrite, (sMemSDCardWrite *) &sqSDTemp, 0);
//                res = sd_append_file(sqSDTemp.aName_File, sqSDTemp.aData_Packet);
                res = SD_WriteLog(sqSDTemp.aName_Folder, sqSDTemp.aName_File, sqSDTemp.aData_Packet);
                if(res == FR_OK)
                {
                    qQueue_Receive(&qSDCardWrite, NULL, 1);
                    Send_Uart("u_app_sd: Write Success \r\n");
                }
                else
                {
                    char dat[50]={0};
                    sprintf(dat,"u_app_sd: Write Fail: %d\r\n", res);   // saoviet
                    Send_Uart(dat);
                }
            }
        }
        else
            fevent_active(sEventSD, event);
    }
    
    SD_CS_HIGH();
    fevent_enable(sEventSD, event);
    return 1;
}


static uint8_t _Cb_Sd_Read (uint8_t event)
{
    return 1;
}


static uint8_t _Cb_Sd_Test (uint8_t event)
{
    
    return 1;
}

uint32_t count_sd = 0;
static uint8_t _Cb_Sd_Check (uint8_t event)
{
    if(HAL_GPIO_ReadPin(SD_CD_GPIO_Port, SD_CD_Pin) == 0)
    {
        if(HAL_GPIO_ReadPin(SPI_NSS_GPIO_Port, SPI_NSS_Pin) == 1)
        {
            FATFS *pfs;
            DWORD fre_clust, fre_sect, free_kb;
            FRESULT res = f_getfree(sd_path, &fre_clust, &pfs);
            if (res == FR_OK && SD_Check() == 1) 
            {
                fre_sect = fre_clust * pfs->csize;
                free_kb = fre_sect / 2;
                
                SD_Free_i32 = (uint32_t)(free_kb/1024);
            }
            else
            {
                SD_Card_Init();
            }
        }
        else
            fevent_active(sEventSD, event);
    }
    else
    {
        Send_Uart("u_app_sd: No Detect SD\r\n");
        SD_Free_i32 = -1;
    }

    SD_CS_HIGH();
    fevent_enable(sEventSD, event);
    return 1;
}


/*==========================Function Handle=========================*/

uint8_t SD_Check(void)
{
    uint8_t buffer[512];

    if (SD_ReadBlocks(buffer, 0, 1) == SD_OK)
        return 1;   // Module + the OK
    else
        return 0;   // Mat module hoac mat the
}

/*------------ Func Handle ------------*/
uint8_t Write_Mem_SDCard(const char *foldername, const char *filename, const char *text)
{
    sMemSDCardWrite sqSDTemp = {0};

    uint16_t lenFolder = strlen(foldername);
    uint16_t lenFile = strlen(filename);
    uint16_t lenText = strlen(text);


    if (lenFile >= sizeof(sqSDTemp.aName_File))
        return 0;   

    if (lenText >= sizeof(sqSDTemp.aData_Packet))
        return 0;   

    memcpy(sqSDTemp.aName_Folder, foldername, lenFolder);  
    memcpy(sqSDTemp.aName_File, filename, lenFile);  
    memcpy(sqSDTemp.aData_Packet, text, lenText);
    
    sqSDTemp.Length_Folder_u16 = lenFolder;
    sqSDTemp.Length_Name_u16 = lenFile;
    sqSDTemp.Length_Packet_u16 = lenText;

    if (qGet_Number_Items(&qSDCardWrite) >= MAX_QUEUE_SD - 1)
        qQueue_Receive(&qSDCardWrite, NULL, 1);

    qQueue_Send(&qSDCardWrite, (sMemSDCardWrite *)&sqSDTemp, _TYPE_SEND_TO_END);

    return 1;   // thành công
}

uint8_t SD_WriteLog(const char *folder,
                    const char *filename,
                    const char *text)
{
    uint8_t res;

    char path[256];

    // Tao folder neu chua co
    res = f_mkdir(folder);

    // Folder da ton tai van OK
    if((res != FR_OK) && (res != FR_EXIST))
    {
        return res;
    }

    // Tao duong dan day du
    sprintf(path, "%s/%s", folder, filename);
    
    res = sd_append_file(path, text);

    return res;
}
/*======================Handle Define AT command===================*/
#ifdef USING_AT_CONFIG
void AT_CMD_Get_SD_Card_Free(sData *str, uint16_t Pos)
{
    char aTemp[50] = {0};   //13 ki tu dau tien

    sprintf(aTemp,"SD_Card_Free: %d MB\r\n", SD_Free_i32);

    Modem_Respond_Str(PortConfig, aTemp, 0);
}
#endif
/*======================Handle Task and Init app====================*/
void SD_Card_Init (void)
{
    SD_Free_i32 = 0;
    sd_unmount();
    SD_POWER_OFF;
    HAL_Delay(50);
    SD_POWER_ON;
    SD_SPI_Init();
    HAL_Delay(50);
    
    sd_mount();   
    
    qQueue_Create (&qSDCardWrite, MAX_QUEUE_SD, sizeof (sMemSDCardWrite), (sMemSDCardWrite *) &sQSDCardWrite);  
  
#ifdef USING_AT_CONFIG
    /* regis cb serial */
    sATCmdList[_GET_SD_CARD_FREE].CallBack = AT_CMD_Get_SD_Card_Free;
#endif
}

uint8_t SD_Card_Task (void)
{
    uint8_t i = 0;
	uint8_t Result = false;

	for (i = 0; i < _EVENT_SD_END; i++)
	{
		if (sEventSD[i].e_status == 1)
		{
            Result = true;
            
			if ((sEventSD[i].e_systick == 0) ||
					((HAL_GetTick() - sEventSD[i].e_systick)  >=  sEventSD[i].e_period))
			{
                sEventSD[i].e_status = 0; 
				sEventSD[i].e_systick = HAL_GetTick();
				sEventSD[i].e_function_handler(i);
			}
		}
	}

	return Result;
}










