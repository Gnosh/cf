/* FILE: cfdp_stubs.h -- Stub declarations for CFDP engine functions
 *
 * PURPOSE: Provides configurable stub implementations of the CFDP engine
 *   functions used by the CF application, for unit testing.
 *
 * DESIGN PATTERN:
 *   - A global struct (CFDP_Stubs) holds configurable return values and
 *     call counts for each stubbed function.
 *   - CFDP_Stubs_Reset() zeroes the entire struct to restore defaults.
 *   - Tests can set return values before calling CF code, and verify
 *     call counts afterward.
 */

#ifndef CFDP_STUBS_H
#define CFDP_STUBS_H

#include "cfdp_provides.h"
#include "cfdp_requires.h"

/*
 * Configurable return values and call counts for every stubbed function.
 */
typedef struct
{
    /* ----- Return values (set by test before exercising CF code) ----- */

    /* cfdp_give_pdu */
    boolean  give_pdu_return;

    /* cfdp_give_request */
    boolean  give_request_return;

    /* cfdp_set_mib_parameter */
    boolean  set_mib_parameter_return;

    /* cfdp_get_mib_parameter */
    boolean  get_mib_parameter_return;
    char     get_mib_parameter_value[MAX_MIB_VALUE_LENGTH + 1];

    /* cfdp_summary_status */
    SUMMARY_STATUS  summary_status_return;

    /* cfdp_transaction_status */
    boolean       transaction_status_return;
    TRANS_STATUS  transaction_status_value;

    /* cfdp_id_as_string  (uses static buffer internally) */
    char  id_as_string_return[MAX_AS_STRING_LENGTH + 1];

    /* cfdp_trans_as_string  (uses static buffer internally) */
    char  trans_as_string_return[MAX_AS_STRING_LENGTH + 1];

    /* cfdp_id_from_string */
    boolean  id_from_string_return;
    ID       id_from_string_value;

    /* cfdp_trans_from_string */
    boolean      trans_from_string_return;
    TRANSACTION  trans_from_string_value;

    /* cfdp_condition_as_string  (uses static buffer internally) */
    char  condition_as_string_return[MAX_AS_STRING_LENGTH + 1];

    /* cfdp_role_as_string  (uses static buffer internally) */
    char  role_as_string_return[MAX_AS_STRING_LENGTH + 1];

    /* cfdp_are_these_trans_equal */
    boolean  are_these_trans_equal_return;

    /* ----- Call counts (incremented by each stub) ----- */

    unsigned int  give_pdu_call_count;
    unsigned int  give_request_call_count;
    unsigned int  cycle_each_transaction_call_count;
    unsigned int  set_mib_parameter_call_count;
    unsigned int  get_mib_parameter_call_count;
    unsigned int  summary_status_call_count;
    unsigned int  transaction_status_call_count;
    unsigned int  id_as_string_call_count;
    unsigned int  trans_as_string_call_count;
    unsigned int  id_from_string_call_count;
    unsigned int  trans_from_string_call_count;
    unsigned int  condition_as_string_call_count;
    unsigned int  role_as_string_call_count;
    unsigned int  are_these_trans_equal_call_count;

    unsigned int  register_indication_call_count;
    unsigned int  register_pdu_output_open_call_count;
    unsigned int  register_pdu_output_ready_call_count;
    unsigned int  register_pdu_output_send_call_count;

    unsigned int  register_fopen_call_count;
    unsigned int  register_fseek_call_count;
    unsigned int  register_fread_call_count;
    unsigned int  register_fwrite_call_count;
    unsigned int  register_feof_call_count;
    unsigned int  register_fclose_call_count;
    unsigned int  register_rename_call_count;
    unsigned int  register_remove_call_count;
    unsigned int  register_file_size_call_count;

    unsigned int  register_printf_debug_call_count;
    unsigned int  register_printf_info_call_count;
    unsigned int  register_printf_warning_call_count;
    unsigned int  register_printf_error_call_count;

    unsigned int  reset_totals_call_count;
    unsigned int  set_trans_seq_num_call_count;
    unsigned int  misc_set_trans_seq_num_call_count;

    /* ----- Captured arguments (last call) ----- */

    u_int_4  set_trans_seq_num_value;
    u_int_4  misc_set_trans_seq_num_value;

} CFDP_Stubs_t;

/*
 * Global instance -- declared in cfdp_stubs.c.
 */
extern CFDP_Stubs_t CFDP_Stubs;

/*
 * Reset the entire stub structure to default values (zeroes everything,
 * then sets sensible defaults for string-returning stubs).
 */
void CFDP_Stubs_Reset(void);

#endif /* CFDP_STUBS_H */
