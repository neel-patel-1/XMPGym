#include "decrypt.h"
#include "print_utils.h"
#include <cstdlib>
#include "payload_gen.h"

IppsAES_GCMState *pState = NULL;


void gen_encrypted_feature(int payload_size, void **p_msgbuf, int *outsize){
  int ippAES_GCM_ctx_size;
  IppStatus status;

  status = ippsAES_GCMGetSize(&ippAES_GCM_ctx_size);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to get AES GCM size\n");
  }

  if(pState != NULL){
    free(pState);
  }
  pState = (IppsAES_GCMState *)malloc(ippAES_GCM_ctx_size);
  int keysize = 16;
  int ivsize = 12;
  int aadSize = 16;
  int taglen = 16;
  Ipp8u *pKey = (Ipp8u *)"0123456789abcdef";
  Ipp8u *pIV = (Ipp8u *)"0123456789ab";
  Ipp8u *pAAD = (Ipp8u *)malloc(aadSize);
  Ipp8u *pSrc = (Ipp8u *)gen_compressible_buf("01234567", payload_size);
  Ipp8u *pDst = (Ipp8u *)malloc(payload_size);
  Ipp8u *pTag = (Ipp8u *)malloc(taglen);

  LOG_PRINT(LOG_VERBOSE, "Plaintext: %s\n", pSrc);

  status = ippsAES_GCMInit(pKey, keysize, pState, ippAES_GCM_ctx_size);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to init AES GCM\n");
  }

  status = ippsAES_GCMStart(pIV, ivsize, pAAD, aadSize, pState);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to start AES GCM\n");
  }

  status = ippsAES_GCMEncrypt(pSrc, pDst, payload_size, pState);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to encrypt AES GCM\n");
  }

  status = ippsAES_GCMGetTag(pTag, taglen, pState);
  if(status != ippStsNoErr){
    LOG_PRINT(LOG_ERR, "Failed to get tag AES GCM\n");
  }

  LOG_PRINT(LOG_VERBOSE, "Ciphertext: %s\n", pDst);

  *p_msgbuf = (void *)pDst;
  *outsize = payload_size;

}
