


#ifndef USER_SD_H_
#define USER_SD_H_

#define USING_APP_SD_CARD


#include "user_util.h"
#include "event_driven.h"
#include "user_mem_comm.h"

#define MAX_QUEUE_SD        5
#define SD_QUEUE_SIZE       512

#ifdef BOARD_QN_V5_1
#define SD_POWER_OFF        HAL_GPIO_WritePin(SD_ON_OFF_GPIO_Port, SD_ON_OFF_Pin, GPIO_PIN_RESET)
#define SD_POWER_ON         HAL_GPIO_WritePin(SD_ON_OFF_GPIO_Port, SD_ON_OFF_Pin, GPIO_PIN_SET)
#endif

/*======== Struct var ===========*/

typedef enum
{
    _EVENT_SD_TEST,
	_EVENT_SD_WRITE,
	_EVENT_SD_READ,
    
    _EVENT_SD_CHECK,
      
	_EVENT_SD_END, 
}eEVENT_SD;

typedef struct
{
    char         aData_Packet[512];
    uint16_t     Length_Packet_u16;             //Length data
    char         aName_File[128];
    uint16_t     Length_Name_u16;
    char         aName_Folder[128];
    uint16_t     Length_Folder_u16;
}sMemSDCardWrite;


extern sEvent_struct sEventSD[];
extern int32_t      SD_Free_i32;



/*================ Func =================*/
void SD_Card_Init (void);
uint8_t SD_Card_Task (void);

uint8_t SD_Check(void);

uint8_t Write_Mem_SDCard(const char *foldername, const char *filename, const char *text);
uint8_t SD_WriteLog(const char *folder,
                    const char *filename,
                    const char *text);

#endif





