
/* --------------------- INCLUDES ------------------------ *
 * ------------------------------------------------------- */
// Standard
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
// ESP-IDF
#include "esp_tls.h"
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
// Custom
#include "base64url.h"
#include "gcpjwt.h"
#include "esp_log.h"

#include "utility.h"

/* --------------------- DEFINES ------------------------- *
 * ------------------------------------------------------- */




/* --------------------- VARIABLES ----------------------- *
 * ------------------------------------------------------- */
static const char* TAG = "XGCPJWT";

long expire_t = 0; // TODO: put into GIOTC structure.

uint8_t *oBuf;

// Device topics.
//char gcpiot_dev_id[30];
//char sub_topic_cmd[100];
//char pub_topic_event[100];
//char pub_topic_state[100];
//char dev_path[200];

//giotc_cfg_t giotc_cfg = {}; // Private structure.



/* --------------------- FUNCTIONS ----------------------- *
 * ------------------------------------------------------- */
/* Setup device ID and topics with MAC address. */
//void setup_gcpiot_struct(void) {
////	char tmpstr[500];
////	sprintf(tmpstr,"/devices/%s/commands/#",giotc_cfg.dev_id);
////	strcpy(sub_topic_cmd,tmpstr);
////	sprintf(tmpstr, GCPIOT_TOPIC_BASE, giotc_cfg.dev_id);
////	strcpy(pub_topic_event,tmpstr);
////	sprintf(tmpstr,"/devices/%s/state",giotc_cfg.dev_id);
////	strcpy(pub_topic_state,tmpstr);
//}
//
//
//
//char *get_gcpiot_sub_topic_cmd(void) {
//	return sub_topic_cmd;
//}
//
//char *get_gcpiot_pub_topic_event(void) {
//	return pub_topic_event;
//}
//
//char *get_gcpiot_pub_topic_state(void) {
//	return pub_topic_state;
//}

/* Return a string representation of an mbedtls error code */
static char* mbedtlsError(int errnum) {
    static char buffer[200];
    mbedtls_strerror(errnum, buffer, sizeof(buffer));
    return buffer;
}


//char* createGCPJWT(const char* projectId, uint8_t* privateKey, uint32_t exp_time_s, size_t privateKeySize) {
//
//    char base64Header[100];
//    const char header[] = "{\"alg\":\"RS256\",\"typ\":\"JWT\"}";
//    ESP_LOGD(TAG,"JWT header: %s", header);
//    base64url_encode( (unsigned char *)header, strlen(header), base64Header);
//
////    char tmp_macstr[20];
////    get_mac_str(tmp_macstr);
////    setup_gcpiot_struct(tmp_macstr);
//
//    // Save expire connection time stamp.
//    expire_t = get_curtimestamp() + exp_time_s;
//
//    time_t now;
//    time(&now);
//    uint32_t iat = now;             	// Set the time now.
//    uint32_t exp = iat + exp_time_s;	// Set the expire time.
////    uint32_t exp = iat + 60*60;    	// Set the expire time.
//
//    char payload[100];
//    sprintf(payload, "{\"iat\":%d,\"exp\":%d,\"aud\":\"%s\"}", iat, exp, projectId);
//    ESP_LOGD(TAG,"JWT payload token: %s", payload);
//
//    char base64Payload[100];
//    base64url_encode( (unsigned char *)payload,  strlen(payload), base64Payload);            // Base64 encoded data.
//
//    uint8_t headerAndPayload[800];
//    sprintf((char*)headerAndPayload, "%s.%s", base64Header, base64Payload);
//
//    // At this point we have created the header and payload parts, converted both to base64 and concatenated them
//    // together as a single string.  Now we need to sign them using RSASSA
//
//    mbedtls_pk_context pk_context;
//    mbedtls_pk_init(&pk_context);
//    int rc = mbedtls_pk_parse_key(&pk_context, privateKey, privateKeySize, NULL, 0);
//    if (rc != 0) {
//        ESP_LOGE(TAG,"Failed to mbedtls_pk_parse_key: %d (-0x%x): %s\n", rc, -rc, mbedtlsError(rc));
//        return NULL;
//    }
//
//    uint8_t oBuf[5000];
//
//    uint8_t digest[32];
//    rc = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), headerAndPayload, strlen((char*)headerAndPayload), digest);
//    if (rc != 0) {
//    	ESP_LOGE(TAG,"Failed to mbedtls_md: %d (-0x%x): %s\n", rc, -rc, mbedtlsError(rc));
//        return NULL;
//    }
//
//    size_t retSize;
////    rc = mbedtls_pk_sign(&pk_context, MBEDTLS_MD_SHA256, digest, sizeof(digest), oBuf, &retSize, mbedtls_ctr_drbg_random, &ctr_drbg);
//    rc = mbedtls_pk_sign(&pk_context, MBEDTLS_MD_SHA256, digest, sizeof(digest), oBuf, &retSize, NULL, NULL);
//    if (rc != 0) {
//        ESP_LOGE(TAG,"Failed to mbedtls_pk_sign: %d (-0x%x): %s\n", rc, -rc, mbedtlsError(rc));
//        return NULL;
//    }
//
//
//    char base64Signature[600];
//    base64url_encode((unsigned char *)oBuf, retSize, base64Signature);
//
//    char* retData = (char*)malloc(strlen((char*)headerAndPayload) + 1 + strlen((char*)base64Signature) + 1);
//
//    sprintf(retData, "%s.%s", headerAndPayload, base64Signature);
//
//    mbedtls_pk_free(&pk_context);
//    return retData;
//}

