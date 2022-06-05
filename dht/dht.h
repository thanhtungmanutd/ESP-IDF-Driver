/******************************************************************************
*@file: dht.h
*@brief: header file for dht sensor
*@author: TungDT53
*@date: 5/4/2022
*******************************************************************************/
#ifndef _DHT_H_
#define _DHT_H_

// -----------------------------------------------------------------------------
//                              Typedefs
// -----------------------------------------------------------------------------

typedef enum
{
    DHT11 = 0,
    DHT22 = 1
}dht_type_t; /**< dht type definition */

typedef struct
{
    float temp;
    float humid;
    uint8_t check_sum;
}dht_data_type_t; /**< dht data type definition */

// -----------------------------------------------------------------------------
//                       Public Function Declarations
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 *    This function initialize the configuration for the DHT sensor
 *
 * @param type
 *  The type of DHT sensor. It can be DHT11 or DHT22
 * @param pin
 *  The GPIO pin that connected to the data pin of the sensor
 *
 ******************************************************************************/
void dht_init(dht_type_t type,gpio_num_t pin);

/***************************************************************************//**
 * @brief
 *    This function read the humidity and temperature from the sensor
 *
 * @return
 *   Return value is a struct that containing humidity and temperature
 * 
 ******************************************************************************/
dht_data_type_t dht_read(void);

#endif /* _DHT_H_ */

// -----------------------------------------------------------------------------
//                              EOF
// -----------------------------------------------------------------------------
