/*
 * Minimal CH585M SPI0 slave key-state source for the H417 SPI bring-up.
 *
 * Wiring expected by the current H417 test firmware:
 *   CH585 PA12 / SPI0 CS   <- H417 PB12 / SPI2 NSS
 *   CH585 PA13 / SPI0 SCK  <- H417 PB13 / SPI2 SCK
 *   CH585 PA14 / SPI0 MOSI <- H417 PC1  / SPI2 MOSI
 *   CH585 PA15 / SPI0 MISO -> H417 PC2  / SPI2 MISO
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "CH58x_common.h"

#define CH585_SCAN_FRAME_MAGIC       0x4BD3U
#define CH585_SCAN_FRAME_VERSION     2U
#define CH585_SCAN_FRAME_TYPE_KEY_STATE 0x10U
#define CH585_SCAN_CMD_MAGIC         0x524BU
#define CH585_SCAN_CMD_GET_STATE     0x01U
#define CH585_SCAN_CMD_GET_DEBUG     0x02U
#define CH585_SCAN_CMD_GET_CONFIG    0x03U
#define CH585_SCAN_CMD_SET_CONFIG    0x04U
#define CH585_SCAN_CMD_CALIBRATE_KEY 0x05U
#define CH585_SCAN_CMD_CALIBRATE_ALL 0x06U
#define CH585_SCAN_SHORT_FRAME_MAGIC 0xD7U
#define CH585_SCAN_SHORT_FRAME_TYPE_KEY_STATE 0x11U
#define CH585_SCAN_SHORT_FRAME_TYPE_KEY_DEBUG 0x12U
#define CH585_SCAN_SHORT_CMD_MAGIC   0xA7U
#define CH585_SCAN_SOURCE_ID         0U
#define CH585_SCAN_KEYS_PER_SOURCE   64U
#define CH585_SCAN_DOWN_BYTES        (CH585_SCAN_KEYS_PER_SOURCE / 8U)
#define CH585_SCAN_FLAG_READY        (1U << 15)
#define CH585_SCAN_FLAG_CMD_ERROR    (1U << 4)
#define CH585_SCAN_SHORT_FLAG_READY  (1U << 7)
#define CH585_SCAN_SHORT_FLAG_CMD_ERROR (1U << 4)
#define CH585_SCAN_SHORT_DEBUG_FLAG_DOWN     (1U << 0)
#define CH585_SCAN_SHORT_DEBUG_FLAG_RT_ARMED (1U << 1)
#define CH585_SCAN_REQ0              'K'
#define CH585_SCAN_REQ1              'R'
#define CH585_SCAN_CFG_RELEASED_ADC       0x01U
#define CH585_SCAN_CFG_PRESSED_ADC        0x02U
#define CH585_SCAN_CFG_MIN_ADC            0x03U
#define CH585_SCAN_CFG_MAX_ADC            0x04U
#define CH585_SCAN_CFG_PRESS_POSITION     0x05U
#define CH585_SCAN_CFG_RELEASE_POSITION   0x06U
#define CH585_SCAN_CFG_RT_PRESS_DELTA     0x07U
#define CH585_SCAN_CFG_RT_RELEASE_DELTA   0x08U
#define CH585_SCAN_CFG_FILTER_SHIFT       0x09U
#define CH585_SCAN_CFG_RT_ENABLE          0x0AU
#define CH585_SCAN_CFG_VALID              0x0BU
#define CH585_SCAN_CFG_GLOBAL_KEY_ID      0x0CU
#define CH585_SIM_RELEASED_ADC       1000U
#define CH585_SIM_PRESSED_ADC        3000U
#define CH585_KEY_DEFAULT_MIN_ADC    0U
#define CH585_KEY_DEFAULT_MAX_ADC    4095U
#define CH585_KEY_DEFAULT_FILTER_SHIFT 2U
#define CH585_KEY_DEFAULT_PRESS_POSITION_PM 500U
#define CH585_KEY_DEFAULT_RELEASE_POSITION_PM 350U
#define CH585_KEY_DEFAULT_RT_PRESS_DELTA_PM 80U
#define CH585_KEY_DEFAULT_RT_RELEASE_DELTA_PM 80U
#define CH585_KEY_INVALID_GLOBAL_ID  0xFFFFU
#define CH585_SIM_ACTIVE_KEYS        4U
#define CH585_SIM_PERIOD_FRAMES      32U

#ifndef CH585_DEBUG_FRAME_INTERVAL
#define CH585_DEBUG_FRAME_INTERVAL   8U
#endif

#ifndef CH585_LINK_TEST_MISO_LOW
#define CH585_LINK_TEST_MISO_LOW     0
#endif

#ifndef CH585_LINK_TEST_SPI_PATTERN
#define CH585_LINK_TEST_SPI_PATTERN  0
#endif

#ifndef CH585_SPI0_MISO_STRONG_DRIVE
#define CH585_SPI0_MISO_STRONG_DRIVE 0
#endif

#ifndef CH585_FAST_SIM_FRAME
#define CH585_FAST_SIM_FRAME       0
#endif

#ifndef CH585_KEY_ENABLE_RAPID_TRIGGER
#define CH585_KEY_ENABLE_RAPID_TRIGGER 1
#endif

#ifndef CH585_STATIC_FRAME_TEST
#define CH585_STATIC_FRAME_TEST    0
#endif

#ifndef CH585_USE_SPI0_SLAVE_DMA
#define CH585_USE_SPI0_SLAVE_DMA     1
#endif

#ifndef CH585_USE_SHORT_FRAME
#define CH585_USE_SHORT_FRAME        1
#endif

#ifndef CH585_USE_PIPELINE_SHORT
#define CH585_USE_PIPELINE_SHORT     0
#endif

#ifndef CH585_USE_REQUEST_ONLY_SHORT
#define CH585_USE_REQUEST_ONLY_SHORT CH585_USE_SHORT_FRAME
#endif

#ifndef CH585_REQUEST_ONLY_CAPTURE_CMD
#define CH585_REQUEST_ONLY_CAPTURE_CMD 1
#endif

#define CH585_MODE_PIPELINE_SHORT \
    (CH585_USE_PIPELINE_SHORT && CH585_USE_SHORT_FRAME)
#define CH585_MODE_REQUEST_ONLY_SHORT \
    (CH585_USE_REQUEST_ONLY_SHORT && CH585_USE_SHORT_FRAME)
#define CH585_MODE_COMMAND_RESPONSE \
    (!CH585_MODE_PIPELINE_SHORT && !CH585_MODE_REQUEST_ONLY_SHORT)

typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint8_t source_id;
    uint8_t key_count;
    uint16_t seq;
    uint16_t flags;
    uint16_t ack_seq;
    uint8_t down_bits[CH585_SCAN_DOWN_BYTES];
    uint16_t diag;
    uint16_t crc16;
} ch585_scan_frame_v2_t;

typedef struct __attribute__((packed))
{
    uint16_t magic;
    uint8_t version;
    uint8_t cmd;
    uint16_t host_seq;
    uint16_t ack_seq;
    uint16_t flags;
    uint16_t crc16;
} ch585_scan_cmd_legacy_t;

typedef struct __attribute__((packed))
{
    uint8_t magic;
    uint8_t type;
    uint8_t source_id;
    uint8_t seq;
    uint8_t ack_seq;
    uint8_t flags;
    uint8_t down_bits[CH585_SCAN_DOWN_BYTES];
    uint16_t crc16;
} ch585_scan_frame_short_t;

typedef struct __attribute__((packed))
{
    uint8_t magic;
    uint8_t type;
    uint8_t source_id;
    uint8_t seq;
    uint8_t key_id;
    uint8_t flags;
    uint16_t raw_adc;
    uint16_t filtered_adc;
    uint16_t position_pm;
    uint16_t peak_pm;
    uint16_t crc16;
} ch585_scan_debug_short_t;

typedef struct __attribute__((packed))
{
    uint8_t magic;
    uint8_t cmd;
    uint8_t host_seq;
    uint8_t ack_seq;
    uint8_t target_key;
    uint8_t param_id;
    uint16_t value;
    uint16_t flags;
    uint16_t aux;
    uint8_t reserved[2];
    uint16_t crc16;
} ch585_scan_cmd_short_t;

#if CH585_USE_SHORT_FRAME
typedef ch585_scan_frame_short_t ch585_scan_wire_frame_t;
typedef ch585_scan_cmd_short_t ch585_scan_wire_cmd_t;
#else
typedef ch585_scan_frame_v2_t ch585_scan_wire_frame_t;
typedef ch585_scan_cmd_legacy_t ch585_scan_wire_cmd_t;
#endif

typedef struct
{
    uint16_t released_adc;
    uint16_t pressed_adc;
    uint16_t min_adc;
    uint16_t max_adc;
    uint16_t press_position_pm;
    uint16_t release_position_pm;
    uint16_t rt_press_delta_pm;
    uint16_t rt_release_delta_pm;
    uint16_t global_key_id;
    uint8_t filter_shift;
    uint8_t rt_enable;
    uint8_t valid;
} ch585_key_config_t;

#if CH585_MODE_PIPELINE_SHORT || CH585_MODE_COMMAND_RESPONSE
static __attribute__((aligned(4))) ch585_scan_wire_frame_t g_frame;
#endif
#if CH585_MODE_REQUEST_ONLY_SHORT
static __attribute__((aligned(4))) ch585_scan_wire_frame_t g_frame_pingpong[2];
#endif
#if CH585_MODE_PIPELINE_SHORT || CH585_MODE_REQUEST_ONLY_SHORT || CH585_MODE_COMMAND_RESPONSE
static __attribute__((aligned(4))) ch585_scan_wire_cmd_t g_cmd;
#endif
#if CH585_MODE_PIPELINE_SHORT
static __attribute__((aligned(4))) uint8_t g_pipe_rx[sizeof(ch585_scan_wire_frame_t)];
#endif
#if CH585_MODE_REQUEST_ONLY_SHORT && CH585_REQUEST_ONLY_CAPTURE_CMD && CH585_USE_SPI0_SLAVE_DMA
static __attribute__((aligned(4))) uint8_t g_req_rx[sizeof(ch585_scan_wire_frame_t)];
#endif
static ch585_key_config_t g_key_config[CH585_SCAN_KEYS_PER_SOURCE];
static uint8_t g_key_down[CH585_SCAN_KEYS_PER_SOURCE];
static uint8_t g_key_filter_valid[CH585_SCAN_KEYS_PER_SOURCE];
static uint8_t g_key_rt_armed[CH585_SCAN_KEYS_PER_SOURCE];
static uint16_t g_key_raw_adc[CH585_SCAN_KEYS_PER_SOURCE];
static uint16_t g_key_filtered_adc[CH585_SCAN_KEYS_PER_SOURCE];
static uint32_t g_key_filtered_q8[CH585_SCAN_KEYS_PER_SOURCE];
static uint16_t g_key_position_pm[CH585_SCAN_KEYS_PER_SOURCE];
static uint16_t g_key_peak_pm[CH585_SCAN_KEYS_PER_SOURCE];
static uint16_t g_key_valley_pm[CH585_SCAN_KEYS_PER_SOURCE];

static uint16_t scan_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;
    uint8_t bit;

    for (i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (bit = 0; bit < 8; bit++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}

static void key_config_init_defaults(void)
{
    uint8_t i;

    for (i = 0; i < CH585_SCAN_KEYS_PER_SOURCE; i++)
    {
        ch585_key_config_t *cfg = &g_key_config[i];

        cfg->released_adc = CH585_SIM_RELEASED_ADC;
        cfg->pressed_adc = CH585_SIM_PRESSED_ADC;
        cfg->min_adc = CH585_KEY_DEFAULT_MIN_ADC;
        cfg->max_adc = CH585_KEY_DEFAULT_MAX_ADC;
        cfg->press_position_pm = CH585_KEY_DEFAULT_PRESS_POSITION_PM;
        cfg->release_position_pm = CH585_KEY_DEFAULT_RELEASE_POSITION_PM;
        cfg->rt_press_delta_pm = CH585_KEY_DEFAULT_RT_PRESS_DELTA_PM;
        cfg->rt_release_delta_pm = CH585_KEY_DEFAULT_RT_RELEASE_DELTA_PM;
        cfg->global_key_id = (uint16_t)i;
        cfg->filter_shift = CH585_KEY_DEFAULT_FILTER_SHIFT;
#if CH585_KEY_ENABLE_RAPID_TRIGGER
        cfg->rt_enable = 1U;
#else
        cfg->rt_enable = 0U;
#endif
        cfg->valid = 1U;

        g_key_raw_adc[i] = cfg->released_adc;
        g_key_filtered_adc[i] = cfg->released_adc;
        g_key_filtered_q8[i] = (uint32_t)cfg->released_adc << 8;
        g_key_position_pm[i] = 0U;
        g_key_peak_pm[i] = 0U;
        g_key_valley_pm[i] = 0U;
    }
}

static const ch585_key_config_t *key_config(uint8_t key_id)
{
    if (key_id >= CH585_SCAN_KEYS_PER_SOURCE)
    {
        return &g_key_config[0];
    }

    return &g_key_config[key_id];
}

#if CH585_MODE_PIPELINE_SHORT || CH585_MODE_REQUEST_ONLY_SHORT || CH585_MODE_COMMAND_RESPONSE
static void key_runtime_reset(uint8_t key_id)
{
    const ch585_key_config_t *cfg = key_config(key_id);

    if (key_id >= CH585_SCAN_KEYS_PER_SOURCE)
    {
        return;
    }

    g_key_down[key_id] = 0U;
    g_key_filter_valid[key_id] = 0U;
    g_key_rt_armed[key_id] = 0U;
    g_key_raw_adc[key_id] = cfg->released_adc;
    g_key_filtered_adc[key_id] = cfg->released_adc;
    g_key_filtered_q8[key_id] = (uint32_t)cfg->released_adc << 8;
    g_key_position_pm[key_id] = 0U;
    g_key_peak_pm[key_id] = 0U;
    g_key_valley_pm[key_id] = 0U;
}

static uint8_t scan_cmd_id_is_supported(uint8_t cmd)
{
    switch (cmd)
    {
    case CH585_SCAN_CMD_GET_STATE:
    case CH585_SCAN_CMD_GET_DEBUG:
    case CH585_SCAN_CMD_GET_CONFIG:
    case CH585_SCAN_CMD_SET_CONFIG:
    case CH585_SCAN_CMD_CALIBRATE_KEY:
    case CH585_SCAN_CMD_CALIBRATE_ALL:
        return 1U;
    default:
        return 0U;
    }
}

static uint8_t key_config_set_u16(uint8_t key_id, uint8_t param_id, uint16_t value)
{
    ch585_key_config_t *cfg;

    if (key_id >= CH585_SCAN_KEYS_PER_SOURCE)
    {
        return 0U;
    }

    cfg = &g_key_config[key_id];

    switch (param_id)
    {
    case CH585_SCAN_CFG_RELEASED_ADC:
        cfg->released_adc = value;
        break;
    case CH585_SCAN_CFG_PRESSED_ADC:
        cfg->pressed_adc = value;
        break;
    case CH585_SCAN_CFG_MIN_ADC:
        cfg->min_adc = value;
        break;
    case CH585_SCAN_CFG_MAX_ADC:
        cfg->max_adc = value;
        break;
    case CH585_SCAN_CFG_PRESS_POSITION:
        cfg->press_position_pm = (value > 1000U) ? 1000U : value;
        break;
    case CH585_SCAN_CFG_RELEASE_POSITION:
        cfg->release_position_pm = (value > 1000U) ? 1000U : value;
        break;
    case CH585_SCAN_CFG_RT_PRESS_DELTA:
        cfg->rt_press_delta_pm = (value > 1000U) ? 1000U : value;
        break;
    case CH585_SCAN_CFG_RT_RELEASE_DELTA:
        cfg->rt_release_delta_pm = (value > 1000U) ? 1000U : value;
        break;
    case CH585_SCAN_CFG_FILTER_SHIFT:
        cfg->filter_shift = (value > 15U) ? 15U : (uint8_t)value;
        break;
    case CH585_SCAN_CFG_RT_ENABLE:
        cfg->rt_enable = (value != 0U) ? 1U : 0U;
        break;
    case CH585_SCAN_CFG_VALID:
        cfg->valid = (value != 0U) ? 1U : 0U;
        break;
    case CH585_SCAN_CFG_GLOBAL_KEY_ID:
        cfg->global_key_id = value;
        break;
    default:
        return 0U;
    }

    key_runtime_reset(key_id);
    return 1U;
}

static uint16_t scan_cmd_apply(const ch585_scan_wire_cmd_t *cmd)
{
    if (cmd == NULL)
    {
        return CH585_SCAN_FLAG_CMD_ERROR;
    }

    switch (cmd->cmd)
    {
    case CH585_SCAN_CMD_GET_STATE:
    case CH585_SCAN_CMD_GET_DEBUG:
    case CH585_SCAN_CMD_GET_CONFIG:
        return 0U;
    case CH585_SCAN_CMD_SET_CONFIG:
#if CH585_USE_SHORT_FRAME
        if (key_config_set_u16(cmd->target_key, cmd->param_id, cmd->value) == 0U)
        {
            return CH585_SCAN_FLAG_CMD_ERROR;
        }
        return 0U;
#else
        return CH585_SCAN_FLAG_CMD_ERROR;
#endif
    case CH585_SCAN_CMD_CALIBRATE_KEY:
#if CH585_USE_SHORT_FRAME
        if (cmd->target_key >= CH585_SCAN_KEYS_PER_SOURCE)
        {
            return CH585_SCAN_FLAG_CMD_ERROR;
        }
        key_runtime_reset(cmd->target_key);
        return 0U;
#else
        return CH585_SCAN_FLAG_CMD_ERROR;
#endif
    case CH585_SCAN_CMD_CALIBRATE_ALL:
        key_config_init_defaults();
        return 0U;
    default:
        return CH585_SCAN_FLAG_CMD_ERROR;
    }
}
#endif

static uint16_t sim_clamp_adc(int32_t value)
{
    if (value < 0)
    {
        return 0U;
    }

    if (value > 4095)
    {
        return 4095U;
    }

    return (uint16_t)value;
}

static uint16_t sim_key_position_pm(uint16_t seq, uint8_t key_id)
{
    uint16_t phase;

    if (key_id >= CH585_SIM_ACTIVE_KEYS)
    {
        return 0U;
    }

    phase = (uint16_t)((seq + ((uint16_t)key_id * 8U)) &
                       (CH585_SIM_PERIOD_FRAMES - 1U));

    if (phase < 8U)
    {
        return (uint16_t)((phase * 1000U) / 7U);
    }

    if (phase < 16U)
    {
        return 1000U;
    }

    if (phase < 24U)
    {
        return (uint16_t)(((23U - phase) * 1000U) / 7U);
    }

    return 0U;
}

static uint16_t sim_adc_value(uint16_t seq, uint8_t key_id)
{
    const ch585_key_config_t *cfg = key_config(key_id);
    uint16_t position_pm = sim_key_position_pm(seq, key_id);
    int32_t span = (int32_t)cfg->pressed_adc - (int32_t)cfg->released_adc;
    int16_t noise = (int16_t)(((seq * 17U) + ((uint16_t)key_id * 13U)) & 7U) - 3;
    int32_t value;

    if (cfg->valid == 0U)
    {
        return cfg->released_adc;
    }

    value = (int32_t)cfg->released_adc +
                    ((span * (int32_t)position_pm) / 1000) +
                    (int32_t)noise;

    return sim_clamp_adc(value);
}

static void key_state_set_bit(ch585_scan_wire_frame_t *frame, uint8_t key_id)
{
    frame->down_bits[key_id >> 3] |= (uint8_t)(1U << (key_id & 7U));
}

static uint16_t key_adc_to_position_pm(uint8_t key_id, uint16_t adc)
{
    const ch585_key_config_t *cfg = key_config(key_id);
    int32_t span = (int32_t)cfg->pressed_adc - (int32_t)cfg->released_adc;
    int32_t pos;

    if ((cfg->valid == 0U) || (span == 0))
    {
        return 0U;
    }

    pos = (((int32_t)adc - (int32_t)cfg->released_adc) * 1000) / span;
    if (pos < 0)
    {
        return 0U;
    }

    if (pos > 1000)
    {
        return 1000U;
    }

    return (uint16_t)pos;
}

static uint16_t key_filter_adc(uint8_t key_id, uint16_t adc)
{
    const ch585_key_config_t *cfg = key_config(key_id);
    uint32_t raw_q8 = (uint32_t)adc << 8;
    uint32_t filtered;
    uint8_t shift = cfg->filter_shift;

    if (shift > 15U)
    {
        shift = 15U;
    }

    if (g_key_filter_valid[key_id] == 0U)
    {
        g_key_filtered_q8[key_id] = raw_q8;
        g_key_filter_valid[key_id] = 1U;
    }
    else
    {
        filtered = g_key_filtered_q8[key_id];
        if (raw_q8 >= filtered)
        {
            filtered += (raw_q8 - filtered) >> shift;
        }
        else
        {
            filtered -= (filtered - raw_q8) >> shift;
        }
        g_key_filtered_q8[key_id] = filtered;
    }

    return (uint16_t)((g_key_filtered_q8[key_id] + 128U) >> 8);
}

static void key_press(uint8_t key_id, uint16_t position_pm)
{
    g_key_down[key_id] = 1U;
    g_key_rt_armed[key_id] = 0U;
    g_key_peak_pm[key_id] = position_pm;
    g_key_valley_pm[key_id] = position_pm;
}

static void key_release(uint8_t key_id, uint16_t position_pm, uint8_t rt_armed)
{
    g_key_down[key_id] = 0U;
    g_key_rt_armed[key_id] = rt_armed;
    g_key_peak_pm[key_id] = position_pm;
    g_key_valley_pm[key_id] = position_pm;
}

static void update_key_state_from_adc(uint8_t key_id, uint16_t adc)
{
    const ch585_key_config_t *cfg = key_config(key_id);
    uint16_t filtered_adc = key_filter_adc(key_id, adc);
    uint16_t position_pm = key_adc_to_position_pm(key_id, filtered_adc);

    if (cfg->valid == 0U)
    {
        g_key_down[key_id] = 0U;
        g_key_rt_armed[key_id] = 0U;
        g_key_raw_adc[key_id] = adc;
        g_key_filtered_adc[key_id] = filtered_adc;
        g_key_position_pm[key_id] = 0U;
        return;
    }

    g_key_raw_adc[key_id] = adc;
    g_key_filtered_adc[key_id] = filtered_adc;
    g_key_position_pm[key_id] = position_pm;

    if (g_key_down[key_id] == 0U)
    {
        if (position_pm < g_key_valley_pm[key_id])
        {
            g_key_valley_pm[key_id] = position_pm;
        }

#if CH585_KEY_ENABLE_RAPID_TRIGGER
        if ((cfg->rt_enable != 0U) &&
            (g_key_rt_armed[key_id] != 0U) &&
            (position_pm >=
             (uint16_t)(g_key_valley_pm[key_id] + cfg->rt_press_delta_pm)))
        {
            key_press(key_id, position_pm);
            return;
        }
#endif

        if (position_pm >= cfg->press_position_pm)
        {
            key_press(key_id, position_pm);
        }
    }
    else
    {
        if (position_pm > g_key_peak_pm[key_id])
        {
            g_key_peak_pm[key_id] = position_pm;
        }

#if CH585_KEY_ENABLE_RAPID_TRIGGER
        if ((cfg->rt_enable != 0U) &&
            ((position_pm + cfg->rt_release_delta_pm) <= g_key_peak_pm[key_id]))
        {
            key_release(key_id, position_pm, 1U);
            return;
        }
#endif

        if (position_pm <= cfg->release_position_pm)
        {
            key_release(key_id, position_pm, 0U);
        }
    }
}

static void update_all_key_states(uint16_t seq)
{
    uint8_t i;

    for (i = 0; i < CH585_SCAN_KEYS_PER_SOURCE; i++)
    {
        update_key_state_from_adc(i, sim_adc_value(seq, i));
    }
}

#if CH585_MODE_PIPELINE_SHORT || CH585_MODE_REQUEST_ONLY_SHORT || CH585_MODE_COMMAND_RESPONSE
static uint8_t scan_cmd_is_valid(const ch585_scan_wire_cmd_t *cmd)
{
    uint16_t expected_crc;

#if CH585_USE_SHORT_FRAME
    if (cmd->magic != CH585_SCAN_SHORT_CMD_MAGIC)
    {
        return 0U;
    }
    if (scan_cmd_id_is_supported(cmd->cmd) == 0U)
    {
        return 0U;
    }
#else
    if (cmd->magic != CH585_SCAN_CMD_MAGIC)
    {
        return 0U;
    }
    if (cmd->version != CH585_SCAN_FRAME_VERSION)
    {
        return 0U;
    }
    if (scan_cmd_id_is_supported(cmd->cmd) == 0U)
    {
        return 0U;
    }
#endif

    expected_crc = scan_crc16((const uint8_t *)cmd,
                              (uint16_t)offsetof(ch585_scan_wire_cmd_t, crc16));
    return (cmd->crc16 == expected_crc) ? 1U : 0U;
}
#endif

static void build_scan_frame_into(ch585_scan_wire_frame_t *frame,
                                  uint16_t seq,
                                  uint16_t flags,
                                  uint16_t ack_seq)
{
    uint8_t i;

    memset(frame, 0, sizeof(*frame));

#if CH585_USE_SHORT_FRAME
    (void)flags;
    frame->magic = CH585_SCAN_SHORT_FRAME_MAGIC;
    frame->type = CH585_SCAN_SHORT_FRAME_TYPE_KEY_STATE;
    frame->source_id = CH585_SCAN_SOURCE_ID;
    frame->seq = (uint8_t)seq;
    frame->flags = CH585_SCAN_SHORT_FLAG_READY;
    frame->ack_seq = (uint8_t)ack_seq;
#else
    frame->magic = CH585_SCAN_FRAME_MAGIC;
    frame->version = CH585_SCAN_FRAME_VERSION;
    frame->type = CH585_SCAN_FRAME_TYPE_KEY_STATE;
    frame->source_id = CH585_SCAN_SOURCE_ID;
    frame->key_count = CH585_SCAN_KEYS_PER_SOURCE;
    frame->seq = seq;
    frame->flags = flags;
    frame->ack_seq = ack_seq;
#endif

#if CH585_FAST_SIM_FRAME
    (void)i;
    frame->down_bits[0] = (uint8_t)(1U << ((seq >> 3U) & 7U));
    frame->down_bits[1] = (uint8_t)(1U << ((seq >> 4U) & 7U));
#else
    update_all_key_states(seq);
    for (i = 0; i < CH585_SCAN_KEYS_PER_SOURCE; i++)
    {
        if (g_key_down[i] != 0U)
        {
            key_state_set_bit(frame, i);
        }
    }
#endif

    frame->crc16 = scan_crc16((const uint8_t *)frame,
                              (uint16_t)offsetof(ch585_scan_wire_frame_t, crc16));
}

#if CH585_USE_SHORT_FRAME && CH585_MODE_REQUEST_ONLY_SHORT
static void build_debug_frame_into(ch585_scan_wire_frame_t *frame,
                                   uint16_t seq)
{
    ch585_scan_debug_short_t *debug = (ch585_scan_debug_short_t *)frame;
    uint8_t key_id;
    uint8_t debug_flags = CH585_SCAN_SHORT_FLAG_READY;

    memset(frame, 0, sizeof(*frame));
    update_all_key_states(seq);

#if CH585_DEBUG_FRAME_INTERVAL != 0
    key_id = (uint8_t)(((uint16_t)(seq / CH585_DEBUG_FRAME_INTERVAL)) %
                       CH585_SIM_ACTIVE_KEYS);
#else
    key_id = 0U;
#endif

    if (g_key_down[key_id] != 0U)
    {
        debug_flags |= CH585_SCAN_SHORT_DEBUG_FLAG_DOWN;
    }
    if (g_key_rt_armed[key_id] != 0U)
    {
        debug_flags |= CH585_SCAN_SHORT_DEBUG_FLAG_RT_ARMED;
    }

    debug->magic = CH585_SCAN_SHORT_FRAME_MAGIC;
    debug->type = CH585_SCAN_SHORT_FRAME_TYPE_KEY_DEBUG;
    debug->source_id = CH585_SCAN_SOURCE_ID;
    debug->seq = (uint8_t)seq;
    debug->key_id = key_id;
    debug->flags = debug_flags;
    debug->raw_adc = g_key_raw_adc[key_id];
    debug->filtered_adc = g_key_filtered_adc[key_id];
    debug->position_pm = g_key_position_pm[key_id];
    debug->peak_pm = g_key_peak_pm[key_id];
    debug->crc16 = scan_crc16((const uint8_t *)debug,
                              (uint16_t)offsetof(ch585_scan_debug_short_t, crc16));
}
#endif

#if CH585_USE_SHORT_FRAME
#if CH585_MODE_REQUEST_ONLY_SHORT
static void build_scan_or_debug_frame_into(ch585_scan_wire_frame_t *frame,
                                           uint16_t seq,
                                           uint16_t flags,
                                           uint16_t ack_seq)
{
#if CH585_DEBUG_FRAME_INTERVAL != 0
    if ((CH585_DEBUG_FRAME_INTERVAL != 0U) &&
        (seq != 0U) &&
        ((seq % CH585_DEBUG_FRAME_INTERVAL) == 0U))
    {
        (void)flags;
        (void)ack_seq;
        build_debug_frame_into(frame, seq);
        return;
    }
#endif

    build_scan_frame_into(frame, seq, flags, ack_seq);
}
#endif
#endif

#if CH585_MODE_PIPELINE_SHORT || CH585_MODE_COMMAND_RESPONSE
static void build_scan_frame(uint16_t seq, uint16_t flags, uint16_t ack_seq)
{
    build_scan_frame_into(&g_frame, seq, flags, ack_seq);
}
#endif

#if CH585_MODE_COMMAND_RESPONSE
static void finish_scan_frame(uint16_t flags, uint16_t ack_seq)
{
#if CH585_USE_SHORT_FRAME
    uint8_t short_flags = 0U;

    if ((flags & CH585_SCAN_FLAG_READY) != 0U)
    {
        short_flags |= CH585_SCAN_SHORT_FLAG_READY;
    }
    if ((flags & CH585_SCAN_FLAG_CMD_ERROR) != 0U)
    {
        short_flags |= CH585_SCAN_SHORT_FLAG_CMD_ERROR;
    }

    g_frame.flags = short_flags;
    g_frame.ack_seq = (uint8_t)ack_seq;
#else
    g_frame.flags = flags;
    g_frame.ack_seq = ack_seq;
#endif
    g_frame.crc16 = scan_crc16((const uint8_t *)&g_frame,
                               (uint16_t)offsetof(ch585_scan_wire_frame_t, crc16));
}
#endif

static void spi0_slave_stream_reset(void)
{
    R8_SPI0_CTRL_MOD = RB_SPI_ALL_CLEAR;
    R8_SPI0_CTRL_MOD = RB_SPI_MISO_OE | RB_SPI_MODE_SLAVE;
    R8_SPI0_CTRL_CFG |= RB_SPI_AUTO_IF;
    SPI0_DataMode(Mode0_HighBitINFront);
}

static void spi0_slave_pin_init(void)
{
#if CH585_SPI0_MISO_STRONG_DRIVE
    /*
     * One-CH585 high-speed experiment only: keep PA15/MISO on the strong
     * 20mA output driver. Do not use this unchanged on a shared MISO bus.
     */
    GPIOA_ModeCfg(GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_15, GPIO_ModeOut_PP_20mA);
