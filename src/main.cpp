/*
 Name:		WirelessStickLite Heltec LoraWan Receiver
 Created:	31-Mar-22 12:37:25
 Author:	Zhelyazkov (unrealbg)
*/

#include <ESP32_LoRaWAN.h>
#include <Arduino.h>


#define RF_FREQUENCY                                868000000 // Hz

#define TX_OUTPUT_POWER                             15        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]

#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]

#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]

#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx

#define LORA_SYMBOL_TIMEOUT                         0         // Symbols

#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);
void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);

typedef enum
{
    STATUS_LOWPOWER,
    STATUS_RX,
    STATUS_TX
}States_t;


int16_t txNumber;
States_t state;
bool sleepMode = false;
int16_t Rssi, rxSize;

uint32_t  license[4] = { 0xD5397DF0, 0x8573F814, 0x7A38C73D, 0x48E68607 };

void setup()
{
    Serial.begin(115200);
    while (!Serial);
    SPI.begin(SCK, MISO, MOSI, SS);
    Mcu.init(SS, RST_LoRa, DIO0, DIO1, license);


    txNumber = 0;
    Rssi = 0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxDone = OnRxDone;

    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);
    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
        LORA_SPREADING_FACTOR, LORA_CODINGRATE,
        LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
        true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
        LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
        LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
        0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
    state = STATUS_TX;
}


void loop()
{
    switch (state)
    {
    case STATUS_TX:
        delay(1000);
        txNumber++;
        sprintf(txpacket, "%s", "hello");
        sprintf(txpacket + strlen(txpacket), "%d", txNumber);
        sprintf(txpacket + strlen(txpacket), "%s", " Rssi : ");
        sprintf(txpacket + strlen(txpacket), "%d", Rssi);

        Radio.Send((uint8_t*)txpacket, strlen(txpacket));
        state = STATUS_LOWPOWER;
        break;
    case STATUS_RX:
        Radio.Rx(0);
        state = STATUS_LOWPOWER;
        break;
    case STATUS_LOWPOWER:
        LoRaWAN.sleep(CLASS_C, 0);
        break;
    default:
        break;
    }
}

void OnTxDone(void)
{
    state = STATUS_RX;
}

void OnTxTimeout(void)
{
    Radio.Sleep();
    state = STATUS_TX;
}
void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr)
{
    Rssi = rssi;
    rxSize = size;
    memcpy(rxpacket, payload, size);
    rxpacket[size] = '\0';
    Radio.Sleep();
    state = STATUS_TX;
}