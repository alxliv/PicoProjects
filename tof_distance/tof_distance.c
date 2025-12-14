#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include "vl53l0x_i2c_platform.h"
#include "async_task.h"

// Details of time-of-flight ranging sensor VL53L0X and its API are from https://www.st.com/en/imaging-and-photonics-solutions/vl53l0x.html
// Details of carrier/breakout board from Pololu: https://www.pololu.com/product/2490
// See also https://github.com/pololu/vl53l0x-arduino

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
#if 0
VL53L0X_Error WaitMeasurementDataReady(VL53L0X_DEV Dev) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t NewDatReady=0;
    uint32_t LoopNb;

    // Wait until it finished
    // use timeout to avoid deadlock
    if (Status == VL53L0X_ERROR_NONE) {
        LoopNb = 0;
        do {
            Status = VL53L0X_GetMeasurementDataReady(Dev, &NewDatReady);
            if ((NewDatReady == 0x01) || Status != VL53L0X_ERROR_NONE) {
                break;
            }
            LoopNb = LoopNb + 1;
            VL53L0X_PollingDelay(Dev);
        } while (LoopNb < VL53L0X_DEFAULT_MAX_LOOP);

        if (LoopNb >= VL53L0X_DEFAULT_MAX_LOOP) {
            Status = VL53L0X_ERROR_TIME_OUT;
        }
    }

    return Status;
}
#endif

#define LED_1   (7)
#define LED_2   (8)
#define LED_RED     (LED_1)
#define LED_GREEN   (LED_2)

int all_leds[] = {LED_1, LED_2};

int pico_leds_init()
{
    int n = sizeof(all_leds)/sizeof(int);
    for (int i=0; i<n; i++)
    {
        gpio_init(all_leds[i]);
        gpio_set_dir(all_leds[i], GPIO_OUT);
    }
    return PICO_OK;
}

#define led_on(pin)  do { gpio_put((pin), true); } while (0)
#define led_off(pin) do { gpio_put((pin), false); } while (0)

TaskList ActiveTasksList;

typedef struct
{
    Task task;
    int pin;
    bool state;
} led_t;

static void led_callback(Task* task) {
    // Your code here - runs every task->interval;
    // To change behavior:
    // task->callback = another_callback;
    // To change interval:
    // task->interval = 2000;
    led_t *d = (led_t *)task;
    d->state = !d->state;
    if (d->state)
        led_on(d->pin);
    else
        led_off(d->pin);
}

static void init_led_task(led_t* led, uint32_t pin, uint32_t interval, TaskCallback callbk)
{
    led->task.callback = callbk;
    led->task.interval = interval;
    led->pin = pin;
    led->state = false;
}

// Time-of-Flight range sensor task
typedef struct {
    Task task;
    VL53L0X_Dev_t *dev;
    VL53L0X_RangingMeasurementData_t valid_data;
    uint32_t start_ms;
    uint32_t last_valid_ms;
    uint32_t latency;
    bool valid;
} range_task_t;

static void range_task_callback(Task* task) {
    range_task_t* rt = (range_task_t *)task;
    uint8_t new_data_ready=0;
    VL53L0X_Error status = VL53L0X_ERROR_NONE;

    status = VL53L0X_GetMeasurementDataReady(rt->dev, &new_data_ready);
   if ((status!=VL53L0X_ERROR_NONE)||(new_data_ready==0))
    {
        rt->valid = false;
        return;
    }
    VL53L0X_GetRangingMeasurementData(rt->dev, &rt->valid_data);
    uint32_t ms = millis();
    rt->latency = ms - rt->last_valid_ms;
    rt->last_valid_ms = ms;
    rt->valid_data.TimeStamp = ms - rt->start_ms;
    rt->valid = true;
    VL53L0X_ClearInterruptMask(rt->dev, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);

}

typedef struct
{
    Task task;
    uint32_t prev_range_time_stamp;
    range_task_t *range;
} print_task_t;


static void printDistance_callback(Task* task) {
    print_task_t* ps = (print_task_t *)task;
    VL53L0X_RangingMeasurementData_t *data = &ps->range->valid_data;

    if (ps->prev_range_time_stamp != data->TimeStamp) {
        printf("[%d ms]: latency=%d ms, D=%d mm\n", data->TimeStamp, ps->range->latency, data->RangeMilliMeter);
        ps->prev_range_time_stamp = data->TimeStamp;
    }
}

