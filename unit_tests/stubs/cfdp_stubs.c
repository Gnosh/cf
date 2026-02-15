/* FILE: cfdp_stubs.c -- Stub implementations of CFDP engine functions
 *
 * PURPOSE: Provides configurable stub implementations of every CFDP engine
 *   function that the CF application calls (cf_app.c, cf_cmds.c,
 *   cf_callbacks.c, cf_utils.c, cf_playback.c).
 *
 * DESIGN PATTERN:
 *   - A single global struct (CFDP_Stubs) holds configurable return values
 *     and per-function call counts.
 *   - CFDP_Stubs_Reset() zeroes the struct and then sets sensible defaults
 *     for string-returning stubs.
 *   - Each stub increments its call count, checks whether the test has
 *     configured a custom return value, and returns it (or a safe default).
 */

#include <stdio.h>
#include <string.h>
#include "cfdp_stubs.h"
#include "cf_platform_cfg.h"

/* -------------------------------------------------------------------------- */
/*  Global stub control structure                                             */
/* -------------------------------------------------------------------------- */

CFDP_Stubs_t CFDP_Stubs;

/* -------------------------------------------------------------------------- */
/*  Reset helper                                                              */
/* -------------------------------------------------------------------------- */

void CFDP_Stubs_Reset(void)
{
    memset(&CFDP_Stubs, 0, sizeof(CFDP_Stubs));

    /* Sensible defaults for string-returning stubs */
    strncpy(CFDP_Stubs.id_as_string_return,        "0.0",   MAX_AS_STRING_LENGTH);
    strncpy(CFDP_Stubs.trans_as_string_return,      "0.0_0", MAX_AS_STRING_LENGTH);
    strncpy(CFDP_Stubs.condition_as_string_return,  "NO_ERROR", MAX_AS_STRING_LENGTH);
    strncpy(CFDP_Stubs.role_as_string_return,       "UNDEFINED", MAX_AS_STRING_LENGTH);

    /* Default boolean returns -- 1 (success) is a safe default for most */
    CFDP_Stubs.give_pdu_return             = 1;
    CFDP_Stubs.give_request_return         = 1;
    CFDP_Stubs.set_mib_parameter_return    = 1;
    CFDP_Stubs.get_mib_parameter_return    = 1;
    CFDP_Stubs.transaction_status_return   = 1;
    CFDP_Stubs.id_from_string_return       = 1;
    CFDP_Stubs.trans_from_string_return    = 1;
    CFDP_Stubs.are_these_trans_equal_return = 0;  /* default: not equal */
}

/* ========================================================================== */
/*  Core routines                                                             */
/* ========================================================================== */

boolean cfdp_give_pdu(CFDP_DATA pdu)
{
    CFDP_Stubs.give_pdu_call_count++;
    return CFDP_Stubs.give_pdu_return;
}

boolean cfdp_give_request(const char *request_as_string)
{
    CFDP_Stubs.give_request_call_count++;
    return CFDP_Stubs.give_request_return;
}

void cfdp_cycle_each_transaction(void)
{
    CFDP_Stubs.cycle_each_transaction_call_count++;
}

/* ========================================================================== */
/*  MIB access                                                                */
/* ========================================================================== */

boolean cfdp_set_mib_parameter(const char *param, const char *value)
{
    CFDP_Stubs.set_mib_parameter_call_count++;
    return CFDP_Stubs.set_mib_parameter_return;
}

boolean cfdp_get_mib_parameter(const char *param, char *value)
{
    CFDP_Stubs.get_mib_parameter_call_count++;

    if (value != NULL)
    {
        /* CF_GetMibCmd declares Value[CF_MAX_CFG_VALUE_CHARS] (32 bytes).
         * Use that as our copy limit to avoid overflowing the caller's buffer.
         * NOTE: The real CFDP engine uses MAX_MIB_VALUE_LENGTH (64), which is
         * a buffer overflow bug in CF_GetMibCmd's local variable. */
        strncpy(value, CFDP_Stubs.get_mib_parameter_value, CF_MAX_CFG_VALUE_CHARS);
        value[CF_MAX_CFG_VALUE_CHARS - 1] = '\0';
    }

    return CFDP_Stubs.get_mib_parameter_return;
}

/* ========================================================================== */
/*  Engine / transaction status                                               */
/* ========================================================================== */

SUMMARY_STATUS cfdp_summary_status(void)
{
    CFDP_Stubs.summary_status_call_count++;
    return CFDP_Stubs.summary_status_return;
}

boolean cfdp_transaction_status(TRANSACTION transaction,
                                TRANS_STATUS *trans_status)
{
    CFDP_Stubs.transaction_status_call_count++;

    if (trans_status != NULL)
    {
        *trans_status = CFDP_Stubs.transaction_status_value;
    }

    return CFDP_Stubs.transaction_status_return;
}

/* ========================================================================== */
/*  Conversion: structure  -->  string                                        */
/* ========================================================================== */

char *cfdp_id_as_string(ID id)
{
    static char buffer[MAX_AS_STRING_LENGTH + 1];

    CFDP_Stubs.id_as_string_call_count++;
    strncpy(buffer, CFDP_Stubs.id_as_string_return, MAX_AS_STRING_LENGTH);
    buffer[MAX_AS_STRING_LENGTH] = '\0';
    return buffer;
}

char *cfdp_trans_as_string(TRANSACTION transaction)
{
    static char buffer[MAX_AS_STRING_LENGTH + 1];

    CFDP_Stubs.trans_as_string_call_count++;
    strncpy(buffer, CFDP_Stubs.trans_as_string_return, MAX_AS_STRING_LENGTH);
    buffer[MAX_AS_STRING_LENGTH] = '\0';
    return buffer;
}