#else
    GPIOA_ModeCfg(GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15, GPIO_ModeIN_PU);
#endif
}

#if CH585_USE_SPI0_SLAVE_DMA
static void spi0_slave_dma_trans_arm(uint8_t *pbuf, uint16_t len)
{
    R8_SPI0_CTRL_MOD &= ~RB_SPI_FIFO_DIR;
    R32_SPI0_DMA_BEG = (uint32_t)pbuf;
    R32_SPI0_DMA_END = (uint32_t)(pbuf + len);
    R16_SPI0_TOTAL_CNT = len;
    R8_SPI0_INT_FLAG = RB_SPI_IF_CNT_END | RB_SPI_IF_DMA_END |
                       RB_SPI_IF_BYTE_END | RB_SPI_IF_FIFO_OV;
    R8_SPI0_CTRL_CFG |= RB_SPI_DMA_ENABLE;
}

#if CH585_MODE_COMMAND_RESPONSE
static void spi0_slave_dma_recv_arm(uint8_t *pbuf, uint16_t len)
{
    R8_SPI0_CTRL_MOD |= RB_SPI_FIFO_DIR;
    R32_SPI0_DMA_BEG = (uint32_t)pbuf;
    R32_SPI0_DMA_END = (uint32_t)(pbuf + len);
    R16_SPI0_TOTAL_CNT = len;
    R8_SPI0_INT_FLAG = RB_SPI_IF_CNT_END | RB_SPI_IF_DMA_END;
    R8_SPI0_CTRL_CFG |= RB_SPI_DMA_ENABLE;
}
#endif

