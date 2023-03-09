/**
 * SSAS - Simple Smart Automotive Software
 * Copyright (C) 2021 Parai Wang <parai@foxmail.com>
 */
#ifndef ISOTP_H
#define ISOTP_H
/* ================================ [ INCLUDES  ] ============================================== */
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* ================================ [ MACROS    ] ============================================== */
#define ISOTP_OTCTL_SET_TX_ID  0

/* ================================ [ TYPES     ] ============================================== */
typedef enum
{
  ISOTP_OVER_CAN,
  ISOTP_OVER_LIN
} isotp_protocol_t;

typedef struct {
  uint32_t RxCanId;
  uint32_t TxCanId;
  uint8_t BlockSize;
  uint8_t STmin; /* ms */
} isotp_can_param_t;

typedef struct {
  uint32_t timeout; /* ms */
  uint8_t RxId;
  uint8_t TxId;
} isotp_lin_param_t;

typedef struct {
  char device[64];
  int port;
  uint32_t baudrate;
  isotp_protocol_t protocol;
  uint32_t ll_dl;
  union {
    isotp_can_param_t CAN;
    isotp_lin_param_t LIN;
  } U;
} isotp_parameter_t;

typedef struct isotp_s isotp_t;
/* ================================ [ DECLARES  ] ============================================== */
/* ================================ [ DATAS     ] ============================================== */
/* ================================ [ LOCALS    ] ============================================== */
/* ================================ [ FUNCTIONS ] ============================================== */
isotp_t *isotp_create(isotp_parameter_t *params);
int isotp_transmit(isotp_t *isotp, const uint8_t *txBuffer, size_t txSize, uint8_t *rxBuffer,
                   size_t rxSize);
int isotp_receive(isotp_t *isotp, uint8_t *rxBuffer, size_t rxSize);
int isotp_ioctl(isotp_t *isotp, int cmd, const void *data, size_t size);
void isotp_destory(isotp_t *isotp);
#ifdef __cplusplus
}
#endif
#endif /* ISOTP_H */