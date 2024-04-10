// Unzip ui.zip and cp to arduino libraries folder

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "ui.h"
#include "pin_config.h"
#define TOUCH_MODULES_CST_SELF
#include <TouchLib.h>

#define RX 1
#define TX 3

int work_flag = 0;

/*Change to your screen resolution*/
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 170;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_DataBus *bus = new Arduino_ESP32LCD8(7 /* DC */, 6 /* CS */, 8 /* WR */, 9 /* RD */, 39 /* D0 */, 40 /* D1 */, 41 /* D2 */, 42 /* D3 */,
                                             45 /* D4 */, 46 /* D5 */, 47 /* D6 */, 48 /* D7 */);

Arduino_GFX *gfx = new Arduino_ST7789(bus, 5 /* RST */, 0 /* rotation */, true /* IPS */, 170 /* width */, 320 /* height */, 35 /* col offset 1 */,
                                      0 /* row offset 1 */, 35 /* col offset 2 */, 0 /* row offset 2 */);

TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS820_SLAVE_ADDRESS, PIN_TOUCH_RES);

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX = 0, touchY = 0;

    if (touch.read())
    {
        data->state = LV_INDEV_STATE_PR;

        uint8_t n = touch.getPointNum();
        for (uint8_t i = 0; i < n; i++)
        {
            TP_Point t = touch.getPoint(i);
            int temp_x = t.y;
            int temp_y = map(t.x, 0, 150, 170, 0);

            // USBSerial.println(temp_x);
            // USBSerial.println(temp_y);

            data->point.x = temp_x;
            data->point.y = temp_y;
            break;
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void setup()
{
    pinMode(PIN_LCD_BL, OUTPUT);
    pinMode(PIN_LCD_RST, OUTPUT);
    pinMode(PIN_TOUCH_RES, OUTPUT);

    digitalWrite(PIN_LCD_BL, HIGH);
    digitalWrite(PIN_LCD_RST, HIGH);
    digitalWrite(PIN_TOUCH_RES, LOW);
    delay(500);
    digitalWrite(PIN_TOUCH_RES, HIGH);

    USBSerial.begin(115200); /* prepare for possible serial debug */

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    USBSerial.println(LVGL_Arduino);
    USBSerial.println("I am LVGL_Arduino");

    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    if (!touch.init())
    {
        USBSerial.println("Touch IC not found");
    }

    gfx->begin();
    gfx->setRotation(1);
    gfx->fillScreen(BLACK);

    lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();

    USBSerial.println("Setup done");

    Serial1.begin(115200, SERIAL_8N1, RX, TX);
    delay(100);
    stop_detect();
    delay(100);
    set_detect_mod();
    delay(100);

    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 20480, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(Task_my, "Task_my", 40960, NULL, 3, NULL, 1);
}

void loop()
{
}

void Task_TFT(void *pvParameters)
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(10);
    }
}

long runtime = millis();

void Task_my(void *pvParameters)
{
    long dist = 0;
    int count = 0;
    while (1)
    {
        if (count >= 5)
        {
            dist = dist / count;
            float meter = dist / 1000.0;

            char s[40];
            sprintf(s, "%4.2f m", meter);

            if (meter <= 80.0 && meter > 0.0)
                if (work_flag == 1)
                    lv_label_set_text(ui_Label1, s);

            count = 0;
            dist = 0;
        }

        if ((millis() - runtime) > 20)
        {
            start_detect();
            runtime = millis();
        }

        if (Serial1.available())
        {
            uint8_t data[8] = {0};

            Serial1.readBytes(data, 8);

            if (data[1] = 0x07)
            {
                int status = data[2];
                long int distance = 0;
                distance = data[3] * 65536 + data[4] * 256 + data[5];
                char s[40];
                sprintf(s, "Status:%d\tDistance:%ld mm\n", status, distance);

                count++;
                dist += distance;

                USBSerial.println(s);
            }
            else
            {
                USBSerial.println("Get...");
                print_hex(data, 8);
            }
        }
    }
}

// Laser -----------------------------------------

void start_detect()
{
    uint8_t cmd[8] = {0};
    uint8_t value[4] = {0};
    create_cmd(cmd, 0x05, value);
    USBSerial.println("[CMD]start_detect");
    // print_hex(cmd, 8);

    Serial1.flush();
    Serial1.write(cmd, 8);
}

void stop_detect()
{
    uint8_t cmd[8] = {0};
    uint8_t value[4] = {0};
    create_cmd(cmd, 0x06, value);
    USBSerial.println("[CMD]stop_detect");
    print_hex(cmd, 8);

    Serial1.flush();
    Serial1.write(cmd, 8);
}

void set_detect_mod()
{
    uint8_t cmd[8] = {0};
    uint8_t value[4] = {0, 0, 0, 0x01};
    create_cmd(cmd, 0x0D, value);
    USBSerial.println("[CMD]set_detect_mod");
    print_hex(cmd, 8);

    Serial1.flush();
    Serial1.write(cmd, 8);
}

void print_hex(uint8_t *data, int len)
{
    for (int i = 0; i < len; i++)
        USBSerial.printf("%02x ", data[i]);
    USBSerial.println();
}

/* 生成多项式为 CRC-8 x8+x5+x4+1 0x31(0x131) */
uint8_t crc_high_first(uint8_t key, uint8_t *value)
{
    uint8_t crc = 0x00; /* 计算的初始 CRC 值 */

    uint8_t data[5];

    data[0] = key;
    data[1] = value[0];
    data[2] = value[1];
    data[3] = value[2];
    data[4] = value[3];

    uint8_t *ptr = data;

    int len = 5;

    while (len--)
    {
        crc ^= *ptr++;              /* 每次先与需要计算的数据异或,计算完指向下一数据 */
        for (int i = 8; i > 0; --i) /* 下面这段计算过程与计算一个字节 CRC 一样 */
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

void create_cmd(uint8_t *cmd, uint8_t key, uint8_t *value)
{
    cmd[0] = 0x55;                       // Head
    cmd[1] = key;                        // Key
    cmd[2] = value[0];                   // Value
    cmd[3] = value[1];                   // Value
    cmd[4] = value[2];                   // Value
    cmd[5] = value[3];                   // Value
    cmd[6] = crc_high_first(key, value); // CRC
    cmd[7] = 0xAA;                       // End
}