#if CH585_MODE_COMMAND_RESPONSE || (CH585_MODE_REQUEST_ONLY_SHORT && !CH585_REQUEST_ONLY_CAPTURE_CMD)
static void spi0_slave_dma_wait_done(void)
{
    while ((R8_SPI0_INT_FLAG & RB_SPI_IF_CNT_END) == 0U)
    {
    }
    R8_SPI0_CTRL_CFG &= ~RB_SPI_DMA_ENABLE;
}
#endif

#if CH585_MODE_REQUEST_ONLY_SHORT && CH585_REQUEST_ONLY_CAPTURE_CMD && CH585_USE_SPI0_SLAVE_DMA
static void spi0_slave_dma_wait_done_capture(uint8_t *rx, uint16_t len)
{
    uint16_t rxi = 0U;

    if (rx != NULL)
    {
        memset(rx, 0, len);
    }

    while ((R8_SPI0_INT_FLAG & RB_SPI_IF_CNT_END) == 0U)
    {
        if ((R8_SPI0_INT_FLAG & RB_SPI_IF_BYTE_END) != 0U)
        {
            if ((rx != NULL) && (rxi < len))
            {
                rx[rxi] = R8_SPI0_BUFFER;
                rxi++;
            }
            else
            {
                (void)R8_SPI0_BUFFER;
            }
            R8_SPI0_INT_FLAG = RB_SPI_IF_BYTE_END;
        }
    }

    while ((R8_SPI0_INT_FLAG & RB_SPI_IF_BYTE_END) != 0U)
    {
        if ((rx != NULL) && (rxi < len))
        {
            rx[rxi] = R8_SPI0_BUFFER;
            rxi++;
        }
        else
        {
            (void)R8_SPI0_BUFFER;
        }
        R8_SPI0_INT_FLAG = RB_SPI_IF_BYTE_END;
    }

    R8_SPI0_CTRL_CFG &= ~RB_SPI_DMA_ENABLE;
}
#endif
#endif

