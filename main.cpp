// TestCot.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <tuple>
#include <ranges>
#include <concepts>
#include <stdint.h>

#include <windows.h>
#include "CH347DLL_EN.H"
#include "spi_flash.h"


namespace ra = std::ranges;
//namespace vi = ra::views;

using namespace std::literals;

#define W25X_ReadData            0x03
#define W25X_FastReadData        0x0B
#define W25X_ManufactDeviceID    0x90
#define W25X_JedecDeviceID       0x9F

UINT16 SPI_Flash_ReadID(ULONG port)
{
    UINT16 Temp = 0;
    UCHAR   cmd = CMD_FLASH_RDID1; //W25X_ManufactDeviceID;
    UCHAR   zero = 0;
    UCHAR   data = 0xFF;

    //GPIO_WriteBit(GPIOA, GPIO_Pin_2, 0);
    CH347SPI_ChangeCS(port, 0);            // active!
    //SPI1_ReadWriteByte(W25X_ManufactDeviceID);
    CH347SPI_WriteRead(port, 0, 1, &cmd); // auto cs if off
    //SPI1_ReadWriteByte(0x00);
    CH347SPI_Write(port, 0, 1, 1, &zero); // WriteStep <= Length
    CH347SPI_Write(port, 0, 1, 1, &zero);
    CH347SPI_Write(port, 0, 1, 1, &zero);

    //Temp |= SPI1_ReadWriteByte(0xFF) << 8;
    CH347SPI_WriteRead(port, 0, 1, &data);  // data едет в порт и в нее же загружается результат
    Temp |= data << 8;
    //Temp |= SPI1_ReadWriteByte(0xFF);
    data = 0xFF;
    CH347SPI_WriteRead(port, 0, 1, &data);
    Temp |= data;
    //GPIO_WriteBit(GPIOA, GPIO_Pin_2, 1);
    CH347SPI_ChangeCS(port, 1);            // INactive!

    return Temp;
}
/* port не несет смысловой нагрузки в данной версии библиотеки, работает значение 0 */
UINT16 SPI_Flash_ReadJEDEC(ULONG port) {
    UINT16 Temp = 0;
    UCHAR cmd = CMD_FLASH_JEDEC_ID;
    UCHAR zero = 0;
    UCHAR data = 0;

    CH347SPI_ChangeCS(port, 0);            // active!
    CH347SPI_WriteRead(port, 0, 1, &cmd);
    CH347SPI_WriteRead(port, 0, 1, &data);  // 0xEF
    printf("pre-jedec is %x\r\n", data);
    data = 0;
    CH347SPI_WriteRead(port, 0, 1, &data);
    Temp |= data << 8;
    data = 0;
    CH347SPI_WriteRead(port, 0, 1, &data);
    Temp |= data;
    data = 0;
    CH347SPI_WriteRead(port, 0, 1, &data);
    printf("post-jedec is %x\r\n", data);
    CH347SPI_ChangeCS(port, 1);            // INactive!
    return Temp;
}
void SPI_Flash_Read(uint32_t port, UCHAR *pBuffer, UINT32 ReadAddr, uint16_t size)
{
    uint32_t i;
    uint8_t cmd = CMD_FLASH_READ;
    uint8_t addr = 0;
    uint8_t data;

    CH347SPI_ChangeCS(port, 0);
    CH347SPI_WriteRead(port, 0, 1, &cmd);
    addr = ((ReadAddr) >> 16);
    CH347SPI_WriteRead(port, 0, 1, &addr);
    addr = ((ReadAddr) >> 8);
    CH347SPI_WriteRead(port, 0, 1, &addr);
    addr = ((ReadAddr) & 0xFF);
    CH347SPI_WriteRead(port, 0, 1, &addr);

    for (i = 0; i < size; i++)
    {
        data = 0xFF;
        CH347SPI_WriteRead(port, 0, 1, &data);
        pBuffer[i] = data;
    }
    CH347SPI_ChangeCS(port, 0);
}
int parse_cmd(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            std::cout << "invalid parameter " << argv[i] << std::endl;
            return 0;
        }
        else {
            switch (argv[i][1]) {
            case 'r':
                std::cout << "file to read:" << argv[i]+2 << std::endl;
                break;
            default:
                std::cout << "invalid parameter " << argv[i] << std::endl;
                return 0;
            }
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    UCHAR drv;
    UCHAR dll;
    UCHAR bcd;
    UCHAR chip;
    UCHAR bser[40];
    mSpiCfgS spi_cfg;
    mDeviceInforS dev_info;
    UCHAR buff[1024 * 40] = { 0 };
    ULONG bsz = 1024;
    UINT16 Flash_Model, Jedec;

    if (argc > 1) {
        if (!parse_cmd(argc, argv)) {
            return 0;
        }
    }
    HANDLE hh = CH347OpenDevice(0);

    if (hh) {
        CH347GetVersion(0, &drv, &dll, &bcd, &chip);
        if (!drv) {
            std::cout << "Loader not found, check board and cable! Is LED on?" << std::endl;
            return 0;
        }
        printf("Found driver CH3_%d, dll ver%d, bcd %d and chip type %d\r\n", drv, dll, bcd, chip);
        CH347GetSerialNumber(0, bser);
        printf("Serial number is %s\r\n", bser);
        CH347GPIO_Get(0, &drv, &bcd);
        printf("GPIO dir: %x, value: %x\r\n", drv, bcd);
        CH347GetDeviceInfor(0, &dev_info);
        CH347SPI_GetCfg(0, &spi_cfg);
        printf("IfNum: %d\r\n", dev_info.CH347IfNum);

        spi_cfg.iMode = 0;
        spi_cfg.CS1Polarity = 0;
        spi_cfg.CS2Polarity = 0;
        spi_cfg.iChipSelect = 0x80; //valid, CS1 is working
        spi_cfg.iClock = 2;     // 3.75 MHz
        spi_cfg.iByteOrder = 1; // MSB First

        CH347SPI_SetFrequency(0, 1500000);
        BOOL r = CH347SPI_Init(0, &spi_cfg);
        if(r) printf("Init is done!\r\n");
        else printf("Init error!\r\n");
        CH347SPI_ChangeCS(0, 1);            // inactive!
        printf("CS set to HIGH\r\n");
        CH347SPI_ChangeCS(0, 0);            // active!
        printf("CS set to LOW\r\n");
        CH347SPI_SetChipSelect(0, 0x101, 1, 0, 0, 0);   // переключает обратно в inactive


        Flash_Model = SPI_Flash_ReadID(0);
        printf("Flash_Model is %04X\r\n", Flash_Model);
        Jedec = SPI_Flash_ReadJEDEC(0);
        printf("JEDEC ID is %04X\r\n", Jedec);
        memset(buff, 0x55, 1024 * 40);
        SPI_Flash_Read(0, buff, 0, 1024 * 40);
/*
    CH347SPI_SetChipSelect(ULONG iIndex,           // Specify device number
                                   USHORT iEnableSelect,   // The lower octet is CS1 and the higher octet is CS2. A byte value of 1= sets CS, 0= ignores this CS setting
                                   USHORT iChipSelect,     // The lower octet is CS1 and the higher octet is CS2. A byte value of 1= sets CS, 0= ignores this CS setting
                                   ULONG iIsAutoDeativeCS, // The lower 16 bits are CS1 and the higher 16 bits are CS2. Whether to undo slice selection automatically after the operation is complete
                                   ULONG iActiveDelay,     // The lower 16 bits are CS1 and the higher 16 bits are CS2. Set the latency of read/write operations after chip selection, the unit is us
                                   ULONG iDelayDeactive);  // The lower 16 bits are CS1 and the higher 16 bits are CS2. Delay time for read and write operations after slice selection the unit is us
*/

        // Read USB data (не подходит, читает 0 байт)
        //CH347ReadData(0, buff, &bsz);
        CH347CloseDevice(0);
    }
    else {
        std::cout << "Driver not found, please install CH347PAR!\r\n";
    }
    return 0;
}