typedef struct
{
    Task task;
    led_t *red_led;
    uint32_t previous_ts;
    range_task_t *range;
} manager_task_t;

static inline uint32_t range_to_interval_ms(uint32_t range_mm) {
    if (range_mm < 30) range_mm = 30;
    if (range_mm > 3000) range_mm = 3000;
    return 50 + (uint32_t)((uint64_t)(range_mm - 30) * (2000 - 50) / (3000 - 30));
}

static void manager_callback(Task* task) {
    manager_task_t* mngr = (manager_task_t *)task;
    VL53L0X_RangingMeasurementData_t *data = &mngr->range->valid_data;

    if (data->TimeStamp != mngr->previous_ts) {
        mngr->previous_ts = data->TimeStamp;
        mngr->red_led->task.interval = range_to_interval_ms(data->RangeMilliMeter);
    }
}

int main()
{
    int rc = 0;

    TaskList_Init(&ActiveTasksList);

    rc = pico_leds_init();
    hard_assert(rc == PICO_OK);

    led_t led1;
    init_led_task(&led1, LED_GREEN, 500, led_callback);
    led_t led2;
    init_led_task(&led2, LED_RED, 5000, led_callback);
    TaskList_Add(&ActiveTasksList, (Task *)&led1);
    TaskList_Add(&ActiveTasksList, (Task *)&led2);

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

    uint8_t model_id = 0;
    rc = VL53L0X_RdByte(ptof, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id);
    hard_assert(rc == 0);
    printf("device model: 0x%02x\n", model_id);
    hard_assert(model_id == 0xEE);

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

    rc = VL53L0X_DataInit(ptof);
    hard_assert(rc == 0);

    // Device initialization
    rc = VL53L0X_StaticInit(ptof);
    hard_assert(rc==0);

    uint32_t refSpadCount;
    uint8_t isApertureSpads;
    uint8_t VhvSettings;
    uint8_t PhaseCal;

    // Calibration
    rc = VL53L0X_PerformRefCalibration(ptof, &VhvSettings, &PhaseCal); // Device Initialization
    hard_assert(rc==0);

    rc = VL53L0X_PerformRefSpadManagement(ptof, &refSpadCount, &isApertureSpads);
    hard_assert(rc==0);
    rc = VL53L0X_SetDeviceMode(ptof, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING); // Setup in single ranging mode
    hard_assert(rc==0);
    rc = VL53L0X_StartMeasurement(ptof);

#if 0
    uint32_t no_of_measurements = 32;
    uint16_t* pResults = (uint16_t*)malloc(sizeof(uint16_t) * no_of_measurements);

    for (uint32_t i=0; i<no_of_measurements; i++)
    {
        VL53L0X_RangingMeasurementData_t RangingMeasurementData;
        rc = WaitMeasurementDataReady(ptof);
        if (rc!=0)
            break;
        VL53L0X_GetRangingMeasurementData(ptof, &RangingMeasurementData);
        *(pResults + i) = RangingMeasurementData.RangeMilliMeter;
        printf("[%d] measurement: %d mm\n", i,  RangingMeasurementData.RangeMilliMeter);

        // Clear the interrupt
        VL53L0X_ClearInterruptMask(ptof, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
        VL53L0X_PollingDelay(ptof);

    }
    free(pResults);
#endif

    range_task_t rangeTask;
    rangeTask.task.callback = range_task_callback;
    rangeTask.task.interval = 0;
    rangeTask.dev = ptof;
    rangeTask.start_ms = millis();
    rangeTask.latency = 0;
    rangeTask.valid_data.TimeStamp = 0;
    TaskList_Add(&ActiveTasksList, (Task *)&rangeTask);

    print_task_t printDistance;
    printDistance.task.callback = printDistance_callback;
    printDistance.task.interval = 1000;
    printDistance.range = &rangeTask;
    TaskList_Add(&ActiveTasksList, (Task *)&printDistance);

    manager_task_t managerTask;
    managerTask.range = &rangeTask;
    managerTask.red_led = &led2;
    managerTask.task.callback = manager_callback;
    managerTask.task.interval = 0;
    managerTask.previous_ts = 0;
    TaskList_Add(&ActiveTasksList, (Task *)&managerTask);

    while (true) {
        Update(&ActiveTasksList);
        sleep_ms(1);
    }
}