static void spi0_wait_cs_high(void)
{
    while ((R8_SPI0_RUN_FLAG & RB_SPI_SLV_SELECT) != 0U)
    {
    }
}

static void spi0_wait_cs_low(void)
{
    while ((R8_SPI0_RUN_FLAG & RB_SPI_SLV_SELECT) == 0U)
    {
    }
}

#if CH585_USE_PIPELINE_SHORT && CH585_USE_SHORT_FRAME
static void spi0_slave_pipeline_transrecv(uint8_t *tx, uint8_t *rx, uint16_t len)
{
    uint16_t txi = 0U;
    uint16_t rxi = 0U;

    memset(rx, 0, len);
    R8_SPI0_CTRL_MOD &= ~RB_SPI_FIFO_DIR;
    R16_SPI0_TOTAL_CNT = len;
    R8_SPI0_INT_FLAG = RB_SPI_IF_CNT_END | RB_SPI_IF_BYTE_END | RB_SPI_IF_FIFO_OV;

    SetFirstData(tx[0]);
    spi0_wait_cs_low();

    while ((rxi < len) || (txi < len))
    {
        while ((txi < len) && (R8_SPI0_FIFO_COUNT < SPI_FIFO_SIZE))
        {
            R8_SPI0_FIFO = tx[txi];
            txi++;
        }

        if ((R8_SPI0_INT_FLAG & RB_SPI_IF_BYTE_END) != 0U)
        {
            if (rxi < len)
            {
                rx[rxi] = R8_SPI0_BUFFER;
                rxi++;
            }
            R8_SPI0_INT_FLAG = RB_SPI_IF_BYTE_END;
        }

        if ((R8_SPI0_INT_FLAG & RB_SPI_IF_CNT_END) != 0U)
        {
            break;
        }
    }

    while ((R8_SPI0_INT_FLAG & RB_SPI_IF_CNT_END) == 0U)
    {
        if ((R8_SPI0_INT_FLAG & RB_SPI_IF_BYTE_END) != 0U)
        {
            if (rxi < len)
            {
                rx[rxi] = R8_SPI0_BUFFER;
                rxi++;
            }
            R8_SPI0_INT_FLAG = RB_SPI_IF_BYTE_END;
        }
    }

    R8_SPI0_INT_FLAG = RB_SPI_IF_CNT_END | RB_SPI_IF_BYTE_END | RB_SPI_IF_FIFO_OV;
}
#endif