long get_expire_t ( void ) {
	return expire_t;
}

int xgiotc_gen_JWT(char *jwtstr, uint32_t len, uint32_t exp_time_s) {


	const uint8_t *privateKey = private_key_pem_start;
	size_t privateKeySize = (private_key_pem_end - private_key_pem_start);


	//printf("PRIVATE KEY %s [%d]",privateKey,privateKeySize);

    char base64Header[100];
    const char header[] = "{\"alg\":\"RS256\",\"typ\":\"JWT\"}";
    ESP_LOGD(TAG,"JWT header: %s", header);
    base64url_encode( (unsigned char *)header, strlen(header), base64Header);

    // Save expire connection time stamp.
    expire_t = get_curtimestamp() + exp_time_s;

    time_t now;
    time(&now);
    uint32_t iat = now;             	// Set the time now.
    uint32_t exp = iat + exp_time_s;	// Set the expire time.

    char payload[100];
    ESP_LOGI(TAG,"{\"iat\":%d,\"exp\":%d,\"aud\":\"%s\"}", iat, exp, GCPIOT_PROJECT_ID);
    sprintf(payload, "{\"iat\":%d,\"exp\":%d,\"aud\":\"%s\"}", iat, exp, GCPIOT_PROJECT_ID);
    ESP_LOGD(TAG,"JWT payload token: %s", payload);

    char base64Payload[100];
    base64url_encode( (unsigned char *)payload,  strlen(payload), base64Payload);        // Base64 encoded data.

    uint8_t headerAndPayload[800];
    sprintf((char*)headerAndPayload, "%s.%s", base64Header, base64Payload);

    // At this point we have created the header and payload parts, converted both to base64 and concatenated them
    // together as a single string.  Now we need to sign them using RSASSA
    mbedtls_pk_context pk_context;
    mbedtls_pk_init(&pk_context);
    int rc = mbedtls_pk_parse_key(&pk_context, privateKey, privateKeySize, NULL, 0);
    if (rc != 0) {
        ESP_LOGE(TAG,"Failed to mbedtls_pk_parse_key: %d (-0x%x): %s\n", rc, -rc, mbedtlsError(rc));
        return -1;
    }

//    uint8_t oBuf[5000];
    oBuf = (uint8_t*) malloc(8000);
    if (oBuf == NULL) {
    	ESP_LOGE(TAG,"error: oBuf null");
    	free(oBuf);
    	return -1;
    }

    uint8_t digest[32];
    rc = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), headerAndPayload, strlen((char*)headerAndPayload), digest);
    if (rc != 0) {
    	ESP_LOGE(TAG,"Failed to mbedtls_md: %d (-0x%x): %s\n", rc, -rc, mbedtlsError(rc));
    	free(oBuf);
        return -1;
    }

    size_t retSize;
    rc = mbedtls_pk_sign(&pk_context, MBEDTLS_MD_SHA256, digest, sizeof(digest), oBuf, &retSize, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG,"Failed to mbedtls_pk_sign: %d (-0x%x): %s\n", rc, -rc, mbedtlsError(rc));
    	free(oBuf);
        return -1;
    }


    char base64Signature[600];
    base64url_encode((unsigned char *)oBuf, retSize, base64Signature);

    int rqlen = 0;
    if ( (rqlen = (strlen((char*)headerAndPayload) + 1 + strlen((char*)base64Signature) + 1)) >= len ) {
    	ESP_LOGE(TAG,"error: too short external JWT buffer, needed: %d, given: %d.", rqlen, len);
    	free(oBuf);
        return -1;
    }
    sprintf(jwtstr, "%s.%s", headerAndPayload, base64Signature);

    mbedtls_pk_free(&pk_context);

	free(oBuf);
    return 0;
}


