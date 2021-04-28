// WIFI imports and settings
/* ------------------------------------------------------------------------ */

//get this info from the "My Key" section of the Adafruit IO website
#define IO_USERNAME  "increment"
#define IO_KEY       "aio_eMSd21wfnef8F3Homj2DMgmUnmSV"

//Input your WiFi username and password here so the ESP8266 can connect to it
#define WIFI_SSID "Atlas"
#define WIFI_PASS "Jp26772!"

#include "AdafruitIO_WiFi.h"

#if defined(USE_AIRLIFT) || defined(ADAFRUIT_METRO_M4_AIRLIFT_LITE) ||         \
    defined(ADAFRUIT_PYPORTAL)
// Configure the pins used for the ESP32 connection
#if !defined(SPIWIFI_SS) // if the wifi definition isnt in the board variant
// Don't change the names of these #define's! they match the variant ones
#define SPIWIFI SPI
#define SPIWIFI_SS 10 // Chip select pin
#define NINA_ACK 9    // a.k.a BUSY or READY pin
#define NINA_RESETN 6 // Reset pin
#define NINA_GPIO0 -1 // Not connected
#endif
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS, SPIWIFI_SS,
                   NINA_ACK, NINA_RESETN, NINA_GPIO0, &SPIWIFI);
#else
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
#endif

// BLUEFRUIT IMPORTS / SETTINGS
/* ------------------------------------------------------------------------ */
// these settings used in both SW UART, HW UART and SPI mode
#define BUFSIZE 128 // size of read buffer for incoming data
#define VERBOSE_MODE true // enables debug output

// Software UART settings
#define BLUEFRUIT_SWUART_RXO_PIN 9
#define BLUEFRUIT_SWUART_TXO_PIN 10 
#define BLUEFRUIT_UART_CTS_PIN 11 // unused
#define BLUEFRUIT_UART_RTS_PIN -1

// Hardware UART settings
#ifdef Serial1
  #define BLUEFRUIT_HWSERIAL_NAME Serial1
#endif

#define BLUEFRUIT_UART_MODE_PIN 4