int main(void)
{
#if CH585_LINK_TEST_MISO_LOW
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(SYSCLK_FREQ);

    /*
     * Physical-link test only: drive PA15/MISO low as a plain GPIO.
     * H417 should then read 0x00 on every sampled SPI byte if the wire,
     * power and common ground are correct.
     */
    GPIOA_ResetBits(GPIO_Pin_15);
    GPIOA_ModeCfg(GPIO_Pin_15, GPIO_ModeOut_PP_20mA);

    while (1)
    {
        GPIOA_ResetBits(GPIO_Pin_15);
    }
#elif CH585_LINK_TEST_SPI_PATTERN
    static uint8_t pattern[] = {
        0xA5U, 0x5AU, 0x3CU, 0xC3U,
        0x11U, 0x22U, 0x33U, 0x44U,
        0x55U, 0x66U, 0x77U, 0x88U,
        0x99U, 0xAAU, 0xBBU, 0xCCU,
    };

    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(SYSCLK_FREQ);

    spi0_slave_pin_init();
    SPI0_SlaveInit();
    SPI0_DataMode(Mode0_HighBitINFront);
    SetFirstData(pattern[0]);

    while (1)
    {
        SPI0_SlaveTrans(pattern, (uint16_t)sizeof(pattern));
    }
#else
    uint16_t seq = 0U;
    uint16_t ack_host_seq = 0xFFFFU;
    uint16_t frame_flags;

    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(SYSCLK_FREQ);
    key_config_init_defaults();

    spi0_slave_pin_init();
    SPI0_SlaveInit();
    SPI0_DataMode(Mode0_HighBitINFront);

#if CH585_USE_PIPELINE_SHORT && CH585_USE_SHORT_FRAME
    build_scan_frame(seq, CH585_SCAN_FLAG_READY, 0xFFFFU);
    while (1)
    {
        spi0_wait_cs_high();
        spi0_slave_stream_reset();
        spi0_slave_pipeline_transrecv((uint8_t *)&g_frame,
                                      g_pipe_rx,
                                      (uint16_t)sizeof(g_frame));
        spi0_wait_cs_high();

        memset(&g_cmd, 0, sizeof(g_cmd));
        memcpy(&g_cmd, g_pipe_rx, sizeof(g_cmd));

        if (scan_cmd_is_valid(&g_cmd) != 0U)
        {
            ack_host_seq = g_cmd.host_seq;
            seq++;
            frame_flags = (uint16_t)(CH585_SCAN_FLAG_READY | scan_cmd_apply(&g_cmd));
            build_scan_frame(seq, frame_flags, ack_host_seq);
        }
    }
#elif CH585_USE_REQUEST_ONLY_SHORT && CH585_USE_SHORT_FRAME
    uint8_t tx_index = 0U;
    uint16_t next_ack_seq = 0xFFFFU;
    uint16_t next_frame_flags = CH585_SCAN_FLAG_READY;

    (void)ack_host_seq;
    (void)frame_flags;
    build_scan_or_debug_frame_into(&g_frame_pingpong[0],
                                   seq,
                                   CH585_SCAN_FLAG_READY,
                                   0xFFFFU);
#if !CH585_STATIC_FRAME_TEST
    build_scan_or_debug_frame_into(&g_frame_pingpong[1],
                                   (uint16_t)(seq + 1U),
                                   next_frame_flags,
                                   next_ack_seq);
#endif

    while (1)
    {
        ch585_scan_wire_frame_t *tx_frame = &g_frame_pingpong[tx_index];
        ch585_scan_wire_frame_t *next_frame = &g_frame_pingpong[tx_index ^ 1U];

        spi0_wait_cs_high();
        spi0_slave_stream_reset();
        SetFirstData(((uint8_t *)tx_frame)[0]);
#if CH585_USE_SPI0_SLAVE_DMA
        spi0_slave_dma_trans_arm((uint8_t *)tx_frame, (uint16_t)sizeof(*tx_frame));
        spi0_wait_cs_low();
#if CH585_REQUEST_ONLY_CAPTURE_CMD
        spi0_slave_dma_wait_done_capture(g_req_rx, (uint16_t)sizeof(g_req_rx));
#else
        spi0_slave_dma_wait_done();
#endif
#else
        spi0_wait_cs_low();
        SPI0_SlaveTrans((uint8_t *)tx_frame, (uint16_t)sizeof(*tx_frame));
#endif
        spi0_wait_cs_high();
#if !CH585_STATIC_FRAME_TEST
        next_ack_seq = 0xFFFFU;
        next_frame_flags = CH585_SCAN_FLAG_READY;
#if CH585_REQUEST_ONLY_CAPTURE_CMD && CH585_USE_SPI0_SLAVE_DMA
        memset(&g_cmd, 0, sizeof(g_cmd));
        memcpy(&g_cmd, g_req_rx, sizeof(g_cmd));
        if (scan_cmd_is_valid(&g_cmd) != 0U)
        {
            next_ack_seq = g_cmd.host_seq;
            next_frame_flags |= scan_cmd_apply(&g_cmd);
        }
#endif
        build_scan_or_debug_frame_into(next_frame,
                                       (uint16_t)(seq + 1U),
                                       next_frame_flags,
                                       next_ack_seq);
#endif
#if !CH585_STATIC_FRAME_TEST
        seq++;
        tx_index ^= 1U;
#endif
    }
#else
    while (1)
    {
        spi0_wait_cs_high();

        /*
         * Prepare the key snapshot before the host command arrives. After a
         * valid command we only need to patch ack_seq and CRC, which keeps the
         * command-to-data turnaround short.
         */
        build_scan_frame(seq, CH585_SCAN_FLAG_READY, 0xFFFFU);

        memset(&g_cmd, 0, sizeof(g_cmd));
        spi0_slave_stream_reset();
#if CH585_USE_SPI0_SLAVE_DMA
        spi0_slave_dma_recv_arm((uint8_t *)&g_cmd, (uint16_t)sizeof(g_cmd));
        spi0_wait_cs_low();
        spi0_slave_dma_wait_done();
#else
        spi0_wait_cs_low();
        SPI0_SlaveRecv((uint8_t *)&g_cmd, (uint16_t)sizeof(g_cmd));
#endif
        spi0_wait_cs_high();

        frame_flags = CH585_SCAN_FLAG_READY;
        if (scan_cmd_is_valid(&g_cmd) != 0U)
        {
            ack_host_seq = g_cmd.host_seq;
            frame_flags |= scan_cmd_apply(&g_cmd);
        }
        else
        {
            continue;
        }

        finish_scan_frame(frame_flags, ack_host_seq);
        spi0_slave_stream_reset();
        SetFirstData(((uint8_t *)&g_frame)[0]);
#if CH585_USE_SPI0_SLAVE_DMA
        spi0_slave_dma_trans_arm((uint8_t *)&g_frame, (uint16_t)sizeof(g_frame));
        spi0_wait_cs_low();
        spi0_slave_dma_wait_done();
#else
        spi0_wait_cs_low();
        SPI0_SlaveTrans((uint8_t *)&g_frame, (uint16_t)sizeof(g_frame));
#endif
        seq++;
    }
#endif
#endif
}