char *cfdp_condition_as_string(CONDITION_CODE cc)
{
    static char buffer[MAX_AS_STRING_LENGTH + 1];

    CFDP_Stubs.condition_as_string_call_count++;
    strncpy(buffer, CFDP_Stubs.condition_as_string_return, MAX_AS_STRING_LENGTH);
    buffer[MAX_AS_STRING_LENGTH] = '\0';
    return buffer;
}

char *cfdp_role_as_string(ROLE role)
{
    static char buffer[MAX_AS_STRING_LENGTH + 1];

    CFDP_Stubs.role_as_string_call_count++;
    strncpy(buffer, CFDP_Stubs.role_as_string_return, MAX_AS_STRING_LENGTH);
    buffer[MAX_AS_STRING_LENGTH] = '\0';
    return buffer;
}

/* ========================================================================== */
/*  Conversion: string  -->  structure                                        */
/* ========================================================================== */

boolean cfdp_id_from_string(const char *value_as_dotted_string, ID *id)
{
    CFDP_Stubs.id_from_string_call_count++;

    if (id != NULL)
    {
        *id = CFDP_Stubs.id_from_string_value;
    }

    return CFDP_Stubs.id_from_string_return;
}

boolean cfdp_trans_from_string(const char *string, TRANSACTION *trans)
{
    CFDP_Stubs.trans_from_string_call_count++;

    if (trans != NULL)
    {
        *trans = CFDP_Stubs.trans_from_string_value;
    }

    return CFDP_Stubs.trans_from_string_return;
}

/* ========================================================================== */
/*  Comparison                                                                */
/* ========================================================================== */

boolean cfdp_are_these_trans_equal(TRANSACTION t1, TRANSACTION t2)
{
    CFDP_Stubs.are_these_trans_equal_call_count++;
    return CFDP_Stubs.are_these_trans_equal_return;
}

/* ========================================================================== */
/*  Callback registration (PDU output)                                        */
/* ========================================================================== */

void register_indication(void (*function)(INDICATION_TYPE, TRANS_STATUS))
{
    CFDP_Stubs.register_indication_call_count++;
}

void register_pdu_output_open(boolean (*function)(ID my_id, ID partner_id))
{
    CFDP_Stubs.register_pdu_output_open_call_count++;
}

void register_pdu_output_ready(
    boolean (*function)(PDU_TYPE pdu_type, TRANSACTION trans_id, ID partner_id))
{
    CFDP_Stubs.register_pdu_output_ready_call_count++;
}

void register_pdu_output_send(
    void (*function)(TRANSACTION trans, ID partner_id, CFDP_DATA *pdu))
{
    CFDP_Stubs.register_pdu_output_send_call_count++;
}

/* ========================================================================== */
/*  Callback registration (Virtual Filestore)                                 */
/* ========================================================================== */

void register_fopen(CFDP_FILE *(*function)(const char *name,
                                            const char *mode))
{
    CFDP_Stubs.register_fopen_call_count++;
}

void register_fseek(int (*function)(CFDP_FILE *file, long int offset,
                                     int whence))
{
    CFDP_Stubs.register_fseek_call_count++;
}

void register_fread(size_t (*function)(void *buffer, size_t size,
                                        size_t count, CFDP_FILE *file))
{
    CFDP_Stubs.register_fread_call_count++;
}

void register_fwrite(size_t (*function)(const void *buff, size_t size,
                                         size_t count, CFDP_FILE *file))
{
    CFDP_Stubs.register_fwrite_call_count++;
}

void register_feof(int (*function)(CFDP_FILE *file))
{
    CFDP_Stubs.register_feof_call_count++;
}

void register_fclose(int (*function)(CFDP_FILE *file))
{
    CFDP_Stubs.register_fclose_call_count++;
}

void register_rename(int (*function)(const char *current, const char *new_name))
{
    CFDP_Stubs.register_rename_call_count++;
}

void register_remove(int (*function)(const char *name))
{
    CFDP_Stubs.register_remove_call_count++;
}

void register_file_size(u_int_4 (*function)(const char *file_name))
{
    CFDP_Stubs.register_file_size_call_count++;
}

/* ========================================================================== */
/*  Callback registration (printf levels)                                     */
/* ========================================================================== */

void register_printf_debug(int (*function)(const char *, ...))
{
    CFDP_Stubs.register_printf_debug_call_count++;
}

void register_printf_info(int (*function)(const char *, ...))
{
    CFDP_Stubs.register_printf_info_call_count++;
}

void register_printf_warning(int (*function)(const char *, ...))
{
    CFDP_Stubs.register_printf_warning_call_count++;
}

void register_printf_error(int (*function)(const char *, ...))
{
    CFDP_Stubs.register_printf_error_call_count++;
}

/* ========================================================================== */
/*  Miscellaneous                                                             */
/* ========================================================================== */

void cfdp_reset_totals(void)
{
    CFDP_Stubs.reset_totals_call_count++;
}

void cfdp_set_trans_seq_num(u_int_4 value)
{
    CFDP_Stubs.set_trans_seq_num_call_count++;
    CFDP_Stubs.set_trans_seq_num_value = value;
}

void misc__set_trans_seq_num(u_int_4 value)
{
    CFDP_Stubs.misc_set_trans_seq_num_call_count++;
    CFDP_Stubs.misc_set_trans_seq_num_value = value;
}
