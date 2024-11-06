
/* --------------------- INCLUDES ------------------------ *
 * ------------------------------------------------------- */
// ESP-IDF
#include "freertos/FreeRTOS.h"
#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_tls.h"
// Custom
#include "gcpjwt.h"
#include "mqtt.h"
#include "sdkconfig.h"

extern char device_id_string[150];

/* --------------------- DEFINES ------------------------- *
 * ------------------------------------------------------- */

//char rsa_priv_key[] = "(-----BEGIN PRIVATE KEY-----"
//		"MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQC8hM18PTgqm6Ek"
//		"JEC5BY2XMmUcYhbntn0lR3/k+iRh3BHaAiZ8UH58FfIE/22UV/CutyHS//9NNYIV"
//		"cUM2ZDCLlCPsDQHvczD8FBdyE7qR0B4Ylmi69deK59ApNiKgvqE/YkoR+gTexSdY"
//		"d33R3eucA89zOZ3CiS//h8i85RSjbLvbpO85D8amNSd3GTIFhMkJ93Cg5esRImfg"
//		"waooqbB5viR+0AzHU0E4xuSJ7u6XPVOYdR1tDMwR4CtSgEFmXt9SorVZAfYlC9nf"
//		"PUbEarud82mdrhTeDo4Jh5kG/QWKvgLyr0VfQ0fVO+q0z6+/c/rc0ILGUFvC2ue1"
//		"9XErsy8pAgMBAAECgf90l+LgqppfdHvY5+Xb8jNHfuk7AO+W1GqOPGDx946MpNTW"
//		"kZC4IScoV3HSEqgTKDnmESx2euGPAscZBzIMfvXpIXfwdUCv/yqWCIRC8TC78qqM"
//		"WXoOctSDv9AiQ4CMz/BfPZ3R/Kv7IvCd+Too32yth+J4sifadlysDa7EnZcY6/bo"
//		"2Wu/vOewwCANoR+8G2nVIF2ip0gteR0iGEIWDBq0YkPE9M+5o/sNYx4C1D0I6e2y"
//		"mN9NHlH5cUiIKGW8V2LhQoLuWRnAFItJfSzFKqbdX0yBOGDgLGC49yiq8wlPb+LQ"
//		"eapIfRA5AdlvObctNHtVHLqrAzHnI6+gHFR5CsECgYEA0M0DYCvDnG7AjGDjccfq"
//		"MNzABfv/kwzAuzwXbHb34mZo4jz7vAZqVjD/A9WdlqqgSWxnjRdsZSaTD5+CkFy2"
//		"HsbmUWJheF62c0VKNYCr4aYPyqioDbkRQDvJMFKz/gm8mRbr5zKk3SATFdMz2lAz"
//		"PGvllqlGkTnCjGhLpAm3nMECgYEA5yIYx5cj5f7+ff3LgxcywaWk8qTeZKeO3ieF"
//		"GRTjQFvZgtOn4y3RIPqPZ9d25ptkqcnPgq8dvR3tNo/Dxusx1YdBZh2il46Rgn4r"
//		"6NcNsV18qV5Nlbavc2vqR60JWB4fVEQoEyGEEePmGkBM5lIHMjNTImVh/UVS5Uik"
//		"XnHI5GkCgYA8DYrYAR5MgqoMdd1Unzvi+8qargSupW9VKo5OGtVmny+xSfrY9Dtn"
//		"OMZPswOVDtP06iHKOZ9qZDLNvoHNxWx345rgc1w3Q35UHuyeALjuuFU4Y1OaRs0I"
//		"XWma5V5xRRmiKi6FG1LBanUyxMDYHFLwINQyhdMO977VpTWEI2tUwQKBgQDcLr8s"
//		"Y+7uy1vk6kFVQGQ51XKtNCIoAPc46q0Al8VlKhqrZrYo1RzELe0vHzNFvALJPFdS"
//		"R0AtSLURpSL7/FIc+Bx1CwY5GrfDeC6cQUHqFQUM06lVVhMOAV/NqCmeqBNwUv1a"
//		"PDN0V4Z8u4SGZN42FG699ZbDj6bIhcsPrZWTAQKBgQCWSpSvQQ6cKIExjlde5Q2G"
//		"OfHBja8HijqY9X2lhfcY05OpEuj7fMxpLT0geqiGB0M4WKeSL9SCb6lJ/GAFe5TR"
//		"wJ3mAZilw/6ZwRnrty67dbMERkURPWFpm/6P3hPtmg2Ylfttcv7xIzjSkqeTnM/X"
//		"7NJQbhkdcZza7TSpC2LKhg=="
//		"-----END PRIVATE KEY-----)";
//
//char rsa_publ_key[] = "(-----BEGIN PUBLIC KEY-----"
//		"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvITNfD04KpuhJCRAuQWN"
//		"lzJlHGIW57Z9JUd/5PokYdwR2gImfFB+fBXyBP9tlFfwrrch0v//TTWCFXFDNmQw"
//		"i5Qj7A0B73Mw/BQXchO6kdAeGJZouvXXiufQKTYioL6hP2JKEfoE3sUnWHd90d3r"
//		"nAPPczmdwokv/4fIvOUUo2y726TvOQ/GpjUndxkyBYTJCfdwoOXrESJn4MGqKKmw"
//		"eb4kftAMx1NBOMbkie7ulz1TmHUdbQzMEeArUoBBZl7fUqK1WQH2JQvZ3z1GxGq7"
//		"nfNpna4U3g6OCYeZBv0Fir4C8q9FX0NH1TvqtM+vv3P63NCCxlBbwtrntfVxK7Mv"
//		"KQIDAQAB"
//		"-----END PUBLIC KEY-----)";

