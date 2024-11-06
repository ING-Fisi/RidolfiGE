/* MQTT over SSL Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h"
#include "WifiConnect.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"


#include <stdint.h>
#include "mqtt_client.h"
#include "esp_tls.h"
#include "gcpjwt.h"
#include "mqtt.h"
#include "fsntp.h"
#include "utility.h"
#include "modbus.h"


extern bool wifi_connect;

#define GOOGLE_SNTP "time.google.com"
#define DEFAULT_SNTP_SERVER GOOGLE_SNTP // Select one of the previous servers

static const char *TAG = "MQTTS_EXAMPLE";


esp_mqtt_client_handle_t mqttc;
esp_mqtt_client_config_t mqttcfg;
bool isconnected = false;
extern uint16_t array_modbus[128];

char payload_char[2000];
char tmp_macstr[20];


//************************* LED ******************************//
#define GPIO_OUTPUT_IO_0    33
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_IO_0)



#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
#endif
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");

//
// Note: this function is for testing purposes only publishing part of the active partition
//       (to be checked against the original binary)
//
//static void send_binary(esp_mqtt_client_handle_t client)
//{
//	spi_flash_mmap_handle_t out_handle;
//	const void *binary_address;
//	const esp_partition_t *partition = esp_ota_get_running_partition();
//	esp_partition_mmap(partition, 0, partition->size, SPI_FLASH_MMAP_DATA, &binary_address, &out_handle);
//	// sending only the configured portion of the partition (if it's less than the partition size)
//	int binary_size = MIN(CONFIG_BROKER_BIN_SIZE_TO_SEND,partition->size);
//	int msg_id = esp_mqtt_client_publish(client, "/topic/binary", binary_address, binary_size, 0, 0);
//	ESP_LOGI(TAG, "binary sent with msg_id=%d", msg_id);
//}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
//static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
//{
//	ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
//	esp_mqtt_event_handle_t event = event_data;
//	esp_mqtt_client_handle_t client = event->client;
//	int msg_id;
//	switch ((esp_mqtt_event_id_t)event_id) {
//	case MQTT_EVENT_CONNECTED:
//		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
//		msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
//		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
//
//		msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
//		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
//
//		msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
//		ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
//		break;
//	case MQTT_EVENT_DISCONNECTED:
//		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
//		mqtt_app_start( &mqttc, &mqttcfg );
//		break;
//
//	case MQTT_EVENT_SUBSCRIBED:
//		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//		msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
//		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
//		break;
//	case MQTT_EVENT_UNSUBSCRIBED:
//		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
//		break;
//	case MQTT_EVENT_PUBLISHED:
//		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
//		break;
//	case MQTT_EVENT_DATA:
//		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
//		printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
//		printf("DATA=%.*s\r\n", event->data_len, event->data);
//		if (strncmp(event->data, "send binary please", event->data_len) == 0) {
//			ESP_LOGI(TAG, "Sending the binary");
//			send_binary(client);
//		}
//		break;
//	case MQTT_EVENT_ERROR:
//		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
//		if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
//			ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
//			ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
//			ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
//					strerror(event->error_handle->esp_transport_sock_errno));
//		} else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
//			ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
//		} else {
//			ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
//		}
//		break;
//	default:
//		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
//		break;
//	}
//}


