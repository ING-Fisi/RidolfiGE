// Copyright 2016-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "modbus.h"



#define CONFIG_MB_UART_PORT_NUM 2
#define CONFIG_MB_UART_BAUD_RATE 19200
#define CONFIG_MB_UART_RXD 16
#define CONFIG_MB_UART_TXD 17
#define CONFIG_MB_UART_RTS 4
#define CONFIG_MB_COMM_MODE_RTU 1

#define MB_PORT_NUM     (CONFIG_MB_UART_PORT_NUM)   // Number of UART port used for Modbus connection
#define MB_DEV_SPEED    (CONFIG_MB_UART_BAUD_RATE)  // The communication speed of the UART

// Note: Some pins on target chip cannot be assigned for UART communication.
// See UART documentation for selected board and target to configure pins using Kconfig.

// The number of parameters that intended to be used in the particular control process
#define MASTER_MAX_CIDS num_device_parameters

// Number of reading of parameters from slave
#define MASTER_MAX_RETRY 30

// Timeout to update cid over Modbus
#define UPDATE_CIDS_TIMEOUT_MS          (500)
#define UPDATE_CIDS_TIMEOUT_TICS        (UPDATE_CIDS_TIMEOUT_MS / portTICK_RATE_MS)

// Timeout between polls
#define POLL_TIMEOUT_MS                 (1)
#define POLL_TIMEOUT_TICS               (POLL_TIMEOUT_MS / portTICK_RATE_MS)

// The macro to get offset for parameter in the appropriate structure
#define HOLD_OFFSET(field) ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))
#define INPUT_OFFSET(field) ((uint16_t)(offsetof(input_reg_params_t, field) + 1))
#define COIL_OFFSET(field) ((uint16_t)(offsetof(coil_reg_params_t, field) + 1))
// Discrete offset macro
#define DISCR_OFFSET(field) ((uint16_t)(offsetof(discrete_reg_params_t, field) + 1))

#define STR(fieldname) ((const char*)( fieldname ))
// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

static const char *TAG = "MASTER_TEST";

uint16_t array_modbus[128] = {0};
uint16_t array_analog[4] = {0};

// Enumeration of modbus device addresses accessed by master device
enum {
	MB_DEVICE_ADDR1 = 1, // Only one slave device used for the test (add other slave addresses here)
	MB_ANALOG_BOARD = 2
};

// Enumeration of all supported CIDs for device (used in parameter definition table)
enum {
	CID_INP_DATA_0 = 0,
	CID_HOLD_DATA_0,
	CID_INP_DATA_1,
	CID_HOLD_DATA_1,
	CID_INP_DATA_2,
	CID_HOLD_DATA_2,
	CID_HOLD_TEST_REG,
	CID_RELAY_P1,
	CID_RELAY_P2,
	CID_COUNT
};


#define NUM_REG 80

