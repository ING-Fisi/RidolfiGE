/*
 * modbus.h
 *
 *  Created on: 27 nov 2023
 *      Author: clott
 */

#ifndef MODBUS_H_
#define MODBUS_H_


#include "string.h"
#include "esp_log.h"
#include "modbus_params.h"  // for modbus parameters structures
#include "mbcontroller.h"
#include "sdkconfig.h"

esp_err_t master_init(void);
void master_operation_func(void *arg);
esp_err_t request_modbus_info(void);


#endif /* MODBUS_H_ */
