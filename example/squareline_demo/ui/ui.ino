#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "ui.h"
#include "DHT.h"
#include "pin_config.h"

#define TOUCH_MODULES_CST_SELF
#include <TouchLib.h>

#define DHTPIN 1      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // DHT 11

#define BUTTON_PIN 2
#define LED_PIN 3

static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 170;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_DataBus *bus = new Arduino_ESP32LCD8(7 /* DC */, 6 /* CS */, 8 /* WR */, 9 /* RD */, 39 /* D0 */, 40 /* D1 */, 41 /* D2 */, 42 /* D3 */,
                                             45 /* D4 */, 46 /* D5 */, 47 /* D6 */, 48 /* D7 */);

Arduino_GFX *gfx = new Arduino_ST7789(bus, 5 /* RST */, 0 /* rotation */, true /* IPS */, 170 /* width */, 320 /* height */, 35 /* col offset 1 */,
                                      0 /* row offset 1 */, 35 /* col offset 2 */, 0 /* row offset 2 */);

TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS820_SLAVE_ADDRESS, PIN_TOUCH_RES);

DHT dht(DHTPIN, DHTTYPE);

float temperature = 0.0;
float humidity = 0.0;
int led_flag = 0;
int led_status = 1;

void setup()
{
    USBSerial.begin(115200); /* prepare for possible serial debug */

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, led_status);

    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);

    pinMode(PIN_LCD_RST, OUTPUT);
    digitalWrite(PIN_LCD_RST, HIGH);

    pinMode(PIN_TOUCH_RES, OUTPUT);
    digitalWrite(PIN_TOUCH_RES, LOW);
    delay(500);
    digitalWrite(PIN_TOUCH_RES, HIGH);

    ledcSetup(0, 2000, 8);
    ledcAttachPin(PIN_LCD_BL, 0);
    ledcWrite(0, 255);

    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    if (!touch.init())
    {
        USBSerial.println("Touch IC not found");
        Serial.println("Touch IC not found");
    }

    dht.begin();

    gfx->begin();
    gfx->setRotation(1);

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    USBSerial.println(LVGL_Arduino);
    USBSerial.println("I am LVGL_Arduino");

    lv_init();
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

    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 20480, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(Task_my, "Task_my", 40960, NULL, 3, NULL, 1);
}

void loop()
{
}

// -------------------------------------------------------------------------------------

void Task_TFT(void *pvParameters)
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(10);
    }
}

void Task_my(void *pvParameters)
{
    long task_runtime_1 = 0;
    long task_runtime_2 = 0;
    long task_runtime_3 = 0;
    while (1)
    {
        if ((millis() - task_runtime_1) > 5000)
        {
            my_func_1();
            task_runtime_1 = millis();
        }
        if ((millis() - task_runtime_2) > 100)
        {
            my_func_2();
            task_runtime_2 = millis();
        }
        if ((millis() - task_runtime_3) > 200)
        {
            my_func_3();
            task_runtime_3 = millis();
        }
        vTaskDelay(100);
    }
}

void my_func_1()
{
    sensor_read();

    lv_bar_set_value(ui_Bar1, (int)temperature, LV_ANIM_OFF);
    lv_bar_set_value(ui_Bar2, (int)humidity, LV_ANIM_OFF);

    char temp[20];
    sprintf(temp, "Temp:\t%.1f C", temperature);
    lv_label_set_text(ui_Label1, temp);
    sprintf(temp, "Humi:\t%.1f %%", humidity);
    lv_label_set_text(ui_Label4, temp);
}

void my_func_2()
{
    if (digitalRead(BUTTON_PIN) == 1)
    {
        USBSerial.println("Button Presh");
        lv_bar_set_value(ui_Bar4, 1, LV_ANIM_OFF);
        // lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);
        // lv_switch_on(ui_Switch1, LV_ANIM_ON)
    }
    else
    {
        lv_bar_set_value(ui_Bar4, 0, LV_ANIM_OFF);
        // lv_obj_clear_state(ui_Switch1, LV_STATE_CHECKED);
        // lv_switch_off(ui_Switch1, LV_ANIM_ON)
        // USBSerial.println("Button Release");
    }
}

void my_func_3()
{
    if (led_flag == 1)
    {
        USBSerial.println("Led Toggle");
        // USBSerial.println(lv_obj_get_state());
        led_status = (led_status + 1) % 2;
        digitalWrite(LED_PIN, led_status);
        led_flag = 0;
    }
}

void sensor_read()
{
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    USBSerial.print(F("Humidity: "));
    USBSerial.print(humidity);
    USBSerial.print(F("%  Temperature: "));
    USBSerial.print(temperature);
    USBSerial.println(F("°C "));
}

//------------------------------------------------------------------------

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