// Example Data (Object) Dictionary for Modbus parameters:
// The CID field in the table must be unique.
// Modbus Slave Addr field defines slave address of the device with correspond parameter.
// Modbus Reg Type - Type of Modbus register area (Holding register, Input Register and such).
// Reg Start field defines the start Modbus register number and Reg Size defines the number of registers for the characteristic accordingly.
// The Instance Offset defines offset in the appropriate parameter structure that will be used as instance to save parameter value.
// Data Type, Data Size specify type of the characteristic and its data size.
// Parameter Options field specifies the options that can be used to process parameter value (limits or masks).
// Access Mode - can be used to implement custom options for processing of characteristic (Read/Write restrictions, factory mode values and etc).
const mb_parameter_descriptor_t device_parameters[] = {
		// { CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size, Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
		{ CID_INP_DATA_0, STR("Data_channel_0"), STR("Volts"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0, NUM_REG,
				INPUT_OFFSET(input_data0), PARAM_TYPE_U16, 2 * NUM_REG, OPTS( 0, 65536, 1 ), PAR_PERMS_READ_WRITE_TRIGGER },
				{ CID_HOLD_DATA_0, STR("Data_channel_1"), STR("Ampere"), MB_ANALOG_BOARD, MB_PARAM_HOLDING, 0, 4,
						INPUT_OFFSET(input_data0_analog), PARAM_TYPE_U16, 2 * 4, OPTS( 0, 65536, 1 ), PAR_PERMS_READ_WRITE_TRIGGER },




						//				{ CID_HOLD_DATA_0, STR("Humidity_1"), STR("%rH"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0, 2,
						//						HOLD_OFFSET(holding_data0), PARAM_TYPE_FLOAT, 4, OPTS( 0, 100, 1 ), PAR_PERMS_READ_WRITE_TRIGGER },
						//						{ CID_INP_DATA_1, STR("Temperature_1"), STR("C"), MB_DEVICE_ADDR1, MB_PARAM_INPUT, 2, 2,
						//								INPUT_OFFSET(input_data1), PARAM_TYPE_FLOAT, 4, OPTS( -40, 100, 1 ), PAR_PERMS_READ_WRITE_TRIGGER },
						//								{ CID_HOLD_DATA_1, STR("Humidity_2"), STR("%rH"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 2, 2,
						//										HOLD_OFFSET(holding_data1), PARAM_TYPE_FLOAT, 4, OPTS( 0, 100, 1 ), PAR_PERMS_READ_WRITE_TRIGGER },
						//										{ CID_INP_DATA_2, STR("Temperature_2"), STR("C"), MB_DEVICE_ADDR1, MB_PARAM_INPUT, 4, 2,
						//												INPUT_OFFSET(input_data2), PARAM_TYPE_FLOAT, 4, OPTS( -40, 100, 1 ), PAR_PERMS_READ_WRITE_TRIGGER },
						//												{ CID_HOLD_DATA_2, STR("Humidity_3"), STR("%rH"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 4, 2,
						//														HOLD_OFFSET(holding_data2), PARAM_TYPE_FLOAT, 4, OPTS( 0, 100, 1 ), PAR_PERMS_READ_WRITE_TRIGGER },
						//														{ CID_HOLD_TEST_REG, STR("Test_regs"), STR("__"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 10, 58,
						//																HOLD_OFFSET(test_regs), PARAM_TYPE_ASCII, 116, OPTS( 0, 100, 1 ), PAR_PERMS_READ_WRITE_TRIGGER },
						//																{ CID_RELAY_P1, STR("RelayP1"), STR("on/off"), MB_DEVICE_ADDR1, MB_PARAM_COIL, 0, 8,
						//																		COIL_OFFSET(coils_port0), PARAM_TYPE_U16, 2, OPTS( BIT1, 0, 0 ), PAR_PERMS_READ_WRITE_TRIGGER },
						//																		{ CID_RELAY_P2, STR("RelayP2"), STR("on/off"), MB_DEVICE_ADDR1, MB_PARAM_COIL, 8, 8,
						//																				COIL_OFFSET(coils_port1), PARAM_TYPE_U16, 2, OPTS( BIT0, 0, 0 ), PAR_PERMS_READ_WRITE_TRIGGER }
};

// Calculate number of parameters in the table
const uint16_t num_device_parameters = (sizeof(device_parameters)/sizeof(device_parameters[0]));

// The function to get pointer to parameter storage (instance) according to parameter description table
static void* master_get_param_data(const mb_parameter_descriptor_t* param_descriptor)
{
	assert(param_descriptor != NULL);
	void* instance_ptr = NULL;
	if (param_descriptor->param_offset != 0) {
		switch(param_descriptor->mb_param_type)
		{
		case MB_PARAM_HOLDING:
			instance_ptr = ((void*)&holding_reg_params + param_descriptor->param_offset - 1);
			break;
		case MB_PARAM_INPUT:
			instance_ptr = ((void*)&input_reg_params + param_descriptor->param_offset - 1);
			break;
		case MB_PARAM_COIL:
			instance_ptr = ((void*)&coil_reg_params + param_descriptor->param_offset - 1);
			break;
		case MB_PARAM_DISCRETE:
			instance_ptr = ((void*)&discrete_reg_params + param_descriptor->param_offset - 1);
			break;
		default:
			instance_ptr = NULL;
			break;
		}
	} else {
		ESP_LOGE(TAG, "Wrong parameter offset for CID #%d", param_descriptor->cid);
		assert(instance_ptr != NULL);
	}
	return instance_ptr;
}

esp_err_t request_modbus_info(void)
{

	esp_err_t err = ESP_OK;
	const mb_parameter_descriptor_t* param_descriptor = NULL;
	uint16_t cid = 0;

	err = mbc_master_get_cid_info(cid, &param_descriptor);

	if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL))
	{

		void* temp_data_ptr = master_get_param_data(param_descriptor);
		assert(temp_data_ptr);
		uint8_t type = 0;

		err = mbc_master_get_parameter(cid, (char*)param_descriptor->param_key,(uint8_t*)array_modbus, &type);

		if (err == ESP_OK) {


			for(int i = 0;i<NUM_REG;i++)
			{
				ESP_LOGI(TAG, "[%d]", array_modbus[i]);
			}



		} else {
			ESP_LOGE(TAG, "Characteristic #%d (%s) read fail, err = 0x%x (%s).",
					param_descriptor->cid,
					(char*)param_descriptor->param_key,
					(int)err,
					(char*)esp_err_to_name(err));
		}

	}

	return err;

}

