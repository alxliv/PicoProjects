#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include "vl53l0x_i2c_platform.h"

// Details of time-of-flight ranging sensor VL53L0X and its API are from https://www.st.com/en/imaging-and-photonics-solutions/vl53l0x.html


static VL53L0X_Dev_t tofDev;

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_scan(i2c_inst_t *i2c)
{
    printf("Scanning I2C bus...\n");
    for (uint8_t addr = 0; addr <= 127; addr++)
    {
        if (reserved_addr(addr)) continue; // your existing check
    uint8_t reg = 0x00;   // a register that all devices often support (maybe 0x00 or ID)
    int rc = i2c_write_blocking(i2c, addr, &reg, 1, true);
    if (rc >= 0) {
        uint8_t data;
        int rc2 = i2c_read_blocking(i2c, addr, &data, 1, false);
        if (rc2 >= 0) // good
        {
            printf("I2C Device found at 0x%02X\n", addr);
            if (addr==0x29)
                printf("VL5310X device found on I2C!");
        }
    }
    //    // probe with zero-length write: master will send address, then stop; if ack -> active device
    //     uint8_t tx_dummy = 0;
    //     int32_t ret = i2c_write_blocking(i2c, addr, &tx_dummy, 0, false); // len=0 OK
    //     // success returns >= 0 (num bytes written; 0), not sure if ret >= 0 or ret == PICO_ERROR_GENERIC; check any non-negative
    //     if (ret >= 0) {
    //         printf("I2C Device found at 0x%02X\n", addr);
    //     }
    }

    //     uint8_t dummy = 0;
    //     if (!reserved_addr(addr))
    //     {
    //         int ret = i2c_read_blocking(i2c, addr, &dummy, 1, false);
    //         if (ret >= 0)
    //         {
    //             printf("I2C Device found at 0x%02X\n", addr);
    //         }
    //     }
    // }
    printf("Scan complete.\n");
}

int main()
{
    int rc = 0;

    VL53L0X_Dev_t *ptof = &tofDev;
    VL53L0X_Error tof_status = VL53L0X_ERROR_NONE;

    stdio_init_all();

    VL53L0X_Version_t Version;
    rc = VL53L0X_GetVersion(&Version);
    hard_assert(rc == 0);
    printf("VL53L0X PAL Version: %d.%d Rev. %d Build %d\n", Version.major,Version.minor, Version.revision, Version.build);
    ptof->I2cDevAddr = 0x29;             // 0x29 (7-bit) or 0x52 (8-bit) is VL53L0X device address on I2C bus by default. Can be changed/
    ptof->comms_speed_khz = 400;
    ptof->comms_type = I2C;
    rc = VL53L0X_comms_initialise(ptof->comms_type, ptof->comms_speed_khz);
    hard_assert(rc == 0);

    i2c_scan(i2c0);

    uint8_t rev=0;
    rc = VL53L0X_RdByte(ptof, VL53L0X_REG_IDENTIFICATION_REVISION_ID, &rev);
    printf("VL53L0X_RdByte rc=%d rev=0x%02x\n", rc, rev);
    hard_assert(rc == 0);
    printf("device revison: %d\n", rev);

    VL53L0X_DeviceInfo_t di={0,};
    rc = VL53L0X_GetDeviceInfo(ptof, &di);
    hard_assert(rc == 0);
    printf("DeviceInfo: Name=%s,Type=%s, ProductId=%s\n", di.Name, di.Type, di.ProductId);
    printf("ProductType=%d\n", di.ProductType);

 //   rc = VL53L0X_DataInit(ptof);
 //   hard_assert(rc == 0);

    // uint8_t VhvSettings=0;
    // uint8_t PhaseCal=0;
    // rc = VL53L0X_PerformRefCalibration(ptof, &VhvSettings, &PhaseCal);
    // hard_assert(rc == 0);


    int counter = 0;
    while (true) {
        counter++;
        printf("Loop: #%d\n", counter);
        sleep_ms(1000);
    }
}
