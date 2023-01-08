#ifndef _RTC_DS1307_H_
#define _RTC_DS1307_H_

// -----------------------------------------------------------------------------
//                               Includes
// -----------------------------------------------------------------------------

#include "esp_err.h"
#include "driver/i2c.h"

// -----------------------------------------------------------------------------
//                               Typedefs
// -----------------------------------------------------------------------------

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t day_of_week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} date_time_t; /* struct to hold the date time value */

// -----------------------------------------------------------------------------
//                               Public functions
// -----------------------------------------------------------------------------

/***************************************************************************//**
 * @brief
 *  Initialize RTC DS1307.
 *
 * @param[in] i2c_num
 *  The I2C NUM to use for RTC DS1307.
 *
 ******************************************************************************/
void rtc_ds1307_init(i2c_port_t i2c_num);

/***************************************************************************//**
 * @brief
 *  Set date and time for RTC DS1307..
 *
 * @param[in] year
 *  The year that set to RTC DS1307.
 * @param[in] month
 *  The month that set to RTC DS1307.
 * @param[in] date
 *  The date that set to RTC DS1307.
 * @param[in] hour
 *  The hour that set to RTC DS1307.
 * @param[in] minute
 *  The minute that set to RTC DS1307.
 * @param[in] second
 *  The second that set to RTC DS1307.
 *
 * @retval ESP_OK   Success
 * @retval ESP_FAIL Fail
 ******************************************************************************/
esp_err_t rtc_ds1307_set_date_time(uint16_t year,
                                   uint8_t month,
                                   uint8_t date,
                                   uint8_t hour,
                                   uint8_t minute,
                                   uint8_t second);

/***************************************************************************//**
 * @brief
 *  Get date and time from RTC DS1307..
 *
 * @param[out] dt
 *  The date and time get from RTC DS1307.
 *
 * @retval ESP_OK   Success
 * @retval ESP_FAIL Fail
 ******************************************************************************/
esp_err_t rtc_ds1307_get_current_date_time(date_time_t *dt);

#endif /* _RTC_DS1307_H_ */