// User operation function to read slave values and check alarm
void master_operation_func(void *arg)
{
	esp_err_t err = ESP_OK;
	//float value = 0;
	bool alarm_state = false;
	const mb_parameter_descriptor_t* param_descriptor = NULL;

	ESP_LOGI(TAG, "Start modbus test...");


	while(1)
	{

		uint16_t cid = 0;
		err = mbc_master_get_cid_info(cid, &param_descriptor);
		if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL))
		{

			void* temp_data_ptr = master_get_param_data(param_descriptor);
			assert(temp_data_ptr);
			uint8_t type = 0;

			err = mbc_master_get_parameter(cid, (char*)param_descriptor->param_key,(uint8_t*)array_modbus, &type);

			if (err == ESP_OK) {


				for(int i = 0;i<NUM_REG;i++)
				{
					ESP_LOGI(TAG, "[%d]", array_modbus[i]);
				}



			} else {
				ESP_LOGE(TAG, "Characteristic #%d (%s) read fail, err = 0x%x (%s).",
						param_descriptor->cid,
						(char*)param_descriptor->param_key,
						(int)err,
						(char*)esp_err_to_name(err));
			}

		}

		vTaskDelay(UPDATE_CIDS_TIMEOUT_TICS); //


		//		cid = 1;
		//		err = mbc_master_get_cid_info(cid, &param_descriptor);
		//		if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL))
		//		{
		//
		//			void* temp_data_ptr = master_get_param_data(param_descriptor);
		//			assert(temp_data_ptr);
		//			uint8_t type = 0;
		//
		//			err = mbc_master_get_parameter(cid, (char*)param_descriptor->param_key,(uint8_t*)array_analog, &type);
		//
		//			if (err == ESP_OK) {
		//
		//				ESP_LOGI(TAG, "%d,%d,%d,%d",
		//						array_analog[0],
		//						array_analog[1],
		//						array_analog[2],
		//						array_analog[3]);
		//
		//
		//			} else {
		//				ESP_LOGE(TAG, "Characteristic #%d (%s) read fail, err = 0x%x (%s).",
		//						param_descriptor->cid,
		//						(char*)param_descriptor->param_key,
		//						(int)err,
		//						(char*)esp_err_to_name(err));
		//			}
		//
		//		}
		//
		//		vTaskDelay(UPDATE_CIDS_TIMEOUT_TICS); //
	}

	if (alarm_state) {
		ESP_LOGI(TAG, "Alarm triggered by cid #%d.",
				param_descriptor->cid);
	} else {
		ESP_LOGE(TAG, "Alarm is not triggered after %d retries.",
				MASTER_MAX_RETRY);
	}
	ESP_LOGI(TAG, "Destroy master...");
	ESP_ERROR_CHECK(mbc_master_destroy());
}

// Modbus master initialization
esp_err_t master_init(void)
{
	// Initialize and start Modbus controller
	mb_communication_info_t comm = {
			.port = MB_PORT_NUM,
#if CONFIG_MB_COMM_MODE_ASCII
			.mode = MB_MODE_ASCII,
#elif CONFIG_MB_COMM_MODE_RTU
			.mode = MB_MODE_RTU,
#endif
			.baudrate = MB_DEV_SPEED,
			.parity = UART_PARITY_EVEN
	};
	void* master_handler = NULL;

	esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler);
	MB_RETURN_ON_FALSE((master_handler != NULL), ESP_ERR_INVALID_STATE, TAG,
			"mb controller initialization fail.");
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
			"mb controller initialization fail, returns(0x%x).",
			(uint32_t)err);
	err = mbc_master_setup((void*)&comm);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
			"mb controller setup fail, returns(0x%x).",
			(uint32_t)err);

	// Set UART pin numbers
	err = uart_set_pin(MB_PORT_NUM, CONFIG_MB_UART_TXD, CONFIG_MB_UART_RXD,
			CONFIG_MB_UART_RTS, UART_PIN_NO_CHANGE);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
			"mb serial set pin failure, uart_set_pin() returned (0x%x).", (uint32_t)err);

	err = mbc_master_start();
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
			"mb controller start fail, returns(0x%x).",
			(uint32_t)err);

	// Set driver mode to Half Duplex
	err = uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
			"mb serial set mode failure, uart_set_mode() returned (0x%x).", (uint32_t)err);

	vTaskDelay(5);
	err = mbc_master_set_descriptor(&device_parameters[0], num_device_parameters);
	MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
			"mb controller set descriptor fail, returns(0x%x).",
			(uint32_t)err);
	ESP_LOGI(TAG, "Modbus master stack initialized...");
	return err;
}

//void app_main(void)
//{
//	// Initialization of device peripheral and objects
//	ESP_ERROR_CHECK(master_init());
//	vTaskDelay(10);
//
//	master_operation_func(NULL);
//}