esp_err_t mqtt_event_handler( esp_mqtt_event_handle_t event ) {
	esp_mqtt_client_handle_t client = event->client;
	//int msg_id = 0;
	// your_context_t *context = event->context;

	switch (event->event_id) {

	case MQTT_EVENT_CONNECTED:
		ESP_LOGW(TAG, "MQTT_EVENT_CONNECTED");
		//mqtt_service_state = MQTT_SERV_CONNECTED;
		//msg_id = esp_mqtt_client_subscribe(client, "/devices/prototype1/events", 1);
		//ESP_LOGD(TAG, "sent subscribe successful, msg_id=%d", msg_id);
		esp_mqtt_client_subscribe(client, DATA_TOPIC_SUB, 1);
		set_mqtt_service_state( MQTT_SERV_CONNECTED );
		break;

	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
		set_mqtt_service_state( MQTT_SERV_DISCONNECTED );

		if(wifi_connect == true)
		{
			mqtt_app_start( &mqttc, &mqttcfg );
		}
		else
		{
			esp_restart();
		}



		//            realloc_buff(10);
		//            mqtt_service_state = MQTT_SERV_DISCONNECTED;
		break;

	case MQTT_EVENT_SUBSCRIBED:
		//            realloc_buff(8192);
		ESP_LOGW(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		//            msg_id = esp_mqtt_client_publish(client, "/devices/prototype1/events", "data", 0, 1, 0);
		//            ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);
		//set_mqtt_service_state( MQTT_SERV_SUBCRIBED );
		break;

	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGW(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;

	case MQTT_EVENT_PUBLISHED:
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;

	case MQTT_EVENT_DATA:
		ESP_LOGW(TAG, "MQTT_EVENT_DATA");
		ESP_LOGI(TAG,"TOPIC=%.*s\r\n", event->topic_len, event->topic);
		ESP_LOGI(TAG,"DATA=%.*s\r\n", event->data_len, event->data);
		//		if ( strncmp( event->topic, get_gcpiot_sub_topic_cmd(), strlen(get_gcpiot_sub_topic_cmd()) ) ) {
		//			event->data[event->data_len] = 0;
		//			//mqtt_cmd_parse( &curdev, event->data );
		//		}
		break;

	case MQTT_EVENT_ERROR:
		ESP_LOGW(TAG, "MQTT_EVENT_ERROR");
		//            mqtt_service_state = MQTT_SERV_ERROR;
		//            realloc_buff(10);
		int mbedtls_err = 0;
		esp_err_t err = esp_tls_get_and_clear_last_error(event->error_handle, &mbedtls_err, NULL);
		ESP_LOGD(TAG, "Last esp error code: 0x%x", err);
		ESP_LOGD(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
		set_mqtt_service_state( MQTT_SERV_ERROR );
		break;

	default:
		ESP_LOGW(TAG, "Other event id:%d", event->event_id);
		break;
	}
	return ESP_OK;
}


void mqtt_app_start_main(void)
{

	mqttcfg.uri = GCPIOT_BROKER_URI;
	mqttcfg.event_handle = mqtt_event_handler;
	mqttcfg.task_stack = 5*(1024);

	//ESP_ERROR_CHECK( mqtt_app_start( &mqttc, &mqttcfg ) );
	mqtt_app_start( &mqttc, &mqttcfg );

}



bool toggle = false;


void ctrl_tsk(void) {

	ESP_ERROR_CHECK(master_init());

	while(wifi_connect == false)
	{
		vTaskDelay(200/portTICK_PERIOD_MS);
	}

	mqtt_app_start_main();

	get_mac_str(tmp_macstr);


	while (1) {

		esp_err_t err = request_modbus_info();

		if(err == ESP_OK)
		{

			gpio_set_level(GPIO_OUTPUT_IO_0, 1000);

			if(get_mqtt_service_state()==MQTT_SERV_CONNECTED)
			{


				//•	5 (WAR0)
				//•	6 (WAR1)
				//•	17 (WAHR0)
				//•	18 (WAHR1)
				//•	19 (WAHR2)
				//•	20 (WAHS0)
				//•	21 (WAHS1)
				//•	22 (WAHS2)
				//•	23 (WAHT0)
				//•	24 (WAHT1)
				//•	25 (WAHT2)
				//•	67 (AMP)
				//•	76 (C_R)
				//•	77 (C_S)
				//•	78 (C_A)

				//************** fill payload *****************//
				sprintf(payload_char,
						"{"
						"\"rt\": true,"
						"\"u\": \"%s\"," //"\"u\": \"testdev\","
						"\"ts\": %ld,"
						"\"m\": "
						"[{"

						"\"m\": \"_1_5\",\"v\": %d"// WAR0
						"},{"

						"\"m\": \"_1_6\",\"v\": %d"// WAR1
						"},{"

						"\"m\": \"_1_7\",\"v\": %d"// ERR0 Flag d'Allarme
						"},{"

						"\"m\": \"_1_8\",\"v\": %d"// ERR1 Flag d'Allarme
						"},{"

						"\"m\": \"_1_14\",\"v\": %d"// MANUP Manutenzione Programmata
						"},{"

						"\"m\": \"_1_15\",\"v\": %d"// ORE0 Ore Lavorate
						"},{"

						"\"m\": \"_1_16\",\"v\": %d"// ORE1 Ore Lavorate, minuti
						"},{"

						"\"m\": \"_1_17\",\"v\": %d"// WAHR0
						"},{"

						"\"m\": \"_1_18\",\"v\": %d"// WAHR1
						"},{"

						"\"m\": \"_1_19\",\"v\": %d"// WAHR2
						"},{"

						"\"m\": \"_1_20\",\"v\": %d"// WAHS0
						"},{"

						"\"m\": \"_1_21\",\"v\": %d"// WAHS1
						"},{"

						"\"m\": \"_1_22\",\"v\": %d"// WAHS2
						"},{"

						"\"m\": \"_1_23\",\"v\": %d"// WAHT0
						"},{"

						"\"m\": \"_1_24\",\"v\": %d"// WAHT1
						"},{"

						"\"m\": \"_1_25\",\"v\": %d"// WAHT2
						"},{"

						"\"m\": \"_1_61\",\"v\": %d"// BAR Pressione Olio
						"},{"

						"\"m\": \"_1_62\",\"v\": %d"// GRD Temperatura Acqua
						"},{"

						"\"m\": \"_1_63\",\"v\": %d"// VBAT Tensione Batteria, decimi di V
						"},{"

						"\"m\": \"_1_66\",\"v\": %d"// AIN0 Livello Carburante, %
						"},{"

						"\"m\": \"_1_67\",\"v\": %d"// AMP
						"},{"

						"\"m\": \"_1_68\",\"v\": %d"// VGE Tensione G.E. Massima, V
						"},{"

						"\"m\": \"_1_69\",\"v\": %d"// FREQ Frequenza in decimi di Hz
						"},{"

						"\"m\": \"_1_76\",\"v\": %d"// C_R
						"},{"

						"\"m\": \"_1_77\",\"v\": %d"// C_S
						"},{"

						"\"m\": \"_1_78\",\"v\": %d"// C_A
						"}]"


						"}",

						//•	76 (C_R)
						//•	77 (C_S)
						//•	78 (C_A)

						tmp_macstr,
						get_curtimestamp(),//JGuardian_param_istance.tp.tv_sec,
						array_modbus[5],
						array_modbus[6],
						array_modbus[7],
						array_modbus[8],
						array_modbus[14],
						array_modbus[15],
						array_modbus[16],
						array_modbus[17],
						array_modbus[18],
						array_modbus[19],
						array_modbus[20],
						array_modbus[21],
						array_modbus[22],
						array_modbus[23],
						array_modbus[24],
						array_modbus[25],
						array_modbus[61],
						array_modbus[62],
						array_modbus[63],
						array_modbus[66],
						array_modbus[67],
						array_modbus[68],
						array_modbus[69],
						array_modbus[76],
						array_modbus[77],
						array_modbus[78]
				);

				int ret = esp_mqtt_client_publish(mqttc,DATA_TOPIC,payload_char, strlen(payload_char), 1, 0);

				ESP_LOGI(TAG, "START PUBLISH %d",ret);

				vTaskDelay(60 * 1000/portTICK_PERIOD_MS);
			}
		}
		else
		{

			if(toggle == true)
			{
				toggle = false;
				gpio_set_level(GPIO_OUTPUT_IO_0, 0);

			}
			else
			{
				toggle = true;
				gpio_set_level(GPIO_OUTPUT_IO_0, 1000);
			}

		}

		vTaskDelay(1 * 1000/portTICK_PERIOD_MS);

	} // task main cycle: end

	vTaskDelete(NULL);
}



void app_main(void)
{
	ESP_LOGI(TAG, "[APP] Startup..");
	ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
	ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

	esp_log_level_set("*", ESP_LOG_INFO);
	esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
	esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
	esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
	esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
	esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());


	//zero-initialize the config structure.
	gpio_config_t io_conf = {};
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);



	gpio_set_level(GPIO_OUTPUT_IO_0, 0);

	/* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
	 * Read "Establishing Wi-Fi or Ethernet Connection" section in
	 * examples/protocols/README.md for more information about this function.
	 */
	ESP_ERROR_CHECK(fisi_example_connect());


	//***************************************************************************//
	while (1) {
		if ( sntp_init_time( DEFAULT_SNTP_SERVER, 20) != 0 ) { // UNIFI_SNTP
			ESP_LOGW(TAG,"fail obtaining time from specified SNTP server.");
		}
		else {
			break;
		}
	}


	gpio_set_level(GPIO_OUTPUT_IO_0, 1000);

	//*************************************************************************//

	xTaskCreatePinnedToCore((TaskFunction_t)ctrl_tsk, "ctrl_tsk", 1024*5, NULL, 4, NULL, 1/*tskNO_AFFINITY*/);
}
