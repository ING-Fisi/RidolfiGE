/* mqtt.h
 */

#ifndef MAIN_X_MQTT_H_
#define MAIN_X_MQTT_H_

/* --------------------- INCLUDES ------------------------ *
 * ------------------------------------------------------- */
// IDF
#include "mqtt_client.h"



/* --------------------- DEFINES ------------------------- *
 * ------------------------------------------------------- */
typedef enum mqtt_servicve_state {
	MQTT_SERV_ERROR = 0,
	MQTT_SERV_DISCONNECTED,
	MQTT_SERV_CONNECTED,
	MQTT_SERV_SUBCRIBED,
} mqtt_serv_state_t;



/* --------------------- VARIABLES ----------------------- *
 * ------------------------------------------------------- */

/* --------------------- FUNCTIONS ----------------------- *
 * ------------------------------------------------------- */
/* Function that initialize MQTT service and parameters,	*
 * creates a dedicated task.								*
 *															*
 * @param mqtt_client	: MQTT client handle, has to be		*
 * 						  passed from caller.				*
 * @param mqtt_ext_cfg	: MQTT configuration parameters,	*
 * 						  if NULL will be use defaults.		*
 *															*
 * @return result		: 0, -1 or -2 (timeout)				*/
esp_err_t mqtt_app_start( esp_mqtt_client_handle_t *mqtt_client,esp_mqtt_client_config_t *mqtt_ext_cfg );

/* Function to get current MQTT service	state				*
 *															*
 * @param void					: (void)					*
 *															*
 * @return mqtt_serv_state_t	: service state				*/
mqtt_serv_state_t get_mqtt_service_state( void );

/* Function to set current MQTT service	state				*
 *															*
 * @param mqtt_serv_state_t		: new state					*
 *															*
 * @return void					: ( void )					*/
void set_mqtt_service_state( mqtt_serv_state_t s );

#endif /* MAIN_X_MQTT_H_ */
