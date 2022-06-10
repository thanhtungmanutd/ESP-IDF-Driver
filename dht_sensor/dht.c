// -----------------------------------------------------------------------------
//                              Includes
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "esp32/rom/ets_sys.h"
#include "driver/gpio.h"
#include "dht.h"

// -----------------------------------------------------------------------------
//                              Variables
// -----------------------------------------------------------------------------

static dht_type_t dht_type;
static gpio_num_t dht_pin;

// -----------------------------------------------------------------------------
//                       Local Function Declarations
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 *  Send request signal to DHT sensor.
 *
 ******************************************************************************/
static void send_request(void);

/***************************************************************************//**
 * @brief
 *  Wait for the response from the DHT sensor.
 *
 ******************************************************************************/
static void wait_response(void);

/***************************************************************************//**
 * @brief
 *  Read one byte from DHT sensor.
 *
 * @return
 *  Return value is a byte of data.
 *
 ******************************************************************************/
static uint8_t dht_read_byte(void);

// -----------------------------------------------------------------------------
//                       Local Function Definitions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 *  Send request signal to DHT sensor.
 ******************************************************************************/
static void send_request(void)
{
    gpio_set_direction(dht_pin,GPIO_MODE_OUTPUT);
    gpio_set_level(dht_pin,0);
    if(dht_type == DHT11)
    {
        ets_delay_us(20000);
    }
    if(dht_type == DHT22)
    {
        ets_delay_us(1000);
    }
    gpio_set_level(dht_pin,1);
    ets_delay_us(30);
}

/***************************************************************************//**
 *  Wait for the response from the DHT sensor.
 ******************************************************************************/
static void wait_response(void)
{
    gpio_set_direction(dht_pin,GPIO_MODE_INPUT);
    while(!gpio_get_level(dht_pin));
    while(gpio_get_level(dht_pin));
}

/***************************************************************************//**
 *  Read one byte from DHT sensor.
 ******************************************************************************/
static uint8_t dht_read_byte(void)
{
    uint8_t value = 0;
    for(uint8_t i = 0; i < 8; i++)
    {
        while(!gpio_get_level(dht_pin));
        ets_delay_us(30);
        if(gpio_get_level(dht_pin))
            value = (value<<1) | 0x01;
        else
            value = value<<1;
    while(gpio_get_level(dht_pin));
    }
    return value;
}

// -----------------------------------------------------------------------------
//                       Public Function Definitions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 *  Initialize the configuration for the DHT sensor.
 ******************************************************************************/
void dht_init(dht_type_t type,gpio_num_t pin)
{
    dht_type = type;
    dht_pin  = pin;
}

/***************************************************************************//**
 *  Read the humidity and temperature from the sensor.
 ******************************************************************************/
dht_data_type_t dht_read(void)
{
    dht_data_type_t value;
    uint8_t data[5];
    uint8_t sum;

    send_request();
    wait_response();

    if(dht_type == DHT22)
    {
        for(uint8_t i = 0; i < 5; i++)
        {
            data[i] = dht_read_byte();
        }

        sum = data[0]+data[1]+data[2]+data[3];

        if(sum == data[4])
        {
            value.check_sum = true;
            if(data[2] > 127)
                value.temp = (float)data[3]/10*(-1);
            else
                value.temp = (float)((data[2]<<8)|data[3])/10;
            value.humid = (float)((data[0]<<8)|data[1])/10;
        }
        else
            value = (dht_data_type_t){0,0,false};
    }
    else if(dht_type == DHT11)
    {
        for(int i=0;i<5;i++)
        {
            data[i] = dht_read_byte();
        }
        sum = data[0]+data[1]+data[2]+data[3];

        if(data[4] == sum)
        {
            value.humid = data[0];
            value.temp = data[2];
            for(int i=0;i<8;i++)
            {
                value.temp += ((data[3]>>(7-i))&0x01) * (float)pow(10,-(i+1));
                value.humid +=  ((data[1]>>(7-i))&0x01) * (float)pow(10,-(i+1));
            }
            value.check_sum = true;
        }
        else
        {
            value = (dht_data_type_t){0,0,false};
        }
    }
    return value;
}
// -----------------------------------------------------------------------------
//                              EOF
// -----------------------------------------------------------------------------
