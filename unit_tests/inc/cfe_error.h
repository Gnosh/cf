/*
 * Standalone cFE error codes for CF unit tests.
 */
#ifndef _cfe_error_h_
#define _cfe_error_h_

#define CFE_SUCCESS               0x00000000
#define CFE_STATUS_NO_COUNTER_INCREMENT  0x48000001
#define CFE_STATUS_WRONG_MSG_LENGTH      0x48000002
#define CFE_SB_BAD_ARGUMENT       0xCA000003
#define CFE_SB_PIPE_CR_ERR        0xCA000004
#define CFE_SB_MAX_MSGS_MET       0xCA000005
#define CFE_ES_ERR_MEM_HANDLE     0xC4000006
#define CFE_ES_ERR_MEM_BLOCK_SIZE 0xC4000007

#endif /* _cfe_error_h_ */
