#define TOUCH_MODULES_CST_SELF

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <TouchLib.h>
#include <WiFi.h>
#include "img_logo.h"
#include "pin_config.h"

//#include "JpegFunc.h"
//#include <SD.h>
#include <FS.h>


#define JPEG_FILENAME "/logo_320170.jpg"

Arduino_DataBus *bus = new Arduino_ESP32LCD8(7 /* DC */, 6 /* CS */, 8 /* WR */, 9 /* RD */, 39 /* D0 */, 40 /* D1 */, 41 /* D2 */, 42 /* D3 */,
                                             45 /* D4 */, 46 /* D5 */, 47 /* D6 */, 48 /* D7 */);

Arduino_GFX *gfx = new Arduino_ST7789(bus, 5 /* RST */, 0 /* rotation */, true /* IPS */, 170 /* width */, 320 /* height */, 35 /* col offset 1 */,
                                      0 /* row offset 1 */, 35 /* col offset 2 */, 0 /* row offset 2 */);

TouchLib touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS820_SLAVE_ADDRESS, PIN_TOUCH_RES);

int button_status = 0;

// pixel drawing callback
//static int jpegDrawCallback(JPEGDRAW *pDraw);

void setup()
{
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

    USBSerial.begin(115200);
    USBSerial.println("Hello MaTouch 1.9inch ");
    
    Serial.begin(115200);
    Serial.println("Hello MaTouch 1.9inch ");

    // Set WiFi to station mode and disconnect from an AP if it was previously connected.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    WiFi.begin("Makerfabs", "20160704");

    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    if (!touch.init())
    {
        USBSerial.println("Touch IC not found");
        Serial.println("Touch IC not found");
    }



    gfx->begin();
    gfx->setRotation(1);

#if 0
    if (!LittleFS.begin())
    {
      USBSerial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
    
#endif

    gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)gImage_logo_320X170, 320, 172);

    delay(1000);

    gfx->fillScreen(RED);
    delay(1000);
    gfx->fillScreen(GREEN);
    delay(1000);
    gfx->fillScreen(BLUE);
    delay(1000);
    gfx->fillScreen(WHITE);
    delay(1000);
    gfx->fillScreen(BLACK);
    delay(1000);    

    gfx->setTextSize(2);
    gfx->setTextColor(WHITE);
    gfx->println("Connecting WIFI...");
    USBSerial.println("Connecting WIFI...");
    Serial.println("Connecting WIFI...");
    int connect_count = 0;

    while (1)
    {

        if (WiFi.status() == WL_CONNECTED)
        {
            gfx->println("WIFI OK!");
            gfx->println(WiFi.localIP());
            USBSerial.println(WiFi.localIP());
            Serial.println(WiFi.localIP());
            delay(1000);
            break;
        }

        if (connect_count > 10)
        {
            gfx->setTextColor(RED);
            gfx->println("Can't connect WIFI");
            USBSerial.println("Can't connect WIFI");
            Serial.println("Can't connect WIFI");
            delay(5000);
            break;
        }

        connect_count++;
        delay(1000);
    }

    gfx->fillScreen(WHITE);
    gfx->setTextColor(BLACK);
    gfx->println(" Start Draw");
    if (WiFi.status() == WL_CONNECTED)
    {
      gfx->println(WiFi.localIP());
      gfx->println(WiFi.RSSI());
      USBSerial.print(WiFi.RSSI());    
      Serial.print(WiFi.RSSI());  
      delay(100);
    }
    // draw_button(button_status);
}

void loop(void)
{

    int x = -1;
    int y = -1;
    if (get_touch(&x, &y) == 1)
    {
        // if (x < 160)
        //     button_status = 1;
        // else
        //     button_status = 0;

        // draw_button(button_status);
        gfx->fillCircle(x, y, 5, RED);
    }
    delay(10);
    USBSerial.print(WiFi.RSSI());
    Serial.print(WiFi.RSSI());  
    // USBSerial.println("loop");
}

int get_touch(int *x, int *y)
{
    if (touch.read())
    {
        uint8_t n = touch.getPointNum();
        for (uint8_t i = 0; i < n; i++)
        {
            TP_Point t = touch.getPoint(i);
            int temp_x = t.y;
            int temp_y = map(t.x, 0, 150, 170, 0);
            //USBSerial.printf("x:%4d y:%4d\r\n", temp_x, temp_y);
            //Serial.printf("x:%4d y:%4d\r\n", temp_x, temp_y);
            *x = temp_x;
            *y = temp_y;
            break;
        }
        //USBSerial.println("------------------------------------------");
        //Serial.println("------------------------------------------");
        return 1;
    }

    return 0;
}

#if 0
// pixel drawing callback
static int jpegDrawCallback(JPEGDRAW *pDraw)
{
    // USBSerial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
    gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
    return 1;
}
#endif

void draw_button(int status)

{
    gfx->fillScreen(WHITE);
    if (status == 0)
        gfx->fillCircle(80, 85, 40, RED);
    else
        gfx->fillCircle(240, 85, 40, RED);
}