/* --------------------- VARIABLES ----------------------- *
 * ------------------------------------------------------- */
static const char* TAG = "MQTT";

// IOT Core topic definition
char device_path[200];
mqtt_serv_state_t mqtt_service_state = 0; 	// state holder.

char *jwt;

esp_err_t mqtt_app_start( esp_mqtt_client_handle_t *mqtt_client, esp_mqtt_client_config_t *mqtt_ext_cfg ) {
	esp_err_t ret = ESP_OK;
	esp_mqtt_client_config_t mqtt_cfg;

	memcpy( &mqtt_cfg, mqtt_ext_cfg, sizeof(esp_mqtt_client_config_t) ); 	// Copy external configuration.

	if ( mqtt_ext_cfg->uri == NULL ) {	ESP_LOGE(TAG,"error: missing URI into external esp_mqtt_client_config_t.");
	return ESP_FAIL;
	}

	jwt = (char*) malloc(5000);
	if ( strcmp(mqtt_ext_cfg->uri, GCPIOT_BROKER_URI) == 0 ) { 	// If google URI detected use GIOTC.

		//******************************************************************* JWT ****************************************************//
		if ( xgiotc_gen_JWT(jwt, 5000, 3600) < 0 ) {
			free(jwt);
			return ESP_FAIL;
		}
		if (jwt != NULL) {
			ESP_LOGV(TAG,"JWT: %s", jwt);
		}
		else {
			ESP_LOGE(TAG,"error: jwt null");
			free(jwt);
			return ESP_FAIL;
		}


		//xgiotc_print_cfg(NULL);
		mqtt_cfg.uri = GCPIOT_BROKER_URI;
		mqtt_cfg.client_id = GCPIOT_CLIENT_ID;
		mqtt_cfg.cert_pem = (const char *)root_pem_start;
		mqtt_cfg.username = "device";
		mqtt_cfg.password = (const char *)jwt;


		//		mqtt_cfg.keepalive = 60;
		//		memcpy( &mqtt_cfg, &mqtt_giotc_cfg, sizeof(esp_mqtt_client_config_t) );
	}
	else {
		ESP_LOGW(TAG,"Don't use GIOTC.");
	}


	printf("JWT TOCKEN %s\n\r",jwt);


//	if (mqtt_cfg.event_handle == NULL ) {
//		mqtt_cfg.event_handle = mqtt_event_handler;
//	}


	*mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

	if ( *mqtt_client == NULL ) {
		ESP_LOGE(TAG,"error: mqtt client NULL.");
		free(jwt);
		return -1;
	}


	//esp_mqtt_client_register_event(*mqtt_client, ESP_EVENT_ANY_ID,mqtt_event_handler, NULL);

	ret = esp_mqtt_client_start(*mqtt_client);
	if (jwt != NULL) {
		free(jwt);
	}


	return ret;
}

mqtt_serv_state_t get_mqtt_service_state( void ) {
	return mqtt_service_state;
}

void set_mqtt_service_state( mqtt_serv_state_t s ) {
	// TODO: protect with semaphore
	mqtt_service_state = s;
}


